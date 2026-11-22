# Design: Gradle

Target: `gradle generateSbom` writes a protobom `Document` describing the
resolved dependency graph of the project. Built-in task (no plugin to apply),
opt-in, no new dependency, no network at execution time.

The design follows `build-systems/gradle.md`: data source is the resolved
configuration graph, the entry point is a built-in task named `generateSbom`.

## 0. The one decision that changes the shape

`build-systems/gradle.md` says "Java has protobuf support — use protobom's
Java bindings if published, else generate from the `.proto`." Recon shows
Gradle has **no protobuf runtime at all** (no `.proto` files, no
`com.google.protobuf` dependency anywhere in `platforms/`, `build-logic/`,
`gradle/`). Depending on `protobuf-java` would ship it in the distribution
whether or not the task runs, violating "zero new mandatory runtime deps for
users who do not pass the flag".

Chosen: a hand-rolled proto3 wire encoder (varint + length-delimited only),
the same approach as `emitter/src/sbom_emitter.c`, living in Gradle as
`ProtobomEncoder.java`. Zero new dependency. Alternative rejected: add
`protobuf-java` (~500 KB to every Gradle distribution for an opt-in task).

## 1. File-by-file change list

| File | Change | Rationale |
| --- | --- | --- |
| `platforms/core-configuration/base-diagnostics/src/main/java/org/gradle/api/tasks/diagnostics/internal/DiagnosticsTaskNames.java` (+8 LOC) | add `String GENERATE_SBOM_TASK = "generateSbom";` | Task-name constants live here (`:29`, @since 8.13) and `HelpTasksPlugin` re-exports each one (`HelpTasksPlugin.java:43-84`). |
| `platforms/core-configuration/base-diagnostics/src/main/java/org/gradle/api/plugins/HelpTasksPlugin.java` (+8 LOC) | add `public static final String GENERATE_SBOM_TASK = DiagnosticsTaskNames.GENERATE_SBOM_TASK;` | Same re-export pattern as `DEPENDENCIES_TASK` (`:55`). |
| `platforms/software/software-diagnostics/src/main/java/org/gradle/api/tasks/diagnostics/internal/ProtobomEncoder.java` (new, ~130 LOC) | `public static byte[] encode(SbomDocument)` plus private `writeVarint`/`writeTag`/`writeBytes`; no external dependency | Protobuf is absent from Gradle (see §0). Field numbers match `emitter/src/sbom_emitter.c`. |
| `platforms/software/software-diagnostics/src/main/java/org/gradle/api/tasks/diagnostics/SbomReportTask.java` (new, ~120 LOC) | `@NullMarked @DisableCachingByDefault @Incubating public abstract class SbomReportTask extends DefaultTask` | Sits next to `DependencyReportTask.java:26` in the same package; `software-diagnostics/build.gradle.kts` already has every dependency the task needs (`:20-49`). |
| `platforms/software/software-diagnostics/src/main/java/org/gradle/api/plugins/SoftwareReportingTasksPlugin.java` (+6 LOC) | `tasks.register(HelpTasksPlugin.GENERATE_SBOM_TASK, SbomReportTask.class, …)` alongside `:49-51`, using the file's private-static-`Action` convention (`:46`) | That plugin is auto-applied to every project by `SoftwareReportingTasksAutoApplyAction.java:29`, which is what makes the task built-in. |
| `platforms/software/software-diagnostics/src/integTest/groovy/org/gradle/api/tasks/diagnostics/SbomReportTaskIntegrationTest.groovy` (new, ~70 LOC) | functional test: resolve a `mavenLocal()` dependency, run `generateSbom`, assert the file exists and is non-empty; run twice with `SOURCE_DATE_EPOCH` and compare bytes | Mirrors `BuildEnvironmentReportTaskIntegrationTest.groovy` (3.3K, the smallest precedent in that directory). |
| `platforms/documentation/docs/src/docs/userguide/releases/upgrading/…` — none | no upgrade note | New task, no behaviour change. |

Net new: ~340 LOC, under the 500 LOC budget.

No docs change is included. `project_report_plugin.adoc` documents the
`project-report` plugin's tasks, not the auto-applied reporting tasks, and it
is under a different license (CC-BY-NC-SA-4.0) than the code. A docs addition
belongs in the "command line project reporting" page; left out of this change
to keep the diff to one concern.

## 2. Task shape

```java
@DisableCachingByDefault(because = "Not worth caching")
@Incubating
public abstract class SbomReportTask extends DefaultTask {
    @Option(option = "configuration", description = "The configuration to generate the SBOM for.")
    public void setConfiguration(String name) { … }        // optional, reuses ConfigurationFinder

    @OutputFile
    public abstract RegularFileProperty getOutput();       // convention: build/reports/sbom/sbom.protobom.bin

    @TaskAction
    public void generate() { … }
}
```

Default scope: every resolvable, non-deprecated configuration of the project,
deduplicated. This reuses the exact filter `ConfigurationDetails.of()` already
applies (`ConfigurationDetails.java:28,38`: `configuration.isCanBeResolved()
&& !((DeprecatableConfiguration) configuration).isDeprecatedForResolution()`),
and matches the `dependencies` task, which reports all configurations
(`AbstractDependencyReportTask.java:142-150`). Nodes are deduplicated by id
and edges by `(from, to)`, so a component reachable from several
configurations appears once.

