"""sbom_emitter — ctypes binding for the C emitter.

SPDX-License-Identifier: MIT

Set SBOM_EMITTER_LIB to the path of libsbom_emitter.so if it is not on the
loader path.
"""
import ctypes
import os

_lib = ctypes.CDLL(os.environ.get("SBOM_EMITTER_LIB", "libsbom_emitter.so"))

_lib.sbom_init.restype = ctypes.c_void_p
_lib.sbom_init.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
_lib.sbom_free.argtypes = [ctypes.c_void_p]
_lib.sbom_add_node.restype = ctypes.c_char_p
_lib.sbom_add_node.argtypes = [ctypes.c_void_p] + [ctypes.c_char_p] * 6
_lib.sbom_add_edge.restype = ctypes.c_int
_lib.sbom_add_edge.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_char_p]
_lib.sbom_set_root.restype = ctypes.c_int
_lib.sbom_set_root.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
_lib.sbom_set_author.restype = ctypes.c_int
_lib.sbom_set_author.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_int]
_lib.sbom_set_timestamp.restype = ctypes.c_int
_lib.sbom_set_timestamp.argtypes = [ctypes.c_void_p, ctypes.c_int64]
_lib.sbom_write.restype = ctypes.c_int
_lib.sbom_write.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
_lib.sbom_last_error.restype = ctypes.c_char_p
_lib.sbom_last_error.argtypes = [ctypes.c_void_p]


def _b(s):
    return None if s is None else s.encode()


class Emitter:
    def __init__(self, tool, version):
        self._h = _lib.sbom_init(_b(tool), _b(version))
        if not self._h:
            raise MemoryError("sbom_init failed")

    def close(self):
        if self._h:
            _lib.sbom_free(self._h)
            self._h = None

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        self.close()

    def __del__(self):
        self.close()

    def error(self):
        return _lib.sbom_last_error(self._h).decode()

    def add_node(self, purl=None, name=None, version=None, supplier=None,
                 license=None, hash=None):
        r = _lib.sbom_add_node(self._h, _b(purl), _b(name), _b(version),
                               _b(supplier), _b(license), _b(hash))
        if not r:
            raise RuntimeError(self.error())
        return r.decode()

    def add_edge(self, from_id, to_id):
        if _lib.sbom_add_edge(self._h, _b(from_id), _b(to_id)) != 0:
            raise RuntimeError(self.error())

    def set_root(self, node_id):
        if _lib.sbom_set_root(self._h, _b(node_id)) != 0:
            raise RuntimeError(self.error())

    def set_author(self, name, email=None, is_org=True):
        if _lib.sbom_set_author(self._h, _b(name), _b(email), int(is_org)) != 0:
            raise RuntimeError(self.error())

    def set_timestamp(self, unix_seconds):
        if _lib.sbom_set_timestamp(self._h, int(unix_seconds)) != 0:
            raise RuntimeError(self.error())

    def write(self, path):
        if _lib.sbom_write(self._h, _b(path)) != 0:
            raise RuntimeError(self.error())
