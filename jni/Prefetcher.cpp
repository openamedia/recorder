/*
 * Copyright (c) 2014 Jim Qian
 *
 * This file is part of OpenaMedia-C(client).
 *
 * OpenaMedia-C is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with OpenaMedia-C; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "Prefetcher.h"

#define LOG_TAG "Prefetcher"
#include "android/Log.h"

namespace openamedia {

	///////////////////////////////////////
	Prefetcher::SubSource::SubSource(int capacity)
		:mDone(false){
		for(int i = 0; i < capacity; ++i){
			mGroup.add_buffer(new MediaBuffer(MAX_SUB_SOURCE_BUFFER_SIZE));
		}
	}
	
	Prefetcher::SubSource::~SubSource(){
		stop();		
	}

	void Prefetcher::SubSource::stop(){
		Mutex::Autolock ao(mLock);

		mDone = true;

		clear_l();
	}
	
	bool Prefetcher::SubSource::read(MediaBuffer** buffer) {
		Mutex::Autolock ao(mLock);
		
		while(!mDone && mBusyList.empty()){
			mCond.waitRelative(mLock, 50 * 1000000);
		}

		if(mDone)
			return false;

		MediaBuffer* buf = *mBusyList.begin();
		*buffer = buf;
		
		mBusyList.erase(mBusyList.begin());

		return true;
	}
		
	bool Prefetcher::SubSource::write(MediaBuffer* buffer){
		MediaBuffer* dstbuf = NULL;
		mGroup.acquire_buffer(&dstbuf);
		
		Mutex::Autolock ao(mLock);

		if(dstbuf == NULL)
			return false;

		dstbuf->copyDataFrom(buffer);

		mBusyList.push_back(dstbuf);

		mCond.signal();
		
		return true;
	}

	void Prefetcher::SubSource::clear(){
		Mutex::Autolock ao(mLock);
		
		clear_l();
	}
	
	void Prefetcher::SubSource::clear_l(){
		List<MediaBuffer*>::iterator it = mBusyList.begin();
		while(it != mBusyList.end()){
			MediaBuffer* buf = *it;
			buf->release();
			
			it = mBusyList.erase(it);
		}
	}
	///////////////////////////////////

	Prefetcher::Prefetcher(Source* src)
		:mStarted(false),
		 mSource(src),
		 mSeeking(false),
		 mSeekTime(0),
		 mDone(false),
		 mThreadExited(false){
		mVideoSource = new SubSource(DEFAULT_VIDEO_SUB_SOURCE_CAPACITY);
		mAudioSource = new SubSource(DEFAULT_AUDIO_SUB_SOURCE_CAPACITY);		
	}
	
	Prefetcher::~Prefetcher(){
		stop();

		delete mVideoSource;
		mVideoSource = NULL;
		delete mAudioSource;
		mAudioSource = NULL;
	}

	bool Prefetcher::start(){
		if(mStarted)
			return true;

		bool res = mSource->start();
		if(!res){
			ALOGE("fail to start mSource!!");
			return false;
		}

		pthread_attr_t attr;
		pthread_attr_init(&attr);
		//pthread_attr_setdetachstate(&attr, PTREAD_CREATE_JOINABLE);
		pthread_create(&mThread, &attr, ThreadWrapper, this);
		pthread_attr_destroy(&attr);

		mStarted = true;

		return true;
	}
	
	bool Prefetcher::stop(){		
		Mutex::Autolock ao(mLock);
				
		if(!mStarted)
			return true;
				
		mSource->stop();
		
		mDone = true;
		while(!mThreadExited){
			mCond.waitRelative(mLock, 40 * 1000000);
		}

		if(mVideoSource != NULL){
			mVideoSource->stop();
		}

		if(mAudioSource != NULL){
			mAudioSource->stop();
		}
		
		mStarted = false;
		
		return true;
	}

	bool Prefetcher::seek(int64_t timeMS){
		Mutex::Autolock ao(mLock);

		if(!mStarted)
			return false;

		mSeeking = true;
		mSeekTime = timeMS;

		mSource->seek(timeMS);

		mVideoSource->clear();
		mAudioSource->clear();

		return true;
	}

	Prefetcher::SubSource* Prefetcher::getSource(MediaType type){
		if(type == MEDIA_TYPE_VIDEO)
			return mVideoSource;
		else if(type == MEDIA_TYPE_AUDIO)
			return mAudioSource;
		else
			return NULL;
	}

	void* Prefetcher::ThreadWrapper(void* me){
		Prefetcher* p = (Prefetcher*)me;
		p->threadEntry();

		return NULL;
	}
	
	void Prefetcher::threadEntry(){
		for(;;){
			{
				Mutex::Autolock ao(mLock);
				
				if(mDone){
					break;
				}
			}
			
			bool res;
			MediaBuffer* srcbuf;
			
			status_t ret = mSource->read(&srcbuf);
			if(ret != OK){
				if(ret == ERROR_END_OF_STREAM){
					ALOGE("EOS!!");
					break;
				}
				
				ALOGE("tmp fail to read src and continue!");
				mCond.waitRelative(mLock, 20 * 1000000);
				continue;//wait shall be placed before every 'continue' to previent the too much frequent token of mLock. which will block the other mLock(in stop()).
			}
			
			int mediaType;
			if(!srcbuf->meta_data()->findInt32(kKeyMediaType, &mediaType)){
				ALOGE("fail to find media type info from the buffer %p!!", srcbuf->data());
				srcbuf->release();
				break;
			}

			SubSource* sub = NULL;
			if(mediaType == MEDIA_TYPE_VIDEO){
				sub = mVideoSource;
			}else if(mediaType == MEDIA_TYPE_AUDIO){
				sub = mAudioSource;
			}else{
				ALOGE("unknown buffer type:%d read from source!!", mediaType);
				srcbuf->release();
				break;
			}

			sub->write(srcbuf);//TODO: blocking call.

			srcbuf->release();
		}

		{
			ALOGW("Prefetcher thread exited!!");
			Mutex::Autolock ao(mLock);
			mThreadExited = true;
			mCond.signal();
		}
	}

}
