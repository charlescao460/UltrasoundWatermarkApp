package com.csr460.ultrasoundwatermark.ui

import android.app.Application
import android.content.Context
import android.content.pm.PackageManager
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import com.csr460.ultrasoundwatermark.R
import com.csr460.ultrasoundwatermark.WatermarkCaller
import com.csr460.ultrasoundwatermark.WatermarkCallee
import com.csr460.ultrasoundwatermark.WatermarkNativeException
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch
import java.io.File
import java.io.FileOutputStream
import java.io.InputStream
import java.net.Inet4Address
import java.net.Inet6Address
import java.net.NetworkInterface

class MainViewModel(application: Application) : AndroidViewModel(application) {

    sealed class InitState {
        object Initializing : InitState()
        object Ready : InitState()
        data class Error(val message: String) : InitState()
    }

    private val _initState = MutableStateFlow<InitState>(InitState.Initializing)
    val initState: StateFlow<InitState> = _initState.asStateFlow()

    private var watermarkCaller: WatermarkCaller? = null
    private var watermarkCallee: WatermarkCallee? = null

    // Callee State
    private val _calleeState = MutableStateFlow(CalleeScreenState())
    val calleeState: StateFlow<CalleeScreenState> = _calleeState.asStateFlow()

    // Caller State
    private val _callerState = MutableStateFlow(CallerScreenState())
    val callerState: StateFlow<CallerScreenState> = _callerState.asStateFlow()

    init {
        viewModelScope.launch(Dispatchers.IO) {
            try {
                initialize()
            } catch (e: Exception) {
                _initState.value = InitState.Error(e.message ?: "Unknown error")
            }
        }
    }

    private fun initialize() {
        val context = getApplication<Application>().applicationContext
        val lastVersionCode = getAppLastVersionCode(context)
        val currentVersionCode = getAppCurrentVersionCode(context)


        copyRawResources(context)
        setAppLastVersionCode(context, currentVersionCode)


        val callerParamPath = File(context.cacheDir, "generator_param").absolutePath
        val callerModelPath = File(context.cacheDir, "generator_bin").absolutePath
        val calleeParamPath = File(context.cacheDir, "detector_param").absolutePath
        val calleeModelPath = File(context.cacheDir, "detector_bin").absolutePath

        watermarkCaller = WatermarkCaller(callerParamPath, callerModelPath)
        watermarkCallee = WatermarkCallee(calleeParamPath, calleeModelPath).apply {
            setOnWatermarkResultsCallback { instantaneous, average ->
                _calleeState.value = _calleeState.value.copy(
                    instantaneousProbability = instantaneous,
                    averageProbability = average
                )
            }
        }
        loadIpAddress()
        loadHostname()
        _initState.value = InitState.Ready
    }

    private fun copyRawResources(context: Context) {
        val rawResourceIds = mapOf(
            R.raw.generator_param to "generator_param",
            R.raw.generator_bin to "generator_bin",
            R.raw.detector_param to "detector_param",
            R.raw.detector_bin to "detector_bin",
            R.raw.multitone to "multitone.wav"
        )
        rawResourceIds.forEach { (resId, filename) ->
            copyRawResourceToFile(context, resId, File(context.cacheDir, filename))
        }
    }

    private fun copyRawResourceToFile(context: Context, resourceId: Int, file: File) {
        val inputStream: InputStream = context.resources.openRawResource(resourceId)
        val outputStream = FileOutputStream(file)
        inputStream.use { input ->
            outputStream.use { output ->
                input.copyTo(output)
            }
        }
    }

    private fun getAppLastVersionCode(context: Context): Long {
        val prefs = context.getSharedPreferences("app_prefs", Context.MODE_PRIVATE)
        return prefs.getLong("version_code", 0)
    }

    private fun setAppLastVersionCode(context: Context, versionCode: Long) {
        val prefs = context.getSharedPreferences("app_prefs", Context.MODE_PRIVATE)
        prefs.edit().putLong("version_code", versionCode).apply()
    }

