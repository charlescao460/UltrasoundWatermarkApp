package com.csr460.ultrasoundwatermark

class WatermarkCaller(paramPath: String, modelPath: String) {
    private var nativePtr: Long = 0

    init {
        nativePtr = nativeCreate(paramPath, modelPath)
    }

    fun startCall(host: String, playDeviceId: Int, recordDeviceId: Int) {
        nativeStartCall(nativePtr, host, playDeviceId, recordDeviceId)
    }

    fun stopCall() {
        nativeStopCall(nativePtr)
    }

    fun release() {
        nativeDelete(nativePtr)
        nativePtr = 0
    }

    private external fun nativeCreate(paramPath: String, modelPath: String): Long
    private external fun nativeStartCall(nativePtr: Long, host: String, playDeviceId: Int, recordDeviceId: Int)
    private external fun nativeStopCall(nativePtr: Long)
    private external fun nativeDelete(nativePtr: Long)

    companion object {
        init {
            System.loadLibrary("ultrasound_watermark")
        }
    }
}