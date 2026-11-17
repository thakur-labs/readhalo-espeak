#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Generate a read-only data table in a stable order."""
from pathlib import Path
import sys

root = Path(sys.argv[1])
files = ("en_dict", "intonations", "phondata", "phonindex", "phontab", "lang/gmw/en", "lang/gmw/en-US")
for index, name in enumerate(files):
    data = (root / name).read_bytes()
    print(f"static const unsigned char data{index}[] = {{")
    for start in range(0, len(data), 32):
        print(",".join(map(str, data[start:start + 32])) + ",")
    print("};")

print("static const struct embedded_file files[] = {")
for index, name in enumerate(files):
    print(f'  {{"/data/{name.lower()}", data{index}, sizeof data{index}}},')
print("};")
