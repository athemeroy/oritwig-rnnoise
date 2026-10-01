# Verification

- Exact source verifier passes for 21 files, 536,397 bytes; Git subtree 91c3012f2d1c6b357b879be725ca3df50bca50a5
- Host native contract and separate unwrapped-original reference comparison pass
- Actual Java/JNI PCM/VAD, reset/close, chunking, validation and concurrent-stream tests pass
- Actual CLI streaming, incomplete-frame rejection and overwrite protection pass
- All four Android ABIs compile; packaged libraries and 16 KB alignment checks pass
- API26 x86 ART exercises actual JNI processing and lifecycle successfully
- The Oritwig Voice consumer passed 44 on-device checks and actual system-picker import/export; it preserves the original and exports the tested 9.685-second synthetic spoken fixture with exact sample count

Run `tools/test-host.sh`, `tools/build-android.sh`, `tools/verify_android_artifacts.py` and `tools/build-android-test.sh` to reproduce the applicable checks. See each script’s prerequisite variables. Test-only artifacts and machine-specific logs are generated locally and are not checked in.

Synthetic/reference evidence does not establish intelligibility, music restoration, physical microphone quality, real-time performance or exhaustive memory-exhaustion/security behavior. ASan/UBSan passed in the extraction environment with LeakSanitizer disabled because it was unavailable there. Untouched upstream `rnn.c` has two warnings on invalid activation-tag failure paths; the exposed embedded-model path passes the tests.
