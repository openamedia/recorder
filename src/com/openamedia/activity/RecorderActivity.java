package com.openamedia.activity;

import java.text.SimpleDateFormat;

import com.openamedia.recorder.ARecorder;
import com.openamedia.recorder.ARecorder.ARecorderListener;
import com.openamedia.recorder.R;

import android.support.v7.app.ActionBarActivity;
import android.util.Log;
import android.view.KeyEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceHolder.Callback;
import android.view.SurfaceView;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.ImageButton;
import android.hardware.Camera;
import android.hardware.Camera.CameraInfo;
import android.hardware.Camera.PreviewCallback;
import android.media.MediaCodecInfo;
import android.os.Bundle;
import android.os.Environment;

public class RecorderActivity extends ActionBarActivity {

	ImageButton mButton = null;
	ARecorder mRecorder = null;
	Camera mCamera = null;
	boolean mPreviewRunning = false;
	
	SurfaceView mSurfaceView = null;
	
	enum State {
		STATE_INITIALIZED,
		STATE_STARTED,
		STATE_STOPPED,
	}
	State mState = State.STATE_INITIALIZED;
	
	private MyRecorderListener mRecorderListener = new MyRecorderListener();
	
	private class MyRecorderListener implements ARecorderListener {

		@Override
		public void onStart() {
			if(mPreviewRunning){
				mCamera.stopPreview();
				mPreviewRunning = false;
			}
			
	        Camera.Parameters p = mCamera.getParameters();
	        p.setPreviewSize(320, 240);
	        p.setPreviewFpsRange(10, 15);
	        mCamera.setParameters(p);
	        
	        mCamera.setPreviewCallback(new PreviewCallback(){
				@Override
				public void onPreviewFrame(byte[] arg0, Camera arg1) {
					mRecorder.writeVideo(arg0, 320 * 240 * 3 / 2);
				}
	        });
	        
	        try {
	            mCamera.setPreviewDisplay(mSurfaceView.getHolder());
	        } catch (Exception ex) {
	        }
	        
	        mCamera.startPreview();
	        
	        mPreviewRunning = true;
		}
		
	}
	
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
                
        mButton = (ImageButton)this.findViewById(R.id.imageButton1);
        mButton.setClickable(true);
        mButton.setOnClickListener(new OnClickListener(){

			@Override
			public void onClick(View arg0) {
				if(mState == State.STATE_INITIALIZED){
					if(mRecorder == null){
						mRecorder = new ARecorder();
						SimpleDateFormat sDateFormat = new SimpleDateFormat("/yyyy-MM-dd_hh-mm-ss");
						String date = sDateFormat.format(new java.util.Date());
						String path = Environment.getExternalStorageDirectory().toString() + date + ".flv";
						Log.e("RecorderActivity", path);
						mRecorder.setOutputFile(path);
						mRecorder.setListener(mRecorderListener);
						mRecorder.setChannels(2);
						mRecorder.setSampleRate(44100);
						mRecorder.setVideoSize(320, 240);
						mRecorder.setColorFormat("nv21");
						mRecorder.start();
					}
			        
					mState = State.STATE_STARTED;
					mButton.setBackgroundResource(R.drawable.controller_stop);
				}else if(mState == State.STATE_STARTED){
					if(mPreviewRunning){
						mCamera.stopPreview();
						mPreviewRunning = false;
					}
					//mRecorder.stop();
					if(mRecorder != null){
						mRecorder.release();
						mRecorder = null;
					}
					mState = State.STATE_INITIALIZED;
					mButton.setBackgroundResource(R.drawable.controller_record);
				}
			}
        	
        });
        
        mSurfaceView = (SurfaceView)this.findViewById(R.id.surfaceView1);
        mSurfaceView.getHolder().setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);//what the fuck?? but it is necessary, otherwise it will crash at startpreview.
        mSurfaceView.getHolder().addCallback(new Callback(){

			@Override
			public void surfaceChanged(SurfaceHolder arg0, int arg1, int arg2,
					int arg3) {
				if(mPreviewRunning){
					mCamera.stopPreview();
					mPreviewRunning = false;
				}
				
		        Camera.Parameters p = mCamera.getParameters();
		        p.setPreviewSize(320, 240);
		        mCamera.setParameters(p);
		        
		        try {
		            mCamera.setPreviewDisplay(mSurfaceView.getHolder());
		        } catch (Exception ex) {
		        }  
		        
		        mCamera.startPreview();
		        
		        mPreviewRunning = true;
			}

			@Override
			public void surfaceCreated(SurfaceHolder arg0) {
		        for(int i = 0; i < Camera.getNumberOfCameras(); ++i){
		        	CameraInfo info = new CameraInfo();
		        	Camera.getCameraInfo(i, info);
		        	if(info.facing == CameraInfo.CAMERA_FACING_FRONT){
		        		mCamera = Camera.open(i);
		        		break;
		        	}
		        }
			}

			@Override
			public void surfaceDestroyed(SurfaceHolder arg0) {
				if(mRecorder != null){
					mRecorder.release();
					mRecorder = null;
				}
				
		        if (mCamera != null) {  
		            mCamera.setPreviewCallback(null);  
		            mCamera.stopPreview();
		            mPreviewRunning = false;
		            mCamera.release();
		            mCamera = null;  
		        }
			}
        	
        });
    }
    
	@Override 
	public boolean onKeyDown(int keyCode, KeyEvent event) {
				
		return super.onKeyDown(keyCode, event);
	}
}
