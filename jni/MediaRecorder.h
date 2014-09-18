#ifndef OPENAMEDIA_MEDIA_RECORDER_H
#define OPENAMEDIA_MEDIA_RECORDER_H

#include <stdint.h>
#include <android/native_window_jni.h>
#include "openmax/OMX_IVCommon.h"

namespace openamedia {

	class Engine {
	public:
		virtual ~Engine();
		
		virtual bool start() = 0;
		virtual bool stop() = 0;
		
	protected:
		Engine();
		
	private:		
		Engine(const Engine&);
		Engine& operator=(const Engine&);
	};

	class MediaRecorderListener {
	public:
		virtual ~MediaRecorderListener(){}
		
		static const int NATIVE_MSG_SET_OUTPUT_FILE_DONE = 0;
		static const int NATIVE_MSG_SET_LISTENER_DONE = 1;
		static const int NATIVE_MSG_SET_PARAMETER_DONE = 2;
		static const int NATIVE_MSG_SET_PREVIEW_DONE = 3;
		static const int NATIVE_MSG_START_DONE = 4;
		static const int NATIVE_MSG_ERROR = 5;
		static const int NATIVE_MSG_NOTIFY_LOG_INFO = 6;
		
		virtual void notify(int msg, int ext1 = 0, int ext2 = 0, const void* data = NULL, int dataSize = 0) = 0;
		virtual void notifyLog(const char* log) = 0;
	
		virtual void registerCurThread() = 0;
		virtual void unRegisterCurThread() = 0;

	protected:
		MediaRecorderListener(){}
		
	private:
		MediaRecorderListener(const MediaRecorderListener&);
		MediaRecorderListener& operator=(const MediaRecorderListener&);
	};

	class MediaRecorder {
	public:
		virtual ~MediaRecorder(){}

		virtual void setOutputFile(const char* path) = 0;
		virtual void setListener(MediaRecorderListener* listener) = 0;
		virtual void setParameter(const char* param) = 0;
		virtual void setPreview(ANativeWindow* window) = 0;
		virtual void setChannels(int channels) = 0;
		virtual void setSampleRate(int sampleRate) = 0;
		virtual void setVideoSize(int width, int height) = 0;
		virtual void setColorFormat(OMX_COLOR_FORMATTYPE fmt) = 0;
		virtual void start() = 0;
		virtual void stop() = 0;
		virtual void writeVideo(void* data, int dataSize) = 0;

	protected:
		MediaRecorder(){}

	private:
		MediaRecorder(const MediaRecorder&);
		MediaRecorder& operator=(const MediaRecorder&);
	};

}//namespace

#endif//OPENAMEDIA_MEDIA_RECORDER_H
