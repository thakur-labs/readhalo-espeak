#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Run with the read-only pinned reference Python and a POC fixture directory."""
import hashlib
import json
from collections import Counter
from importlib.metadata import version
from pathlib import Path
import subprocess
import sys

from misaki.espeak import EspeakFallback

poc, wasm = map(Path, sys.argv[1:])
assert version("misaki") == "0.9.4"
assert version("espeakng-loader") == "0.2.4"
assert version("phonemizer-fork") == "3.3.2"
mappers = {accent: EspeakFallback(british=accent == "gb") for accent in ("us", "gb")}
assert all(mapper.backend._espeak.version == (1, 52, 0) for mapper in mappers.values())

def load(name):
    return json.loads((poc / name).read_bytes())

calls = load("calls.json")
captured = {tuple(json.loads(key)): value for key, value in load("raw-wasm.json").items()}
report = load("mapping-report.json")
assert report["raw"] == {"total": len(captured), "different": []}
assert report["mapped"] == {"total": len(calls), "different": []}

chunks = {}
for accent, text in calls:
    backend = mappers[accent].backend
    for chunk in backend._phonemize_preprocess([text])[0]:
        chunks[accent, chunk] = backend._espeak.text_to_phonemes(chunk, "^")
assert chunks == captured, "Current oracle chunks differ from the captured POC"

def expected(accent, text):
    lines = mappers[accent].backend.phonemize([text])
    return lines[0].strip() if lines else ""

probes = [
    "", "   ", " .! ", "Hello, world!", "hello   —  …  world",
    "[who", "401(k", "a (b) c", "¡hola! ¿Qué?", "a\t;\u00a0b",
    "take\ncare", "a…b…c", "…hello…world…", "hello;: . good night",
    "école", "a^b", "it's well-known", "Mr. O’Connor", "résumé",
    "b\u200bc", "a… \u200b", "\u200b […]", "😊, 😊",
]
extra = [(accent, probe) for probe in probes for accent in mappers]
whole = [(accent, text, expected(accent, text)) for accent, text in [*calls, *extra]]
raw = [(accent, text, ipa) for (accent, text), ipa in chunks.items()]
reference = {"raw": raw, "phonemize": whole, "batchSizes": report["batchSizes"]}
subprocess.run(
    ["node", str(Path(__file__).with_name("conformance.mjs")), str(wasm)],
    input=json.dumps(reference, ensure_ascii=False).encode("utf-8"),
    check=True,
)
print("Raw chunks by accent:", dict(Counter(accent for accent, _, _ in raw)))
print("Whole calls by accent:", dict(Counter(accent for accent, _, _ in whole)))
for name in ("calls.json", "raw-wasm.json", "mapping-report.json"):
    print(hashlib.sha256((poc / name).read_bytes()).hexdigest(), name)
