package com.willkernel.www.ffmpeg;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.util.Log;
import android.view.Surface;

/**
 * Created by willkernel on 2018/1/17.
 */

public class VideoPlayer {
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

    public native void render(String input, Surface surface);

    public native void play(String input);

    public AudioTrack createAudioTrack(int sampleRateInHz, int nb_channels) {
        //固定格式音频码流
        int audioFormat = AudioFormat.ENCODING_PCM_16BIT;
        Log.e("nb_channels", String.valueOf(nb_channels));
        //声道布局
        int channelConfig;
        if (nb_channels == 1) {
            channelConfig = AudioFormat.CHANNEL_OUT_MONO;
        } else if (nb_channels == 2) {
            channelConfig = AudioFormat.CHANNEL_OUT_STEREO;
        } else {
            channelConfig = AudioFormat.CHANNEL_OUT_STEREO;
        }

        int bufferSizeInBytes = AudioTrack.getMinBufferSize(sampleRateInHz, channelConfig, audioFormat);

        AudioTrack audioTrack = new AudioTrack(AudioManager.STREAM_MUSIC, sampleRateInHz, channelConfig, audioFormat, bufferSizeInBytes, AudioTrack.MODE_STREAM);
//        播放
//        audioTrack.play();
//        写入PCM
//        audioTrack.write(audioData, offsetInBytes, sizeInBytes);
//        播放完调用stop即可
        audioTrack.stop();
        return audioTrack;
    }
}
