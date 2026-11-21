# sbom-upstream

Prompt pack + tracking repo for contributing first-class SBOM generation to
build systems upstream.

Every build system already knows the components required to build software:
source files, static and dynamic modules, direct and indirect dependencies, and
which of them land in the final binary. That is an SBOM. Regulations (NTIA
minimum elements, EO 14028, EU CRA) demand one. The goal is a built-in
`-sbom` / `--sbom` flag in each tool that emits a Protocol Buffers SBOM using
the [protobom](https://github.com/protobom/protobom) schema, so downstream
patching and third-party scanners become unnecessary.

## Repo queue

| Tool | Language | Status |
| --- | --- | --- |
| CMake | C++ | queued |
| Meson | Python | queued |
| Bazel | Starlark/Java | queued |
| Gradle | Java/Kotlin | queued |
| Maven | Java | queued |
| Cargo | Rust | queued |
| Go | Go | queued |
| Autotools | shell/m4 | queued |

Live status, PR links and maintainer objections: [STATUS.md](STATUS.md).

## Constraints (apply to every PR)

- Output: protobuf, protobom schema (`proto/` directory). Fall back to
  SPDX-JSON or CycloneDX-JSON only if protobuf is infeasible for that tool, and
  say so in the PR.
- Minimum content: NTIA minimum elements — supplier, component name, version,
  unique identifier (purl/CPE), dependency relationships, author of SBOM data,
  timestamp.
- Zero new mandatory runtime deps for users who do not pass the flag.
- Flag is opt-in, off by default, documented, and covered by tests.
- No network calls at build time. No telemetry.
- Respect the project's style, license headers, CONTRIBUTING.md and PR template.
- Never fabricate a field. If the build system cannot supply it, omit it and
  document the gap.

## Workflow

One repo at a time. Never open a PR without explicit human approval.

1. Recon → `recon/<repo>.md` ([prompts/01-recon.md](prompts/01-recon.md))
2. Design → `design/<repo>.md` ([prompts/02-design.md](prompts/02-design.md))
3. **STOP** — human approves the design
4. Implement on branch `sbom-support` ([prompts/03-implement.md](prompts/03-implement.md))
5. PR body → `pr/<repo>.md` ([prompts/04-pr.md](prompts/04-pr.md))
6. Record in `STATUS.md`

Preflight checklist before any code: [prompts/00-preflight.md](prompts/00-preflight.md).
Orchestration loop for an agent: [prompts/05-orchestrator.md](prompts/05-orchestrator.md).

## Layout

```
prompts/         the copy-pasteable prompts, in order
build-systems/   per-tool data source, flag shape, format tradeoff, gaps
emitter/         design brief for a small embeddable SBOM emitter
recon/           recon findings per repo (created as work proceeds)
design/          approved designs per repo
pr/              PR titles + bodies per repo
STATUS.md        tracking table
```

## Related

- protobom: https://github.com/protobom/protobom
- NTIA minimum elements: https://www.ntia.gov/report/2021/minimum-elements-software-bill-materials-sbom
- EO 14028: https://www.federalregister.gov/d/2021-10408
- EU Cyber Resilience Act: https://eur-lex.europa.eu/eli/reg/2024/2847/oj
