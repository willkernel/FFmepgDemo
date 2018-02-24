package com.willkernel.www.ffmpeg;

import android.content.Context;
import android.graphics.PixelFormat;
import android.util.AttributeSet;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

/**
 * Created by willkernel on 2018/1/17.
 */

public class VideoView extends SurfaceView {
    public VideoView(Context context) {
        this(context,null,0);
    }

    public VideoView(Context context, AttributeSet attrs) {
        this(context,attrs,0);
    }

    public VideoView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        init();
    }

    private void init() {
        SurfaceHolder surfaceHolder=getHolder();
        //初始化像素绘制的格式为RGBA_8888（色彩最丰富）
        surfaceHolder.setFormat(PixelFormat.RGBA_8888);
    }
}
