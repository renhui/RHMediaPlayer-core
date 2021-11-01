//
// Created by maomao on 2020/1/16.
//

#include "AudioHandler.h"

AudioHandler::AudioHandler(PlayerStatus *ps, JavaCall *jc) {
    streamIndex = -1;
    out_buffer = (uint8_t *) malloc(sample_rate * 2 * 2 * 2 / 3);
    buffer = (uint8_t *) av_malloc(sample_rate * 2 * 2);
    playerQueue = new PlayerQueue(ps);
    playerStatus = ps;
    javaCall = jc;
    dst_format = AV_SAMPLE_FMT_S16;

    sampleBuffer = static_cast<SAMPLETYPE *>(malloc(sample_rate * 2 * 2));
    soundTouch = new SoundTouch();

    soundTouch->setSampleRate(sample_rate);
    soundTouch->setChannels(2);
    // 音调
    soundTouch->setPitch(1);
    // 速率
    soundTouch->setTempo(1);
}

AudioHandler::~AudioHandler() {
    LOGE("AudioHandler析构函数执行");
}

void *audioPlayThread(void *context) {
    AudioHandler *audio = (AudioHandler *) context;
    audio->initOpenSL();
    pthread_exit(&audio->audioThread);
}

void AudioHandler::playAudio() {
    pthread_create(&audioThread, NULL, audioPlayThread, this);
}

// 设置倍速
void AudioHandler::setSpeed(double speed) {
    if (!playerStatus->exit && soundTouch) {
        playSpeed = speed;
        soundTouch->setTempo(playSpeed);
    }
}

double AudioHandler::getSpeed() {
    return playSpeed;
}

int AudioHandler::getPcmData(void **pcmbuf) {
    while (!playerStatus->exit) {
        isExit = false;

        // 判断当前播放状态是否是暂停的，如果是，就执行暂停相关逻辑
        if (playerStatus->pause) {
            av_usleep(1000 * 100);
            continue;
        }

        // ？？此逻辑需要再梳理
        if (playerStatus->seek) {
            isReadPacketFinish = true;
            continue;
        }

        // 判断播放器队列是否有数据，如果没有就继续循环，不往下执行，等待当前packet队列有数据
        if (playerQueue->getAvPacketSize() == 0) {
            continue;
        }

        if (isReadPacketFinish) {
            isReadPacketFinish = false;
            packet = av_packet_alloc();
            if (playerQueue->getAvPacket(packet) != 0) {
                av_packet_free(&packet);
                av_free(packet);
                packet = NULL;
                isReadPacketFinish = true;
                continue;
            }
            ret = avcodec_send_packet(avCodecContext, packet);
            if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
                av_packet_free(&packet);
                av_free(packet);
                packet = NULL;
                isReadPacketFinish = true;
                continue;
            }
        }

        AVFrame *frame = av_frame_alloc();
        if (avcodec_receive_frame(avCodecContext, frame) == 0) {
            // 设置通道数或channel_layout
            if (frame->channels > 0 && frame->channel_layout == 0)
                frame->channel_layout = av_get_default_channel_layout(frame->channels);
            else if (frame->channels == 0 && frame->channel_layout > 0)
                frame->channels = av_get_channel_layout_nb_channels(frame->channel_layout);

            SwrContext *swr_ctx;
            //重采样为立体声
            dst_layout = AV_CH_LAYOUT_STEREO;
            // 设置转换参数
            swr_ctx = swr_alloc_set_opts(NULL, dst_layout, dst_format, frame->sample_rate,
                                         frame->channel_layout, (enum AVSampleFormat) frame->format,
                                         frame->sample_rate, 0, NULL);
            if (!swr_ctx || (ret = swr_init(swr_ctx)) < 0) {
                av_frame_free(&frame);
                av_free(frame);
                frame = NULL;
                swr_free(&swr_ctx);
                av_packet_free(&packet);
                av_free(packet);
                packet = NULL;
                continue;
            }
            // 计算转换后的sample个数 a * b / c
            dst_nb_samples = av_rescale_rnd(swr_get_delay(swr_ctx, frame->sample_rate) + frame->nb_samples,
                    frame->sample_rate, frame->sample_rate, AV_ROUND_INF);
            // 转换，返回值为转换后的sample个数
            nb = swr_convert(swr_ctx, &buffer, dst_nb_samples, (const uint8_t **) frame->data, frame->nb_samples);

            //根据布局获取声道数
            out_channels = av_get_channel_layout_nb_channels(dst_layout);
            data_size = out_channels * nb * av_get_bytes_per_sample(dst_format);
            now_time = frame->pts * av_q2d(time_base);
            if (now_time < clock) {
                now_time = clock;
            }
            clock = now_time;
            *pcmbuf = buffer;
            av_frame_free(&frame);
            av_free(frame);
            frame = NULL;
            swr_free(&swr_ctx);
            break;
        } else {
            isReadPacketFinish = true;
            av_frame_free(&frame);
            av_free(frame);
            frame = NULL;
            av_packet_free(&packet);
            av_free(packet);
            packet = NULL;
            continue;
        }
    }
    isExit = true;
    return data_size;
}


