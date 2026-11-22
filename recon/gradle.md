# Recon: gradle/gradle

- **Repo:** gradle/gradle @ `5956d973` (depth-1 clone)
- **License:** Apache-2.0 (`LICENSE`) — compatible with protobom (also Apache-2.0).
- **Language:** Java (Groovy for tests), Gradle's own build. `version.txt` = `9.10.0`; `released-versions.json` latest final = `9.9.2`, so the in-development version is 9.10.0 and new API takes `@since 9.10.0`.
- **AI policy:** `AI_POLICY.md` welcomes AI-assisted contributions and requires disclosure in the PR description (not commit co-author trailers). `AGENTS.md` (755B) delegates to `CONTRIBUTING.md#code-change-guidelines` plus `contributing/ErrorMessages.md`, `JavadocStyleGuide.md`, `Nullability.md`, `Testing.md`. `CLAUDE.md` just points at `AGENTS.md`. No authorship ban — unlike Meson, a PR body can be drafted here.
- **Repo layout:** restructured from `subprojects/` to `platforms/<group>/`. Both exist: public API still lives in `subprojects/core-api`, implementation in `platforms/software/…`, `platforms/core-configuration/…`.

## Existing SBOM support

None. `grep -rniI "sbom"` over `platforms`, `testing`, `build-logic` returns two unrelated hits:

- `platforms/software/dependency-management/src/main/java/org/gradle/api/internal/runtimeshaded/RuntimeShadedJarCreator.java:269` — a comment: "gradleApi()/TestKit advertise ~150 phantom org.gradle artifacts to SBOM/vulnerability scanners." Confirms the gap is known upstream, no generator exists.
- `testing/internal-distribution-testing/…/AbstractMavenModule.groovy:594` — `isBom`, a POM packaging check, unrelated.

`grep -rniI "cyclonedx"` over `platforms` and `testing`: empty. `spdx` hits are only `SPDX-License-Identifier` headers (`plugins-application/src/main/resources/…/unixStartScript.txt:18`, `wrapper-main/build.gradle.kts:78`) and unrelated `WrapperGenerationIntegrationTest.groovy`.

## Data source: the resolved configuration graph

- `subprojects/core-api/src/main/java/org/gradle/api/artifacts/result/ResolutionResult.java:110` — `Set<ResolvedComponentResult> getAllComponents()` (@since 1.10). Comment at :45 states "Gradle's built-in 'dependencies' task uses this to render the dependency tree."
- `ResolutionResult.java:92` — `void allDependencies(Action<? super DependencyResult>)` (@since 1.3), documented at :81 as "In dependency graph terminology, this method returns the edges of the graph."
- `ResolutionResult.java:58` — `Provider<ResolvedComponentResult> getRootComponent()` (@since 7.4), lazy.
- `ResolutionResult.java:136` — variant-scoped accessor (@since 5.6).
- `subprojects/core-api/…/artifacts/result/ResolvedComponentResult.java:36` — `interface ResolvedComponentResult extends ComponentResult`; :52 `Set<? extends DependencyResult> getDependencies()`; :60 `getDependents()`; :90 `@Nullable ModuleVersionIdentifier getModuleVersion()`; :100 `List<ResolvedVariantResult> getVariants()` (@since 5.2).
- `subprojects/core-api/…/artifacts/result/ComponentResult.java:38` — `getComponentId()` (@since 2.0).
- `subprojects/core-api/…/artifacts/component/ModuleComponentIdentifier.java:34,42,50` — `getGroup()`, `getModule()`, `getVersion()` (@since 1.10); :60 `getModuleIdentifier()` (@since 4.9).

### How the existing task reaches the graph (reuse target)

- `platforms/software/software-diagnostics/src/main/java/org/gradle/api/tasks/diagnostics/DependencyReportTask.java:26` — `public abstract class DependencyReportTask extends DefaultTask {}`, an empty subclass of:
- `platforms/software/software-diagnostics/src/main/java/org/gradle/api/tasks/diagnostics/AbstractDependencyReportTask.java` — :53 `private transient Set<Configuration> configurations;`, :85-89 builds a `SortedSet` ordered by name and wraps each in `ConfigurationDetails.of(configuration)`, :103-131 `getConfigurations()`/`setConfigurations()`/`setConfiguration(String)` with `@Option(option = "configuration", …)`.
- `platforms/software/software-diagnostics/src/main/java/org/gradle/api/tasks/diagnostics/internal/ConfigurationDetails.java:27` — `public static ConfigurationDetails of(Configuration configuration)`; :28 `canBeResolved(configuration)`; :33 `configuration.getIncoming().getResolutionResult().getRootComponent()`; :38 `canBeResolved()` filters via `((DeprecatableConfiguration) configuration).isDeprecatedForResolution()`. This resolvable-configuration filter is exactly what an SBOM task needs and already exists.

## Where a built-in task goes

