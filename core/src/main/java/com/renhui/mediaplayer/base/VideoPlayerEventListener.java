package com.renhui.mediaplayer.base;

public interface VideoPlayerEventListener {

    /**
     * 播放异常
     *
     * @param type 错误Code
     */
    void onError(int type, int extra);

    /**
     * 完成
     */
    void onCompletion();

    /**
     * 视频信息
     *
     * @param what  what
     * @param extra extra
     */
    void onInfo(int what, int extra);

    /**
     * 开始缓冲
     */
    void onBufferStart();

    /**
     * 缓冲结束
     */
    void onBufferEnd();

    /**
     * 准备
     */
    void onPrepared();

    /**
     * 视频size变化监听
     *
     * @param width  宽
     * @param height 高
     */
    void onVideoSizeChanged(int width, int height);

}
