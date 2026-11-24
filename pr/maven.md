# PR: maven

PR: https://github.com/apache/maven/pull/14093 (open)

Title: `Add opt-in SBOM generation (--sbom)`

Branch: `sbom-support`, commits `280635bc`, `fc81e18`, `2b36806` and `635b898`, 309 insertions across 7 files.

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
- `SbomReportWriter.java` (new, 148 LOC) runs after the build request completes
  in `MavenInvoker`, only when `!result.hasExceptions()`, walks
  `MavenExecutionResult.getTopologicallySortedProjects()` and emits each project
  and resolved artifact as a CycloneDX component with a dependency edge — zero
  new resolution, `ProjectArtifactsCache` already holds the set.
- Flag is no-arg; output path fixed to `<topDirectory>/sbom.json`.
- Format: CycloneDX 1.6 JSON via a hand-rolled encoder: `bomFormat`,
  `specVersion`, `version`, `metadata`, sorted `components` with `group`, `name`,
  `version`, `purl`, and sorted `dependencies` with `ref` / `dependsOn`.
  maven-core has zero protobuf or JSON dependency; adding one for a report is the
  change most likely to be rejected.
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
  option is present; existing parse tests unaffected.
- `SbomReportWriterTest.java`: invokes `SbomReportWriter.write()` with a minimal
  `MavenExecutionResult`, asserts CycloneDX `bomFormat`, component `group`, `name`,
  root `ref`, `dependsOn` edge, and an empty `dependsOn` entry for a leaf component.
- Local Maven 4.2.0-SNAPSHOT: `mvn -pl impl/maven-cli -am -Dtest=CommonsCliMavenOptionsSbomTest,SbomReportWriterTest -Dsurefire.failIfNoSpecifiedTests=false test` passes 3/3.

## Backwards compatibility

- Nothing runs unless `--sbom` is passed; no lifecycle binding, no default-build
  cost, no new dependency.

## Review comments

- SBOM write before exception check: guarded by `!result.hasExceptions()` in
  `MavenInvoker`.
- `protobom-json` not a real format: switched to CycloneDX 1.6 JSON.
- `name` drops `groupId`: components now emit `group` and `name` separately.
- Incomplete JSON escaping: control characters U+0000–U+001F are escaped.
- Javadoc said "file name": now says path relative to the top-level project
  directory.
- No writer test: `SbomReportWriterTest` added.
- Components with no dependencies: every component now has an entry in
  `dependencies[]`, with an empty `dependsOn` for leaf components.
- Deprecated `tools` array: switched to the CycloneDX 1.6 object form,
  `{"components": [{"type": "application", "name": "mvn"}]}`.
  Final commit `635b898`.

## Gaps (documented, never fabricated)

- supplier: not on `Artifact` — omitted.
- license: not on `Artifact` — omitted.
- hash: not retained in the resolved set — omitted.
