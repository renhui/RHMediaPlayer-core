package com.renhui.mediaplayer.base;

import android.view.Surface;

public abstract class AbstractVideoPlayer {

    // 播放视频模式
    public static final int PLAY_VIDEO_MODE = 0x0001;

    // 播放音频模式
    public static final int PLAY_AUDIO_MODE = 0x0002;

    public VideoPlayerEventListener videoPlayerEventListener = null;

    public void setVideoPlayerEventListener(VideoPlayerEventListener videoPlayerEventListener) {
        this.videoPlayerEventListener = videoPlayerEventListener;
    }

    /*----------------------------第一部分：视频初始化实例对象方法----------------------------------*/

    /**
     * 初始化播放器实例
     * 视频播放器第一步：创建视频播放器
     */
    public abstract void initPlayer();

    /**
     * 设置播放地址
     * 视频播放器第二步：设置数据
     *
     * @param path 播放地址
     */
    public abstract void setDataSource(String path);

    /*
     * 设置视频播放模式（视频 or 音频）
     */
    public abstract void setPlayMode(int playMode);

    /**
     * 设置渲染视频的View,主要用于TextureView
     * 视频播放器第三步：设置surface
     *
     * @param surface surface
     */
    public abstract void setSurface(Surface surface);

    /**
     * 设置渲染视频的View的宽高
     */
    public abstract void setSurfaceSize(int width, int height);

    /**
     * 准备开始播放（异步）
     * 视频播放器第四步：开始加载【异步】
     */
    public abstract void prepareAsync();

    /*----------------------------第二部分：视频播放器状态方法----------------------------------*/

    /**
     * 播放
     */
    public abstract void start();

    /**
     * 暂停
     */
    public abstract void pause();

    /**
     * 停止
     */
    public abstract void stop();

    /**
     * 重置播放器
     */
    public abstract void reset();

    /**
     * 是否正在播放
     *
     * @return 是否正在播放
     */
    public abstract boolean isPlaying();

    /**
     * 是否可播放
     */
    public abstract boolean isPlayable();

    /**
     * 调整进度
     */
    public abstract void seekTo(long time);

    /**
     * 释放播放器
     */
    public abstract void release();

    /**
     * 获取当前播放的位置
     *
     * @return 获取当前播放的位置
     */
    public abstract long getCurrentPosition();

    /**
     * 获取视频总时长
     *
     * @return 获取视频总时长
     */
    public abstract long getDuration();

    /**
     * 设置播放速度
     *
     * @param speed 速度
     */
    public abstract void setSpeed(float speed);

    /**
     * 获取播放速度
     *
     * @return 播放速度
     */
    public abstract float getSpeed();

    /**
     * 请求获取视频画面截取
     */
    public abstract void requestVideoImageCapture();
}
