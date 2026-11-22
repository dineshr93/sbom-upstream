# Go design

## Recommendation first

Not a skip. `BuildInfo` (`src/runtime/debug/mod.go:41`) is an SBOM precursor but not
an SBOM: no author, no timestamp, no purl, no license, and nothing labels it as one.
Unlike Cargo (which has an actual `Sbom{}` schema behind `-Zsbom`), Go has zero SBOM
output, so there is a real feature gap, not a format gap.

Two accepted constraints:

1. Format is protobom protobuf, written by a hand-rolled proto3 encoder (~60 LOC).
   `src/cmd/go.mod` vendors no protobuf; adding one to `cmd/go` is unacceptable.
2. Contribution flow is Gerrit, not a GitHub PR. The `pr/go.md` step becomes a Gerrit
   commit message; same approval rule applies.

## In-repo design

`go build -sbom` — a bool flag on the existing build command, no value, no argument
parsing ambiguity (matches `-json` at `src/cmd/go/internal/work/build.go:349`).

Scope, smallest first:

- `src/cmd/go/internal/work/build.go` — register `sbom` bool flag next to the other
  build flags (`:250-349`), stored on `cfg`.
- `src/cmd/go/internal/load/pkg.go:2348` — `setBuildInfo` already produces the exact
  `debug.BuildInfo` (main + all deps + sums). Reuse it; zero new resolution. The
  build path already calls it for main packages.
- New `src/cmd/go/internal/work/sbom.go` (~120 LOC) — hand-rolled proto3 writer for
  the protobom `Document`, plus the mapping from `debug.BuildInfo` to nodes/edges.
- Output: `sbom.protobom.bin` written next to the built executable (directory of the
  `-o` target; cwd when `-o` is absent). Fixed name, no `--sbom=<path>` ambiguity.

Total ~150 LOC, under the 500 budget.

## Format

protobom protobuf `Document` via two wire types only (varint, length-delimited),
same encoder recipe as the C emitter. Nodes sorted by id, edges by `(from, to)`,
map entries by key.

## Field mapping

| NTIA element | Source | Status |
|---|---|---|
| name | `debug.Module.Path` | available |
| version | `debug.Module.Version` (`(devel)` fallback already in `setBuildInfo`) | available |
| purl | `pkg:golang/<Path>@<Version>` | synthesised |
| relationships | main module `dependsOn` each `Deps` entry (Edge type 10) | available |
| author | `"go build"` as org Person | available |
| timestamp | `SOURCE_DATE_EPOCH`, else `vcs.time` build setting, else now | available |
| supplier | none in `BuildInfo` | omitted |
| license | none in `BuildInfo` (grep: zero `License` in `cmd/go/internal`) | omitted |
| hash | `Module.Sum` is `h1:` (dirhash of module zip), not a standard protobom hash algorithm | omitted |

`h1:` is deliberately omitted rather than mapped to `sha256=3`: it is a SHA-256-based
dirhash of the module zip, not a file hash, and claiming `sha256` for it would be a
fabrication. Gap documented instead.

## Opt-in

Nothing runs unless `-sbom` is passed. No cost in the default build.

## Open questions

1. `go build -sbom` vs `go mod sbom`? The data lives in the main package's
   `BuildInfo`, which only `go build` produces; a `go mod` subcommand would have to
   re-derive it. Build flag wins.
2. `go test -sbom` too? `load/test.go:294` already carries `BuildInfo` into test
   binaries. Free after the build path exists, but out of scope for the first PR.
3. Gerrit flow means the GitHub PR step of this repo's workflow does not apply;
   confirm the commit-message-only deliverable is acceptable.
