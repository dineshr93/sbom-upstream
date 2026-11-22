/* sbom_emitter.h — minimal embeddable SBOM emitter (protobom protobuf).
 *
 * SPDX-License-Identifier: MIT
 *
 * C99, no dependencies beyond libc. Produces a protobom Document with the
 * NTIA minimum elements: supplier, name, version, identifier (purl), dependency
 * relationships, author of SBOM data, timestamp.
 *
 * Usage:
 *   sbom_emitter *e = sbom_init("cmake", "3.30.0");
 *   sbom_add_node(e, "pkg:generic/zlib@1.3", "zlib", "1.3", "zlib-ng", "Zlib", "sha256:abc...");
 *   sbom_add_edge(e, "pkg:generic/app@1.0", "pkg:generic/zlib@1.3");
 *   sbom_write(e, "sbom.protobom.bin");
 *   sbom_free(e);
 */
#ifndef SBOM_EMITTER_H
#define SBOM_EMITTER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sbom_emitter sbom_emitter;

/* Create an emitter. tool_name/tool_version identify the build system that
 * produced the document (NTIA "author of SBOM data"). Returns NULL on OOM. */
sbom_emitter *sbom_init(const char *tool_name, const char *tool_version);

void sbom_free(sbom_emitter *e);

/* Add a component. Any argument may be NULL or "" to omit that field; nothing
 * is fabricated. `hash` is "<alg>:<hex>" for a protobom HashAlgorithm name, or
 * a bare hex string (assumed sha256). Unknown algorithms are rejected. Returns
 * the node id (purl when given, else name@version, else name), owned by the
 * emitter and valid until sbom_free(). NULL on error. */
const char *sbom_add_node(sbom_emitter *e, const char *purl, const char *name,
                          const char *version, const char *supplier,
                          const char *license, const char *hash);

/* Add a dependsOn edge. Both ids must name nodes added by sbom_add_node. */
int sbom_add_edge(sbom_emitter *e, const char *from, const char *to);

/* Mark an existing node as a root element of the graph. */
int sbom_set_root(sbom_emitter *e, const char *id);

/* Optional document author (person or organisation). */
int sbom_set_author(sbom_emitter *e, const char *name, const char *email,
                    int is_org);

/* Pin the document timestamp (unix seconds). Default: SOURCE_DATE_EPOCH if
 * set, else time(NULL). Pin it for reproducible builds. */
int sbom_set_timestamp(sbom_emitter *e, int64_t unix_seconds);

/* Serialise to `path`. Nodes, edges and root elements are sorted, so identical
 * input yields byte-identical output. Returns 0 on success, -1 on error. */
int sbom_write(sbom_emitter *e, const char *path);

/* Last error message, or "" when none. */
const char *sbom_last_error(const sbom_emitter *e);

#ifdef __cplusplus
}
#endif

#endif /* SBOM_EMITTER_H */
