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

#ifndef OPENAMEDIA_ANATIVE_WINDOW_RENDERER_H
#define OPENAMEDIA_ANATIVE_WINDOW_RENDERER_H

#include <android/native_window_jni.h>
#include "android/ColorConverter.h"

#include "Common.h"
#include "android/MetaData.h"
#include "android/MediaBuffer.h"

namespace openamedia {

	class ANativeWindowRenderer {
	public:
		ANativeWindowRenderer(ANativeWindow* window, MetaData* meta);
		~ANativeWindowRenderer();

		bool init();
		bool deInit();
		
		void render(MediaBuffer* buffer);
		
	private:
		ANativeWindow* mNativeWindow;
		ColorConverter* mConverter;

		MetaData* mMetaData;
		bool mInited;

		int mWidth;
		int mHeight;
		
		int mCropTop;
		int mCropLeft;
		int mCropRight;
        int mCropBottom;
		int mCropWidth;
		int mCropHeight;

		ARect mRect;

		void reset();

		ANativeWindowRenderer(const ANativeWindowRenderer&);
		ANativeWindowRenderer& operator=(const ANativeWindowRenderer&);
	};

}//namespace

#endif//ANATIVE_WINDOW_RENDERER_H
