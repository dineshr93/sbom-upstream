# Status

One row per repo. Update after every step. Maintainer objections go in verbatim
so the next PR preempts them.

| Repo | Stage | Flag | Format | NTIA gaps | PR | Maintainer feedback |
| --- | --- | --- | --- | --- | --- | --- |
| cmake | skipped | `install(SBOM)` / `CMAKE_INSTALL_SBOM_FORMATS` | SPDX 3.0 JSON | supplier, hash, purl, per-target license | — | Already ships SBOM generation (`Source/cmSbomBuilder.cxx`); gap is protobom format only. See `recon/cmake.md`. |
| meson | pr-open | `--sbom` | protobom binary | supplier, hash, per-target license | [#16374](https://github.com/mesonbuild/meson/pull/16374) | No maintainer feedback yet. Branch `sbom-support` in the Meson clone, commits `9698d44` and `433efed`, 252 insertions; pushed to `dineshr93/meson`. `AGENTS.md` bans AI-written PR text — user explicitly overrode that rule for this PR only. See `recon/meson.md`, `design/meson.md`, `pr/meson.md`. |
| bazel | pr-open | `--sbom_output` | protobom binary | supplier, license, hash, version for non-bzlmod targets | [#32863](https://github.com/bazelbuild/bazel/pull/32863) | No maintainer feedback yet. From `dineshr93/sbom-bazel:sbom-support`, commits `764e327` and `9b5edf8` (457 insertions, 8 files). Body = `pr/bazel.md`. |
| gradle | pr-open | `sbom` task / `--configuration` | JSON (hand-rolled encoder) | supplier, license, hash | [#39928](https://github.com/gradle/gradle/pull/39928) | No maintainer feedback yet. From `dineshr93/gradle:sbom-support`, commits `04bf0537` and `925ef93` (356 insertions, 5 files). Body = `pr/gradle.md`. |
| maven | pr-open | `--sbom` flag on mvn | CycloneDX 1.6 JSON (hand-rolled encoder) | supplier, license, hash | [#14093](https://github.com/apache/maven/pull/14093) | No maintainer feedback yet. From `dineshr93/maven:sbom-support`, commits `280635bc`, `fc81e18` and `2b36806` (303 insertions, 7 files). Body = `pr/maven.md`. |
| cargo | skipped | `-Zsbom` / `build.sbom` (existing) | JSON precursor (existing) | supplier, license, hash, timestamp in existing schema | — | Already ships SBOM generation (`src/compiler/output_sbom.rs`, `tests/testsuite/sbom.rs`); gap is protobom format only, and maintainers scope it as a precursor for external tools. See `recon/cargo.md`. |
| go | pr-drafted | `go build -sbom` | protobuf (hand-rolled proto3 writer) | supplier, license, hash (`h1:` omitted, not a standard algorithm) | — | `pr/go.md` holds the Gerrit change message, drafted from commit `c604c53d` (206 insertions, 4 files). Raise blocked: no Gerrit credentials on this machine (push to go.googlesource.com needs CLA-linked auth). |
| autotools | pr-drafted | `--enable-sbom` + `make sbom` | protobuf (shell printf writer) | purl, hash, license, supplier | — | `pr/autotools.md` holds the Savannah cover letter, drafted from commit `fbf8409` (84 insertions, 6 files). Raise blocked: no MTA/Savannah account on this machine. |

Stages: queued → recon → design → approved → implemented → pr-drafted → pr-open
→ merged / rejected / skipped (already has SBOM support).

Verification: Maven Docker compile + targeted tests pass (3/3); Gradle Docker
`embeddedIntegTest` for `SbomReportTaskIntegrationTest` passes; Bazel Docker
build target and `sbom_test` shell integration test pass.
