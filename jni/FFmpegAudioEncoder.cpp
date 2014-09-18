#include "FFmpegAudioEncoder.h"

#define LOG_TAG "FFmpegAudioEncoder"
#include "android/Log.h"

namespace openamedia {

	FFmpegAudioEncoder::FFmpegAudioEncoder()
		:mInited(false),
		 mBitRate(64000),
		 mSampleRate(0),
		 mChannels(0),
		 mCodecID(AV_CODEC_ID_NONE),
		 codec(NULL),
		 c(NULL),
		 frame(NULL),
		 buffer_size(0),
		 mOut(NULL),
		 mOutCapacity(0),
		 mOutSize(0){
		mOut = malloc(DEFAULT_AUDIO_ENCODER_OUT_CAPACITY);
		mOutCapacity = DEFAULT_AUDIO_ENCODER_OUT_CAPACITY;
	}
	
	FFmpegAudioEncoder::~FFmpegAudioEncoder(){
		deInit();
		free(mOut);
	}

	bool FFmpegAudioEncoder::setBitRate(int bitRate){
		mBitRate = bitRate;

		return true;
	}
	
	bool FFmpegAudioEncoder::setSampleRate(int sampleRate){
		if(sampleRate != 8000 && sampleRate != 16000 && sampleRate != 44100){
			ALOGE("samplerate %d is not a valid one!", sampleRate);
			return false;
		}
		
		mSampleRate = sampleRate;

		return true;
	}
	
	bool FFmpegAudioEncoder::setChannels(int channels){
		if(channels != 1 && channels != 2){
			ALOGE("channels :%d is not a valid one!", channels);
			return false;
		}
		
		mChannels = channels;

		return true;
	}

	bool FFmpegAudioEncoder::setEncoder(CodecType codec){
		switch(codec){
		case CODEC_TYPE_MP3:
			mCodecID = AV_CODEC_ID_MP3;
			break;
		case CODEC_TYPE_AAC:
			mCodecID = AV_CODEC_ID_AAC;
			break;
		case CODEC_TYPE_H264:
			mCodecID = AV_CODEC_ID_H264;
			break;
		default:
			return false;
		}

		return true;
	}

	bool FFmpegAudioEncoder::getEncoderBufferSize(int* size){
		if(!mInited)
			return false;
		
		*size = buffer_size;
		
		return true;
	}

	bool FFmpegAudioEncoder::init(){
		if(mInited)
			return true;

		ALOGE("FFmpegAudioEncoder::init 0");
		
		do{
			int ret;
		
			/* find the MP2 encoder */
			codec = avcodec_find_encoder(mCodecID);
			if (!codec) {
				ALOGE("Codec not found!!");
				break;
			}

			ALOGE("codec name:%s", codec->name);
			
			c = avcodec_alloc_context3(codec);
			if (!c) {
				ALOGE("Could not allocate audio codec context!!!");
				break;
			}
		
			/* put sample parameters */
			c->bit_rate = mBitRate;
			/* check that the encoder supports s16 pcm input */
			if (!select_sample_fmt(AV_SAMPLE_FMT_S16)) {//TODO: should not be fixed!!
				ALOGE("Encoder does not support sample format %s!!", av_get_sample_fmt_name(AV_SAMPLE_FMT_S16));
				break;
			}

			if(!select_sample_rate(mSampleRate)){
				ALOGE("Encoder does not support sample rate:%d", mSampleRate);
				break;
			}

			if(!select_channels(mChannels)){
				ALOGE("Encoder does not support channels:%d", mChannels);
				break;
			}
			
			/* open it */
			if (avcodec_open2(c, codec, NULL) < 0) {
				ALOGE("Could not open codec!!");
				break;
			}

			/* frame containing input raw audio */
			frame = av_frame_alloc();
			if (!frame) {
				ALOGE("Could not allocate audio frame!!");
				break;
			}
		
			frame->nb_samples     = c->frame_size;
			frame->format         = c->sample_fmt;
			frame->channel_layout = c->channel_layout;	
			
			/* the codec gives us the frame size, in samples,
			 * we calculate the size of the samples buffer in bytes */
			buffer_size = av_samples_get_buffer_size(NULL, c->channels, c->frame_size,
													 c->sample_fmt, 0);
			if (buffer_size < 0) {
				ALOGE("Could not get sample buffer size!!");
				break;
			}
					
			mInited = true;
		}while(0);

		if(!mInited)
			reset();

		//ALOGE("FFmpegAudioEncoder::init out buffersize:%d, c->channels:%d, layout:%d, c->sample_fmt:%d, c->frame_size:%d, c->sample_rate:%d", buffer_size, c->channels, c->channel_layout, c->sample_fmt, c->frame_size, c->sample_rate);

		return mInited;
	}

