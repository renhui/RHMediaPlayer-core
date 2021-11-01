//
// Created by maomao on 2020/1/14.
//

#ifndef MEDIAEDITOR_AUDIOCHANNEL_H
#define MEDIAEDITOR_AUDIOCHANNEL_H

extern "C" {
#include <libavutil/rational.h>
};

class AudioChannel {
public:
    int channelId;    //  声道ID
    AVRational time_base;   // 时间基
    int fps;   // 帧率

public:
    AudioChannel(int id, AVRational base);

    AudioChannel(int id, AVRational base, int f);

    ~AudioChannel();
};

#endif //MEDIAEDITOR_AUDIOCHANNEL_H
