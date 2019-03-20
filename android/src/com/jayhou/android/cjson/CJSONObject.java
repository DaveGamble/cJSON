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

public class CJSONObject {
    private static final String TAG = "CJSONObject";
    private boolean mHasReleased = false;
    private boolean mIsRoot = false;
    public long mNativeObjectPtr;
    private ConcurrentLinkedQueue<CJSONObject> mChild = new ConcurrentLinkedQueue<>();
    private ConcurrentLinkedQueue<CJSONArray> mChildArray = new ConcurrentLinkedQueue<>();

    CJSONObject(String json) {
        this(false, json);
    }

    CJSONObject() {
        this(false);
    }

    CJSONObject(boolean isRoot, String json) {
        mIsRoot = isRoot;
        init(json);
        Log.d(TAG, "native CJSONObject with json string:" + Long.toHexString(mNativeObjectPtr));
    }

    CJSONObject(boolean isRoot) {
        mIsRoot = isRoot;
        init();
        Log.d(TAG, "native CJSONObject:" + Long.toHexString(mNativeObjectPtr));
    }

    private void checkReleased() {
        if(mHasReleased) {
            throw new IllegalStateException("Object has been released!");
        }
    }

    public void release() {
        checkReleased();
        if(cJSON.DEBUG) {
            Log.d(TAG, "Object " + Long.toHexString(mNativeObjectPtr) + " released");
        }
        for(CJSONObject child:mChild) {
            child.release();
        }
        for(CJSONArray childArray:mChildArray) {
            childArray.release();
        }
        mHasReleased = true;
        if(mIsRoot) {
            if(cJSON.DEBUG) {
                Log.d(TAG, "Destroy root object.");
            }
            destroy();
        }
    }

    public String toString() {
        checkReleased();
        return formatJsonObject2String();
    }

    public void put(String key, String value) {
        checkReleased();
        addStringToObject(key,value);
    }

    public void put(String key, boolean value) {
        checkReleased();
        addStringToObject(key, String.valueOf(value));
    }

    public void put(String key, long value) {
        checkReleased();
        addStringToObject(key, String.valueOf(value));
    }

    public void put(String key, int value) {
        checkReleased();
        addStringToObject(key, String.valueOf(value));
    }

    public void putCJSONObject(String key, CJSONObject object) {
        checkReleased();
        mChild.add(object);
        addCJSONObjectToObject(key, object);
    }

    public void putCJSONArray(String key, CJSONArray array) {
        checkReleased();
        mChildArray.add(array);
        addCJSONArraytToObject(key, array);
    }

    public CJSONObject addCJSONObjectWithJsonString(String key, String jsonstr) {
        checkReleased();
        CJSONObject object = new CJSONObject(jsonstr);
        putCJSONObject(key,object);
        return object;
    }


    public CJSONObject addCJSONObject(String key) {
        checkReleased();
        CJSONObject object = new CJSONObject();
        putCJSONObject(key,object);
        return object;
    }

    public CJSONArray addCJSONArray(String key) {
        checkReleased();
        CJSONArray array = new CJSONArray();
        putCJSONArray(key, array);
        return array;
    }

    private native final void init(String json);
    private native final void init();
    private native final void destroy();
    private native final void addCJSONObjectToObject(String key, CJSONObject item);
    private native final void addCJSONArraytToObject(String key, CJSONArray item);
    private native final void addStringToObject(String key, String values);
    private native final String formatJsonObject2String();
}