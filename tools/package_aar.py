#!/usr/bin/env python3
"""Package the compiled Java/JNI binding without a Telegram/Gradle dependency."""
import io
import pathlib
import zipfile
ROOT = pathlib.Path(__file__).resolve().parents[1]
DATE = (2026, 10, 1, 0, 0, 0)

def put(archive, name, contents):
    item = zipfile.ZipInfo(name, DATE)
    item.compress_type = zipfile.ZIP_DEFLATED
    item.external_attr = 0o644 << 16
    archive.writestr(item, contents)

classes = io.BytesIO()
with zipfile.ZipFile(classes, "w") as jar:
    for path in sorted((ROOT / "build/android-java").rglob("*.class")):
        put(jar, path.relative_to(ROOT / "build/android-java").as_posix(), path.read_bytes())
destination = ROOT / "dist/oritwig-rnnoise-proof-0.1.0.aar"
destination.parent.mkdir(exist_ok=True)
with zipfile.ZipFile(destination, "w") as aar:
    put(aar, "AndroidManifest.xml", (ROOT / "android/AndroidManifest.xml").read_bytes())
    put(aar, "classes.jar", classes.getvalue())
    put(aar, "proguard.txt", (ROOT / "android/consumer-rules.pro").read_bytes())
    put(aar, "R.txt", b"")
    for abi in ("arm64-v8a", "armeabi-v7a", "x86_64", "x86"):
        put(aar, f"jni/{abi}/liboritwig_rnnoise_jni.so",
            (ROOT / f"build/android-{abi}/liboritwig_rnnoise_jni.so").read_bytes())
    put(aar, "assets/oritwig-rnnoise/THIRD_PARTY_NOTICES.txt",
        (ROOT / "THIRD_PARTY_NOTICES.txt").read_bytes())
    put(aar, "assets/oritwig-rnnoise/ADAPTER_LICENSE.txt", (ROOT / "LICENSE").read_bytes())
    put(aar, "assets/oritwig-rnnoise/source-manifest.json",
        (ROOT / "provenance/source-manifest.json").read_bytes())
print(destination)
