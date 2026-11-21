// usage.java — 30-line example: emit an SBOM from a Java build system.
//
// SPDX-License-Identifier: MIT
//
// Run: java -Djava.library.path=. usage
public class usage {
    public static void main(String[] args) {
        try (SbomEmitter e = new SbomEmitter("mybuild", "1.0")) {
            e.setAuthor("MyBuild", null, true);
            e.setTimestamp(1700000000L); // pin for reproducible output

            String app = e.addNode("pkg:generic/app@1.0", "app", "1.0",
                                   "ACME", "MIT", null);
            String zlib = e.addNode("pkg:generic/zlib@1.3", "zlib", "1.3",
                                    "zlib-ng", "Zlib", "sha256:deadbeef");

            e.addEdge(app, zlib);
            e.setRoot(app);
            e.write("sbom.protobom.bin");
        }
    }
}
