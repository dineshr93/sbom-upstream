# Implementation prompt

```
Implement the approved design for <repo> on branch `sbom-support`.

Rules:
- Match surrounding code style exactly (naming, error handling, logging).
- Add license header consistent with the repo.
- Keep the diff minimal and reviewable; no drive-by refactors.
- Add the flag to `--help` and to the docs page that lists flags.
- Add a test that runs the build with the flag and validates the output
  against the protobom schema (or a JSON schema for SPDX/CycloneDX).
- If a field is unavailable, omit it — do not fabricate values.

Then output:
- `git diff` of the change
- The exact commands to run the new test locally
- A list of anything you could not verify without running the build
```
