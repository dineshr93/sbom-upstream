# Recon: Cargo

Repo: https://github.com/rust-lang/cargo
Commit inspected: `930a674` (2026-11-22 main)
License: MIT OR Apache-2.0 (`LICENSE-MIT`, `LICENSE-APACHE`)

## Verdict: skipped — Cargo already ships SBOM generation behind `-Zsbom`

Cargo emits `*.cargo-sbom.json` files per compiled unit from the unit graph.
The format is an explicit "cargo-sbom precursor" (its own JSON schema, not
SPDX/CycloneDX/protobom) and is documented as input for external tools. The
gap versus our goal is format plus missing NTIA fields, not capability — same
position as the CMake row.

## Findings

| # | Question | Answer | Evidence |
| --- | --- | --- | --- |
| 1 | Where is the resolved component graph built? | `build_sbom_graph(build_runner, root)` walks the `BuildRunner` unit graph; output files come from `FileFlavor::Sbom` outputs | `src/compiler/output_sbom.rs:46`, `src/compiler/build_runner/mod.rs:570-577` |
| 2 | Per-component metadata | `SbomCrate { id: PackageIdSpec, features, dependencies, kind }` — no supplier, license, hash, or purl in the SBOM schema. Upstream in the resolver: checksums and license do exist | `crates/cargo-util-schemas/src/sbom.rs:90-95`, `src/resolver/resolve.rs:397` (`Resolve::checksums()`), `src/workspace/summary.rs:119` (`Summary::checksum()`), `src/workspace/manifest.rs:142` (`license: Option<String>`) |
| 3 | Existing serialization | `serde_json::to_writer` of the `Sbom` struct; extension `.cargo-sbom.json` | `src/compiler/mod.rs:370-481`, `src/compiler/build_runner/compilation_files.rs:674` |
| 4 | Flag surface | No CLI flag; unstable feature `-Zsbom` enabling `build.sbom` in `.cargo/config.toml` | `src/workspace/features.rs:930`, `src/workspace/features.rs:1473` |
| 5 | Test framework | `tests/testsuite` integration tests with snapbox | `tests/testsuite/sbom.rs` (873 lines), `crates/cargo-util-schemas/src/sbom.rs:136-141` (schema snapshot test) |
| 6 | Existing SBOM code | Yes: builder, schema, tests, feature gate | `src/compiler/output_sbom.rs` (129 lines), `crates/cargo-util-schemas/src/sbom.rs` (141 lines), `tests/testsuite/sbom.rs` |
| 7 | Language / dependency policy | Rust; no protobuf/prost dependency in `Cargo.toml`; vendored crates under `crates/` | `Cargo.toml` (grep `protobuf|prost`: no hits) |
| 8 | License compatibility | MIT OR Apache-2.0; protobom is Apache-2.0 — compatible | `LICENSE-APACHE`, protobom `LICENSE` |

## NTIA fields not obtainable from the current SBOM schema

- **Supplier**: not in `SbomCrate`; registry index does not carry it either.
- **License**: not in `SbomCrate`, though `Manifest::license`
  (`src/workspace/manifest.rs:142`) could feed a follow-up.
- **Hash**: not in `SbomCrate`, though `Resolve::checksums()`
  (`src/resolver/resolve.rs:397`) has registry checksums.
- **Timestamp / author**: absent by design — the precursor has neither; a
  protobom follow-up would need `SOURCE_DATE_EPOCH` handling.

## Why skipped

Per the repo workflow: "If recon shows the repo already has SBOM support, skip
and note why in `STATUS.md`." Cargo generates SBOM-precursor JSON per unit
today. A protobom output format (pulling license + checksum from `Manifest`
and `Resolve`) would be a format follow-up, same as the CMake row, not a new
feature — and the maintainers deliberately scoped the in-tree format as a
precursor for external converters, so a full-NTIA in-tree emitter is likely
out of their roadmap.
