# ReadHalo eSpeak NG

English phonemization from eSpeak NG 1.52.0. The release is one standalone
WebAssembly binary with embedded US and GB voice data. It has zero imports
and runs inside its own linear memory.

## Build the engine

Release `1.52.0-r2` updates the notice and reuses the exact Wasm from
`1.52.0-r1`. The Wasm still reports `1.52.0-r1` through the ABI.

Use Bash, Git, Python 3 with venv, and a native C toolchain. The source under
`vendor/` is pinned to commit `4870adfa25b1a32b4361592f1be8a40337c58d6c`.
The bootstrap installs Emscripten 3.1.64, CMake and Ninja under `.tooling/`.

```sh
./scripts/bootstrap.sh
./scripts/build.sh
shasum -a 256 -c SHA256SUMS
```

The native data compiler builds the English dictionary and its required phoneme
and intonation tables from the vendored source. The Wasm links the same eSpeak
sources with the C bridge and embeds the compiled data. The build output is
`dist/espeak-ng-en.wasm`. The SHA-256 in `SHA256SUMS` identifies that asset.

## C ABI v1

Instantiate the verified Wasm bytes with an empty imports object. Call
`_initialize()` once per instance, check `rh_espeak_abi()`, and call
`rh_espeak_init()` before using the module.

| Export | Wasm signature | Contract |
| --- | --- | --- |
| `memory` | memory | Linear memory. Obtain a fresh view after a call that can grow it. |
| `rh_espeak_abi` | `() -> i32` | ABI revision `1`. |
| `rh_espeak_version` | `() -> i32` | Pointer to immutable NUL-terminated UTF-8 asset version `1.52.0-r1`. |
| `rh_espeak_init` | `() -> i32` | Load embedded tables. Returns `0` on success. |
| `rh_espeak_alloc` | `(i32) -> i32` | Allocate input bytes, including the NUL. Returns `0` on failure. |
| `rh_espeak_free` | `(i32) -> ()` | Free an input block returned by `rh_espeak_alloc`. |
| `rh_espeak_phonemize` | `(i32, i32) -> i32` | Accent `0` for en-US or `1` for en-GB, then a NUL-terminated UTF-8 original token. Returns a NUL-terminated result or `0` on error. |
| `rh_espeak_phonemize_raw` | `(i32, i32) -> i32` | Same signature. Diagnostic entry point for the original POC bridge: pass a preprocessed chunk, get IPA with U+0361 tie bars. |

`rh_espeak_phonemize` returns the output of the pinned Phonemizer backend's
single-input call after trimming outer whitespace. It preserves punctuation
and language-switch flags, uses `^` ties, and returns IPA before Misaki's E2M
mapping. Do not strip punctuation or change spacing in the input. Input must be
valid UTF-8 with no NULs and at most 65,535 bytes before the terminator.
The output buffer holds up to 65,535 bytes. A nonzero pointer to a NUL
represents an empty result. Copy the result before the next phonemization call;
the engine owns the returned pointer. Serialize calls within each instance.
For a batch, call once per input and preserve order.

## Conformance

Use Node.js 22+ and the pinned reference environment with Misaki 0.9.4,
espeakng-loader 0.2.4 (eSpeak NG 1.52.0) and phonemizer-fork 3.3.2.
Pass the POC directory containing `calls.json`, `raw-wasm.json`, and
`mapping-report.json`.

```sh
"$REFERENCE_PYTHON" -B scripts/conformance.py "$POC_CORPUS" dist/espeak-ng-en.wasm
```

The test runs both accents, checks the raw chunk snapshot against the live
reference, and checks raw chunks and original-token results against that
reference in singleton calls and reordered batches of 1, 8, 32 and 128.
It instantiates the Wasm with zero imports.

See [NOTICE](NOTICE) and [LICENSE](LICENSE) for attribution and licensing.

To prepare the downloadable NOTICE, append the immutable source commit to the
notice from the release tag after the tag has been created:

```sh
tag=1.52.0-r2
commit=$(git rev-parse "${tag}^{commit}")
mkdir -p dist
{
  git show "${commit}:NOTICE"
  printf '\nSource commit: %s\n' "$commit"
  printf 'Source tree: https://github.com/thakur-labs/readhalo-espeak/tree/%s\n' "$commit"
} > dist/NOTICE
```
