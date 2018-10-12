/****************************************************************************
Copyright (c) 2010 cocos2d-x.org
Copyright (c) 2017-2018 Xiamen Yaji Software Co., Ltd.

http://www.cocos2d-x.org

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
****************************************************************************/
#include "base/CCDirector.h"
#include "base/CCEventKeyboard.h"
#include "base/CCEventDispatcher.h"
#include "platform/android/CCGLViewImpl-android.h"

#include <android/log.h>
#include <jni.h>
#include <mutex>

using namespace cocos2d;
namespace {
    const int MAX_TOUCH_NUM = 5;
    class TouchEvents {
    public:
        TouchEvents()
        {
            count = 0;
        }
        void AddTouch(int _id, float _x, float _y)
        {
            if (count < MAX_TOUCH_NUM) {
                id[count] = _id;
                x[count] = _x;
                y[count] = _y;
                ++count;
            }
        }
        void Clear()
        {
            count = 0;
        }
        int count;
        int id[MAX_TOUCH_NUM];
        float x[MAX_TOUCH_NUM];
        float y[MAX_TOUCH_NUM];
    };
    static TouchEvents beginTouches;
    static TouchEvents moveTouches;
    static TouchEvents endTouches;
    static TouchEvents cancelTouches;
    static std::mutex mutex;
}

namespace cocos2d {
    void ExecuteTouchEvents() {
        if (0 < beginTouches.count) {
            mutex.lock();
            cocos2d::Director::getInstance()->getOpenGLView()->handleTouchesBegin(
                    beginTouches.count, beginTouches.id, beginTouches.x, beginTouches.y);
            beginTouches.Clear();
            mutex.unlock();
        }
        if (0 < moveTouches.count) {
            mutex.lock();
            cocos2d::Director::getInstance()->getOpenGLView()->handleTouchesMove(moveTouches.count,
                                                                                 moveTouches.id,
                                                                                 moveTouches.x,
                                                                                 moveTouches.y);
            moveTouches.Clear();
            mutex.unlock();
        }
        if (0 < cancelTouches.count) {
            mutex.lock();
            cocos2d::Director::getInstance()->getOpenGLView()->handleTouchesCancel(
                    cancelTouches.count, cancelTouches.id, cancelTouches.x, cancelTouches.y);
            cancelTouches.Clear();
            mutex.unlock();
        }
        if (0 < endTouches.count) {
            mutex.lock();
            cocos2d::Director::getInstance()->getOpenGLView()->handleTouchesEnd(endTouches.count,
                                                                                endTouches.id,
                                                                                endTouches.x,
                                                                                endTouches.y);
            endTouches.Clear();
            mutex.unlock();
        }
    }
}

extern "C" {
    JNIEXPORT void JNICALL Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeTouchesBegin(JNIEnv * env, jobject thiz, jint id, jfloat x, jfloat y) {
        mutex.lock();
        beginTouches.AddTouch(id, x, y);
        mutex.unlock();
    }

    JNIEXPORT void JNICALL Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeTouchesEnd(JNIEnv * env, jobject thiz, jint id, jfloat x, jfloat y) {
        mutex.lock();
        endTouches.AddTouch(id, x, y);
        mutex.unlock();
    }

    JNIEXPORT void JNICALL Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeTouchesMove(JNIEnv * env, jobject thiz, jintArray ids, jfloatArray xs, jfloatArray ys) {
        mutex.lock();
        int size = env->GetArrayLength(ids);
        jint* id = env->GetIntArrayElements(ids, NULL);
        jfloat* x = env->GetFloatArrayElements(xs, NULL);
        jfloat* y = env->GetFloatArrayElements(ys, NULL);
        for (int i = 0; i < size; ++i) {
            moveTouches.AddTouch(id[i], x[i], y[i]);
        }
        env->ReleaseFloatArrayElements(ys, y, JNI_ABORT);
        env->ReleaseFloatArrayElements(xs, x, JNI_ABORT);
        env->ReleaseIntArrayElements(ids, id, JNI_ABORT);
        mutex.unlock();
    }

    JNIEXPORT void JNICALL Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeTouchesCancel(JNIEnv * env, jobject thiz, jintArray ids, jfloatArray xs, jfloatArray ys) {
        mutex.lock();
        int size = env->GetArrayLength(ids);
        jint* id = env->GetIntArrayElements(ids, NULL);
        jfloat* x = env->GetFloatArrayElements(xs, NULL);
        jfloat* y = env->GetFloatArrayElements(ys, NULL);
        for (int i = 0; i < size; ++i) {
            cancelTouches.AddTouch(id[i], x[i], y[i]);
        }
        env->ReleaseFloatArrayElements(ys, y, JNI_ABORT);
        env->ReleaseFloatArrayElements(xs, x, JNI_ABORT);
        env->ReleaseIntArrayElements(ids, id, JNI_ABORT);
        mutex.unlock();
    }

#define KEYCODE_BACK 0x04
#define KEYCODE_MENU 0x52
#define KEYCODE_DPAD_UP 0x13
#define KEYCODE_DPAD_DOWN 0x14
#define KEYCODE_DPAD_LEFT 0x15
#define KEYCODE_DPAD_RIGHT 0x16
#define KEYCODE_ENTER 0x42
#define KEYCODE_PLAY  0x7e
#define KEYCODE_DPAD_CENTER  0x17

#define THROW_EXCEPTION(env) env->ThrowNew(env->FindClass("java/lang/Exception"), "Unexpected error occurred."); throw

    static std::unordered_map<int, cocos2d::EventKeyboard::KeyCode> g_keyCodeMap = {
        { KEYCODE_BACK , cocos2d::EventKeyboard::KeyCode::KEY_ESCAPE},
        { KEYCODE_MENU , cocos2d::EventKeyboard::KeyCode::KEY_MENU},
        { KEYCODE_DPAD_UP  , cocos2d::EventKeyboard::KeyCode::KEY_DPAD_UP },
        { KEYCODE_DPAD_DOWN , cocos2d::EventKeyboard::KeyCode::KEY_DPAD_DOWN },
        { KEYCODE_DPAD_LEFT , cocos2d::EventKeyboard::KeyCode::KEY_DPAD_LEFT },
        { KEYCODE_DPAD_RIGHT , cocos2d::EventKeyboard::KeyCode::KEY_DPAD_RIGHT },
        { KEYCODE_ENTER  , cocos2d::EventKeyboard::KeyCode::KEY_ENTER},
        { KEYCODE_PLAY  , cocos2d::EventKeyboard::KeyCode::KEY_PLAY},
        { KEYCODE_DPAD_CENTER  , cocos2d::EventKeyboard::KeyCode::KEY_DPAD_CENTER},
        
    };
    
    JNIEXPORT jboolean JNICALL Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeKeyEvent(JNIEnv * env, jobject thiz, jint keyCode, jboolean isPressed) {
        try {
            Director *pDirector = Director::getInstance();

            auto iterKeyCode = g_keyCodeMap.find(keyCode);
            if (iterKeyCode == g_keyCodeMap.end()) {
                return JNI_FALSE;
            }

            cocos2d::EventKeyboard::KeyCode cocos2dKey = g_keyCodeMap.at(keyCode);
            cocos2d::EventKeyboard event(cocos2dKey, isPressed);
            cocos2d::Director::getInstance()->getEventDispatcher()->dispatchEvent(&event);
            return JNI_TRUE;
        }
        catch (...) {
            THROW_EXCEPTION(env);
        }
        
    }}
