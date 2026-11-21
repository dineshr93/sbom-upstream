# Design: Meson

Target: `meson setup --sbom` emits a protobom Document describing the
configured project to `<builddir>/sbom.protobom.bin`. Opt-in, off by default, no
new dependency.

Note: the flag is a plain boolean, not `--sbom [<path>]`. An optional-value
argument is ambiguous with the positional `<builddir>` (`argparse` consumes the
build directory as the flag's value), so the output path is fixed.

## 1. File-by-file change list

| File | Change | Rationale |
| --- | --- | --- |
| `mesonbuild/sbom.py` (new, ~230 LOC) | proto3 wire encoder (varint + length-delimited only) and protobom document builder | Meson is pure Python and ships no protobuf runtime. A hand-rolled encoder is ~40 LOC and avoids vendoring `google.protobuf`. |
| `mesonbuild/msetup.py` (+8 LOC) | `--sbom` boolean argument in `add_arguments()` (`mesonbuild/msetup.py:52`); call `sbom.emit(b, path)` after `mintro.generate_introspection_file(b, intr.backend)` (`mesonbuild/msetup.py:311`) | The graph is complete at that point and no backend code is involved. |
| `unittests/sbomtests.py` (new, ~70 LOC) | golden-bytes unit test plus one `BasePlatformTests` integration test | Deterministic output makes a byte-exact golden test the cheapest real check. |
| `docs/markdown/Commands.md` (+15 LOC) | document `--sbom` under `meson setup` | Meson documents setup flags there (`docs/markdown/Commands.md`). |
| `docs/markdown/snippets/sbom.md` (new, ~10 LOC) | release-note snippet | Project convention for new features (`docs/genrelnotes.py:70`). |

Net new: ~325 LOC, under the 500 LOC budget.

## 2. Data mapping

| Meson field | protobom field | NTIA element |
| --- | --- | --- |
| `build.project_name` (`mesonbuild/build.py:514`) | `Document.metadata.name` | — |
| `coredata.version` | `Metadata.tools[0].version` (name `meson`) | author of SBOM data |
| `SOURCE_DATE_EPOCH` or `time.time()` | `Metadata.date.seconds` | timestamp |
| `Target.get_id()` (`mesonbuild/build.py:3642`) | `Node.id` | unique identifier |
| `Target.get_name()` (`mesonbuild/build.py:3783`) | `Node.name` | component name |
| `build.project_version` (`mesonbuild/build.py:515`) | `Node.version` | version |
| synthesized `pkg:generic/<name>@<version>` | `Node.identifiers[PURL]` | unique identifier |
| `Dependency.get_name()` / `get_version()` (`mesonbuild/dependencies/base.py:232`, `:235`) | `Node.name` / `Node.version` | component name, version |
| `Target.get_all_link_deps()` (`mesonbuild/build.py:3648`) and `get_external_deps()` (`mesonbuild/build.py:1743`) | `Edge{type=dependsOn, from, to}` | dependency relationships |
| — (not modelled) | `Node.suppliers` omitted | supplier — **gap** |
| — (artifacts not built yet) | `Node.hashes` omitted | hash — **gap** |
| — (no root node) | `Node.licenses` omitted | license — **gap** |

Node id rule: local targets use `Target.get_id()` (path-qualified, unique);
external dependencies use the purl when a version is known, else `name@version`,
else `name`.

## 3. Failure modes

| Condition | Degradation |
| --- | --- |
| Target has no version | `Node.version` omitted; name and id still present. Documented in `Commands.md`. |
| Dependency has no version | same. |
| No purl derivable | `Node.identifiers` omitted; `Node.id` still unique. |
| No supplier | `Node.suppliers` omitted for every node; documented as an NTIA gap. |
| No hash | `Node.hashes` omitted; documented as an NTIA gap (hash is not an NTIA minimum element). |
| Duplicate ids (two targets, same name/version) | dedupe by id, union edges. |
| `CustomTarget` | included as a node with name and id, no version. |
| Write failure | `MesonException` with the path; setup fails loudly rather than silently skipping the SBOM. |

Never fabricate a field. Every omission is listed in the docs.

## 4. Test plan

1. **Unit (golden bytes)**: build a fixed in-memory graph (2 targets, 1 external
   dep, 1 edge), fixed timestamp, assert the serialized bytes equal a checked-in
   hex string. Fails if field numbers, ordering, or varint encoding drift.
2. **Integration**: `BasePlatformTests` project with two targets and one
   dependency; run `meson setup --sbom out.bin`; assert the file exists and
   `protoc --decode=protobom.protobom.Document` succeeds when `protoc` is on
   `PATH` (skip otherwise).
3. **Determinism**: run setup twice with the same `SOURCE_DATE_EPOCH`, assert
   byte equality.
4. **Off by default**: existing tests unchanged; no file written without the flag.

## 5. Backwards compatibility

- Flag absent: no new code path runs, no file written, no import cost beyond the
  module import.
- No new runtime dependency; `mesonbuild/sbom.py` imports only `struct`, `time`,
  `os`, and `mesonbuild` internals.
- No change to `install.dat`, introspection JSON, or backend output.
- Python 3.7+ syntax consistent with the rest of `mesonbuild/`.

## Open question for the maintainer

Should the SBOM be emitted at configure time (graph only, no hashes) or at
install time (hashes available, but `InstallData` does not carry the dependency
graph)? This design picks configure time because it meets the NTIA minimum
elements and needs no changes to the install pipeline. If maintainers want
hashes, the follow-up is a second emission point in `minstall.py` that reuses
the same builder and fills `Node.hashes` from installed files.
