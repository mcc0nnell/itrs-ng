#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
CACHE="$ROOT/.cache"
SRC="$CACHE/sipx"
OUT="$ROOT/out"
SIPX_COMMIT="cb71afd95a0fe2bf7405b45285cae17a2195b4d4"
RUST_TOOLCHAIN="1.95.0-x86_64-unknown-linux-gnu"

export PATH="$HOME/.cargo/bin:$PATH"
mkdir -p "$CACHE" "$OUT" "$ROOT/evidence"

if [[ ! -d "$SRC/.git" ]]; then
  git clone https://github.com/codewandler/sipx.git "$SRC"
fi

git -C "$SRC" fetch origin "$SIPX_COMMIT"
git -C "$SRC" checkout --detach "$SIPX_COMMIT"

rustup target add wasm32-unknown-unknown --toolchain "$RUST_TOOLCHAIN"

(
  cd "$SRC/wasm"
  RUSTFLAGS="-C link-arg=--max-memory=33554432 --remap-path-prefix=$SRC=/src/sipx --remap-path-prefix=$HOME/.cargo=/cargo" \
    cargo +"$RUST_TOOLCHAIN" build --release --target wasm32-unknown-unknown
)

WASM_SRC="$SRC/wasm/target/wasm32-unknown-unknown/release/sipx_browser_wasm.wasm"
WASM_OUT="$OUT/sipx_browser.wasm"
cp "$WASM_SRC" "$WASM_OUT"

node "$SRC/wasm/harness.mjs" "$WASM_OUT"
node "$ROOT/twitch.mjs" "$WASM_OUT" "$ROOT/evidence/twitch.json"

printf '\nartifact: %s\n' "$WASM_OUT"
sha256sum "$WASM_OUT"
printf 'evidence: %s\n' "$ROOT/evidence/twitch.json"
