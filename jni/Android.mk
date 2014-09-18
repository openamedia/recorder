LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE    := static-avutil
LOCAL_SRC_FILES := ffmpeg/lib/libavutil.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := static-avformat
LOCAL_SRC_FILES := ffmpeg/lib/libavformat.a
include $(PREBUILT_STATIC_LIBRARY) 

include $(CLEAR_VARS)
LOCAL_MODULE    := static-avcodec
LOCAL_SRC_FILES := ffmpeg/lib/libavcodec.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := static-faac
LOCAL_SRC_FILES := ffmpeg/lib/libfaac.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := static-swresample
LOCAL_SRC_FILES := ffmpeg/lib/libswresample.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := static-swscale
LOCAL_SRC_FILES := ffmpeg/lib/libswscale.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := static-x264
LOCAL_SRC_FILES := ffmpeg/lib/libx264.a
include $(PREBUILT_STATIC_LIBRARY)


include $(CLEAR_VARS)

LOCAL_MODULE    := arecorder-jni

LOCAL_SRC_FILES :=	com_openamedia_recorder_ARecorder.cpp \
					android/AString.cpp \
					android/MetaData.cpp \
					android/TimedEventQueue.cpp \
					android/ColorConverter.cpp \
					android/MediaBuffer.cpp \
					android/MediaBufferGroup.cpp \
					io/ANativeWindowRenderer.cpp \
					io/OpenSLRecorder.cpp \
					Common.cpp \
					MessageQueue.cpp \
					LMRSource.cpp \
					Prefetcher.cpp \
					FFMPEGer.cpp \
					FFmpegAudioEncoder.cpp \
					MuxEngine.cpp \
					LocalMediaRecorder.cpp

LOCAL_STATIC_LIBRARIES :=	static-avformat \
							static-avcodec \
							static-x264 \
							static-faac \
							static-swresample \
							static-swscale \
							static-avutil


LOCAL_C_INCLUDES :=	ffmpeg/include \
					openmax \

LOCAL_LDLIBS    += 	-llog \
					-landroid \
					-lOpenSLES \
					-lz \

LOCAL_CFLAGS    += 	-Wno-multichar \
					-D__cplusplus \
					-D__STDC_CONSTANT_MACROS \
					-D__STDC_FORMAT_MACROS \
					-DFF_API_SWS_GETCONTEXT


include $(BUILD_SHARED_LIBRARY)