void AudioHandler::pause() {
    if (pcmPlayerPlay != NULL) {
        (*pcmPlayerPlay)->SetPlayState(pcmPlayerPlay, SL_PLAYSTATE_PAUSED);
    }
}

void AudioHandler::resume() {
    if (pcmPlayerPlay != NULL) {
        (*pcmPlayerPlay)->SetPlayState(pcmPlayerPlay, SL_PLAYSTATE_PLAYING);
    }
}

int AudioHandler::getSLSampleRate() {
    switch (sample_rate) {
        case 8000:
            return SL_SAMPLINGRATE_8;
        case 11025:
            return SL_SAMPLINGRATE_11_025;
        case 12000:
            return SL_SAMPLINGRATE_12;
        case 16000:
            return SL_SAMPLINGRATE_16;
        case 22050:
            return SL_SAMPLINGRATE_22_05;
        case 24000:
            return SL_SAMPLINGRATE_24;
        case 32000:
            return SL_SAMPLINGRATE_32;
        case 44100:
            return SL_SAMPLINGRATE_44_1;
        case 48000:
            return SL_SAMPLINGRATE_48;
        case 64000:
            return SL_SAMPLINGRATE_64;
        case 88200:
            return SL_SAMPLINGRATE_88_2;
        case 96000:
            return SL_SAMPLINGRATE_96;
        case 192000:
            return SL_SAMPLINGRATE_192;
        default:
            return SL_SAMPLINGRATE_44_1;
    }
}

int AudioHandler::getSoundTouchData() {
    while (playerStatus != NULL && !playerStatus->exit) {
        out_buffer = NULL;
        if (finished) {
            finished = false;
            data_size = getPcmData(reinterpret_cast<void **>(&out_buffer));
            if (data_size > 0) {
                for (int i = 0; i < data_size / 2 + 1; i++) {
                    sampleBuffer[i] = (out_buffer[i * 2] | out_buffer[i * 2 + 1] << 8);
                }
                soundTouch->putSamples(sampleBuffer, nb);
                num = soundTouch->receiveSamples(sampleBuffer, data_size / 4);
            } else {
                soundTouch->flush();
            }
        }
        if (num == 0) {
            finished = true;
            continue;
        } else {
            if (out_buffer == NULL) {
                num = soundTouch->receiveSamples(sampleBuffer, data_size / 4);
                if (num == 0) {
                    finished = true;
                    continue;
                }
            }
            return num;
        }
    }
    return 0;
}

void pcmBufferCallBack(SLAndroidSimpleBufferQueueItf bf, void *context) {
    AudioHandler *audioHandler = (AudioHandler *) context;
    if (audioHandler != NULL) {
        audioHandler->pcmsize = audioHandler->getSoundTouchData();
        if (audioHandler->sampleBuffer && audioHandler->pcmsize > 0) {
            audioHandler->clock +=audioHandler->pcmsize / ((double) (audioHandler->sample_rate * 2 * 2));
            audioHandler->javaCall->onVideoInfo(audioHandler->clock, audioHandler->duration);
            (*audioHandler->pcmBufferQueue)->Enqueue(audioHandler->pcmBufferQueue, audioHandler->sampleBuffer, audioHandler->pcmsize * 2 * 2);
        }
    }
}

