package com.houde.ffmpeg.test

import android.content.Intent
import android.os.Bundle
import android.provider.MediaStore
import android.support.v7.app.AppCompatActivity
import kotlinx.android.synthetic.main.activity_main.*

class MainActivity : AppCompatActivity() {


    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        // Example of a call to a native method
        sample_text.text = stringFromJNI()
        sample_text.setOnClickListener { startActivity(Intent(this,ThirdActivity::class.java)) }
//        var uri = Uri.parse(inputFilePath)
//        //调用系统自带的播放器
//        var intent = Intent(Intent.ACTION_VIEW)
//        Log.v("TAG", uri.toString())
//        intent.setDataAndType(uri, "video/mp4")
//        startActivity(intent)
//        var list = getVideoFromSDCard()
//        Log.e("TAG",list.toString())
//        var file = File(Environment.getExternalStorageState() + File.separator + "decoder")
//        if(!file.exists()){
//            file.mkdirs()
//        }
//        var inputFile = File(list[0])
//        Log.e("TAG",inputFile.absolutePath)
//        Log.e("TAG","length = " + inputFile.length())
//        decoder(list[0], file.absolutePath + File.separator + "abc.mp4")
    }


    /**
    * 从本地得到所有的视频地址
    */
    private fun getVideoFromSDCard():List<String> {
        var list = ArrayList<String>(10)
        var projection:Array<String> = arrayOf(MediaStore.Video.Media.DATA)
        var cursor = contentResolver.query(
            MediaStore.Video.Media.EXTERNAL_CONTENT_URI, projection, null,
            null, null)
        while (cursor.moveToNext()) {
            var path = cursor.getString(cursor
                .getColumnIndexOrThrow(MediaStore.Video.Media.DATA))
            list.add(path)
        }
        cursor.close()
        return list
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    external fun stringFromJNI(): String

    companion object {

        // Used to load the 'native-lib' library on application startup.
        init {
            System.loadLibrary("native-lib")
        }
    }
}
