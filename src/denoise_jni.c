#include <jni.h>
#include <stdint.h>
#include "oritwig_denoise.h"

static void fail(JNIEnv *env, const char *type, const char *message) {
  jclass exception = (*env)->FindClass(env, type);
  if (exception) (*env)->ThrowNew(env, exception, message);
}

JNIEXPORT jlong JNICALL
Java_org_oritwig_audio_RnnoiseDenoiser_nativeCreate(JNIEnv *env, jclass type) {
  (void)type;
  OritwigDenoise *state = oritwig_denoise_create();
  if (!state) fail(env, "java/lang/OutOfMemoryError", "RNNoise state initialization failed");
  return (jlong)(intptr_t)state;
}

JNIEXPORT void JNICALL
Java_org_oritwig_audio_RnnoiseDenoiser_nativeDestroy(JNIEnv *env, jclass type, jlong handle) {
  (void)env; (void)type;
  oritwig_denoise_destroy((OritwigDenoise *)(intptr_t)handle);
}

JNIEXPORT void JNICALL
Java_org_oritwig_audio_RnnoiseDenoiser_nativeReset(JNIEnv *env, jclass type, jlong handle) {
  (void)type;
  if (oritwig_denoise_reset((OritwigDenoise *)(intptr_t)handle) != ORITWIG_OK)
    fail(env, "java/lang/IllegalStateException", "Denoiser is closed");
}

JNIEXPORT jfloat JNICALL
Java_org_oritwig_audio_RnnoiseDenoiser_nativeProcessFrame(
    JNIEnv *env, jclass type, jlong handle, jshortArray samples, jint offset) {
  (void)type;
  if (!handle) {
    fail(env, "java/lang/IllegalStateException", "Denoiser is closed");
    return 0;
  }
  if (!samples) {
    fail(env, "java/lang/NullPointerException", "samples");
    return 0;
  }
  jsize length = (*env)->GetArrayLength(env, samples);
  if (offset < 0 || length < ORITWIG_FRAME_SAMPLES ||
      offset > length - ORITWIG_FRAME_SAMPLES) {
    fail(env, "java/lang/IndexOutOfBoundsException", "A complete 480-sample frame is required");
    return 0;
  }
  jshort buffer[ORITWIG_FRAME_SAMPLES];
  (*env)->GetShortArrayRegion(env, samples, offset, ORITWIG_FRAME_SAMPLES, buffer);
  if ((*env)->ExceptionCheck(env)) return 0;
  float vad = 0;
  int result = oritwig_denoise_pcm16((OritwigDenoise *)(intptr_t)handle,
      (const int16_t *)buffer, (int16_t *)buffer, ORITWIG_FRAME_SAMPLES, &vad);
  if (result != ORITWIG_OK) {
    fail(env, "java/lang/IllegalArgumentException", "Invalid RNNoise frame");
    return 0;
  }
  (*env)->SetShortArrayRegion(env, samples, offset, ORITWIG_FRAME_SAMPLES, buffer);
  return vad;
}
