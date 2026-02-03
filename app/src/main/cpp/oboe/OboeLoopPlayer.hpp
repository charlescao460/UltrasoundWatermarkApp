#ifndef LOWLATENCYAUDIOPLAYERRECORDER_OBOEPLAYER_H
#define LOWLATENCYAUDIOPLAYERRECORDER_OBOEPLAYER_H

#include <cstdint>
#include <mutex>
#include <memory>
#include <thread>
#include <exception>
#include <utility>
#include <algorithm>

#include <oboe/Oboe.h>
#include <android/log.h>
#include <ase/utilities/SpinLock.hpp>
#include <ase/Common.hpp>
#include "OboePlayerBase.hpp"

namespace ase_android
{

    template<typename SAMPLE_T>
    class OboeLoopPlayer : public OboePlayerBase<SAMPLE_T>
    {
        using base = OboePlayerBase<SAMPLE_T>;
    public:
        explicit OboeLoopPlayer(int32_t device, int32_t sample_rate, int32_t channels, oboe::PerformanceMode mode, int32_t callback_samples)
                : OboePlayerBase<SAMPLE_T>(device, sample_rate, channels, mode, callback_samples),
                  _zeros{ase::simd_new_array_raw<SAMPLE_T>(base::frames_per_callback_ * base::_num_channels), std::default_delete<SAMPLE_T[]>()}
        {
            std::fill(_zeros.get(), _zeros.get() + base::frames_per_callback_ * base::_num_channels, 0);
            _buffer.data = _zeros;
            _buffer.size_in_frames = base::frames_per_callback_;
            _buffer.read_position_in_frames = 0;
        }


        void start() override
        {
            _buffer.mutex.lock();
            _buffer.data = _zeros;
            _buffer.size_in_frames = base::frames_per_callback_;
            _buffer.read_position_in_frames = 0;
            _buffer.mutex.unlock();
            base::start();
        }

        /**
         * Set the content to be played. Call clearBuffer() to remove the content.
         * Call stop() will also remove the content
         * @param ptr Pointer to the audio content (buffer). The ownership of this pointer will be transferred
         * @param buffer_size_frames The size of the supplied buffer, in terms of frames.
         */
        void setBuffer(ase::aligned_unique_ptr<SAMPLE_T[]> &&ptr, size_t buffer_size_frames)
        {
            std::lock_guard lock{_buffer.mutex};
            // __cpp_lib_shared_ptr_arrays has not been migrated in to NDK's clang.
            // Therefore, the moving constructor from unique_ptr to shared_ptr is not implemented. We have to use this workaround.
            SAMPLE_T *raw_ptr = ptr.release();
            _buffer.data = std::shared_ptr<SAMPLE_T>(raw_ptr, ptr.get_deleter());
            _buffer.size_in_frames = buffer_size_frames;
            _buffer.read_position_in_frames = 0;
        }

        /**
         * Clear the content set by setBuffer(). But the underlying low-level stream status is unchanged.
         * If underlying low-level stream is running, it will play samples of zeros.
         *
         * To shutdown the stream, call stop()
         */
        void clearBuffer()
        {
            std::lock_guard lock{_buffer.mutex};
            _buffer.size_in_frames = base::frames_per_callback_;
            _buffer.data = _zeros;
            _buffer.read_position_in_frames = 0;
        }

        /**
         * Stop the underlying low-level stream started by start(). The content set through setBuffer()
         * will also be removed.
         */
        void stop() override
        {
            base::stop();
            std::lock_guard lock{_buffer.mutex};
            _buffer.size_in_frames = base::frames_per_callback_;
            _buffer.data = _zeros;
            _buffer.read_position_in_frames = 0;
        }

        oboe::DataCallbackResult
        onAudioReady(oboe::AudioStream *audioStream, void *audioData, int32_t numFrames) override
        {
            std::lock_guard lock{_buffer.mutex};
            if (unlikely(!_buffer.data)) // For safe destruction
            {
                return oboe::DataCallbackResult::Stop;
            }
            for (int32_t frames_left = numFrames; frames_left > 0;)
            {
                int32_t buffer_left = _buffer.size_in_frames - _buffer.read_position_in_frames;
                const SAMPLE_T *const start = _buffer.data.get() + _buffer.read_position_in_frames * base::_num_channels;
                if (likely(buffer_left > numFrames))
                {
                    const SAMPLE_T *const end = start + numFrames * base::_num_channels;
                    std::copy(start, end, reinterpret_cast<SAMPLE_T *>(audioData));
                    frames_left -= numFrames;
                    base::setFramesWritten(base::getFramesWritten() + numFrames);
                    _buffer.read_position_in_frames += numFrames;
                } else
                {
                    const SAMPLE_T *const end = start + buffer_left * base::_num_channels;
                    std::copy(start, end, reinterpret_cast<SAMPLE_T *>(audioData));
                    frames_left -= buffer_left;
                    base::setFramesWritten(base::getFramesWritten() + buffer_left);
                    _buffer.read_position_in_frames = 0;
                }
            }
            return oboe::DataCallbackResult::Continue;
        }


        virtual ~OboeLoopPlayer() override
        {
            stop();
        }

    protected:
        struct
        {
            std::shared_ptr<SAMPLE_T> data;
            int size_in_frames;
            int read_position_in_frames;
            ase::SpinLock mutex;
        } _buffer;
        std::shared_ptr<SAMPLE_T> _zeros;
    };
}
#endif //LOWLATENCYAUDIOPLAYERRECORDER_OBOEPLAYER_H
