#include <jni.h>
#include <string.h>
#include <android/log.h>

//编码
#include "libavcodec/avcodec.h"
//封装格式处理
#include "libavformat/avformat.h"
//像素处理
#include "libswscale/swscale.h"

#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <libswresample/swresample.h>
#include <libavutil/time.h>
#include "yuv/libyuv.h"

#define LOGI(FORMAT, ...) __android_log_print(ANDROID_LOG_INFO,"ffmpeg",FORMAT,##__VA_ARGS__);
#define LOGE(FORMAT, ...) __android_log_print(ANDROID_LOG_ERROR,"ffmpeg",FORMAT,##__VA_ARGS__);
#define MAX_AUDIO_FRME_SIZE 48000*4

//extern "C"
JNIEXPORT void JNICALL
Java_com_willkernel_www_ffmpeg_VideoUtils_decode(JNIEnv *env, jclass type, jstring input_,
                                                 jstring output_) {
    //需要转码的视频文件(输入的视频文件)
    const char *input = (*env)->GetStringUTFChars(env, input_, 0);
    const char *outptu = (*env)->GetStringUTFChars(env, output_, 0);

    //1.注册所有组件，例如初始化一些全局的变量、初始化网络等等
    av_register_all();
    //avcodec_register_all();

    //封装格式上下文，统领全局的结构体，保存了视频文件封装格式的相关信息
    AVFormatContext *pFormatCtx = avformat_alloc_context();

    //2.打开输入视频文件
    if (avformat_open_input(&pFormatCtx, input, NULL, NULL) != 0) {
        LOGE("%s", "无法打开输入视频文件");
        return;
    }

    //3.获取视频文件信息，例如得到视频的宽高
    //第二个参数是一个字典，表示你需要获取什么信息，比如视频的元数据
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        LOGE("%s", "无法获取视频文件信息");
        return;
    }

    //获取视频流的索引位置
    //遍历所有类型的流（音频流、视频流、字幕流），找到视频流
    int v_stream_idx = -1;
    int i = 0;
    //number of streams
    for (; i < pFormatCtx->nb_streams; i++) {
        //流的类型
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            v_stream_idx = i;
            break;
        }
    }

    if (v_stream_idx == -1) {
        LOGE("%s", "找不到视频流\n");
        return;
    }

    //只有知道视频的编码方式，才能够根据编码方式去找到解码器
    //获取视频流中的编解码上下文
    AVCodecContext *pCodecCtx = pFormatCtx->streams[v_stream_idx]->codec;
    //4.根据编解码上下文中的编码id查找对应的解码
    AVCodec *pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    //（迅雷看看，找不到解码器，临时下载一个解码器，比如视频加密了）
    if (pCodec == NULL) {
        LOGE("%s", "找不到解码器，或者视频已加密\n");
        return;
    }

    //5.打开解码器，解码器有问题（比如说我们编译FFmpeg的时候没有编译对应类型的解码器）
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        LOGE("%s", "解码器无法打开\n");
        return;
    }

    //输出视频信息
    LOGI("视频的文件格式：%s", pFormatCtx->iformat->name);
    LOGI("视频时长：%lld", (pFormatCtx->duration) / 1000000);
    LOGI("视频的宽高：%d,%d", pCodecCtx->width, pCodecCtx->height);
    LOGI("解码器的名称：%s", pCodec->name);

    //准备读取
    //AVPacket用于存储一帧一帧的压缩数据（H264）
    //缓冲区，开辟空间
    AVPacket *packet = (AVPacket *) av_malloc(sizeof(AVPacket));

    //AVFrame用于存储解码后的像素数据(YUV)
    //内存分配
    AVFrame *pFrame = av_frame_alloc();
    //YUV420
    AVFrame *pFrameYUV = av_frame_alloc();
    //只有指定了AVFrame的像素格式、画面大小才能真正分配内存
    //缓冲区分配内存
    uint8_t *out_buffer = (uint8_t *) av_malloc(
            avpicture_get_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height));
    //初始化缓冲区
    avpicture_fill((AVPicture *) pFrameYUV, out_buffer, AV_PIX_FMT_YUV420P, pCodecCtx->width,
                   pCodecCtx->height);

    //用于转码（缩放）的参数，转之前的宽高，转之后的宽高，格式等
    struct SwsContext *sws_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height,
                                                pCodecCtx->pix_fmt,
                                                pCodecCtx->width, pCodecCtx->height,
                                                AV_PIX_FMT_YUV420P,
                                                SWS_BICUBIC, NULL, NULL, NULL);


    int got_picture, ret;

    FILE *fp_yuv = fopen(outptu, "wb+");

    int frame_count = 0;

    //6.一帧一帧的读取压缩数据
    while (av_read_frame(pFormatCtx, packet) >= 0) {
        //只要视频压缩数据（根据流的索引位置判断）
        if (packet->stream_index == v_stream_idx) {
            //7.解码一帧视频压缩数据，得到视频像素数据
            ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
            if (ret < 0) {
                LOGE("%s", "解码错误");
                return;
            }

            //为0说明解码完成，非0正在解码
            if (got_picture) {
                //AVFrame转为像素格式YUV420，宽高
                //2 6输入、输出数据
                //3 7输入、输出画面一行的数据的大小 AVFrame 转换是一行一行转换的
                //4 输入数据第一列要转码的位置 从0开始
                //5 输入画面的高度
                sws_scale(sws_ctx, pFrame->data, pFrame->linesize, 0, pCodecCtx->height,
                          pFrameYUV->data, pFrameYUV->linesize);

                //输出到YUV文件
                //AVFrame像素帧写入文件
                //data解码后的图像像素数据（音频采样数据）
                //Y 亮度 UV 色度（压缩了） 人对亮度更加敏感
                //U V 个数是Y的1/4
                int y_size = pCodecCtx->width * pCodecCtx->height;
                fwrite(pFrameYUV->data[0], 1, y_size, fp_yuv);
                fwrite(pFrameYUV->data[1], 1, y_size / 4, fp_yuv);
                fwrite(pFrameYUV->data[2], 1, y_size / 4, fp_yuv);

                frame_count++;
                LOGI("解码第%d帧", frame_count);
            }
        }

        //释放资源
        av_free_packet(packet);
    }

    fclose(fp_yuv);

    av_frame_free(&pFrame);

    avcodec_close(pCodecCtx);

    avformat_free_context(pFormatCtx);

    (*env)->ReleaseStringUTFChars(env, input_, input);
    (*env)->ReleaseStringUTFChars(env, output_, outptu);
}

