//
// Created by CSR on 2026/2/2.
//

#ifndef ULTRASOUNDWATERMARK_OBOESTREAMCONSUMERPLAYER_HPP
#define ULTRASOUNDWATERMARK_OBOESTREAMCONSUMERPLAYER_HPP

#include <readerwriterqueue.h>
#include <ase/stream/AudioDataStreamBase.hpp>
#include "OboePlayerBase.hpp"


namespace ase_android
{
    template<typename SAMPLE_T>
    class OboeStreamConsumerPlayer : public OboePlayerBase<SAMPLE_T>, public ase::AudioDataStreamBase<SAMPLE_T>
    {
        using oboeBase = OboePlayerBase<SAMPLE_T>;

    public:
        OboeStreamConsumerPlayer(int32_t device,
                                 int32_t sample_rate,
                                 int32_t channels,
                                 oboe::PerformanceMode mode,
                                 int32_t callback_samples,
                                 int32_t buffer_samples)
                : OboePlayerBase<SAMPLE_T>{device, sample_rate, channels, mode, callback_samples},
                  ase::AudioDataStreamBase<SAMPLE_T>{sample_rate, channels},
                  input_buffer_{static_cast<size_t>(buffer_samples)},
                  input_buffer_samples_{static_cast<size_t>(buffer_samples)},
                  read_position_samples_{0},
                  write_position_samples{0}
        {
        }

        void consume(const SAMPLE_T *samples, size_t size /* size is in SAMPLES */) override
        {
            if (samples == nullptr || size == 0) return;

            if (!oboeBase::isRunning())
            {
                return;
            }

            std::lock_guard lock{spin_lock_};
            if (unlikely(!input_buffer_) || input_buffer_samples_ == 0) return;

            // If incoming chunk larger than capacity, keep the newest tail.
            if (size > input_buffer_samples_)
            {
                samples += (size - input_buffer_samples_);
                size = input_buffer_samples_;
            }

            const size_t capacity = input_buffer_samples_;
            const size_t available = write_position_samples - read_position_samples_;

            // Overflow policy: drop oldest to make room (low latency).
            if (available + size > capacity)
            {
                const size_t to_drop = (available + size) - capacity;
                read_position_samples_ += to_drop;
            }

            // Write into ring using modulo indexing.
            size_t w = write_position_samples % capacity;
            const size_t first = std::min(size, capacity - w);
            std::memcpy(input_buffer_.get() + w, samples, first * sizeof(SAMPLE_T));
            if (size > first)
            {
                std::memcpy(input_buffer_.get(), samples + first, (size - first) * sizeof(SAMPLE_T));
            }

            write_position_samples += size;
        }

        ~OboeStreamConsumerPlayer()
        {
            std::lock_guard lock{spin_lock_};
            input_buffer_.reset();
            read_position_samples_ = 0;
            write_position_samples = 0;
        }

    protected:
        ase::aligned_unique_ptr<SAMPLE_T[]> input_buffer_;
        const size_t input_buffer_samples_;
        ase::SpinLock spin_lock_;
        size_t read_position_samples_;
        size_t write_position_samples;

        // Append N zeros into ring and advance write_position_samples.
        // Keeps monotonic positions, applies same overflow policy (drop oldest).
        void appendZerosLocked(size_t n)
        {

            if (unlikely(n == 0 || !input_buffer_ || input_buffer_samples_ == 0))
            {
                return;
            }

            const size_t capacity = input_buffer_samples_;

            // If asked to append more than capacity, only the last 'capacity' zeros matter.
            if (n > capacity)
            {
                // Clear buffered history; we'll effectively output zeros anyway.
                read_position_samples_ = write_position_samples;
                n = capacity;
            }

            const size_t available = write_position_samples - read_position_samples_;
            if (available + n > capacity)
            {
                const size_t to_drop = (available + n) - capacity;
                read_position_samples_ += to_drop;
            }

            size_t w = write_position_samples % capacity;
            const size_t first = std::min(n, capacity - w);
            std::memset(input_buffer_.get() + w, 0, first * sizeof(SAMPLE_T));
            if (n > first)
            {
                std::memset(input_buffer_.get(), 0, (n - first) * sizeof(SAMPLE_T));
            }

            write_position_samples += n;
        }

        oboe::DataCallbackResult onAudioReady(oboe::AudioStream *audioStream, void *audioData, int32_t numFrames) override
        {
            std::lock_guard lock{spin_lock_};
            if (unlikely(!input_buffer_ || input_buffer_samples_ == 0 || audioData == nullptr || numFrames <= 0))
            {
                return oboe::DataCallbackResult::Stop;
            }

            auto *out = static_cast<SAMPLE_T *>(audioData);

            // Callback counts FRAMES; we need SAMPLES = frames * channels.
            const size_t required_samples = static_cast<size_t>(numFrames) * static_cast<size_t>(oboeBase::_num_channels);

            const size_t available = write_position_samples - read_position_samples_;
            if (available < required_samples)
            {
                appendZerosLocked(required_samples - available);
            }

            // Copy required_samples from ring at read_position_samples_ to output (wrap aware).
            const size_t capacity = input_buffer_samples_;
            size_t r = read_position_samples_ % capacity;
            const size_t first = std::min(required_samples, capacity - r);
            std::memcpy(out, input_buffer_.get() + r, first * sizeof(SAMPLE_T));
            if (required_samples > first)
            {
                std::memcpy(out + first, input_buffer_.get(), (required_samples - first) * sizeof(SAMPLE_T));
            }

            read_position_samples_ += required_samples;

            // Optional reset when empty to avoid unbounded growth.
            if (read_position_samples_ == write_position_samples)
            {
                read_position_samples_ = 0;
                write_position_samples = 0;
            }

            return oboe::DataCallbackResult::Continue;
        }

    };

} // ase_android

#endif //ULTRASOUNDWATERMARK_OBOESTREAMCONSUMERPLAYER_HPP
