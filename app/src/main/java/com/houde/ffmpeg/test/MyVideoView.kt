package com.houde.ffmpeg.test

import android.content.Context
import android.graphics.PixelFormat
import android.util.AttributeSet
import android.util.Log
import android.view.Surface
import android.view.SurfaceView

/**
 * @author : Houde
 * Date : 2019/5/12 15:19
 * Desc :
 */
class MyVideoView @JvmOverloads constructor(context: Context, attrs: AttributeSet? = null, defStyleAttr: Int = 0) :
    SurfaceView(context, attrs, defStyleAttr) {
    lateinit var surface: Surface

    init {
        init()
    }

    private fun init() {
        holder.setFormat(PixelFormat.RGBA_8888)
        surface = holder.surface
    }

    /**
     * 开始播放
     * @param videoPath
     */
    fun startPlay(videoPath: String) {
        Thread(Runnable {
            Log.d("MyVideoView", "------>>调用native方法")
            play(videoPath, surface)
        }).start()
    }
    private external fun play (path:String, surface: Surface)
    companion object {
        init {
            System.loadLibrary("native-lib")
        }
    }


}

