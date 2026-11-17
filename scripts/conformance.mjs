// SPDX-License-Identifier: GPL-3.0-or-later
import assert from "node:assert/strict";
import { readFileSync } from "node:fs";

const module = new WebAssembly.Module(readFileSync(process.argv[2]));
assert.deepEqual(WebAssembly.Module.imports(module), [], "Engine requires host imports");
const { raw, phonemize, batchSizes } = JSON.parse(readFileSync(0, "utf8"));

function engine() {
  const { exports } = new WebAssembly.Instance(module);
  const encoder = new TextEncoder();
  const decoder = new TextDecoder("utf-8", { fatal: true });
  const memory = () => new Uint8Array(exports.memory.buffer);
  function string(pointer) {
    const bytes = memory();
    const end = bytes.indexOf(0, pointer);
    assert(pointer > 0 && end >= pointer, "Invalid UTF-8 pointer");
    return decoder.decode(bytes.subarray(pointer, end));
  }

  exports._initialize();
  assert.equal(exports.rh_espeak_abi(), 1);
  assert.equal(string(exports.rh_espeak_version()), "1.52.0-r1");
  assert.equal(exports.rh_espeak_init(), 0);

  return (method, accent, inputs) =>
    inputs.map((input) => {
      const text = encoder.encode(input + "\0");
      const pointer = exports.rh_espeak_alloc(text.length);
      assert(pointer);
      try {
        memory().set(text, pointer);
        return string(exports[`rh_espeak_${method}`](accent === "gb" ? 1 : 0, pointer));
      } finally {
        exports.rh_espeak_free(pointer);
      }
    });
}

for (const [method, cases] of [["phonemize_raw", raw], ["phonemize", phonemize]]) {
  const singleton = engine();
  for (const [accent, input, expected] of cases) {
    assert.equal(singleton(method, accent, [input])[0], expected, `${method} ${accent} ${JSON.stringify(input)}`);
  }
  const grouped = engine();
  const ordered = cases.toReversed();
  for (const size of batchSizes) {
    const groups = ["us", "gb"].map((accent) => ordered.filter(([a]) => a === accent));
    for (let index = 0; index < Math.max(...groups.map((group) => group.length)); index += size) {
      for (const group of groups) {
        const batch = group.slice(index, index + size);
        if (!batch.length) continue;
        const [[accent]] = batch;
        assert.deepEqual(
          grouped(method, accent, batch.map(([, text]) => text)),
          batch.map(([, , expected]) => expected),
          `${method} ${accent} batch ${size} at ${index}`,
        );
      }
    }
  }
  console.log(`${method}: ${cases.length} exact outputs, singleton and batches ${batchSizes.join(", ")}`);
}
