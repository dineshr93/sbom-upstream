PACKAGE = sbom-demo
VERSION = 1.0
am__sbom_components = alpha beta
AM_SBOM_TARGET = sbom.protobom.bin
include ../../impl/autotools/sbom.am

all: app

app: main.c
	$(CC) $(CFLAGS) -o app main.c