int AudioHandler::initOpenSL() {
    LOGI("Run initOpenSL Method！");
    SLresult result;
    result = slCreateEngine(&engineObject, 0, 0, 0, 0, 0);
    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine);

    //第二步，创建混音器
    const SLInterfaceID mids[1] = {SL_IID_ENVIRONMENTALREVERB};
    const SLboolean mreq[1] = {SL_BOOLEAN_FALSE};
    result = (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 1, mids, mreq);
    (void) result;
    result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
    (void) result;
    result = (*outputMixObject)->GetInterface(outputMixObject, SL_IID_ENVIRONMENTALREVERB,
                                              &outputMixEnvironmentalReverb);
    if (SL_RESULT_SUCCESS == result) {
        result = (*outputMixEnvironmentalReverb)->SetEnvironmentalReverbProperties(
                outputMixEnvironmentalReverb, &reverbSettings);
        (void) result;
    }
    SLDataLocator_OutputMix outputMix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};
    SLDataSink audioSnk = {&outputMix, 0};


    // 第三步，配置PCM格式信息
    SLDataLocator_AndroidSimpleBufferQueue android_queue = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
                                                            2};

    SLDataFormat_PCM pcm = {SL_DATAFORMAT_PCM,//播放pcm格式的数据
                            2,//2个声道（立体声）
                            static_cast<SLuint32>(getSLSampleRate()),//44100hz的频率
                            SL_PCMSAMPLEFORMAT_FIXED_16,//位数 16位
                            SL_PCMSAMPLEFORMAT_FIXED_16,//和位数一致就行
                            SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,//立体声（前左前右）
                            SL_BYTEORDER_LITTLEENDIAN//结束标志
    };
    SLDataSource slDataSource = {&android_queue, &pcm};

    const SLInterfaceID ids[3] = {SL_IID_BUFFERQUEUE, SL_IID_EFFECTSEND, SL_IID_VOLUME};
    const SLboolean req[3] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};

    result = (*engineEngine)->CreateAudioPlayer(engineEngine, &pcmPlayerObject, &slDataSource,
                                                &audioSnk, 3, ids, req);

    // 初始化播放器
    (*pcmPlayerObject)->Realize(pcmPlayerObject, SL_BOOLEAN_FALSE);

    // 得到接口后调用  获取Player接口
    (*pcmPlayerObject)->GetInterface(pcmPlayerObject, SL_IID_PLAY, &pcmPlayerPlay);

    // 注册回调缓冲区 获取缓冲队列接口
    (*pcmPlayerObject)->GetInterface(pcmPlayerObject, SL_IID_BUFFERQUEUE, &pcmBufferQueue);
    //缓冲接口回调
    (*pcmBufferQueue)->RegisterCallback(pcmBufferQueue, pcmBufferCallBack, this);
    // 获取音量接口
    (*pcmPlayerObject)->GetInterface(pcmPlayerObject, SL_IID_VOLUME, &pcmPlayerVolume);

    // 获取播放状态接口
    (*pcmPlayerPlay)->SetPlayState(pcmPlayerPlay, SL_PLAYSTATE_PLAYING);
    pcmBufferCallBack(pcmBufferQueue, this);
    return 0;
}

void AudioHandler::release() {
    pause();
    if (playerQueue != NULL) {
        playerQueue->noticeThread();
    }
    int count = 0;
    while (!isExit) {
        if (count > 1000) {
            isExit = true;
        }
        count++;
        av_usleep(1000);
    }
    if (playerQueue != NULL) {
        playerQueue->release();
        delete (playerQueue);
        playerQueue = NULL;
    }
    if (pcmPlayerObject != NULL) {
        (*pcmPlayerObject)->Destroy(pcmPlayerObject);
        pcmPlayerObject = NULL;
        pcmPlayerPlay = NULL;
        pcmPlayerVolume = NULL;
        pcmBufferQueue = NULL;
        pcmsize = 0;
    }
    if(buffer != NULL) {
        free(buffer);
        buffer = NULL;
    }

    if (outputMixObject != NULL) {
        (*outputMixObject)->Destroy(outputMixObject);
        outputMixObject = NULL;
        outputMixEnvironmentalReverb = NULL;
    }
    // destroy engine object, and invalidate all associated interfaces
    if (engineObject != NULL) {
        (*engineObject)->Destroy(engineObject);
        engineObject = NULL;
        engineEngine = NULL;
    }
    if (avCodecContext != NULL) {
        avcodec_close(avCodecContext);
        avcodec_free_context(&avCodecContext);
        avCodecContext = NULL;
    }
    if (playerStatus != NULL) {
        playerStatus = NULL;
    }
    LOGE("AudioHandler Released!");
}


