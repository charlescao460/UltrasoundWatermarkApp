package com.csr460.ultrasoundwatermark

fun interface OnWatermarkResultsListener {
    fun onWatermarkResults(instantaneous: Float, average: Float)
}

class WatermarkCallee(paramPath: String, modelPath: String) {
    private var nativePtr: Long = 0

    init {
        nativePtr = nativeCreate(paramPath, modelPath)
    }

    fun startServer(playDeviceId: Int) {
        nativeStartServer(nativePtr, playDeviceId)
    }

    fun setOnWatermarkResultsCallback(listener: OnWatermarkResultsListener) {
        nativeSetOnWatermarkResultsCallback(nativePtr, listener)
    }

    fun stop() {
        nativeStop(nativePtr)
    }

    fun release() {
        nativeDelete(nativePtr)
        nativePtr = 0
    }

    private external fun nativeCreate(paramPath: String, modelPath: String): Long
    private external fun nativeStartServer(nativePtr: Long, playDeviceId: Int)
    private external fun nativeSetOnWatermarkResultsCallback(nativePtr: Long, callback: OnWatermarkResultsListener)
    private external fun nativeStop(nativePtr: Long)
    private external fun nativeDelete(nativePtr: Long)

    companion object {
        init {
            System.loadLibrary("ultrasound_watermark")
        }
    }
}