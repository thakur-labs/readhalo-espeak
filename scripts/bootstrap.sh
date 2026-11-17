#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
sdk_revision=708dcbcc9ee23bc7d2dad44fc90db377e38b771c
mkdir -p .tooling
if [[ ! -d .tooling/emsdk/.git ]]; then
  git init -q .tooling/emsdk
  git -C .tooling/emsdk remote add origin https://github.com/emscripten-core/emsdk.git
fi
if ! git -C .tooling/emsdk cat-file -e "$sdk_revision^{commit}" 2>/dev/null; then
  git -C .tooling/emsdk fetch --depth 1 origin "$sdk_revision"
fi
git -C .tooling/emsdk checkout --detach "$sdk_revision"
.tooling/emsdk/emsdk install 3.1.64
.tooling/emsdk/emsdk activate --embedded 3.1.64
python3 -m venv .tooling/python
.tooling/python/bin/pip install 'cmake==4.4.5' 'ninja==1.13.2'
