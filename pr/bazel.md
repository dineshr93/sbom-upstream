# PR: bazel

PR: https://github.com/bazelbuild/bazel/pull/32863 (open)

Title: `Add opt-in SBOM generation (--sbom_output)`

Branch: `sbom-support`, commit `764e327`, 457 insertions across 8 files.

---

## Motivation

Regulations (NTIA minimum elements, EO 14028, EU CRA) require an SBOM, and
Bazel already knows every component in the build. `--sbom_output=<path>` emits
a protobom `Document` describing the configured targets reachable from the
requested targets, so downstream patching and third-party scanners become
unnecessary.

## Design

- `SbomOptions.java`: one `@Option` class, added to `BuildCommand`'s options
  list — Bazel's per-feature option convention.
- `SbomWriter.java`: walks the Skyframe graph from the top-level targets
  (`WalkableGraph.getDirectDeps` filtered to `ConfiguredTargetKey`) and writes
  the document with `CodedOutputStream`. One call site after
  `BuildTool.processRequest` covers both Skymeld and non-Skymeld paths, since
  `BuildResult.getActualTargets()` is populated in both.
- Version comes from `ModuleKey.version()` via `BazelDepGraphValue`; targets
  outside a bzlmod module omit version/purl rather than fabricate one.
- Node id = canonical label form; edges are `dependsOn` (type 10); nodes
  sorted by id, edges by `(from, to)`, roots sorted by id.

## Alternatives considered

- `protobuf-java` runtime: already linked into the `commands` target, so
  nothing new is added; a hand-rolled encoder (as in Gradle) would be more
  code for the same bytes.
- SPDX/CycloneDX JSON: rejected — protobom protobuf is the project format and
  Bazel has native protobuf support.
- `--experimental_sbom_output`: Bazel prefixes unstable flags with
  `experimental_`. One-line rename if maintainers prefer it.

## Test plan

- `src/test/shell/integration/sbom_test.sh`: builds a genrule with the flag;
  asserts the file is non-empty and contains the top-level node id, the
  implicit `genrule-setup.sh` node, and the `bazel` tool entry.
- Determinism: two runs with `SOURCE_DATE_EPOCH=1700000000` produce
  byte-identical files.
- Off by default: `bazel build` without the flag writes no file.
- Schema conformance verified out of band with
  `protoc --decode=protobom.protobom.Document`; emitted bytes decode cleanly.

## Backwards compatibility

- Flag is opt-in, off by default; no behaviour change for existing builds.
- No new dependency; no new Skyframe node; graph walk is read-only and runs
  after the build completes.

## Gaps (documented, never fabricated)

- supplier: not modelled in Bazel's core data — omitted.
- license: not modelled in core — omitted.
- hash: not modelled — omitted (not an NTIA minimum element).
- version/purl for non-bzlmod targets: omitted, no module mapping exists.
