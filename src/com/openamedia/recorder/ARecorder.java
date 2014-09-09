package com.openamedia.recorder;

import java.lang.ref.WeakReference;

import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.view.Surface;
import android.view.SurfaceHolder;

public class ARecorder {
	
	private Handler mEventHandler = null;
	
	private class EventHandler extends Handler {
		private ARecorder mRecorder = null;
		
        public EventHandler(ARecorder ar, Looper looper) {
            super(looper);
            mRecorder = ar;
        }
        
        @Override
        public void handleMessage(Message msg) {
            switch(msg.what) {

            }
        }
	}
	
	public ARecorder() {
        Looper looper;
        if ((looper = Looper.myLooper()) != null) {
            mEventHandler = new EventHandler(this, looper);
        } else if ((looper = Looper.getMainLooper()) != null) {
            mEventHandler = new EventHandler(this, looper);
        } else {
            mEventHandler = null;
        }

        nativeSetup(new WeakReference<ARecorder>(this));
	}

    private static void postEventFromNative(Object arecorder_ref, int what, int arg1, int arg2, byte[] bytes) {
    	ARecorder mr = (ARecorder)((WeakReference)arecorder_ref).get();
    	if (mr == null) {
    		return;
    	}

    	if (mr.mEventHandler != null) {
    		Message m = mr.mEventHandler.obtainMessage(what, arg1, arg2);
    		m.obj = bytes;
    		mr.mEventHandler.sendMessage(m);
    	}
    }

	static {
		System.loadLibrary("stlport_shared");
		System.loadLibrary("arecorder-jni");
		nativeInit();
	}
	
	public void setOutputFile(String path){
		nativeSetOutputFile(path);
	}
	
	public void setPreivew(SurfaceHolder holder){
		nativeSetPreview(holder.getSurface());
	}
	
	public void start(){
		nativeStart();
	}
	
	public void stop(){
		nativeStop();
	}
	
	public void release(){
		nativeRelease();
	}
	
    @Override
    protected void finalize() {
    	nativeRelease();
    }

	//native
	private int mNativeContext = 0;
	
	private native void nativeSetOutputFile(String path);
	private native void nativeSetPreview(Surface surfaace);
	private native void nativeStart();
	private native void nativeStop();
	private native void nativeRelease();

    private native void setParameter(String nameValuePair);
    private native final void nativeSetup(Object arecorder_this);

	private static native void nativeInit();
}
