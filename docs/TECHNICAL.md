# Oritwig RNNoise independent-engine proof

An offline speech-denoise module using **the complete RNNoise subtree vendored by Telegram**, with its original native processing core and embedded default model unchanged. This is a bounded engine extraction proof, not a finished Android recorder/importer.

The real processing is RNNoise: signal analysis, FFT and pitch processing, recurrent-network inference, model weights, and synthesis. New code is limited to build isolation, lifecycle/PCM/JNI adapters, a tiny raw-PCM console consumer, and verification. There is no newly invented denoising model, DSP, or resampler.

## Difference from the upstream library

The pinned RNNoise README already documents a reusable noise-suppression library and a raw-PCM example CLI. This extraction preserves its entire 21-file Telegram-vendored snapshot and default model unchanged, and adds isolated CMake targets, checked PCM/state/JNI adapters, Android AAR packaging, provenance checks and tests. It does not replace upstream DSP, add a denoising model, or claim the original project lacked a command-line demo. The useful difference here is independent, verifiable Android consumption without the Telegram client or accounts. The parent Oritwig Voice app adds microphone capture, named local WAVs, A/B playback and SAF export; its narrow V1 accepts only 48 kHz mono PCM16 WAV, up to 5 minutes. RNNoise is Xiph/Mozilla/contributor work actually used by Telegram, not Telegram-authored DSP. See the exact source URLs, pin and license notices below. Original vendor README/source files are intentionally untouched.

## Origin and identity

