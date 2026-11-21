// Package sbomemitter is a thin cgo binding for the C SBOM emitter.
//
// SPDX-License-Identifier: MIT
package sbomemitter

/*
#cgo CFLAGS: -I${SRCDIR}/../../include
#cgo LDFLAGS: ${SRCDIR}/../../libsbom_emitter.a
#include <stdlib.h>
#include "sbom_emitter.h"
*/
import "C"

import (
	"errors"
	"unsafe"
)

type Emitter struct{ h *C.sbom_emitter }

func New(tool, version string) (*Emitter, error) {
	ct, cv := C.CString(tool), C.CString(version)
	defer C.free(unsafe.Pointer(ct))
	defer C.free(unsafe.Pointer(cv))
	h := C.sbom_init(ct, cv)
	if h == nil {
		return nil, errors.New("sbom_init failed")
	}
	return &Emitter{h: h}, nil
}

func (e *Emitter) Close() {
	if e.h != nil {
		C.sbom_free(e.h)
		e.h = nil
	}
}

func (e *Emitter) err() error {
	return errors.New(C.GoString(C.sbom_last_error(e.h)))
}

func cstr(s string) *C.char {
	if s == "" {
		return nil
	}
	return C.CString(s)
}

func (e *Emitter) AddNode(purl, name, version, supplier, license, hash string) (string, error) {
	cp, cn, cv := cstr(purl), cstr(name), cstr(version)
	cs, cl, ch := cstr(supplier), cstr(license), cstr(hash)
	defer C.free(unsafe.Pointer(cp))
	defer C.free(unsafe.Pointer(cn))
	defer C.free(unsafe.Pointer(cv))
	defer C.free(unsafe.Pointer(cs))
	defer C.free(unsafe.Pointer(cl))
	defer C.free(unsafe.Pointer(ch))
	id := C.sbom_add_node(e.h, cp, cn, cv, cs, cl, ch)
	if id == nil {
		return "", e.err()
	}
	return C.GoString(id), nil
}

func (e *Emitter) AddEdge(from, to string) error {
	cf, ct := C.CString(from), C.CString(to)
	defer C.free(unsafe.Pointer(cf))
	defer C.free(unsafe.Pointer(ct))
	if C.sbom_add_edge(e.h, cf, ct) != 0 {
		return e.err()
	}
	return nil
}

func (e *Emitter) SetRoot(id string) error {
	ci := C.CString(id)
	defer C.free(unsafe.Pointer(ci))
	if C.sbom_set_root(e.h, ci) != 0 {
		return e.err()
	}
	return nil
}

func (e *Emitter) SetAuthor(name, email string, isOrg bool) error {
	cn, ce := cstr(name), cstr(email)
	defer C.free(unsafe.Pointer(cn))
	defer C.free(unsafe.Pointer(ce))
	org := C.int(0)
	if isOrg {
		org = 1
	}
	if C.sbom_set_author(e.h, cn, ce, org) != 0 {
		return e.err()
	}
	return nil
}

func (e *Emitter) SetTimestamp(unixSeconds int64) error {
	if C.sbom_set_timestamp(e.h, C.int64_t(unixSeconds)) != 0 {
		return e.err()
	}
	return nil
}

func (e *Emitter) Write(path string) error {
	cp := C.CString(path)
	defer C.free(unsafe.Pointer(cp))
	if C.sbom_write(e.h, cp) != 0 {
		return e.err()
	}
	return nil
}
