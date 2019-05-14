#include <jni.h>
#include <string>
#include "android/log.h"
#include <android/native_window_jni.h>
#include <unistd.h>
#include "stdio.h"
#include "SLES/OpenSLES.h"
#include "SLES/OpenSLES_Android.h"
#include "FFmpegMusic.h"
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
#include "libswresample/swresample.h"



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
        //输出视频信息
        FFLOGI("视频的文件格式：%s",formatContext->iformat->name);
        FFLOGI("视频时长：%d", (formatContext->duration)/1000000);
        FFLOGI("视频的宽高：%d,%d",codecContext->width,codecContext->height);
        FFLOGI("解码器的名称：%s",videoDecoder->name);
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

extern "C"
JNIEXPORT void JNICALL
Java_com_houde_ffmpeg_test_ThirdActivity_playAudio(JNIEnv * env, jobject instance,jstring audioPath){
    const char* path = env->GetStringUTFChars(audioPath,0);
    av_register_all();
    AVFormatContext* pFormatContext = avformat_alloc_context();
    if(avformat_open_input(&pFormatContext,path,NULL,NULL)!=0){
        LOGE("%s","不能打开文件");
        return;
    }
    if(avformat_find_stream_info(pFormatContext,NULL) < 0){
        LOGE("%s","没有找到流文件");
        return;
    }
    av_dump_format(pFormatContext,0,path,0);
    int audio_index = -1;
    for(int i = 0 ; i < pFormatContext->nb_streams; i ++){
        if(pFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO){
            audio_index = i;
            break;
        }
    }
    if(audio_index == -1){
        LOGE("%s","没有找到音频流");
        return;
    }
    AVCodecParameters* pCodecParams = pFormatContext->streams[audio_index]->codecpar;
    AVCodecID  pCodecId = pCodecParams->codec_id;
    if(pCodecId == NULL){
        LOGE("%s","CodecID == null");
        return;
    }
    AVCodec* pCodec = avcodec_find_decoder(pCodecId);
    if(pCodec == NULL){
        LOGE("%s","没有找到解码器");
        return;
    }
    AVCodecContext* pCodecContext = avcodec_alloc_context3(pCodec);
    if(pCodecContext == NULL){
        LOGE("%s","不能为CodecContext分配内存");
        return;
    }
    if(avcodec_parameters_to_context(pCodecContext,pCodecParams)<0){
        LOGE("%s","创建codecContext失败");
        return;
    }
    if(avcodec_open2(pCodecContext,pCodec,NULL) <0 ){
        LOGE("%s","打开解码器失败");
        return;
    }
    AVPacket *avp = av_packet_alloc();
    AVFrame *avf = av_frame_alloc();

    //frame->16bit 44100 PCM 统一音频采样格式与采样率
    SwrContext* swr_cxt = swr_alloc();
    //重采样设置选项-----------------------------------------------------------start
    //输入的采样格式
    enum AVSampleFormat in_sample_fmt = pCodecContext->sample_fmt;
    //输出的采样格式
    enum AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;
    //输入的采样率
    int in_sample_rate = pCodecContext->sample_rate;
    printf("sample rate = %d \n" ,in_sample_rate);
    //输出的采样率
    int out_sample_rate = 44100;
    //输入的声道布局
    uint64_t in_ch_layout = pCodecContext->channel_layout;
    //输出的声道布局
    uint64_t out_ch_layout = AV_CH_LAYOUT_MONO;
    //SwrContext 设置参数
    swr_alloc_set_opts(swr_cxt,out_ch_layout,out_sample_fmt,out_sample_rate,in_ch_layout,in_sample_fmt,in_sample_rate,0,NULL);
    //初始化SwrContext
    swr_init(swr_cxt);
    //重采样设置选项-----------------------------------------------------------end
    //获取输出的声道个数
    int out_channel_nb = av_get_channel_layout_nb_channels(out_ch_layout);
    jclass clazz = env->GetObjectClass(instance);
    //调用Java方法MethodID
    jmethodID methodId = env->GetMethodID(clazz,"createTrack","(II)V");
    jmethodID methodID1 = env->GetMethodID(clazz,"playTrack","([BI)V");
    int gotFrame;
    //通过methodId调用Java方法
    env->CallVoidMethod(instance,methodId,44100,out_channel_nb);
    //存储pcm数据
    uint8_t *out_buf = (uint8_t*)av_malloc(2*44100);
    int got_frame, frame_count = 0;
    //6.一帧一帧读取压缩的音频数据AVPacket
    int ret;
    while(av_read_frame(pFormatContext,avp) >= 0){
        if(avp->stream_index == audio_index){
            //解码从avpacket到avframe
            ret = avcodec_decode_audio4(pCodecContext,avf,&got_frame,avp);
            // =0 表示解码完成
            if(ret < 0){
                av_log(NULL,AV_LOG_INFO,"解码完成  \n");
            }
            //表示正在解码
            if(got_frame != 0){
                LOGE("正在解码第%d帧  \n",++frame_count);
                swr_convert(swr_cxt , &out_buf , 2 * 44100 , (const uint8_t **)avf->data , avf->nb_samples);
                //获取sample的size
                int out_buf_size = av_samples_get_buffer_size(NULL,out_channel_nb,avf->nb_samples,out_sample_fmt,1);
                jbyteArray audioArray = env->NewByteArray(out_buf_size);
                env->SetByteArrayRegion(audioArray,0,out_buf_size,(const jbyte*)out_buf);
                //调用Java方法
                env->CallVoidMethod(instance,methodID1,audioArray,out_buf_size);
                env->DeleteLocalRef(audioArray);
            }
        }
        av_packet_unref(avp);
    }
    av_frame_free(&avf);
    swr_free(&swr_cxt);
    avcodec_close(pCodecContext);
    avformat_close_input(&pFormatContext);
    env->ReleaseStringUTFChars(audioPath,path);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_houde_ffmpeg_test_MusicPlayer_playAudio(JNIEnv * env, jobject instance,jstring audioPath){
    const char* path = env->GetStringUTFChars(audioPath,0);
    av_register_all();
    AVFormatContext* pFormatContext = avformat_alloc_context();
    if(avformat_open_input(&pFormatContext,path,NULL,NULL)!=0){
        LOGE("%s","不能打开文件");
        return;
    }
    if(avformat_find_stream_info(pFormatContext,NULL) < 0){
        LOGE("%s","没有找到流文件");
        return;
    }
    av_dump_format(pFormatContext,0,path,0);
    int audio_index = -1;
    for(int i = 0 ; i < pFormatContext->nb_streams; i ++){
        if(pFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO){
            audio_index = i;
            break;
        }
    }
    if(audio_index == -1){
        LOGE("%s","没有找到音频流");
        return;
    }
    AVCodecParameters* pCodecParams = pFormatContext->streams[audio_index]->codecpar;
    AVCodecID  pCodecId = pCodecParams->codec_id;
    if(pCodecId == NULL){
        LOGE("%s","CodecID == null");
        return;
    }
    AVCodec* pCodec = avcodec_find_decoder(pCodecId);
    if(pCodec == NULL){
        LOGE("%s","没有找到解码器");
        return;
    }
    AVCodecContext* pCodecContext = avcodec_alloc_context3(pCodec);
    if(pCodecContext == NULL){
        LOGE("%s","不能为CodecContext分配内存");
        return;
    }
    if(avcodec_parameters_to_context(pCodecContext,pCodecParams)<0){
        LOGE("%s","创建codecContext失败");
        return;
    }
    if(avcodec_open2(pCodecContext,pCodec,NULL) <0 ){
        LOGE("%s","打开解码器失败");
        return;
    }
    AVPacket *avp = av_packet_alloc();
    AVFrame *avf = av_frame_alloc();

    //frame->16bit 44100 PCM 统一音频采样格式与采样率
    SwrContext* swr_cxt = swr_alloc();
    //重采样设置选项-----------------------------------------------------------start
    //输入的采样格式
    enum AVSampleFormat in_sample_fmt = pCodecContext->sample_fmt;
    //输出的采样格式
    enum AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;
    //输入的采样率
    int in_sample_rate = pCodecContext->sample_rate;
    printf("sample rate = %d \n" ,in_sample_rate);
    //输出的采样率
    int out_sample_rate = 44100;
    //输入的声道布局
    uint64_t in_ch_layout = pCodecContext->channel_layout;
    //输出的声道布局
    uint64_t out_ch_layout = AV_CH_LAYOUT_MONO;
    //SwrContext 设置参数
    swr_alloc_set_opts(swr_cxt,out_ch_layout,out_sample_fmt,out_sample_rate,in_ch_layout,in_sample_fmt,in_sample_rate,0,NULL);
    //初始化SwrContext
    swr_init(swr_cxt);
    //重采样设置选项-----------------------------------------------------------end
    //获取输出的声道个数
    int out_channel_nb = av_get_channel_layout_nb_channels(out_ch_layout);
    jclass clazz = env->GetObjectClass(instance);
    //调用Java方法MethodID
    jmethodID methodId = env->GetMethodID(clazz,"createTrack","(II)V");
    jmethodID methodID1 = env->GetMethodID(clazz,"playTrack","([BI)V");
    //通过methodId调用Java方法
    env->CallVoidMethod(instance,methodId,44100,out_channel_nb);
    //存储pcm数据
    uint8_t *out_buf = (uint8_t*)av_malloc(2*44100);
    int got_frame, frame_count = 0;
    //6.一帧一帧读取压缩的音频数据AVPacket
    int ret;
    while(av_read_frame(pFormatContext,avp) >= 0){
        if(avp->stream_index == audio_index){
            //解码从avpacket到avframe
            ret = avcodec_decode_audio4(pCodecContext,avf,&got_frame,avp);
            // =0 表示解码完成
            if(ret < 0){
                av_log(NULL,AV_LOG_INFO,"解码完成  \n");
            }
            //表示正在解码
            if(got_frame != 0){
                LOGE("正在解码第%d帧  \n",++frame_count);
                swr_convert(swr_cxt , &out_buf , 2 * 44100 , (const uint8_t **)avf->data , avf->nb_samples);
                //获取sample的size
                int out_buf_size = av_samples_get_buffer_size(NULL,out_channel_nb,avf->nb_samples,out_sample_fmt,1);
                jbyteArray audioArray = env->NewByteArray(out_buf_size);
                env->SetByteArrayRegion(audioArray,0,out_buf_size,(const jbyte*)out_buf);
                //调用Java方法
                env->CallVoidMethod(instance,methodID1,audioArray,out_buf_size);
                env->DeleteLocalRef(audioArray);
            }
        }
        av_packet_unref(avp);
    }
    av_frame_free(&avf);
    swr_free(&swr_cxt);
    avcodec_close(pCodecContext);
    avformat_close_input(&pFormatContext);
    env->ReleaseStringUTFChars(audioPath,path);
}

//--------------------------------------openSL ES 播放声音--------------------------------------------------------------
SLObjectItf engineObject=NULL;//用SLObjectItf声明引擎接口对象
SLEngineItf engineEngine = NULL;//声明具体的引擎对象


SLObjectItf outputMixObject = NULL;//用SLObjectItf创建混音器接口对象
SLEnvironmentalReverbItf outputMixEnvironmentalReverb = NULL;////具体的混音器对象实例
SLEnvironmentalReverbSettings settings = SL_I3DL2_ENVIRONMENT_PRESET_DEFAULT;//默认情况


SLObjectItf audioplayer=NULL;//用SLObjectItf声明播放器接口对象
SLPlayItf  slPlayItf=NULL;//播放器接口
SLAndroidSimpleBufferQueueItf  slBufferQueueItf=NULL;//缓冲区队列接口


size_t buffersize =0;
void *buffer;
//将pcm数据添加到缓冲区中
void getQueueCallBack(SLAndroidSimpleBufferQueueItf  slBufferQueueItf, void* context){

    buffersize=0;

    getPcm(&buffer,&buffersize);
    if(buffer!=NULL&&buffersize!=0){
        //将得到的数据加入到队列中
        (*slBufferQueueItf)->Enqueue(slBufferQueueItf,buffer,buffersize);
    }
}

//创建引擎
void createEngine(){
    slCreateEngine(&engineObject,0,NULL,0,NULL,NULL);//创建引擎
    (*engineObject)->Realize(engineObject,SL_BOOLEAN_FALSE);//实现engineObject接口对象
    (*engineObject)->GetInterface(engineObject,SL_IID_ENGINE,&engineEngine);//通过引擎调用接口初始化SLEngineItf
}

//创建混音器
void createMixVolume(){
    (*engineEngine)->CreateOutputMix(engineEngine,&outputMixObject,0,0,0);//用引擎对象创建混音器接口对象
    (*outputMixObject)->Realize(outputMixObject,SL_BOOLEAN_FALSE);//实现混音器接口对象
    SLresult   sLresult = (*outputMixObject)->GetInterface(outputMixObject,SL_IID_ENVIRONMENTALREVERB,&outputMixEnvironmentalReverb);//利用混音器实例对象接口初始化具体的混音器对象
    //设置
    if (SL_RESULT_SUCCESS == sLresult) {
        (*outputMixEnvironmentalReverb)->
                SetEnvironmentalReverbProperties(outputMixEnvironmentalReverb, &settings);
    }
}

//创建播放器
void createPlayer(const char* path){
    //初始化ffmpeg
    int rate;
    int channels;
    createFFmpeg(&rate,&channels,path);
    LOGE("RATE %d",rate);
    LOGE("channels %d",channels);
    /*
     * typedef struct SLDataLocator_AndroidBufferQueue_ {
    SLuint32    locatorType;//缓冲区队列类型
    SLuint32    numBuffers;//buffer位数
} */

    SLDataLocator_AndroidBufferQueue android_queue = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,2};
    /**
    typedef struct SLDataFormat_PCM_ {
        SLuint32 		formatType;  pcm
        SLuint32 		numChannels;  通道数
        SLuint32 		samplesPerSec;  采样率
        SLuint32 		bitsPerSample;  采样位数
        SLuint32 		containerSize;  包含位数
        SLuint32 		channelMask;     立体声
        SLuint32		endianness;    end标志位
    } SLDataFormat_PCM;
     */
    SLDataFormat_PCM pcm = {SL_DATAFORMAT_PCM,(SLuint32)channels,(SLuint32)rate*1000
            ,SL_PCMSAMPLEFORMAT_FIXED_16
            ,SL_PCMSAMPLEFORMAT_FIXED_16
            ,SL_SPEAKER_FRONT_LEFT|SL_SPEAKER_FRONT_RIGHT,SL_BYTEORDER_LITTLEENDIAN};

    /*
     * typedef struct SLDataSource_ {
	        void *pLocator;//缓冲区队列
	        void *pFormat;//数据样式,配置信息
        } SLDataSource;
     * */
    SLDataSource dataSource = {&android_queue,&pcm};


    SLDataLocator_OutputMix slDataLocator_outputMix={SL_DATALOCATOR_OUTPUTMIX,outputMixObject};


    SLDataSink slDataSink = {&slDataLocator_outputMix,NULL};


    const SLInterfaceID ids[3]={SL_IID_BUFFERQUEUE,SL_IID_EFFECTSEND,SL_IID_VOLUME};
    const SLboolean req[3]={SL_BOOLEAN_FALSE,SL_BOOLEAN_FALSE,SL_BOOLEAN_FALSE};

    /*
     * SLresult (*CreateAudioPlayer) (
		SLEngineItf self,
		SLObjectItf * pPlayer,
		SLDataSource *pAudioSrc,//数据设置
		SLDataSink *pAudioSnk,//关联混音器
		SLuint32 numInterfaces,
		const SLInterfaceID * pInterfaceIds,
		const SLboolean * pInterfaceRequired
	);
     * */
    LOGE("执行到此处")
    (*engineEngine)->CreateAudioPlayer(engineEngine,&audioplayer,&dataSource,&slDataSink,3,ids,req);
    (*audioplayer)->Realize(audioplayer,SL_BOOLEAN_FALSE);
    LOGE("执行到此处2")
    (*audioplayer)->GetInterface(audioplayer,SL_IID_PLAY,&slPlayItf);//初始化播放器
    //注册缓冲区,通过缓冲区里面 的数据进行播放
    (*audioplayer)->GetInterface(audioplayer,SL_IID_BUFFERQUEUE,&slBufferQueueItf);
    //设置回调接口
    (*slBufferQueueItf)->RegisterCallback(slBufferQueueItf,getQueueCallBack,NULL);
    //播放
    (*slPlayItf)->SetPlayState(slPlayItf,SL_PLAYSTATE_PLAYING);

    //开始播放
    getQueueCallBack(slBufferQueueItf,NULL);

}
//释放资源
void releaseResource(){
    if(audioplayer!=NULL){
        (*audioplayer)->Destroy(audioplayer);
        audioplayer=NULL;
        slBufferQueueItf=NULL;
        slPlayItf=NULL;
    }
    if(outputMixObject!=NULL){
        (*outputMixObject)->Destroy(outputMixObject);
        outputMixObject=NULL;
        outputMixEnvironmentalReverb=NULL;
    }
    if(engineObject!=NULL){
        (*engineObject)->Destroy(engineObject);
        engineObject=NULL;
        engineEngine=NULL;
    }
    releaseFFmpeg();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_houde_ffmpeg_test_MusicPlayer_play(JNIEnv *env, jobject instance,jstring audioPath) {
    createEngine();
    createMixVolume();
    const char* path = env->GetStringUTFChars(audioPath,0);
    createPlayer(path);
    env->ReleaseStringUTFChars(audioPath,path);
}


extern "C"
JNIEXPORT void JNICALL
Java_com_houde_ffmpeg_test_MusicPlayer_stop(JNIEnv *env, jobject instance) {
    releaseResource();
}
#ifdef __cplusplus
};
#endif
