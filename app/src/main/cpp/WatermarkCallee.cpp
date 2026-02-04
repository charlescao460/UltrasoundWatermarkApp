//
// Created by CSR on 2026/2/2.
//

#include "WatermarkCallee.hpp"

using namespace ase;
using namespace ase_android;

namespace ase_ultrasound_watermark
{
    WatermarkCallee::WatermarkCallee(const std::filesystem::path &param_path, const std::filesystem::path &model_path)
            : is_running_{false}
    {
        detector_ = std::make_shared<WatermarkDetector>(param_path, model_path);
        converter_ = std::make_shared<FormatConversionStream<int16_t, float>>(WatermarkDetector::INPUT_FS, 1);
        model_flex_sizer_ = std::make_shared<FlexibleSizeStreamProducer<int16_t>>(WatermarkDetector::INPUT_FS, 1, WatermarkDetector::WINDOW_STEP, 16);
    }

    void WatermarkCallee::StartServer(int play_device_id)
    {
        if (!state_mutex_.try_lock())
        {
            return;
        }
        std::lock_guard lock{state_mutex_, std::adopt_lock};
        if (is_running_)
        {
            return;
        }
        server_ = std::make_shared<KcpServerStreamProducer>(WatermarkDetector::INPUT_FS, KcpServerStreamProducer::L3_MTU / sizeof(float) + 1, 32);

        player_ = std::make_shared<OboeStreamConsumerPlayer<int16_t>>(
                play_device_id,
                WatermarkDetector::INPUT_FS,
                1,
                oboe::PerformanceMode::LowLatency,
                PLAYER_CALLBACK_SIZE,
                PLAYER_CALLBACK_BUFFER_SIZE
        );
        player_->start();
        // Connect everything
        server_->attachConsumer(player_);
        server_->attachConsumer(model_flex_sizer_);
        model_flex_sizer_->attachConsumer(converter_);
        converter_->attachConsumer(detector_);
        is_running_ = true;
    }

    void WatermarkCallee::Stop()
    {
        if (!state_mutex_.try_lock())
        {
            return;
        }
        std::lock_guard lock{state_mutex_, std::adopt_lock};
        if (!is_running_)
        {
            return;
        }
        player_->stop();
        server_.reset();
        player_.reset();
        model_flex_sizer_->detachAllConsumers();
        converter_->detachAllConsumers();
        is_running_ = false;
    }

    void WatermarkCallee::SetOnWatermarkResultsCallback(std::function<void(float, float)> callback)
    {
        std::lock_guard lock{state_mutex_};
        detector_->setCallback(callback);
    }
} // ase_ultrasound_watermark