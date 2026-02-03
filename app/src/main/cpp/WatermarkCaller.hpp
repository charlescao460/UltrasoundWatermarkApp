//
// Created by CSR on 2026/2/2.
//

#ifndef ULTRASOUNDWATERMARK_WATERMARKCALLER_HPP
#define ULTRASOUNDWATERMARK_WATERMARKCALLER_HPP

#include "ase/stream/FormatConversionStream.hpp"
#include "oboe/OboeLoopPlayer.hpp"
#include "oboe/OboeRecorder.hpp"
#include "KcpClientStreamConsumer.hpp"
#include "WatermarkGenerator.hpp"

namespace ase_ultrasound_watermark
{
    class WatermarkCaller
    {
    public:
        WatermarkCaller(const std::filesystem::path &param_path, const std::filesystem::path &model_path);

        void StartCall(std::string &host, int play_device_id, int record_device_id);

        void StopCall();

    private:
        constexpr static std::array<int, 6> MULTI_TONE = {16000, 16300, 16600, 16900, 17200, 17500};
        constexpr static float PROBING_SIGNAL_AMP = 0.8;
        constexpr static float PROBING_SIGNAL_DURATION_S = 20;

        bool is_running_;
        std::mutex state_mutex_;
        std::shared_ptr<ase_android::OboeLoopPlayer<int16_t>> player_;
        std::shared_ptr<ase_android::OboeRecorder<int16_t>> recorder_;
        std::shared_ptr<ase::FormatConversionStream<int16_t, float>> converter_in_;
        std::shared_ptr<WatermarkGenerator> generator_;
        std::shared_ptr<ase::FormatConversionStream<float, int16_t>> converter_out_;
        std::shared_ptr<KcpClientStreamConsumer> kcp_client_;
    };

} // ase_ultrasound_watermark

#endif //ULTRASOUNDWATERMARK_WATERMARKCALLER_HPP
