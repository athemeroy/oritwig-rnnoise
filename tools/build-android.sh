#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
: "${JAVA_HOME:?Set JAVA_HOME to an installed JDK}"
: "${ANDROID_NDK_HOME:?Set ANDROID_NDK_HOME to an installed Android NDK (tested r27c)}"
python3 tools/verify_source.py
python3 tools/verify_build_inputs.py
for abi in arm64-v8a armeabi-v7a x86_64 x86; do
  cmake -S . -B "build/android-$abi" -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake" \
    -DANDROID_ABI="$abi" -DANDROID_PLATFORM=android-21 \
    -DANDROID_SUPPORT_FLEXIBLE_PAGE_SIZES=ON \
    -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF -DORITWIG_BUILD_JNI=ON
  cmake --build "build/android-$abi" --parallel 4
done
mkdir -p build/android-java
javac --release 8 -Xlint:all,-options -Werror -d build/android-java java/org/oritwig/audio/RnnoiseDenoiser.java
python3 tools/package_aar.py
