#include "com_openamedia_recorder_ARecorder.h"
#include "LocalMediaRecorder.h"

#include <jni.h>
#include <stdint.h>
#include <android/native_window_jni.h>

#define  LOG_TAG "APlayer-jni"
#include "android/Log.h"

using namespace openamedia;


static jfieldID JCLASS_FIELD_ID_NATIVE_CONTEXT = NULL;
static jmethodID JCLASS_METHOD_ID_POST_EVENT_FROM_NATIVE = NULL;

static JavaVM* g_jvm = NULL;

/////////////////////////////////////
class JNIMediaRecorderListener : public MediaRecorderListener {
public:
	JNIMediaRecorderListener(JNIEnv *env, jobject thiz, jobject weak_this);
	virtual ~JNIMediaRecorderListener();

	virtual void notify(int msg, int ext1, int ext2, const void* data, int data_size);
	virtual void notifyLog(const char* log);
	
	virtual void registerCurThread();
	virtual void unRegisterCurThread();
	
private:
	JNIEnv* mEnv;
	JNIEnv* mOrgEnv;
	jclass mClass;
	jobject mObject;

	JNIMediaRecorderListener(const JNIMediaRecorderListener&);
	JNIMediaRecorderListener& operator=(const JNIMediaRecorderListener&);
};

JNIMediaRecorderListener::JNIMediaRecorderListener(JNIEnv *env, jobject thiz, jobject weak_this){
	jclass clazz = env->GetObjectClass(thiz);
	if(clazz == NULL){
		ALOGE("can not get jclass from jobject");
		return;
	}
	
	// Hold onto the APlayer class for use in calling the static method
    // that posts events to the application thread.
	mClass = (jclass)env->NewGlobalRef(clazz);

	// We use a weak reference so the APlayer object can be garbage collected.
    // The reference is only used as a proxy for callbacks.
	mObject = env->NewGlobalRef(weak_this);

	mOrgEnv = mEnv = env;
}

JNIMediaRecorderListener::~JNIMediaRecorderListener(){
	// remove global references
    mOrgEnv->DeleteGlobalRef(mObject);
    mOrgEnv->DeleteGlobalRef(mClass);
}

//call back to java
void JNIMediaRecorderListener::notify(int msg, int ext1, int ext2, const void* data, int data_size){
	jbyteArray array = NULL;
	if(data != NULL){
		array = mEnv->NewByteArray(data_size);
		if(!array){
			ALOGE("fail to new byteArray for notify!!");
			return;
		}else{
			jbyte* bytes = mEnv->GetByteArrayElements(array, NULL);
			if (bytes != NULL) {
				memcpy(bytes, data, data_size);
				mEnv->ReleaseByteArrayElements(array, bytes, 0);
			}
		}
	}
	
	mEnv->CallStaticVoidMethod(mClass, JCLASS_METHOD_ID_POST_EVENT_FROM_NATIVE, mObject, msg, ext1, ext2, array);

	//check while native call back to java.
	if (mEnv->ExceptionCheck()) {
        ALOGE("An exception occurred while notifying an event.");
        mEnv->ExceptionClear();
    }
}

void JNIMediaRecorderListener::notifyLog(const char* log){
	notify(NATIVE_MSG_NOTIFY_LOG_INFO, 0, 0, log, strlen(log) + 1);
}

void JNIMediaRecorderListener::registerCurThread(){
	g_jvm->AttachCurrentThread(&mEnv, NULL);
}

void JNIMediaRecorderListener::unRegisterCurThread(){
	g_jvm->DetachCurrentThread();
}

/////////////////////////////////////


JNIEXPORT void JNICALL Java_com_openamedia_recorder_ARecorder_nativeInit(JNIEnv *env, jclass clazz){
	JCLASS_FIELD_ID_NATIVE_CONTEXT = env->GetFieldID(clazz, "mNativeContext", "I");
	JCLASS_METHOD_ID_POST_EVENT_FROM_NATIVE = env->GetStaticMethodID(clazz, "postEventFromNative", "(Ljava/lang/Object;III[B)V");
}

//call back to java
static MediaRecorder* getMediaRecorder(JNIEnv* env, jobject thiz){
	MediaRecorder* mr = (MediaRecorder*)env->GetIntField(thiz, JCLASS_FIELD_ID_NATIVE_CONTEXT);
	return mr;
}

//call back to java
static void setMediaRecorder(JNIEnv* env, jobject thiz, const MediaRecorder* recorder){
	MediaRecorder* old = getMediaRecorder(env, thiz);
	if(old){
		delete old;
		old = NULL;
	}

	env->SetIntField(thiz, JCLASS_FIELD_ID_NATIVE_CONTEXT, (int)recorder);
}

JNIEXPORT void JNICALL Java_com_openamedia_recorder_ARecorder_nativeSetup(JNIEnv *env, jobject thiz, jobject weak_this){
	MediaRecorder* mp = new LocalMediaRecorder;
	if (mp == NULL) {
        ALOGE("create mediarecorder failed by Out of memory");
        return;
    }

	JNIMediaRecorderListener* listener = new JNIMediaRecorderListener(env, thiz, weak_this);
	mp->setListener(listener);

	setMediaRecorder(env, thiz, mp);
}

