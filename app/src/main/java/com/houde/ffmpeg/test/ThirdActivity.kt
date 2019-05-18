package com.houde.ffmpeg.test

import android.media.AudioFormat
import android.media.AudioManager
import android.media.AudioTrack
import android.os.Bundle
import android.support.v7.app.AppCompatActivity
import android.view.View
import kotlinx.android.synthetic.main.activity_third.*

open class ThirdActivity : AppCompatActivity() {
    private val inputFilePath = "/storage/emulated/0/GreenCheng/video/g4.mp4"
    private var audioTrack:AudioTrack? = null
    private lateinit var musicPlayer: MusicPlayer
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_third)
        musicPlayer = MusicPlayer(inputFilePath)
    }

    fun play(view: View) {
        surface_view.startPlay(inputFilePath)
    }

    fun playAudio(view: View){
//        Thread(Runnable {
//            Log.d("ThirdActivity", "------>>调用native方法")
//            playAudio(inputFilePath)
//        }).start()
        musicPlayer.playAudio()
    }

    fun stopAudio(view: View){
        musicPlayer.stopAudio()
    }

    /**
     * 这是初始化AudioTrack的方法，也是在C中调用的
     */
    fun createTrack(sampleRateInHn:Int,nbChannel:Int){
        val channelConfig =when (nbChannel) {
            1 ->  AudioFormat.CHANNEL_OUT_MONO
            2 -> AudioFormat.CHANNEL_OUT_STEREO
            else -> AudioFormat.CHANNEL_OUT_MONO
        }
        val bufferSize = AudioTrack.getMinBufferSize(sampleRateInHn,channelConfig,AudioFormat.ENCODING_PCM_16BIT)
        audioTrack = AudioTrack(AudioManager.STREAM_MUSIC,sampleRateInHn,channelConfig,AudioFormat.ENCODING_PCM_16BIT,bufferSize,AudioTrack.MODE_STREAM)
        audioTrack?.play()
    }

    /**
     * 这是在C代码中调用的，就是在解码出PCM就会调用这个方法，让AudioTrack进行播放
     */
    fun playTrack(buffer:ByteArray,length:Int){
        if(audioTrack != null && audioTrack?.playState == AudioTrack.PLAYSTATE_PLAYING){
            audioTrack?.write(buffer,0,length)
        }
    }

    private external fun playAudio(path:String)

    companion object {
        init {
            System.loadLibrary("native-lib")
        }
    }
}
