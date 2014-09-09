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

#define LOG_TAG "MediaBuffer"
#include "Log.h"

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>

#include "ADebug.h"
#include "MediaBuffer.h"

#include <sys/atomics.h>

namespace openamedia {

	MediaBuffer::MediaBuffer(void *data, size_t size)
		: mObserver(NULL),
		  mNextBuffer(NULL),
		  mRefCount(0),
		  mData(data),
		  mSize(size),
		  mRangeOffset(0),
		  mRangeLength(size),
		  mMetaData(new MetaData),
		  mOwnsData(false),
		  mOriginal(NULL) {
	}

	MediaBuffer::MediaBuffer(size_t size)
		: mObserver(NULL),
		  mNextBuffer(NULL),
		  mRefCount(0),
		  mData(malloc(size)),
		  mSize(size),
		  mRangeOffset(0),
		  mRangeLength(size),
		  mMetaData(new MetaData),
		  mOwnsData(true),
		  mOriginal(NULL) {
	}

	void MediaBuffer::release() {
		if (mObserver == NULL) {
			CHECK_EQ(mRefCount, 0);
			delete this;
			return;
		}

		int prevCount = __atomic_dec(&mRefCount);
		if (prevCount == 1) {
			if (mObserver == NULL) {
				delete this;
				return;
			}

			mObserver->signalBufferReturned(this);
		}
		CHECK(prevCount > 0);
	}

	void MediaBuffer::add_ref() {
		(void) __atomic_inc(&mRefCount);
	}

	void *MediaBuffer::data() const {
		return mData;
	}

	size_t MediaBuffer::size() const {
		return mSize;
	}

	size_t MediaBuffer::range_offset() const {
		return mRangeOffset;
	}

	size_t MediaBuffer::range_length() const {
		return mRangeLength;
	}

	void MediaBuffer::set_range(size_t offset, size_t length) {
		if (offset + length > mSize) {
			ALOGE("offset = %d, length = %d, mSize = %d", offset, length, mSize);
		}
		CHECK(offset + length <= mSize);

		mRangeOffset = offset;
		mRangeLength = length;
	}

	MetaData* MediaBuffer::meta_data() {
		return mMetaData;
	}

	void MediaBuffer::reset() {
		mMetaData->clear();
		set_range(0, mSize);
	}

	MediaBuffer::~MediaBuffer() {
		CHECK(mObserver == NULL);

		if (mOwnsData && mData != NULL) {
			free(mData);
			mData = NULL;
		}

		if (mOriginal != NULL) {
			mOriginal->release();
			mOriginal = NULL;
		}
	}

	void MediaBuffer::setObserver(MediaBufferObserver *observer) {
		CHECK(observer == NULL || mObserver == NULL);
		mObserver = observer;
	}

	void MediaBuffer::setNextBuffer(MediaBuffer *buffer) {
		mNextBuffer = buffer;
	}

	MediaBuffer *MediaBuffer::nextBuffer() {
		return mNextBuffer;
	}

	int MediaBuffer::refcount() const {
		return mRefCount;
	}

	MediaBuffer *MediaBuffer::clone() {
		MediaBuffer *buffer = new MediaBuffer(mData, mSize);
		buffer->set_range(mRangeOffset, mRangeLength);
		buffer->mMetaData = new MetaData(*mMetaData);

		add_ref();
		buffer->mOriginal = this;

		return buffer;
	}

	void MediaBuffer::copyDataFrom(MediaBuffer* buffer) {
		if(buffer->size() > mSize){
			free(mData);
			mData = malloc(buffer->size());
			mSize = buffer->size();
		}

		memcpy(mData, buffer->data(), buffer->size());
		mRangeOffset = buffer->range_offset();
		mRangeLength = buffer->range_length();

		delete mMetaData;
		mMetaData = new MetaData(*buffer->meta_data());
	}

}  // namespace android
