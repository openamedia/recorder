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

#include "OpenSLRecorder.h"

#define LOG_TAG "OpenSLRecorder"
#include "android/Log.h"

namespace openamedia {

	SLEnvironmentalReverbSettings OpenSLRecorder::reverbSettings = SL_I3DL2_ENVIRONMENT_PRESET_STONECORRIDOR;
	
	OpenSLRecorder::OpenSLRecorder(bool (*acquireBuffer)(void** data, int* dataSize, void* context), void* context)
		:mInited(false),
		 mStarted(false),
		 engineObject(NULL),
		 engineEngine(NULL),
		 outputMixObject(NULL),
		 outputMixEnvironmentalReverb(NULL),
		 recorderObject(NULL),
		 recorderRecord(NULL),
		 recorderBufferQueue(NULL),
		 mAcquireBuffer(acquireBuffer),
		 mAcquireBufferContext(context){
	}
	
	OpenSLRecorder::~OpenSLRecorder(){
		deInit();
	}

	bool OpenSLRecorder::init(int sampleRate, int channels){
		if(mInited)
			return true;

		SLresult result;

		do{
			// create engine
			result = slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
			if(SL_RESULT_SUCCESS != result){
				ALOGE("failed to create slengine obj!!");
				break;
			}
			(void)result;

			// realize the engine
			result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
			if(SL_RESULT_SUCCESS != result){
				ALOGE("failed to realize slengine obj!!");
				break;
			}
			(void)result;

			// get the engine interface, which is needed in order to create other objects
			result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine);
			if(SL_RESULT_SUCCESS != result){
				ALOGE("failed to get engine itf!!");
				break;
			}
			(void)result;

			// create output mix, with environmental reverb specified as a non-required interface
			const SLInterfaceID ids[1] = {SL_IID_ENVIRONMENTALREVERB};
			const SLboolean req[1] = {SL_BOOLEAN_FALSE};
			result = (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 1, ids, req);
			if(SL_RESULT_SUCCESS != result){
				ALOGE("failed to create sloutput mix!!");
				break;
			}
			(void)result;

			// realize the output mix
			result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
			if(SL_RESULT_SUCCESS != result){
				ALOGE("failed to realize sl outputmix!!");
				break;
			}
			(void)result;

			// get the environmental reverb interface
			// this could fail if the environmental reverb effect is not available,
			// either because the feature is not present, excessive CPU load, or
			// the required MODIFY_AUDIO_SETTINGS permission was not requested and granted
			result = (*outputMixObject)->GetInterface(outputMixObject, SL_IID_ENVIRONMENTALREVERB,
													  &outputMixEnvironmentalReverb);
			if (SL_RESULT_SUCCESS == result) {
				result = (*outputMixEnvironmentalReverb)->SetEnvironmentalReverbProperties(outputMixEnvironmentalReverb, &reverbSettings);
				(void)result;
			}

			
			/////////////////// configure audio source
			SLDataLocator_IODevice loc_dev = {SL_DATALOCATOR_IODEVICE, SL_IODEVICE_AUDIOINPUT,
											  SL_DEFAULTDEVICEID_AUDIOINPUT, NULL};
			SLDataSource audioSrc = {&loc_dev, NULL};

			// configure audio sink
			SLDataLocator_AndroidSimpleBufferQueue loc_bq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
			SLDataFormat_PCM format_pcm = {SL_DATAFORMAT_PCM, 2, SL_SAMPLINGRATE_44_1,
										   SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16,
										   SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT, SL_BYTEORDER_LITTLEENDIAN};
			SLDataSink audioSnk = {&loc_bq, &format_pcm};

			// create audio recorder
			// (requires the RECORD_AUDIO permission)
			const SLInterfaceID ids2[1] = {SL_IID_ANDROIDSIMPLEBUFFERQUEUE};
			const SLboolean req2[1] = {SL_BOOLEAN_TRUE};
			result = (*engineEngine)->CreateAudioRecorder(engineEngine, &recorderObject, &audioSrc,
														  &audioSnk, 1, ids2, req2);
			if(SL_RESULT_SUCCESS != result){
				ALOGE("failed to create sl audio recorder obj!!");
				break;
			}

			// realize the audio recorder
			result = (*recorderObject)->Realize(recorderObject, SL_BOOLEAN_FALSE);
			if(SL_RESULT_SUCCESS != result){
				ALOGE("failed to realize sl recorder obj!!");
				break;
			}

			// get the record interface
			result = (*recorderObject)->GetInterface(recorderObject, SL_IID_RECORD, &recorderRecord);
			if(SL_RESULT_SUCCESS != result){
				ALOGE("failed to get record itf!!");
				break;
			}
			(void)result;

			// get the buffer queue interface
			result = (*recorderObject)->GetInterface(recorderObject, SL_IID_ANDROIDSIMPLEBUFFERQUEUE,
													 &recorderBufferQueue);
			if(SL_RESULT_SUCCESS != result){
				ALOGE("failed to get buffer queue itf for audio record!!");
				break;
			}
			(void)result;

			// register callback on the buffer queue
			result = (*recorderBufferQueue)->RegisterCallback(recorderBufferQueue, bqRecorderCallback,
															  this);
			if(SL_RESULT_SUCCESS != result){
				ALOGE("failed to register callback for audio record!!");
				break;
			}
			(void)result;
			
			mInited = true;
		}while(0);

