#!/usr/bin/env python3
"""Fail closed if the pinned integration/source/build inputs have changed."""
from pathlib import Path
import hashlib,json
ROOT=Path(__file__).resolve().parents[1]
def files():
    names={"CMakeLists.txt","LICENSE","THIRD_PARTY_NOTICES.txt","provenance/source-manifest.json","provenance/telegram-use.json","provenance/reviewed-android-native.json"}
    for folder in ("vendor","src","include","java","android","examples","tests","tools"):
        for p in (ROOT/folder).rglob("*"):
            if p.is_file() and "__pycache__" not in p.parts:names.add(p.relative_to(ROOT).as_posix())
    return {name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest() for name in sorted(names)}
if __name__=="__main__":
    expected=json.loads((ROOT/"provenance/build-inputs.json").read_text())["files"]
    actual=files()
    if actual!=expected:
        changed=sorted(k for k in set(actual)|set(expected) if actual.get(k)!=expected.get(k))
        raise SystemExit("Pinned integration/build inputs changed: "+", ".join(changed))
    print("PASS: complete pinned source/build inputs ("+str(len(actual))+" files)")
