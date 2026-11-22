# Maven recon

Clone: `apache/maven` (depth 1), Maven 4 layout: `api/`, `impl/`, `compat/`.
License Apache-2.0. `.github/AGENTS.md` and root `AGENTS.md` exist; neither bans
AI-authored contributions (only a pointer to `SECURITY.md`).

## 1. Does Maven already have SBOM?

No. `grep -ril "sbom"` returns only case-insensitive false positives:

- `impl/maven-core/src/main/java/org/apache/maven/internal/transformation/impl/DefaultConsumerPomBuilder.java` — matches `isBom`
- `impl/maven-cli/src/main/java/org/apache/maven/cling/invoker/mvnup/goals/PluginUpgradeStrategy.java` — matches `QuarkusBom`
- `impl/maven-cli/src/test/java/.../QuarkusPlatformVersionTest.java` — same

No CycloneDX, no SPDX, no SBOM writer anywhere in the tree.

## 2. Where is the resolved dependency graph?

- `impl/maven-core/src/main/java/org/apache/maven/project/MavenProject.java:897` —
  `public Set<Artifact> getArtifacts()`: the resolved transitive set of one project.
- `impl/maven-core/src/main/java/org/apache/maven/project/artifact/ProjectArtifactsCache.java:36` —
  `interface ProjectArtifactsCache` with `Key`, `CacheRecord`, `get(Key)`, `put(Key, Set<Artifact>)`,
  `register(MavenProject, Key, CacheRecord)`. Resolution is already cached per project, so an
  SBOM step needs no new resolution pass.
- `impl/maven-core/src/main/java/org/apache/maven/graph/DefaultProjectDependencyGraph.java` —
  reactor graph between modules, not third-party dependencies. Wrong graph for SBOM.

## 3. Is protobuf available?

No. `impl/maven-core/pom.xml` dependencies are exclusively internal `org.apache.maven` modules
(`maven-api-core`, `maven-api-di`, `maven-model`, `maven-model-builder`, `maven-artifact`,
`maven-impl`, `maven-support`, ...). No protobuf, not even Jackson. The only protobuf mention in
the whole tree is a test resource: `its/core-it-suite/src/test/resources/mng-8383-unknown-type-dependencies/pom.xml`.

Adding `protobuf-java` to maven-core would be a new external runtime dependency on the module
Maven is most conservative about.

## 4. Can a build step live in core?

No — this is the decisive constraint. `impl/maven-core/src/main/java/org/apache/maven/lifecycle/providers/packaging/JarLifecycleMappingProvider.java:33-44`:

```java
private static final String[] BINDINGS = {
    "process-resources", "org.apache.maven.plugins:maven-resources-plugin:" + PluginVersions.RESOURCES + ":resources",
    "compile",           "org.apache.maven.plugins:maven-compiler-plugin:" + PluginVersions.COMPILER + ":compile",
    "package",           "org.apache.maven.plugins:maven-jar-plugin:" + PluginVersions.JAR + ":jar",
    "install",           "org.apache.maven.plugins:maven-install-plugin:" + PluginVersions.INSTALL + ":install",
    "deploy",            "org.apache.maven.plugins:maven-deploy-plugin:" + PluginVersions.DEPLOY + ":deploy"
};
```

Every binding points at `org.apache.maven.plugins:maven-*-plugin`, which lives in the separate
`maven-plugins` repository. Core only maps phase to goal; it never implements a step itself.
`impl/maven-core/src/main/resources/org/apache/maven/lifecycle/plugin-versions.properties`
pins those versions and nothing else. `impl/maven-core/src/site/markdown/default-bindings.md.vm:31`
states the default lifecycle "is defined without any plugin binding".

So "core lifecycle binding" cannot host SBOM logic. A real `sbom` goal belongs in `maven-plugins`,
a different repository.

## 5. In-repo alternative: the invoker pattern

`impl/maven-cli` does host self-contained features that are not plugins, as separate invokers
with their own goals:

- `impl/maven-cli/src/main/java/org/apache/maven/cling/invoker/mvnenc/goals/` — `Init`, `Encrypt`,
  `Decrypt`, `Diag`, `GoalSupport`, `ConfiguredGoalSupport`
- `impl/maven-cli/src/main/java/org/apache/maven/cling/invoker/mvnup/goals/` — `PluginUpgrade`,
  `Apply`, `Check`, `Help`, plus `StrategyOrchestrator` and ~10 `*Strategy` classes

The plain `mvn` invoker (`invoker/mvn/`) has no `goals/` directory — it delegates to the lifecycle.
Its options are `api/maven-api-cli/src/main/java/org/apache/maven/api/cli/mvn/MavenOptions.java:39`
(interface, `Optional<String> artifactsUpdatePolicy()` at :90), implemented by
`CommonsCliMavenOptions.java:33` and `LayeredMavenOptions.java`, with the shared
`CLIManager` at `impl/maven-cli/src/main/java/org/apache/maven/cling/invoker/CommonsCliOptions.java:298`.

## 6. Gaps against NTIA

- supplier: not on `Artifact`. Absent.
- license: `Artifact` carries none; Maven only has it in the POM, not the resolved artifact.
- hash: `Artifact.getFile()` gives a path; no checksum is retained in the resolved set.
- purl: no purl anywhere in Maven. Must be synthesised as `pkg:maven/group/artifact@version`.
