# Recon: Bazel

Repo: https://github.com/bazelbuild/bazel
Clone: `/home/dinesh/.hermes/cache/scratch/bazel`, commit `227605f`.
License: Apache-2.0 (`LICENSE:2`). Compatible with protobom (Apache-2.0).

Verdict: **no SBOM support in-tree**. Proceed to design.

## 1. Where is the resolved component graph built and stored?

Bazel has no single "component graph" object. The resolved graph is the Skyframe
graph: each `ConfiguredTargetKey` node evaluates to a `ConfiguredTargetValue`
holding a `ConfiguredTarget`, and its Skyframe direct deps are the configured
targets it depends on.

| What | Where |
| --- | --- |
| Top-level configured targets after analysis | `AnalysisResult.getTargetsToBuild()` — `src/main/java/com/google/devtools/build/lib/analysis/AnalysisResult.java:84` |
| Top-level targets after execution (both Skymeld and non-Skymeld) | `BuildResult.getActualTargets()` — `src/main/java/com/google/devtools/build/lib/buildtool/BuildResult.java:193` |
| Configured target value | `ConfiguredTargetValue.getConfiguredTarget()` — `src/main/java/com/google/devtools/build/lib/analysis/ConfiguredTargetValue.java:25` |
| Graph edges | `WalkableGraph.getDirectDeps(SkyKey)` — `src/main/java/com/google/devtools/build/skyframe/WalkableGraph.java:89` |
| Graph accessor | `SkyframeExecutor.getWalkableGraph()` — `src/main/java/com/google/devtools/build/lib/skyframe/SkyframeExecutor.java:2790` |
| Key from a configured target | `ConfiguredTargetKey.fromConfiguredTarget(target)` — `src/main/java/com/google/devtools/build/lib/query2/cquery/ConfiguredTargetQueryEnvironment.java:364` |
| Value from a key | `ConfiguredTargetQueryEnvironment.getValueFromKey` — `src/main/java/com/google/devtools/build/lib/query2/cquery/ConfiguredTargetQueryEnvironment.java:326-330` |
| Label of a configured target | `TransitiveInfoCollection.getLabel()` — `src/main/java/com/google/devtools/build/lib/analysis/TransitiveInfoCollection.java:44` |

This is exactly how `cquery` walks the graph, so the traversal is a known-good
pattern rather than a new one.

## 2. What metadata is available per component?

| Field | Available? | Source |
| --- | --- | --- |
| name | yes | `Label.getName()` — `src/main/java/com/google/devtools/build/lib/cmdline/Label.java:449` |
| unique id | yes | `Label.getCanonicalForm()` — `src/main/java/com/google/devtools/build/lib/cmdline/Label.java:472` |
| version | **partly** | bzlmod module version only: `BazelDepGraphValue.getDepGraph()` → `ModuleKey.version()` — `src/main/java/com/google/devtools/build/lib/bazel/bzlmod/BazelDepGraphValue.java:85`, `src/main/java/com/google/devtools/build/lib/bazel/bzlmod/ModuleKey.java:34` |
| repo → module mapping | yes | `BazelDepGraphValue.getCanonicalRepoNameLookup()` — `src/main/java/com/google/devtools/build/lib/bazel/bzlmod/BazelDepGraphValue.java:88`; target repo via `Label.getRepository()` — `src/main/java/com/google/devtools/build/lib/cmdline/Label.java:341` |
| supplier | **no** | no such concept anywhere in `src/main/java/com/google/devtools/build/lib/analysis` or `.../rules` |
| license | **no** | no license provider in Bazel core; `rules_license` is an external repo, not a Bazel dependency |
| hash | **no** | artifacts exist only after execution; no per-component digest is modelled |
| purl/CPE | **no** | must be synthesized from name + version |

## 3. Existing output/serialization mechanisms

| Mechanism | Where | Cost to extend |
| --- | --- | --- |
| Post-build Skyframe dump to a file | `BuildTool.dumpSkyframeStateAfterBuild` — `src/main/java/com/google/devtools/build/lib/buildtool/BuildTool.java:897`; flag `--experimental_aquery_dump_after_build_output_file` — `src/main/java/com/google/devtools/build/lib/buildtool/BuildRequestOptions.java:609` | **cheapest** — same shape: a flag, a path, a write after the build |
| Analysis post-processor hook | `BuildTool.AnalysisPostProcessor` — `src/main/java/com/google/devtools/build/lib/buildtool/BuildTool.java:168`; called at `:665` | used by cquery/aquery; runs before execution |
| Build event protocol | `BuildEventProtocolOptions` — `src/main/java/com/google/devtools/build/lib/runtime/commands/BuildCommand.java:58` | wrong channel: BEP is a stream, not a file |
| Starlark rule | `tools/build_defs/` | cannot see the whole build graph, only its own deps |

