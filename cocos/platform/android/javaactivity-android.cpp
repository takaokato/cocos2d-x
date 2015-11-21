/****************************************************************************
Copyright (c) 2013-2014 Chukong Technologies Inc.

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

#include "platform/CCPlatformConfig.h"
#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID

#include "CCApplication-android.h"
#include "CCGLViewImpl-android.h"
#include "base/CCDirector.h"
#include "base/CCEventCustom.h"
#include "base/CCEventType.h"
#include "base/CCEventDispatcher.h"
#include "renderer/CCGLProgramCache.h"
#include "renderer/CCTextureCache.h"
#include "renderer/ccGLStateCache.h"
#include "2d/CCDrawingPrimitives.h"
#include "platform/android/jni/JniHelper.h"
#include "network/CCDownloader-android.h"
#include <android/log.h>
#include <jni.h>
#include <signal.h>
#include <dlfcn.h>
#include <unwind.h>
#include <ucontext.h>
#include <cxxabi.h>

#define  LOG_TAG    "main"
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)

void cocos_android_app_init(JNIEnv* env) __attribute__((weak));

using namespace cocos2d;

static void ExitWithStackTrace(int sig_num, siginfo_t * info, void * ucontext)
{
	ucontext_t *uc = (ucontext_t*)ucontext;
#if defined(__arm__)
	// reference:
	// http://stackoverflow.com/questions/5397041/getting-the-saved-instruction-pointer-address-from-a-signal-handler
	// http://stackoverflow.com/questions/15752188/arm-link-register-and-frame-pointer
	void* pc = (void**)uc->uc_mcontext.arm_pc;
	void* sp = (void**)uc->uc_mcontext.arm_sp;
	void** fp = (void**)uc->uc_mcontext.arm_fp;
	void* lr = (void*)uc->uc_mcontext.arm_lr;
	Dl_info dlInfo;
	if (dladdr(pc, &dlInfo)) {
		int status;
		const char* symbol = dlInfo.dli_sname;
		char* demangledName = abi::__cxa_demangle(symbol, NULL, NULL, &status);
		if (status == 0) {
			symbol = demangledName;
		}
		__android_log_print(ANDROID_LOG_ERROR, "SIGNAL", "%s(%p) at %s", strsignal(sig_num), info->si_addr, symbol);
		if (demangledName != NULL) {
			free(demangledName);
		}
	}
	else {
		__android_log_print(ANDROID_LOG_ERROR, "SIGNAL", "%s(%p) at PC(%p)", strsignal(sig_num), info->si_addr, pc);
	}
	__android_log_print(ANDROID_LOG_ERROR, "SIGNAL", "sp:%p, fp:%p, lr:%p, pc:%p", sp, fp, lr, pc);

	__android_log_print(ANDROID_LOG_ERROR, "Stack Trace", "Start");
	for ( ; ; ) {
		if (dladdr(lr, &dlInfo)) {
			int status;
			const char* symbol = dlInfo.dli_sname;
			char* demangledName = abi::__cxa_demangle(symbol, NULL, NULL, &status);
			if (status == 0) {
				symbol = demangledName;
			}
			__android_log_print(ANDROID_LOG_ERROR, "Stack Trace", "%p:<%s>+%p", lr, symbol, dlInfo.dli_saddr);
			if (demangledName != NULL) {
				free(demangledName);
			}
		}
		else {
			__android_log_print(ANDROID_LOG_ERROR, "Stack Trace", "%p", lr);
		}
		if (fp == NULL || (void*)fp < sp) {
			sp = fp;
			lr = fp[-1];
			fp = (void**)fp[-3];
		}
	}
	__android_log_print(ANDROID_LOG_ERROR, "Stack Trace", "End");
#endif
	exit(-1);
}

extern "C"
{

JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved)
{
	struct sigaction sigact;
	sigact.sa_sigaction = ExitWithStackTrace;
	sigact.sa_flags = SA_SIGINFO;
	sigaction(SIGABRT, &sigact, (struct sigaction *)NULL);
	sigaction(SIGSEGV, &sigact, (struct sigaction *)NULL);

	JniHelper::setJavaVM(vm);

    cocos_android_app_init(JniHelper::getEnv());

    return JNI_VERSION_1_4;
}

JNIEXPORT void Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeInit(JNIEnv*  env, jobject thiz, jint w, jint h)
{
    auto director = cocos2d::Director::getInstance();
    auto glview = director->getOpenGLView();
    if (!glview)
    {
        glview = cocos2d::GLViewImpl::create("Android app");
        glview->setFrameSize(w, h);
        director->setOpenGLView(glview);

        cocos2d::Application::getInstance()->run();
    }
    else
    {
        cocos2d::GL::invalidateStateCache();
        cocos2d::GLProgramCache::getInstance()->reloadDefaultGLPrograms();
        cocos2d::DrawPrimitives::init();
        cocos2d::VolatileTextureMgr::reloadAllTextures();

        cocos2d::EventCustom recreatedEvent(EVENT_RENDERER_RECREATED);
        director->getEventDispatcher()->dispatchEvent(&recreatedEvent);
        director->setGLDefaultValues();
    }
    cocos2d::network::_preloadJavaDownloaderClass();
}

JNIEXPORT jintArray Java_org_cocos2dx_lib_Cocos2dxActivity_getGLContextAttrs(JNIEnv*  env, jobject thiz)
{
    cocos2d::Application::getInstance()->initGLContextAttrs(); 
    GLContextAttrs _glContextAttrs = GLView::getGLContextAttrs();
    
    int tmp[6] = {_glContextAttrs.redBits, _glContextAttrs.greenBits, _glContextAttrs.blueBits,
                           _glContextAttrs.alphaBits, _glContextAttrs.depthBits, _glContextAttrs.stencilBits};


    jintArray glContextAttrsJava = env->NewIntArray(6);
        env->SetIntArrayRegion(glContextAttrsJava, 0, 6, tmp); 
    
    return glContextAttrsJava;
}

JNIEXPORT void Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeOnSurfaceChanged(JNIEnv*  env, jobject thiz, jint w, jint h)
{
    cocos2d::Application::getInstance()->applicationScreenSizeChanged(w, h);
}

}

#endif // CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID

