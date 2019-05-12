#include <jni.h>
#include <string>
#include "android/log.h"
#include <android/native_window_jni.h>
#include <unistd.h>
#ifdef __cplusplus
extern "C" {
#endif
#include "libavcodec/avcodec.h"
#include "libavutil/avutil.h"
#include "libswscale/swscale.h"
#include <libavutil/imgutils.h>
//封装格式处理
#include "libavformat/avformat.h"
//像素处理
#include "libswscale/swscale.h"
#define FFLOGI(FORMAT,...) __android_log_print(ANDROID_LOG_INFO,"ffmpeg_test",FORMAT,##__VA_ARGS__);
#define LOGD(FORMAT,...) __android_log_print(ANDROID_LOG_INFO,"ffmpeg_test",FORMAT,##__VA_ARGS__);
#define FFLOGE(FORMAT,...) __android_log_print(ANDROID_LOG_ERROR,"ffmpeg_test",FORMAT,##__VA_ARGS__);

extern "C"
JNIEXPORT jstring JNICALL
Java_com_houde_ffmpeg_test_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = avcodec_configuration();
    return env->NewStringUTF(hello.c_str());
}

extern "C"
JNIEXPORT void JNICALL
Java_com_houde_ffmpeg_test_SecondActivity_decoder(JNIEnv * env,jobject obj,jstring input,jstring output){
    const char* _input = env->GetStringUTFChars(input,0);
    const char* _output = env->GetStringUTFChars(output,0);
    FFLOGE("%s",_input);
    FFLOGE("%s",_output);
    av_register_all();
//    avcodec_register_all();
//    avformat_network_init();
    av_log_set_level(AV_LOG_INFO);
    AVFormatContext * pInputContext = avformat_alloc_context();
    char buf[1024];
    int result = avformat_open_input(&pInputContext,_input, NULL, NULL);
    if( result != 0){
        av_strerror(result, buf, 1024);
        FFLOGE("Couldn’t open file %s: %d(%s)", _input, result, buf);
        FFLOGE("%s","无法打开视频文件");
        return;
    }

    if(avformat_find_stream_info(pInputContext, NULL) < 0){
        FFLOGE("%s","无法获取视频信息" + result);
        return;
    }

    av_dump_format(pInputContext,0,_input,0);

    int video_stream_index = -1;
    for(int i = 0 ; i < pInputContext->nb_streams; i++){
        if(pInputContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO){
            video_stream_index = i;
            break;
        }
    }
    if(video_stream_index == -1){
        FFLOGE("%s","没有找到视频流");
        return;
    }

    //只有知道视频的编码方式，才能够根据编码方式去找到解码器
    //获取视频流中的编解码上下文
    AVCodecContext *pCodecCtx = pInputContext->streams[video_stream_index]->codec;
    //4.根据编解码上下文中的编码id查找对应的解码
    AVCodec *pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if(pCodec == NULL){
        FFLOGE("%s","没有找到编码器");
        return;
    }
    if(avcodec_open2(pCodecCtx,pCodec,NULL) < 0){
        FFLOGE("%s","无法打开编码器");
        return;
    }
    //输出视频信息
    FFLOGI("视频的文件格式：%s",pInputContext->iformat->name);
    FFLOGI("视频时长：%d", (pInputContext->duration)/1000000);
    FFLOGI("视频的宽高：%d,%d",pCodecCtx->width,pCodecCtx->height);
    FFLOGI("解码器的名称：%s",pCodec->name);

    //准备读取
    //AVPacket用于存储一帧一帧的压缩数据（H264）
    //缓冲区，开辟空间
    AVPacket *packet = (AVPacket*)av_malloc(sizeof(AVPacket));

    //AVFrame用于存储解码后的像素数据(YUV)
    //内存分配
    AVFrame *pFrame = av_frame_alloc();
    //YUV420
    AVFrame *pFrameYUV = av_frame_alloc();
    //只有指定了AVFrame的像素格式、画面大小才能真正分配内存
    //缓冲区分配内存
    uint8_t *out_buffer = (uint8_t *)av_malloc(avpicture_get_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height));
    //初始化缓冲区
    avpicture_fill((AVPicture *)pFrameYUV, out_buffer, AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height);

    //用于转码（缩放）的参数，转之前的宽高，转之后的宽高，格式等
    struct SwsContext *sws_ctx = sws_getContext(pCodecCtx->width,pCodecCtx->height,pCodecCtx->pix_fmt,
                                                pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P,
                                                SWS_BICUBIC, NULL, NULL, NULL);
    int got_picture, ret;

    FILE *fp_yuv = fopen(_output, "wb+");

    int frame_count = 0;

    //6.一帧一帧的读取压缩数据
    while (av_read_frame(pInputContext, packet) >= 0)
    {
        //只要视频压缩数据（根据流的索引位置判断）
        if (packet->stream_index == video_stream_index)
        {
            //7.解码一帧视频压缩数据，得到视频像素数据
            ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
            if (ret < 0)
            {
                printf("%s","解码错误");
                return;
            }

            //为0说明解码完成，非0正在解码
            if (got_picture)
            {
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
                FFLOGI("解码第%d帧\n",frame_count);
            }
        }
        //释放资源
        av_packet_unref(packet);
    }

    //释放资源
    fclose(fp_yuv);

    av_frame_free(&pFrame);

    avcodec_close(pCodecCtx);

    avformat_free_context(pInputContext);

    env->ReleaseStringUTFChars(input, _input);

    env->ReleaseStringUTFChars(output, _output);
}
extern "C"
JNIEXPORT void JNICALL
Java_com_houde_ffmpeg_test_MyVideoView_play(JNIEnv *env, jobject instance,
            jstring videoPath, jobject surface) {
        const char *input = env->GetStringUTFChars(videoPath, NULL);
        if (input == NULL) {
            LOGD("字符串转换失败......");
            return;
        }
        //注册FFmpeg所有编解码器，以及相关协议。
        av_register_all();
        //分配结构体
        AVFormatContext *formatContext = avformat_alloc_context();
        //打开视频数据源。由于Android 对SDK存储权限的原因，如果没有为当前项目赋予SDK存储权限，打开本地视频文件时会失败
        int open_state = avformat_open_input(&formatContext, input, NULL, NULL);
        if (open_state < 0) {
            char errbuf[128];
            if (av_strerror(open_state, errbuf, sizeof(errbuf)) == 0){
                LOGD("打开视频输入流信息失败，失败原因： %s", errbuf);
            }
            return;
        }
        //为分配的AVFormatContext 结构体中填充数据
        if (avformat_find_stream_info(formatContext, NULL) < 0) {
            LOGD("读取输入的视频流信息失败。");
            return;
        }
        int video_stream_index = -1;//记录视频流所在数组下标
        LOGD("当前视频数据，包含的数据流数量：%d", formatContext->nb_streams);
        //找到"视频流".AVFormatContext 结构体中的nb_streams字段存储的就是当前视频文件中所包含的总数据流数量——
        //视频流，音频流，字幕流
        for (int i = 0; i < formatContext->nb_streams; i++) {

            //如果是数据流的编码格式为AVMEDIA_TYPE_VIDEO——视频流。
            if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                video_stream_index = i;//记录视频流下标
                break;
            }
        }
        if (video_stream_index == -1) {
            LOGD("没有找到 视频流。");
            return;
        }
        //通过编解码器的id——codec_id 获取对应（视频）流解码器
        AVCodecParameters *codecParameters=formatContext->streams[video_stream_index]->codecpar;
        AVCodec *videoDecoder = avcodec_find_decoder(codecParameters->codec_id);

        if (videoDecoder == NULL) {
            LOGD("未找到对应的流解码器。");
            return;
        }
        //通过解码器分配(并用  默认值   初始化)一个解码器context
        AVCodecContext *codecContext = avcodec_alloc_context3(videoDecoder);

        if (codecContext == NULL) {
            LOGD("分配 解码器上下文失败。");
            return;
        }
        //更具指定的编码器值填充编码器上下文
        if(avcodec_parameters_to_context(codecContext,codecParameters)<0){
            LOGD("填充编解码器上下文失败。");
            return;
        }
        //通过所给的编解码器初始化编解码器上下文
        if (avcodec_open2(codecContext, videoDecoder, NULL) < 0) {
            LOGD("初始化 解码器上下文失败。");
            return;
        }
        AVPixelFormat dstFormat = AV_PIX_FMT_RGBA;
        //分配存储压缩数据的结构体对象AVPacket
        //如果是视频流，AVPacket会包含一帧的压缩数据。
        //但如果是音频则可能会包含多帧的压缩数据
        AVPacket *packet = av_packet_alloc();
        //分配解码后的每一数据信息的结构体（指针）
        AVFrame *frame = av_frame_alloc();
        //分配最终显示出来的目标帧信息的结构体（指针）
        AVFrame *outFrame = av_frame_alloc();
        uint8_t *out_buffer = (uint8_t *) av_malloc(
                (size_t) av_image_get_buffer_size(dstFormat, codecContext->width, codecContext->height,
                                                  1));
        //更具指定的数据初始化/填充缓冲区
        av_image_fill_arrays(outFrame->data, outFrame->linesize, out_buffer, dstFormat,
                             codecContext->width, codecContext->height, 1);
        //初始化SwsContext
        SwsContext *swsContext = sws_getContext(
                codecContext->width   //原图片的宽
                ,codecContext->height  //源图高
                ,codecContext->pix_fmt //源图片format
                ,codecContext->width  //目标图的宽
                ,codecContext->height  //目标图的高
                ,dstFormat,SWS_BICUBIC
                , NULL, NULL, NULL
        );
        if(swsContext==NULL){
            LOGD("swsContext==NULL");
            return;
        }
        //Android 原生绘制工具
        ANativeWindow *nativeWindow = ANativeWindow_fromSurface(env, surface);
        //定义绘图缓冲区
        ANativeWindow_Buffer outBuffer;
        //通过设置宽高限制缓冲区中的像素数量，而非屏幕的物流显示尺寸。
        //如果缓冲区与物理屏幕的显示尺寸不相符，则实际显示可能会是拉伸，或者被压缩的图像
        ANativeWindow_setBuffersGeometry(nativeWindow, codecContext->width, codecContext->height,
                                         WINDOW_FORMAT_RGBA_8888);
        //循环读取数据流的下一帧
        while (av_read_frame(formatContext, packet) == 0) {

            if (packet->stream_index == video_stream_index) {
                //讲原始数据发送到解码器
                int sendPacketState = avcodec_send_packet(codecContext, packet);
                if (sendPacketState == 0) {
                    int receiveFrameState = avcodec_receive_frame(codecContext, frame);
                    if (receiveFrameState == 0) {
                        //锁定窗口绘图界面
                        ANativeWindow_lock(nativeWindow, &outBuffer, NULL);
                        //对输出图像进行色彩，分辨率缩放，滤波处理
                        sws_scale(swsContext, (const uint8_t *const *) frame->data, frame->linesize, 0,
                                  frame->height, outFrame->data, outFrame->linesize);
                        uint8_t *dst = (uint8_t *) outBuffer.bits;
                        //解码后的像素数据首地址
                        //这里由于使用的是RGBA格式，所以解码图像数据只保存在data[0]中。但如果是YUV就会有data[0]
                        //data[1],data[2]
                        uint8_t *src = outFrame->data[0];
                        //获取一行字节数
                        int oneLineByte = outBuffer.stride * 4;
                        //复制一行内存的实际数量
                        int srcStride = outFrame->linesize[0];
                        for (int i = 0; i < codecContext->height; i++) {
                            memcpy(dst + i * oneLineByte, src + i * srcStride, srcStride);
                        }
                        //解锁
                        ANativeWindow_unlockAndPost(nativeWindow);
                        //进行短暂休眠。如果休眠时间太长会导致播放的每帧画面有延迟感，如果短会有加速播放的感觉。
                        //一般一每秒60帧——16毫秒一帧的时间进行休眠
                        usleep(1000 * 20);//20毫秒

                    } else if (receiveFrameState == AVERROR(EAGAIN)) {
                        LOGD("从解码器-接收-数据失败：AVERROR(EAGAIN)");
                    } else if (receiveFrameState == AVERROR_EOF) {
                        LOGD("从解码器-接收-数据失败：AVERROR_EOF");
                    } else if (receiveFrameState == AVERROR(EINVAL)) {
                        LOGD("从解码器-接收-数据失败：AVERROR(EINVAL)");
                    } else {
                        LOGD("从解码器-接收-数据失败：未知");
                    }
                } else if (sendPacketState == AVERROR(EAGAIN)) {//发送数据被拒绝，必须尝试先读取数据
                    LOGD("向解码器-发送-数据包失败：AVERROR(EAGAIN)");//解码器已经刷新数据但是没有新的数据包能发送给解码器
                } else if (sendPacketState == AVERROR_EOF) {
                    LOGD("向解码器-发送-数据失败：AVERROR_EOF");
                } else if (sendPacketState == AVERROR(EINVAL)) {//遍解码器没有打开，或者当前是编码器，也或者需要刷新数据
                    LOGD("向解码器-发送-数据失败：AVERROR(EINVAL)");
                } else if (sendPacketState == AVERROR(ENOMEM)) {//数据包无法压如解码器队列，也可能是解码器解码错误
                    LOGD("向解码器-发送-数据失败：AVERROR(ENOMEM)");
                } else {
                    LOGD("向解码器-发送-数据失败：未知");
                }
            }
            av_packet_unref(packet);
        }
        //内存释放
        ANativeWindow_release(nativeWindow);
        av_frame_free(&outFrame);
        av_frame_free(&frame);
        av_packet_free(&packet);
        avcodec_free_context(&codecContext);
        avformat_close_input(&formatContext);
        avformat_free_context(formatContext);
        env->ReleaseStringUTFChars(videoPath, input);
    }
#ifdef __cplusplus
};
#endif
