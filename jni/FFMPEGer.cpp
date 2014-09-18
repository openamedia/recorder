#include "FFMPEGer.h"

#define LOG_TAG "FFMPEGer"
#include "android/Log.h"

#include "openmax/OMX_IVCommon.h"

namespace openamedia {

#define STREAM_DURATION   10.0
#define STREAM_FRAME_RATE 15 /* 25 images/s */
#define STREAM_PIX_FMT    AV_PIX_FMT_YUV420P /* default pix_fmt */
#define SCALE_FLAGS SWS_BICUBIC
	
	FFMPEGer::FFMPEGer()
		:mInited(false),
		 fmt(NULL),
		 fmt_ctx(NULL),
		 audio_codec(NULL),
		 video_codec(NULL),
		 have_video(false),
		 have_audio(false),
		 mMetaData(NULL){
		memset(&video_st, 0, sizeof(OutputStream));
		memset(&audio_st, 0, sizeof(OutputStream));
	}
	
	FFMPEGer::~FFMPEGer(){
		deInit();
	}

	void FFMPEGer::setOutputFile(const char* path){
		strncpy(mOutputFile, path, sizeof(mOutputFile));
	}

	void FFMPEGer::setVideoSize(int width, int height){
		mWidth = width;
		mHeight = height;
	}
	
	void FFMPEGer::setVideoColorFormat(OMX_COLOR_FORMATTYPE fmt){
		mColor = fmt;
		if(mColor == OMX_COLOR_FormatYUV420SemiPlanar){
			mPixFmt = AV_PIX_FMT_NV21;
		}else if(mColor == OMX_COLOR_FormatYUV420Planar){
			mPixFmt = AV_PIX_FMT_YUV420P;
		}

	}
	
	bool FFMPEGer::init(MetaData* meta){
		if(mInited)
			return true;
		
		do{
			AVDictionary *opt = NULL;
			int ret;
			
			av_register_all();
			
			avformat_alloc_output_context2(&fmt_ctx, NULL, NULL, mOutputFile);
			if(fmt_ctx == NULL){
				ALOGE("fail to avformat_alloc_output_context2 for %s", mOutputFile);
				break;
			}

			fmt = fmt_ctx->oformat;
			
			/* Add the audio and video streams using the default format codecs
			 * and initialize the codecs. */
			if (fmt->video_codec != AV_CODEC_ID_NONE) {
				add_stream(&video_st, fmt_ctx, &video_codec, fmt->video_codec);
				have_video = true;
			}
			if (fmt->audio_codec != AV_CODEC_ID_NONE) {
				add_stream(&audio_st, fmt_ctx, &audio_codec, fmt->audio_codec);
				have_audio = true;
			}
			
			if(!have_audio && !have_video){
				ALOGE("no audio or video codec found for the fmt!");
				break;
			}

			/* Now that all the parameters are set, we can open the audio and
			 * video codecs and allocate the necessary encode buffers. */
			if (have_video)
				open_video(video_codec, &video_st, opt);
			
			if (have_audio)
				open_audio(audio_codec, &audio_st, opt);
			
			/* open the output file, if needed */
			if (!(fmt->flags & AVFMT_NOFILE)) {
				ret = avio_open(&fmt_ctx->pb, mOutputFile, AVIO_FLAG_WRITE);
				if (ret < 0) {
					ALOGE("Could not open '%s': %s", mOutputFile, av_err2str(ret));
					break;
				}
			}

			/* Write the stream header, if any. */
			ret = avformat_write_header(fmt_ctx, NULL);
			if (ret < 0) {
				ALOGE("Error occurred when opening output file: %s", av_err2str(ret));
				break;
			}
			
			mInited = true;
		}while(0);

		if(!mInited)
			reset();

		return mInited;
	}

	/* Add an output stream. */
	bool FFMPEGer::add_stream(OutputStream *ost, AVFormatContext *oc,
							  AVCodec **codec,
							  enum AVCodecID codec_id) {
		AVCodecContext *c;
		int i;

		/* find the encoder */
		*codec = avcodec_find_encoder(codec_id);
		if (!(*codec)) {
			ALOGE("Could not find encoder for '%s'",
				  avcodec_get_name(codec_id));
			return false;
		}

