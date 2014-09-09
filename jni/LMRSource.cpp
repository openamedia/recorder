#include "LMRSource.h"

#define LOG_TAG "LMRSource"
#include "android/Log.h"

namespace openamedia {

	//////////////////////////////////// LMRAudioSource
	LMRAudioSource::LMRAudioSource()
		:mInited(false),
		 mStarted(false),
		 mDone(false),
		 mSLRecorder(NULL),
		 mMetaData(NULL),
		 mGroup(NULL),
		 mSLBuffer(NULL){
	}
	
	LMRAudioSource::~LMRAudioSource(){
		deInit();
	}

	bool LMRAudioSource::init(int sampleRate, int channels, int bufSize){
		if(mInited)
			return true;
		
		do{			
			if(sampleRate != 8000 && sampleRate != 16000 && sampleRate != 44100){
				ALOGE("unsupported samplerate:%d", sampleRate);
				break;
			}
			
			if(channels != 1 && channels != 2){
				ALOGE("unsupported channels:%d", channels);
				break;
			}
			
			mMetaData = new MetaData;
			mMetaData->setInt32(kKeySampleRate, sampleRate);
			mMetaData->setInt32(kKeyChannelCount, channels);
			
			mGroup = new MediaBufferGroup;
			for(int i = 0; i < 5; ++i){
				MediaBuffer* buffer = new MediaBuffer(bufSize * 10);
				mGroup->add_buffer(buffer);
			}
			
			mSLRecorder = new OpenSLRecorder(&LMRAudioSource::AcquireBuffer, this);//TODO:a real AudioInfo
			mInited = mSLRecorder->init(sampleRate, channels);
		}while(0);

		if(!mInited)
			reset();

		return mInited;
	}
	
	bool LMRAudioSource::deInit(){
		if(!mInited)
			return true;

		reset();

		mInited = false;
		
		return true;
	}
	
	void LMRAudioSource::reset(){
		if(mMetaData != NULL){
			delete mMetaData;
			mMetaData = NULL;
		}

		if(mSLRecorder != NULL){
			delete mSLRecorder;
			mSLRecorder = NULL;
		}

		List<MediaBuffer*>::iterator it = mBusyList.begin();
		while(it != mBusyList.end()){
			MediaBuffer* buffer = *it;
			buffer->release();
			it = mBusyList.erase(it);
		}

		if(mGroup != NULL){
			delete mGroup;
			mGroup = NULL;
		}
	}

	bool LMRAudioSource::start(){
		if(mStarted)
			return true;

		void* data = NULL;
		int dataSize = 0;
		acquireBuffer(&data, &dataSize);
		
		bool res = mSLRecorder->start(data, dataSize);
		if(!res){
			return false;
		}
		
		mStarted = true;

		return true;
	}
	
	bool LMRAudioSource::stop(){
		{
			Mutex::Autolock ao(mLock);
			
			if(!mStarted)
				return true;
			
			mDone = true;
			mStarted = false;
		}

		mGroup->stop_acquire();
		
		bool res = mSLRecorder->stop();
		if(!res){
			return false;
		}

		if(mSLBuffer != NULL){
			mSLBuffer->release();
			mSLBuffer = NULL;
		}

		return true;
	}

	status_t LMRAudioSource::read(MediaBuffer** buffer){
		Mutex::Autolock ao(mLock);
		while(!mDone && mBusyList.empty()){
			mCond.waitRelative(mLock, 20000000);
		}
		
		if(mDone)
			return ERROR_END_OF_STREAM;
		
		*buffer = *mBusyList.begin();
		mBusyList.erase(mBusyList.begin());
		
		return OK;
	}

	bool LMRAudioSource::AcquireBuffer(void** data, int* dataSize, void* context){
		LMRAudioSource* me = (LMRAudioSource*)context;

		return me->acquireBuffer(data, dataSize);
	}
	
	bool LMRAudioSource::acquireBuffer(void** data, int* dataSize){		
		{
			Mutex::Autolock ao(mLock);
			
			if(mSLBuffer != NULL){
				mSLBuffer->meta_data()->setInt32(kKeyMediaType, MEDIA_TYPE_AUDIO);
				mBusyList.push_back(mSLBuffer);
				mSLBuffer = NULL;
				mCond.signal();
			}
		}
		
		MediaBuffer* buffer = NULL;
		mGroup->acquire_buffer(&buffer);
		
		if(buffer == NULL)
			return false;

		*data = buffer->data();
		*dataSize = buffer->size();
		buffer->set_range(0, *dataSize);
		
		mSLBuffer = buffer;
		
		return true;
	}
	////////////////////////////////////
	
	LMRSource::LMRSource()
		:mInited(false),
		 mStarted(false),
		 mMetaData(NULL),
		 mAudioSource(NULL){
		mMetaData = new MetaData;
	}
	
	LMRSource::~LMRSource(){
		deInit();
		
		if(mMetaData != NULL){
			delete mMetaData;
			mMetaData = NULL;
		}
	}

	bool LMRSource::init(MetaData* meta) {
		if(mInited)
			return true;

		do{
			int hasAudio = 0;
			int hasVideo = 0;
			meta->findInt32(kKeyHasAudio, &hasAudio);
			meta->findInt32(kKeyHasAudio, &hasVideo);

			if(hasAudio != 0){
				int sampleRate = 0;
				int channels = 0;
				if(!meta->findInt32(kKeySampleRate, &sampleRate) || !meta->findInt32(kKeyChannelCount, &channels)){
					ALOGE("no samplerate or channels set for recording.");
					break;
				}

				int bufSize;
				if(!meta->findInt32(kKeyAudioEncodeBufSize, &bufSize)){
					ALOGE("no audio encode bufsize set for recording.");
					break;
				}
			
				mAudioSource = new LMRAudioSource;
				bool res = mAudioSource->init(sampleRate, channels, bufSize);
				if(!res){
					ALOGE("fail to init audio source!!");
					break;
				}
			}

			mInited = true;
		}while(0);

		if(!mInited)
			reset();

		return mInited;
	}

	bool LMRSource::deInit(){
		if(!mInited)
			return true;

		reset();

		mInited = false;
	}
	
	void LMRSource::reset(){
		if(mAudioSource != NULL){
			delete mAudioSource;
			mAudioSource = NULL;
		}
	}
	
	bool LMRSource::start(){
		if(mStarted)
			return true;

		bool res;
		if(mAudioSource != NULL){
			res = mAudioSource->start();
			if(!res){
				return false;
			}
			
			mMetaData->setInt32(kKeyHasAudio, 1);
		}
		
		mStarted = true;
		
		return true;
	}
	
	bool LMRSource::stop(){
		if(!mStarted)
			return true;

		mAudioSource->stop();
		
		mStarted = false;
		
		return true;
	}
	
	status_t LMRSource::read(MediaBuffer** buffer){
		return mAudioSource->read(buffer);
	}
	
	bool LMRSource::seek(int64_t timeMS){
		ALOGE("LMRSource::seek not supported!!");
		return false;
	}

	MetaData* LMRSource::getMetaData(){
		return mMetaData;
	}
	
}//namespace
