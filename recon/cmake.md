# Recon: CMake

Repo: https://github.com/Kitware/CMake
Commit inspected: `b8f3a353` (2026-11-21 nightly)
License: BSD-3-Clause (`LICENSE.rst:1`)

## Verdict: skipped — CMake already ships first-class SBOM generation

CMake generates SPDX 3.0 JSON SBOMs from the install/build export sets. There is
no protobom/protobuf output, but the format layer is already abstracted behind a
serializer interface, so adding a protobom format is a small follow-up rather
than a new feature.

## Findings

| # | Question | Answer | Evidence |
| --- | --- | --- | --- |
| 1 | Where is the resolved component graph built? | Export sets (`cmExportSet`) resolved per local generator; link graph walked from `cmGeneratorTarget` via `cmLinkItem` | `Source/cmSbomBuilder.h:40-149`, `Source/cmSbomBuilder.cxx` (588 lines), `Source/cmExportFileGenerator.h` |
| 2 | Per-component metadata available | name, version, description, website, package URL, declared license, data license, CPS package info (name/version/compat version) | `Source/cmSbomBuilder.h:125-131`, `Source/cmPackageInfoReader.h:43-58` |
| 3 | Existing serialization mechanisms | `cmSbomSerializer` interface with `cmSpdxSerializer` (JSON via `cm3p/json`) | `Source/cmSpdxSerializer.h:15-40`, `Source/cmSpdx.h` (404 lines) |
| 4 | CLI / command surface | `install(SBOM <name> ...)` and `export(SBOM <name> ...)`; `CMAKE_INSTALL_SBOM_FORMATS` auto-generates one SBOM per export set | `Source/cmInstallCommand.cxx:2853-2912`, `Source/cmGlobalGenerator.cxx:1833-1837`, `Help/command/install.rst:1312`, `Help/command/export.rst:173`, `Help/variable/CMAKE_INSTALL_SBOM_FORMATS.rst:1-24` |
| 5 | Test framework | `Tests/RunCMake` with per-feature directories and `*-check.cmake` assertions | `Tests/RunCMake/Sbom/`, `Tests/RunCMake/InstallSbom/`, `Tests/RunCMake/ExportSbom/`, `Tests/RunCMake/EnvSbom/` |
| 6 | Existing SBOM code | Yes: builders, generators, serializer, SPDX model | `Source/cmSbomBuilder.*`, `Source/cmBuildSbomBuilder.*`, `Source/cmInstallSbomBuilder.*`, `Source/cmBuildSbomGenerator.*`, `Source/cmInstallSbomGenerator.*`, `Source/cmSpdxSerializer.*`, `Source/cmSpdx.h` |
| 7 | Language / dependency policy | C++17, bootstrapped; vendored third-party under `Utilities/cm3p` (json, curl, etc.); no new mandatory runtime deps | `Source/cmSpdxSerializer.h:9-10`, `Utilities/` |
| 8 | License compatibility | BSD-3-Clause; protobom is Apache-2.0 — compatible for a vendored writer | `LICENSE.rst:1`, protobom `LICENSE` |

## Supported formats

`cmSbomArguments::SbomFormat` has exactly two values:

```cpp
enum class SbomFormat
{
  SPDX_3_0_JSON,
  NONE,
};
```

`Source/cmSbomArguments.h:20-24`. Format names accepted: `spdx`, `spdx-3.0+json`,
`spdx-3.0.1+json` (`Source/cmSbomArguments.cxx:20-22`). The docs state the
interface is designed to allow additional formats
(`Help/command/install.rst:1335-1337`).

## Recommended integration point (if pursued)

Add `PROTOBOM` to `cmSbomArguments::SbomFormat` and a `cmProtobomSerializer`
implementing `cmSbomSerializer` (`Source/cmSbomSerializer.h`). The SPDX model in
`Source/cmSpdx.h` is already format-neutral enough to drive it; the serializer
interface is the seam the CMake authors left for exactly this.

## NTIA fields not obtainable from the current graph

- **Supplier**: not modelled per target. Only available when a CPS package or
  `find_package` config supplies it.
- **Hash**: not tracked per target; would require hashing installed artifacts.
- **Unique identifier**: purl is only present when the project passes
  `PACKAGE_URL` to `install(SBOM)`; otherwise absent.
- **License**: only the project-level `DEFAULT_LICENSE` and CPS-declared
  licenses; per-target license is not a CMake concept.

## Why skipped

Per the repo workflow: "If recon shows the repo already has SBOM support, skip
and note why in `STATUS.md`." CMake satisfies the regulatory requirement today
with SPDX 3.0 JSON. The remaining gap is format, not capability.
