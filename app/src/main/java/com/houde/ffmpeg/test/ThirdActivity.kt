package com.houde.ffmpeg.test

import android.os.Bundle
import android.support.v7.app.AppCompatActivity
import android.view.View
import kotlinx.android.synthetic.main.activity_third.*

open class ThirdActivity : AppCompatActivity() {
    private val inputFilePath = "/storage/emulated/0/GreenCheng/video/123.mp4"
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_third)
    }

    fun play(view: View) {
        surface_view.startPlay(inputFilePath)
    }
}
