project(quickjs)

# This is QuickJS by Fabrice Bellard
# Version 2019-07-09
# https://bellard.org/quickjs/

add_definitions(-D_GNU_SOURCE -DCONFIG_BIGNUM -DCONFIG_VERSION="2019-07-09")

add_library(quickjs OBJECT
	cutils.c
	libbf.c
	libregexp.c
	libunicode.c
	quickjs-libc.c
	quickjs.c
)
