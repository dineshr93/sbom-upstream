/* sbom_emitter.c — minimal protobom protobuf writer.
 *
 * SPDX-License-Identifier: MIT
 *
 * Hand-rolled proto3 wire encoder: varint + length-delimited only, which is
 * all the protobom schema needs. No libprotobuf, no generated code.
 */
#include "sbom_emitter.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ---- growable byte buffer ------------------------------------------------ */

typedef struct {
  unsigned char *p;
  size_t n, cap;
} buf;

static int b_grow(buf *b, size_t need) {
  if (b->n + need <= b->cap) return 0;
  size_t cap = b->cap ? b->cap : 256;
  while (cap < b->n + need) cap *= 2;
  unsigned char *p = realloc(b->p, cap);
  if (!p) return -1;
  b->p = p;
  b->cap = cap;
  return 0;
}

static int b_put(buf *b, const void *src, size_t n) {
  if (b_grow(b, n)) return -1;
  if (n) memcpy(b->p + b->n, src, n);
  b->n += n;
  return 0;
}

static int pb_varint(buf *b, uint64_t v) {
  unsigned char tmp[10];
  size_t i = 0;
  do {
    tmp[i++] = (unsigned char)((v & 0x7f) | (v > 0x7f ? 0x80 : 0));
    v >>= 7;
  } while (v);
  return b_put(b, tmp, i);
}

static int pb_tag(buf *b, unsigned field, unsigned wire) {
  return pb_varint(b, ((uint64_t)field << 3) | wire);
}

static int pb_bytes(buf *b, unsigned field, const void *p, size_t n) {
  if (pb_tag(b, field, 2)) return -1;
  if (pb_varint(b, n)) return -1;
  return b_put(b, p, n);
}

static int pb_str(buf *b, unsigned field, const char *s) {
  if (!s || !*s) return 0;
  return pb_bytes(b, field, s, strlen(s));
}

static int pb_int(buf *b, unsigned field, uint64_t v) {
  if (!v) return 0;
  if (pb_tag(b, field, 0)) return -1;
  return pb_varint(b, v);
}

/* Append `sub` as a length-delimited message and release it. */
static int pb_msg(buf *b, unsigned field, buf *sub) {
  int rc = pb_bytes(b, field, sub->p, sub->n);
  free(sub->p);
  sub->p = NULL;
  sub->n = sub->cap = 0;
  return rc;
}

/* ---- model --------------------------------------------------------------- */

typedef struct {
  char *id, *name, *version, *supplier, *license, *purl, *hash;
  unsigned hash_alg;
} node;

typedef struct {
  char *from, *to;
} edge;

struct sbom_emitter {
  char *tool_name, *tool_version;
  char *author, *author_email;
  int author_is_org;
  int64_t timestamp;
  node *nodes;
  size_t nnodes, capnodes;
  edge *edges;
  size_t nedges, capedges;
  char **roots;
  size_t nroots, caproots;
  char err[256];
};

static char *dup_or_null(const char *s) {
  if (!s || !*s) return NULL;
  size_t n = strlen(s) + 1;
  char *p = malloc(n);
  if (p) memcpy(p, s, n);
  return p;
}

static void set_err(sbom_emitter *e, const char *msg) {
  if (!e) return;
  snprintf(e->err, sizeof e->err, "%s", msg);
}

/* protobom HashAlgorithm enum values */
static unsigned hash_alg_from_name(const char *name) {
  static const struct {
    const char *name;
    unsigned value;
  } map[] = {
      {"md5", 1},       {"sha1", 2},      {"sha256", 3},   {"sha384", 4},
      {"sha512", 5},    {"sha3-256", 6},  {"sha3_256", 6}, {"sha3-384", 7},
      {"sha3_512", 8},  {"blake2b-256", 9}, {"blake3", 12},
  };
  for (size_t i = 0; i < sizeof map / sizeof map[0]; i++)
    if (strcmp(map[i].name, name) == 0) return map[i].value;
  return 3; /* sha256 */
}

/* ---- message builders ---------------------------------------------------- */

static int build_timestamp(buf *out, unsigned field, int64_t secs) {
  buf t = {0};
  if (pb_int(&t, 1, (uint64_t)secs)) {
    free(t.p);
    return -1;
  }
  return pb_msg(out, field, &t);
}

static int build_person(buf *out, unsigned field, const char *name,
                        const char *email, int is_org) {
  buf p = {0};
  int rc = 0;
  rc |= pb_str(&p, 1, name);
  rc |= pb_int(&p, 2, is_org ? 1 : 0);
  rc |= pb_str(&p, 3, email);
  if (rc) {
    free(p.p);
    return -1;
  }
  return pb_msg(out, field, &p);
}

