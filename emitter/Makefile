CC ?= cc
CFLAGS ?= -O2 -std=c99 -Wall -Wextra -pedantic
PREFIX ?= /usr/local

all: libsbom_emitter.a libsbom_emitter.so

build/sbom_emitter.o: src/sbom_emitter.c include/sbom_emitter.h
	@mkdir -p build
	$(CC) $(CFLAGS) -Iinclude -fPIC -c $< -o $@

libsbom_emitter.a: build/sbom_emitter.o
	ar rcs $@ $<

libsbom_emitter.so: build/sbom_emitter.o
	$(CC) -shared -o $@ $<

test: all
	$(CC) $(CFLAGS) -Iinclude tests/test_emitter.c libsbom_emitter.a -o build/test_emitter
	./build/test_emitter build/test.protobom.bin
	sh tests/conformance.sh build/test.protobom.bin

test-bindings: all
	sh tests/bindings.sh

install: all
	install -d $(DESTDIR)$(PREFIX)/include $(DESTDIR)$(PREFIX)/lib
	install -m 644 include/sbom_emitter.h $(DESTDIR)$(PREFIX)/include/
	install -m 644 libsbom_emitter.a $(DESTDIR)$(PREFIX)/lib/
	install -m 755 libsbom_emitter.so $(DESTDIR)$(PREFIX)/lib/

clean:
	rm -rf build libsbom_emitter.a libsbom_emitter.so

.PHONY: all test install clean
