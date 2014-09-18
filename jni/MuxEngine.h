#ifndef OPENAMEDIA_MUX_ENGINE_H
#define OPENAMEDIA_MUX_ENGINE_H

#include <pthread.h>

#include "android/Mutex.h"
#include "android/Condition.h"
#include "Prefetcher.h"
#include "FFMPEGer.h"
#include "FFmpegAudioEncoder.h"

namespace openamedia {

	class MuxEngine {
	public:
		MuxEngine();
		virtual ~MuxEngine();

		void setOutputFile(const char* path);

		void setAudioSource(Prefetcher::SubSource* src);
		void setVideoSource(Prefetcher::SubSource* src);

		bool init();
		
		bool start();
		bool stop();

		bool getAudioEncodeBufferSize(int* bufSize);
		
	private:
		Mutex mLock;
		Condition mCond;
		pthread_t mThreadID;
		bool mInited;
		bool mStarted;
		bool mDone;
		bool mThreadExited;

		Prefetcher::SubSource* mAudioSrc;
		Prefetcher::SubSource* mVideoSrc;

		FFMPEGer* mFFMPEG;
		FFmpegAudioEncoder* mAudioEncoder;

		bool mNextReadAudio;

		char mOutputFile[MAX_STRING_PATH_LEN];

		bool deInit();
		void reset();

		static void* ThreadWrapper(void* context);
		void threadEntry();
		
		MuxEngine(const MuxEngine&);
		MuxEngine& operator=(const MuxEngine&);
	};
	
}//namespace

#endif//OPENAMEDIA_MUX_ENGINE_H
