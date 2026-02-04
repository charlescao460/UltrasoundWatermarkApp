//
// Created by CSR on 2026/2/2.
//

#ifndef ULTRASOUNDWATERMARK_WATERMARKCALLEE_HPP
#define ULTRASOUNDWATERMARK_WATERMARKCALLEE_HPP

#include "ase/stream/FormatConversionStream.hpp"
#include "oboe/OboeStreamConsumerPlayer.hpp"
#include "WatermarkDetector.hpp"
#include "KcpServerStreamProducer.hpp"

namespace ase_ultrasound_watermark
{

    class WatermarkCallee
    {
    public:
        constexpr static int PLAYER_CALLBACK_SIZE = 512;
        constexpr static int PLAYER_CALLBACK_BUFFER_SIZE = 64 * PLAYER_CALLBACK_SIZE;

        WatermarkCallee(const std::filesystem::path &param_path, const std::filesystem::path &model_path);

        void StartServer(int play_device_id);

        /// Set callback when the watermark detection result is available
        /// \param callback first float is watermarking probability of current window (instantaneous probability),
        /// second float is probability of current frame (Overall average)
        void SetOnWatermarkResultsCallback(std::function<void(float, float)> callback);

        void Stop();

    private:
        bool is_running_;
        std::mutex state_mutex_;
        std::shared_ptr<ase_android::OboeStreamConsumerPlayer<int16_t>> player_;
        std::shared_ptr<WatermarkDetector> detector_;
        std::shared_ptr<ase::FormatConversionStream<int16_t, float>> converter_;
        std::shared_ptr<ase::FlexibleSizeStreamProducer<int16_t>> model_flex_sizer_;
        std::shared_ptr<KcpServerStreamProducer> server_;

    };

} // ase_ultrasound_watermark

#endif //ULTRASOUNDWATERMARK_WATERMARKCALLEE_HPP
