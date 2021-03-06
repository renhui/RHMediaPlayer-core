//
// Created by maomao on 2020/1/14.
//
#include <jni.h>
#include "JavaCall.h"
#include "FFmpegHandler.h"
#include "FFmpegPlayer.h"
#include <android/native_window_jni.h>
#include <android/native_window.h>
#include <../AndroidLog.h>

#define JNI_CLASS_FFMPEG_PLAYER  "com/renhui/mediaplayer/FFmpegPlayer"

_JavaVM *javaVm = NULL;
JavaCall *javaCall = NULL;
PlayerStatus *playerStatus = NULL;
FFmpegHandler *ffmpegHandler = NULL;

pthread_t thread_start;

static void FFmpegPlayer_prepare(JNIEnv *env, jobject thiz, jstring url_, jboolean is_only_music) {
    const char *url = env->GetStringUTFChars(url_, 0);
    LOGI("Media Url = %s", url);
    if (ffmpegHandler == NULL) {
        if (javaCall == NULL) {
            javaCall = new JavaCall(javaVm, env, &thiz);
        }
        javaCall->onLoad(true);
        playerStatus = new PlayerStatus();
        ffmpegHandler = new FFmpegHandler(playerStatus, javaCall, url, is_only_music);
        ffmpegHandler->preparedFFmpeg();
    }
}

void *startCallBack(void *data) {
    FFmpegHandler *handler = (FFmpegHandler *) data;
    handler->start();
    pthread_exit(&thread_start);
}

static void FFmpegPlayer_start(JNIEnv *env, jobject thiz) {
    if (ffmpegHandler != NULL) {
        pthread_create(&thread_start, NULL, startCallBack, ffmpegHandler);
    }
}

static void FFmpegPlayer_seek(JNIEnv *env, jobject thiz, jint secds) {
    if(ffmpegHandler != NULL) {
        ffmpegHandler->seek(secds);
    }
}

static jlong FFmpegPlayer_dutation(JNIEnv *env, jobject thiz) {
    if(ffmpegHandler != NULL) {
        return ffmpegHandler->getDuration();
    }
}

static void FFmpegPlayer_resume(JNIEnv *env, jobject thiz) {
    if(ffmpegHandler != NULL) {
        ffmpegHandler->resume();
    }
}

static void FFmpegPlayer_setSpeed(JNIEnv *env, jobject thiz, jdouble speed) {
    if (ffmpegHandler != NULL) {
        ffmpegHandler->setSpeed(speed);
    }
}

static double FFmpegPlayer_getSpeed(JNIEnv *env, jobject thiz) {
    if (ffmpegHandler != NULL) {
       return ffmpegHandler->getSpeed();
    }
    return 1;
}

static void FFmpegPlayer_pause(JNIEnv *env, jobject thiz) {
    if(ffmpegHandler != NULL) {
        ffmpegHandler->pause();
    }
}

static void FFmpegPlayer_stop(JNIEnv *env, jobject thiz) {
    if (ffmpegHandler != NULL) {
        ffmpegHandler->stop();
        delete (ffmpegHandler);
        ffmpegHandler = NULL;
        if (javaCall != NULL) {
            delete (javaCall);
            javaCall = NULL;
        }
        if (playerStatus != NULL) {
            delete (playerStatus);
            playerStatus = NULL;
        }
    }
}

static void FFmpegPlayer_release(JNIEnv *env, jobject thiz) {
    if (ffmpegHandler != NULL) {
        ffmpegHandler->release();
        delete (ffmpegHandler);
        ffmpegHandler = NULL;
        if (javaCall != NULL) {
            delete (javaCall);
            javaCall = NULL;
        }
        if (playerStatus != NULL) {
            delete (playerStatus);
            playerStatus = NULL;
        }
    }
}

static JNINativeMethod gMethods[] = {
        {"_start", "()V", (void *) FFmpegPlayer_start},
        {"_resume", "()V", (void *) FFmpegPlayer_resume},
        {"_pause", "()V", (void *) FFmpegPlayer_pause},
        {"_seek", "(I)V", (void *) FFmpegPlayer_seek},
        {"_setSpeed", "(D)V", (void *) FFmpegPlayer_setSpeed},
        {"_getSpeed", "()D", (void *) FFmpegPlayer_getSpeed},
        {"_duration", "()J", (void *) FFmpegPlayer_dutation},
        {"_prepare", "(Ljava/lang/String;Z)V", (void *) FFmpegPlayer_prepare},
        {"_stop", "()V", (void *) FFmpegPlayer_stop},
        {"_release", "()V", (void *) FFmpegPlayer_release}
};

/**
 * Java??????System.loadLibrary()?????????????????????????????????????????????JNI_OnLoad()????????????????????????????????????????????????
???JNI_OnLoad()???????????????????????????
???1?????????JVM?????????????????????????????????JNI???????????????
???2????????????????????????
???3??????JavaVM???????????????????????????????????????????????????????????????JNIEnv?????????

  ??????: JNI_OnLoad?????????????????????????????????????????????
  ???????????????????????????????????????native???????????????????????????????????????????????????
 */
extern "C"
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    jint result = -1;
    javaVm = vm; // ????????????jvm
    JNIEnv *env = NULL;
    if (vm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        return result;
    }
    jclass clazz = env->FindClass(JNI_CLASS_FFMPEG_PLAYER);
    env->RegisterNatives(clazz, gMethods, NELEM(gMethods));
    return JNI_VERSION_1_6;
}