JNIEXPORT void JNICALL
Java_com_willkernel_www_ffmpeg_VideoPlayer_render(JNIEnv *env, jclass type, jstring input_,
                                                  jobject surface) {
    const char *input_cstr = (*env)->GetStringUTFChars(env, input_, NULL);
    //1.注册组件
    av_register_all();

    //封装格式上下文
    AVFormatContext *pFormatCtx = avformat_alloc_context();

    //2.打开输入视频文件
    if (avformat_open_input(&pFormatCtx, input_cstr, NULL, NULL) != 0) {
        LOGE("%s", "打开输入视频文件失败");
        return;
    }
    //3.获取视频信息
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        LOGE("%s", "获取视频信息失败");
        return;
    }

    //视频解码，需要找到视频对应的AVStream所在pFormatCtx->streams的索引位置
    int video_stream_idx = -1;
    int i = 0;
    for (; i < pFormatCtx->nb_streams; i++) {
        //根据类型判断，是否是视频流
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_idx = i;
            break;
        }
    }

    //4.获取视频解码器
    AVCodecContext *pCodeCtx = pFormatCtx->streams[video_stream_idx]->codec;
    AVCodec *pCodec = avcodec_find_decoder(pCodeCtx->codec_id);
    if (pCodec == NULL) {
        LOGE("%s", "无法解码");
        return;
    }

    //5.打开解码器
    if (avcodec_open2(pCodeCtx, pCodec, NULL) < 0) {
        LOGE("%s", "解码器无法打开");
        return;
    }

    //编码数据
    AVPacket *packet = (AVPacket *) av_malloc(sizeof(AVPacket));

    //像素数据（解码数据）
    AVFrame *yuv_frame = av_frame_alloc();
    AVFrame *rgb_frame = av_frame_alloc();

    /*//ScaleImg視頻縮放
    if(ScaleImg(pCodeCtx,yuv_frame,rgb_frame,pCodeCtx->width/6,pCodeCtx->width/6)!=1){
        LOGE("%s","縮放失敗");
        return;
    }*/

    //native绘制
    //窗体

    ANativeWindow *nativeWindow = ANativeWindow_fromSurface(env, surface);

    //绘制时的缓冲区
    ANativeWindow_Buffer outBuffer;

    int len, got_frame, framecount = 0;
    //6.一阵一阵读取压缩的视频数据AVPacket
    while (av_read_frame(pFormatCtx, packet) >= 0) {
        //解码AVPacket->AVFrame
        len = avcodec_decode_video2(pCodeCtx, yuv_frame, &got_frame, packet);

        //Zero if no frame could be decompressed
        //非零，正在解码
        if (got_frame) {
            LOGI("解码%d帧", framecount++);
            //lock
            //设置缓冲区的属性（宽、高、像素格式）
            ANativeWindow_setBuffersGeometry(nativeWindow, pCodeCtx->width, pCodeCtx->height,
                                             WINDOW_FORMAT_RGBA_8888);
            ANativeWindow_lock(nativeWindow, &outBuffer, NULL);

            //设置rgb_frame的属性（像素格式、宽高）和缓冲区
            //rgb_frame缓冲区与outBuffer.bits是同一块内存
            avpicture_fill((AVPicture *) rgb_frame, outBuffer.bits, AV_PIX_FMT_RGBA,
                           pCodeCtx->width, pCodeCtx->height);

            //YUV->RGBA_8888
            I420ToARGB(yuv_frame->data[0], yuv_frame->linesize[0],
                       yuv_frame->data[2], yuv_frame->linesize[2],
                       yuv_frame->data[1], yuv_frame->linesize[1],
                       rgb_frame->data[0], rgb_frame->linesize[0],
                       pCodeCtx->width, pCodeCtx->height);

            //unlock
            ANativeWindow_unlockAndPost(nativeWindow);

            av_usleep(1000 * 16);

        }

        av_free_packet(packet);
    }

    ANativeWindow_release(nativeWindow);
    av_frame_free(&yuv_frame);
    avcodec_close(pCodeCtx);
    avformat_free_context(pFormatCtx);

    (*env)->ReleaseStringUTFChars(env, input_, input_cstr);
}

