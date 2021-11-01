package com.renhui.mediaplayer;

import android.media.Image;
import android.media.MediaCodec;
import android.media.MediaFormat;
import android.text.TextUtils;
import android.util.Log;
import android.view.Surface;

import com.renhui.mediaplayer.listener.ErrorCode;
import com.renhui.mediaplayer.listener.OnCompleteListener;
import com.renhui.mediaplayer.listener.OnErrorListener;
import com.renhui.mediaplayer.listener.OnPreparedListener;
import com.renhui.mediaplayer.listener.OnVideoPlayInfoListener;
import com.renhui.mediaplayer.listener.OnVideoSizeParamListener;
import com.renhui.mediaplayer.utils.GlobalConfig;

import java.nio.ByteBuffer;

import javax.microedition.khronos.opengles.GL;

/**
 * FFmpeg播放器核心类
 */
public class FFmpegPlayer {

    private static final String LOG_TAG = "DEBUG";

    // 加载底层so库
    static {
        System.loadLibrary("media-editor-lib");
    }

    private boolean videoRenderStarted = false;

    /**
     * 是否只有音频（只播放音频流）
     */
    private boolean isOnlyMusic = false;

    /**
     * 渲染surface
     */
    private Surface surface;

    /**
     * 播放文件路径
     */
    private String dataSource;

    /**
     * 当前播放进度
     */
    private long currentPosition;

    /**
     * 总播放时长
     */
    private long duration;

    /**
     * 硬解码mime
     */
    private MediaFormat mediaFormat;
    /**
     * 视频解码器info
     */
    private MediaCodec.BufferInfo info = new MediaCodec.BufferInfo();

    /**
     * 视频硬解码器
     */
    private MediaCodec mediaCodec;

    /**
     * 是否播放中
     */
    private boolean isPlaying = false;

    /**
     * 是否可播放
     */
    private boolean isPlayable = false;

    // 构造函数
    public FFmpegPlayer() {}

    // 设置播放源地址
    public void setDataSource(String dataSource) {
        this.dataSource = dataSource;
    }

    // 设置解码渲染的Surface
    public void setSurface(Surface surface) {
        this.surface = surface;
    }

    /**
     * 准备&加载视频资源
     */
    public void prepareAsync() {
        if (TextUtils.isEmpty(dataSource)) {
            onError(ErrorCode.ERROR_DATA_SOURCE_NULL, "DataSource is Null！");
            return;
        }
        _prepare(dataSource, isOnlyMusic);
    }

    /**
     * 恢复播放
     */
    public void start() {
        _resume();
        isPlaying = true;
    }

    /**
     * 暂停播放
     */
    public void pause() {
        _pause();
        isPlaying = false;
    }

    public long getCurrentPosition() {
        return this.currentPosition;
    }

    public long getDuration() {
        return this.duration;
    }

    public void setSpeed(float speed) {
        _setSpeed(speed);
    }

    public float getSpeed() {
        return (float) _getSpeed();
    }

    //  执行Seek操作
    public void seek(int position) {
        int seekSecond = position / 1000;
        _seek(seekSecond); // 单位：秒
    }

    /**
     * 调用停止视频播放的逻辑
     */
    public void stop() {
        _stop();
    }

    public void reset() {

    }

    public boolean isPlaying() {
        return isPlaying;
    }

    public boolean isPlayable() {
        return isPlayable;
    }

    public void seekTo(long time) {
        seek((int) time);
    }

    /**
     * 调用视频播放器资源销毁
     */
    public void release() {
        this.isPlayable = false;
        _release();
    }


    /**
     * 调用准备播放逻辑
     *
     * @param url         播放地址
     * @param isOnlyMusic 是否只播放音频
     */
    private native void _prepare(String url, boolean isOnlyMusic);

    /**
     * 调用播放逻辑
     */
    private native void _start();

    /**
     * 调用暂停逻辑
     */
    private native void _pause();

    /**
     * 调用恢复播放逻辑
     */
    private native void _resume();

    /**
     * 执行Seek操作 （单位：秒）
     */
    private native void _seek(int seconds);

    /**
     * 设置倍速
     */
    private native void _setSpeed(double speed);

    /**
     * 获取当前倍速
     */
    private native double _getSpeed();

    /**
     * 获取视频时长
     */
    private native long _duration();

    /**
     * 停止播放
     */
    private native void _stop();

    /**
     * 是否播放器
     */
    private native void _release();

    /**
     * 回调播放器加载完毕（Call By Native）
     */
    private void onLoad(boolean load) {
        Log.e(LOG_TAG, "播放器内核加载完毕，Result = " + load);
    }

    /**
     * 回调播放准备完成（Call By Native）
     */
    private void onPrepared() {
        _start();
        this.isPlayable = true;
        this.videoRenderStarted = false;
        if (mPreparedListener != null) {
            mPreparedListener.onPrepared();
        }
    }

    /**
     * 回调视频参数（Call By Native）
     *
     * @param width  视频画面宽度
     * @param height 视频画面高度
     */
    private void onVideoParam(int width, int height) {
        if (mVideoSizeParamListener != null) {
            Log.e("DEBUG", "Video width = " + width + ", Height = " + height);
            mVideoSizeParamListener.onVideoSizeChange(width, height);
        }
    }

    /**
     * 回调播放信息（Call By Native）
     *
     * @param currentSecond 当前播放时间
     * @param totalSecond   播放总时长
     */
    private void onVideoInfo(int currentSecond, int totalSecond) {
        this.currentPosition = currentSecond * 1000;
        this.duration = totalSecond * 1000;
    }

