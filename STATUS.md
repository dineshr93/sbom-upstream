# Status

One row per repo. Update after every step. Maintainer objections go in verbatim
so the next PR preempts them.

| Repo | Stage | Flag | Format | NTIA gaps | PR | Maintainer feedback |
| --- | --- | --- | --- | --- | --- | --- |
| cmake | skipped | `install(SBOM)` / `CMAKE_INSTALL_SBOM_FORMATS` | SPDX 3.0 JSON | supplier, hash, purl, per-target license | — | Already ships SBOM generation (`Source/cmSbomBuilder.cxx`); gap is protobom format only. See `recon/cmake.md`. |
| meson | implemented | `--sbom` | protobom binary | supplier, hash, per-target license | — | Branch `sbom-support` in the Meson clone, commit `9698d44`, 238 insertions. `AGENTS.md` bans AI-written PR text; PR must be human-authored. See `recon/meson.md`, `design/meson.md`. |
| bazel | implemented | `--sbom_output` | protobom binary | supplier, license, hash, version for non-bzlmod targets | — | Branch `sbom-support` in the Bazel clone, commit `764e327`, 459 insertions. `AGENTS.md` has no AI-authorship ban. See `recon/bazel.md`, `design/bazel.md`. |
| gradle | implemented | `sbom` task / `--configuration` | JSON (hand-rolled encoder) | supplier, hash | — | Branch `sbom-support` in the Gradle clone, commit `04bf0537`, 356 insertions. Task name, output path and configurations are all configurable properties. No protobuf runtime in Gradle, so the encoder is dependency-free. See `recon/gradle.md`, `design/gradle.md`. |
| maven | design | `--sbom=<path>` flag on mvn | JSON (no protobuf in core) | supplier, license, hash | — | No SBOM in-tree (all `sbom` grep hits are `isBom`/`QuarkusBom`). maven-core has zero protobuf dep and implements no build step itself (`JarLifecycleMappingProvider.java:33-44` binds only to external `maven-*-plugin`s), so a real `sbom` goal belongs in the separate `maven-plugins` repo. Design uses the mvn-invoker flag fallback. Awaiting approval. See `recon/maven.md`, `design/maven.md`. |
| cargo | queued | `cargo sbom` / `--sbom` | protobuf | supplier, license | — | — |
| go | queued | `go build -sbom` | protobuf | supplier, license | — | — |
| autotools | queued | `--enable-sbom` | protobuf (vendored writer) | graph, versions, hash, license | — | — |

Stages: queued → recon → design → approved → implemented → pr-drafted → pr-open
→ merged / rejected / skipped (already has SBOM support).