//音频解码，最终会输出pcm格式的文件
JNIEXPORT void JNICALL
Java_com_willkernel_www_ffmpeg_VideoUtils_decodeAudio(JNIEnv *env, jclass type, jstring inputVideo_,
                                                      jstring outputVideo_) {
    const char *input = (*env)->GetStringUTFChars(env, inputVideo_, 0);
    const char *output = (*env)->GetStringUTFChars(env, outputVideo_, 0);

    av_register_all();
    AVFormatContext *pFormatCtx = avformat_alloc_context();
    //打开音频文件
    if (avformat_open_input(&pFormatCtx, input, NULL, NULL) != 0) {
        LOGI("%s", "cannot open file")
        return;
    }
    //获取信息
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        LOGI("%s", "cannot get info")
        return;
    }

    //获取音频流索引位置
    int i = 0, audio_stream_idx = -1;
    for (; i < pFormatCtx->nb_streams; i++) {
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_idx = i;
            break;
        }
    }

    //获取解码器
    AVCodecContext *codecCtx = pFormatCtx->streams[audio_stream_idx]->codec;
    AVCodec *codec = avcodec_find_decoder(codecCtx->codec_id);
    if (codec == NULL) {
        LOGI("%s", "cannot get codec")
        return;
    }
    //打开解码器
    if (avcodec_open2(codecCtx, codec, NULL) < 0) {
        LOGI("%s", "cannot open codec");
        return;
    }
    //压缩数据
    AVPacket *packet = av_malloc(sizeof(AVPacket));
    //解压缩数据
    AVFrame *frame = av_frame_alloc();
    //frame->16bit 44100 PCM 统一音频采样格式与采样率
    SwrContext *swrCtx = swr_alloc();

    //重采样设置参数
    //输入采样格式
    enum AVSampleFormat in_sample_fmt = codecCtx->sample_fmt;
    //输出采样格式16bit PCM
    enum AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;
    //输入采样率
    int in_sample_rate = codecCtx->sample_rate;
    //输出采样率
    int out_sample_rate = 44100;
    //获取输入的声道布局
    //根据声道个数获取默认的声道布局（2个声道，默认立体声stereo）
    //av_get_default_channel_layout(codecCtx->channels);
    uint64_t in_ch_layout = codecCtx->channel_layout;
    //输出的声道布局
    uint64_t out_ch_layout = AV_CH_LAYOUT_STEREO;

    swr_alloc_set_opts(swrCtx, out_ch_layout, out_sample_fmt, out_sample_rate, in_ch_layout,
                       in_sample_fmt, in_sample_rate, 0, NULL);

    swr_init(swrCtx);

    //输出声道个数
    int out_channel_nb = av_get_channel_layout(out_ch_layout);;

    //重采样设置参数
    //16bit 44100 PCM数据
    uint8_t *out_buffter = (uint8_t *) av_malloc(MAX_AUDIO_FRME_SIZE);

    FILE *fp_pcm = fopen(output, "wb");

    int got_frame = 0, index = 0, ret;
    //读取压缩数据
    while (av_read_frame(pFormatCtx, packet) >= 0) {
        //解码
        ret = avcodec_decode_audio4(codecCtx, frame, &got_frame, packet);

        if (ret < 0) {
            LOGI("%s", "code complete");
        }
        //解码一帧成功
        if (got_frame > 0) {
            LOGI("解码 ：%d", index++);
            swr_convert(swrCtx, &out_buffter, MAX_AUDIO_FRME_SIZE, frame->data, frame->nb_samples);
            //获取sample的size
            int out_buffer_size = av_samples_get_buffer_size(NULL, out_channel_nb,
                                                             frame->nb_samples, out_sample_fmt, 1);
            fwrite(out_buffter, 1, out_buffer_size, fp_pcm);

        }
        av_free_packet(packet);

    }

    fclose(fp_pcm);
    av_frame_free(&frame);
    av_free(out_buffter);

    swr_free(&swrCtx);
    avcodec_close(codecCtx);
    avformat_close_input(&pFormatCtx);


    (*env)->ReleaseStringUTFChars(env, inputVideo_, input);
    (*env)->ReleaseStringUTFChars(env, outputVideo_, output);
}

