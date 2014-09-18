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
		 mSLBuffer(NULL),
		 mSampleRate(0),
		 mChannels(0),
		 mBufSize(0){
	}
	
	LMRAudioSource::~LMRAudioSource(){
		deInit();
	}

	void LMRAudioSource::setSampleRate(int sampleRate){
		mSampleRate = sampleRate;
	}
	
	void LMRAudioSource::setChannels(int channels){
		mChannels = channels;
	}
	
	void LMRAudioSource::setBufSize(int bufSize){
		mBufSize = bufSize;
	}

	bool LMRAudioSource::init(){
		if(mInited)
			return true;
		
		do{			
			if(mSampleRate != 8000 && mSampleRate != 16000 && mSampleRate != 44100){
				ALOGE("unsupported samplerate:%d", mSampleRate);
				break;
			}
			
			if(mChannels != 1 && mChannels != 2){
				ALOGE("unsupported channels:%d", mChannels);
				break;
			}
			
			mMetaData = new MetaData;
			mMetaData->setInt32(kKeySampleRate, mSampleRate);
			mMetaData->setInt32(kKeyChannelCount, mChannels);
			
			mGroup = new MediaBufferGroup;
			for(int i = 0; i < 5; ++i){
				MediaBuffer* buffer = new MediaBuffer(mBufSize);
				mGroup->add_buffer(buffer);
			}
			
			mSLRecorder = new OpenSLRecorder(&LMRAudioSource::AcquireBuffer, this);//TODO:a real AudioInfo
			mInited = mSLRecorder->init(mSampleRate, mChannels);
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

	
	////////////////////////////////////
	LMRVideoSource::LMRVideoSource()
		:mInited(false),
		 mStarted(false),
		 mDone(false),
		 mMetaData(NULL),
		 mWidth(0),
		 mHeight(0),
		 mGroup(NULL){
		
	}
	
	LMRVideoSource::~LMRVideoSource(){
		deInit();
	}

	void LMRVideoSource::setRect(int width, int height){
		mWidth = width;
		mHeight = height;
	}

	void LMRVideoSource::setColorFormat(OMX_COLOR_FORMATTYPE color){
		mColor = color;
	}
		
	bool LMRVideoSource::init(){
		if(mInited)
			return true;
		
		do{			
			if(mWidth > 1280 || mHeight > 720){
				ALOGE("unsupported widthheight:%d*%d", mWidth, mHeight);
				break;
			}
			
			if(mColor != OMX_COLOR_FormatYUV420Planar && mColor != OMX_COLOR_FormatYUV420SemiPlanar){
				ALOGE("unsupported color:%d", mColor);
				break;
			}
			
			mMetaData = new MetaData;
			mMetaData->setInt32(kKeyWidth, mWidth);
			mMetaData->setInt32(kKeyHeight, mHeight);
			mMetaData->setInt32(kKeyColorFormat, mColor);
			
			mGroup = new MediaBufferGroup;
			for(int i = 0; i < 5; ++i){
				MediaBuffer* buffer = new MediaBuffer(mWidth * mHeight * 3 / 2);
				mGroup->add_buffer(buffer);
			}
			
			mInited = true;
		}while(0);

		if(!mInited)
			reset();

		return mInited;		
	}

	bool LMRVideoSource::deInit(){
		if(!mInited)
			return true;

		reset();
		
		mInited = false;
		
		return true;
	}
	
	void LMRVideoSource::reset(){
		if(mMetaData != NULL){
			delete mMetaData;
			mMetaData = NULL;
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

	bool LMRVideoSource::start(){
		if(mStarted)
			return true;

		mStarted = true;

		return true;
	}
	
	bool LMRVideoSource::stop(){
		{
			Mutex::Autolock ao(mLock);

			if(!mStarted)
				return true;
			
			mDone = true;
			mStarted = false;
		}

		mGroup->stop_acquire();
	}
	
	status_t LMRVideoSource::read(MediaBuffer** buffer){
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
		
	status_t LMRVideoSource::write(void* data, int dataSize){
		{
			Mutex::Autolock ao(mLock);
			if(!mStarted)
				return INVALID_OPERATION;
			
			if(mDone)
				return ERROR_END_OF_STREAM;
		}
		
		MediaBuffer* buffer = NULL;
		status_t res = mGroup->acquire_buffer_with_timeout(&buffer, 30);
		if(res != OK)
			return res;
		
		if(buffer == NULL)
			return ERROR_END_OF_STREAM;

		memcpy(buffer->data(), data, dataSize);
		buffer->set_range(0, dataSize);
		buffer->meta_data()->setInt32(kKeyMediaType, MEDIA_TYPE_VIDEO);
		
		Mutex::Autolock ao(mLock);
		mBusyList.push_back(buffer);
		mCond.signal();
		
		return OK;
	}
	////////////////////////////////////
	
	LMRSource::LMRSource()
		:mInited(false),
		 mStarted(false),
		 mMetaData(NULL),
		 mAudioSource(NULL),
		 mVideoSource(NULL),
		 mNextReadAudio(true){
		mMetaData = new MetaData;
	}
	
	LMRSource::~LMRSource(){
		deInit();
		
		if(mMetaData != NULL){
			delete mMetaData;
			mMetaData = NULL;
		}

		if(mAudioSource != NULL){
			delete mAudioSource;
			mAudioSource = NULL;
		}

		if(mVideoSource != NULL){
			delete mVideoSource;
			mVideoSource = NULL;
		}
	}

	bool LMRSource::init(MetaData* meta) {
		if(mInited)
			return true;

		do{
			bool res;
			int hasAudio = 0;
			int hasVideo = 0;
			meta->findInt32(kKeyHasAudio, &hasAudio);
			meta->findInt32(kKeyHasVideo, &hasVideo);
			
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
				mAudioSource->setSampleRate(sampleRate);
				mAudioSource->setChannels(channels);
				mAudioSource->setBufSize(bufSize);
				res = mAudioSource->init();
				if(!res){
					ALOGE("fail to init audio source!!");
					break;
				}
			}
			
			if(hasVideo != 0){
				int width, height;
				if(!meta->findInt32(kKeyWidth, &width) || !meta->findInt32(kKeyHeight, &height)){
					ALOGE("no width or height set for recording");
					break;
				}
				
				int color_fmt;
				if(!meta->findInt32(kKeyColorFormat, &color_fmt)){
					ALOGE("no color fmt set for recording");
					break;
				}
				
				mVideoSource = new LMRVideoSource;
				mVideoSource->setRect(width, height);
				mVideoSource->setColorFormat((OMX_COLOR_FORMATTYPE)color_fmt);
				res = mVideoSource->init();
				if(!res){
					ALOGE("fail to init video source!!");
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

		if(mVideoSource != NULL){
			res = mVideoSource->start();
			if(!res){
				return false;
			}

			mMetaData->setInt32(kKeyHasVideo, 1);
		}
		
		mStarted = true;
		
		return true;
	}
	
	bool LMRSource::stop(){
		if(!mStarted)
			return true;

		if(mAudioSource != NULL){
			mAudioSource->stop();
		}

		if(mVideoSource != NULL){
			mVideoSource->stop();
		}
		
		mStarted = false;
		
		return true;
	}
	
	status_t LMRSource::read(MediaBuffer** buffer){
		if(mNextReadAudio && mAudioSource != NULL){
			if(mVideoSource != NULL)
				mNextReadAudio = false;
			
			return mAudioSource->read(buffer);
		}else if(!mNextReadAudio && mVideoSource != NULL){
			if(mAudioSource != NULL)
				mNextReadAudio = true;
			
			return mVideoSource->read(buffer);
		}
	}

	status_t LMRSource::writeVideo(void* data, int dataSize){
		return mVideoSource->write(data, dataSize);
	}
	
	bool LMRSource::seek(int64_t timeMS){
		ALOGE("LMRSource::seek not supported!!");
		return false;
	}

	MetaData* LMRSource::getMetaData(){
		return mMetaData;
	}
	
}//namespace
