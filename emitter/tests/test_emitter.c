/* test_emitter.c — self-check for the emitter: build, write, verify determinism.
 *
 * SPDX-License-Identifier: MIT
 */
#include "sbom_emitter.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int same_file(const char *a, const char *b) {
  FILE *fa = fopen(a, "rb"), *fb = fopen(b, "rb");
  if (!fa || !fb) return 0;
  int same = 1;
  for (;;) {
    int ca = fgetc(fa), cb = fgetc(fb);
    if (ca != cb) {
      same = 0;
      break;
    }
    if (ca == EOF) break;
  }
  fclose(fa);
  fclose(fb);
  return same;
}

int main(int argc, char **argv) {
  const char *out = argc > 1 ? argv[1] : "sbom.protobom.bin";
  char out2[512];
  snprintf(out2, sizeof out2, "%s.again", out);

  sbom_emitter *e = sbom_init("cmake", "3.30.0");
  assert(e);
  assert(sbom_set_timestamp(e, 1700000000) == 0);
  assert(sbom_set_author(e, "CMake", NULL, 1) == 0);

  const char *app = sbom_add_node(e, "pkg:generic/app@1.0", "app", "1.0", "ACME", "MIT", NULL);
  const char *zlib = sbom_add_node(e, "pkg:generic/zlib@1.3", "zlib", "1.3", "zlib-ng", "Zlib", "sha256:deadbeef");
  const char *ssl = sbom_add_node(e, NULL, "openssl", "3.0.0", NULL, "Apache-2.0", NULL);
  assert(app && zlib && ssl);
  assert(strcmp(ssl, "openssl@3.0.0") == 0);

  assert(sbom_add_node(e, NULL, "bad", "1", NULL, NULL, "sha999:abc") == NULL);
  assert(strcmp(sbom_last_error(e), "unknown hash algorithm") == 0);

  const char *hashed = sbom_add_node(e, "pkg:generic/x@2", "x", "2", NULL, NULL, "blake2b-512:abcd");
  assert(hashed);
  const char *app2 = sbom_add_node(e, "pkg:generic/app@1.0", "app2", "1.0", "ACME2", "MIT", NULL);
  assert(app2 && strcmp(app2, app) == 0);

  assert(sbom_add_edge(e, app, zlib) == 0);
  assert(sbom_add_edge(e, zlib, ssl) == 0);
  assert(sbom_add_edge(e, "missing", app) == -1);
  assert(sbom_set_root(e, app) == 0);
  assert(sbom_set_root(e, hashed) == 0);
  assert(sbom_set_root(e, "missing") == -1);

  assert(sbom_write(e, out) == 0);
  assert(sbom_write(e, out2) == 0);
  assert(same_file(out, out2));
  remove(out2);

  sbom_free(e);
  puts("ok");
  return 0;
}
