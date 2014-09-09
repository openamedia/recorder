/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef OPENAMEDIA_MEDIA_BUFFER_H
#define OPENAMEDIA_MEDIA_BUFFER_H

#include <pthread.h>

#include "Errors.h"
#include "MetaData.h"

namespace openamedia {

	class MediaBuffer;
	class MediaBufferObserver;

	class MediaBufferObserver {
	public:
		MediaBufferObserver() {}
		virtual ~MediaBufferObserver() {}

		virtual void signalBufferReturned(MediaBuffer *buffer) = 0;

	private:
		MediaBufferObserver(const MediaBufferObserver &);
		MediaBufferObserver &operator=(const MediaBufferObserver &);
	};

	class MediaBuffer {
	public:
		// The underlying data remains the responsibility of the caller!
		MediaBuffer(void *data, size_t size);

		MediaBuffer(size_t size);

		// Decrements the reference count and returns the buffer to its
		// associated MediaBufferGroup if the reference count drops to 0.
		void release();

		// Increments the reference count.
		void add_ref();

		void *data() const;
		size_t size() const;

		size_t range_offset() const;
		size_t range_length() const;

		void set_range(size_t offset, size_t length);

		MetaData* meta_data();
		// Clears meta data and resets the range to the full extent.
		void reset();

		void setObserver(MediaBufferObserver *group);

		// Returns a clone of this MediaBuffer increasing its reference count.
		// The clone references the same data but has its own range and
		// MetaData.
		MediaBuffer *clone();

		//every member data includes data and size could change after this call.
		void copyDataFrom(MediaBuffer* buffer);

		int refcount() const;

	protected:
		virtual ~MediaBuffer();

	private:
		friend class MediaBufferGroup;

		MediaBufferObserver *mObserver;
		MediaBuffer *mNextBuffer;
		int mRefCount;

		void *mData;
		size_t mSize, mRangeOffset, mRangeLength;

		MetaData* mMetaData;
		bool mOwnsData;

		MediaBuffer *mOriginal;

		void setNextBuffer(MediaBuffer *buffer);
		MediaBuffer *nextBuffer();

		MediaBuffer(const MediaBuffer &);
		MediaBuffer &operator=(const MediaBuffer &);
	};

}  // namespace

#endif  // OPENAMEDIA_MEDIA_BUFFER_H
