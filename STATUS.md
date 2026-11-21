# Status

One row per repo. Update after every step. Maintainer objections go in verbatim
so the next PR preempts them.

| Repo | Stage | Flag | Format | NTIA gaps | PR | Maintainer feedback |
| --- | --- | --- | --- | --- | --- | --- |
| cmake | queued | `--sbom` / `CMAKE_SBOM` | protobuf | license, hash, purl | — | — |
| meson | queued | `--sbom` | protobom JSON or vendored proto | transitive graph, hash, license | — | — |
| bazel | queued | `--sbom_output` | protobuf | license, supplier | — | — |
| gradle | queued | `generateSbom` task | protobuf | supplier, license | — | — |
| maven | queued | lifecycle binding / plugin | protobuf | supplier, license | — | — |
| cargo | queued | `cargo sbom` / `--sbom` | protobuf | supplier, license | — | — |
| go | queued | `go build -sbom` | protobuf | supplier, license | — | — |
| autotools | queued | `--enable-sbom` | protobuf (vendored writer) | graph, versions, hash, license | — | — |

Stages: queued → recon → design → approved → implemented → pr-drafted → pr-open
→ merged / rejected / skipped (already has SBOM support).
