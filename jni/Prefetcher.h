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

#ifndef OPENAMEDIA_PREFETCHER_H
#define OPENAMEDIA_PREFETCHER_H

#include <pthread.h>
#include "android/List.h"
#include "android/Mutex.h"
#include "android/Condition.h"
#include "android/Errors.h"
#include "android/MediaBuffer.h"
#include "android/MediaBufferGroup.h"

#include "Common.h"

namespace openamedia {

#define MAX_SUB_SOURCE_BUFFER_SIZE (1920*1080*3/2)
#define DEFAULT_VIDEO_SUB_SOURCE_CAPACITY 40//decided by the max num of continual video packets from av_read_frame, especially after seeking.
#define DEFAULT_AUDIO_SUB_SOURCE_CAPACITY 20//normally, the num of audio packets(not samples) from av_read_frame is less than video ones.
	
	class Prefetcher {
	public:
		class Source {
		public:
			virtual ~Source(){}
			
			virtual bool start() = 0;
			virtual bool stop() = 0;
			virtual status_t read(MediaBuffer** buffer) = 0;
			virtual bool seek(int64_t timeMS) = 0;

		protected:
			Source(){}

		private:			
			Source(const Source&);
			Source& operator=(const Source&);
		};
		
		Prefetcher(Prefetcher::Source* src);
		~Prefetcher();

		bool start();
		bool stop();
		bool seek(int64_t timeMS);

		class SubSource {
		public:
			SubSource(int capacity);
			~SubSource();
			
			bool read(MediaBuffer** buffer);

		private:
			friend class Prefetcher;
			
			Mutex mLock;
			Condition mCond;
			bool mDone;

			bool write(MediaBuffer* buffer);
			void stop();
			void clear();
			void clear_l();
			
			List<MediaBuffer*> mBusyList;
			MediaBufferGroup mGroup;
			
			SubSource(const SubSource&);
			SubSource& operator=(const SubSource&);
		};

		/*
		  TODO: Prefetcher would better be a general one, without any binding to video/audio type.
		  :( unlike the Prefetcher in old android stagefright, here 1 source refers to more than 2 subsources. seems no good way to distinguish them without specifying mime type. 
		 */
		SubSource* getSource(MediaType type);
		
	private:
		Mutex mLock;
		Condition mCond;

		bool mStarted;
		Source* mSource;

		bool mSeeking;
		int64_t mSeekTime;
		
		SubSource* mVideoSource;
		SubSource* mAudioSource;

		pthread_t mThread;
		bool mDone;
		bool mThreadExited;

		static void* ThreadWrapper(void* me);
		void threadEntry();
			
		Prefetcher(const Prefetcher&);
		Prefetcher& operator=(const Prefetcher&);
	};
}

#endif
