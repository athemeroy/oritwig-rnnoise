#!/usr/bin/env python3
"""Offline provenance gate: exact Git tree and optional SHA-256 manifest."""
import argparse
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PIN = "f2908b14133bbffbf7ab04f641ecb5bfaf533242"
TREE = "91c3012f2d1c6b357b879be725ca3df50bca50a5"
SOURCE = "TMessagesProj/jni/voip/rnnoise"

def git_hash(kind, data):
    return hashlib.sha1(kind.encode() + b" " + str(len(data)).encode() + b"\0" + data).hexdigest()

def verify(write=False):
    metadata = json.loads((ROOT / "provenance/telegram-rnnoise-tree.json").read_text())
    assert metadata["sha"] == TREE and not metadata["truncated"]
    files = [item for item in metadata["tree"] if item["type"] == "blob"]
    root = ROOT / "vendor/rnnoise"
    assert sorted(x["path"] for x in files) == sorted(
        p.relative_to(root).as_posix() for p in root.rglob("*") if p.is_file()
    ), "Unexpected or missing vendored file"
    entries = []
    hashes = {}
    for item in files:
        contents = (root / item["path"]).read_bytes()
        digest = git_hash("blob", contents)
        assert len(contents) == item["size"] and digest == item["sha"], item["path"]
        assert item["mode"] == "100644"
        hashes[item["path"]] = digest
        entries.append({
            "path": item["path"], "bytes": len(contents), "git_blob_sha1": digest,
            "sha256": hashlib.sha256(contents).hexdigest(),
            "source_url": f"https://github.com/DrKLO/Telegram/blob/{PIN}/{SOURCE}/{item['path']}",
            "modified": False,
        })
    def tree_hash(directory):
        direct = []
        for child in (root / directory).iterdir():
            relative = child.relative_to(root).as_posix()
            is_dir = child.is_dir()
            digest = tree_hash(relative) if is_dir else hashes[relative]
            name = child.name.encode()
            direct.append((name + (b"/" if is_dir else b""), (b"40000" if is_dir else b"100644")
                           + b" " + name + b"\0" + bytes.fromhex(digest)))
        return git_hash("tree", b"".join(value for _, value in sorted(direct)))
    assert tree_hash("") == TREE, "Vendored tree differs from Telegram pin"
    result = {
        "repository": "https://github.com/DrKLO/Telegram",
        "telegram_commit": PIN, "source_path": SOURCE, "source_tree_sha1": TREE,
        "authorship": "Xiph/Mozilla RNNoise and listed upstream contributors; vendored/used by Telegram",
        "vendored_file_count": len(entries),
        "vendored_bytes": sum(item["bytes"] for item in entries),
        "modified_upstream_files": [], "files": entries,
    }
    manifest = ROOT / "provenance/source-manifest.json"
    if write:
        manifest.write_text(json.dumps(result, indent=2) + "\n")
    else:
        assert json.loads(manifest.read_text()) == result, "Manifest mismatch"
    print(f"PASS: {len(entries)} unchanged files, {result['vendored_bytes']} bytes, exact tree {TREE}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--write-manifest", action="store_true")
    verify(parser.parse_args().write_manifest)