	/* check that a given sample format is supported by the encoder */
	bool FFmpegAudioEncoder::select_sample_fmt(AVSampleFormat sample_fmt){
		const enum AVSampleFormat *p = codec->sample_fmts;
		while (*p != AV_SAMPLE_FMT_NONE) {
			if (*p == sample_fmt){
				c->sample_fmt = sample_fmt;
				return true;
			}
			p++;
		}
		
		return false;
	}
	
	/* just pick the highest supported samplerate */
	bool FFmpegAudioEncoder::select_sample_rate(int sampleRate){
		const int *p;
		if(!codec->supported_samplerates){
			c->sample_rate = sampleRate;
			return true;
		}
		
		p = codec->supported_samplerates;
		while (*p) {
			if(*p == sampleRate){
				c->sample_rate = sampleRate;
				return true;
			}
			p++;
		}
		
		return false;
	}
	
	/* select layout with the highest channel count */
	bool FFmpegAudioEncoder::select_channels(int channels){
		const uint64_t *p;
		if (!codec->channel_layouts){
			c->channel_layout = AV_CH_LAYOUT_STEREO;
			c->channels = av_get_channel_layout_nb_channels(c->channel_layout);

			return true;
		}
		
		p = codec->channel_layouts;
		while (*p) {
			int nb_channels = av_get_channel_layout_nb_channels(*p);
			if (nb_channels == channels){
				c->channel_layout = *p;
				c->channels = nb_channels;
				return true;
			}
			p++;
		}
		
		return false;
	}

	bool FFmpegAudioEncoder::deInit(){
		if(!mInited)
			return true;

		reset();

		mInited = false;

		return true;
	}
	
	void FFmpegAudioEncoder::reset(){
		if(frame != NULL){
			av_frame_free(&frame);
			frame = NULL;
		}

		if(c != NULL){
			avcodec_close(c);
			av_free(c);
			c = NULL;
		}
	}
	
	bool FFmpegAudioEncoder::encode(void* in, int inSize, void** out, int* outSize){
		if(!mInited)
			return false;

		ALOGE("encode inSize:%d", inSize);
		
		int left = inSize;
		int fill = 0;
		int ret, got_output;
		mOutSize = 0;
		
		while(left > 0){
			fill = buffer_size>left?left:buffer_size;
			left -= fill;
			
			av_init_packet(&pkt);
			pkt.data = NULL; // packet data will be allocated by the encoder
			pkt.size = 0;
		
			/* setup the data pointers in the AVFrame */
			ret = avcodec_fill_audio_frame(frame, c->channels, c->sample_fmt,
										   (const uint8_t*)in, fill, 0);
			if (ret < 0) {
				ALOGE("Could not setup audio frame fill:%d!!", fill);
				av_free_packet(&pkt);
				return false;
			}

			/* encode the samples */
			ret = avcodec_encode_audio2(c, &pkt, frame, &got_output);
			if (ret < 0) {
				ALOGE("Error encoding audio frame!!");
				av_free_packet(&pkt);
				return false;
			}

#if 0
				static int count = 0;
				char a[50] = {0};
				sprintf(a, "/sdcard/epcm%d", count++);
				FILE* f1 = fopen(a, "ab");
				if(f1 != NULL){
					size_t res = fwrite(pkt.data, 1, pkt.size, f1);
					fclose(f1);
					ALOGV("fwrite %d of %d to /sdcard/epcm!", res, pkt.size);
				}else
					ALOGE("can not fopen /sdcard/epcm!!");
#endif

			
			if (got_output) {
				//fwrite(pkt.data, 1, pkt.size, f);
				if((mOutSize + pkt.size) > mOutCapacity){
					void* temp = malloc(mOutSize + pkt.size);
					memcpy(temp, mOut, mOutSize);
					free(mOut);
					mOut = temp;
					mOutCapacity = mOutSize + pkt.size;
				}
				
				memcpy((unsigned char*)mOut + mOutSize, pkt.data, pkt.size);
				mOutSize += pkt.size;
				av_free_packet(&pkt);
			}
		}

		if(mOutSize <= 0){
			ALOGE("encode out size 0, failed!");
			return false;
		}
		
		*out = mOut;
		*outSize = mOutSize;
		
		return true;
	}
	
}//namespace
