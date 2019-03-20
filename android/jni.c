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

#define LOG_TAG "cJSON-Jni"
#include <jni.h>
#include <JNIHelp.h>
#include <utils/Log.h>

#include "cJSON.h"

#define CJSON_CLASS_NAME  ("com/jayhou/android/cjson/cJSON")
#define CJSONOBJECT_CLASS_NAME  ("com/jayhou/android/cjson/CJSONObject")
#define CJSONARRAY_CLASS_NAME  ("com/jayhou/android/cjson/CJSONArray")

#define LOG_NDDEBUG 1

#if LOG_NDDEBUG
#define ALOGD(...)   ((void)0)
#endif

static jobject nativeCreateCJSONObject() {
    return NULL;
}


static cJSON* getObjectNativePtr(JNIEnv* env, jobject obj) {
    jclass objclazz = (*env)->GetObjectClass(env, obj);
    jfieldID nativePtrID = (*env)->GetFieldID(env, objclazz, "mNativeObjectPtr", "J");
    jlong ptr = (*env)->GetLongField(env, obj, nativePtrID);
    ALOGD("getNativePtr cJSONOBJ:%08x",(cJSON*)ptr);
    return (cJSON*)ptr;
}

static void initCJSONObjectWithString(JNIEnv* env, jobject obj, jstring jsonstr) {
    ALOGD("initCJSONObjectWithString env:%08x");
    jboolean value = 0;
    const char* svalue = (*env)->GetStringUTFChars(env,jsonstr,&value);
    cJSON *nativePtr = cJSON_Parse(svalue);
    (*env)->ReleaseStringUTFChars(env, jsonstr, svalue);
    ALOGD("addr to cjsonobj:%08x",nativePtr);
    jclass objclazz = (*env)->GetObjectClass(env, obj);
    jfieldID nativePtrID = (*env)->GetFieldID(env, objclazz, "mNativeObjectPtr", "J");
    (*env)->SetLongField(env, obj, nativePtrID, (jlong)nativePtr);
}

static void initCJSONObject(JNIEnv* env, jobject obj) {
    ALOGD("initCJSONObject env:%08x");
    cJSON *nativePtr = cJSON_CreateObject();
    ALOGD("addr to cjsonobj:%08x",nativePtr);
    jclass objclazz = (*env)->GetObjectClass(env, obj);
    jfieldID nativePtrID = (*env)->GetFieldID(env, objclazz, "mNativeObjectPtr", "J");
    (*env)->SetLongField(env, obj, nativePtrID, (jlong)nativePtr);
}

static void destroyCJSONObject(JNIEnv* env, jobject obj) {
    ALOGD("destroyCJSONObject env:%08x  jobject:%08x",env,obj);
    cJSON* ptr = getObjectNativePtr(env, obj);
    ALOGI("addr to delete :%08x",ptr);
    cJSON_Delete(ptr);
}

static void initCJSONArray(JNIEnv* env, jobject obj) {
    ALOGD("initCJSONArray");
    cJSON *nativePtr = cJSON_CreateArray();
    ALOGD("addr of cjsonarray:%08x",nativePtr);
    jclass objclazz = (*env)->GetObjectClass(env, obj);
    jfieldID nativePtrID = (*env)->GetFieldID(env, objclazz, "mNativeObjectPtr", "J");
    (*env)->SetLongField(env, obj, nativePtrID, (jlong)nativePtr);
}

static void destroyCJSONArray(JNIEnv* env, jclass clazz, jobject obj) {
    ALOGI("destroyCJSONArray not implement yet");
}

static void CJSONArray_addStringItemToArray(JNIEnv* env, jobject obj, jstring stringItem) {
    ALOGD("CJSONArray_addStringItemToArray");
    cJSON* array = getObjectNativePtr(env,obj);
    jboolean value = 0;
    const char* svalue = (*env)->GetStringUTFChars(env,stringItem,&value);
    cJSON_AddItemToArray(array,cJSON_CreateString(svalue));
    (*env)->ReleaseStringUTFChars(env, stringItem, svalue);
}

static void CJSONArray_addItemToArray(JNIEnv* env, jobject obj, jobject arrayItem) {
    ALOGD("CJSONArray_addItemToArray");
    cJSON* array = getObjectNativePtr(env,obj);
    cJSON* item = getObjectNativePtr(env,arrayItem);
    ALOGD(">>>> array :0x08, item:0x08",array, item);
    cJSON_AddItemToArray(array, item);
}

static void CJSONObject_addStringToObject(JNIEnv* env, jobject obj, jstring keyObj, jstring stringValue) {
    jboolean value = 0;
    const char* key = (*env)->GetStringUTFChars(env,keyObj,&value);
    const char* svalue = (*env)->GetStringUTFChars(env,stringValue,&value);
    if(key == NULL) {
        ALOGE("null string not allowed!");//or out of memory
        return;
    }
    cJSON* cjsonroot = getObjectNativePtr(env,obj);
    ALOGD("CJSONObject_addStringToObject object addr:%08x, key:%s  string:%s",cjsonroot,key, svalue);
    cJSON_AddStringToObject(cjsonroot,key,svalue);
    (*env)->ReleaseStringUTFChars(env, keyObj, key);
    (*env)->ReleaseStringUTFChars(env, stringValue, svalue);
}

