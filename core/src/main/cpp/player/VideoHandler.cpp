//
// Created by maomao on 2020/1/22.
//

extern "C" {
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}
#include "VideoHandler.h"


VideoHandler::VideoHandler(JavaCall *jc, AudioHandler *audio, PlayerStatus *ps) {
    streamIndex = -1;
    clock = 0;
    javaCall = jc;
    audioHandler = audio;
    playerQueue = new PlayerQueue(ps);
    playerStatus = ps;
    pthread_mutex_init(&codecMutex, NULL);
}

void *decodeVideoT(void *data) {
    VideoHandler *videoHandler = (VideoHandler *) data;
    videoHandler->decodeVideo();
    pthread_exit(&videoHandler->videoThread);
}

void VideoHandler::release() {
    if (playerStatus != NULL) {
        playerStatus->exit = true;
    }
    if (abs_ctx != NULL) {
        av_bsf_free(&abs_ctx);
        abs_ctx = NULL;
    }
    if (playerQueue != NULL) {
        playerQueue->noticeThread();
    }
    if (playerQueue != NULL) {
        playerQueue->release();
        delete (playerQueue);
        playerQueue = NULL;
    }
    if (javaCall != NULL) {
        javaCall = NULL;
    }
    if (audioHandler != NULL) {
        audioHandler = NULL;
    }
    if (avCodecContext != NULL) {
        pthread_mutex_lock(&codecMutex);
        avcodec_close(avCodecContext);
        avcodec_free_context(&avCodecContext);
        avCodecContext = NULL;
        pthread_mutex_unlock(&codecMutex);
    }
    if (playerStatus != NULL) {
        playerStatus = NULL;
    }
}

void VideoHandler::decodeVideo() {
    while (playerStatus != NULL && !playerStatus->exit) {
        // 判断是否暂停
        if (playerStatus->pause) {
            continue;
        }
        // 加载
        if (playerQueue->getAvPacketSize() == 0) {
            continue;
        }
        AVPacket *packet = av_packet_alloc();
        if(playerQueue->getAvPacket(packet) != 0) {
            av_free(packet->data);
            av_free(packet->buf);
            av_free(packet->side_data);
            packet = NULL;
            continue;
        }
        if (av_bsf_send_packet(abs_ctx, packet) != 0) {
            av_packet_free(&packet);
            av_free(packet);
            packet = NULL;
            continue;
        }
        while (abs_ctx != NULL && packet != NULL && av_bsf_receive_packet(abs_ctx, packet) == 0) {
            double diff = getFrameDiffTime(NULL, packet);
            av_usleep(getDelayTime(diff) * 1000000);
            if (javaCall != NULL) {
                javaCall->onMediaCodecData(packet->size, packet->data, 0);
            }
            av_packet_free(&packet);
            av_free(packet);
            continue;
        }
        packet = NULL;
    }
}

double VideoHandler::getFrameDiffTime(AVFrame *avFrame, AVPacket *avPacket) {
    double pts = 0;
    if (avFrame != NULL) {
        pts = av_frame_get_best_effort_timestamp(avFrame);
    }
    if (avPacket != NULL) {
        pts = avPacket->pts;
    }
    if (pts == AV_NOPTS_VALUE) {
        pts = 0;
    }
    pts *= av_q2d(time_base);
    if (pts > 0) {
        clock = pts;
    }
    double diff = audioHandler->clock - clock;
    return diff;
}

VideoHandler::~VideoHandler() {
    LOGE("VideoHandler析构函数执行");
    pthread_mutex_destroy(&codecMutex);
}

double VideoHandler::getDelayTime(double diff) {
    if (diff > 0.003) {
        delayTime = delayTime * 2 / 3;
        if (delayTime < defaultDelayTime / 2) {
            delayTime = defaultDelayTime * 2 / 3;
        } else if (delayTime > defaultDelayTime * 2) {
            delayTime = defaultDelayTime * 2;
        }
    } else if (diff < -0.003) {
        delayTime = delayTime * 3 / 2;
        if (delayTime < defaultDelayTime / 2) {
            delayTime = defaultDelayTime * 2 / 3;
        } else if (delayTime > defaultDelayTime * 2) {
            delayTime = defaultDelayTime * 2;
        }
    } else if (diff == 0.003) {

    }
    if (diff >= 0.5) {
        delayTime = 0;
    } else if (diff <= -0.5) {
        delayTime = defaultDelayTime * 2;
    }

    if (fabs(diff) >= 10) {
        delayTime = defaultDelayTime;
    }
    return delayTime;
}

void VideoHandler::playVideo() {
    pthread_create(&videoThread, NULL, decodeVideoT, this);
}
