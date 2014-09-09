package com.openamedia.activity;

import java.text.SimpleDateFormat;

import com.openamedia.recorder.ARecorder;
import com.openamedia.recorder.R;

import android.support.v7.app.ActionBarActivity;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.ImageButton;
import android.os.Bundle;
import android.os.Environment;

public class RecorderActivity extends ActionBarActivity {

	ImageButton mButton = null;
	ARecorder mRecorder = null;
	
	enum State {
		STATE_INITIALIZED,
		STATE_STARTED,
		STATE_STOPPED,
	}
	State mState = State.STATE_INITIALIZED;
	
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
						String path = Environment.getExternalStorageDirectory().toString() + date + ".mp4";
						Log.e("RecorderActivity", path);
						mRecorder.setOutputFile(path);
						mRecorder.start();
					}
					mState = State.STATE_STARTED;
					mButton.setBackgroundResource(R.drawable.controller_stop);
				}else if(mState == State.STATE_STARTED){
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
    }
    
	@Override 
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		if (keyCode == KeyEvent.KEYCODE_BACK){
			if(mRecorder != null){
				mRecorder.release();
				mRecorder = null;
			}
		}
				
		return super.onKeyDown(keyCode, event);
	}


}
