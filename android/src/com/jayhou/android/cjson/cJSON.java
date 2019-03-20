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

public class cJSON {
    private static final String TAG = "cJSON";
    public static boolean DEBUG = true;
    private boolean mHasReleased = false;
    static {
        System.loadLibrary("libcjson-jni.so");
    }

    private void checkReleased() {
        if(mHasReleased) {
            throw new IllegalStateException("Object has been released!");
        }
    }

    private CJSONObject mRootObject;

    public cJSON() {
        initRootCJSONObject();
    }

    public cJSON(String json) {
        initRootCJSONObject(json);
    }

    public void put(String key, String value) {
        checkReleased();
        mRootObject.put(key,value);
    }

    public void put(String key, boolean value) {
        checkReleased();
        mRootObject.put(key,value);
    }

    public void put(String key, long value) {
        checkReleased();
        mRootObject.put(key,value);
    }

    public void put(String key, int value) {
        checkReleased();
        mRootObject.put(key,value);
    }

    public CJSONObject addCJSONObject(String key) {
        checkReleased();
        CJSONObject object = new CJSONObject();
        mRootObject.putCJSONObject(key,object);
        return object;
    }

    public CJSONArray addCJSONArray(String key) {
        checkReleased();
        CJSONArray array = new CJSONArray();
        mRootObject.putCJSONArray(key, array);
        return array;
    }

    private void initRootCJSONObject(String json) {
        mRootObject = new CJSONObject(true, json);
        Log.d(TAG, "initRootCJSONObject addr" + Long.toHexString(mRootObject.mNativeObjectPtr));
    }

    private void initRootCJSONObject() {
        mRootObject = new CJSONObject(true);
        Log.d(TAG, "initRootCJSONObject addr" + Long.toHexString(mRootObject.mNativeObjectPtr));
    }

    private void releaseRootObject() {
        checkReleased();
        Log.d(TAG, "releaseRootObject");
        mHasReleased = true;
        mRootObject.release();
    }

    @Override
    public String toString() {
        checkReleased();
        return mRootObject.toString();
    }

    @Override
    protected void finalize() throws Throwable {
        try {
            releaseRootObject();
        } finally {
            super.finalize();
        }
    }
}