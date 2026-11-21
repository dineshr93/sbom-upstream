# Cargo

- **Data source:** `Cargo.lock` plus the resolved feature graph.
- **Flag:** `cargo sbom` subcommand or `--sbom` on `cargo build`.
- **Format:** protobuf (protobom). Rust protobuf support is mature (`prost`).
  Keep it in the cargo crate, not a plugin.
- **Gaps:** supplier is not in the registry index; license is declared in
  `Cargo.toml` and may be missing; hashes are in `Cargo.lock` (checksums).
