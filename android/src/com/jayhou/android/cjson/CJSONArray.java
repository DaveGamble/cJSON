/*
  Copyright (c) 2009-2017 Dave Gamble and cJSON contributors

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
*/
package com.jayhou.android.cjson;

import android.util.Log;

import java.util.concurrent.ConcurrentLinkedQueue;

public class CJSONArray {
    private static final String TAG = "CJSONArray";

    public long mNativeObjectPtr;
    private boolean mHasReleased = false;
    private ConcurrentLinkedQueue<CJSONObject> mChild = new ConcurrentLinkedQueue<>();

    CJSONArray() {
        init();
        Log.d(TAG, "native CJSONArray:" + Long.toHexString(mNativeObjectPtr));
    }

    public CJSONObject addArrayItem() {
        checkReleased();
        CJSONObject item = new CJSONObject();
        addItemToArray(item);
        return item;
    }

    public void putString(String value) {
        checkReleased();
        addStringItemToArray(value);
    }

    private void checkReleased() {
        if(mHasReleased) {
            throw new IllegalStateException("Object has been released!");
        }
    }

    public void release() {
        checkReleased();
        if(cJSON.DEBUG) {
            Log.d(TAG, "Array Object " + Long.toHexString(mNativeObjectPtr) + " released");
        }
        mHasReleased = true;
    }

    private native final void init();
    private native final void destroy();
    private native final void addItemToArray(CJSONObject item);
    private native final void addStringItemToArray(String value);
}