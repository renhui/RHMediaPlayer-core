//
// Created by maomao on 2020/1/22.
//

#ifndef MEDIAEDITOR_VIDEOHANDLER_H
#define MEDIAEDITOR_VIDEOHANDLER_H


#include "BasePlayer.h"
#include "PlayerQueue.h"
#include "JavaCall.h"
#include "AudioHandler.h"
#include "../AndroidLog.h"

extern "C" {
#include <libavutil/time.h>
#include <libswscale/swscale.h>
};

class VideoHandler : public BasePlayer {

public:
    AVCodecParameters *codecpar = NULL;
    PlayerQueue *playerQueue = NULL;
    AudioHandler *audioHandler = NULL;
    PlayerStatus *playerStatus = NULL;
    pthread_t videoThread;
    pthread_mutex_t codecMutex;
    JavaCall *javaCall = NULL;

    double delayTime = 0;
    double defaultDelayTime = 0.04;

    int rate = 0;
    AVBSFContext *abs_ctx = NULL;

public:

    VideoHandler(JavaCall *javaCall, AudioHandler *audio, PlayerStatus *playStatus);

    ~VideoHandler();

    void playVideo();

    void decodeVideo();

    void release();

    double getDelayTime(double diff);

    double getFrameDiffTime(AVFrame *avFrame, AVPacket *avPacket);

};

#endif //MEDIAEDITOR_VIDEOHANDLER_H
