package com.huami.watch.cjson;

import android.util.Log;

public class cJSON {
    private static final String TAG = "cJSON";
    public static boolean DEBUG = true;
    private boolean mHasReleased = false;
    static {
        System.load("/system/lib/libcjson-jni.so");
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

    private void initRootCJSONObject() {
        Log.d(TAG, "initRootCJSONObject");
        mRootObject = new CJSONObject(true);
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