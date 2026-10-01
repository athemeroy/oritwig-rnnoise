#!/usr/bin/env python3
"""Verify the local AAR's ABI contents, exports, dependencies and page alignment."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import zipfile

ROOT = Path(__file__).resolve().parents[1]
BIN = Path(os.environ["ANDROID_NDK_HOME"]) / "toolchains/llvm/prebuilt/linux-x86_64/bin"
aar_path = ROOT / "dist/oritwig-rnnoise-proof-0.1.0.aar"
results = {}
with zipfile.ZipFile(aar_path) as aar:
    manifest = aar.read("AndroidManifest.xml").decode()
    assert "uses-permission" not in manifest
    assert "android:minSdkVersion=\"21\"" in manifest
    assert aar.read("assets/oritwig-rnnoise/source-manifest.json") == (ROOT / "provenance/source-manifest.json").read_bytes()
    for abi in ("arm64-v8a", "armeabi-v7a", "x86_64", "x86"):
        binary = ROOT / f"build/android-{abi}/liboritwig_rnnoise_jni.so"
        assert aar.read(f"jni/{abi}/liboritwig_rnnoise_jni.so") == binary.read_bytes()
        dynamic = subprocess.check_output([str(BIN / "llvm-readelf"), "-d", str(binary)], text=True)
        headers = subprocess.check_output([str(BIN / "llvm-readelf"), "-l", str(binary)], text=True)
        exports = subprocess.check_output([str(BIN / "llvm-nm"), "-D", "--defined-only", str(binary)], text=True)
        needed = [line.split("[")[1].split("]")[0] for line in dynamic.splitlines() if "(NEEDED)" in line]
        assert set(needed) <= {"libc.so", "libm.so", "libdl.so"}, (abi, needed)
        symbols = [line.split()[-1].split("@")[0] for line in exports.splitlines() if line.strip()]
        assert set(symbols) - {"ORITWIG_JNI_1.0"} == {
            "Java_org_oritwig_audio_RnnoiseDenoiser_nativeCreate",
            "Java_org_oritwig_audio_RnnoiseDenoiser_nativeDestroy",
            "Java_org_oritwig_audio_RnnoiseDenoiser_nativeReset",
            "Java_org_oritwig_audio_RnnoiseDenoiser_nativeProcessFrame"}, (abi, symbols)
        imports = subprocess.check_output([str(BIN / "llvm-nm"), "-D", "--undefined-only", str(binary)], text=True)
        imported_symbols = {line.split()[-1].split("@")[0] for line in imports.splitlines() if line.strip()}
        assert not imported_symbols.intersection({
            "socket", "connect", "send", "recv", "sendto", "recvfrom", "getaddrinfo",
            "gethostbyname", "SSL_connect", "curl_easy_perform"}), (abi, imported_symbols)
        alignments = [int(line.split()[-1], 16) for line in headers.splitlines() if line.strip().startswith("LOAD ")]
        assert alignments
        if abi in ("arm64-v8a", "x86_64"):
            assert all(value >= 16384 for value in alignments), (abi, alignments)
        results[abi] = {"bytes": binary.stat().st_size,
            "sha256": hashlib.sha256(binary.read_bytes()).hexdigest(),
            "needed_libraries": needed, "load_segment_alignment": alignments,
            "exported_symbols": symbols}
result = {"artifact": aar_path.name, "sha256": hashlib.sha256(aar_path.read_bytes()).hexdigest(),
    "bytes": aar_path.stat().st_size, "min_sdk": 21, "permissions": [], "abis": results}
(ROOT / "evidence").mkdir(exist_ok=True)
(ROOT / "evidence/android-artifact-manifest.json").write_text(json.dumps(result, indent=2) + "\n")
print(json.dumps(result, indent=2))
