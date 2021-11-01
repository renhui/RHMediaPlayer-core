//
// Created by renhui on 2020-01-12.
//

#ifndef MEDIAEDITOR_PLAYERSTATUS_H
#define MEDIAEDITOR_PLAYERSTATUS_H

#define FFMPEG_CAN_NOT_OPEN_URL 3
#define FFMPEG_CAN_NOT_FIND_STREAMS 4

class PlayerStatus {

public:

    bool exit;

    bool pause;

    bool seek;


public:

    // 构造函数
    PlayerStatus();

    // 析构函数
    ~PlayerStatus();

};

#endif //MEDIAEDITOR_PLAYERSTATUS_H