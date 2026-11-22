# PR: gradle

PR: https://github.com/gradle/gradle/pull/39928 (open)

Title: `Add opt-in SBOM generation (sbom task)`

Branch: `sbom-support`, commits `04bf0537` and `925ef93`, 356 insertions across 5 files.

---

## Motivation

Regulations (NTIA minimum elements, EO 14028, EU CRA) require an SBOM. Gradle
already resolves the full dependency graph; the `sbom` task emits it as a
protobom-style JSON document so downstream tooling needs no plugin or scanner.

## Design

- `SbomReportTask.java` (new, 250 LOC) sits next to `DependencyReportTask` in
  `software-diagnostics` and contains a hand-rolled JSON writer. Gradle has no
  protobuf runtime, and adding `protobuf-java` to the distribution for an
  opt-in task would violate "zero new mandatory runtime deps".
- Registered by `SoftwareReportingTasksPlugin`, which is auto-applied to every
  project — the task is built-in, no plugin to apply, registered lazily so an
  unrelated build pays nothing.
- Default scope: `runtimeClasspath`; `--configuration` reuses
  `ConfigurationFinder.find` to select one configuration.
- Data: `configurations.getIncoming().getResolutionResult().getRootComponent()`
  plus its `ResolvedDependencyResult`s; node id = `ComponentIdentifier.getDisplayName()`;
  purl synthesized as `pkg:maven/<group>/<module>@<version>` only where all
  three parts exist.
- Deterministic: nodes and edges are `TreeMap`/`TreeSet`, roots are emitted in
  sorted order.

## Alternatives considered

- `protobuf-java` dependency: rejected, ~500 KB to every distribution for an
  opt-in task.
- Separate plugin: rejected, the data lives in the project Gradle already
  resolves; a built-in task matches `dependencies`/`buildEnvironment`.
- SPDX/CycloneDX JSON: rejected; protobom wire format achieved with zero
  dependency instead.

## Test plan

- `SbomReportTaskIntegrationTest.groovy`: publishes `org:leaf2:1.0` via the test
  maven repo, runs `gradle sbom`, asserts the JSON file exists and contains the
  dependency purl, `dependsOn` edges, and `root project` root element.
- `--configuration` selects `annotationProcessor` and excludes the runtime
  dependency.
- Custom output path and configuration list are exercised.
- Determinism: nodes and edges use sorted maps; roots are emitted sorted.

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
