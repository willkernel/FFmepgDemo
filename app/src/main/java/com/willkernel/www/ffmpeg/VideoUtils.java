package com.willkernel.www.ffmpeg;

/**
 * 这里需要注意的是，FFmpeg的动态库是有依赖关系的，
 * 加载的时候需要按照一定的顺序，先加载最基础的库
 */

public class VideoUtils {
    static {
        System.loadLibrary("avutil-54");
        System.loadLibrary("swresample-1");
        System.loadLibrary("avcodec-56");
        System.loadLibrary("avformat-56");
        System.loadLibrary("swscale-3");
        System.loadLibrary("postproc-53");
        System.loadLibrary("avfilter-5");
        System.loadLibrary("avdevice-56");
        System.loadLibrary("ffmpeg-lib");
    }

    //视频解码
    public static native void decode(String input, String output);

    public static native void decodeAudio(String input, String output);
}
