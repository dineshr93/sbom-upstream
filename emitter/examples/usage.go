// usage.go — 30-line example: emit an SBOM from a Go build system.
//
// SPDX-License-Identifier: MIT
//
// Run: go run usage.go   (from emitter/examples, with ../libsbom_emitter.a built)
package main

import (
	"fmt"
	"os"

	sbom "github.com/dineshr93/sbom-upstream/emitter/bindings/go"
)

func main() {
	e, err := sbom.New("mybuild", "1.0")
	if err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(1)
	}
	defer e.Close()

	e.SetAuthor("MyBuild", "", true)
	e.SetTimestamp(1700000000) // pin for reproducible output

	app, err := e.AddNode("pkg:generic/app@1.0", "app", "1.0", "ACME", "MIT", "")
	if err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(1)
	}
	zlib, err := e.AddNode("pkg:generic/zlib@1.3", "zlib", "1.3", "zlib-ng", "Zlib", "sha256:deadbeef")
	if err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(1)
	}

	e.AddEdge(app, zlib)
	e.SetRoot(app)
	if err := e.Write("sbom.protobom.bin"); err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(1)
	}
}
