# Savannah patch cover letter: automake

(Savannah patch mail, not a GitHub PR. Branch `sbom-support` in the automake
clone, commit `fbf8409`, 84 insertions across 6 files. Send to the automake
list; no Change-Id/PR mechanics apply.)

---

Subject: [PATCH] sbom: add --enable-sbom and 'make sbom'

Hello,

This series adds opt-in SBOM generation to automake. With
--enable-sbom, 'make sbom' writes sbom.protobom.bin, a protobom
Document describing the package and its programs/libraries.

The implementation follows the silent-rules pattern end to end
(m4/silent.m4 + handle_silent + AM_V_* vars):

- m4/sbom.m4 (new): AC_ARG_ENABLE([sbom]), default disabled,
  mirroring m4/silent.m4.
- m4/init.m4: one AC_REQUIRE line next to _AM_SILENT_RULES.
- bin/automake.in: handle_sbom after handle_silent; defines
  am__sbom_components (programs/libraries of the Makefile) and pulls
  in lib/am/sbom.am.
- lib/am/sbom.am (new): sbom target + sbom-am rule; a printf-based
  shell writer emits the protobom wire format (varint tags +
  length-delimited strings). No new dependency, no Perl serialization,
  no C runtime to vendor.

Wire format: Document{metadata=1,node_list=2}, Metadata{name=3,
date=4,tools=5,authors=6}, Timestamp{seconds=1}, Tool{name=1,
version=2}, NodeList{nodes=1,edges=2,root_elements=3}, Node{id=1,
name=3,version=4}, Edge{type=1,from=2,to=3}, type 10=dependsOn.
Root node is $(PACKAGE)@$(VERSION) (distdir.am precedent); one
dependsOn edge per component; sorted by name; timestamp honours
SOURCE_DATE_EPOCH.

Testing: ran automake on a small package, 'make sbom' produced a
97-byte file that 'protoc --decode=protobom.protobom.Document' parses
cleanly (metadata, two nodes, one edge). The sbom target exists in
every Makefile (like tags); --enable-sbom only gates whether all-am
depends on it, which keeps the gating logic out of automake.in.

Documented gaps, omitted rather than fabricated: purl (no standard
purl type for autotools packages), hash, license, supplier (no data
in automake's model).

Two shell gotchas solved along the way, noted in the recipe comments:
make expands '$m' inside '$meta' (use $$meta), and command
substitution strips trailing newlines (pack \012 mid-string).

Thanks,
Dinesh
