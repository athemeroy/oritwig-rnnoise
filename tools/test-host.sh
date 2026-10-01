#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
python3 tools/verify_source.py
python3 tools/verify_build_inputs.py
cmake -S . -B build/host -G Ninja -DCMAKE_BUILD_TYPE=Release -DORITWIG_BUILD_JNI=ON
cmake --build build/host --parallel 4
ctest --test-dir build/host --output-on-failure -V
mkdir -p build/java
javac --release 8 -Xlint:all,-options -Werror -d build/java \
  java/org/oritwig/audio/RnnoiseDenoiser.java tests/RnnoiseDenoiserTest.java
java -Xcheck:jni -Djava.library.path=build/host -cp build/java RnnoiseDenoiserTest
python3 tools/test_cli.py build/host/oritwig-denoise-pcm