    /**
     * 初始化硬解码器（Call By Native）
     */
    public void onInitMediaCodec(int mimetype, int width, int height, byte[] csd0, byte[] csd1) {
        Log.e(LOG_TAG, "初始化硬编码， Width = " + width + ",Height = " + height);
        if (surface != null) {
            try {
                String mtype = getMimeType(mimetype);
                mediaFormat = MediaFormat.createVideoFormat(mtype, width, height);
                mediaFormat.setInteger(MediaFormat.KEY_WIDTH, width);
                mediaFormat.setInteger(MediaFormat.KEY_HEIGHT, height);
                mediaFormat.setLong(MediaFormat.KEY_MAX_INPUT_SIZE, width * height);
                // "csd-0"和"csd-1"，对于H264视频的话，它对应的是sps和pps，
                // 对于AAC音频的话，对应的是ADTS.
                // sps和pps存在于编码器生成的IDR帧之中。
                mediaFormat.setByteBuffer("csd-0", ByteBuffer.wrap(csd0));
                mediaFormat.setByteBuffer("csd-1", ByteBuffer.wrap(csd1));
                // 根据格式，创建解码器
                mediaCodec = MediaCodec.createDecoderByType(mtype);
                if (surface != null) {
                    mediaCodec.configure(mediaFormat, surface, null, 0);
                    mediaCodec.start();
                }
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
    }

    /**
     * 回调硬解码播放数据（Call By Native）
     *
     * @param bytes packet数据
     * @param size  数据大小
     * @param pts   pts
     */
    public void onMediaCodecData(byte[] bytes, int size, int pts) {
        if (bytes != null && mediaCodec != null && info != null) {
            try {
                if (!videoRenderStarted) {
                    if (mVideoPlayInfoListener != null) {
                        Log.e("DEBUG", "回调播放开始");
                        // int MEDIA_INFO_VIDEO_RENDERING_START = 3;
                        // int MEDIA_INFO_AUDIO_RENDERING_START  = 10002;
                        // 同 Ijk 相同Code MEDIA_INFO_AUDIO_RENDERING_START，便于UI层同意处理
                        mVideoPlayInfoListener.onInfo(3, 0);
                        videoRenderStarted = true;
                    }
                }
                int inputBufferIndex = mediaCodec.dequeueInputBuffer(10);
                if (inputBufferIndex >= 0) {
                    ByteBuffer byteBuffer = mediaCodec.getInputBuffers()[inputBufferIndex];
                    byteBuffer.clear();
                    byteBuffer.put(bytes);
                    mediaCodec.queueInputBuffer(inputBufferIndex, 0, size, pts, 0);
                }
                int index = mediaCodec.dequeueOutputBuffer(info, 10);
                while (index >= 0) {
                    mediaCodec.releaseOutputBuffer(index, true);
                    index = mediaCodec.dequeueOutputBuffer(info, 10);
                }
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
    }

    /**
     * 回调播放错误（Call By Native）
     *
     * @param errorCode 错误码
     * @param msg       错误信息
     */
    private void onError(int errorCode, String msg) {
        if (mediaCodec != null) {
            mediaCodec.stop();
            mediaCodec.release();
        }
        if (mErrorListener != null) {
            mErrorListener.onError(errorCode, msg);
        }
    }

    /**
     * 回调播放完成信息（Call By Native）
     */
    private void onComplete() {
        if (mediaCodec != null) {
            mediaCodec.stop();
            mediaCodec.release();
        }
        if (mOnCompleteListener != null) {
            mOnCompleteListener.onComplete();
        }
        isPlaying = false;
    }

    //********************************* 播放格式 ****************************************/

    private String getMimeType(int type) {
        if (type == 1) {
            return "video/avc";
        } else if (type == 2) {
            return "video/hevc";
        } else if (type == 3) {
            return "video/mp4v-es";
        } else if (type == 4) {
            return "video/x-ms-wmv";
        }
        return "";
    }

    public void requestVideoImageCapture() {
        Log.e("Debug", "Request Video Capture");
    }

    //******************************** 播放器回调事件相关 *****************************************/

    // 播放错误时回调
    OnErrorListener mErrorListener;

    // 播放信息回调
    OnVideoPlayInfoListener mVideoPlayInfoListener;

    // 视频加载回调
    OnPreparedListener mPreparedListener;

    OnVideoSizeParamListener mVideoSizeParamListener;

    // 视频播放完成回调
    OnCompleteListener mOnCompleteListener;

    public OnErrorListener getErrorListener() {
        return mErrorListener;
    }

    public void setErrorListener(OnErrorListener errorListener) {
        mErrorListener = errorListener;
    }

    public OnPreparedListener getPreparedListener() {
        return mPreparedListener;
    }

    public void setPreparedListener(OnPreparedListener preparedListener) {
        mPreparedListener = preparedListener;
    }

    public OnCompleteListener getOnCompleteListener() {
        return mOnCompleteListener;
    }

    public void setOnCompleteListener(OnCompleteListener onCompleteListener) {
        mOnCompleteListener = onCompleteListener;
    }

    public void setVideoPlayInfoListener(OnVideoPlayInfoListener videoPlayInfoListener) {
        mVideoPlayInfoListener = videoPlayInfoListener;
    }

    public OnVideoSizeParamListener getVideoSizeParamListener() {
        return mVideoSizeParamListener;
    }

    public void setVideoSizeParamListener(OnVideoSizeParamListener videoSizeParamListener) {
        this.mVideoSizeParamListener = videoSizeParamListener;
    }
}
