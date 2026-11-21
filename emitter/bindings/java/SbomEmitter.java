// Thin JNI binding for the C SBOM emitter.
//
// SPDX-License-Identifier: MIT
public final class SbomEmitter implements AutoCloseable {
    static {
        System.loadLibrary("sbom_emitter_jni");
    }

    private long handle;

    public SbomEmitter(String tool, String version) {
        handle = init(tool, version);
        if (handle == 0) {
            throw new IllegalStateException("sbom_init failed");
        }
    }

    public String addNode(String purl, String name, String version,
                          String supplier, String license, String hash) {
        String id = addNode0(handle, purl, name, version, supplier, license, hash);
        if (id == null) {
            throw new IllegalStateException(lastError(handle));
        }
        return id;
    }

    public void addEdge(String from, String to) {
        check(addEdge0(handle, from, to));
    }

    public void setRoot(String id) {
        check(setRoot0(handle, id));
    }

    public void setAuthor(String name, String email, boolean isOrg) {
        check(setAuthor0(handle, name, email, isOrg));
    }

    public void setTimestamp(long unixSeconds) {
        check(setTimestamp0(handle, unixSeconds));
    }

    public void write(String path) {
        check(write0(handle, path));
    }

    @Override
    public void close() {
        if (handle != 0) {
            free(handle);
            handle = 0;
        }
    }

    private void check(int rc) {
        if (rc != 0) {
            throw new IllegalStateException(lastError(handle));
        }
    }

    private static native long init(String tool, String version);
    private static native void free(long handle);
    private static native String addNode0(long handle, String purl, String name,
                                          String version, String supplier,
                                          String license, String hash);
    private static native int addEdge0(long handle, String from, String to);
    private static native int setRoot0(long handle, String id);
    private static native int setAuthor0(long handle, String name, String email, boolean isOrg);
    private static native int setTimestamp0(long handle, long unixSeconds);
    private static native int write0(long handle, String path);
    private static native String lastError(long handle);
}
