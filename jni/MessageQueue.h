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

#ifndef OPENAMEDIA_MESSAGE_QUEUE_H
#define OPENAMEDIA_MESSAGE_QUEUE_H

#include <pthread.h>
#include "android/Mutex.h"
#include "android/Condition.h"
#include "android/List.h"

namespace openamedia {

	struct Message {
		int what;
		int arg1;
		int arg2;
		int64_t arg3;
		void* obj;
		int objSize;
	};
	
	class MessageQueue {
	public:
		MessageQueue(void (*handleMessage)(Message* msg, void* context), void (*handleThreadExit)(void* context), void* context);
		~MessageQueue();
		
		Message* obtainMessage();
		void sendMessage(Message* msg);
		
	private:
		Mutex mLock;
		Condition mCond;
		pthread_t mThread;
		bool mDone;
		bool mThreadExited;
		bool mStarted;

		List<Message*> mFreeList;
		List<Message*> mObtainList;
		List<Message*> mRunList;

		static void* ThreadWrapper(void* me);
		void threadEntry();

		void start();
		void stop();

		void (*mHandleMessage)(Message* msg, void* context);
		void (*mHandleThreadExitFnc)(void* context);
		void* mContext;
				
		MessageQueue(const MessageQueue&);
		MessageQueue& operator=(const MessageQueue&);
	};
	
}//namespace

#endif
