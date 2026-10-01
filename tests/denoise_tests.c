#include "oritwig_denoise.h"
#include "rnnoise.h"
#include "fixtures.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); exit(1); } } while (0)

static void *parallel_stream(void *unused) {
  (void)unused;
  OritwigDenoise *state = oritwig_denoise_create();
  CHECK(state);
  float input[480], output[480], vad;
  for (unsigned f = 0; f < 40; ++f) {
    fixture_frame(input, f + 200);
    CHECK(oritwig_denoise_frame(state, input, output, 480, &vad) == ORITWIG_OK);
    CHECK(isfinite(vad) && vad >= 0 && vad <= 1);
    for (unsigned i = 0; i < 480; ++i) CHECK(isfinite(output[i]));
  }
  oritwig_denoise_destroy(state);
  return NULL;
}
static void test_parallel_initialization(void) {
  pthread_t workers[8];
  for (unsigned i = 0; i < 8; ++i) CHECK(pthread_create(&workers[i], NULL, parallel_stream, NULL) == 0);
  for (unsigned i = 0; i < 8; ++i) CHECK(pthread_join(workers[i], NULL) == 0);
}
static void test_silence_and_equivalence(void) {
  OritwigDenoise *state = oritwig_denoise_create();
  DenoiseState *reference = rnnoise_create(NULL);
  CHECK(state && reference);
  float input[480], output[480], expected[480], actual_vad;
  int changed = 0;
  for (unsigned frame = 0; frame < 600; ++frame) {
    fixture_frame(input, frame);
    CHECK(oritwig_denoise_frame(state, input, output, 480, &actual_vad) == ORITWIG_OK);
    float expected_vad = rnnoise_process_frame(reference, expected, input);
    CHECK(memcmp(output, expected, sizeof(output)) == 0);
    CHECK(memcmp(&actual_vad, &expected_vad, sizeof(float)) == 0);
    CHECK(isfinite(actual_vad) && actual_vad >= 0 && actual_vad <= 1);
    for (unsigned i = 0; i < 480; ++i) {
      CHECK(isfinite(output[i]));
      if (frame < 100) CHECK(output[i] == 0.f);
      else if (output[i] != input[i]) changed = 1;
    }
  }
  CHECK(changed); /* Actually executes the engine, rather than a pass-through. */
  oritwig_denoise_destroy(state);
  rnnoise_destroy(reference);
}
static void test_pcm_chunking_and_rounding(void) {
  enum { FRAMES = 37, COUNT = 480 * FRAMES };
  int16_t input[COUNT], batch[COUNT], chunked[COUNT], in_place[COUNT];
  float vad_batch[FRAMES], vad_chunked[FRAMES], floating[480], expected[480];
  OritwigDenoise *a = oritwig_denoise_create(), *b = oritwig_denoise_create();
  OritwigDenoise *c = oritwig_denoise_create();
  DenoiseState *reference = rnnoise_create(NULL);
  CHECK(a && b && c && reference);
  for (unsigned f = 0; f < FRAMES; ++f) {
    fixture_frame(floating, f + 200);
    for (unsigned i = 0; i < 480; ++i) input[f * 480 + i] = (int16_t)roundf(floating[i]);
  }
  memcpy(in_place, input, sizeof(input));
  CHECK(oritwig_denoise_pcm16(a, input, batch, COUNT, vad_batch) == ORITWIG_OK);
  for (unsigned f = 0; f < FRAMES; ++f)
    CHECK(oritwig_denoise_pcm16(b, input + f * 480, chunked + f * 480, 480, vad_chunked + f) == ORITWIG_OK);
  CHECK(oritwig_denoise_pcm16(c, in_place, in_place, COUNT, NULL) == ORITWIG_OK);
  CHECK(memcmp(batch, chunked, sizeof(batch)) == 0);
  CHECK(memcmp(batch, in_place, sizeof(batch)) == 0);
  CHECK(memcmp(vad_batch, vad_chunked, sizeof(vad_batch)) == 0);
  for (unsigned f = 0; f < FRAMES; ++f) {
    for (unsigned i = 0; i < 480; ++i) floating[i] = input[f * 480 + i];
    float vad = rnnoise_process_frame(reference, expected, floating);
    CHECK(vad == vad_batch[f]);
    for (unsigned i = 0; i < 480; ++i) {
      float sample = expected[i];
      if (sample > 32767) sample = 32767;
      if (sample < -32768) sample = -32768;
      CHECK(batch[f * 480 + i] == (int16_t)roundf(sample));
    }
  }
  oritwig_denoise_destroy(a); oritwig_denoise_destroy(b); oritwig_denoise_destroy(c);
  rnnoise_destroy(reference);
}
static void test_validation_reset_lifecycle(void) {
  oritwig_denoise_destroy(NULL);
  CHECK(oritwig_denoise_reset(NULL) == ORITWIG_INVALID_ARGUMENT);
  for (unsigned repetition = 0; repetition < 40; ++repetition) {
    OritwigDenoise *a = oritwig_denoise_create(), *b = oritwig_denoise_create();
    CHECK(a && b);
    float input[480], output[480], expected[480], vad;
    fixture_frame(input, 200);
    memset(output, 0x55, sizeof(output));
    CHECK(oritwig_denoise_frame(a, input, output, 479, &vad) == ORITWIG_INVALID_ARGUMENT);
    CHECK(oritwig_denoise_frame(a, NULL, output, 480, NULL) == ORITWIG_INVALID_ARGUMENT);
    CHECK(oritwig_denoise_frame(a, input, NULL, 480, NULL) == ORITWIG_INVALID_ARGUMENT);
    CHECK(oritwig_denoise_frame(NULL, input, output, 480, NULL) == ORITWIG_INVALID_ARGUMENT);
    const float bad[] = {NAN, INFINITY, -INFINITY, 32768.f, -32769.f};
    float saved = input[200];
    for (unsigned i = 0; i < sizeof(bad)/sizeof(bad[0]); ++i) {
      input[200] = bad[i];
      CHECK(oritwig_denoise_frame(a, input, output, 480, NULL) == ORITWIG_INVALID_SAMPLE);
    }
    input[200] = saved;
    CHECK(oritwig_denoise_frame(a, input, output, 480, NULL) == ORITWIG_OK);
    CHECK(oritwig_denoise_frame(b, input, expected, 480, NULL) == ORITWIG_OK);
    CHECK(memcmp(output, expected, sizeof(output)) == 0); /* Invalid calls did not advance state. */
    CHECK(oritwig_denoise_reset(a) == ORITWIG_OK);
    CHECK(oritwig_denoise_frame(a, input, input, 480, NULL) == ORITWIG_OK);
    CHECK(memcmp(input, expected, sizeof(input)) == 0); /* Reset and float in-place. */
    int16_t pcm[480] = {0};
    CHECK(oritwig_denoise_pcm16(a, pcm, pcm, 0, NULL) == ORITWIG_INVALID_ARGUMENT);
    CHECK(oritwig_denoise_pcm16(a, pcm, pcm, 479, NULL) == ORITWIG_INVALID_ARGUMENT);
    CHECK(oritwig_denoise_pcm16(NULL, pcm, pcm, 480, NULL) == ORITWIG_INVALID_ARGUMENT);
    oritwig_denoise_destroy(a); oritwig_denoise_destroy(b);
  }
}
int main(void) {
  CHECK(rnnoise_get_frame_size() == 480);
  test_parallel_initialization();
  test_silence_and_equivalence();
  test_pcm_chunking_and_rounding();
  test_validation_reset_lifecycle();
  puts("PASS: concurrency, silence, finite output, exact raw equivalence, PCM conversion, chunking, invalid input, reset, in-place, lifecycle");
  return 0;
}
