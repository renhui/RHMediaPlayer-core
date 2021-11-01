
#include "FFmpegHandler.h"


void *decodeThread(void *data) {
    FFmpegHandler *ffmpegHandler = (FFmpegHandler *) data;
    ffmpegHandler->decodeFFmpeg();
    pthread_exit(&ffmpegHandler->decThread);
}

int avformat_interrupt_cb(void *ctx) {
    FFmpegHandler *ffmpegHandler = (FFmpegHandler *) ctx;
    if (ffmpegHandler->playerStatus->exit) {
        return AVERROR_EOF;
    }
    return 0;
}

FFmpegHandler::FFmpegHandler(PlayerStatus *ps, JavaCall *jc, const char *up, bool onlymusic) {
    pthread_mutex_init(&init_mutex, NULL);
    pthread_mutex_init(&seek_mutex, NULL);
    isOnlyMusic = onlymusic;
    javaCall = jc;
    playerStatus = ps;
    urlpath = up;
    exit = false;
}

int FFmpegHandler::preparedFFmpeg() {
    pthread_create(&decThread, NULL, decodeThread, this);
    return 0;
}

int FFmpegHandler::start() {
    if (videoHandler == NULL) {
        LOGE("VideoHandler is NULL, Start Failed");
        return -1;
    }
    if (audioHandler == NULL) {
        LOGE("AudioHandler is NULL, Start Failed");
        return -1;
    }
    videoHandler->audioHandler = audioHandler;
    /**
     * 参考文章：FFmpeg为AVPacket添加解码头信息 --> https://cloud.tencent.com/developer/article/1333501。
     * FFmpeg解码获得的AVPacket只包含视频压缩数据，并没有包含相关的解码信息(比如：h264的sps pps头信息，AAC的adts头信息)，
     * 没有这些编码头信息解码器（MediaCodec）是识别不到不能解码的。在FFmpeg中，这些头信息是保存在解码器上下文（AVCodecContext）
     * 的extradata中的，所以我们需要为每一种格式的视频添加相应的解码头信息，这样解码器（MediaCodec）才能正确解析每一个AVPacket里的视频数据。
     */
    const char *codecName = videoHandler->avCodecContext->codec->name;
    // 1. 找到相应解码器的过滤器
    if (strcasecmp(codecName, "h264") == 0) {
        bsFilter = av_bsf_get_by_name("h264_mp4toannexb");
    } else if (strcasecmp(codecName, "h265") == 0) {
        bsFilter = av_bsf_get_by_name("hevc_mp4toannexb");
    }
    if (bsFilter == NULL) {
        return -1;
    }
    // 2. 给过滤器分配内存
    if (av_bsf_alloc(bsFilter, &videoHandler->abs_ctx) != 0) {
        return -1;
    }
    // 3. 添加解码器属性
    if (avcodec_parameters_copy(videoHandler->abs_ctx->par_in, videoHandler->codecpar) < 0) {
        av_bsf_free(&videoHandler->abs_ctx);
        videoHandler->abs_ctx = NULL;
        return -1;
    }
    // 4. 初始化过滤器上下文
    if (av_bsf_init(videoHandler->abs_ctx) != 0) {
        av_bsf_free(&videoHandler->abs_ctx);
        videoHandler->abs_ctx = NULL;
        return -1;
    }
    videoHandler->abs_ctx->time_base_in = videoHandler->time_base;

    if (audioHandler != NULL) {
        audioHandler->playAudio();
    }
    if (videoHandler != NULL) {
        videoHandler->playVideo();
    }

    bool isExitByFinish = false; // 播放完成后退出循环的标识

    while (playerStatus != NULL && !playerStatus->exit) {
        exit = false;
        //暂停
        if (playerStatus->pause) {
            av_usleep(1000 * 100);
            continue;
        }
        if (audioHandler != NULL && audioHandler->playerQueue->getAvPacketSize() > 100) {
            av_usleep(1000 * 100);
            continue;
        }
        if (videoHandler != NULL && videoHandler->playerQueue->getAvPacketSize() > 100) {
            av_usleep(1000 * 100);
            continue;
        }
        AVPacket *packet = av_packet_alloc();
        pthread_mutex_lock(&seek_mutex);
        int ret = av_read_frame(pFormatCtx, packet);
        pthread_mutex_unlock(&seek_mutex);
        if (playerStatus->seek) {
            av_packet_free(&packet);
            av_free(packet);
            continue;
        }
        if (ret == 0) {
            if (audioHandler != NULL && packet->stream_index == audioHandler->streamIndex) {
                audioHandler->playerQueue->putAvPacket(packet);
            } else if (videoHandler != NULL && packet->stream_index == videoHandler->streamIndex) {
                videoHandler->playerQueue->putAvPacket(packet);
            } else {
                av_packet_free(&packet);
                av_free(packet);
                packet = NULL;
            }
        } else {
            av_packet_free(&packet);
            av_free(packet);
            packet = NULL;
            if ((videoHandler != NULL && videoHandler->playerQueue->getAvFrameSize() == 0) ||
                (audioHandler != NULL && audioHandler->playerQueue->getAvPacketSize() == 0)) {
                playerStatus->exit = true;
                isExitByFinish = true;
                break;
            }
        }
    }

    if (javaCall != NULL) {
        if (isExitByFinish) {
            javaCall->onComplete();
        } else {
            LOGE("Stop Play By Java Call Stop");
        }
    }
    exit = true;
    return 0;
}

