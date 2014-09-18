package com.openamedia.recorder;

import java.lang.ref.WeakReference;

import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.view.Surface;
import android.view.SurfaceHolder;

public class ARecorder {
	
	private ARecorderListener mListener;
	public interface ARecorderListener {
		public void onStart();
	}
	
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
            case NATIVE_MSG_START_DONE:
            	if(mListener != null)
            		mListener.onStart();
            	break;
            default:
            	break;
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
	
	public void setListener(ARecorderListener listener){
		mListener = listener;
	}
	
	public void setOutputFile(String path){
		nativeSetOutputFile(path);
	}
	
	public void setPreivew(SurfaceHolder holder){
		nativeSetPreview(holder.getSurface());
	}
	
	public void setChannels(int channels){
		nativeSetChannels(channels);
	}
	
	public void setSampleRate(int sampleRate){
		nativeSetSampleRate(sampleRate);
	}
	
	public void setVideoSize(int width, int height){
		nativeSetVideoSize(width, height);
	}
	
	public void setColorFormat(String fmt){
		nativeSetColorFormat(fmt);
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
	
	public void writeVideo(byte[] yuv, int len){
		nativeWriteVideo(yuv, len);
	}
	
    @Override
    protected void finalize() {
    	nativeRelease();
    }

	//native
	private int mNativeContext = 0;
	
	private static final int NATIVE_MSG_SET_OUTPUT_FILE_DONE = 0;
	private static final int NATIVE_MSG_SET_LISTENER_DONE = 1;
	private static final int NATIVE_MSG_SET_PARAMETER_DONE = 2;
	private static final int NATIVE_MSG_SET_PREVIEW_DONE = 3;
	private static final int NATIVE_MSG_START_DONE = 4;
	private static final int NATIVE_MSG_ERROR = 5;
	private static final int NATIVE_MSG_NOTIFY_LOG_INFO = 6;
	
	private native void nativeSetOutputFile(String path);
	private native void nativeSetPreview(Surface surface);
	private native void nativeSetChannels(int channels);
	private native void nativeSetSampleRate(int sampleRate);
	private native void nativeSetVideoSize(int width, int height);
	private native void nativeSetColorFormat(String fmt);
	private native void nativeStart();
	private native void nativeStop();
	private native void nativeRelease();
	private native void nativeWriteVideo(byte[] yuv, int len);

    private native void setParameter(String nameValuePair);
    private native final void nativeSetup(Object arecorder_this);

	private static native void nativeInit();
}
