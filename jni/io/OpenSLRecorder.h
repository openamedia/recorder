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

#ifndef OPENAMEDIA_OPENSL_RECORDER_H
#define OPENAMEDIA_OPENSL_RECORDER_H

extern "C"
{
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
}

#include <stdint.h>

#include "android/MetaData.h"
#include "Common.h"

namespace openamedia {

	class OpenSLRecorder {
	public:
		OpenSLRecorder(bool (*acquireBuffer)( void** data, int* dataSize, void* context), void* context);
		~OpenSLRecorder();

		bool init(int sampleRate, int channels);

		bool start(void* data, int dataSize);
		bool stop();
		
	private:
		bool mInited;
		bool mStarted;
		MetaData* mMetaData;
		
		// engine interfaces
		SLObjectItf engineObject;
		SLEngineItf engineEngine;

		// output mix interfaces
		SLObjectItf outputMixObject;
		SLEnvironmentalReverbItf outputMixEnvironmentalReverb;
		
		// buffer queue player interfaces
		SLObjectItf recorderObject;
		SLRecordItf recorderRecord;
		SLAndroidSimpleBufferQueueItf recorderBufferQueue;
		static SLEnvironmentalReverbSettings reverbSettings;

		bool deInit();
		void reset();

		static void bqRecorderCallback(SLAndroidSimpleBufferQueueItf bq, void *context);
		void bufferQueueCallback(SLAndroidSimpleBufferQueueItf bq);

		bool (*mAcquireBuffer)(void**, int*, void*);
		void* mAcquireBufferContext;

		OpenSLRecorder(const OpenSLRecorder&);
		OpenSLRecorder& operator=(const OpenSLRecorder&);
	};
	
}//namespace

#endif//OPENAMEDIA_OPENSL_MIXER_H
