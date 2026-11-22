# Recon: Meson

Repo: https://github.com/mesonbuild/meson
Commit inspected: `fe8b913` (2026-11-21)
License: Apache-2.0 (`COPYING:2`)

## Verdict: no SBOM support — proceed to design

`grep -rn "sbom\|SBOM\|spdx\|cyclonedx" mesonbuild docs unittests` returns
nothing. Meson has no SBOM, SPDX, or CycloneDX code path.

## Repo rules that constrain the PR (read first)

`AGENTS.md:10-54` forbids agents from opening/updating PRs, issues, or
discussions, and bans AI-written PR descriptions, commit messages, issue text,
and reviewer replies. It also requires the human to demonstrate understanding of
the change before submission. **The PR for this repo must be written and opened
by a human.** This recon/design work is fine; the PR step is not automatable.

## Findings

| # | Question | Answer | Evidence |
| --- | --- | --- | --- |
| 1 | Where is the resolved component graph built? | `build.Build.targets` holds every target; each `BuildTarget` keeps `external_deps` (resolved `Dependency` objects) and internal link deps | `mesonbuild/build.py:518`, `mesonbuild/build.py:642`, `mesonbuild/build.py:1065`, `mesonbuild/build.py:1743`, `mesonbuild/build.py:3648` |
| 2 | Per-component metadata available | target id, name, version, output filename; dependency name and version; project name/version; project license list | `mesonbuild/build.py:3642`, `mesonbuild/build.py:3783`, `mesonbuild/build.py:3752`, `mesonbuild/build.py:3639`, `mesonbuild/dependencies/base.py:232`, `mesonbuild/dependencies/base.py:235`, `mesonbuild/build.py:514-515`, `mesonbuild/build.py:400-403` |
| 3 | Existing serialization mechanisms | introspection JSON (`meson introspect`), pickled `InstallData` for the install step | `mesonbuild/mintro.py:515`, `mesonbuild/mintro.py:154`, `mesonbuild/mintro.py:53`, `mesonbuild/backend/backends.py:123` |
| 4 | CLI surface | subcommand dispatch in `mesonmain.py`; `meson setup` args in `msetup.py`; `meson compile` in `mcompile.py` | `mesonbuild/mesonmain.py:88`, `mesonbuild/msetup.py:52`, `mesonbuild/mcompile.py` |
| 5 | Test framework | `unittests/` (unittest, `BasePlatformTests` runs real projects), `run_project_tests.py` over `test cases/` | `unittests/baseplatformtests.py:60`, `unittests/allplatformstests.py:121`, `unittests/internaltests.py:67` |
| 6 | Existing SBOM code | none | grep above |
| 7 | Language / dependency policy | Python 3, standard library only, no runtime dependencies | `setup.py`, `pyproject.toml`, `mesonbuild/` imports |
| 8 | License compatibility | Apache-2.0; protobom is Apache-2.0 — compatible | `COPYING:2` |

## Integration point

`MesonApp.generate()` in `mesonbuild/msetup.py` already has both the fully
populated `build.Build` (`b`) and the `Interpreter` (`intr`) right after
`mintro.generate_introspection_file(b, intr.backend)` (`mesonbuild/msetup.py:311`).
That is the natural place to emit an SBOM: the graph is complete, no build has
run yet, and no backend-specific code is involved.

## NTIA fields not obtainable at configure time

- **Hash**: artifacts do not exist yet. Omit and document.
- **Supplier**: Meson does not model a supplier for targets or dependencies.
  Omit and document.
- **Unique identifier**: no purl concept. Synthesize `pkg:generic/<name>@<version>`
  for local targets; external dependencies have no purl — omit and document.
- **License**: only the project-level license list exists
  (`build.DepManifest.license`); per-target license is not a Meson concept.
- **Author of SBOM data**: Meson itself (tool name/version), which satisfies the
  element.

## Risk

Low. The change is additive: one new module, one new `meson setup` flag, one
test file. No existing code path changes behaviour when the flag is absent.
