/* usage.c — 30-line example: emit an SBOM from a C build system.
 *
 * SPDX-License-Identifier: MIT
 *
 * Build: cc -I../include usage.c ../libsbom_emitter.a -o usage
 */
#include "sbom_emitter.h"

#include <stdio.h>

int main(void) {
  sbom_emitter *e = sbom_init("mybuild", "1.0");
  if (!e) return 1;

  sbom_set_author(e, "MyBuild", NULL, 1);
  sbom_set_timestamp(e, 1700000000); /* pin for reproducible output */

  const char *app = sbom_add_node(e, "pkg:generic/app@1.0", "app", "1.0",
                                  "ACME", "MIT", NULL);
  const char *zlib = sbom_add_node(e, "pkg:generic/zlib@1.3", "zlib", "1.3",
                                   "zlib-ng", "Zlib", "sha256:deadbeef");
  if (!app || !zlib) {
    fprintf(stderr, "%s\n", sbom_last_error(e));
    sbom_free(e);
    return 1;
  }

  sbom_add_edge(e, app, zlib);
  sbom_set_root(e, app);

  int rc = sbom_write(e, "sbom.protobom.bin");
  if (rc != 0) fprintf(stderr, "%s\n", sbom_last_error(e));
  sbom_free(e);
  return rc != 0;
}
