# Design prompt

```
Using the recon findings for <repo>, produce an implementation design.

Requirements:
- New opt-in flag: `-sbom` / `--sbom` / `-Dsbom=ON` matching the tool's
  existing flag conventions. Optional value for output path; default
  `<build-dir>/sbom.protobom.bin` (or `.json` if protobuf is not viable).
- Emit protobom Document: metadata (tool, timestamp, authors), nodes
  (components with purl where derivable), edges (depends-on).
- Include only components that actually contribute to the build output
  where the tool can distinguish that; otherwise include all resolved
  components and say so in the docs.
- Implementation must be <500 LOC net new, in the tool's native language.
- No new mandatory dependency. If protobuf is needed, vendor a minimal
  writer or gate it behind the flag's build option.

Deliver:
1. File-by-file change list with rationale.
2. Data mapping table: build-system field -> protobom field -> NTIA element.
3. Failure modes (missing version, missing supplier) and how they degrade.
4. Test plan: unit + integration + a golden-file test.
5. Backwards-compatibility statement.
```

Save to `design/<repo>.md`. **STOP and ask for approval before implementing.**
