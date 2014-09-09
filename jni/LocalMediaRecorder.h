#ifndef OPENAMEDIA_LOCAL_MEDIA_RECORDER_H
#define OPENAMEDIA_LOCAL_MEDIA_RECORDER_H

#include "android/MetaData.h"

#include "MediaRecorder.h"
#include "MessageQueue.h"
#include "LMRSource.h"
#include "Prefetcher.h"
#include "MuxEngine.h"

namespace openamedia {

	class LocalMediaRecorder : public MediaRecorder {
	public:
		LocalMediaRecorder();

		enum MsgType {
			MSG_SET_OUTPUT_FILE,
			MSG_SET_LISTENER,
			MSG_SET_PARAMETER,
			MSG_SET_PREVIEW,
			MSG_START,
			MSG_STOP,
		};

		virtual void setOutputFile(const char* path);
		virtual void setListener(MediaRecorderListener* listener);
		virtual void setParameter(const char* param);
		virtual void setPreview(ANativeWindow* window);
		virtual void start();
		virtual void stop();

	protected:
		virtual ~LocalMediaRecorder();

	private:
		char mOutputFile[MAX_STRING_PATH_LEN];
		
		MediaRecorderListener* mListener;
		MessageQueue* mMsgQueue;

		LMRSource* mLMRSource;
		Prefetcher* mPrefetcher;
		MuxEngine* mMuxEngine;

		MetaData* mMetaData;

		static void HandleMessage(Message* msg, void* context);
		void handleMessage(Message* msg);
		
		static void HandleThreadExit(void* context);
		void handleThreadExit();

		void onSetParameter(const char*);
		void onStart();

		LocalMediaRecorder(const LocalMediaRecorder&);
		LocalMediaRecorder& operator=(const LocalMediaRecorder&);
	};
}//namespace

#endif//OPENAMEDIA_LOCAL_MEDIA_RECORDER_H