		ALOGV("success to find encoder for '%s'",
			  avcodec_get_name(codec_id));
		
		ost->st = avformat_new_stream(oc, *codec);
		if (!ost->st) {
			ALOGE("Could not allocate stream");
			return false;
		}
		ost->st->id = oc->nb_streams-1;
		c = ost->st->codec;
		
		switch ((*codec)->type) {
		case AVMEDIA_TYPE_AUDIO:
			c->sample_fmt  = (*codec)->sample_fmts ?
				(*codec)->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;//TODO:
			c->bit_rate    = 64000;//TODO:
			c->sample_rate = 44100;//TODO:
			if ((*codec)->supported_samplerates) {
				c->sample_rate = (*codec)->supported_samplerates[0];
				for (i = 0; (*codec)->supported_samplerates[i]; i++) {
					if ((*codec)->supported_samplerates[i] == 44100)
						c->sample_rate = 44100;
				}
			}
			c->channels = av_get_channel_layout_nb_channels(c->channel_layout);
			c->channel_layout = AV_CH_LAYOUT_STEREO;
			if ((*codec)->channel_layouts) {
				c->channel_layout = (*codec)->channel_layouts[0];
				for (i = 0; (*codec)->channel_layouts[i]; i++) {
					if ((*codec)->channel_layouts[i] == AV_CH_LAYOUT_STEREO)
						c->channel_layout = AV_CH_LAYOUT_STEREO;
				}
			}
			c->channels = av_get_channel_layout_nb_channels(c->channel_layout);
			ost->st->time_base = (AVRational){ 1, c->sample_rate };
			break;
		case AVMEDIA_TYPE_VIDEO:
			c->codec_id = codec_id;
			if(codec_id == AV_CODEC_ID_H264){
				c->profile = FF_PROFILE_H264_BASELINE;
			}
			
			c->bit_rate = 400000;
			/* Resolution must be a multiple of two. */
			c->width    = mWidth;
			c->height   = mHeight;
			/* timebase: This is the fundamental unit of time (in seconds) in terms
			 * of which frame timestamps are represented. For fixed-fps content,
			 * timebase should be 1/framerate and timestamp increments should be
			 * identical to 1. */
			ost->st->time_base = (AVRational){ 1, STREAM_FRAME_RATE };
			c->time_base       = ost->st->time_base;
			c->gop_size      = 12; /* emit one intra frame every twelve frames at most */
			c->pix_fmt       = STREAM_PIX_FMT;//input fmt for the encoder.
			break;
		default:
			break;
		}
		/* Some formats want stream headers to be separate. */
		if (oc->oformat->flags & AVFMT_GLOBALHEADER)
			c->flags |= CODEC_FLAG_GLOBAL_HEADER;

		return true;
	}

	bool FFMPEGer::open_audio(AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg){
		AVCodecContext *c;
		int nb_samples;
		int ret;
		AVDictionary *opt = NULL;
		c = ost->st->codec;
		/* open it */
		av_dict_copy(&opt, opt_arg, 0);

		ret = avcodec_open2(c, codec, &opt);
		av_dict_free(&opt);
		if (ret < 0) {
			ALOGE("Could not open audio codec: %s", av_err2str(ret));
			return false;
		}

		if (c->codec->capabilities & CODEC_CAP_VARIABLE_FRAME_SIZE)
			nb_samples = 10000;
		else
			nb_samples = c->frame_size;
				
		ost->frame = alloc_audio_frame(c->sample_fmt, c->channel_layout,
									   c->sample_rate, nb_samples);
		ost->tmp_frame = alloc_audio_frame(AV_SAMPLE_FMT_S16, c->channel_layout,
										   c->sample_rate, nb_samples);

		/* create resampler context */
        ost->swr_ctx = swr_alloc();
        if (!ost->swr_ctx) {
            ALOGE("Could not allocate resampler context");
            return false;
        }
		
        /* set options */
        av_opt_set_int       (ost->swr_ctx, "in_channel_count",   c->channels,       0);
        av_opt_set_int       (ost->swr_ctx, "in_sample_rate",     c->sample_rate,    0);
        av_opt_set_sample_fmt(ost->swr_ctx, "in_sample_fmt",      AV_SAMPLE_FMT_S16, 0);
        av_opt_set_int       (ost->swr_ctx, "out_channel_count",  c->channels,       0);
        av_opt_set_int       (ost->swr_ctx, "out_sample_rate",    c->sample_rate,    0);
        av_opt_set_sample_fmt(ost->swr_ctx, "out_sample_fmt",     c->sample_fmt,     0);

		/* initialize the resampling context */
		ret = swr_init(ost->swr_ctx);
        if (ret < 0){
            ALOGE("Failed to initialize the resampling context");
			return false;
		}

		return true;
	}

