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

#include "MediaInfo.h"

namespace openamedia {
	VideoInfo::VideoInfo()
		:width(0),
		 height(0),
		 frameRate(0),
		 colorFmt(OMX_COLOR_FormatUnused),
		 streamDuration(0),
		 bitRate(0){
	}
	
	VideoInfo::~VideoInfo(){
	}

	AudioInfo::AudioInfo()
		:sampleRate(0),
		 channels(0),
		 bitsPerSample(0),
		 streamDuration(0),
		 bitRate(0){
	}
	
	AudioInfo::~AudioInfo(){
	}
	
	MediaBuffer::MediaBuffer()
		:type(MEDIA_TYPE_UNKNOWN),
		 ptsMS(0),
		 data(NULL),
		 size(0),
		 decompressed(false){
	}
	
	MediaBuffer::~MediaBuffer(){
	}

}//namespace
