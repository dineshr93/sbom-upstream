# Bazel

- **Data source:** an aspect (`sbom_aspect`) that walks the configured target
  graph, reusing existing `providers` for metadata.
- **Flag:** `--sbom_output`.
- **Implementation:** Starlark. Shell out to nothing.
- **Format:** protobuf (protobom); Java protobuf support is available, so use
  protobom's Java bindings if published, else generate from the `.proto`.
- **Gaps:** license and supplier are only present when rules declare them;
  external repo versions come from the module graph, not the target graph.