	AVFrame* FFMPEGer::alloc_audio_frame(enum AVSampleFormat sample_fmt,
										 uint64_t channel_layout,
										 int sample_rate, int nb_samples) {
		AVFrame *frame = av_frame_alloc();
		int ret;
		if (!frame) {
			ALOGE("Error allocating an audio frame");
			return NULL;
		}
		
		frame->format = sample_fmt;
		frame->channel_layout = channel_layout;
		frame->sample_rate = sample_rate;
		frame->nb_samples = nb_samples;
		
		if (nb_samples) {
			ret = av_frame_get_buffer(frame, 0);
			if (ret < 0) {
				ALOGE("Error allocating an audio buffer");
				return NULL;
			}
		}
		
		return frame;
	}

	bool FFMPEGer::open_video(AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg){
		int ret;
		AVCodecContext *c = ost->st->codec;
		//AVDictionary *opt = NULL;
		//av_dict_copy(&opt, opt_arg, 0);

		/* open the codec */
		ret = avcodec_open2(c, codec, NULL);
		//av_dict_free(&opt);
		if (ret < 0) {
			ALOGE("Could not open video codec: %s", av_err2str(ret));
			return false;
		}
		
		/* allocate and init a re-usable frame */
		ost->frame = alloc_picture(c->pix_fmt, c->width, c->height);
		if (!ost->frame) {
			ALOGE("Could not allocate video frame");
			return false;
		}
		
		/* If the output format is not YUV420P, then a temporary YUV420P
		 * picture is needed too. It is then converted to the required
		 * output format. */
		ost->tmp_frame = NULL;
		if (c->pix_fmt != mPixFmt) {
			ALOGE("open_video alloc_picture for te");
			ost->tmp_frame = alloc_picture(mPixFmt, c->width, c->height);
			if (!ost->tmp_frame) {
				ALOGE("Could not allocate temporary picture");
				return false;
			}
		}

		return true;
	}

	AVFrame* FFMPEGer::alloc_picture(enum AVPixelFormat pix_fmt, int width, int height) {
		AVFrame *picture;
		int ret;

		picture = av_frame_alloc();
		if (!picture)
			return NULL;
		
		picture->format = pix_fmt;
		picture->width  = width;
		picture->height = height;

		/* allocate the buffers for the frame data */
		ret = av_frame_get_buffer(picture, 32);
		if (ret < 0) {
			ALOGE("Could not allocate frame data.");
			return NULL;
		}
		
		return picture;
	}

	bool FFMPEGer::deInit(){
		if(!mInited)
			return true;

		reset();

		mInited = false;

		return true;
	}
	
	void FFMPEGer::reset(){
		/* Close each codec. */
		if (have_video)
			close_stream(fmt_ctx, &video_st);
		if (have_audio)
			close_stream(fmt_ctx, &audio_st);
		
		if (!(fmt->flags & AVFMT_NOFILE)){
			/* Close the output file. */
			avio_close(fmt_ctx->pb);
			fmt_ctx->pb = NULL;
		}
	   		
		/* free the stream */
		if(fmt_ctx != NULL){
			avformat_free_context(fmt_ctx);
			fmt_ctx = NULL;
		}
	}

