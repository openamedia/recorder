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

#include "MessageQueue.h"

#define LOG_TAG "MessageQueue"
#include "android/Log.h"

namespace openamedia {

	MessageQueue::MessageQueue(void (*handleMessage)(Message* msg, void* context), void (*handleThreadExit)(void* context), void* context)
		:mDone(false),
		 mThreadExited(false),
		 mStarted(false),
		 mHandleMessage(handleMessage),
		 mHandleThreadExitFnc(handleThreadExit),
		 mContext(context){
		start();
	}
	
	MessageQueue::~MessageQueue(){		
		stop();

		List<Message*>::iterator it = mFreeList.begin();
		while(it != mFreeList.end()){
			Message* tmp = *it;
			free(tmp);
			it = mFreeList.erase(it);
		}

		it = mObtainList.begin();
		while(it != mObtainList.end()){
			Message* tmp = *it;
			free(tmp);
			it = mObtainList.erase(it);
		}

		it = mRunList.begin();
		while(it != mRunList.end()){
			Message* tmp = *it;
			free(tmp);
			it = mRunList.erase(it);
		}
	}
		
	Message* MessageQueue::obtainMessage(){
		Mutex::Autolock ao(mLock);

		int tryTimes = 3;
		while(!mDone && mFreeList.empty() && tryTimes-- > 0){
			mCond.waitRelative(mLock, 100000000);
		}

		if(mDone)
			return NULL;

		Message* msg = NULL;
		
		if(!mFreeList.empty()){
			msg = *mFreeList.begin();
			mFreeList.erase(mFreeList.begin());
			mObtainList.push_back(msg);
		}else{
			msg = (Message*)calloc(sizeof(Message), 1);
			mObtainList.push_back(msg);
		}

		memset(msg, 0, sizeof(Message));
		
		return msg;
	}
	
	void MessageQueue::sendMessage(Message* msg){
		Mutex::Autolock ao(mLock);

		if(mDone)
			return;

		for(List<Message*>::iterator it = mObtainList.begin(); it != mObtainList.end(); ++it){
			Message* tmp = *it;
			if(tmp == msg){
				mObtainList.erase(it);
				mRunList.push_back(tmp);
				mCond.signal();
				break;
			}
		}
	}

	void* MessageQueue::ThreadWrapper(void* me){
		MessageQueue* mq = (MessageQueue*)me;
		mq->threadEntry();
		
		return NULL;
	}
	
	void MessageQueue::threadEntry(){		
		for(;;){
			Message* msg = NULL;

			{
				Mutex::Autolock ao(mLock);

				while(!mDone && mRunList.empty()){
					mCond.waitRelative(mLock, 200000000);
				}
				
				if(mDone){
					break;
				}

				msg = *mRunList.begin();
			}

			if(msg == NULL)
				continue;
			
			mHandleMessage(msg, mContext);
			
			{
				Mutex::Autolock ao(mLock);
				mRunList.erase(mRunList.begin());
				mFreeList.push_back(msg);
				mCond.signal();
			}
		}

		mHandleThreadExitFnc(mContext);

		{
			Mutex::Autolock ao(mLock);
			mThreadExited = true;
			mCond.signal();
		}
		
		return;
	}
	
	void MessageQueue::start(){
		Mutex::Autolock ao(mLock);
		
		if(mStarted)
			return;
		
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
		pthread_create(&mThread, &attr, ThreadWrapper, this);
		pthread_attr_destroy(&attr);

		mStarted = true;
	}
	
	void MessageQueue::stop(){
		Mutex::Autolock ao(mLock);

		if(!mStarted)
			return;

		mDone = true;
		while(!mThreadExited){
			mCond.waitRelative(mLock, 40000000);
		}
	}

}//namespace
