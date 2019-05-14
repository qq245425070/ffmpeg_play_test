//
// Created by houde on 2019/5/14.
//

#ifndef FFMPEG_TEST_FFMPEGMUSIC_H
#define FFMPEG_TEST_FFMPEGMUSIC_H
#include <jni.h>
#include <string>
#include <android/log.h>
extern "C" {
//编码
#include "libavcodec/avcodec.h"
//封装格式处理
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
//像素处理
#include "libswscale/swscale.h"
#include <android/native_window_jni.h>
#include <unistd.h>
}
#define FFLOGI(FORMAT,...) __android_log_print(ANDROID_LOG_INFO,"ffmpeg_test",FORMAT,##__VA_ARGS__);
#define LOGD(FORMAT,...) __android_log_print(ANDROID_LOG_INFO,"ffmpeg_test",FORMAT,##__VA_ARGS__);
#define FFLOGE(FORMAT,...) __android_log_print(ANDROID_LOG_ERROR,"ffmpeg_test",FORMAT,##__VA_ARGS__);
#define LOGE(FORMAT,...) __android_log_print(ANDROID_LOG_ERROR,"ffmpeg_test",FORMAT,##__VA_ARGS__);

int createFFmpeg(int *rate,int *channel,const char* path);

int getPcm(void **pcm,size_t *pcm_size);

void releaseFFmpeg();
#endif //FFMPEG_TEST_FFMPEGMUSIC_H
