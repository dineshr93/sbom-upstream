# Maven design

## Recommendation first

The honest answer from the recon: a real `sbom` goal does not belong in `apache/maven`.
Core maps phases to goals but implements no step itself
(`JarLifecycleMappingProvider.java:33-44`), and every bound plugin lives in the separate
`maven-plugins` repository. The idiomatic upstream contribution is `maven-sbom-plugin` there,
which is out of scope for this repo.

Two things are needed to proceed, and they are separate decisions:

1. Accept that the contribution targets `apache/maven` anyway (the queue says Maven).
2. Accept a dependency-free encoder, because maven-core has no protobuf.

If either is unacceptable, the correct move is to skip Maven like CMake was skipped.

## In-repo design (if proceeding)

`mvn --sbom[=<path>]` — a flag on the existing `mvn` invoker, not a new invoker.

Scope, smallest first:

- `api/maven-api-cli/src/main/java/org/apache/maven/api/cli/mvn/MavenOptions.java` — add
  `Optional<String> sbomFile();` (next to `artifactsUpdatePolicy()` at :90).
- `impl/maven-cli/src/main/java/org/apache/maven/cling/invoker/CommonsCliOptions.java:298` —
  add the `sbom` `Option` to the shared `CLIManager`.
- `impl/maven-cli/src/main/java/org/apache/maven/cling/invoker/mvn/CommonsCliMavenOptions.java`
  and `LayeredMavenOptions.java` — implement the accessor, same shape as the existing
  `Optional<String>` accessors.
- `impl/maven-cli/src/main/java/org/apache/maven/cling/invoker/mvn/MavenInvoker.java` — after the
  build request completes, if `sbomFile()` is present, write the report.
- New `impl/maven-cli/src/main/java/org/apache/maven/cling/invoker/mvn/SbomReportWriter.java`
  (~120 LOC) — walks `MavenSession.getProjects()`, unions `MavenProject.getArtifacts()` (:897),
  emits the document.

Total ~180 LOC, under the 500 budget. No new resolution: `ProjectArtifactsCache` already holds
the resolved set.

## Format

JSON, not protobuf. maven-core has zero protobuf dependency, and adding `protobuf-java` to core
for one report is the change most likely to be rejected. Same reasoning as Gradle, which also
ended up with a hand-rolled encoder. If protobuf is required, the wire format can be hand-encoded
in ~60 extra LOC with no dependency, but JSON first.

## Field mapping

| NTIA element | Source | Status |
|---|---|---|
| name | `Artifact.getArtifactId()` | available |
| version | `Artifact.getVersion()` | available |
| purl | `pkg:maven/<groupId>/<artifactId>@<version>` | synthesised |
| supplier | none on `Artifact` | omitted |
| license | none on `Artifact` | omitted |
| hash | none retained in resolved set | omitted |

Deterministic: nodes sorted by purl, timestamp from `SOURCE_DATE_EPOCH` when set.

## Opt-in

Nothing runs unless `--sbom` is passed. No lifecycle binding, so no default-build cost.

## Open questions

1. `--sbom` vs `--sbom-output`? Maven has no `experimental_` prefix convention, so plain `--sbom`
   is fine, but the flag needs a value for the path or a fixed default.
2. Should this instead be a `mvnsbom` invoker (mvnenc/mvnup precedent)? More boilerplate for no
   gain; the data lives in the session that `mvn` already builds.
3. Is contributing to `maven-plugins` acceptable instead? That is the better home.
