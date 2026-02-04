//
// Created by CSR on 2026/2/2.
//

#include <ase/utilities/AudioBufferOperations.hpp>
#include <ase/utilities/WaveformGenerator.hpp>
#include "WatermarkCaller.hpp"

using namespace ase;
using namespace ase_android;

namespace ase_ultrasound_watermark
{
    WatermarkCaller::WatermarkCaller(const std::filesystem::path &param_path,
                                     const std::filesystem::path &model_path)
            : is_running_{false}
    {
        converter_in_ = std::make_shared<FormatConversionStream<int16_t, float>>(WatermarkGenerator::INPUT_FS, 1);
        generator_ = std::make_shared<WatermarkGenerator>(param_path, model_path);
        converter_out_ = std::make_shared<FormatConversionStream<float, int16_t>>(WatermarkGenerator::OUTPUT_FS, 1);
    }

    void WatermarkCaller::StartCall(std::string &host, int play_device_id, int record_device_id, const std::filesystem::path &signal_path)
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
        // Create kcp client
        kcp_client_ = std::make_shared<KcpClientStreamConsumer>(WatermarkGenerator::OUTPUT_FS);
        kcp_client_->connect(host);
        // Create new streams if needed
        if (player_ == nullptr || player_->getDeviceId() != play_device_id)
        {
            player_ = std::make_shared<OboeLoopPlayer<int16_t>>(play_device_id, WatermarkGenerator::INPUT_FS, 1,
                                                                oboe::PerformanceMode::None, WatermarkGenerator::INPUT_FS / 2);
        }
        if (recorder_ == nullptr || recorder_->getDeviceId() != record_device_id)
        {
            recorder_ = std::make_shared<OboeRecorder<int16_t>>(record_device_id, WatermarkGenerator::INPUT_FS, 1, oboe::PerformanceMode::LowLatency, WatermarkGenerator::WINDOW_STEP, 16);
        }
        // Connect everything
        recorder_->attachConsumer(converter_in_);
        converter_in_->attachConsumer(generator_);
        generator_->attachConsumer(converter_out_);
        converter_out_->attachConsumer(kcp_client_);
        // Read signal from file
        int fs = 0;
        int ch = 0;
        size_t player_buffer_len = 0;
        auto play_buffer = readBufferFromWavFile<int16_t>(signal_path, fs, ch, player_buffer_len);
        // Start
        player_->start();
        player_->setBuffer(std::move(play_buffer), player_buffer_len);
        recorder_->start();
        is_running_ = true;
    }

    void WatermarkCaller::StopCall()
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
        // Stop audio I/O
        recorder_->stop();
        player_->stop();
        // Disconnect everything
        converter_out_->detachAllConsumers();
        generator_->detachAllConsumers();
        converter_in_->detachAllConsumers();
        recorder_->detachAllConsumers();
        // Release KCP Client
        kcp_client_.reset();
        is_running_ = false;
    }
} // ase_ultrasound_watermark