- `platforms/software/software-diagnostics/src/main/java/org/gradle/api/plugins/SoftwareReportingTasksPlugin.java:41` — `public abstract class SoftwareReportingTasksPlugin implements Plugin<Project>` (@since 8.13, `@NullMarked`); :49-51 `tasks.register(HelpTasksPlugin.DEPENDENCY_INSIGHT_TASK, …)`, `DEPENDENCIES_TASK`, `BuildEnvironmentReportTask.TASK_NAME`; :53-70 more report tasks with `setGroup(HelpTasksPlugin.HELP_GROUP)` and `setImpliesSubProjects(true)`; :77-121 private static `Action` classes with the comment at :46 "static classes are used for the actions to avoid implicitly dragging project/tasks into the model registry".
- `platforms/software/software-diagnostics/src/main/java/org/gradle/api/plugins/internal/SoftwareReportingTasksAutoApplyAction.java:22` — `implements ProjectConfigureAction`; :29 `project.getPluginManager().apply("org.gradle.software-reporting-tasks")`. This is what makes those tasks built-in on every project.
- `platforms/core-configuration/base-diagnostics/src/main/java/org/gradle/api/tasks/diagnostics/internal/DiagnosticsTaskNames.java:29` — `public interface DiagnosticsTaskNames` (@since 8.13) holding `HELP_GROUP`, `PROPERTIES_TASK`, `DEPENDENCIES_TASK`, `DEPENDENCY_INSIGHT_TASK`, … Task-name constants live here.

## Protobuf availability — the blocker

`grep -rn "com.google.protobuf"` over `platforms`, `build-logic`, `gradle/` and `find . -name "*.proto"`: **no protobuf runtime anywhere in Gradle.** The only hits are documentation snippets for the third-party `com.google.protobuf` Gradle plugin (`platforms/documentation/docs/src/snippets/best-practices/…`). `platforms/software/software-diagnostics/build.gradle.kts:37-49` lists `groovy`, `jspecify`, `inject`, `commonsLang`, `guava`, `gson`, `jatl` — no protobuf.

Consequence: `build-systems/gradle.md` says "Java has protobuf support — use protobom's Java bindings if published, else generate from the `.proto`." Adding `protobuf-java` to the distribution would violate the README constraint "zero new mandatory runtime deps for users who do not pass the flag", because Gradle's distribution is monolithic — a dependency of `software-diagnostics` ships whether or not the task runs. The only compliant option is a hand-rolled proto3 wire encoder (varint + length-delimited), the same approach as `emitter/src/sbom_emitter.c`.

## NTIA element availability

| Element | Source | Status |
| --- | --- | --- |
| Component name | `ModuleComponentIdentifier.getModule()`; project components via `ProjectComponentIdentifier` | available |
| Version | `ModuleComponentIdentifier.getVersion()`; `ResolvedComponentResult.getModuleVersion()` | available for module/project components |
| Unique id (purl) | derivable as `pkg:maven/<group>/<module>@<version>` from `ModuleComponentIdentifier` | derivable |
| Dependency relationships | `ResolutionResult.getAllDependencies()` / `ResolvedComponentResult.getDependencies()` | available |
| Author of SBOM data | tool entry (`gradle` + version) | available |
| Timestamp | `SOURCE_DATE_EPOCH` or `Instant.now()` | available |
| Supplier | no first-class Gradle concept | **gap** |
| License | not on `ResolvedComponentResult`; POM licenses are internal to the metadata parser and not exposed | **gap** |
| Hash | `ResolvedArtifactResult` (subprojects/core-api/…/ResolvedArtifactResult.java:29) exposes `getFile()`, `getAttributes()` (9.7.0), `getCapabilities()` (9.7.0), `getVariant()` — no checksums. `getChecksums()` exists only on dependency-verification metadata (`…/verification/model/ArtifactVerificationMetadata.java:24`), not on resolution results | **gap** |

## Tests

- `platforms/software/software-diagnostics/src/integTest/groovy/org/gradle/api/tasks/diagnostics/` — `DependencyReportTaskIntegrationTest.groovy` (31.5K), `DependencyInsightReportTaskIntegrationTest.groovy` (101K), `BuildEnvironmentReportTaskIntegrationTest.groovy` (3.3K, smallest precedent), `ResolvableConfigurationsReportTaskIntegrationTest.groovy`.
- `platforms/software/software-diagnostics/src/test/groovy/org/gradle/api/tasks/diagnostics/` — `DependencyReportTaskTest.groovy`, `AbstractReportTaskTest.groovy` (ProjectBuilder-based unit tests).
- `contributing/Testing.md` governs test placement; `.agents/testing.md` summarises it.

## Docs

- `platforms/documentation/docs/src/docs/userguide/reference/core-plugins/project_report_plugin.adoc` (88 lines, CC-BY-NC-SA-4.0 header) documents the reporting tasks with a `task — link:{groovyDslPath}/…[Type]::` table and a "Report output" section. Docs live under a different license than the code — note in the PR.

## Verdict

Feasible, ~350 LOC net, no new dependency, no network at build time. The one real design risk to surface to maintainers: shipping a hand-rolled protobuf wire encoder inside Gradle core rather than depending on `protobuf-java`.
