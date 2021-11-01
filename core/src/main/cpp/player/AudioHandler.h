//
// Created by maomao on 2020/1/16.
//

#pragma once

#ifndef MEDIAEDITOR_AUDIOHANDLER_H
#define MEDIAEDITOR_AUDIOHANDLER_H

#include "PlayerQueue.h"
#include "PlayerStatus.h"
#include "JavaCall.h"
#include "BasePlayer.h"
#include <../AndroidLog.h>

extern "C" {
#include <libswresample/swresample.h>
#include <libavutil/time.h>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
};

#include "SoundTouch.h"

using namespace soundtouch;

class AudioHandler : public BasePlayer {

public:
    PlayerQueue *playerQueue = NULL;
    PlayerStatus *playerStatus = NULL;
    JavaCall *javaCall = NULL;

    pthread_t audioThread;

    int ret = 0;//函数调用返回结果
    int64_t dst_layout = 0;//重采样为立体声
    int dst_nb_samples = 0;// 计算转换后的sample个数 a * b / c
    int nb = 0;//转换，返回值为转换后的sample个数
    int num = 0;
    uint8_t *buffer = NULL;
    uint8_t *out_buffer = NULL;//buffer 内存区域
    int out_channels = 0;//输出声道数
    int data_size = 0;//buffer大小
    enum AVSampleFormat dst_format;

    // OpenSL
    int pcmsize = 0;
    int sample_rate = 44100;
    bool isExit = false;

    bool isReadPacketFinish = true;
    AVPacket *packet;

    // 引擎接口
    SLObjectItf engineObject = NULL;
    SLEngineItf engineEngine = NULL;

    //混音器
    SLObjectItf outputMixObject = NULL;
    SLEnvironmentalReverbItf outputMixEnvironmentalReverb = NULL;
    SLEnvironmentalReverbSettings reverbSettings = SL_I3DL2_ENVIRONMENT_PRESET_STONECORRIDOR;

    //pcm
    SLObjectItf pcmPlayerObject = NULL;
    SLPlayItf pcmPlayerPlay = NULL;
    SLVolumeItf pcmPlayerVolume = NULL;

    //缓冲器队列接口
    SLAndroidSimpleBufferQueueItf pcmBufferQueue = NULL;

    SoundTouch *soundTouch = NULL;
    SAMPLETYPE *sampleBuffer = NULL;

    double playSpeed;

    bool finished = true;

public:

    AudioHandler(PlayerStatus *ps, JavaCall *jc);

    ~AudioHandler();

    void playAudio();
    int getPcmData(void **pcm);
    int initOpenSL();
    void pause();
    void resume();
    void release();

    void setSpeed(double speed);
    double getSpeed();

    int getSLSampleRate();

    int getSoundTouchData();

};


#endif //MEDIAEDITOR_AUDIOHANDLER_H
