#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
: "${ANDROID_HOME:?Set ANDROID_HOME}"
mkdir -p build/java build/android-test
javac --release 8 -Xlint:all,-options -Werror -d build/java \
  java/org/oritwig/audio/RnnoiseDenoiser.java tests/RnnoiseDenoiserTest.java
"$ANDROID_HOME/build-tools/35.0.0/d8" \
  --lib "$ANDROID_HOME/platforms/android-35/android.jar" --min-api 26 \
  --output build/android-test/rnnoise-tests.jar \
  $(find build/java -name '*.class')
# This script deliberately does not boot or control a device. Run the generated
# JAR through app_process on an already authorized test device as documented.
