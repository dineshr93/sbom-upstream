# Status

One row per repo. Update after every step. Maintainer objections go in verbatim
so the next PR preempts them.

| Repo | Stage | Flag | Format | NTIA gaps | PR | Maintainer feedback |
| --- | --- | --- | --- | --- | --- | --- |
| cmake | skipped | `install(SBOM)` / `CMAKE_INSTALL_SBOM_FORMATS` | SPDX 3.0 JSON | supplier, hash, purl, per-target license | — | Already ships SBOM generation (`Source/cmSbomBuilder.cxx`); gap is protobom format only. See `recon/cmake.md`. |
| meson | implemented | `--sbom` | protobom binary | supplier, hash, per-target license | — | Branch `sbom-support` in the Meson clone, commit `9698d44`, 238 insertions. `AGENTS.md` bans AI-written PR text; PR must be human-authored. See `recon/meson.md`, `design/meson.md`. |
| bazel | implemented | `--sbom_output` | protobom binary | supplier, license, hash, version for non-bzlmod targets | — | Branch `sbom-support` in the Bazel clone, commit `764e327`, 459 insertions. `AGENTS.md` has no AI-authorship ban. See `recon/bazel.md`, `design/bazel.md`. |
| gradle | implemented | `sbom` task / `--configuration` | JSON (hand-rolled encoder) | supplier, hash | — | Branch `sbom-support` in the Gradle clone, commit `04bf0537`, 356 insertions. Task name, output path and configurations are all configurable properties. No protobuf runtime in Gradle, so the encoder is dependency-free. See `recon/gradle.md`, `design/gradle.md`. |
| maven | implemented | `--sbom` flag on mvn | JSON (hand-rolled encoder) | supplier, license, hash | — | Branch `sbom-support` in the Maven clone, commit `280635bc`, 181 insertions. Flag is no-arg (not `--sbom=<path>` as in design) so a following goal is not swallowed; output fixed to `<topDirectory>/sbom.json`. Option added to the mvn-specific `CLIManager` in `CommonsCliMavenOptions`, not the shared one. `mvn test-compile` + new parse test pass (2/2). Real `sbom` goal would belong in `maven-plugins` (core binds only external plugins, `JarLifecycleMappingProvider.java:33-44`); invoker-flag fallback used per design. See `recon/maven.md`, `design/maven.md`. |
| cargo | skipped | `-Zsbom` / `build.sbom` (existing) | JSON precursor (existing) | supplier, license, hash, timestamp in existing schema | — | Already ships SBOM generation (`src/compiler/output_sbom.rs`, `tests/testsuite/sbom.rs`); gap is protobom format only, and maintainers scope it as a precursor for external tools. See `recon/cargo.md`. |
| go | design | `go build -sbom` | protobuf (hand-rolled proto3 writer) | supplier, license, hash (`h1:` omitted, not a standard algorithm) | — | No SBOM in tree (zero `sbom`/`spdx`/`cyclonedx` in `cmd/go`). `BuildInfo` (`src/runtime/debug/mod.go:41`, built at `load/pkg.go:2348`, embedded at `ld/data.go:2633`) is the data source — zero new resolution. Not a cargo-style skip: no SBOM schema exists, only build metadata. No vendored protobuf in `cmd/go`. Contribution is Gerrit, not GitHub PR. Awaiting approval. See `recon/go.md`, `design/go.md`. |
| autotools | queued | `--enable-sbom` | protobuf (vendored writer) | graph, versions, hash, license | — | — |

Stages: queued → recon → design → approved → implemented → pr-drafted → pr-open
→ merged / rejected / skipped (already has SBOM support).
