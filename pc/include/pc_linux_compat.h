// Force-included into every game TU of the Linux build (pc/CMakeLists.txt).
// Pulls in glibc headers up front so their declarations stop clashing with the
// decomp's, which then compile unchanged (mingw's headers don't clash).
#ifndef PC_LINUX_COMPAT_H
#define PC_LINUX_COMPAT_H

// Dl_info/dladdr (pc_main.c); g++ defines this anyway
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

// osContPad.h #undefs errno for a struct member, then re-includes <errno.h> to
// "restore" it. Already included here, so that's a no-op and errno stays
// undefined after it, like on mingw.
#include <errno.h>

// MSL_C/ctype.h guards itself with _CTYPE_H, glibc's own guard, so its
// #include <ctype.h> would include nothing
#include <ctype.h>

// libultra.h declares these with non-const params, glibc with const
#define bcmp pc_glibc_bcmp
#define bcopy pc_glibc_bcopy
#include <strings.h>
#undef bcmp
#undef bcopy

// glibc's C23 narrowing fsqrt(double) vs math64.h's fsqrt(f32)
#define fsqrt pc_glibc_fsqrt
#include <math.h>
#undef fsqrt

// Before _mem.h, whose memcpy/memset/memcmp lack glibc's noexcept in C++
#include <string.h>

// SDL_stdinc.h includes <float.h> here (not on mingw); sys_math.h then redefines
// FLT_MAX (same value). Include it now and let sys_math.h's definition stand.
#include <float.h>
#undef FLT_MAX

#endif
