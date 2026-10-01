#ifndef ORITWIG_DENOISE_H
#define ORITWIG_DENOISE_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

enum { ORITWIG_SAMPLE_RATE = 48000, ORITWIG_FRAME_SAMPLES = 480 };
enum {
  ORITWIG_OK = 0,
  ORITWIG_INVALID_ARGUMENT = -1,
  ORITWIG_INVALID_SAMPLE = -2
};
typedef struct OritwigDenoise OritwigDenoise;

/* One state per continuous mono stream. Default embedded RNNoise model only.
 * Different states may be used concurrently. Serialize calls on the same state,
 * including reset and destroy. No I/O, accounts, networking, or model download. */
OritwigDenoise *oritwig_denoise_create(void);
void oritwig_denoise_destroy(OritwigDenoise *state); /* NULL is allowed. */
int oritwig_denoise_reset(OritwigDenoise *state);

/* Float samples use signed-16-bit PCM amplitude, NOT normalized [-1, 1].
 * Exactly 480 samples, all finite and in [-32768, 32767]. Output is not clipped.
 * in == out is supported; other partial overlap is not supported.
 * Optional vad receives the original model's voice-activity probability.
 * Invalid input is rejected before the state is advanced. */
int oritwig_denoise_frame(OritwigDenoise *state, const float *input,
                          float *output, size_t count, float *vad);

/* PCM16 adapter. count must be a positive multiple of 480. Input and output may
 * be identical, but must not partially overlap. Float output is rounded to the
 * nearest integer (half away from zero) and saturated to int16_t. If non-NULL,
 * vad_per_frame must have room for count/480 floats. All frames share state.
 * Caller buffers are host-endian; the example CLI explicitly decodes S16LE. */
int oritwig_denoise_pcm16(OritwigDenoise *state, const int16_t *input,
                          int16_t *output, size_t count, float *vad_per_frame);

#ifdef __cplusplus
}
#endif
#endif
