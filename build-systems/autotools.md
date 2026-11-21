# Autotools

- **Data source:** limited — no resolved graph. Emit from `configure`-detected
  libraries and `pkg-config` results.
- **Flag:** `--enable-sbom` configure flag.
- **Format:** protobuf (protobom) via a vendored minimal writer; shell/m4 has no
  protobuf tooling.
- **Gaps:** this is the weakest integration. No transitive graph, no versions for
  many detected libs, no hashes, no licenses. Document the accuracy limitation
  loudly in the PR and in the generated document's metadata.
