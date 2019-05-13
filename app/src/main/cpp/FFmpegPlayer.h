//
// Created by houde on 2019/5/13.
//

#ifndef FFMPEG_TEST_FFMPEGPLAYER_H
#define FFMPEG_TEST_FFMPEGPLAYER_H
#include "String"
#include <jni.h>
#include <string>
#include <android/log.h>
#include <android/native_window.h>
class FFmpegPlayer{
private:
    char* videoPath;
    void setVideoPath(char* videoPath);

public:
    FFmpegPlayer();

    void play(const char* videoPath,ANativeWindow* window);
    void pause();
    void stop();
};


#endif //FFMPEG_TEST_FFMPEGPLAYER_H