void FFmpegHandler::seek(int64_t secds) {
    LOGE("seek time %d", secds);
    if (duration <= 0) {
        return;
    }
    if (secds >= 0 && secds <= duration) {
        playerStatus->seek = true;
        pthread_mutex_lock(&seek_mutex);
        int64_t rel = secds * AV_TIME_BASE;
        LOGE("rel time %d", secds);
        avformat_seek_file(pFormatCtx, -1, INT64_MIN, rel, INT64_MAX, 0);
        if (audioHandler != NULL) {
            audioHandler->playerQueue->clearAvPacket();
            audioHandler->clock = 0;
        }
        if (videoHandler != NULL) {
            videoHandler->playerQueue->clearAvFrame();
            videoHandler->playerQueue->clearAvPacket();
            videoHandler->clock = 0;
        }
        audioHandler->now_time = 0;
        pthread_mutex_unlock(&seek_mutex);
        playerStatus->seek = false;
    }
}

void FFmpegHandler::setSpeed(double speed) {
    if (audioHandler != NULL) {
        audioHandler->setSpeed(speed);
    }
}

double FFmpegHandler::getSpeed() {
    if (audioHandler != NULL) {
        return audioHandler->getSpeed();
    }
    return 1;
}

void FFmpegHandler::pause() {
    if (playerStatus != NULL) {
        playerStatus->pause = true;
    }
    if (audioHandler != NULL) {
        audioHandler->pause();
    }
}

void FFmpegHandler::resume() {
    if (playerStatus != NULL) {
        playerStatus->pause = false;
    }
    if (audioHandler != NULL) {
        audioHandler->resume();
    }
}

long FFmpegHandler::getDuration() {
    return duration;
}

int FFmpegHandler::getAudioChannels() {
    return audiochannels.size();
}

void FFmpegHandler::setAudioChannel(int index) {
    if (audioHandler != NULL) {
        int channelsize = audiochannels.size();
        if (index < channelsize) {
            for (int i = 0; i < channelsize; i++) {
                if (i == index) {
                    audioHandler->time_base = audiochannels.at(i)->time_base;
                    audioHandler->streamIndex = audiochannels.at(i)->channelId;
                }
            }
        }
    }
}

