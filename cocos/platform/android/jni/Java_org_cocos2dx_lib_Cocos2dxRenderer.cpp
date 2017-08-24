#include "base/CCIMEDispatcher.h"
#include "base/CCDirector.h"
#include "base/CCEventType.h"
#include "base/CCEventCustom.h"
#include "base/CCEventDispatcher.h"
#include "../CCGLViewImpl-android.h"
#include "platform/CCApplication.h"
#include "platform/CCFileUtils.h"
#include "platform/android/jni/JniHelper.h"
#include <jni.h>

#include "base/ccUTF8.h"

using namespace cocos2d;
namespace cocos2d {
	void ExecuteTouchEvents();
}

#define THROW_EXCEPTION(env) env->ThrowNew(env->FindClass("java/lang/Exception"), "Unexpected error occurred."); throw

extern "C" {

    JNIEXPORT void JNICALL Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeRender(JNIEnv* env, jobject thiz, jobject view) {
        try {
            GLViewImpl* pGLView = static_cast<GLViewImpl*>(cocos2d::Director::getInstance()->getOpenGLView());
            pGLView->setGLSurfaceView(env, view);
            cocos2d::ExecuteTouchEvents();
            cocos2d::Director::getInstance()->mainLoop();
            pGLView->setGLSurfaceView(NULL, NULL);
        }
        catch (...) {
            THROW_EXCEPTION(env);
        }
    }

    JNIEXPORT void JNICALL Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeOnPause(JNIEnv* env, jobject thiz) {
        try {
            if (Director::getInstance()->getOpenGLView()) {
                Application::getInstance()->applicationDidEnterBackground();
                cocos2d::EventCustom backgroundEvent(EVENT_COME_TO_BACKGROUND);
                cocos2d::Director::getInstance()->getEventDispatcher()->dispatchEvent(&backgroundEvent);
            }
        }
        catch (...) {
            THROW_EXCEPTION(env);
        }
    }

    JNIEXPORT void JNICALL Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeOnResume(JNIEnv* env, jobject thiz) {
        static bool firstTime = true;
        try {
            if (Director::getInstance()->getOpenGLView()) {
				// don't invoke at first to keep the same logic as iOS
				// can refer to https://github.com/cocos2d/cocos2d-x/issues/14206
				if (!firstTime)
					Application::getInstance()->applicationWillEnterForeground();

				cocos2d::EventCustom foregroundEvent(EVENT_COME_TO_FOREGROUND);
                cocos2d::Director::getInstance()->getEventDispatcher()->dispatchEvent(&foregroundEvent);
            }
			firstTime = false;
		}
        catch (...) {
            THROW_EXCEPTION(env);
        }
    }

    JNIEXPORT void JNICALL Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeInsertText(JNIEnv* env, jobject thiz, jstring text) {
        try {
            std::string strValue = cocos2d::StringUtils::getStringUTFCharsJNI(env, text);
            const char *pszText = strValue.c_str();
            cocos2d::IMEDispatcher::sharedDispatcher()->dispatchInsertText(pszText, strlen(pszText));
        }
        catch (...) {
            THROW_EXCEPTION(env);
        }
    }

    JNIEXPORT void JNICALL Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeDeleteBackward(JNIEnv* env, jobject thiz) {
        try {
            cocos2d::IMEDispatcher::sharedDispatcher()->dispatchDeleteBackward();
        }
        catch (...) {
            THROW_EXCEPTION(env);
        }
    }

    JNIEXPORT jstring JNICALL Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeGetContentText(JNIEnv* env, jobject thiz) {
        try {
            std::string pszText = cocos2d::IMEDispatcher::sharedDispatcher()->getContentText();
            return cocos2d::StringUtils::newStringUTFJNI(env, pszText);
        }
        catch (...) {
            THROW_EXCEPTION(env);
        }
    }
}
