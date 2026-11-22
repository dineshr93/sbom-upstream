# Status

One row per repo. Update after every step. Maintainer objections go in verbatim
so the next PR preempts them.

| Repo | Stage | Flag | Format | NTIA gaps | PR | Maintainer feedback |
| --- | --- | --- | --- | --- | --- | --- |
| cmake | skipped | `install(SBOM)` / `CMAKE_INSTALL_SBOM_FORMATS` | SPDX 3.0 JSON | supplier, hash, purl, per-target license | — | Already ships SBOM generation (`Source/cmSbomBuilder.cxx`); gap is protobom format only. See `recon/cmake.md`. |
| meson | implemented | `--sbom` | protobom binary | supplier, hash, per-target license | — | Branch `sbom-support` in the Meson clone, commit `9698d44`, 238 insertions. `AGENTS.md` bans AI-written PR text; PR must be human-authored. See `recon/meson.md`, `design/meson.md`. |
| bazel | pr-drafted | `--sbom_output` | protobom binary | supplier, license, hash, version for non-bzlmod targets | — | `pr/bazel.md` drafted from commit `764e327` (459 insertions, 8 files). `AGENTS.md` has no AI-authorship ban. |
| gradle | pr-drafted | `generateSbom` task / `--configuration` | JSON (hand-rolled encoder) | supplier, hash | — | `pr/gradle.md` drafted from commit `04bf0537` (356 insertions, 5 files). |
| maven | pr-drafted | `--sbom` flag on mvn | JSON (hand-rolled encoder) | supplier, license, hash | — | `pr/maven.md` drafted from commit `280635bc` (181 insertions, 6 files). |
| cargo | skipped | `-Zsbom` / `build.sbom` (existing) | JSON precursor (existing) | supplier, license, hash, timestamp in existing schema | — | Already ships SBOM generation (`src/compiler/output_sbom.rs`, `tests/testsuite/sbom.rs`); gap is protobom format only, and maintainers scope it as a precursor for external tools. See `recon/cargo.md`. |
| go | pr-drafted | `go build -sbom` | protobuf (hand-rolled proto3 writer) | supplier, license, hash (`h1:` omitted, not a standard algorithm) | — | `pr/go.md` holds the Gerrit change message, drafted from commit `c604c53d` (206 insertions, 4 files). |
| autotools | pr-drafted | `--enable-sbom` + `make sbom` | protobuf (shell printf writer) | purl, hash, license, supplier | — | `pr/autotools.md` holds the Savannah cover letter, drafted from commit `fbf8409` (84 insertions, 6 files). |

Stages: queued → recon → design → approved → implemented → pr-drafted → pr-open
→ merged / rejected / skipped (already has SBOM support).
