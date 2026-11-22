##                                                          -*- Autoconf -*-
# Copyright (C) 2026 Free Software Foundation, Inc.
#
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# _AM_SBOM
# --------
# Enable 'make sbom' SBOM generation support.
AC_DEFUN([_AM_SBOM],
[AC_ARG_ENABLE([sbom], [AS_HELP_STRING([--enable-sbom],
  [generate sbom.protobom.bin as part of the default build])])dnl
AM_SBOM_TARGET=
AS_IF([test "x$enable_sbom" = xyes],
  [AM_SBOM_TARGET=sbom.protobom.bin])
AC_SUBST([AM_SBOM_TARGET])dnl
])
