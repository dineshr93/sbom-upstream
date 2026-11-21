# Go

- **Data source:** the module graph (`go list -m -json all`) plus build info
  already embedded in binaries (`debug/buildinfo`).
- **Flag:** `go build -sbom` or a `go mod` subcommand.
- **Format:** protobuf (protobom). Go has native protobuf.
- **Gaps:** supplier is not in the module graph; license is not recorded;
  hashes are available from `go.sum` for module zips only.
