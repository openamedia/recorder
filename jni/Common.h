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

#ifndef OPENAMEDIA_COMMON_H
#define OPENAMEDIA_COMMON_H

#include <stdint.h>

namespace openamedia {

#define MAX_STRING_PATH_LEN 512

	enum MediaType {
		MEDIA_TYPE_UNKNOWN = 0,
		MEDIA_TYPE_AUDIO,
		MEDIA_TYPE_VIDEO,
	};

	enum CodecType {
		CODEC_TYPE_MP3,
		CODEC_TYPE_AAC,
		CODEC_TYPE_H264,
	};

	int64_t getCurrentTimeUS();

}//namespace

#endif//OPENAMEDIA_COMMON_H