		if(!mInited)
			reset();

		return mInited;
	}

	bool OpenSLRecorder::deInit(){
		if(!mInited)
			return true;

		reset();

		mInited = false;

		return true;
	}

	void OpenSLRecorder::reset(){
		// destroy audio recorder object, and invalidate all associated interfaces
		if (recorderObject != NULL) {
			(*recorderObject)->Destroy(recorderObject);
			recorderObject = NULL;
			recorderRecord = NULL;
			recorderBufferQueue = NULL;
		}

		// destroy output mix object, and invalidate all associated interfaces
		if (outputMixObject != NULL) {
			(*outputMixObject)->Destroy(outputMixObject);
			outputMixObject = NULL;
			outputMixEnvironmentalReverb = NULL;
		}
		
		// destroy engine object, and invalidate all associated interfaces
		if (engineObject != NULL) {
			ALOGE("OpenSLRecorder::reset 0 *engineObject:%p", *engineObject);
			(*engineObject)->Destroy(engineObject);
			engineObject = NULL;
			engineEngine = NULL;
		}
	}

	bool OpenSLRecorder::start(void* data, int dataSize){
		if(!mInited)
			return false;

		if(mStarted)
			return true;
		
		SLresult result;		
		// enqueue an empty buffer to be filled by the recorder
		// (for streaming recording, we would enqueue at least 2 empty buffers to start things off)
		result = (*recorderBufferQueue)->Enqueue(recorderBufferQueue, data,
												 dataSize);
		
		// the most likely other result is SL_RESULT_BUFFER_INSUFFICIENT,
		// which for this code example would indicate a programming error
		if(SL_RESULT_SUCCESS != result){
			ALOGE("failed to enqueue for sl record!!");
			return false;
		}
		(void)result;

		// start recording
		result = (*recorderRecord)->SetRecordState(recorderRecord, SL_RECORDSTATE_RECORDING);
		if(SL_RESULT_SUCCESS != result){
			ALOGE("failed to set recordstate to recording!!");
			return false;
		}
		(void)result;

		mStarted = true;

		return true;
	}

	bool OpenSLRecorder::stop(){
		if(!mInited)
			return false;

		if(!mStarted)
			return true;
		
		SLresult result = (*recorderRecord)->SetRecordState(recorderRecord, SL_RECORDSTATE_STOPPED);
		if(SL_RESULT_SUCCESS != result)
			ALOGE("failed to set sl audio record state to stop!!");
        (void)result;

		mStarted = false;

		return true;
	}
	
	void OpenSLRecorder::bqRecorderCallback(SLAndroidSimpleBufferQueueItf bq, void *context){
		OpenSLRecorder* recorder = (OpenSLRecorder*)context;
		
		recorder->bufferQueueCallback(bq);
	}
	
	void OpenSLRecorder::bufferQueueCallback(SLAndroidSimpleBufferQueueItf bq){		
		//last acquired buffer has been filled up completely.
		if(bq != recorderBufferQueue){
			ALOGE("callback bqRecorderCallback params bq not the same with the passed one!!");
			return;
		}

		bool res;
		void* data;
		int dataSize;
		res = mAcquireBuffer(&data, &dataSize, mAcquireBufferContext);//blocking call.
		if(!res){
			SLresult result;
			result = (*recorderRecord)->SetRecordState(recorderRecord, SL_RECORDSTATE_STOPPED);
			return;
		}
		
		//enqueue another buffer
		SLresult result = (*recorderBufferQueue)->Enqueue(recorderBufferQueue, data, dataSize);
		// the most likely other result is SL_RESULT_BUFFER_INSUFFICIENT,
		// which for this code example would indicate a programming error
		if(SL_RESULT_SUCCESS != result)
			ALOGE("failed to enqueue bufferqueue with frames:%d!!", dataSize);
		(void)result;
	}
	
}//namespace
