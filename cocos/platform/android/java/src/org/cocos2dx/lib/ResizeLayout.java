/****************************************************************************
 Copyright (c) 2010-2013 cocos2d-x.org
 
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
package org.cocos2dx.lib;

import android.content.Context;
import android.os.Handler;
import android.util.AttributeSet;
import android.widget.FrameLayout;

public class ResizeLayout extends FrameLayout {
    private Handler mForceLayoutHandler = null;
    private Runnable mForceLayoutFunc = null;

    public ResizeLayout(Context context){
        super(context);
    }

    public ResizeLayout(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public void setEnableForceDoLayout(boolean flag){
        if (flag) {
            if (mForceLayoutHandler == null) {
                mForceLayoutHandler = new Handler();
            }
            if (mForceLayoutFunc == null) {
                mForceLayoutFunc = new Runnable() {
                    @Override
                    public void run() {
                        requestLayout();
                        invalidate();
                    }
                };
            }
        }
        else {
            mForceLayoutHandler = null;
        }
    }

    @Override
    protected void onLayout(boolean changed, int l, int t, int r, int b) {
        super.onLayout(changed, l, t, r, b);
        if(mForceLayoutHandler != null){
            /*This is a hot-fix for some android devices which don't do layout when the main window
            * is paned. We refresh the layout in 24 frames per seconds.
            * When the editBox is lose focus or when user begin to type, the do layout is disabled.
            */
            mForceLayoutHandler.postDelayed(mForceLayoutFunc, 1000 / 24);
        }
    }

}
