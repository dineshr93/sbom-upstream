# Embeddable SBOM emitter — design brief

Design a small, embeddable "SBOM emitter" that build systems can vendor or link,
producing protobom protobuf output with NTIA minimum elements.

## Constraints

- C ABI core (usable from CMake/Meson/Autotools), with thin wrappers for Rust,
  Go, Java, Python.
- <2k LOC, no runtime deps beyond a protobuf encoder.
- API: `init(tool_name, tool_version)` → `add_node(purl, name, version,
  supplier, license, hash)` → `add_edge(from, to)` → `write(path)`.
- Deterministic output (stable ordering) so SBOMs diff cleanly in CI.
- Include a conformance test against the protobom schema.

## Deliver

Repo layout, API header, and a 30-line usage example for each host language.

## Why it exists

The fallback path for any build system whose dependency policy forbids new
third-party libraries, or whose language has no protobom binding (Meson,
Autotools). Vendoring this emitter is a smaller ask than adding a dependency.
