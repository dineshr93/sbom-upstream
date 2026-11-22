PR: https://github.com/mesonbuild/meson/pull/16374 (open)

## What

Adds `meson setup --sbom`: writes a protobom (protobuf) SBOM for the configured project to `<builddir>/sbom.protobom.bin`. Opt-in, off by default, no new dependency.

```console
$ meson setup --sbom builddir
...
SBOM: builddir/sbom.protobom.bin
```

## Why

protobom (https://github.com/protobom/protobom) is a language-neutral SBOM model with tooling in Go, Java, TypeScript and Rust. A native serialization for build systems that don't embed a protobuf runtime is a ~40-line wire encoder (varint + length-delimited only), not a dependency.

## How

- `mesonbuild/sbom.py`: minimal proto3 wire encoder + document builder. Imports only `os`, `time` and mesonbuild internals; no protobuf runtime.
- `mesonbuild/msetup.py`: `--sbom` boolean; emission after `mintro.generate_introspection_file()`, where the dependency graph is complete and no backend code is involved.
- Content: one node per build target (`Target.get_id()`/`get_name()`, project version) plus one node per external dependency, `dependsOn` edges from link deps, `pkg:generic/<name>@<version>` purls. If an external dependency has no version, its id is the name alone (no `name@None`). Timestamp honors `SOURCE_DATE_EPOCH`, so output is byte-deterministic.
- Deliberate gaps, documented: no `Node.hashes` (artifacts are not built at configure time), no `Node.suppliers`, no `Node.licenses`. Nothing is fabricated.
- The flag is a plain boolean, not `--sbom [<path>]`: an optional value would be ambiguous with the positional builddir.

## Tests

- Golden-bytes unit test: fixed in-memory graph (one target, project version), fixed timestamp, byte-exact assertion.
- External-dependency edge case: no version means id/name alone, no `name@None` purl.
- Integration test: `meson setup --sbom` on a two-target project; decodes with `protoc --decode` when protoc is on PATH (skipped otherwise).
- Determinism test: two runs with the same `SOURCE_DATE_EPOCH` produce identical bytes.
