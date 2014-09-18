#ifndef OPENAMEDIA_LMRSOURCE_H
#define OPENAMEDIA_LMRSOURCE_H

#include "Prefetcher.h"
#include "io/OpenSLRecorder.h"
#include "android/MetaData.h"
#include "openmax/OMX_IVCommon.h"

namespace openamedia {

#define LMR_AUDIO_RECORD_BUFFERS_NUM 5
#define LMR_AUDIO_RECORD_BUFFER_SIZE 1024
	
	class LMRAudioSource {
	public:
		LMRAudioSource();
		~LMRAudioSource();

		void setSampleRate(int sampleRate);
		void setChannels(int channels);
		void setBufSize(int bufSize);

		bool init();
		
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

		int mSampleRate;
		int mChannels;
		int mBufSize;

		bool deInit();
		void reset();
		
		static bool AcquireBuffer(void** data, int* dataSize, void* context);
		bool acquireBuffer(void** data, int* dataSize);

		LMRAudioSource(const LMRAudioSource&);
		LMRAudioSource& operator=(const LMRAudioSource&);
	};
	
	class LMRVideoSource {
	public:
		LMRVideoSource();
		~LMRVideoSource();

		void setRect(int width, int height);
		void setColorFormat(OMX_COLOR_FORMATTYPE color);
		
		bool init();

		bool start();
		bool stop();
		status_t read(MediaBuffer** buffer);
		
		status_t write(void* data, int dataSize);

	private:
		Mutex mLock;
		Condition mCond;
		bool mInited;
		bool mStarted;
		bool mDone;
		MetaData* mMetaData;

		int mWidth;
		int mHeight;
		OMX_COLOR_FORMATTYPE mColor;

		MediaBufferGroup* mGroup;
		List<MediaBuffer*> mBusyList;

		bool deInit();
		void reset();
		
		LMRVideoSource(const LMRVideoSource&);
		LMRVideoSource& operator=(const LMRVideoSource&);
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

		status_t writeVideo(void* data, int dataSize);

		MetaData* getMetaData();
		
	private:
		bool mInited;
		bool mStarted;
		MetaData* mMetaData;
		LMRAudioSource* mAudioSource;
		LMRVideoSource* mVideoSource;

		bool mNextReadAudio;

		bool deInit();
		void reset();
		
		LMRSource(const LMRSource&);
		LMRSource& operator=(const LMRSource&);
	};
	
}//namespace

#endif//OPENAMEDIA_LMRSOURCE_H
