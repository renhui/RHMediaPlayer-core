package com.renhui.mediaplayer.listener;

/**
 * @author：renh
 * @date：2020/1/14
 * @des：错误回调
 */
public interface OnErrorListener {

    /**
     * 错误回调
     *
     * @param code    错误码
     * @param message 错误信息
     */
    void onError(int code, String message);
}
