package com.huami.watch.cjson;

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