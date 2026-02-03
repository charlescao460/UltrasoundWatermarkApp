//
// Created by CSR on 2026/2/2.
//

#ifndef ULTRASOUNDWATERMARK_OBOEPLAYERBASE_HPP
#define ULTRASOUNDWATERMARK_OBOEPLAYERBASE_HPP

#include <oboe/Oboe.h>
#include <android/log.h>
#include <ase/utilities/SpinLock.hpp>
#include <ase/Common.hpp>
#include "OboeStreamAdapter.hpp"

namespace ase_android
{
    template<typename SAMPLE_T>
    class OboePlayerBase : public OboeStreamAdapter<SAMPLE_T>
    {
        using base = OboeStreamAdapter<SAMPLE_T>;
    public:
        OboePlayerBase(int32_t device, int32_t sample_rate, int32_t channels, oboe::PerformanceMode mode, int32_t callback_samples)
                : OboeStreamAdapter<SAMPLE_T>{device, sample_rate, channels, mode},
                  frames_per_callback_{callback_samples}
        {
        }

        /**
        * Start the audio stream to the low-level Audio API. After calling start(), a low-level stream will
        * start and playing samples of zeros (equivalent to mute). Call setBuffer() to set the playing contents.
        * The purpose of playing zeros is to avoid start-up delay.
        *
        * Call stop() to stop the low-level stream.
        */
        void start() override
        {
            std::lock_guard<std::recursive_mutex> lock{base::_oboe_stream_lock};
            const char *error_message = nullptr;
            oboe::AudioStreamBuilder builder;
            if (base::_device_id != base::DEFAULT_DEVICE_ID)
            {
                builder.setDeviceId(base::_device_id);
            }

            auto result = builder
                    .setDirection(oboe::Direction::Output)
                    ->setContentType(oboe::ContentType::Music)
                    ->setUsage(oboe::Usage::Media)
                    ->setSharingMode(oboe::SharingMode::Shared)
                    ->setAudioApi(base::_audio_api)
                            // LowLatency or PowerSaving on some phones cause glitch due to limited CPU resource
                    ->setPerformanceMode(base::_performance_mode)
                    ->setChannelCount(base::_num_channels)
                    ->setSampleRate(base::_sample_rate)
                    ->setSampleRateConversionQuality(oboe::SampleRateConversionQuality::None)
                    ->setFormat(base::getOboeAudioFormat())
                    ->setDataCallback(this)
                    ->setFramesPerDataCallback(frames_per_callback_)
                    ->openStream(base::_oboe_stream);

            if (result != oboe::Result::OK)
            {
                error_message = "Cannot create oboe output stream, error=";
            }

            result = base::_oboe_stream->requestStart();

            if (result != oboe::Result::OK)
            {
                error_message = "Cannot start oboe output stream, error=";
            }

            if (error_message != nullptr)
            {
                if (base::_oboe_stream)
                {
                    base::_oboe_stream->stop();
                    base::_oboe_stream->close();
                    base::_oboe_stream.reset();
                }
                throw std::runtime_error(
                        std::string(error_message) +
                        std::to_string(static_cast<int>(result)));
            }
            base::setFramesWritten(0);
        }

    protected:
        const int frames_per_callback_;
    };

} // ase_android

#endif //ULTRASOUNDWATERMARK_OBOEPLAYERBASE_HPP