- Source: [DrKLO/Telegram at f2908b14133bbffbf7ab04f641ecb5bfaf533242](https://github.com/DrKLO/Telegram/tree/f2908b14133bbffbf7ab04f641ecb5bfaf533242/TMessagesProj/jni/voip/rnnoise)
- Complete vendored subtree: **91c3012f2d1c6b357b879be725ca3df50bca50a5**, 21 files, 536,397 bytes
- All source files and the default model are byte-for-byte original; the verifier rebuilds the exact Git tree and checks every blob hash and SHA-256
- RNNoise is authored by Xiph/Mozilla and the contributors named in its source notices. It is an actual dependency used by Telegram, **not Telegram-authored RNNoise** and not a Telegram client fork
- Telegram's [capture post-processor](https://github.com/DrKLO/Telegram/blob/f2908b14133bbffbf7ab04f641ecb5bfaf533242/TMessagesProj/jni/voip/tgcalls/group/GroupAudioCapturePostProcessor.cpp#L48-L99) creates a default RNNoise state and processes audio frames. Its [header defaults USE_RNNOISE to 1](https://github.com/DrKLO/Telegram/blob/f2908b14133bbffbf7ab04f641ecb5bfaf533242/TMessagesProj/jni/voip/tgcalls/group/GroupAudioCapturePostProcessor.h#L13-L15), and the [native build links RNNoise](https://github.com/DrKLO/Telegram/blob/f2908b14133bbffbf7ab04f641ecb5bfaf533242/TMessagesProj/jni/CMakeLists.txt#L251-L255)
- That caller, tgcalls, WebRTC, tgnet, account/session code, and Telegram UI are **not included or linked**

See [source-manifest.json](../provenance/source-manifest.json), [telegram-use.json](../provenance/telegram-use.json), and [the diff ledger](DIFF_LEDGER.md).

## Reuse boundary

Three usable layers:

1. **RNNoise::Core**: original RNNoise C API, with the complete embedded default model
2. **Oritwig::Denoise**: a C lifecycle/frame/PCM16 adapter with one state per audio stream
3. **RnnoiseDenoiser**: a small AutoCloseable Java/JNI binding, packaged in a four-ABI Android AAR

There is no runtime network request, Telegram account, login, external model file, API token, or paid service. Android native dependencies are only libc, libm, and libdl. The library manifest requests no permissions. A recording app would separately need the microphone permission and a capture UI.

The intended standalone consumer is an offline voice cleanup recorder/importer: capture or decode audio, feed continuous PCM into this module, then preview/export the result. That product shell, recording/import/export UX, established decode/resampling integration, and listening validation remain future work. The console example demonstrates a real independent consumer today but is not that finished app.

## Strict input contract

- **48,000 Hz, mono, 480 samples (10 ms) per frame**
- C float samples use signed-16-bit PCM magnitude, **not normalized [-1, 1]**; inputs must be finite and between -32768 and 32767
- C/Java PCM16 calls use signed 16-bit samples; output conversion rounds half away from zero and saturates to the signed-16-bit range
- The C batch function and Java process method require a positive multiple of 480 samples; they retain the same RNNoise state across every frame
- Keep one state alive for each continuous stream. Reset between unrelated recordings, not between frames
- Same-state C calls must be serialized. Separate adapter states can run concurrently. Java methods serialize processing, reset, and close
- No arbitrary rate conversion, stereo mixdown, WAV/container parsing, tail padding, latency compensation, or end-of-stream flushing is silently performed
- The original RNNoise analysis/synthesis state has streaming delay. A production importer must define and test its padding, flush and trim policy; the proof CLI preserves frame count without compensating for that delay
- For other input formats, use Android's supported capture/codec path or an established upstream resampler. Do not replace RNNoise or invent a home-grown resampling filter
- Only the embedded model is exposed by the adapter. Upstream custom-model parsing remains preserved in the core source but is not part of this adapter's tested input surface

## Build and test on a host

Requires a C99 compiler, CMake, Ninja, Python 3, and a JDK. Linux is tested.

```sh
export JAVA_HOME=/path/to/jdk
export PATH="$JAVA_HOME/bin:/path/to/cmake/bin:$PATH"
bash tools/test-host.sh
```

This verifies provenance, builds the original core and adapters, runs native tests and an independent original-source reference comparison, then executes the real JNI binding with the JVM's JNI checks and tests the console consumer.

The independent reference target compiles the same pinned original source files **without the adapter and without the integration symbol-renaming definitions**, and runs in a separate process. The comparison is byte-exact for 600 synthetic frames and their VAD values on the same compiler/architecture. It does not assert bit identity across architectures, across compiler versions, against Telegram's full app build, or against a newer upstream model.

Optional host ASan/UBSan:

```sh
cmake -S . -B build/sanitize -G Ninja -DCMAKE_BUILD_TYPE=Debug \
  -DORITWIG_SANITIZE=ON -DORITWIG_BUILD_JNI=OFF
cmake --build build/sanitize --parallel 4
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 \
  ctest --test-dir build/sanitize --output-on-failure
```

LeakSanitizer cannot run in the supplied ptrace-backed sandbox; that check is explicitly **not passed**. Address/undefined-behavior checks pass with leak detection disabled.

## Tiny console consumer

```sh
build/host/oritwig-denoise-pcm input.s16le output.s16le
```

The input is headerless signed 16-bit little-endian mono PCM at 48 kHz. The output has the same encoding. Existing output paths are refused. An incomplete final frame is rejected with a nonzero status; the partial output contains only preceding complete frames. This is a proof tool, not a general audio-file converter.

## Android library

Requires an already installed NDK, CMake/Ninja, and JDK. Tested with NDK **27.2.12479018 (r27c)** and CMake **3.22.1**.

```sh
export JAVA_HOME=/path/to/jdk
export ANDROID_NDK_HOME=/path/to/android-sdk/ndk/27.2.12479018
export PATH="$JAVA_HOME/bin:/path/to/cmake/bin:$PATH"
bash tools/build-android.sh
python3 tools/verify_android_artifacts.py
```

Output: `dist/oritwig-rnnoise-proof-0.1.0.aar`, with arm64-v8a, armeabi-v7a, x86_64, and x86 JNI libraries; API 21 native target; Java 8-compatible bytecode. The 64-bit libraries use 16 KB load-segment alignment. Final APK packaging/alignment is the consumer app's responsibility.

The AAR contains license notices, a source hash manifest, and a consumer keep rule for JNI names. To consume it in a normal Android app, use a local AAR dependency and import `org.oritwig.audio.RnnoiseDenoiser`:

```java
try (RnnoiseDenoiser denoiser = new RnnoiseDenoiser()) {
    // pcm is actual 48 kHz mono PCM16, exactly 480 samples here.
    float voiceActivity = denoiser.processFrame(pcm);
    // pcm now contains RNNoise output. Keep this instance for later frames.
}
```

For a native consumer, add this project as a CMake subdirectory and link `Oritwig::Denoise`; include `oritwig_denoise.h`. No Android dependency is needed for that path.

## Evidence and limits

See the verification document and run the included tests. Machine-specific logs are generated locally. Synthetic tests establish engine execution, finite output, state/chunking consistency, wrapper equivalence, and lifecycle behavior. They **do not establish real-world audio quality, intelligibility, music quality, or hardware real-time performance**. No listening test has been claimed.

Two compiler warnings in untouched upstream `rnn.c` refer to deliberate null writes for invalid model activation tags. The default embedded model follows the supported paths and passes these tests. No source patch was hidden to remove those warnings. This proof is not a comprehensive security or memory-exhaustion audit.

## Licensing

Retain all upstream notices under `vendor/rnnoise` and distribute [THIRD_PARTY_NOTICES.txt](../THIRD_PARTY_NOTICES.txt) with binaries. Per-file copyright/license headers are preserved and included in the combined notices. New integration/test code uses the root BSD-3-Clause [LICENSE](../LICENSE). No Telegram, Xiph, Mozilla, or contributor endorsement is implied.
