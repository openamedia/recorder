#include "LocalMediaRecorder.h"

#define LOG_TAG "LocalMediaRecorder"
#include "android/Log.h"

#include "openmax/OMX_IVCommon.h"

namespace openamedia {

	LocalMediaRecorder::LocalMediaRecorder()
		:mListener(NULL),
		 mMsgQueue(NULL),
		 mLMRSource(NULL),
		 mPrefetcher(NULL),
		 mMuxEngine(NULL),
		 mChannels(0),
		 mSampleRate(0),
		 mWidth(0),
		 mHeight(0),
		 mColorFormat(OMX_COLOR_FormatUnused){
		mMsgQueue = new MessageQueue(&LocalMediaRecorder::HandleMessage, &LocalMediaRecorder::HandleThreadExit, this);
		mMetaData = new MetaData;
	}
	
	LocalMediaRecorder::~LocalMediaRecorder(){
		stop();

		if(mMsgQueue != NULL){
			delete mMsgQueue;
			mMsgQueue = NULL;
		}

		if(mListener != NULL){
			delete mListener;
			mListener = NULL;
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

		if(mLMRSource != NULL){
			delete mLMRSource;
			mLMRSource = NULL;
		}
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

	void LocalMediaRecorder::setChannels(int channels){
		Message* msg = mMsgQueue->obtainMessage();
		msg->what = MSG_SET_CHANNELS;
		msg->arg1 = channels;
		mMsgQueue->sendMessage(msg);				
	}
	
	void LocalMediaRecorder::setSampleRate(int sampleRate){
		Message* msg = mMsgQueue->obtainMessage();
		msg->what = MSG_SET_SAMPLE_RATE;
		msg->arg1 = sampleRate;
		mMsgQueue->sendMessage(msg);		
	}
	
	void LocalMediaRecorder::setVideoSize(int width, int height){
		Message* msg = mMsgQueue->obtainMessage();
		msg->what = MSG_SET_VIDEO_SIZE;
		msg->arg1 = width;
		msg->arg2 = height;
		mMsgQueue->sendMessage(msg);
	}
	
	void LocalMediaRecorder::setColorFormat(OMX_COLOR_FORMATTYPE fmt){
		Message* msg = mMsgQueue->obtainMessage();
		msg->what = MSG_SET_COLOR_FORMAT;
		msg->arg1 = fmt;
		mMsgQueue->sendMessage(msg);		
	}

	void LocalMediaRecorder::start(){
		Message* msg = mMsgQueue->obtainMessage();
		msg->what = MSG_START;
		mMsgQueue->sendMessage(msg);
	}
	
	void LocalMediaRecorder::stop(){
		mPrefetcher->stop();
		mMuxEngine->stop();
	}

	void LocalMediaRecorder::writeVideo(void* data, int dataSize){
		if(mLMRSource != NULL){
			mLMRSource->writeVideo(data, dataSize);
		}
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
				mListener->notify(MediaRecorderListener::NATIVE_MSG_SET_OUTPUT_FILE_DONE);
				break;
			}
		case MSG_SET_LISTENER:
			mListener = (MediaRecorderListener*)msg->obj;
			mListener->registerCurThread();
			mListener->notify(MediaRecorderListener::NATIVE_MSG_SET_LISTENER_DONE);
			break;
		case MSG_SET_PARAMETER:
			{
				char* param = (char*)msg->obj;
				onSetParameter(param);
				free(param);
				mListener->notify(MediaRecorderListener::NATIVE_MSG_SET_PARAMETER_DONE);
				break;
			}
		case MSG_SET_PREVIEW:
			mListener->notify(MediaRecorderListener::NATIVE_MSG_SET_PREVIEW_DONE);
			break;
		case MSG_SET_CHANNELS:
			mChannels = msg->arg1;
			break;
		case MSG_SET_SAMPLE_RATE:
			mSampleRate = msg->arg1;
			break;
		case MSG_SET_VIDEO_SIZE:
			mWidth = msg->arg1;
			mHeight = msg->arg2;
			break;
		case MSG_SET_COLOR_FORMAT:
			mColorFormat = (OMX_COLOR_FORMATTYPE)msg->arg1;
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
		if(!res){
			mListener->notify(MediaRecorderListener::NATIVE_MSG_ERROR);
			return;
		}
		
		mLMRSource = new LMRSource;
		
		mMetaData->setInt32(kKeyHasAudio, 1);
		mMetaData->setInt32(kKeyChannelCount, mChannels);
		mMetaData->setInt32(kKeySampleRate, mSampleRate);
		int bufSize;
		res = mMuxEngine->getAudioEncodeBufferSize(&bufSize);
		if(res){
			mMetaData->setInt32(kKeyAudioEncodeBufSize, bufSize);
		}
		
		mMetaData->setInt32(kKeyHasVideo, 1);
		mMetaData->setInt32(kKeyWidth, mWidth);
		mMetaData->setInt32(kKeyHeight, mHeight);
		mMetaData->setInt32(kKeyColorFormat, mColorFormat);

		res = mLMRSource->init(mMetaData);
		if(!res){
			mListener->notify(MediaRecorderListener::NATIVE_MSG_ERROR);
			return;
		}
		
		mPrefetcher = new Prefetcher(mLMRSource);
		mPrefetcher->start();

		mMuxEngine->setAudioSource(mPrefetcher->getSource(MEDIA_TYPE_AUDIO));
		mMuxEngine->setVideoSource(mPrefetcher->getSource(MEDIA_TYPE_VIDEO));
		mMuxEngine->start();

		mListener->notify(MediaRecorderListener::NATIVE_MSG_START_DONE);
	}
			
	void LocalMediaRecorder::HandleThreadExit(void* context){
		LocalMediaRecorder* me = (LocalMediaRecorder*)context;

		me->handleThreadExit();
	}

	void LocalMediaRecorder::handleThreadExit(){
		mListener->unRegisterCurThread();
	}

}//namespace
