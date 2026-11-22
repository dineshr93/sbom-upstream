# Design: Bazel

Target: `bazel build --sbom_output=<path>` writes a protobom `Document`
describing the configured targets reachable from the requested targets.
Opt-in, off by default, no new dependency.

## 1. File-by-file change list

| File | Change | Rationale |
| --- | --- | --- |
| `src/main/java/com/google/devtools/build/lib/sbom/SbomOptions.java` (new, ~25 LOC) | `@OptionsClass` with one `@Option(name = "sbom_output", converter = OptionsUtils.PathFragmentConverter.class)` and `@Nullable PathFragment getSbomOutput()` | Bazel's option convention: one small options class per feature, added to the command's `options` list (`src/main/java/com/google/devtools/build/lib/runtime/KeepGoingOption.java:24-35`). |
| `src/main/java/com/google/devtools/build/lib/sbom/SbomWriter.java` (new, ~190 LOC) | Walks the Skyframe graph from the top-level targets and writes the protobom document with `CodedOutputStream` | Protobuf is already a dependency of the `commands` target (`src/main/java/com/google/devtools/build/lib/runtime/commands/BUILD:187`), so no encoder needs to be hand-rolled and no `.proto` needs to be added. |
| `src/main/java/com/google/devtools/build/lib/sbom/BUILD` (new, ~25 LOC) | `java_library` with deps on `//src/main/java/com/google/devtools/build/lib/analysis`, `.../skyframe`, `.../cmdline`, `.../bazel/bzlmod`, `//third_party/java/protobuf` | Bazel declares deps per package. |
| `src/main/java/com/google/devtools/build/lib/runtime/commands/BuildCommand.java` (+12 LOC) | add `SbomOptions.class` to `@Command(options = {...})` (`:49-60`); after `BuildTool.processRequest` returns (`:106-107`), call `SbomWriter.write(env, result, options)` when the flag is set | One call site covers both the Skymeld and non-Skymeld paths, because `BuildResult.getActualTargets()` is populated in both (`src/main/java/com/google/devtools/build/lib/buildtool/BuildTool.java:661`, `:801`). |
| `src/test/shell/integration/sbom_test.sh` (new, ~70 LOC) | integration test: build a small graph, decode the output with `protoc`, assert fields; determinism; off-by-default | Mirrors `test_dump_skyframe_state_after_build_to_specified_file` (`src/test/shell/integration/aquery_test.sh:1589-1611`). |
| `src/test/shell/integration/BUILD` (+8 LOC) | `sh_test(name = "sbom_test", ...)` | Same shape as `aquery_test` (`src/test/shell/integration/BUILD:68-78`). |

Net new: ~330 LOC, under the 500 LOC budget.

## 2. Data mapping

| Build-system field | protobom field | NTIA element |
| --- | --- | --- |
| `"bazel"` | `Document.metadata.name` | — |
| `SOURCE_DATE_EPOCH` or `System.currentTimeMillis()` | `Metadata.date.seconds` | timestamp |
| `BlazeVersionInfo.instance().getVersion()` (`src/main/java/com/google/devtools/build/lib/analysis/BlazeVersionInfo.java:138`) | `Metadata.tools[0].version` | author of SBOM data |
| `"Bazel"`, `is_org = true` | `Metadata.authors[0]` | author of SBOM data |
| `Label.getCanonicalForm()` (`src/main/java/com/google/devtools/build/lib/cmdline/Label.java:472`) | `Node.id` | unique identifier |
| `Label.getName()` (`src/main/java/com/google/devtools/build/lib/cmdline/Label.java:449`) | `Node.name` | component name |
| `ModuleKey.version()` for the target's repo (`src/main/java/com/google/devtools/build/lib/bazel/bzlmod/ModuleKey.java:34`) | `Node.version` | version |
| synthesized `pkg:generic/<name>@<version>` | `Node.identifiers[PURL=1]` | unique identifier |
| `WalkableGraph.getDirectDeps(key)` filtered to `ConfiguredTargetKey` (`src/main/java/com/google/devtools/build/skyframe/WalkableGraph.java:89`) | `Edge{type=dependsOn, from, to}` | dependency relationships |
| top-level target ids | `NodeList.root_elements` | — |
| — (not modelled) | `Node.suppliers` omitted | supplier — **gap** |
| — (not modelled in core) | `Node.licenses` omitted | license — **gap** |
| — (not modelled) | `Node.hashes` omitted | hash — **gap** |

Node id rule: the canonical label form, e.g. `//foo:bar` or `@@rules_cc+//cc:defs`.
That is unique per configured target and stable across builds.

Version rule: a target's repo is mapped through
`BazelDepGraphValue.getCanonicalRepoNameLookup()` (`src/main/java/com/google/devtools/build/lib/bazel/bzlmod/BazelDepGraphValue.java:88`)
to a `ModuleKey`. If found, the module version is used. Otherwise the version
and purl are omitted — never fabricated.

Scope: every configured target reachable from the requested targets, including
toolchain and implicit deps. Bazel cannot cheaply distinguish targets that
contribute to the final artifact from those that do not, so the document
includes all resolved components and the docs say so.

## 3. Failure modes

| Condition | Behaviour |
| --- | --- |
| Target has no bzlmod module (e.g. `//foo:bar` in the main repo) | `Node.version` and `Node.identifiers` omitted; node still present with id and name |
| No supplier | field omitted; documented gap |
| No license | field omitted; documented gap |
| No hash | field omitted; documented gap (hash is not an NTIA minimum element) |
| Graph node cleared or missing | node skipped, one `Event.warn` per skipped label |
| Output path not writable | `Event.error` and a failed build; the user asked for the file, so silently succeeding would be wrong |
| `--sbom_output` not set | nothing happens; no file, no graph walk |

## 4. Test plan

1. **Integration** (`src/test/shell/integration/sbom_test.sh`): a `genrule`
   with a `srcs` dep, `bazel build --sbom_output=$TEST_TMPDIR/sbom.bin`, then
   `protoc --decode=protobom.protobom.Document` and assert the metadata name,
   the tool version, both node ids, and the `dependsOn` edge.
2. **Determinism**: run twice with `SOURCE_DATE_EPOCH=1700000000` and compare
   the two files byte for byte.
3. **Off by default**: `bazel build` without the flag writes no file.
4. **Golden bytes**: the encoder is deterministic (nodes sorted by id, edges
   sorted by `(from, to)`), so the decoded textproto is the golden artifact;
   no separate golden binary is checked in.

## 5. Backwards compatibility

- Flag is off by default; no behaviour change for existing builds.
- No new dependency: `com.google.protobuf.CodedOutputStream` is already linked
  into the `commands` target.
- No new Skyframe node, no change to analysis or execution.
- The graph walk is read-only and runs after the build completes.
- `--sbom_output` is a new flag name; if maintainers prefer the experimental
  prefix, renaming to `--experimental_sbom_output` is a one-line change.

## 6. Open question for the maintainer

Should the flag be `--sbom_output` or `--experimental_sbom_output`? Bazel
convention is to prefix new, not-yet-stable flags with `experimental_`. This
design uses `--sbom_output` because the feature is opt-in and read-only.
