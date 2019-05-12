package com.houde.ffmpeg.test

import android.Manifest
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import android.support.v7.app.AppCompatActivity
import android.util.Log

class SecondActivity : AppCompatActivity() {

    private val permissionRequestCode = 1
    private val inputFilePath = "/storage/emulated/0/GreenCheng/video/123.mp4"
    private val outputFilePath = "/storage/emulated/0/GreenCheng/video/abc.yuv"
    private val permissions =
        arrayOf(Manifest.permission.WRITE_EXTERNAL_STORAGE, Manifest.permission.READ_EXTERNAL_STORAGE)
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_second)
        requestPermissions()
    }
    private fun requestPermissions() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            for (i in permissions.indices) {

                if (checkSelfPermission(permissions[0]) == PackageManager.PERMISSION_DENIED) {
                    requestPermissions(permissions, permissionRequestCode)
                    return
                }
            }
            Log.e("TAG","decoder1")
            decoder(inputFilePath, outputFilePath)
        } else {
            Log.e("TAG","decoder2")
            decoder(inputFilePath, outputFilePath)
        }
    }
    override fun onRequestPermissionsResult(requestCode: Int, permissions: Array<String>, grantResults: IntArray) {

        if (requestCode == permissionRequestCode) {

            for (result in grantResults) {
                if (result == PackageManager.PERMISSION_DENIED) {
                    return
                }
            }
            Log.e("TAG","decoder3")
            decoder(inputFilePath, outputFilePath)

        }
    }

    external fun decoder(inputPath:String,outputString:String)

    companion object {
        // Used to load the 'native-lib' library on application startup.
        init {
            System.loadLibrary("native-lib")
        }
    }
}
