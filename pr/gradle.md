# PR: gradle

PR: https://github.com/gradle/gradle/pull/39928 (open)

Title: `Add opt-in SBOM generation (generateSbom task)`

Branch: `sbom-support`, commit `04bf0537`, 356 insertions across 5 files.

---

## Motivation

Regulations (NTIA minimum elements, EO 14028, EU CRA) require an SBOM. Gradle
already resolves the full dependency graph; the `generateSbom` task emits it as
a protobom document so downstream tooling needs no plugin or scanner.

## Design

- `SbomReportTask.java` (new, 250 LOC) sits next to `DependencyReportTask` in
  `software-diagnostics` and contains a hand-rolled proto3 wire encoder
  (varint + length-delimited only) — Gradle has no protobuf runtime anywhere,
  and adding `protobuf-java` to the distribution for an opt-in task would
  violate "zero new mandatory runtime deps".
- Registered by `SoftwareReportingTasksPlugin`, which is auto-applied to every
  project — the task is built-in, no plugin to apply, registered lazily so an
  unrelated build pays nothing.
- Default scope: every resolvable, non-deprecated configuration, reusing the
  exact filter `ConfigurationDetails.of()` applies; `--configuration` reuses
  `AbstractDependencyReportTask`'s `ConfigurationFinder` pattern.
- Data: `ResolutionResult.getAllDependencies()` → `ResolvedDependencyResult`;
  node id = `ComponentIdentifier.getDisplayName()`; purl synthesized as
  `pkg:maven/<group>/<module>@<version>` only where all three parts exist.

## Alternatives considered

- `protobuf-java` dependency: rejected, ~500 KB to every distribution for an
  opt-in task.
- Separate plugin: rejected, the data lives in the project Gradle already
  resolves; a built-in task matches `dependencies`/`buildEnvironment`.
- SPDX/CycloneDX JSON: rejected; protobom wire format achieved with zero
  dependency instead.

## Test plan

- `SbomReportTaskIntegrationTest.groovy`: project with a `mavenLocal()`
  dependency, `gradle generateSbom`, asserts the file exists, is non-empty,
  and contains the dependency's display name and the `Gradle` tool entry.
- Determinism: two runs with `SOURCE_DATE_EPOCH=1700000000` are byte-identical.
- `--configuration` restricts the document.

## Backwards compatibility

- New task only; no existing task, plugin or resolution behaviour changes.
- No new dependency. `@Incubating` on the new public task type per Gradle API
  policy. Constants added to `DiagnosticsTaskNames`/`HelpTasksPlugin` are
  additive.

## Gaps (documented, never fabricated)

- supplier: not on `ComponentIdentifier` — omitted.
- license: not on the resolution result — omitted.
- hash: not retained on the resolved graph — omitted.
- purl for project components: omitted, no version to put in it.
