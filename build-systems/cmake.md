# CMake

- **Data source:** `install(EXPORT)` / `export()` target export sets, plus the
  `cmake_file_api` codemodel. `CPack` component metadata already carries
  supplier/version.
- **Flag:** `cmake --sbom` CLI flag plus a `CMAKE_SBOM` variable.
- **Output:** `file(GENERATE)` to write the document.
- **Format:** protobuf (protobom). CMake is C++, so a vendored minimal writer or
  a gated protobuf dependency is viable.
- **Gaps:** license and hash are not tracked per target by default; purl is not
  derivable for non-`find_package` components. Supplier comes from CPack
  metadata only when the project sets it.
