#include "oritwig_denoise.h"
#include "rnnoise.h"
#include <math.h>
#include <pthread.h>
#include <stdlib.h>

struct OritwigDenoise { DenoiseState *core; };
static pthread_once_t common_once = PTHREAD_ONCE_INIT;
static int common_ready = 0;

/* This pinned upstream core lazily initializes shared FFT/window tables without
 * a lock. Warm them once with a disposable silent state before exposing states.
 * No RNNoise source or algorithm is changed; the real stream starts fresh. */
static void initialize_common(void) {
  DenoiseState *core = (DenoiseState *)calloc(1, (size_t)rnnoise_get_size());
  if (!core) return;
  if (rnnoise_init(core, NULL) == 0) {
    float input[ORITWIG_FRAME_SAMPLES] = {0};
    float output[ORITWIG_FRAME_SAMPLES];
    rnnoise_process_frame(core, output, input);
    common_ready = 1;
  }
  rnnoise_destroy(core);
}

OritwigDenoise *oritwig_denoise_create(void) {
  if (rnnoise_get_frame_size() != ORITWIG_FRAME_SAMPLES) return NULL;
  if (pthread_once(&common_once, initialize_common) || !common_ready) return NULL;
  OritwigDenoise *state = (OritwigDenoise *)calloc(1, sizeof(*state));
  if (!state) return NULL;
  /* The original rnnoise_create does not check its own allocation before init.
   * Use the documented preallocated-state API so this adapter can check it. */
  state->core = (DenoiseState *)calloc(1, (size_t)rnnoise_get_size());
  if (!state->core || rnnoise_init(state->core, NULL) != 0) {
    rnnoise_destroy(state->core);
    free(state);
    return NULL;
  }
  return state;
}

void oritwig_denoise_destroy(OritwigDenoise *state) {
  if (!state) return;
  rnnoise_destroy(state->core);
  free(state);
}

int oritwig_denoise_reset(OritwigDenoise *state) {
  if (!state) return ORITWIG_INVALID_ARGUMENT;
  return rnnoise_init(state->core, NULL) == 0 ? ORITWIG_OK : ORITWIG_INVALID_ARGUMENT;
}

int oritwig_denoise_frame(OritwigDenoise *state, const float *input,
                          float *output, size_t count, float *vad) {
  if (!state || !input || !output || count != ORITWIG_FRAME_SAMPLES)
    return ORITWIG_INVALID_ARGUMENT;
  for (size_t i = 0; i < count; ++i)
    if (!isfinite(input[i]) || input[i] < -32768.f || input[i] > 32767.f)
      return ORITWIG_INVALID_SAMPLE;
  const float probability = rnnoise_process_frame(state->core, output, input);
  if (vad) *vad = probability;
  return ORITWIG_OK;
}

int oritwig_denoise_pcm16(OritwigDenoise *state, const int16_t *input,
                          int16_t *output, size_t count, float *vad_per_frame) {
  if (!state || !input || !output || !count || count % ORITWIG_FRAME_SAMPLES)
    return ORITWIG_INVALID_ARGUMENT;
  for (size_t offset = 0; offset < count; offset += ORITWIG_FRAME_SAMPLES) {
    float source[ORITWIG_FRAME_SAMPLES], result[ORITWIG_FRAME_SAMPLES];
    for (size_t j = 0; j < ORITWIG_FRAME_SAMPLES; ++j)
      source[j] = (float)input[offset + j];
    const float vad = rnnoise_process_frame(state->core, result, source);
    if (vad_per_frame) vad_per_frame[offset / ORITWIG_FRAME_SAMPLES] = vad;
    for (size_t j = 0; j < ORITWIG_FRAME_SAMPLES; ++j) {
      float value = result[j];
      if (value > 32767.f) value = 32767.f;
      if (value < -32768.f) value = -32768.f;
      output[offset + j] = (int16_t)roundf(value);
    }
  }
  return ORITWIG_OK;
}
