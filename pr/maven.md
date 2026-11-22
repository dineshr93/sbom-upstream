# PR: maven

Title: `Add opt-in SBOM generation (--sbom)`

Branch: `sbom-support`, commit `280635bc`, 181 insertions across 6 files.

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
- `SbomReportWriter.java` (new, 94 LOC) runs after the build request completes
  in `MavenInvoker`, unions `MavenProject.getArtifacts()` from
  `MavenSession.getProjects()` — zero new resolution, `ProjectArtifactsCache`
  already holds the set.
- Flag is no-arg; output path fixed to `<topDirectory>/sbom.json`.
- Format: protobom JSON via a hand-rolled encoder. maven-core has zero
  protobuf dependency and adding `protobuf-java` for one report is the change
  most likely to be rejected.

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
