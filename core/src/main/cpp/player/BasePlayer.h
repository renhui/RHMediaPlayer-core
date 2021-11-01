//
// Created by maomao on 2020/1/14.
//

#ifndef MEDIAEDITOR_BASEPLAYER_H
#define MEDIAEDITOR_BASEPLAYER_H

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/rational.h>
};

class BasePlayer {

public:
    int streamIndex;  // 流索引
    int duration;  // 时长
    double clock = 0;
    double now_time = 0;
    AVCodecContext *avCodecContext = NULL;  // 编解码上下文
    AVRational time_base; // 时间基

public:
    BasePlayer();

    ~BasePlayer();
};

#endif //MEDIAEDITOR_BASEPLAYER_H