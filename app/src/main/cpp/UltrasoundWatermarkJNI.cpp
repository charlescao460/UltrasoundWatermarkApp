#include <jni.h>
#include <string>
#include <memory>
#include <filesystem>
#include <map>
#include <stdexcept>

#include "WatermarkCaller.hpp"
#include "WatermarkCallee.hpp"

// for logging
#include <android/log.h>
#define APPNAME "UltrasoundWatermarkJNI"

static JavaVM *g_jvm = nullptr;

void throw_java_exception(JNIEnv *env, const char *message) {
    jclass exception_class = env->FindClass("com/csr460/ultrasoundwatermark/WatermarkNativeException");
    if (exception_class) {
        env->ThrowNew(exception_class, message);
    }
}

jint JNI_OnLoad(JavaVM *vm, void *reserved)
{
    g_jvm = vm;
    return JNI_VERSION_1_6;
}

extern "C"
{

JNIEXPORT jlong JNICALL
Java_com_csr460_ultrasoundwatermark_WatermarkCaller_nativeCreate(JNIEnv *env, jobject thiz, jstring param_path, jstring model_path)
{
    try {
        const char *param_path_str = env->GetStringUTFChars(param_path, nullptr);
        const char *model_path_str = env->GetStringUTFChars(model_path, nullptr);
        auto *caller = new ase_ultrasound_watermark::WatermarkCaller(param_path_str, model_path_str);
        env->ReleaseStringUTFChars(param_path, param_path_str);
        env->ReleaseStringUTFChars(model_path, model_path_str);
        return reinterpret_cast<jlong>(caller);
    } catch (const std::runtime_error& e) {
        throw_java_exception(env, e.what());
        return 0;
    }
}

JNIEXPORT void JNICALL
Java_com_csr460_ultrasoundwatermark_WatermarkCaller_nativeStartCall(JNIEnv *env, jobject thiz, jlong native_ptr, jstring host, jint play_device_id, jint record_device_id, jstring signal_path)
{
    try {
        auto *caller = reinterpret_cast<ase_ultrasound_watermark::WatermarkCaller *>(native_ptr);
        if (caller)
        {
            const char *host_str = env->GetStringUTFChars(host, nullptr);
            std::string host_std_str = host_str;
            const char *signal_path_str = env->GetStringUTFChars(signal_path, nullptr);
            caller->StartCall(host_std_str, play_device_id, record_device_id, signal_path_str);
            env->ReleaseStringUTFChars(host, host_str);
            env->ReleaseStringUTFChars(signal_path, signal_path_str);
        }
    } catch (const std::runtime_error& e) {
        throw_java_exception(env, e.what());
    }
}

JNIEXPORT void JNICALL
Java_com_csr460_ultrasoundwatermark_WatermarkCaller_nativeStopCall(JNIEnv *env, jobject thiz, jlong native_ptr)
{
    try {
        auto *caller = reinterpret_cast<ase_ultrasound_watermark::WatermarkCaller *>(native_ptr);
        if (caller)
        {
            caller->StopCall();
        }
    } catch (const std::runtime_error& e) {
        throw_java_exception(env, e.what());
    }
}

JNIEXPORT void JNICALL
Java_com_csr460_ultrasoundwatermark_WatermarkCaller_nativeDelete(JNIEnv *env, jobject thiz, jlong native_ptr)
{
    auto *caller = reinterpret_cast<ase_ultrasound_watermark::WatermarkCaller *>(native_ptr);
    if (caller)
    {
        delete caller;
    }
}

// WatermarkCallee JNI
struct CalleeCallback
{
    jobject callback_obj;
    jmethodID on_watermark_results;
};

static std::map<ase_ultrasound_watermark::WatermarkCallee*, CalleeCallback*> g_callee_callbacks;

JNIEXPORT jlong JNICALL
Java_com_csr460_ultrasoundwatermark_WatermarkCallee_nativeCreate(JNIEnv *env, jobject thiz, jstring param_path, jstring model_path)
{
    try {
        const char *param_path_str = env->GetStringUTFChars(param_path, nullptr);
        const char *model_path_str = env->GetStringUTFChars(model_path, nullptr);
        auto *callee = new ase_ultrasound_watermark::WatermarkCallee(param_path_str, model_path_str);
        env->ReleaseStringUTFChars(param_path, param_path_str);
        env->ReleaseStringUTFChars(model_path, model_path_str);
        return reinterpret_cast<jlong>(callee);
    } catch (const std::runtime_error& e) {
        throw_java_exception(env, e.what());
        return 0;
    }
}

JNIEXPORT void JNICALL
Java_com_csr460_ultrasoundwatermark_WatermarkCallee_nativeStartServer(JNIEnv *env, jobject thiz, jlong native_ptr, jint play_device_id)
{
    try {
        auto *callee = reinterpret_cast<ase_ultrasound_watermark::WatermarkCallee *>(native_ptr);
        if (callee)
        {
            callee->StartServer(play_device_id);
        }
    } catch (const std::runtime_error& e) {
        throw_java_exception(env, e.what());
    }
}

JNIEXPORT void JNICALL
Java_com_csr460_ultrasoundwatermark_WatermarkCallee_nativeSetOnWatermarkResultsCallback(JNIEnv *env, jobject thiz, jlong native_ptr, jobject callback)
{
    try {
        auto *callee = reinterpret_cast<ase_ultrasound_watermark::WatermarkCallee *>(native_ptr);
        if (callee)
        {
            auto it = g_callee_callbacks.find(callee);
            if (it != g_callee_callbacks.end()) {
                env->DeleteGlobalRef(it->second->callback_obj);
                delete it->second;
                g_callee_callbacks.erase(it);
            }

            auto *callback_holder = new CalleeCallback();
            callback_holder->callback_obj = env->NewGlobalRef(callback);
            jclass callback_class = env->GetObjectClass(callback);
            callback_holder->on_watermark_results = env->GetMethodID(callback_class, "onWatermarkResults", "(FF)V");

            g_callee_callbacks[callee] = callback_holder;

            callee->SetOnWatermarkResultsCallback([callback_holder](float instantaneous, float average) {
                JNIEnv *env;
                int getEnvStat = g_jvm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6);
                if (getEnvStat == JNI_EDETACHED)
                {
                    if (g_jvm->AttachCurrentThread(&env, nullptr) != 0)
                    {
                        __android_log_print(ANDROID_LOG_ERROR, APPNAME, "Failed to attach current thread");
                        return;
                    }
                }
                else if (getEnvStat == JNI_EVERSION)
                {
                    __android_log_print(ANDROID_LOG_ERROR, APPNAME, "Unsupported JNI version");
                    return;
                }

                env->CallVoidMethod(callback_holder->callback_obj, callback_holder->on_watermark_results, instantaneous, average);

                if (getEnvStat == JNI_EDETACHED)
                {
                    g_jvm->DetachCurrentThread();
                }
            });
        }
    } catch (const std::runtime_error& e) {
        throw_java_exception(env, e.what());
    }
}

JNIEXPORT void JNICALL
Java_com_csr460_ultrasoundwatermark_WatermarkCallee_nativeStop(JNIEnv *env, jobject thiz, jlong native_ptr)
{
    try {
        auto *callee = reinterpret_cast<ase_ultrasound_watermark::WatermarkCallee *>(native_ptr);
        if (callee)
        {
            callee->Stop();
        }
    } catch (const std::runtime_error& e) {
        throw_java_exception(env, e.what());
    }
}

JNIEXPORT void JNICALL
Java_com_csr460_ultrasoundwatermark_WatermarkCallee_nativeDelete(JNIEnv *env, jobject thiz, jlong native_ptr)
{
    auto *callee = reinterpret_cast<ase_ultrasound_watermark::WatermarkCallee *>(native_ptr);
    if (callee)
    {
        auto it = g_callee_callbacks.find(callee);
        if (it != g_callee_callbacks.end()) {
            env->DeleteGlobalRef(it->second->callback_obj);
            delete it->second;
            g_callee_callbacks.erase(it);
        }
        delete callee;
    }
}

}