static int build_tool(buf *out, const char *name, const char *version) {
  buf t = {0};
  int rc = 0;
  rc |= pb_str(&t, 1, name);
  rc |= pb_str(&t, 2, version);
  if (rc) {
    free(t.p);
    return -1;
  }
  return pb_msg(out, 5, &t);
}

static int build_node(buf *out, const node *n) {
  buf m = {0};
  int rc = 0;
  rc |= pb_str(&m, 1, n->id);
  rc |= pb_str(&m, 3, n->name);
  rc |= pb_str(&m, 4, n->version);
  rc |= pb_str(&m, 8, n->license);
  if (n->supplier) rc |= build_person(&m, 19, n->supplier, NULL, 1);
  if (n->purl) {
    buf kv = {0};
    rc |= pb_int(&kv, 1, 1); /* SoftwareIdentifierType.PURL */
    rc |= pb_str(&kv, 2, n->purl);
    rc |= pb_msg(&m, 28, &kv);
  }
  if (n->hash) {
    buf kv = {0};
    rc |= pb_int(&kv, 1, n->hash_alg);
    rc |= pb_str(&kv, 2, n->hash);
    rc |= pb_msg(&m, 29, &kv);
  }
  if (rc) {
    free(m.p);
    return -1;
  }
  return pb_msg(out, 1, &m);
}

static int build_edge(buf *out, const edge *e) {
  buf m = {0};
  int rc = 0;
  rc |= pb_int(&m, 1, 10); /* Edge.Type.dependsOn */
  rc |= pb_str(&m, 2, e->from);
  rc |= pb_str(&m, 3, e->to);
  if (rc) {
    free(m.p);
    return -1;
  }
  return pb_msg(out, 2, &m);
}

static int cmp_node(const void *a, const void *b) {
  const node *x = *(const node *const *)a;
  const node *y = *(const node *const *)b;
  return strcmp(x->id, y->id);
}

static int cmp_edge(const void *a, const void *b) {
  const edge *x = a;
  const edge *y = b;
  int c = strcmp(x->from, y->from);
  return c ? c : strcmp(x->to, y->to);
}

static int build_document(const sbom_emitter *e, buf *out) {
  buf meta = {0}, nl = {0};
  int rc = 0;

  rc |= pb_str(&meta, 3, e->tool_name);
  rc |= build_timestamp(&meta, 4, e->timestamp);
  rc |= build_tool(&meta, e->tool_name, e->tool_version);
  if (e->author) rc |= build_person(&meta, 6, e->author, e->author_email, e->author_is_org);

  node **sorted = NULL;
  if (e->nnodes) {
    sorted = malloc(e->nnodes * sizeof *sorted);
    if (!sorted) {
      free(meta.p);
      return -1;
    }
    for (size_t i = 0; i < e->nnodes; i++) sorted[i] = &e->nodes[i];
    qsort(sorted, e->nnodes, sizeof *sorted, cmp_node);
    for (size_t i = 0; i < e->nnodes; i++) rc |= build_node(&nl, sorted[i]);
    free(sorted);
  }

  if (e->nedges) {
    edge *sorted_edges = malloc(e->nedges * sizeof *sorted_edges);
    if (!sorted_edges) {
      free(meta.p);
      free(nl.p);
      return -1;
    }
    memcpy(sorted_edges, e->edges, e->nedges * sizeof *sorted_edges);
    qsort(sorted_edges, e->nedges, sizeof *sorted_edges, cmp_edge);
    for (size_t i = 0; i < e->nedges; i++) rc |= build_edge(&nl, &sorted_edges[i]);
    free(sorted_edges);
  }

  for (size_t i = 0; i < e->nroots; i++) rc |= pb_str(&nl, 3, e->roots[i]);

  if (rc) {
    free(meta.p);
    free(nl.p);
    return -1;
  }
  rc |= pb_msg(out, 1, &meta);
  rc |= pb_msg(out, 2, &nl);
  return rc ? -1 : 0;
}

/* ---- public API ---------------------------------------------------------- */

sbom_emitter *sbom_init(const char *tool_name, const char *tool_version) {
  sbom_emitter *e = calloc(1, sizeof *e);
  if (!e) return NULL;
  e->tool_name = dup_or_null(tool_name);
  e->tool_version = dup_or_null(tool_version);
  const char *sde = getenv("SOURCE_DATE_EPOCH");
  e->timestamp = sde && *sde ? (int64_t)strtoll(sde, NULL, 10) : (int64_t)time(NULL);
  return e;
}

void sbom_free(sbom_emitter *e) {
  if (!e) return;
  for (size_t i = 0; i < e->nnodes; i++) {
    node *n = &e->nodes[i];
    free(n->id);
    free(n->name);
    free(n->version);
    free(n->supplier);
    free(n->license);
    free(n->purl);
    free(n->hash);
  }
  for (size_t i = 0; i < e->nedges; i++) {
    free(e->edges[i].from);
    free(e->edges[i].to);
  }
  for (size_t i = 0; i < e->nroots; i++) free(e->roots[i]);
  free(e->nodes);
  free(e->edges);
  free(e->roots);
  free(e->tool_name);
  free(e->tool_version);
  free(e->author);
  free(e->author_email);
  free(e);
}

