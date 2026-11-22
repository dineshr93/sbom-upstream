# SBOM examples

Each directory contains a small project that exercises one patched build-system implementation.

The generated SBOM files are committed next to the sample project:

- `meson/sbom.protobom.bin`: generated with `meson setup --sbom`
- `autotools/sbom.protobom.bin`: generated with `make sbom` using `impl/autotools/sbom.am`
- `go/sbom.protobom.bin`: generated with `go build -sbom`
- `bazel/sbom.protobom.bin`: generated with `bazel build --sbom_output=...`
- `gradle/sbom.json`: generated with the `sbom` task
- `maven/sbom.json`: generated with `mvn --sbom package`

The Gradle and Maven samples use local file repositories so dependency edges appear in the SBOM.
The Maven repository also includes SHA-1 checksum files because Maven checks local file artifacts.
