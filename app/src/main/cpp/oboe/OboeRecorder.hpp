#ifndef LOWLATENCYAUDIOPLAYERRECORDER_OBOERECORDER_H
#define LOWLATENCYAUDIOPLAYERRECORDER_OBOERECORDER_H

#include <cstdint>
#include <exception>
#include <utility>
#include <android/log.h>

#include "OboeStreamAdapter.hpp"
#include "ase/stream/AudioDataStreamProducer.hpp"
#include "ase/stream/FileWriterStreamConsumer.hpp"


namespace ase_android
{
    template<typename SAMPLE_T>
    class OboeRecorder
            : public OboeStreamAdapter<SAMPLE_T>,
              public ase::AudioDataStreamProducer<SAMPLE_T, true>
    {
        using oboeBase = OboeStreamAdapter<SAMPLE_T>;
        using streamBase = ase::AudioDataStreamProducer<SAMPLE_T, true>;
    public:
        explicit OboeRecorder(int32_t device, int32_t sample_rate, int32_t channels, oboe::PerformanceMode mode, int32_t block_size, int32_t num_blocks)
                : OboeStreamAdapter<SAMPLE_T>(device, sample_rate, channels, mode),
                  ase::AudioDataStreamProducer<SAMPLE_T, true>{sample_rate, channels, block_size, num_blocks}
        {
        }

        void start() override
        {
            std::lock_guard<std::recursive_mutex> lock{oboeBase::_oboe_stream_lock};
            const char *error_message = nullptr;
            oboe::Result result{};
            oboe::AudioStreamBuilder builder;
            if (oboeBase::_device_id != oboeBase::DEFAULT_DEVICE_ID)
            {
                builder.setDeviceId(oboeBase::_device_id);
            }
            result = builder
                    .setInputPreset(oboe::InputPreset::Unprocessed)
                    ->setDirection(oboe::Direction::Input)
                    ->setAudioApi(oboeBase::_audio_api) // AAudio has problem on HarmonyOS
                    ->setSharingMode(oboe::SharingMode::Shared)
                    ->setPerformanceMode(oboeBase::_performance_mode)
                    ->setChannelCount(streamBase::_num_channels)
                    ->setChannelConversionAllowed(false)
                    ->setSampleRate(oboeBase::_sample_rate)
                    ->setSampleRateConversionQuality(oboe::SampleRateConversionQuality::None)
                    ->setFormat(oboeBase::getOboeAudioFormat())
                    ->setFormatConversionAllowed(true)
                    ->setDataCallback(this)
                    ->setFramesPerDataCallback(streamBase::_block_size_frames)
                    ->openStream(oboeBase::_oboe_stream);
            if (result != oboe::Result::OK)
            {
                error_message = "Cannot create oboe input stream, error=";
            }

            result = oboeBase::_oboe_stream->requestStart();

            if (result != oboe::Result::OK)
            {
                error_message = "Cannot start oboe input stream, error=";
            }


            if (error_message != nullptr)
            {
                if (oboeBase::_oboe_stream)
                {
                    oboeBase::_oboe_stream->stop();
                    oboeBase::_oboe_stream->close();
                    oboeBase::_oboe_stream.reset();
                }
                throw std::runtime_error(
                        std::string(error_message) +
                        std::to_string(static_cast<int>(result)));
            }
            oboeBase::_frames_written = 0;
        }

        void stop() override
        {
            std::lock_guard<std::recursive_mutex> lock{oboeBase::_oboe_stream_lock};
            oboeBase::stop();
            streamBase::flush();
            oboeBase::_frames_written = 0;
        }

        oboe::DataCallbackResult
        onAudioReady(oboe::AudioStream *audioStream, void *audioData, int32_t numFrames) override
        {
            streamBase::produce(reinterpret_cast<const SAMPLE_T *>(audioData), numFrames);
            oboeBase::setFramesWritten(oboeBase::getFramesWritten() + numFrames);
            return oboe::DataCallbackResult::Continue;
        }

        virtual ~OboeRecorder() override
        {
            stop();
        }

    private:
        static constexpr const char *TAG = "OboeRecorder";

    };
}
#endif //LOWLATENCYAUDIOPLAYERRECORDER_OBOERECORDER_H