const char *sbom_add_node(sbom_emitter *e, const char *purl, const char *name,
                          const char *version, const char *supplier,
                          const char *license, const char *hash) {
  if (!e) return NULL;
  if (e->nnodes == e->capnodes) {
    size_t cap = e->capnodes ? e->capnodes * 2 : 16;
    node *p = realloc(e->nodes, cap * sizeof *p);
    if (!p) {
      set_err(e, "out of memory");
      return NULL;
    }
    e->nodes = p;
    e->capnodes = cap;
  }
  node *n = &e->nodes[e->nnodes];
  memset(n, 0, sizeof *n);
  n->purl = dup_or_null(purl);
  n->name = dup_or_null(name);
  n->version = dup_or_null(version);
  n->supplier = dup_or_null(supplier);
  n->license = dup_or_null(license);

  if (hash && *hash) {
    const char *colon = strchr(hash, ':');
    if (colon) {
      char alg[32];
      size_t len = (size_t)(colon - hash);
      if (len >= sizeof alg) len = sizeof alg - 1;
      memcpy(alg, hash, len);
      alg[len] = '\0';
      n->hash_alg = hash_alg_from_name(alg);
      n->hash = dup_or_null(colon + 1);
    } else {
      n->hash_alg = 3; /* sha256 */
      n->hash = dup_or_null(hash);
    }
  }

  if (n->purl) {
    n->id = dup_or_null(n->purl);
  } else if (n->name && n->version) {
    size_t len = strlen(n->name) + strlen(n->version) + 2;
    n->id = malloc(len);
    if (n->id) snprintf(n->id, len, "%s@%s", n->name, n->version);
  } else {
    n->id = dup_or_null(n->name);
  }
  if (!n->id) {
    set_err(e, "node needs a purl or a name");
    return NULL;
  }
  e->nnodes++;
  return n->id;
}

int sbom_add_edge(sbom_emitter *e, const char *from, const char *to) {
  if (!e || !from || !to) return -1;
  if (e->nedges == e->capedges) {
    size_t cap = e->capedges ? e->capedges * 2 : 16;
    edge *p = realloc(e->edges, cap * sizeof *p);
    if (!p) {
      set_err(e, "out of memory");
      return -1;
    }
    e->edges = p;
    e->capedges = cap;
  }
  e->edges[e->nedges].from = dup_or_null(from);
  e->edges[e->nedges].to = dup_or_null(to);
  if (!e->edges[e->nedges].from || !e->edges[e->nedges].to) {
    set_err(e, "out of memory");
    return -1;
  }
  e->nedges++;
  return 0;
}

int sbom_set_root(sbom_emitter *e, const char *id) {
  if (!e || !id || !*id) return -1;
  if (e->nroots == e->caproots) {
    size_t cap = e->caproots ? e->caproots * 2 : 4;
    char **p = realloc(e->roots, cap * sizeof *p);
    if (!p) {
      set_err(e, "out of memory");
      return -1;
    }
    e->roots = p;
    e->caproots = cap;
  }
  e->roots[e->nroots] = dup_or_null(id);
  if (!e->roots[e->nroots]) {
    set_err(e, "out of memory");
    return -1;
  }
  e->nroots++;
  return 0;
}

int sbom_set_author(sbom_emitter *e, const char *name, const char *email,
                    int is_org) {
  if (!e) return -1;
  free(e->author);
  free(e->author_email);
  e->author = dup_or_null(name);
  e->author_email = dup_or_null(email);
  e->author_is_org = is_org;
  return 0;
}

int sbom_set_timestamp(sbom_emitter *e, int64_t unix_seconds) {
  if (!e) return -1;
  e->timestamp = unix_seconds;
  return 0;
}

int sbom_write(sbom_emitter *e, const char *path) {
  if (!e || !path) return -1;
  buf out = {0};
  if (build_document(e, &out)) {
    free(out.p);
    set_err(e, "failed to encode document");
    return -1;
  }
  FILE *f = fopen(path, "wb");
  if (!f) {
    free(out.p);
    set_err(e, "cannot open output file");
    return -1;
  }
  size_t wrote = out.n ? fwrite(out.p, 1, out.n, f) : 0;
  int close_rc = fclose(f);
  free(out.p);
  if (wrote != out.n || close_rc != 0) {
    set_err(e, "short write");
    return -1;
  }
  return 0;
}

const char *sbom_last_error(const sbom_emitter *e) {
  return e ? e->err : "no emitter";
}
