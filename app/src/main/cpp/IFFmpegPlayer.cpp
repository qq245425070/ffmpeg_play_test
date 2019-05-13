//
// Created by houde on 2019/5/13.
//

#include "FFmpegPlayer.h"
#include <jni.h>
#include <android/native_window_jni.h>
#include <android/native_window.h>

extern "C"
JNIEXPORT jlong JNICALL
Java_com_houde_ffmpeg_test_VideoView_getPlayer(JNIEnv* env,jobject instance){
    FFmpegPlayer* player = new FFmpegPlayer();
    return long(player);
}
extern "C"
JNIEXPORT void JNICALL
Java_com_houde_ffmpeg_test_VideoView_play(JNIEnv* env,jobject instance,jlong fFmpegPlayer,jstring inputPath,jobject surface){
    FFmpegPlayer* player = (FFmpegPlayer*) fFmpegPlayer;
    const char* path = env->GetStringUTFChars(inputPath, 0);
    ANativeWindow* nativeWindow = ANativeWindow_fromSurface(env,surface);
    player->play(path,nativeWindow);
    env->ReleaseStringUTFChars(inputPath,path);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_houde_ffmpeg_test_VideoView_pause(JNIEnv* env,jobject instance,jlong ffmpegPlayer){
    FFmpegPlayer* player = (FFmpegPlayer*) ffmpegPlayer;
    player->pause();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_houde_ffmpeg_test_VideoView_stop(JNIEnv * env,jobject instance,jlong ffmpegPlayer){
    FFmpegPlayer * player = (FFmpegPlayer*) ffmpegPlayer;
    player->stop();
}