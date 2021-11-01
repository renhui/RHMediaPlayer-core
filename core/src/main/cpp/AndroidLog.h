//
//  android_log.h
//
#pragma once

#ifdef ANDROID

#include <jni.h>
#include <android/log.h>

#define LOGE(format, ...)  __android_log_print(ANDROID_LOG_ERROR, "DEBUG", format, ##__VA_ARGS__)
#define LOGI(format, ...)  __android_log_print(ANDROID_LOG_INFO,  "DEBUG", format, ##__VA_ARGS__)

#else
#define LOGE(format, ...)  printf("renhui" format "\n", ##__VA_ARGS__)
#define LOGI(format, ...)  printf("renhui" format "\n", ##__VA_ARGS__)
#endif