	void FFMPEGer::close_stream(AVFormatContext *oc, OutputStream *ost){
		if(ost->st->codec != NULL){
			avcodec_close(ost->st->codec);
		}
			
		if(ost->frame != NULL){
			av_frame_free(&ost->frame);
			ost->frame = NULL;
		}

		if(ost->tmp_frame != NULL){
			av_frame_free(&ost->tmp_frame);
			ost->tmp_frame = NULL;
		}

		if(ost->sws_ctx != NULL){
			sws_freeContext(ost->sws_ctx);
			ost->sws_ctx = NULL;
		}

		if(ost->swr_ctx != NULL){
			swr_free(&ost->swr_ctx);
			ost->swr_ctx = NULL;
		}
	}
	
	status_t FFMPEGer::encodeAudio(MediaBuffer* src, MediaBuffer* dst){
		AVCodecContext *c;
		
		AVFrame *frame = NULL;
		int ret;
		int got_packet;
		int dst_nb_samples;
		OutputStream* ost = &audio_st;
		
		unsigned char* srcData = (unsigned char*)src->data() + src->range_offset();
		int copySize = getAudioEncodeBufferSize();

		while(srcData < ((unsigned char*)src->data() + src->range_offset() + src->range_length())){
			AVPacket pkt = { 0 }; // data and size must be 0;
			av_init_packet(&pkt);
			c = ost->st->codec;
		
			frame = audio_st.tmp_frame;
			memcpy(frame->data[0], srcData, copySize);
			srcData += copySize;
			frame->pts = audio_st.next_pts;
			
			audio_st.next_pts += frame->nb_samples;

			if (frame) {
				/* convert samples from native format to destination codec format, using the resampler */
				/* compute destination number of samples */
				dst_nb_samples = av_rescale_rnd(swr_get_delay(ost->swr_ctx, c->sample_rate) + frame->nb_samples,
												c->sample_rate, c->sample_rate, AV_ROUND_UP);
				av_assert0(dst_nb_samples == frame->nb_samples);

				/* when we pass a frame to the encoder, it may keep a reference to it
				 * internally;
				 * make sure we do not overwrite it here
				 */
				ret = av_frame_make_writable(ost->frame);
				if (ret < 0)
					return UNKNOWN_ERROR;

				/* convert to destination format */
				ret = swr_convert(ost->swr_ctx,
								  ost->frame->data, dst_nb_samples,
								  (const uint8_t **)frame->data, frame->nb_samples);
				if (ret < 0) {
					ALOGE("Error while converting");
					return UNKNOWN_ERROR;
				}
				
				frame = ost->frame;
				frame->pts = av_rescale_q(ost->samples_count, (AVRational){1, c->sample_rate}, c->time_base);
				ost->samples_count += dst_nb_samples;
			}

			ret = avcodec_encode_audio2(c, &pkt, frame, &got_packet);
			if (ret < 0) {
				ALOGE("Error encoding audio frame: %s", av_err2str(ret));
				return UNKNOWN_ERROR;
			}

			pkt.pts = frame->pts;//
			
#if 0
			static int count = 0;
			char a[50] = {0};
			sprintf(a, "/sdcard/pcm%d", count++);
			FILE* f1 = fopen(a, "ab");
			if(f1 != NULL){
				size_t res = fwrite(pkt.data, 1, pkt.size, f1);
				fclose(f1);
				ALOGV("fwrite %d of %d to /sdcard/pcm!", res, pkt.size);
			}else
				ALOGE("can not fopen /sdcard/pcm!!");
#endif
			
			
			if (got_packet) {
				ret = write_frame(fmt_ctx, &c->time_base, ost->st, &pkt);
				if (ret < 0) {
					ALOGE("Error while writing audio frame: %s", av_err2str(ret));
					return UNKNOWN_ERROR;
				}
			}
		}
	}

	int FFMPEGer::write_frame(AVFormatContext *fmt_ctx, const AVRational *time_base, AVStream *st, AVPacket *pkt) {
		/* rescale output packet timestamp values from codec to stream timebase */
		//av_packet_rescale_ts(pkt, *time_base, st->time_base);
		
		pkt->stream_index = st->index;
		/* Write the compressed frame to the media file. */
		//log_packet(fmt_ctx, pkt);
		return av_interleaved_write_frame(fmt_ctx, pkt);
	}
	
