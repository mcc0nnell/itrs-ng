#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="${repo_root}/build-wasm"
clang_bin="${CLANG:-clang}"
wasm_ld_bin="${WASM_LD:-wasm-ld}"
resource_dir="$("${clang_bin}" -print-resource-dir)"

rm -rf "${build_dir}"
mkdir -p "${build_dir}"

cflags=(
  --target=wasm32
  -std=c11
  -Os
  -ffreestanding
  -fno-builtin
  -ffunction-sections
  -fdata-sections
  -fvisibility=hidden
  -nostdinc
  -I"${repo_root}/wasm/include"
  -I"${repo_root}/include"
  -isystem "${resource_dir}/include"
)

"${clang_bin}" "${cflags[@]}" -c "${repo_root}/src/edge/edge.c" -o "${build_dir}/edge.o"
"${clang_bin}" "${cflags[@]}" -c "${repo_root}/src/number/resolver.c" -o "${build_dir}/resolver.o"
"${clang_bin}" "${cflags[@]}" -c "${repo_root}/wasm/src/freestanding.c" -o "${build_dir}/freestanding.o"
"${clang_bin}" "${cflags[@]}" -c "${repo_root}/wasm/src/edge_wasm.c" -o "${build_dir}/edge_wasm.o"

"${wasm_ld_bin}"   --no-entry   --gc-sections   --strip-all   --export-memory   --initial-memory=262144   --max-memory=1048576   --export=itrs_edge_wasm_input_ptr   --export=itrs_edge_wasm_input_capacity   --export=itrs_edge_wasm_output_ptr   --export=itrs_edge_wasm_output_capacity   --export=itrs_edge_wasm_output_len   --export=itrs_edge_wasm_step   --export=itrs_edge_wasm_resume   "${build_dir}/edge.o"   "${build_dir}/resolver.o"   "${build_dir}/freestanding.o"   "${build_dir}/edge_wasm.o"   -o "${build_dir}/itrs_edge.wasm"

printf 'built %s\n' "${build_dir}/itrs_edge.wasm"
