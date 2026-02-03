#ifndef LOWLATENCYAUDIOPLAYERRECORDER_OBOESTREAMADAPTER_H
#define LOWLATENCYAUDIOPLAYERRECORDER_OBOESTREAMADAPTER_H

#include <functional>
#include <thread>
#include <type_traits>
#include <oboe/Oboe.h>
#include <mutex>
#include "ase/Common.hpp"


namespace ase_android
{

    template<typename SAMPLE_T, typename = typename std::enable_if<std::is_arithmetic<SAMPLE_T>::value, SAMPLE_T>::type>
    class OboeStreamAdapter : public oboe::AudioStreamDataCallback, public oboe::AudioStreamErrorCallback
    {
    public:
        static constexpr int DEFAULT_DEVICE_ID = -1;

        OboeStreamAdapter() = delete;

        OboeStreamAdapter(int32_t device, int32_t sample_rate, int32_t channels, oboe::PerformanceMode mode) :
                _oboe_stream_lock{},
                _device_id{device},
                _sample_rate{sample_rate},
                _num_channels{channels},
                _performance_mode{mode},
                _frames_written{0},
                _audio_api{oboe::AudioApi::Unspecified}
        {
        }

        virtual void start() = 0;

        virtual void stop()
        {
            std::lock_guard<std::recursive_mutex> lock{_oboe_stream_lock};
            if (_oboe_stream)
            {
                _oboe_stream->stop();
                _oboe_stream->close();
                _oboe_stream.reset();
            }
        }

        void setFramesWritten(int64_t frames)
        {
            _frames_written = frames;
        }

        [[nodiscard]] int64_t getFramesWritten() const
        {
            return _frames_written;
        }

        [[nodiscard]] int32_t getSampleRate() const
        {
            if (_oboe_stream)
            {
                return _oboe_stream->getSampleRate();
            }
            return _sample_rate;
        }

        [[nodiscard]] int32_t getNumberOfChannels() const
        {
            if (_oboe_stream)
            {
                return _oboe_stream->getChannelCount();
            }
            return _num_channels;
        }

        [[nodiscard]] int32_t getDeviceId() const
        {
            if (_oboe_stream)
            {
                return _oboe_stream->getDeviceId();
            }
            return _device_id;
        }

        [[nodiscard]] bool isRunning() const
        {
            if (_oboe_stream)
            {
                return _oboe_stream->getState() == oboe::StreamState::Started;
            }
            return false;
        }

        /**
        * Set callback when oboe returned error. Callback will be called on another thread.
        * Callback is called concurrent, meaning multiple callbacks could be executed at the same time.
        * @param callback
        */
        void setOnErrorCallback(std::function<void(oboe::AudioStream *stream)> callback)
        {
            _on_error_callback = std::move(callback);
        }

        void setAudioApi(oboe::AudioApi api)
        {
            _audio_api = api;
        }

    protected:
        std::recursive_mutex _oboe_stream_lock;
        std::shared_ptr<oboe::AudioStream> _oboe_stream;
        const int32_t _device_id;
        const int32_t _sample_rate;
        const int32_t _num_channels;
        const oboe::PerformanceMode _performance_mode;
        int64_t _frames_written;
        std::function<void(oboe::AudioStream *stream)> _on_error_callback;
        oboe::AudioApi _audio_api;

        void onErrorAfterClose(oboe::AudioStream *stream, oboe::Result error) override
        {
            if (_on_error_callback)
            {
                std::thread(_on_error_callback, stream).detach();
            }
        }

        static constexpr oboe::AudioFormat getOboeAudioFormat()
        {
            if constexpr (std::is_integral_v<SAMPLE_T>)
            {
                if constexpr (sizeof(SAMPLE_T) == sizeof(int16_t))
                {
                    return oboe::AudioFormat::I16;
                } else if constexpr (sizeof(SAMPLE_T) == sizeof(int32_t))
                {
                    return oboe::AudioFormat::I32;
                } else if constexpr (sizeof(SAMPLE_T) == 3)
                {
                    return oboe::AudioFormat::I24;
                }
            } else if constexpr (std::is_floating_point_v<SAMPLE_T>)
            {
                if constexpr (sizeof(SAMPLE_T) == sizeof(float))
                {
                    return oboe::AudioFormat::Float;
                }
            }
            return oboe::AudioFormat::Invalid;
        }
    };
}
#endif //LOWLATENCYAUDIOPLAYERRECORDER_OBOESTREAMADAPTER_H
