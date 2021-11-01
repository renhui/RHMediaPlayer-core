#ifndef MEDIAEDITOR_JAVACALL_H
#define MEDIAEDITOR_JAVACALL_H

#include<jni.h>
#include "PlayerStatus.h"

class JavaCall {
public:
    _JavaVM *javaVm = NULL;
    JNIEnv *jniEnv = NULL;
    jmethodID jmid_error;
    jmethodID jmid_load;
    jmethodID jmid_prepared;
    jmethodID jmid_video_param;
    jmethodID jmid_init_mediacodec;
    jmethodID jmid_dec_mediacodec;
    jmethodID jmid_info;
    jmethodID jmid_complete;
    jobject jobj;

public:
    JavaCall(_JavaVM *vm, JNIEnv *env, jobject *obj);

    ~JavaCall();

    void onLoad(bool load);

    void onError(int code, const char *msg);

    void onPrepared();

    void onVideoParam(int width, int height);

    void onVideoInfo(int currentSecond, int totalSecond);

    void onComplete();

    void onInitMediaCodec(int mimetype, int width, int height, int csd_0_size, int csd_1_size, uint8_t * csd_0, uint8_t * csd_1);

    void onMediaCodecData(int size, uint8_t *packet_data, int pts);

};


#endif //MEDIAEDITOR_JAVACALL_H
