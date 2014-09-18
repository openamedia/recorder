#include "MuxEngine.h"

#define LOG_TAG "MuxEngine"
#include "android/Log.h"

namespace openamedia {

	MuxEngine::MuxEngine()
		:mInited(false),
		 mStarted(false),
		 mDone(false),
		 mThreadExited(false),
		 mAudioSrc(NULL),
		 mVideoSrc(NULL),
		 mFFMPEG(NULL),
		 mNextReadAudio(true){
	}
	
	MuxEngine::~MuxEngine(){
		deInit();
	}

	void MuxEngine::setOutputFile(const char* path){
		strncpy(mOutputFile, path, sizeof(mOutputFile));
	}

	void MuxEngine::setAudioSource(Prefetcher::SubSource* src) {
		mAudioSrc = src;
	}

	void MuxEngine::setVideoSource(Prefetcher::SubSource* src) {
		mVideoSrc = src;
	}

	bool MuxEngine::init(){
		if(mInited)
			return true;

		do{
			mFFMPEG = new FFMPEGer;
			mFFMPEG->setOutputFile(mOutputFile);
			mFFMPEG->setVideoSize(320, 240);
			mFFMPEG->setVideoColorFormat(OMX_COLOR_FormatYUV420SemiPlanar);
			bool res = mFFMPEG->init(NULL);
			if(!res)
				break;

			mInited = true;
		}while(0);

		if(!mInited)
			reset();

		return mInited;
	}

	bool MuxEngine::deInit(){
		if(!mInited)
			return true;

		reset();

		mInited = false;

		return true;
	}
	
	void MuxEngine::reset(){		
		if(mFFMPEG != NULL){
			delete mFFMPEG;
			mFFMPEG = NULL;
		}
	}

	bool MuxEngine::start(){
		if(mStarted)
			return true;
		
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_create(&mThreadID, &attr, ThreadWrapper, this);
		pthread_attr_destroy(&attr);

		mStarted = true;

		return true;
	}
	
	bool MuxEngine::stop(){
		Mutex::Autolock ao(mLock);
		if(!mStarted)
			return true;

		mDone = true;
		while(!mThreadExited){
			mCond.waitRelative(mLock, 40000000);
		}

		mFFMPEG->finish();

		mStarted = false;

		return true;
	}

	bool MuxEngine::getAudioEncodeBufferSize(int* bufSize){
		if(!mInited)
			return false;

		*bufSize = mFFMPEG->getAudioEncodeBufferSize();
		return true;
	}

	void* MuxEngine::ThreadWrapper(void* context){
		MuxEngine* me = (MuxEngine*)context;
		me->threadEntry();

		return NULL;
	}
	
	void MuxEngine::threadEntry(){
		for(;;){
			Mutex::Autolock ao(mLock);
			
			if(mDone){
				break;
			}

			MediaBuffer* buffer = NULL;
			MediaBuffer* out;//TODO: mean nothing.
			bool res;
			
			if(mNextReadAudio && mAudioSrc != NULL){
				res = mAudioSrc->read(&buffer);
				if(!res)
					break;
			
				mFFMPEG->encodeAudio(buffer, out);

				if(mVideoSrc != NULL)
					mNextReadAudio = false;
			}else if(!mNextReadAudio && mVideoSrc != NULL){
				res = mVideoSrc->read(&buffer);
				if(!res)
					break;
			
				mFFMPEG->encodeVideo(buffer, out);

				if(mAudioSrc != NULL)
					mNextReadAudio = true;
			}

			buffer->release();
		}

		{
			ALOGW("MuxEngine thread exited!!");
			Mutex::Autolock ao(mLock);
			mThreadExited = true;
			mCond.signal();
		}
	}

}
