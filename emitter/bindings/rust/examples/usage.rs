// usage.rs — 30-line example: emit an SBOM from a Rust build system.
//
// SPDX-License-Identifier: MIT
//
// Run: cargo run --example usage   (from emitter/bindings/rust, after `make`)
use sbom_emitter::Emitter;

fn main() -> Result<(), String> {
    let mut e = Emitter::new("mybuild", "1.0")?;
    e.set_author("MyBuild", None, true)?;
    e.set_timestamp(1700000000)?; // pin for reproducible output

    let app = e.add_node(
        Some("pkg:generic/app@1.0"), Some("app"), Some("1.0"),
        Some("ACME"), Some("MIT"), None,
    )?;
    let zlib = e.add_node(
        Some("pkg:generic/zlib@1.3"), Some("zlib"), Some("1.3"),
        Some("zlib-ng"), Some("Zlib"), Some("sha256:deadbeef"),
    )?;

    e.add_edge(&app, &zlib)?;
    e.set_root(&app)?;
    e.write("sbom.protobom.bin")
}
