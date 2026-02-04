package com.csr460.ultrasoundwatermark

import android.Manifest
import android.content.Context
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.platform.app.InstrumentationRegistry
import androidx.test.rule.GrantPermissionRule
import org.junit.Assert.assertTrue
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith
import java.io.File
import java.io.FileOutputStream
import java.io.InputStream
import java.util.concurrent.CountDownLatch
import java.util.concurrent.TimeUnit

@RunWith(AndroidJUnit4::class)
class UltrasoundWatermarkJniTest {

    @get:Rule
    var permissionRule: GrantPermissionRule = GrantPermissionRule.grant(Manifest.permission.RECORD_AUDIO)

    private fun copyRawResourceToFile(context: Context, resourceId: Int, file: File) {
        val inputStream: InputStream = context.resources.openRawResource(resourceId)
        val outputStream = FileOutputStream(file)
        inputStream.use { input ->
            outputStream.use { output ->
                input.copyTo(output)
            }
        }
    }

    @Test
    fun testCallerAndCallee() {
        val context = InstrumentationRegistry.getInstrumentation().targetContext

        val callerParamPath = File(context.cacheDir, "generator_param").absolutePath
        val callerModelPath = File(context.cacheDir, "generator_bin").absolutePath
        val calleeParamPath = File(context.cacheDir, "detector_param").absolutePath
        val calleeModelPath = File(context.cacheDir, "detector_bin").absolutePath
        val signalPath = File(context.cacheDir, "multitone.wav").absolutePath

        copyRawResourceToFile(context, R.raw.generator_param, File(callerParamPath))
        copyRawResourceToFile(context, R.raw.generator_bin, File(callerModelPath))
        copyRawResourceToFile(context, R.raw.detector_param, File(calleeParamPath))
        copyRawResourceToFile(context, R.raw.detector_bin, File(calleeModelPath))
        copyRawResourceToFile(context, R.raw.multitone, File(signalPath))

        val caller = WatermarkCaller(callerParamPath, callerModelPath)
        val callee = WatermarkCallee(calleeParamPath, calleeModelPath)

        val latch = CountDownLatch(1)

        callee.setOnWatermarkResultsCallback { instantaneous, average ->
            latch.countDown()
        }

        callee.startServer(0) // Default device
        caller.startCall("localhost", 0, 0, signalPath)

        // Wait for 10 seconds for a callback
        val callbackReceived = latch.await(60, TimeUnit.SECONDS)

        caller.stopCall()
        callee.stop()

        caller.release()
        callee.release()

        assertTrue("Callback should have been received", callbackReceived)
    }
}