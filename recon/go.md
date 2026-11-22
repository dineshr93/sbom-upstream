# Go recon

Clone: `golang/go` (depth 1), commit `42452e65`, 242M. License BSD-3-Clause
(`LICENSE:1` "Copyright 2009 The Go Authors"). No `AGENTS.md` at root or in
`.github/`; `CONTRIBUTING.md` exists and points at the Gerrit flow
(go.dev/doc/contribute), not GitHub PRs.

## 1. Does Go already have SBOM?

No. `grep -rli 'sbom|cyclonedx'` over `src/cmd/go` returns nothing; the only
`sbom` hits in all of `src/` are false positives (`cmd/internal/obj/x86/anames.go`,
`crypto/tls/certificates_test.go`). `spdx` has zero hits in `cmd/go`.

## 2. Where is the resolved dependency graph?

Two graphs:

- Module build list: `src/cmd/go/internal/modload/buildlist.go:107`
  (`newRequirements`), `src/cmd/go/internal/mvs/mvs.go:90` (`BuildList`).
- Per-binary module set actually used by the build: `src/cmd/go/internal/load/pkg.go:2348`
  (`func (p *Package) setBuildInfo`) — BFS over `p.Internal.Imports`, collects one
  `debug.Module` per module; checksum via `modfetch.Sum` (`pkg.go:2369`,
  `src/cmd/go/internal/modfetch/fetch.go:1005`). The linker embeds this blob:
  `src/cmd/link/internal/ld/data.go:2633` (`func (ctxt *Link) buildinfo()`),
  symbol `go:buildinfo` at `:2637`.

## 3. What serialization exists?

`BuildInfo` is already serialized twice: the binary blob embedded in every binary
(`ld/data.go:2633`) and JSON via `go version -json -m`
(`src/cmd/go/internal/version/version.go:44`, `:59`, `:187` `json.MarshalIndent`).
This is an SBOM precursor in all but name — but it has no author, no timestamp, no
purl, no license, and is not labelled or consumable as an SBOM.

## 4. CLI flag convention

Per-command `flag.FlagSet`, no global flags. `go build` flags registered in
`src/cmd/go/internal/work/build.go:250-349` (`-o` at `:250`, `-json` at `:349`).
A `go build -sbom` bool flag fits the existing pattern exactly.

## 5. Tests

Script tests in `src/cmd/go/testdata/script/` (e.g. `test_buildinfo.txt`) driven by
`script_test.go`; `go_test.go` (75K) holds Go tests.

## 6. Metadata per component

`src/runtime/debug/mod.go:41` `BuildInfo{GoVersion, Path, Main, Deps, Settings}`;
`:62` `Module{Path, Version, Sum, Replace}`.

- name/version: present.
- hash: `Sum` present (`h1:` scheme — see design, not a standard protobom algorithm).
- license: absent — zero `License` hits in `cmd/go/internal` (grep).
- supplier: absent.
- purl: absent, must synthesise `pkg:golang/<path>@<version>`.

## 7. Dependency policy

`src/cmd/go.mod` module `cmd`, go 1.28. Vendored deps only: `x/arch`, `x/build`,
`x/mod`, `x/sync`, `x/sys`, `x/telemetry`, `x/term`, `x/text`, `x/tools`, `pprof`,
`demangle`, `markdown`. No protobuf anywhere. A protobom writer must be a hand-rolled
proto3 encoder, no new dependency.

## 8. License / contribution

BSD-3-Clause, compatible with protobom (Apache-2.0). Contributions go through Gerrit
(`CONTRIBUTING.md`), not GitHub PRs — affects the PR step.
