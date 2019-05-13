#ifndef __ALLTYPES_H
#define __ALLTYPES_H

#ifdef __wasm32__
#define _Addr int
#define _Int64 long long
#define _Reg int

#elif defined __wasm64__
#define _Addr long
#define _Int64 long long
#define _Reg long

#endif

typedef __builtin_va_list va_list;
typedef __builtin_va_list __isoc_va_list;

#ifndef __cplusplus
typedef __WCHAR_TYPE__ wchar_t;
#endif
typedef __WINT_TYPE__ wint_t;

typedef float float_t;
typedef double double_t;

typedef char int8_t;
typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef int int32_t;
typedef unsigned int uint32_t;
typedef long long int64_t;
typedef unsigned long long uint64_t;
typedef unsigned _Addr uintptr_t;
typedef _Addr intptr_t;
typedef int64_t intmax_t;

typedef unsigned long size_t;
typedef long ssize_t;
typedef _Reg off_t;
typedef _Reg mode_t;
typedef _Addr ptrdiff_t;
typedef struct __locale_struct* locale_t;
typedef struct FILE FILE;
typedef long long time_t;
typedef long long suseconds_t;

typedef struct { union { int __i[9]; volatile int __vi[9]; unsigned __s[9]; } __u; } pthread_attr_t;
typedef struct { union { int __i[6]; volatile int __vi[6]; volatile void *volatile __p[6]; } __u; } pthread_mutex_t;
typedef struct { union { int __i[6]; volatile int __vi[6]; volatile void *volatile __p[6]; } __u; } mtx_t;
typedef struct { union { int __i[12]; volatile int __vi[12]; void *__p[12]; } __u; } pthread_cond_t;
typedef struct { union { int __i[12]; volatile int __vi[12]; void *__p[12]; } __u; } cnd_t;
typedef struct { union { int __i[8]; volatile int __vi[8]; void *__p[8]; } __u; } pthread_rwlock_t;
typedef struct { union { int __i[5]; volatile int __vi[5]; void *__p[5]; } __u; } pthread_barrier_t;

#endif

