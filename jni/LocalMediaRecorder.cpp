#include "LocalMediaRecorder.h"

#define LOG_TAG "LocalMediaRecorder"
#include "android/Log.h"

namespace openamedia {

	LocalMediaRecorder::LocalMediaRecorder()
		:mListener(NULL),
		 mMsgQueue(NULL),
		 mLMRSource(NULL),
		 mPrefetcher(NULL),
		 mMuxEngine(NULL){
		mMsgQueue = new MessageQueue(&LocalMediaRecorder::HandleMessage, &LocalMediaRecorder::HandleThreadExit, this);
		mMetaData = new MetaData;
	}
	
	LocalMediaRecorder::~LocalMediaRecorder(){
		stop();

		if(mMsgQueue != NULL){
			delete mMsgQueue;
			mMsgQueue = NULL;
		}
		
		if(mMetaData != NULL){
			delete mMetaData;
			mMetaData = NULL;
		}

		if(mMuxEngine != NULL){
			delete mMuxEngine;
			mMuxEngine = NULL;
		}

		if(mPrefetcher != NULL){
			delete mPrefetcher;
			mPrefetcher = NULL;
		}

		ALOGE("LocalMediaRecorder::~LocalMediaRecorder 0");
		
		if(mLMRSource != NULL){
			delete mLMRSource;
			mLMRSource = NULL;
		}

		ALOGE("LocalMediaRecorder::~LocalMediaRecorder 1");

		/*
		if(mListener != NULL){
			delete mListener;
			mListener = NULL;
		}
		*/
	}

	void LocalMediaRecorder::setOutputFile(const char* path){
		Message* msg = mMsgQueue->obtainMessage();
		msg->what = MSG_SET_OUTPUT_FILE;
		char* tmp = (char*)malloc(strlen(path) + 1);
		strncpy(tmp, path, strlen(path) + 1);
		msg->obj = tmp;
		mMsgQueue->sendMessage(msg);		
	}
	
	void LocalMediaRecorder::setListener(MediaRecorderListener* listener){
		Message* msg = mMsgQueue->obtainMessage();
		msg->what = MSG_SET_LISTENER;
		msg->obj = listener;
		mMsgQueue->sendMessage(msg);
	}

	void LocalMediaRecorder::setParameter(const char* param){
		Message* msg = mMsgQueue->obtainMessage();
		msg->what = MSG_SET_PARAMETER;
		char* tmp = (char*)malloc(strlen(param) + 1);
		strncpy(tmp, param, strlen(param) + 1);
		msg->obj = tmp;
		mMsgQueue->sendMessage(msg);
	}
	
	void LocalMediaRecorder::setPreview(ANativeWindow* window){
		Message* msg = mMsgQueue->obtainMessage();
		msg->what = MSG_SET_PREVIEW;
		msg->obj = window;
		mMsgQueue->sendMessage(msg);
	}

	void LocalMediaRecorder::start(){
		Message* msg = mMsgQueue->obtainMessage();
		msg->what = MSG_START;
		mMsgQueue->sendMessage(msg);
	}
	
	void LocalMediaRecorder::stop(){
		mMuxEngine->stop();
		mPrefetcher->stop();
	}

	void LocalMediaRecorder::HandleMessage(Message* msg, void* context){
		LocalMediaRecorder* me = (LocalMediaRecorder*)context;
		
		me->handleMessage(msg);
	}
	
	void LocalMediaRecorder::handleMessage(Message* msg){
		switch(msg->what){
		case MSG_SET_OUTPUT_FILE:
			{
				char* path = (char*)msg->obj;
				strncpy(mOutputFile, path, sizeof(mOutputFile));
				free(path);
				break;
			}
		case MSG_SET_LISTENER:
			mListener = (MediaRecorderListener*)msg->obj;
			mListener->registerCurThread();
			break;
		case MSG_SET_PARAMETER:
			{
				char* param = (char*)msg->obj;
				onSetParameter(param);
				free(param);
				break;
			}
		case MSG_SET_PREVIEW:
			break;
		case MSG_START:
			onStart();
			break;
		default:
			break;
		}
	}

	void LocalMediaRecorder::onSetParameter(const char* param){
		
	}
	
	void LocalMediaRecorder::onStart(){
		mMuxEngine = new MuxEngine();
		mMuxEngine->setOutputFile(mOutputFile);
		bool res = mMuxEngine->init();
		if(!res)
			return;
		
		mLMRSource = new LMRSource;

		mMetaData->setInt32(kKeyHasAudio, 1);
		mMetaData->setInt32(kKeyChannelCount, 2);
		mMetaData->setInt32(kKeySampleRate, 44100);

		int bufSize;
		res = mMuxEngine->getAudioEncodeBufferSize(&bufSize);
		if(res){
			mMetaData->setInt32(kKeyAudioEncodeBufSize, bufSize);
		}
		
		mLMRSource->init(mMetaData);
		
		mPrefetcher = new Prefetcher(mLMRSource);
		mPrefetcher->start();

		mMuxEngine->setAudioSource(mPrefetcher->getSource(MEDIA_TYPE_AUDIO));
		mMuxEngine->start();
	}
			
	void LocalMediaRecorder::HandleThreadExit(void* context){
		LocalMediaRecorder* me = (LocalMediaRecorder*)context;

		me->handleThreadExit();
	}

	void LocalMediaRecorder::handleThreadExit(){
		mListener->unRegisterCurThread();
	}

}//namespace