`--configuration` is included because `AbstractDependencyReportTask.java:138`
already has the identical `@Option` and `ConfigurationFinder.find(...)` call,
so it costs four lines of reuse rather than new code.

## 3. Data mapping

| Build-system field | protobom field | NTIA element |
| --- | --- | --- |
| `"gradle"` | `Document.metadata.name` | — |
| `SOURCE_DATE_EPOCH` or `Instant.now()` | `Metadata.date.seconds` | timestamp |
| `"Gradle"` + `GradleVersion.current().getVersion()` | `Metadata.tools[0]` | author of SBOM data |
| `"Gradle"`, `is_org = true` | `Metadata.authors[0]` | author of SBOM data |
| `ComponentIdentifier.getDisplayName()` (`subprojects/core-api/…/component/ComponentIdentifier.java:33`) | `Node.id` | unique identifier |
| `ModuleComponentIdentifier.getModule()` (`…/ModuleComponentIdentifier.java:42`); project components use the project path | `Node.name` | component name |
| `ModuleComponentIdentifier.getVersion()` (`…/ModuleComponentIdentifier.java:50`) | `Node.version` | version |
| synthesized `pkg:maven/<group>/<module>@<version>` | `Node.identifiers[PURL=1]` | unique identifier |
| `ResolutionResult.getAllDependencies()` → `ResolvedDependencyResult.getSelected()` (`…/ResolvedDependencyResult.java:32`) with `getFrom()` (`…/DependencyResult.java:38`) | `Edge{type=dependsOn, from, to}` | dependency relationships |
| root component of each configuration | `NodeList.root_elements` | — |
| — (not modelled) | `Node.suppliers` omitted | supplier — **gap** |
| — (not on the resolution result) | `Node.licenses` omitted | license — **gap** |
| — (not on the resolution result) | `Node.hashes` omitted | hash — **gap** |

Node id rule: `componentId.getDisplayName()`. For a module component that is
`group:module:version`; for a project component it is the project path. Both
are unique within the build and stable across runs.

purl rule: only for `ModuleComponentIdentifier`, where group, module and
version are all present. `ProjectComponentIdentifier`
(`…/ProjectComponentIdentifier.java:43`) has no version, so no purl is
emitted for it — never fabricated.

Edge rule: iterate `getAllDependencies()`, keep only `ResolvedDependencyResult`
(unresolved ones are skipped), skip `isConstraint()`
(`…/DependencyResult.java:56`), drop self-edges.

## 4. Failure modes

| Condition | Behaviour |
| --- | --- |
| Component has no `ModuleVersionIdentifier` (e.g. a file or build dependency) | `Node.version` and `Node.identifiers` omitted; node still present with id and name |
| `UnresolvedDependencyResult` | skipped entirely; an unresolved dependency is not a component |
| No configuration is resolvable | document with only the root node is written; not an error |
| No supplier / license / hash | fields omitted; documented gaps |
| `SOURCE_DATE_EPOCH` not a number | falls back to the wall clock |
| Output directory missing | created before writing |
| `generateSbom` not invoked | nothing happens; the task registers lazily (`tasks.register`), so an unrelated build pays nothing |

## 5. Test plan

1. **Functional** (`SbomReportTaskIntegrationTest.groovy`): a project with a
   `mavenLocal()` dependency, `gradle generateSbom`, assert
   `build/reports/sbom/sbom.protobom.bin` exists and is non-empty, and that
   the bytes contain the dependency's display name and the `Gradle` tool entry.
2. **Determinism**: run twice with `SOURCE_DATE_EPOCH=1700000000`, assert the
   two files are byte-identical.
3. **`--configuration`**: restrict to one configuration, assert the document
   shrinks.
4. **Schema conformance**: verified out of band with
   `protoc --decode=protobom.protobom.Document` against the vendored schema,
   as done for the emitter. No golden binary is checked in.

## 6. Backwards compatibility

- New task only; no existing task, plugin or resolution behaviour changes.
- No new dependency (§0).
- `@Incubating` on the new public task type, per Gradle's API policy; new API
  gets `@since 9.10.0` (`version.txt` = `9.10.0`, latest final release `9.9.2`).
- Adding constants to `DiagnosticsTaskNames` and `HelpTasksPlugin` is
  additive.
- The task performs resolution at execution time, which is what `dependencies`
  already does; no new service, no new daemon state.

## 7. Open questions for the maintainer

1. Is a hand-rolled proto3 encoder acceptable in Gradle core, or would
   maintainers prefer a different serialization (for example, emitting
   protobom JSON, which needs no encoder at all but is not the format the
   spec asks for)?
2. Task name `generateSbom` (as the design brief says) versus the shorter
   `sbom` used by the other report tasks in `DiagnosticsTaskNames`?
3. Should the default scope be all resolvable configurations (this design) or
   just `runtimeClasspath`?
