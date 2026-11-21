# Recon prompt

```
Repo: <URL>
Task: SBOM integration reconnaissance. Do NOT write code yet.

Answer with file paths as evidence:
1. Where is the resolved dependency/component graph built and stored?
   (struct/class names, file:line)
2. What metadata is available per component: name, version, supplier/
   maintainer, license, hash, purl/CPE, source URL?
3. What are the existing output/serialization mechanisms (e.g. CMake
   file(GENERATE), Meson introspection JSON, Bazel providers, Gradle
   tasks)? Which is the cheapest to extend?
4. Where do CLI flags get parsed? Show the pattern for adding one.
5. What test framework and fixture conventions exist? Where would an
   SBOM test live?
6. Any existing SBOM/SPDX/CycloneDX code already in-tree? (search:
   sbom, spdx, cyclonedx, purl, license)
7. Language + build system of the tool itself, and its dependency policy
   (vendoring allowed? new third-party libs allowed?).
8. License of the repo and of protobom; confirm compatibility.

Output: a table of findings + a recommended integration point + a list of
NTIA fields that are NOT obtainable from the current graph.
```

Save the answer to `recon/<repo>.md`.

If recon shows the repo already has SBOM support, skip the repo and note why in
`STATUS.md`.
