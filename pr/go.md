# Gerrit change message: go

(Not a GitHub PR — Go's contribution flow is Gerrit. Commit `c604c53d`,
206 insertions across 4 files. Change-Id added by the commit hook.)

---

cmd/go: add go build -sbom

go build -sbom writes sbom.protobom.bin, a protobom Document describing
the build's module graph. The data comes from setBuildInfo, which already
produces the exact debug.BuildInfo (main module, all deps, sums) for the
main package; nothing new is resolved.

Why a flag on go build rather than a go mod subcommand: the BuildInfo
only exists after a build, so a subcommand would have to re-derive it.
The flag is a bool with no value, matching -json, so no argument parsing
ambiguity.

Implementation:
- work/build.go: register the sbom bool flag, stored on cfg (cfg.go +1).
- work/sbom.go (new, 134 LOC): hand-rolled proto3 writer using only the
  varint and length-delimited wire types. cmd/go vendors no protobuf and
  adding one is unacceptable; the encoder is 60 lines of the file and
  emits the same bytes protoc accepts.
- Output: sbom.protobom.bin next to the built executable (or cwd when -o
  is absent). Fixed name, no path parsing.
- Nodes sorted by id, edges by (from, to), map entries by key; timestamp
  from SOURCE_DATE_EPOCH, else vcs.time, else now. Two runs with
  SOURCE_DATE_EPOCH set are byte-identical.

Fields: name/version from debug.Module, purl synthesized as
pkg:golang/<Path>@<Version>, main module dependsOn each Deps entry,
author "go build" as org person.

Deliberate omissions (never fabricated):
- supplier, license: not in BuildInfo.
- Module.Sum: it is an h1: dirhash of the module zip, not a file hash, so
  it is not mapped to a protobom hash algorithm.

Tests: sbom_test.go (50 LOC) golden test asserts the encoded bytes decode
cleanly with protoc --decode against the protobom schema and match the
golden document; 3/3 pass. Off by default: nothing runs without -sbom, no
cost in the default build, no new dependency.
