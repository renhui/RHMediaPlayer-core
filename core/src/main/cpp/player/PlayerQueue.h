//
// Created by maomao on 2020/1/15.
//

#ifndef MEDIAEDITOR_PLAYERQUEUE_H
#define MEDIAEDITOR_PLAYERQUEUE_H


#include "queue"
#include "PlayerStatus.h"


extern "C" {
#include "../AndroidLog.h"
#include <libavcodec/avcodec.h>
#include "pthread.h"
};

class PlayerQueue {


public:

    std::queue<AVPacket*> queuePacket;
    std::queue<AVFrame*> queueFrame;

    pthread_cond_t condPacket;
    pthread_cond_t condFrame;
    pthread_mutex_t mutexPacket;
    pthread_mutex_t mutexFrame;

    PlayerStatus *playerStatus = NULL;


public:

    PlayerQueue(PlayerStatus *status);

    ~PlayerQueue();

    int putAvPacket(AVPacket *avPacket);
    int getAvPacket(AVPacket *avPacket);
    int getAvPacketSize();
    int clearAvPacket();

    int putAvFrame(AVFrame *avFrame);
    int getAvFrame(AVFrame *avFrame);
    int getAvFrameSize();
    int clearAvFrame();
    int clearToKeyFrame();

    int noticeThread();

    void release();

};


#endif //MEDIAEDITOR_PLAYERQUEUE_H
