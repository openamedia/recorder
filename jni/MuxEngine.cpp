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
		 mFFMPEG(NULL){
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
			bool res = mFFMPEG->init(NULL);
			if(!res)
				break;

			/*
			mAudioEncoder = new FFmpegAudioEncoder;
			mAudioEncoder->setSampleRate(44100);
			mAudioEncoder->setChannels(2);
			mAudioEncoder->setEncoder(CODEC_TYPE_AAC);
			res = mAudioEncoder->init();
			if(!res)
				break;
			*/

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

		/*
		if(mAudioEncoder != NULL){
			delete mAudioEncoder;
			mAudioEncoder = NULL;
		}
		*/
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
				mFFMPEG->finish();
				break;
			}
			
			MediaBuffer* buffer;
			mAudioSrc->read(&buffer);

			/*
			void* outv;
			int outSize;
			bool res  = mAudioEncoder->encode(buffer->data(), buffer->range_length(), &outv, &outSize);
			if(!res){
				ALOGE("mAudioEncoder->encode error!!");
			}
			*/
			
			MediaBuffer* out;//TODO: mean nothing.
			mFFMPEG->encodeAudio(buffer, out);

			buffer->release();
		}

		{
			Mutex::Autolock ao(mLock);
			mThreadExited = true;
			mCond.signal();
		}
	}

}
