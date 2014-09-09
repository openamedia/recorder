#ifndef OPENAMEDIA_FFMPEG_AUDIO_ENCODER_H
#define OPENAMEDIA_FFMPEG_AUDIO_ENCODER_H

#include "Common.h"

extern "C"{
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
#include <libavutil/common.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavutil/samplefmt.h>
}

namespace openamedia {

#define DEFAULT_AUDIO_ENCODER_OUT_CAPACITY 1*1024*1024
	
	class FFmpegAudioEncoder {
	public:
		FFmpegAudioEncoder();
		~FFmpegAudioEncoder();

		bool setBitRate(int bitRate);
		bool setSampleRate(int sampleRate);
		bool setChannels(int channels);
		bool setEncoder(CodecType codec);
		
		bool getEncoderBufferSize(int* size);
		
		bool init();
		
		bool encode(void* in, int inSize, void** out, int* outSize);
		
	private:
		bool mInited;
		int mBitRate;
		int mSampleRate;
		int mChannels;
		AVCodecID mCodecID;

		AVCodec *codec;
		AVCodecContext *c;
		AVFrame *frame;
		AVPacket pkt;
		int buffer_size;

		void* mOut;
		int mOutCapacity;
		int mOutSize;
		
		bool deInit();
		void reset();

		bool select_sample_fmt(AVSampleFormat sample_fmt);
		bool select_sample_rate(int sampleRate);
		bool select_channels(int channels);
		
		FFmpegAudioEncoder(const FFmpegAudioEncoder&);
		FFmpegAudioEncoder& operator=(const FFmpegAudioEncoder&);
	};

}//namespace

#endif//OPENAMEDIA_FFMPEG_AUDIO_ENCODER_H