	status_t FFMPEGer::encodeVideo(MediaBuffer* src, MediaBuffer* dst){
		int ret;
		OutputStream* ost = &video_st;
		AVCodecContext *c;
		AVFrame *frame;
		int got_packet = 0;
		c = ost->st->codec;
		
		if(mPixFmt == AV_PIX_FMT_NV21){
			memcpy(ost->tmp_frame->data[0], src->data(), c->width * c->height);
			memcpy(ost->tmp_frame->data[1], (char*)src->data() + c->width * c->height, c->width * c->height / 2);
		}else if(mPixFmt = AV_PIX_FMT_YUV420P){
			memcpy(ost->frame->data[0], src->data(), c->width * c->height);
			memcpy(ost->frame->data[1], (char*)src->data() + c->width * c->height, c->width * c->height / 4);
			memcpy(ost->frame->data[2], (char*)src->data() + c->width * c->height * 5 / 4, c->width * c->height / 4);
		}
		
		if (c->pix_fmt != mPixFmt) {
			/* as we only generate a YUV420P picture, we must convert it
			 * to the codec pixel format if needed */
			if (!ost->sws_ctx) {
				ost->sws_ctx = sws_getContext(c->width, c->height,
											  mPixFmt,
											  c->width, c->height,
											  c->pix_fmt,
											  SCALE_FLAGS, NULL, NULL, NULL);
				if (!ost->sws_ctx) {
					ALOGE("Could not initialize the conversion context");
					return UNKNOWN_ERROR;
				}
			}
			
			sws_scale(ost->sws_ctx,
					  (const uint8_t * const *)ost->tmp_frame->data, ost->tmp_frame->linesize,
					  0, c->height, ost->frame->data, ost->frame->linesize);
		}
		
		//ost->frame->pts = ost->next_pts++;
		ost->frame->pts = av_rescale_q(ost->next_pts++, (AVRational){1, 15}, ost->st->time_base);
		
		frame = ost->frame;

		if (fmt_ctx->oformat->flags & AVFMT_RAWPICTURE) {
			/* a hack to avoid data copy with some raw video muxers */
			AVPacket pkt = { 0 };
			av_init_packet(&pkt);
			
			pkt.flags        |= AV_PKT_FLAG_KEY;
			pkt.stream_index  = ost->st->index;
			pkt.data          = (uint8_t *)frame;
			pkt.size          = sizeof(AVPicture);
			pkt.pts = pkt.dts = frame->pts;
			//av_packet_rescale_ts(&pkt, c->time_base, ost->st->time_base);
			ret = av_interleaved_write_frame(fmt_ctx, &pkt);
		} else {
			AVPacket pkt = { 0 };
			av_init_packet(&pkt);
			
			/* encode the image */
			ret = avcodec_encode_video2(c, &pkt, frame, &got_packet);
			if (ret < 0) {
				ALOGE("Error encoding video frame: %s", av_err2str(ret));
				return UNKNOWN_ERROR;
			}
			
			if (got_packet) {
				ret = write_frame(fmt_ctx, &c->time_base, ost->st, &pkt);
			} else {
				ret = 0;
			}
		}
		
		if (ret < 0) {
			ALOGE("Error while writing video frame: %s", av_err2str(ret));
			return UNKNOWN_ERROR;
		}
		
		return OK;
	}

	status_t FFMPEGer::finish() {
		/* Write the trailer, if any. The trailer must be written before you
		 * close the CodecContexts open when you wrote the header; otherwise
		 * av_write_trailer() may try to use memory that was freed on
		 * av_codec_close(). */
		av_write_trailer(fmt_ctx);
	}
	
	void FFMPEGer::mux(){
		
	}
	
	MetaData* FFMPEGer::getMetaData(){
		return mMetaData;
	}
	
	int FFMPEGer::getAudioEncodeBufferSize(){
		if(!have_audio)
			return 0;
		
		int frameSize = 0;
		
		AVFrame *frame = audio_st.tmp_frame;
		int ret = av_samples_get_buffer_size(&frameSize, frame->channels,
											 frame->nb_samples, (AVSampleFormat)frame->format,
											 0);

		return frameSize;
	}
}
