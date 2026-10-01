#include "fixtures.h"
#include <stdio.h>
#include <stdlib.h>
#ifdef TEST_ADAPTER
#include "oritwig_denoise.h"
#else
#include "rnnoise.h"
#endif

int main(int argc, char **argv) {
  if (argc != 2) return 2;
  FILE *output = fopen(argv[1], "wb");
  if (!output) return 3;
#ifdef TEST_ADAPTER
  OritwigDenoise *state = oritwig_denoise_create();
#else
  DenoiseState *state = rnnoise_create(NULL);
#endif
  if (!state) { fclose(output); return 4; }
  for (unsigned frame = 0; frame < 600; ++frame) {
    float input[480], result[480], vad;
    fixture_frame(input, frame);
#ifdef TEST_ADAPTER
    if (oritwig_denoise_frame(state, input, result, 480, &vad)) return 5;
#else
    vad = rnnoise_process_frame(state, result, input);
#endif
    if (fwrite(&vad, sizeof(vad), 1, output) != 1 ||
        fwrite(result, sizeof(float), 480, output) != 480) return 6;
  }
#ifdef TEST_ADAPTER
  oritwig_denoise_destroy(state);
#else
  rnnoise_destroy(state);
#endif
  return fclose(output) == 0 ? 0 : 7;
}
