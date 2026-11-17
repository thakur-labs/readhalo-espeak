#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
export PATH="$PWD/.tooling/python/bin:$PATH"
export LC_ALL=C SOURCE_DATE_EPOCH=1733961600
emscripten="$PWD/.tooling/emsdk/upstream/emscripten"
test "$(git -C .tooling/emsdk rev-parse HEAD)" = 708dcbcc9ee23bc7d2dad44fc90db377e38b771c
test "$(cat "$emscripten/emscripten-revision.txt")" = a1fe3902bf73a3802eae0357d273d0e37ea79898
mkdir -p .build dist
# The pinned upstream build otherwise fetches Sonic even with USE_LIBSONIC=OFF.
options=(-G Ninja -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_BUILD_TYPE=Release
  -DBUILD_SHARED_LIBS=OFF -DBUILD_TESTING=OFF -DUSE_LIBSONIC=OFF
  -DSONIC_LIB=unused -DSONIC_INC=unused -DUSE_MBROLA=OFF -DUSE_LIBPCAUDIO=OFF
  -DUSE_ASYNC=OFF -DUSE_KLATT=OFF -DUSE_SPEECHPLAYER=OFF)
cmake -S vendor -B .build/native "${options[@]}"
cmake --build .build/native --parallel "${JOBS:-4}" --target espeak-ng-data/en_dict
"$emscripten/emcmake" cmake -S vendor -B .build/wasm "${options[@]}" -DCMAKE_INSTALL_PREFIX=/ -DCMAKE_C_FLAGS="-include wchar.h"
cmake --build .build/wasm --parallel "${JOBS:-4}" --target espeak-ng
python3 scripts/embed.py .build/native/espeak-ng-data > .build/english.h
"$emscripten/emcc" src/bridge.c src/module.c src/embedded.c src/no-host.c \
  .build/wasm/src/libespeak-ng/libespeak-ng.a .build/wasm/src/ucd-tools/libucd.a \
  -Ivendor/src/include -I.build -O3 --no-entry -sFILESYSTEM=0 \
  -sALLOW_MEMORY_GROWTH=1 -sINITIAL_MEMORY=16777216 -sMAXIMUM_MEMORY=67108864 \
  -sEXPORTED_FUNCTIONS='["_rh_espeak_abi","_rh_espeak_init","_rh_espeak_version","_rh_espeak_alloc","_rh_espeak_free","_rh_espeak_phonemize","_rh_espeak_phonemize_raw"]' \
  -Wl,--wrap=fopen -Wl,--wrap=stat -Wl,--wrap=opendir -Wl,--wrap=readdir -Wl,--wrap=closedir \
  -Wl,--wrap=__wasi_clock_time_get,--wrap=__wasi_proc_exit,--wrap=__wasi_environ_sizes_get,--wrap=__wasi_environ_get \
  -Wl,--wrap=__wasi_fd_close,--wrap=__wasi_fd_write,--wrap=__wasi_fd_seek \
  -o dist/espeak-ng-en.wasm
