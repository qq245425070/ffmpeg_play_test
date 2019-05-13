package com.houde.ffmpeg.test

import android.media.AudioFormat
import android.media.AudioManager
import android.media.AudioTrack

/**
 * @author : Houde
 * Date : 2019/5/13 17:49
 * Desc :
 */
class MusicPlayer(path: String) {
    companion object {
        init {
            System.loadLibrary("native-lib")
        }
    }
    private lateinit var audioTrack:AudioTrack
    private var audioPath:String = path
    open fun createTrack(sampleRateInHn:Int,nbChannel:Int){
        val channelConfig =when (nbChannel) {
            1 ->  AudioFormat.CHANNEL_OUT_MONO
            2 -> AudioFormat.CHANNEL_OUT_STEREO
            else -> AudioFormat.CHANNEL_OUT_MONO
        }
        val bufferSize = AudioTrack.getMinBufferSize(sampleRateInHn,channelConfig, AudioFormat.ENCODING_PCM_16BIT)
        audioTrack = AudioTrack(
            AudioManager.STREAM_MUSIC,sampleRateInHn,channelConfig,
            AudioFormat.ENCODING_PCM_16BIT,bufferSize,
            AudioTrack.MODE_STREAM)
        audioTrack.play()
    }

    open fun playTrack(buffer:ByteArray,length:Int){
        if(audioTrack.playState == AudioTrack.PLAYSTATE_PLAYING){
            audioTrack.write(buffer,0,length)
        }
    }

    fun playAudio(){
       Thread(Runnable { playAudio(audioPath) }).start()
    }

    private external fun playAudio(path:String)

}