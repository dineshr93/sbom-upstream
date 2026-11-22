# SBOM examples

Each directory is a small project that exercises one patched build-system implementation. The committed SBOM next to each sample is a real output, not a hand-written fixture.

## Generate a sample

Run these commands from the example directory after installing the patched tool:

| Sample | Command | Output | Format |
| --- | --- | --- | --- |
| meson | `meson setup --sbom builddir` | `builddir/sbom.protobom.bin` | protobom protobuf |
| autotools | `make sbom` | `sbom.protobom.bin` | protobom protobuf |
| go | `go build -sbom` | `sbom.protobom.bin` | protobom protobuf |
| bazel | `bazel build --sbom_output=sbom.protobom.bin //:app` | `sbom.protobom.bin` | protobom protobuf |
| gradle | `gradle generateSbom` | `build/reports/sbom/sbom.json` | protobom-style JSON |
| maven | `mvn --sbom package` | `sbom.json` | CycloneDX 1.6 JSON |

The committed files are copies of these outputs. Set `SOURCE_DATE_EPOCH=1700000000` when regenerating to match the committed timestamps and byte-identical output.

The Gradle and Maven samples use local file repositories so dependency edges appear in the SBOM. The Maven repository also includes SHA-1 checksum files because Maven checks local file artifacts.

## Generate a desired format later

The patched implementations do not expose a `--format` switch. Each one writes the format chosen in its design. Pick the sample whose output format matches your need:

- protobom protobuf: meson, autotools, go, bazel
- protobom-style JSON: gradle
- CycloneDX 1.6 JSON: maven
- SPDX 3.0 JSON: upstream CMake (`install(SBOM)` / `CMAKE_INSTALL_SBOM_FORMATS`). This repo skipped CMake because it already ships SBOM output
- Cargo SBOM precursor JSON: `cargo -Zsbom` (not SPDX, CycloneDX, or protobom)

For a format a patched tool does not emit, convert the generated SBOM with an external SPDX/CycloneDX converter or add a serializer to that tool. Do not parse the protobom binary as JSON.
