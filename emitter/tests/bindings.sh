#!/bin/sh
# bindings.sh — build and run every language binding, then assert that all of
# them produce byte-identical documents.
#
# SPDX-License-Identifier: MIT
set -eu

root=$(cd "$(dirname "$0")/.." && pwd)
cd "$root"
make -s all

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

# C
cc -O2 -std=c99 -Iinclude examples/usage.c libsbom_emitter.a -o "$tmp/usage_c"
(cd "$tmp" && ./usage_c)

# Python
(cd "$tmp" && PYTHONPATH="$root/bindings/python" \
  SBOM_EMITTER_LIB="$root/libsbom_emitter.so" python3 "$root/examples/usage.py")

# Go
(cd examples && go build -o "$tmp/usage_go" usage.go)
(cd "$tmp" && ./usage_go)

# Rust
(cd bindings/rust && cargo build -q --example usage)
(cd "$tmp" && "$root/bindings/rust/target/debug/examples/usage")

# Java
javac_bin=$(readlink -f "$(command -v javac)")
java_home=$(dirname "$(dirname "$javac_bin")")
mkdir -p "$tmp/java"
javac -d "$tmp/java" bindings/java/SbomEmitter.java examples/usage.java
cc -O2 -std=c99 -fPIC -shared -Iinclude -I"$java_home/include" \
  -I"$java_home/include/linux" bindings/java/sbom_jni.c libsbom_emitter.a \
  -o "$tmp/java/libsbom_emitter_jni.so"
(cd "$tmp" && java -Djava.library.path="$tmp/java" -cp "$tmp/java" usage 2>/dev/null)

count=$(sha256sum "$tmp"/sbom.protobom.bin "$tmp"/sbom.protobom.bin.* 2>/dev/null | awk '{print $1}' | sort -u | wc -l)
[ "$count" -eq 1 ] || {
  echo "FAIL: bindings produced different documents" >&2
  sha256sum "$tmp"/sbom.protobom.bin* >&2
  exit 1
}

echo "bindings: ok"
