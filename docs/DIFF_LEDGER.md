# RNNoise extraction and diff ledger

## Unchanged primary engine

Source repository: DrKLO/Telegram

Pinned commit: f2908b14133bbffbf7ab04f641ecb5bfaf533242

Pinned path: TMessagesProj/jni/voip/rnnoise

Pinned subtree: 91c3012f2d1c6b357b879be725ca3df50bca50a5

All 21 files of that subtree are under vendor/rnnoise, with exactly matching bytes, blob IDs, and complete tree ID. **Upstream source patches: zero. Removed files from the pinned subtree: zero. Modified default-model bytes: zero.** The upstream README, AUTHORS, COPYING, and every native source/header remain present.

This does not claim that the Telegram subtree is the whole contemporary Xiph repository. It is the complete snapshot Telegram vendored at the pinned commit. Xiph/Mozilla/RNNoise authorship is retained.

## Build-only adaptations

- New independent CMake targets compile the seven original C translation units and link standard math/pthread functionality
- The original Telegram CMake target repeats rnn_reader.c; the standalone source list includes it once
- Telegram's RNNoise internal DSP symbol-renaming definitions are reproduced at compile time; an additional common=rnnoise_common definition isolates its generic global-table symbol
- Private native symbols are hidden, and the JNI shared library exports only its four Java JNI entry points
- Telegram-wide WebRTC/VoIP/app flags and dependencies are omitted because this module does not use them
- The original unwrapped reference build deliberately omits these namespacing definitions and adapter code
- Android's supported flexible-page-size build option gives 64-bit JNI libraries 16 KB load-segment alignment

None of these edits alter a vendor file, neural weight, RNNoise operation, or algorithm.

## New adapter responsibilities

- src/denoise_adapter.c: checked outer allocation, state lifecycle/reset, PCM validation/conversion and frame batching
- The pinned core has an unsynchronized process-global lazy FFT/window initialization. A pthread_once call processes one silent frame on a disposable state before publishing real adapter states. This preserves every real stream's initial state and prevents adapter-created streams racing during first initialization
- Upstream initialization still contains internal allocations. The adapter's checks do not imply complete recovery from all memory-exhaustion cases
- src/denoise_jni.c and java/org/oritwig/audio/RnnoiseDenoiser.java: array marshalling, Java exceptions, synchronized lifecycle and AutoCloseable
- examples/denoise_pcm.c: tiny S16LE console reader/writer and engine invocation
- CMakeLists.txt, src/jni-exports.map, Android manifest/keep rule and tools: portable build/provenance/test/package plumbing
- tests: synthetic inputs, original-source equivalence, API misuse/lifecycle/chunking/concurrency/JNI checks

No new neural model, DSP denoiser, resampler, encoder, decoder, recorder UI, network layer, Telegram protocol, login or account code was added.

## Evidence-only Telegram source references

provenance/telegram-use.json records pinned caller, default-enabled header, native build and linking locations plus their Git blob hashes. The referenced tgcalls and Telegram application files are not compiled or distributed as module source.

## Product work intentionally still open

A speech-cleanup recorder/importer would add capture or codec integration, format negotiation/resampling using an established implementation, import/export and storage UX, interruption/lifecycle handling, upstream latency/flush policy, and real-speech listening tests. Those are not represented as completed here.