Recommended: a flag on `build` plus a write after `BuildTool.processRequest`
returns, mirroring the aquery-dump-after-build pattern.

## 4. Where do CLI flags get parsed?

| What | Where |
| --- | --- |
| `build` command option list | `src/main/java/com/google/devtools/build/lib/runtime/commands/BuildCommand.java:49-60` |
| Build options class | `BuildRequestOptions` — `src/main/java/com/google/devtools/build/lib/buildtool/BuildRequestOptions.java:48` |
| Option pattern | `@Option(name = ..., defaultValue = ..., documentationCategory = ..., effectTags = ..., converter = ..., help = ...)` then an abstract getter — e.g. `src/main/java/com/google/devtools/build/lib/buildtool/BuildRequestOptions.java:608-625` |
| Path converter | `OptionsUtils.PathFragmentConverter.class` — `src/main/java/com/google/devtools/build/lib/buildtool/BuildRequestOptions.java:613` |

## 5. Test framework and fixtures

| Kind | Where |
| --- | --- |
| Shell integration tests | `src/test/shell/integration/*.sh`, registered as `sh_test` in `src/test/shell/integration/BUILD` (e.g. `aquery_test` at `:68-78`) |
| Post-build file test precedent | `test_dump_skyframe_state_after_build_to_specified_file` — `src/test/shell/integration/aquery_test.sh:1589-1611` |
| Java integration tests | `BuildIntegrationTestCase` — `src/test/java/com/google/devtools/build/lib/buildtool/util/BuildIntegrationTestCase.java` |
| Java unit tests | `src/test/java/com/google/devtools/build/lib/...` |

An SBOM test belongs in `src/test/shell/integration/sbom_test.sh`, following the
aquery-dump test: write a small `BUILD`, run `bazel build --sbom_output=...`,
then decode the file with `protoc`.

## 6. Existing SBOM/SPDX/CycloneDX code?

None. `grep -rniI "sbom\|cyclonedx" src docs tools` returns nothing.
`grep -rniI "spdx"` returns only two roadmap docs
(`docs/versions/7.7.3/about/roadmap.mdx`, `docs/versions/7.6.1/about/roadmap.mdx`).
`LicenseCommand` (`src/main/java/com/google/devtools/build/lib/runtime/commands/LicenseCommand.java:43`)
prints Bazel's own license text, not a component SBOM.

## 7. Language and dependency policy

Java (server) + C++ (client). Java deps are declared per-package in `BUILD`
files. Protobuf is already a first-class dependency:
`//third_party/java/protobuf` is in the `commands` target's deps —
`src/main/java/com/google/devtools/build/lib/runtime/commands/BUILD:187`.
So `com.google.protobuf.CodedOutputStream` is available without adding a
dependency; no hand-rolled varint encoder is needed.

`AGENTS.md` (`AGENTS.md:1-72`) sets build/test/lint conventions but contains no
AI-authorship ban. Google Java Style applies (`AGENTS.md:71`).

## 8. License compatibility

Bazel: Apache-2.0 (`LICENSE:2`). protobom: Apache-2.0. Compatible.

## Recommended integration point

`bazel build --sbom_output=<path>`:

1. New `SbomOptions` class with one `@Option` (`--sbom_output`, `PathFragment`,
   default `null`), added to `BuildCommand`'s `options` list.
2. New package `com.google.devtools.build.lib.sbom` with `SbomWriter`, which
   takes the `BuildResult` top-level targets, walks the Skyframe graph via
   `SkyframeExecutor.getWalkableGraph()`, and writes a protobom `Document` with
   `CodedOutputStream`.
3. Call it from `BuildCommand.exec` after `BuildTool.processRequest` returns,
   so both the Skymeld and non-Skymeld paths are covered by one call site.

## NTIA fields not obtainable from the current graph

| Element | Status |
| --- | --- |
| supplier | **gap** — not modelled |
| version | **gap** for non-bzlmod targets; available for bzlmod modules |
| license | **gap** — not modelled in core |
| hash | **gap** — not an NTIA minimum element, so the SBOM still meets the minimum |

Obtainable: component name, unique identifier (label + synthesized purl where a
version is known), dependency relationships, author of SBOM data, timestamp.
