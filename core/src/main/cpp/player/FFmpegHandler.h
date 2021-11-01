#ifndef MEDIAEDITOR_FFMPEGHANDLER_H
#define MEDIAEDITOR_FFMPEGHANDLER_H

#include "../AndroidLog.h"
#include "pthread.h"
#include "BasePlayer.h"
#include "JavaCall.h"
#include "AudioHandler.h"
#include "PlayerStatus.h"
#include "AudioChannel.h"
#include "VideoHandler.h"

extern "C" {
#include <libavformat/avformat.h>
}

class FFmpegHandler {

public:
    int mimeType = 1;
    const char *urlpath = NULL;
    JavaCall *javaCall = NULL;
    pthread_t decThread;
    AVFormatContext *pFormatCtx = NULL;//封装格式上下文
    long duration = 0;
    AudioHandler *audioHandler = NULL;
    VideoHandler *videoHandler = NULL;
    PlayerStatus *playerStatus = NULL;
    pthread_mutex_t seek_mutex;
    bool exit = false;
    bool isOnlyMusic = false;
    const AVBitStreamFilter *bsFilter = NULL;

    std::deque<AudioChannel*> audiochannels;
    std::deque<AudioChannel*> videochannels;

    pthread_mutex_t init_mutex;

public:
    FFmpegHandler(PlayerStatus *playerStatus, JavaCall *javaCall, const char *urlpath, bool onlymusic);
    ~FFmpegHandler();
    int preparedFFmpeg();
    int decodeFFmpeg();
    int start();
    long getDuration();
    int getAvCodecContext(AVCodecParameters * parameters, BasePlayer *player);
    void pause();
    void resume();
    void stop();
    void release();
    void seek(int64_t secds);
    void setSpeed(double speed);
    double getSpeed();
    int getMimeType(const char* codecName);
    void setAudioChannel(int id);
    void setVideoChannel(int id);
    int getAudioChannels();
    int getVideoWidth();
    int getVideoHeight();
};

#endif //MEDIAEDITOR_FFMPEGHANDLER_H
