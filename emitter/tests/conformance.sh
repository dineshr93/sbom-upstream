#!/bin/sh
# conformance.sh — decode the emitted document with protoc against the protobom
# schema and assert the NTIA minimum elements are present.
#
# SPDX-License-Identifier: MIT
set -eu

bin=${1:?usage: conformance.sh <sbom.bin>}
here=$(dirname "$0")
decoded=$(mktemp)
trap 'rm -f "$decoded"' EXIT

protoc --proto_path="$here/proto" --proto_path=/usr/include \
  --decode=protobom.protobom.Document "$here/proto/sbom.proto" <"$bin" >"$decoded"

fail() {
  echo "FAIL: $1" >&2
  cat "$decoded" >&2
  exit 1
}

grep -q 'name: "zlib"' "$decoded" || fail "component name"
grep -q 'version: "1.3"' "$decoded" || fail "component version"
grep -q 'suppliers {' "$decoded" || fail "supplier"
grep -q 'name: "zlib-ng"' "$decoded" || fail "supplier name"
grep -q 'identifiers {' "$decoded" || fail "identifier map"
grep -q 'value: "pkg:generic/zlib@1.3"' "$decoded" || fail "purl"
grep -q 'hashes {' "$decoded" || fail "hash map"
grep -q 'value: "deadbeef"' "$decoded" || fail "hash value"
grep -q 'type: dependsOn' "$decoded" || fail "dependency edge"
grep -q 'date {' "$decoded" || fail "timestamp"
grep -q 'seconds: 1700000000' "$decoded" || fail "timestamp value"
grep -q 'tools {' "$decoded" || fail "tool"
grep -q 'name: "cmake"' "$decoded" || fail "tool name"
grep -q 'authors {' "$decoded" || fail "author"
grep -q 'root_elements: "pkg:generic/app@1.0"' "$decoded" || fail "root element"

echo "conformance: ok"
