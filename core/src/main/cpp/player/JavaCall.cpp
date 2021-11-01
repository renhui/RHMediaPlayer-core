#include "JavaCall.h"
#include "malloc.h"
#include "../AndroidLog.h"

JavaCall::JavaCall(_JavaVM *vm, JNIEnv *env, jobject *obj) {
    javaVm = vm;
    jniEnv = env;
    jobj = env->NewGlobalRef(*obj);
    jclass jlz = jniEnv->GetObjectClass(jobj);
    jmid_load = jniEnv->GetMethodID(jlz, "onLoad", "(Z)V");
    jmid_error = jniEnv->GetMethodID(jlz, "onError", "(ILjava/lang/String;)V");
    jmid_prepared = jniEnv->GetMethodID(jlz, "onPrepared", "()V");
    jmid_complete = jniEnv->GetMethodID(jlz, "onComplete", "()V");
    jmid_video_param = jniEnv->GetMethodID(jlz, "onVideoParam", "(II)V");
    jmid_init_mediacodec = jniEnv->GetMethodID(jlz, "onInitMediaCodec", "(III[B[B)V");
    jmid_dec_mediacodec = jniEnv->GetMethodID(jlz, "onMediaCodecData", "([BII)V");
    jmid_info = jniEnv->GetMethodID(jlz, "onVideoInfo", "(II)V");
}

JavaCall::~JavaCall() {

}

/////////////////////////////// 回调在主线程 //////////////////////////////////////////////

void JavaCall::onLoad(bool load) {
    jniEnv->CallVoidMethod(jobj, jmid_load, load);
}

/////////////////////////////// 所有回调均在子线程 //////////////////////////////////////////////

void JavaCall::onPrepared() {
    JNIEnv *jniEnv;
    // 将当前C的子线程和JVM进行绑定关联
    if (javaVm->AttachCurrentThread(&jniEnv, 0) != JNI_OK) {
        return;
    }
    // 在子线程回调方法
    jniEnv->CallVoidMethod(jobj, jmid_prepared);
    // 将当前C的子线程和JVM取消绑定
    javaVm->DetachCurrentThread();
}

void JavaCall::onInitMediaCodec(int mimetype, int width, int height, int csd_0_size, int csd_1_size, uint8_t *csd_0, uint8_t *csd_1) {
    JNIEnv *jniEnv;
    if (javaVm->AttachCurrentThread(&jniEnv, 0) != JNI_OK) {
        return;
    }
    jbyteArray csd0 = jniEnv->NewByteArray(csd_0_size);
    jniEnv->SetByteArrayRegion(csd0, 0, csd_0_size, (jbyte*)csd_0);
    jbyteArray csd1 = jniEnv->NewByteArray(csd_1_size);
    jniEnv->SetByteArrayRegion(csd1, 0, csd_1_size, (jbyte*)csd_1);
    jniEnv->CallVoidMethod(jobj, jmid_init_mediacodec, mimetype, width, height, csd0, csd1);
    jniEnv->DeleteLocalRef(csd0);
    jniEnv->DeleteLocalRef(csd1);
    javaVm->DetachCurrentThread();
}


void JavaCall::onMediaCodecData(int size, uint8_t *packet_data, int pts) {
    JNIEnv *jniEnv;
    if (javaVm->AttachCurrentThread(&jniEnv, 0) != JNI_OK) {
        return;
    }
    jbyteArray data = jniEnv->NewByteArray(size);
    jniEnv->SetByteArrayRegion(data, 0, size, (jbyte*)packet_data);
    jniEnv->CallVoidMethod(jobj, jmid_dec_mediacodec, data, size, pts);
    jniEnv->DeleteLocalRef(data);
    if (javaVm != NULL) {
        javaVm->DetachCurrentThread();
    }
}


void JavaCall::onError(int code, const char *msg) {
    JNIEnv *jniEnv;
    if (javaVm->AttachCurrentThread(&jniEnv, 0) != JNI_OK) {
        return;
    }
    jstring jmsg = jniEnv->NewStringUTF(msg);
    jniEnv->CallVoidMethod(jobj, jmid_error, code, jmsg);
    jniEnv->DeleteLocalRef(jmsg);
    javaVm->DetachCurrentThread();
}

void JavaCall::onComplete() {
    JNIEnv *jniEnv;
    if (javaVm->AttachCurrentThread(&jniEnv, 0) != JNI_OK) {
        return;
    }
    jniEnv->CallVoidMethod(jobj, jmid_complete);
    javaVm->DetachCurrentThread();
}

void JavaCall::onVideoInfo(int currentSecond, int totalSecond) {
    JNIEnv *jniEnv;
    if (javaVm->AttachCurrentThread(&jniEnv, 0) != JNI_OK) {
        return;
    }
    jniEnv->CallVoidMethod(jobj, jmid_info, currentSecond, totalSecond);
    javaVm->DetachCurrentThread();
}

void JavaCall::onVideoParam(int width, int height) {
    JNIEnv *jniEnv;
    if (javaVm->AttachCurrentThread(&jniEnv, nullptr) != JNI_OK) {
        return;
    }
    jniEnv->CallVoidMethod(jobj, jmid_video_param, width, height);
    javaVm->DetachCurrentThread();
}