# Autotools design: `--enable-sbom` + `make sbom`

Follows the silent-rules pattern end to end (m4/silent.m4 + handle_silent +
AM_V_* vars). Three files touched, no new dependency, no Perl serialization.

1. **m4/sbom.m4** (new, ~20 lines): `AC_DEFUN([_AM_SBOM], ...)` with
   `AC_ARG_ENABLE([sbom], [AS_HELP_STRING([--enable-sbom], [generate
   sbom.protobom.bin (make sbom)])])`, default disabled. Mirrors
   m4/silent.m4:13 structure.
2. **m4/init.m4**: one line `AC_REQUIRE([_AM_SBOM])dnl` next to
   `AC_REQUIRE([_AM_SILENT_RULES])` (m4/init.m4:138).
3. **bin/automake.in**: `sub handle_sbom` after handle_silent (:1240), called
   from generate_makefile after `handle_silent;` (:8330). Forward decl at :111
   block. It defines `am__sbom_components` (define_pretty_variable) = names of
   programs/libraries of the current Makefile, and pulls in lib/am/sbom.am.
4. **lib/am/sbom.am** (new, ~35 lines): `sbom:` target + `sbom-am:` rule that
   writes `sbom.protobom.bin` with a printf-based shell writer — varint tags +
   length-delimited strings, same wire subset as the go design (verified there
   with `protoc --decode`): Document{metadata=1,node_list=2},
   Metadata{name=3,date=4,tools=5,authors=6}, Timestamp{seconds=1},
   Tool{name=1,version=2}, NodeList{nodes=1,edges=2,root_elements=3},
   Node{id=1,name=3,version=4}, Edge{type=1,from=2,to=3}, type 10=dependsOn.
   Root node = $(PACKAGE)@$(VERSION) (distdir.am:21 precedent); one dependsOn
   edge root→component per entry in $(am__sbom_components). Sorted by name.
   Timestamp: `${SOURCE_DATE_EPOCH:-$(date +%s)}`.
5. **t/sbom.sh** (new): tiny package, `configure --enable-sbom`, `make sbom`,
   assert file exists, non-empty, first byte 0x0a (field 1 wire type 2).

Deviations from queue row:
- "vendored writer" → shell printf instead: automake ships no C runtime to
  vendor emitter/*.c into; printf emits the same bytes.
- purl omitted (no standard purl type for autotools packages); hash, license,
  supplier omitted (no data, per recon).
- `sbom` target exists in every Makefile (like `tags`); --enable-sbom only
  gates whether `all-am` depends on it. Keeps automake.in gating logic out.

~70 lines total. Contribution is Savannah patch mail, not GitHub PR — no
pr/autotools.md (matches gradle/bazel/go precedent).
