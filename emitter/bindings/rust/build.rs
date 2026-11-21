// Link against the C emitter built by `make` in the emitter root.
fn main() {
    let root = std::path::Path::new(env!("CARGO_MANIFEST_DIR")).join("../..");
    println!("cargo:rustc-link-search=native={}", root.display());
    println!("cargo:rustc-link-lib=static=sbom_emitter");
    println!("cargo:rerun-if-changed=../../libsbom_emitter.a");
}
