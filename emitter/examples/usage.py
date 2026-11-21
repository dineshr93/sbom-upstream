"""usage.py — 30-line example: emit an SBOM from a Python build system.

SPDX-License-Identifier: MIT

Run: SBOM_EMITTER_LIB=../libsbom_emitter.so python3 usage.py
"""
import sys

sys.path.insert(0, "../bindings/python")
from sbom_emitter import Emitter  # noqa: E402

with Emitter("mybuild", "1.0") as e:
    e.set_author("MyBuild", is_org=True)
    e.set_timestamp(1700000000)  # pin for reproducible output

    app = e.add_node(purl="pkg:generic/app@1.0", name="app", version="1.0",
                     supplier="ACME", license="MIT")
    zlib = e.add_node(purl="pkg:generic/zlib@1.3", name="zlib", version="1.3",
                      supplier="zlib-ng", license="Zlib", hash="sha256:deadbeef")

    e.add_edge(app, zlib)
    e.set_root(app)
    e.write("sbom.protobom.bin")
