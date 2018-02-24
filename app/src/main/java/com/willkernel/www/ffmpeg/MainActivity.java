package com.willkernel.www.ffmpeg;

import android.Manifest;
import android.os.Build;
import android.os.Environment;
import android.support.annotation.NonNull;
import android.support.v4.app.ActivityCompat;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.widget.TextView;

import java.io.File;

public class MainActivity extends AppCompatActivity {

    private VideoView videoView;
    private VideoPlayer videoPlayer;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            requestPermissions(new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE,
                    Manifest.permission.READ_EXTERNAL_STORAGE,
                    Manifest.permission.RECORD_AUDIO,
                    Manifest.permission.MOUNT_UNMOUNT_FILESYSTEMS}, 100);
        }
        videoView = findViewById(R.id.videoView);
        videoPlayer=new VideoPlayer();
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);

    }

    public void decode(View view) {
        String inputVideo = Environment.getExternalStorageDirectory().getAbsolutePath() + File.separatorChar
                + "input.mp4";

        String outputVideo = Environment.getExternalStorageDirectory().getAbsolutePath() + File.separatorChar
                + "output_1280x720_yuv420p.yuv";

        VideoUtils.decode(inputVideo, outputVideo);
    }

    public void decodeAudio(View view) {
        String inputAudio = Environment.getExternalStorageDirectory().getAbsolutePath() + File.separatorChar
                + "cb.mp3";

        String outputAudio = Environment.getExternalStorageDirectory().getAbsolutePath() + File.separatorChar
                + "cb.pcm";

        VideoUtils.decodeAudio(inputAudio, outputAudio);
    }

    public void playAudio(View view) {
        String inputVideo = Environment.getExternalStorageDirectory().getAbsolutePath() + File.separatorChar
                + "cb.mp3";

        videoPlayer.play(inputVideo);

    }

    public void play(View view) {
        String inputVideo = Environment.getExternalStorageDirectory().getAbsolutePath() + File.separatorChar
                + "input.mp4";
        videoPlayer.render(inputVideo, videoView.getHolder().getSurface());
    }


}