// FIXME: 如果过快进入播放页，会导致死锁进而导致ANR，此问题需要修复
int FFmpegHandler::decodeFFmpeg() {
    pthread_mutex_lock(&init_mutex);
    av_register_all();
    avformat_network_init();
    pFormatCtx = avformat_alloc_context();
    LOGE("av format open input");
    if (avformat_open_input(&pFormatCtx, urlpath, NULL, NULL) != 0) {
        LOGE("can not open url:%s", urlpath);
        if (javaCall != NULL) {
            javaCall->onError(FFMPEG_CAN_NOT_OPEN_URL, "can not open url");
        }
        exit = true;
        pthread_mutex_unlock(&init_mutex);
        return -1;
    }
    pFormatCtx->interrupt_callback.callback = avformat_interrupt_cb;
    pFormatCtx->interrupt_callback.opaque = this;
    LOGE("av format find stream info");
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        LOGE("can not find streams from %s", urlpath);
        if (javaCall != NULL) {
            javaCall->onError(FFMPEG_CAN_NOT_FIND_STREAMS, "can not find streams from url");
        }
        exit = true;
        pthread_mutex_unlock(&init_mutex);
        return -1;
    }

    if (pFormatCtx == NULL) {
        exit = true;
        pthread_mutex_unlock(&init_mutex);
        return -1;
    }

    duration = pFormatCtx->duration / 1000;

    LOGE("channel numbers is %d", pFormatCtx->nb_streams);
    int videoStreamIndex = -1;

    for (int i = 0; i < pFormatCtx->nb_streams; i++) {
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) { //音频
            AudioChannel *audioChannel = new AudioChannel(i, pFormatCtx->streams[i]->time_base);
            audiochannels.push_front(audioChannel);
        } else if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) { //视频
            int num = pFormatCtx->streams[i]->avg_frame_rate.num;
            int den = pFormatCtx->streams[i]->avg_frame_rate.den;
            videoStreamIndex = i;
            if (num != 0 && den != 0) {
                int fps = pFormatCtx->streams[i]->avg_frame_rate.num / pFormatCtx->streams[i]->avg_frame_rate.den;
                AudioChannel *audioChannel = new AudioChannel(i, pFormatCtx->streams[i]->time_base, fps);
                videochannels.push_front(audioChannel);
            }
        }
    }
    LOGE("Come Here, audio channel size = %d, video channel size = %d!", audiochannels.size(), videochannels.size());
    if (audiochannels.size() > 0) {
        audioHandler = new AudioHandler(playerStatus, javaCall);
        setAudioChannel(0);
        if (audioHandler->streamIndex >= 0 && audioHandler->streamIndex < pFormatCtx->nb_streams) {
            getAvCodecContext(pFormatCtx->streams[audioHandler->streamIndex]->codecpar, audioHandler);
        }
    }

    if (videochannels.size() > 0) {
        videoHandler = new VideoHandler(javaCall, audioHandler, playerStatus);
        setVideoChannel(0);
        if (videoStreamIndex != -1) {
            videoHandler->codecpar = pFormatCtx->streams[videoStreamIndex]->codecpar;
        }
        if (videoHandler->streamIndex >= 0 && videoHandler->streamIndex < pFormatCtx->nb_streams) {
            getAvCodecContext(pFormatCtx->streams[videoHandler->streamIndex]->codecpar, videoHandler);
        }
    }

    // 判斷音視頻軌道是否符合要求
    if (isOnlyMusic) {
        if (audioHandler == NULL) {
            javaCall->onError(0, "当前资源无音频轨！");
            exit = true;
            pthread_mutex_unlock(&init_mutex);
            return -1;
        }
    } else {
        if (audioHandler == NULL || videoHandler == NULL) {
            javaCall->onError(0, "当前资源音&视频轨有缺失！");
            exit = true;
            pthread_mutex_unlock(&init_mutex);
            return 1;
        }
    }

    if (audioHandler != NULL) {
        audioHandler->duration = pFormatCtx->duration / 1000000;
        audioHandler->sample_rate = audioHandler->avCodecContext->sample_rate;
    }

    if (videoHandler != NULL) {
        LOGE("codec name is %s!", videoHandler->avCodecContext->codec->name);
        mimeType = getMimeType(videoHandler->avCodecContext->codec->name);
        if (mimeType != -1) {
            javaCall->onInitMediaCodec(mimeType, videoHandler->avCodecContext->width, videoHandler->avCodecContext->height,
                                       videoHandler->avCodecContext->extradata_size, videoHandler->avCodecContext->extradata_size,
                                       videoHandler->avCodecContext->extradata, videoHandler->avCodecContext->extradata);
        }
        // 回调Java层，当前的视频宽高
        javaCall->onVideoParam(videoHandler->avCodecContext->width, videoHandler->avCodecContext->height);
        videoHandler->duration = pFormatCtx->duration / 1000000;
    }
    javaCall->onPrepared();
    exit = true;
    pthread_mutex_unlock(&init_mutex);
    return 0;
}

