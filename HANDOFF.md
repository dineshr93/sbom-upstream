# Handoff — session state as of 2026-11-22

Snapshot for resuming work in a fresh session. STATUS.md stays the live
tracking table; this file captures session-only context (environment,
decisions, pending actions) that no other file holds.

## Where things stand

| Repo | Stage | Artifact |
| --- | --- | --- |
| cmake | skipped | already ships `install(SBOM)` SPDX 3.0 JSON |
| cargo | skipped | already ships `-Zsbom` JSON precursor |
| meson | pr-open | https://github.com/mesonbuild/meson/pull/16374 |
| bazel | pr-open | https://github.com/bazelbuild/bazel/pull/32863 |
| gradle | pr-open | https://github.com/gradle/gradle/pull/39928 |
| maven | pr-open | https://github.com/apache/maven/pull/14093 |
| go | pr-drafted | Gerrit message in `pr/go.md`; raise blocked |
| autotools | pr-drafted | Savannah cover letter in `pr/autotools.md`; raise blocked |

Implementation patches archived in `impl/` (format-patch, one per repo, plus
`impl/autotools/` files). The scratch clones they came from are gone in 24h;
`impl/` is now the only local copy besides the fork branches.

## Pending actions

1. Watch the four open PRs for maintainer feedback; record responses in
   STATUS.md "Maintainer feedback" column.
2. go: raising needs a Gerrit account (go-review.googlesource.com) with CLA
   signed. No credentials on this machine. `pr/go.md` is ready to push as
   `refs/for/master` once auth exists.
3. autotools: raising needs an MTA or a savannah.gnu.org account for the patch
   tracker. `pr/autotools.md` is the ready cover letter.
4. Stray fork `dineshr93/sbom-bazel` (used as bazel PR source). Deleting it
   needs `delete_repo` scope: `gh auth refresh -h github.com -s delete_repo`.
   Only removable after bazel#32863 closes, since the PR head lives on it.

## Environment facts

- `gh` CLI authed as `dineshr93`, scopes `gist, read:org, repo, ssh`. HTTPS
  push works via gh credential helper; bare `git@github.com:` ssh push fails
  (key not accepted), but `git@github.com:dineshr93/sbom-upstream.git` works.
- This gh build prints odd noise on `gh repo fork` / `gh pr create`
  ("accessibility" / "event subscriptions" lines) while succeeding. Verify
  side effects via `gh api repos/<owner>/<repo>/pulls?head=dineshr93:sbom-support`
  instead of trusting stdout.
- Forks exist: `dineshr93/{meson,gradle,maven,sbom-bazel}`, each with branch
  `sbom-support` pushed.
- Clones were at `~/.hermes/cache/scratch/` (prunes in 24h; patches in `impl/`
  replace them).

## Decisions made this session

- meson `AGENTS.md` bans AI-written PR text. User explicitly overrode that
  rule for PR #16374 only ("override the ban part alone"). AGENTS.md itself
  untouched. Do not treat as standing permission for other meson PRs.
- User gave explicit approval to raise all PRs before the four were opened,
  satisfying the workflow's human-approval gate.
- bazel PR sources from fork repo `sbom-bazel` (odd name, harmless) because
  the plain `bazel` fork POST failed and deletion needs a missing scope.
- PR bodies stored in `pr/<repo>.md` with the PR URL as a line under the
  header; STATUS.md rows keep link + provenance, not summaries.
