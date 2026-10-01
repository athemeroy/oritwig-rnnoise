#include "oritwig_denoise.h"
#include <stdio.h>
#include <string.h>

/* Deliberately tiny console consumer, not the intended Android app. S16LE,
 * 48 kHz, mono only. Files must contain complete 10 ms frames. No WAV parser. */
int main(int argc, char **argv) {
  if (argc != 3 || strcmp(argv[1], argv[2]) == 0) {
    fprintf(stderr, "Usage: %s input.s16le output.s16le\n"
                    "48 kHz mono signed 16-bit little-endian; complete 480-sample frames.\n",
            argv[0]);
    return 2;
  }
  FILE *in = fopen(argv[1], "rb");
  if (!in) { perror("input"); return 1; }
  /* Exclusive create prevents overwriting an existing input/output or symlink. */
  FILE *out = fopen(argv[2], "wbx");
  if (!out) { perror("output"); fclose(in); return 1; }
  OritwigDenoise *state = oritwig_denoise_create();
  if (!state) { fprintf(stderr, "RNNoise initialization failed\n"); fclose(in); fclose(out); return 1; }
  unsigned char bytes[2 * ORITWIG_FRAME_SAMPLES];
  int16_t samples[ORITWIG_FRAME_SAMPLES];
  int result = 0;
  size_t frames = 0;
  for (;;) {
    size_t count = fread(bytes, 1, sizeof(bytes), in);
    if (!count) { if (ferror(in)) { perror("read"); result = 1; } break; }
    if (count != sizeof(bytes)) {
      fprintf(stderr, "Incomplete final frame rejected; output contains preceding full frames only\n");
      result = 1; break;
    }
    for (size_t i = 0; i < ORITWIG_FRAME_SAMPLES; ++i) {
      unsigned value = (unsigned)bytes[2*i] | ((unsigned)bytes[2*i+1] << 8);
      samples[i] = (int16_t)(value <= 32767 ? (int)value : (int)value - 65536);
    }
    if (oritwig_denoise_pcm16(state, samples, samples, ORITWIG_FRAME_SAMPLES, NULL)) {
      result = 1; break;
    }
    for (size_t i = 0; i < ORITWIG_FRAME_SAMPLES; ++i) {
      uint16_t value = (uint16_t)samples[i];
      bytes[2*i] = (unsigned char)(value & 255);
      bytes[2*i+1] = (unsigned char)(value >> 8);
    }
    if (fwrite(bytes, 1, sizeof(bytes), out) != sizeof(bytes)) {
      perror("write"); result = 1; break;
    }
    ++frames;
  }
  if (fclose(out)) { perror("close output"); result = 1; }
  fclose(in);
  oritwig_denoise_destroy(state);
  fprintf(stderr, "Processed %zu frames (480 samples each)\n", frames);
  return result;
}
