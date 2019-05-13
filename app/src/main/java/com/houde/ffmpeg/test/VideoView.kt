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
class VideoView @JvmOverloads constructor(context: Context, attrs: AttributeSet? = null, defStyleAttr: Int = 0) :
    SurfaceView(context, attrs, defStyleAttr) {
    lateinit var surface: Surface
    private var ffmpegPlayer: Long = 0

    init {
        init()
    }

    private fun init() {
        holder.setFormat(PixelFormat.RGBA_8888)
        surface = holder.surface
        ffmpegPlayer = getPlayer()
    }

    /**
     * 开始播放
     * @param videoPath
     */
    fun startPlay(videoPath: String) {
        Thread(Runnable {
            Log.d("MyVideoView", "------>>调用native方法")
            play(ffmpegPlayer,videoPath, surface)
        }).start()
    }
    private external fun play (ffmpegPlayer:Long,path:String, surface: Surface)

    private external fun getPlayer():Long
    private external fun pause(ffmpegPlayer:Long)
    private external fun stop(ffmpegPlayer:Long)
    companion object {
        init {
            System.loadLibrary("native-lib")
        }
    }


}

