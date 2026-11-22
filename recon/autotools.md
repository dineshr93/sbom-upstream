# Autotools (GNU automake) recon

Repo: git.savannah.gnu.org/git/automake.git (depth-1, commit 9fca901, 13M).
No GitHub mirror (github.com/automake/automake: "Repository not found").
License: GPL-3.0+ (COPYING). No AGENTS.md, no CONTRIBUTING*.

1. **Component graph?** No third-party dependency graph anywhere. Automake
   knows programs/libraries per Makefile (lib/am/progs.am, lib/am/ltlib.am)
   only. Gap: graph.
2. **Per-component metadata?** PACKAGE + VERSION exist as Makefile variables
   (lib/am/distdir.am:21 `distdir = $(PACKAGE)-$(VERSION)`). No license, no
   supplier, no hash anywhere. Gaps: license, hash, supplier.
3. **Output mechanisms?** lib/am/*.am templates emitted into every generated
   Makefile.in: tags target (lib/am/tags.am:52-58 `tags:`/`tags-am:`), dist
   (lib/am/distdir.am). A `sbom` target fits this template mechanism.
4. **Flag convention?** AC_ARG_ENABLE inside automake-shipped m4 macros,
   required by AM_INIT_AUTOMAKE: m4/silent.m4:13
   `AC_ARG_ENABLE([silent-rules])`, m4/depend.m4:172 `[dependency-tracking]`,
   m4/maintainer.m4:24 `[maintainer-mode]`, m4/jobserver.m4:11 `[jobserver]`.
   m4/init.m4:138 `AC_REQUIRE([_AM_SILENT_RULES])`. Handler registration in
   bin/automake.in: `sub handle_silent` at :1240, called from generate_makefile
   at :8330; AM_V_* vars consumed by lib/am/dejagnu.am, lisp.am, remake-hdr.am,
   distdir.am. `--enable-sbom` maps 1:1 onto this pattern.
5. **Test framework?** t/ dir, 1316 entries, .sh/.pl/.tap via t/local.mk
   (`LOG_COMPILER = ./pre-inst-env $(AM_TEST_RUNNER_SHELL)`,
   `TEST_EXTENSIONS = .pl .sh .tap`).
6. **Existing SBOM support?** Zero hits for sbom/spdx/cyclonedx in repo
   (grep over .am/.pm/.in/.m4/.ac, t/, doc/). Not a skip candidate.
7. **Serialization?** No protobuf, no JSON lib. Perl (bin/automake.in, 268K)
   but Makefile recipes are shell — writer goes in the recipe (printf), no
   Perl serialization needed.
8. **Contribution flow?** Savannah git + automake-patches maillist; no GitHub
   PR flow, no GitHub mirror.

Verdict: implement. Data source = programs/libraries of each Makefile plus
$(PACKAGE)/$(VERSION); gaps (graph, per-dep versions, hash, license) omitted,
not faked.
