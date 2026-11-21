# Meson

- **Data source:** introspection API — `meson introspect --dependencies
  --targets`.
- **Flag:** `--sbom` on `meson setup` / `meson compile`.
- **Format:** Meson is Python; protobom has no Python binding. Emit
  protobom-compatible JSON or vendor a minimal proto writer. State this
  tradeoff explicitly in the PR.
- **Gaps:** introspection reports declared dependencies, not the resolved
  transitive graph; hashes and licenses are usually absent.
