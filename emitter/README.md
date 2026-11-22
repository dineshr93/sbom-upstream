# Embeddable SBOM emitter

A small C99 library that writes a [protobom](https://github.com/protobom/protobom)
protobuf `Document` with the NTIA minimum elements. No dependencies beyond libc:
the proto3 wire format is encoded by hand (varint + length-delimited only).

Build systems that forbid new third-party libraries, or whose language has no
protobom binding (Meson, Autotools), can vendor `src/sbom_emitter.c` and
`include/sbom_emitter.h` and call it directly.

## Build and test

```sh
make            # libsbom_emitter.a, libsbom_emitter.so
make test       # unit self-check + protoc conformance against the protobom schema
make test-bindings   # C, Python, Go, Rust, Java produce byte-identical output
```

`make test` needs `protoc` and `google/protobuf/timestamp.proto` (shipped with
protoc). `make test-bindings` needs a Go, Rust and JDK toolchain.

## API

```c
sbom_emitter *e = sbom_init("cmake", "3.30.0");
sbom_set_author(e, "CMake", NULL, 1);
sbom_set_timestamp(e, 1700000000);          /* pin for reproducible builds */

const char *app  = sbom_add_node(e, "pkg:generic/app@1.0", "app", "1.0",
                                 "ACME", "MIT", NULL);
const char *zlib = sbom_add_node(e, "pkg:generic/zlib@1.3", "zlib", "1.3",
                                 "zlib-ng", "Zlib", "sha256:deadbeef");
sbom_add_edge(e, app, zlib);
sbom_set_root(e, app);
sbom_write(e, "sbom.protobom.bin");
sbom_free(e);
```

Every argument may be `NULL` or `""` to omit a field. Nothing is fabricated: if
the build system cannot supply a value, the field is absent from the document.

`hash` accepts `"<alg>:<hex>"` for any protobom `HashAlgorithm` name
(`md5`, `sha1`, `sha256`, `sha384`, `sha512`, `sha3-256`, `sha3-384`,
`sha3-512`, `blake2b-256`, `blake2b-384`, `blake2b-512`, `blake3`, `md2`,
`adler32`, `md4`, `md6`, `sha224`, `streebog-256`, `streebog-512`) or a bare
hex string, assumed sha256. Unknown algorithms are rejected, not relabelled.

## NTIA minimum elements

| NTIA element | protobom field | Source |
| --- | --- | --- |
| Supplier name | `Node.suppliers` (`Person`) | `supplier` argument |
| Component name | `Node.name` | `name` argument |
| Version | `Node.version` | `version` argument |
| Other unique identifiers | `Node.identifiers[PURL]` | `purl` argument |
| Dependency relationships | `Edge{type: dependsOn}` | `sbom_add_edge` |
| Author of SBOM data | `Metadata.authors`, `Metadata.tools` | `sbom_set_author`, `sbom_init` |
| Timestamp | `Metadata.date` | `sbom_set_timestamp`, `SOURCE_DATE_EPOCH`, else `time(NULL)` |

## Determinism

Nodes are sorted by id (ties by name/version/fields), edges by `(from, to)`,
and root elements by id before serialisation, so the same input yields
byte-identical output. Pin the timestamp with `sbom_set_timestamp` or
`SOURCE_DATE_EPOCH` to make the whole document reproducible.

## Bindings

| Language | Path | Mechanism |
| --- | --- | --- |
| C | `include/`, `src/` | direct |
| Python | `bindings/python/sbom_emitter.py` | ctypes |
| Go | `bindings/go/sbomemitter.go` | cgo, static link |
| Rust | `bindings/rust/` | `extern "C"`, static link |
| Java | `bindings/java/` | JNI |

Usage examples: `examples/usage.c`, `examples/usage.py`, `examples/usage.go`,
`bindings/rust/examples/usage.rs`, `examples/usage.java`.

## Gaps

The emitter writes what it is given. It does not resolve dependencies, compute
hashes, or infer licenses — the calling build system must supply those. Fields
it cannot populate are omitted, never guessed.

## License

MIT. `tests/proto/sbom.proto` is copied from protobom and remains Apache-2.0.
