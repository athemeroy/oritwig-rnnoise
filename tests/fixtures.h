#ifndef ORITWIG_TEST_FIXTURES_H
#define ORITWIG_TEST_FIXTURES_H
#include <math.h>
#include <stdint.h>
/* Deterministic artificial signal for engineering tests only. Not real speech
 * and not evidence of intelligibility or perceptual noise-removal quality. */
static void fixture_frame(float *out, unsigned frame) {
  uint32_t random = 0x1badf00du ^ (frame * 2654435761u);
  for (unsigned i = 0; i < 480; ++i) {
    random = random * 1664525u + 1013904223u;
    double t = (frame * 480.0 + i) / 48000.0;
    double noise = ((double)(random >> 8) / 16777215.0 - 0.5) * 2800.0;
    double envelope = 0.55 + 0.45 * sin(2.0 * 3.141592653589793 * 3.1 * t);
    double voiced = envelope * (6000.0 * sin(2.0 * 3.141592653589793 * 137.0 * t)
        + 2400.0 * sin(2.0 * 3.141592653589793 * 274.0 * t)
        + 1200.0 * sin(2.0 * 3.141592653589793 * 411.0 * t));
    unsigned kind = (frame / 100) % 6;
    if (kind == 0) out[i] = 0;
    else if (kind == 1) out[i] = (float)noise;
    else if (kind == 5) out[i] = (i % 2 == 0) ? 32767.f : -32768.f;
    else out[i] = (float)(voiced + noise);
  }
}
#endif