static void CJSONObject_addCJSONArrayToObject(JNIEnv* env, jobject obj, jstring keyObj, jobject cjsonArray) {
    jboolean value = 0;
    const char* key = (*env)->GetStringUTFChars(env,keyObj,&value);
    if(key == NULL) {
       ALOGE("null string not allowed!");  //or out of memory
       return;
    }
    cJSON* cjsonroot = getObjectNativePtr(env,obj);
    cJSON* cjsonArrayItem = getObjectNativePtr(env,cjsonArray);
    cJSON_AddItemToObject(cjsonroot, key, cjsonArrayItem);
    ALOGD("CJSONObject_addCJSONArraytToObject root addr:%08x, key:%s  item addr:%08x",cjsonroot,key, cjsonArrayItem);

    (*env)->ReleaseStringUTFChars(env, keyObj, key);
}

static void CJSONObject_addCJSONObjectToObject(JNIEnv* env, jobject obj, jstring keyObj, jobject cjsonObj) {
    jboolean value = 0;
    const char* key = (*env)->GetStringUTFChars(env,keyObj,&value);
    if(key == NULL) {
        ALOGE("null string not allowed!");//or out of memory
        return;
    }
    cJSON* cjsonroot = getObjectNativePtr(env,obj);
    cJSON* cjsonobj = getObjectNativePtr(env,cjsonObj);
    ALOGD("CJSONObject_addCJSONObjectToObject root addr:%08x, key:%s  item addr:%08x",cjsonroot,key, cjsonobj);
    cJSON_AddItemToObject(cjsonroot,key,cjsonobj);
    (*env)->ReleaseStringUTFChars(env, keyObj, key);
}

static jstring CJSONObject_formatJsonObject2String(JNIEnv* env, jobject obj) {
    ALOGD("CJSONObject_formatJsonObject2String");
    cJSON* root = getObjectNativePtr(env,obj);
    ALOGD("CJSONObject_formatJsonObject2String object:%08x",root);
    /* declarations */
    char *out = NULL;
    /* formatted print */
    out = cJSON_PrintUnformatted(root);
    ALOGD("cjson string out: %s", out);
    jstring result = (*env)->NewStringUTF(env,out);
    free(out);
    return result;
}

static JNINativeMethod  nativeCJSONArray[] = {
        {"init","()V",(void*)initCJSONArray},
        {"destroy","()V",(void*)destroyCJSONArray},
        {"addItemToArray","(Lcom/jayhou/android/cjson/CJSONObject;)V",(void*)CJSONArray_addItemToArray},
        {"addStringItemToArray","(Ljava/lang/String;)V",(void*)CJSONArray_addStringItemToArray},
};

static JNINativeMethod  nativeCJSONObjectMethods[] = {
        {"init","(Ljava/lang/String;)V",(void*)initCJSONObjectWithString},
        {"init","()V",(void*)initCJSONObject},
        {"destroy","()V",(void*)destroyCJSONObject},
        {"addCJSONObjectToObject","(Ljava/lang/String;Lcom/jayhou/android/cjson/CJSONObject;)V",(void*)CJSONObject_addCJSONObjectToObject},
        {"addCJSONArraytToObject","(Ljava/lang/String;Lcom/jayhou/android/cjson/CJSONArray;)V",(void*)CJSONObject_addCJSONArrayToObject},
        {"addStringToObject","(Ljava/lang/String;Ljava/lang/String;)V",(void*)CJSONObject_addStringToObject},
        {"formatJsonObject2String","()Ljava/lang/String;",(void*)CJSONObject_formatJsonObject2String},
};

/*
 * This is called by the VM when the shared library is first loaded.
 */
jint JNI_OnLoad(JavaVM* vm, void* reserved) {
        JNIEnv* env = NULL;
        jint result = -1;

        if ((*vm)->GetEnv(vm,(void**) &env, JNI_VERSION_1_4) != JNI_OK) {
                ALOGE("ERROR: GetEnv failed\n");
                goto bail;
        }
/*
        ALOGI("register class:%s",CJSON_CLASS_NAME);
        if( jniRegisterNativeMethods(env,CJSON_CLASS_NAME, nativeCJSONMethods, NELEM(nativeCJSONMethods)) != 0) {
                ALOGE("ERROR: Could not register %s native methods",CJSON_CLASS_NAME);
                goto bail;
        }
*/

        ALOGI("register class:%s",CJSONARRAY_CLASS_NAME);
        if( jniRegisterNativeMethods(env,CJSONARRAY_CLASS_NAME, nativeCJSONArray, NELEM(nativeCJSONArray)) != 0) {
                ALOGE("ERROR: Could not register %s native methods",CJSONARRAY_CLASS_NAME);
                goto bail;
        }
        ALOGI("register class:%s",CJSONOBJECT_CLASS_NAME);
        if( jniRegisterNativeMethods(env,CJSONOBJECT_CLASS_NAME, nativeCJSONObjectMethods, NELEM(nativeCJSONObjectMethods)) != 0) {
                ALOGE("ERROR: Could not register %s native methods",CJSONOBJECT_CLASS_NAME);
                goto bail;
        }
        result = JNI_VERSION_1_4;
bail:
        return result;
}
