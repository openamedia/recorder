/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef OPENAMEDIA_ERRORS_H
#define OPENAMEDIA_ERRORS_H

#include <sys/types.h>
#include <errno.h>

namespace openamedia {

	typedef int32_t status_t;

	enum Error {
		OK                = 0,    // Everything's swell.
		NO_ERROR          = 0,    // No errors.
    
		UNKNOWN_ERROR       = 0x80000000,

		NO_MEMORY           = -ENOMEM,
		INVALID_OPERATION   = -ENOSYS,
		BAD_VALUE           = -EINVAL,
		BAD_TYPE            = 0x80000001,
		NAME_NOT_FOUND      = -ENOENT,
		PERMISSION_DENIED   = -EPERM,
		NO_INIT             = -ENODEV,
		ALREADY_EXISTS      = -EEXIST,
		DEAD_OBJECT         = -EPIPE,
		FAILED_TRANSACTION  = 0x80000002,
		JPARKS_BROKE_IT     = -EPIPE,
		BAD_INDEX           = -EOVERFLOW,
		NOT_ENOUGH_DATA     = -ENODATA,
		WOULD_BLOCK         = -EWOULDBLOCK, 
		TIMED_OUT           = -ETIMEDOUT,
		UNKNOWN_TRANSACTION = -EBADMSG,
		FDS_NOT_ALLOWED     = 0x80000007,
	};

	enum MediaError {
		MEDIA_ERROR_BASE        = -1000,

		ERROR_ALREADY_CONNECTED = MEDIA_ERROR_BASE,
		ERROR_NOT_CONNECTED     = MEDIA_ERROR_BASE - 1,
		ERROR_UNKNOWN_HOST      = MEDIA_ERROR_BASE - 2,
		ERROR_CANNOT_CONNECT    = MEDIA_ERROR_BASE - 3,
		ERROR_IO                = MEDIA_ERROR_BASE - 4,
		ERROR_CONNECTION_LOST   = MEDIA_ERROR_BASE - 5,
		ERROR_MALFORMED         = MEDIA_ERROR_BASE - 7,
		ERROR_OUT_OF_RANGE      = MEDIA_ERROR_BASE - 8,
		ERROR_BUFFER_TOO_SMALL  = MEDIA_ERROR_BASE - 9,
		ERROR_UNSUPPORTED       = MEDIA_ERROR_BASE - 10,
		ERROR_END_OF_STREAM     = MEDIA_ERROR_BASE - 11,

		// Not technically an error.
		INFO_FORMAT_CHANGED    = MEDIA_ERROR_BASE - 12,
		INFO_DISCONTINUITY     = MEDIA_ERROR_BASE - 13,
		INFO_OUTPUT_BUFFERS_CHANGED = MEDIA_ERROR_BASE - 14,

		//
		ERROR_DECODE_FAILED    = MEDIA_ERROR_BASE - 15,
		ERROR_AUDIO_RESAMPLE_FAILED = MEDIA_ERROR_BASE - 16,

		// The following constant values should be in sync with
		// drm/drm_framework_common.h
		DRM_ERROR_BASE = -2000,

		ERROR_DRM_UNKNOWN                       = DRM_ERROR_BASE,
		ERROR_DRM_NO_LICENSE                    = DRM_ERROR_BASE - 1,
		ERROR_DRM_LICENSE_EXPIRED               = DRM_ERROR_BASE - 2,
		ERROR_DRM_SESSION_NOT_OPENED            = DRM_ERROR_BASE - 3,
		ERROR_DRM_DECRYPT_UNIT_NOT_INITIALIZED  = DRM_ERROR_BASE - 4,
		ERROR_DRM_DECRYPT                       = DRM_ERROR_BASE - 5,
		ERROR_DRM_CANNOT_HANDLE                 = DRM_ERROR_BASE - 6,
		ERROR_DRM_TAMPER_DETECTED               = DRM_ERROR_BASE - 7,
		ERROR_DRM_NOT_PROVISIONED               = DRM_ERROR_BASE - 8,
		ERROR_DRM_DEVICE_REVOKED                = DRM_ERROR_BASE - 9,

		ERROR_DRM_VENDOR_MAX                    = DRM_ERROR_BASE - 500,
		ERROR_DRM_VENDOR_MIN                    = DRM_ERROR_BASE - 999,

		// Deprecated
		ERROR_DRM_WV_VENDOR_MAX                 = ERROR_DRM_VENDOR_MAX,
		ERROR_DRM_WV_VENDOR_MIN                 = ERROR_DRM_VENDOR_MIN,

		// Heartbeat Error Codes
		HEARTBEAT_ERROR_BASE = -3000,
		ERROR_HEARTBEAT_TERMINATE_REQUESTED                     = HEARTBEAT_ERROR_BASE,
	};

};// namespace

#endif // ERRORS_H