JNIEXPORT void JNICALL Java_com_openamedia_recorder_ARecorder_setParameter(JNIEnv *env, jobject thiz, jstring param){
	MediaRecorder* mr = getMediaRecorder(env, thiz);
	if(mr == NULL){
		ALOGE("fail to find MediaRecorder for set parameter!");
		return;
	}

	if(param == NULL){
		ALOGE("no param str found for setParameter");
		return;
	}

	const char *tmp = env->GetStringUTFChars(param, NULL);
    if (tmp == NULL) {
		ALOGE("fail to get utfchars from parameter jstr by Out of memory");
        return;
    }
	
	mr->setParameter(tmp);
	
	env->ReleaseStringUTFChars(param, tmp);
	tmp = NULL;
}

JNIEXPORT void JNICALL Java_com_openamedia_recorder_ARecorder_nativeSetOutputFile(JNIEnv *env, jobject thiz, jstring path){
	MediaRecorder* mr = getMediaRecorder(env, thiz);
	if(mr == NULL){
		ALOGE("no Recorder found for setSurface");
		return;
	}

	if(path == NULL){
		ALOGE("no param str found for setOutputFile");
		return;
	}

	const char *tmp = env->GetStringUTFChars(path, NULL);
    if (tmp == NULL) {
		ALOGE("fail to get utfchars from path jstr by Out of memory");
        return;
    }
	
	mr->setOutputFile(tmp);
	
	env->ReleaseStringUTFChars(path, tmp);
	tmp = NULL;
}

JNIEXPORT void JNICALL Java_com_openamedia_recorder_ARecorder_nativeSetPreview(JNIEnv *env, jobject thiz, jobject surface){
	MediaRecorder* mr = getMediaRecorder(env, thiz);
	if(mr == NULL){
		ALOGE("no Recorder found for setSurface");
		return;
	}

	ANativeWindow* window = ANativeWindow_fromSurface(env, surface);
	if(window == NULL){
		ALOGE("no window found for the surface");
		return;
	}

	mr->setPreview(window);
}

JNIEXPORT void JNICALL Java_com_openamedia_recorder_ARecorder_nativeSetChannels(JNIEnv *env, jobject thiz, jint channels){
	MediaRecorder* mr = getMediaRecorder(env, thiz);
	if(mr == NULL){
		ALOGE("no Recorder found for setChannels");
		return;
	}

	mr->setChannels(channels);
}

JNIEXPORT void JNICALL Java_com_openamedia_recorder_ARecorder_nativeSetSampleRate(JNIEnv *env, jobject thiz, jint sampleRate){
	MediaRecorder* mr = getMediaRecorder(env, thiz);
	if(mr == NULL){
		ALOGE("no Recorder found for setSampleRate");
		return;
	}

	mr->setSampleRate(sampleRate);
}

JNIEXPORT void JNICALL Java_com_openamedia_recorder_ARecorder_nativeSetVideoSize(JNIEnv *env, jobject thiz, jint width, jint height){
	MediaRecorder* mr = getMediaRecorder(env, thiz);
	if(mr == NULL){
		ALOGE("no Recorder found for setVideoSize");
		return;
	}

	mr->setVideoSize(width, height);	
}

JNIEXPORT void JNICALL Java_com_openamedia_recorder_ARecorder_nativeSetColorFormat(JNIEnv *env, jobject thiz, jstring fmt){
	MediaRecorder* mr = getMediaRecorder(env, thiz);
	if(mr == NULL){
		ALOGE("no Recorder found for setColorFormat");
		return;
	}

	const char *tmp = env->GetStringUTFChars(fmt, NULL);
    if (tmp == NULL) {
		ALOGE("fail to get utfchars from color fmt jstr by Out of memory");
        return;
    }

	if(!strcmp(tmp,"nv21")){
		mr->setColorFormat(OMX_COLOR_FormatYUV420SemiPlanar);
	}else{
		ALOGE("no support of colorfmt %s for now", tmp);
	}
	
	env->ReleaseStringUTFChars(fmt, tmp);
	tmp = NULL;
}

JNIEXPORT void JNICALL Java_com_openamedia_recorder_ARecorder_nativeStart(JNIEnv *env, jobject thiz){
	MediaRecorder* mr = getMediaRecorder(env, thiz);
	if(mr == NULL){
		ALOGE("no Recorder found for setSurface");
		return;
	}

	mr->start();
}

JNIEXPORT void JNICALL Java_com_openamedia_recorder_ARecorder_nativeStop(JNIEnv *env, jobject thiz){
	MediaRecorder* mr = getMediaRecorder(env, thiz);
	if(mr == NULL){
		ALOGE("no Recorder found for setSurface");
		return;
	}

	mr->stop();	
}

JNIEXPORT void JNICALL Java_com_openamedia_recorder_ARecorder_nativeRelease(JNIEnv *env, jobject thiz){	
	setMediaRecorder(env, thiz, NULL);
}

JNIEXPORT void JNICALL Java_com_openamedia_recorder_ARecorder_nativeWriteVideo(JNIEnv *env, jobject thiz, jbyteArray jb, jint len){
	MediaRecorder* mr = getMediaRecorder(env, thiz);
	if(mr == NULL){
		ALOGE("no Recorder found for setSurface");
		return;
	}

	jbyte* bytes = env->GetByteArrayElements(jb, NULL);
	unsigned char* pBuffer = (unsigned char*)bytes;
	if(pBuffer == NULL){
		ALOGE("fail to get buffer pointer from jbyteArray!!");
		return;
	}

	mr->writeVideo(pBuffer, len);
	
	env->ReleaseByteArrayElements(jb, bytes, 0);
}


jint JNI_OnLoad(JavaVM* vm, void* reserved){
	g_jvm = vm;

	return JNI_VERSION_1_4;
}
