#ifndef OPENAMEDIA_LMRSOURCE_H
#define OPENAMEDIA_LMRSOURCE_H

#include "Prefetcher.h"
#include "io/OpenSLRecorder.h"
#include "android/MetaData.h"

namespace openamedia {

#define LMR_AUDIO_RECORD_BUFFERS_NUM 5
#define LMR_AUDIO_RECORD_BUFFER_SIZE 1024
	
	class LMRAudioSource {
	public:
		LMRAudioSource();
		~LMRAudioSource();

		bool init(int sampleRate, int channels, int bufSize);
		
		bool start();
		bool stop();
		status_t read(MediaBuffer** buffer);
		
	private:
		Mutex mLock;
		Condition mCond;
		bool mInited;
		bool mStarted;
		bool mDone;
		OpenSLRecorder* mSLRecorder;
		MetaData* mMetaData;

		MediaBufferGroup* mGroup;
		List<MediaBuffer*> mBusyList;
		MediaBuffer* mSLBuffer;

		bool deInit();
		void reset();
		
		static bool AcquireBuffer(void** data, int* dataSize, void* context);
		bool acquireBuffer(void** data, int* dataSize);

		LMRAudioSource(const LMRAudioSource&);
		LMRAudioSource& operator=(const LMRAudioSource&);
	};
	
	class LMRVideoSource {
		
	};

	class LMRSource : public Prefetcher::Source {
	public:
		LMRSource();
		virtual ~LMRSource();

		bool init(MetaData* meta);
		virtual bool start();
		virtual bool stop();
		virtual status_t read(MediaBuffer** buffer);
		virtual bool seek(int64_t timeMS);

		MetaData* getMetaData();
		
	private:
		bool mInited;
		bool mStarted;
		MetaData* mMetaData;
		LMRAudioSource* mAudioSource;
		LMRVideoSource* mVideoSource;

		bool deInit();
		void reset();
		
		LMRSource(const LMRSource&);
		LMRSource& operator=(const LMRSource&);
	};
	
}//namespace

#endif//OPENAMEDIA_LMRSOURCE_H
