project(nanolibc)

set(nanolibc_sources
	libc.cpp
	memory.cpp
	printf.cpp
	putchar.cpp
)

add_library(nanolibc OBJECT ${nanolibc_sources})