    private fun getAppCurrentVersionCode(context: Context): Long {
        return try {
            val packageInfo = context.packageManager.getPackageInfo(context.packageName, 0)
            if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.P) {
                packageInfo.longVersionCode
            } else {
                @Suppress("DEPRECATION")
                packageInfo.versionCode.toLong()
            }
        } catch (e: PackageManager.NameNotFoundException) {
            -1
        }
    }

    private fun loadIpAddress() {
        val ipv4Addresses = mutableListOf<String>()
        val ipv6Addresses = mutableListOf<String>()
        try {
            NetworkInterface.getNetworkInterfaces()?.toList()?.forEach { networkInterface ->
                networkInterface.inetAddresses?.toList()?.forEach { inetAddress ->
                    if (!inetAddress.isLoopbackAddress) {
                        if (inetAddress is Inet4Address) {
                            inetAddress.hostAddress?.let { ipv4Addresses.add(it) }
                        } else if (inetAddress is Inet6Address) {
                            inetAddress.hostAddress?.let { ipv6Addresses.add(it.substringBefore('%')) }
                        }
                    }
                }
            }
        } catch (e: Exception) {
            // Handle exceptions in case of network issues
        }
        _calleeState.value = _calleeState.value.copy(
            ipv4Address = ipv4Addresses.joinToString("\n").ifEmpty { "N/A" },
            ipv6Address = ipv6Addresses.joinToString("\n").ifEmpty { "N/A" }
        )
    }

    fun startCallee() {
        viewModelScope.launch(Dispatchers.IO) {
            try {
                watermarkCallee?.startServer(0)
                _calleeState.value = _calleeState.value.copy(isListening = true)
            } catch (e: WatermarkNativeException) {
                _initState.value = InitState.Error(e.message ?: "Unknown native error")
            }
        }
    }

    fun stopCallee() {
        viewModelScope.launch(Dispatchers.IO) {
            try {
                watermarkCallee?.stop()
                _calleeState.value = _calleeState.value.copy(isListening = false)
            } catch (e: WatermarkNativeException) {
                _initState.value = InitState.Error(e.message ?: "Unknown native error")
            }
        }
    }

    private fun loadHostname() {
        val context = getApplication<Application>().applicationContext
        val prefs = context.getSharedPreferences("app_prefs", Context.MODE_PRIVATE)
        val host = prefs.getString("hostname", "localhost") ?: "localhost"
        _callerState.value = _callerState.value.copy(hostname = host)
    }

    fun onHostNameChange(hostname: String) {
        _callerState.value = _callerState.value.copy(hostname = hostname)
        val context = getApplication<Application>().applicationContext
        val prefs = context.getSharedPreferences("app_prefs", Context.MODE_PRIVATE)
        prefs.edit().putString("hostname", hostname).apply()
    }

    fun startCaller() {
        viewModelScope.launch(Dispatchers.IO) {
            try {
                val context = getApplication<Application>().applicationContext
                val signalPath = File(context.cacheDir, "multitone.wav").absolutePath
                watermarkCaller?.startCall(_callerState.value.hostname, 0, 0, signalPath)
                _callerState.value = _callerState.value.copy(isCalling = true)
            } catch (e: WatermarkNativeException) {
                _initState.value = InitState.Error(e.message ?: "Unknown native error")
            }
        }
    }

    fun stopCaller() {
        viewModelScope.launch(Dispatchers.IO) {
            try {
                watermarkCaller?.stopCall()
                _callerState.value = _callerState.value.copy(isCalling = false)
            } catch (e: WatermarkNativeException) {
                _initState.value = InitState.Error(e.message ?: "Unknown native error")
            }
        }
    }

    override fun onCleared() {
        super.onCleared()
        watermarkCaller?.release()
        watermarkCallee?.release()
    }
}

data class CalleeScreenState(
    val isListening: Boolean = false,
    val ipv4Address: String = "",
    val ipv6Address: String = "",
    val instantaneousProbability: Float = 0.0f,
    val averageProbability: Float = 0.0f,
    val probabilityThreshold: Float = 0.2f
)

data class CallerScreenState(
    val isCalling: Boolean = false,
    val hostname: String = "localhost"
)