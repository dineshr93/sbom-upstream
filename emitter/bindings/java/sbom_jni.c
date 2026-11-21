/* sbom_jni.c — JNI shim for SbomEmitter.java.
 *
 * SPDX-License-Identifier: MIT
 */
#include <jni.h>

#include "sbom_emitter.h"

static const char *jstr(JNIEnv *env, jstring s) {
  return s ? (*env)->GetStringUTFChars(env, s, NULL) : NULL;
}

static void jfree(JNIEnv *env, jstring s, const char *c) {
  if (s && c) (*env)->ReleaseStringUTFChars(env, s, c);
}

JNIEXPORT jlong JNICALL Java_SbomEmitter_init(JNIEnv *env, jclass cls,
                                              jstring tool, jstring version) {
  (void)cls;
  const char *ct = jstr(env, tool), *cv = jstr(env, version);
  sbom_emitter *e = sbom_init(ct, cv);
  jfree(env, tool, ct);
  jfree(env, version, cv);
  return (jlong)(intptr_t)e;
}

JNIEXPORT void JNICALL Java_SbomEmitter_free(JNIEnv *env, jclass cls, jlong h) {
  (void)env;
  (void)cls;
  sbom_free((sbom_emitter *)(intptr_t)h);
}

JNIEXPORT jstring JNICALL Java_SbomEmitter_addNode0(
    JNIEnv *env, jclass cls, jlong h, jstring purl, jstring name, jstring version,
    jstring supplier, jstring license, jstring hash) {
  (void)cls;
  const char *cp = jstr(env, purl), *cn = jstr(env, name), *cv = jstr(env, version);
  const char *cs = jstr(env, supplier), *cl = jstr(env, license), *ch = jstr(env, hash);
  const char *id = sbom_add_node((sbom_emitter *)(intptr_t)h, cp, cn, cv, cs, cl, ch);
  jstring out = id ? (*env)->NewStringUTF(env, id) : NULL;
  jfree(env, purl, cp);
  jfree(env, name, cn);
  jfree(env, version, cv);
  jfree(env, supplier, cs);
  jfree(env, license, cl);
  jfree(env, hash, ch);
  return out;
}

JNIEXPORT jint JNICALL Java_SbomEmitter_addEdge0(JNIEnv *env, jclass cls, jlong h,
                                                 jstring from, jstring to) {
  (void)cls;
  const char *cf = jstr(env, from), *ct = jstr(env, to);
  int rc = sbom_add_edge((sbom_emitter *)(intptr_t)h, cf, ct);
  jfree(env, from, cf);
  jfree(env, to, ct);
  return rc;
}

JNIEXPORT jint JNICALL Java_SbomEmitter_setRoot0(JNIEnv *env, jclass cls, jlong h,
                                                 jstring id) {
  (void)cls;
  const char *ci = jstr(env, id);
  int rc = sbom_set_root((sbom_emitter *)(intptr_t)h, ci);
  jfree(env, id, ci);
  return rc;
}

JNIEXPORT jint JNICALL Java_SbomEmitter_setAuthor0(JNIEnv *env, jclass cls, jlong h,
                                                   jstring name, jstring email,
                                                   jboolean is_org) {
  (void)cls;
  const char *cn = jstr(env, name), *ce = jstr(env, email);
  int rc = sbom_set_author((sbom_emitter *)(intptr_t)h, cn, ce, is_org ? 1 : 0);
  jfree(env, name, cn);
  jfree(env, email, ce);
  return rc;
}

JNIEXPORT jint JNICALL Java_SbomEmitter_setTimestamp0(JNIEnv *env, jclass cls,
                                                      jlong h, jlong seconds) {
  (void)env;
  (void)cls;
  return sbom_set_timestamp((sbom_emitter *)(intptr_t)h, (int64_t)seconds);
}

JNIEXPORT jint JNICALL Java_SbomEmitter_write0(JNIEnv *env, jclass cls, jlong h,
                                               jstring path) {
  (void)cls;
  const char *cp = jstr(env, path);
  int rc = sbom_write((sbom_emitter *)(intptr_t)h, cp);
  jfree(env, path, cp);
  return rc;
}

JNIEXPORT jstring JNICALL Java_SbomEmitter_lastError(JNIEnv *env, jclass cls,
                                                     jlong h) {
  (void)cls;
  return (*env)->NewStringUTF(env, sbom_last_error((sbom_emitter *)(intptr_t)h));
}