JNIEXPORT void JNICALL
Java_com_willkernel_www_ffmpeg_VideoPlayer_play(JNIEnv *env, jclass instance, jstring input_) {
    const char *input = (*env)->GetStringUTFChars(env, input_, 0);


    LOGI("%s", "sound");
    //注册组件
    av_register_all();
    AVFormatContext *pFormatCtx = avformat_alloc_context();
    //打开音频文件
    if (avformat_open_input(&pFormatCtx, input, NULL, NULL) != 0) {
        LOGI("%s", "无法打开音频文件");
        return;
    }
    //获取输入文件信息
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        LOGI("%s", "无法获取输入文件信息");
        return;
    }
    //获取音频流索引位置
    int i = 0, audio_stream_idx = -1;
    for (; i < pFormatCtx->nb_streams; i++) {
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_idx = i;
            break;
        }
    }

    //获取解码器
    AVCodecContext *codecCtx = pFormatCtx->streams[audio_stream_idx]->codec;
    AVCodec *codec = avcodec_find_decoder(codecCtx->codec_id);
    if (codec == NULL) {
        LOGI("%s", "无法获取解码器");
        return;
    }
    //打开解码器
    if (avcodec_open2(codecCtx, codec, NULL) < 0) {
        LOGI("%s", "无法打开解码器");
        return;
    }
    //压缩数据
    AVPacket *packet = (AVPacket *) av_malloc(sizeof(AVPacket));
    //解压缩数据
    AVFrame *frame = av_frame_alloc();
    //frame->16bit 44100 PCM 统一音频采样格式与采样率
    SwrContext *swrCtx = swr_alloc();

    //重采样设置参数-------------start
    //输入的采样格式
    enum AVSampleFormat in_sample_fmt = codecCtx->sample_fmt;
    //输出采样格式16bit PCM
    enum AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;
    //输入采样率
    int in_sample_rate = codecCtx->sample_rate;
    //输出采样率
    int out_sample_rate = in_sample_rate;
    //获取输入的声道布局
    //根据声道个数获取默认的声道布局（2个声道，默认立体声stereo）
    //av_get_default_channel_layout(codecCtx->channels);
    uint64_t in_ch_layout = codecCtx->channel_layout;
    //输出的声道布局（立体声）
    uint64_t out_ch_layout = AV_CH_LAYOUT_STEREO;

    swr_alloc_set_opts(swrCtx,
                       out_ch_layout, out_sample_fmt, out_sample_rate,
                       in_ch_layout, in_sample_fmt, in_sample_rate,
                       0, NULL);
    swr_init(swrCtx);

    //输出的声道个数
    int out_channel_nb = av_get_channel_layout_nb_channels(out_ch_layout);

    //重采样设置参数-------------end

    //JNI begin------------------
    //JasonPlayer
    jclass player_class = (*env)->GetObjectClass(env, instance);

    //AudioTrack对象
    jmethodID create_audio_track_mid = (*env)->GetMethodID(env, player_class, "createAudioTrack",
                                                           "(II)Landroid/media/AudioTrack;");
    jobject audio_track = (*env)->CallObjectMethod(env, instance, create_audio_track_mid,
                                                   out_sample_rate, out_channel_nb);

    //调用AudioTrack.play方法
    jclass audio_track_class = (*env)->GetObjectClass(env, audio_track);
    jmethodID audio_track_play_mid = (*env)->GetMethodID(env, audio_track_class, "play", "()V");
    jmethodID audio_track_stop_mid = (*env)->GetMethodID(env, audio_track_class, "stop", "()V");
    (*env)->CallVoidMethod(env, audio_track, audio_track_play_mid);

    //AudioTrack.write
    jmethodID audio_track_write_mid = (*env)->GetMethodID(env, audio_track_class, "write",
                                                          "([BII)I");
    //JNI end------------------

    //16bit 44100 PCM 数据
    uint8_t *out_buffer = (uint8_t *) av_malloc(MAX_AUDIO_FRME_SIZE);

    int got_frame = 0, index = 0, ret;
    //不断读取压缩数据
    while (av_read_frame(pFormatCtx, packet) >= 0) {
        //解码音频类型的Packet
        if (packet->stream_index == audio_stream_idx) {
            //解码
            ret = avcodec_decode_audio4(codecCtx, frame, &got_frame, packet);

            if (ret < 0) {
                LOGI("%s", "解码完成");
            }
            //解码一帧成功
            if (got_frame > 0) {
                LOGI("解码：%d", index++);
                swr_convert(swrCtx, &out_buffer, MAX_AUDIO_FRME_SIZE,
                            (const uint8_t **) frame->data, frame->nb_samples);
                //获取sample的size
                int out_buffer_size = av_samples_get_buffer_size(NULL, out_channel_nb,
                                                                 frame->nb_samples, out_sample_fmt,
                                                                 1);

                //out_buffer缓冲区数据，转成byte数组
                jbyteArray audio_sample_array = (*env)->NewByteArray(env, out_buffer_size);
                jbyte *sample_bytep = (*env)->GetByteArrayElements(env, audio_sample_array, NULL);
                //out_buffer的数据复制到sample_bytep
                memcpy(sample_bytep, out_buffer, out_buffer_size);
                //同步
                (*env)->ReleaseByteArrayElements(env, audio_sample_array, sample_bytep, 0);

                //AudioTrack.write PCM数据
                (*env)->CallIntMethod(env, audio_track, audio_track_write_mid,
                                      audio_sample_array, 0, out_buffer_size);
                //释放局部引用
                (*env)->DeleteLocalRef(env, audio_sample_array);
            }
        }

        av_free_packet(packet);
    }

    (*env)->CallVoidMethod(env, audio_track, audio_track_stop_mid);

    av_frame_free(&frame);
    av_free(out_buffer);

    swr_free(&swrCtx);
    avcodec_close(codecCtx);
    avformat_close_input(&pFormatCtx);


    (*env)->ReleaseStringUTFChars(env, input_, input);
}