int FFmpegHandler::getAvCodecContext(AVCodecParameters *parameters, BasePlayer *player) {
    AVCodec *dec = avcodec_find_decoder(parameters->codec_id);
    if (!dec) {
        javaCall->onError(3, "get avcodec fail");
        exit = true;
        return 1;
    }
    player->avCodecContext = avcodec_alloc_context3(dec);
    if (!player->avCodecContext) {
        javaCall->onError(4, "alloc avcodecctx fail");
        exit = true;
        return 1;
    }
    if (avcodec_parameters_to_context(player->avCodecContext, parameters) != 0) {
        javaCall->onError(5, "copy avcodecctx fail");
        exit = true;
        return 1;
    }
    if (avcodec_open2(player->avCodecContext, dec, 0) != 0) {
        javaCall->onError(6, "open avcodecctx fail");
        exit = true;
        return 1;
    }
    return 0;
}

void FFmpegHandler::setVideoChannel(int id) {
    if (videoHandler != NULL) {
        videoHandler->streamIndex = videochannels.at(id)->channelId;
        videoHandler->time_base = videochannels.at(id)->time_base;
        videoHandler->rate = 1000 / videochannels.at(id)->fps;
    }
}

int FFmpegHandler::getMimeType(const char *codecName) {
    if (strcmp(codecName, "h264") == 0) {
        return 1;
    }
    if (strcmp(codecName, "hevc") == 0) {
        return 2;
    }
    if (strcmp(codecName, "mpeg4") == 0) {
        return 3;
    }
    if (strcmp(codecName, "wmv3") == 0) {
        return 4;
    }
    return -1;
}

void FFmpegHandler::stop() {
    LOGE("执行FFmpegHandler stop!");
    playerStatus->exit = true;
}

void FFmpegHandler::release() {
    LOGE("执行FFmpegHandler Release方法！");
    pthread_mutex_lock(&init_mutex);
    int sleepCount = 0;
    while (!exit) {
        if (sleepCount > 1000) {
            exit = true;
        }
        LOGE("wait ffmpeg  exit %d", sleepCount);
        sleepCount++;
        av_usleep(1000 * 10);//暂停10毫秒
    }

    if (audioHandler != NULL) {
        audioHandler->release();
        delete (audioHandler);
        audioHandler = NULL;
    }

    if (videoHandler != NULL) {
        videoHandler->release();
        delete (videoHandler);
        videoHandler = NULL;
    }

    if (pFormatCtx != NULL) {
        avformat_close_input(&pFormatCtx);
        avformat_free_context(pFormatCtx);
        pFormatCtx = NULL;
    }

    if (javaCall != NULL) {
        javaCall = NULL;
    }
    pthread_mutex_unlock(&init_mutex);
}

FFmpegHandler::~FFmpegHandler() {
    pthread_mutex_destroy(&init_mutex);
    pthread_mutex_destroy(&seek_mutex);
}
