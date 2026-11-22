# PR: maven

PR: https://github.com/apache/maven/pull/14093 (open)

Title: `Add opt-in SBOM generation (--sbom)`

Branch: `sbom-support`, commits `280635bc` and `fc81e18`, 221 insertions across 6 files.

---

## Motivation

Regulations (NTIA minimum elements, EO 14028, EU CRA) require an SBOM. Maven
already resolves every artifact in the session; `mvn --sbom` writes it out so
no external plugin or scanner is needed.

## Design

- A flag on the existing `mvn` invoker, not a new invoker: `sbomFile()` on
  `MavenOptions`, the `Option` on the mvn-specific `CommonsCliMavenOptions`
  CLIManager (not the shared one, so a following goal is not swallowed), the
  accessor wired through `LayeredMavenOptions`.
- `SbomReportWriter.java` (new, 134 LOC) runs after the build request completes
  in `MavenInvoker`, walks `MavenExecutionResult.getTopologicallySortedProjects()`
  and emits the project node plus each resolved artifact as a dependency edge —
  zero new resolution, `ProjectArtifactsCache` already holds the set.
- Flag is no-arg; output path fixed to `<topDirectory>/sbom.json`.
- Format: protobom-style JSON via a hand-rolled encoder, with `nodeList.nodes`,
  sorted `dependsOn` edges and `rootElements`. maven-core has zero
  protobuf dependency and adding `protobuf-java` for one report is the change
  most likely to be rejected.
- `SOURCE_DATE_EPOCH` is honoured; an invalid value falls back to the current
  timestamp instead of throwing.

## Alternatives considered

- A real `sbom` goal: belongs in the separate `maven-plugins` repo (core binds
  only external plugins, `JarLifecycleMappingProvider.java:33-44`); the
  invoker-flag fallback is used here per design.
- `mvnsbom` invoker (mvnenc/mvnup precedent): more boilerplate for no gain,
  the data lives in the session `mvn` already builds.
- Protobuf binary: ~60 extra LOC with no dependency, but JSON first to keep
  the diff minimal.

## Test plan

- `CommonsCliMavenOptionsSbomTest.java`: parses `--sbom` and asserts the
  option is present; existing parse tests unaffected (2/2 pass).
- `mvn test-compile` passes.

## Backwards compatibility

- Nothing runs unless `--sbom` is passed; no lifecycle binding, no default-build
  cost, no new dependency.

## Gaps (documented, never fabricated)

- supplier: not on `Artifact` — omitted.
- license: not on `Artifact` — omitted.
- hash: not retained in the resolved set — omitted.
