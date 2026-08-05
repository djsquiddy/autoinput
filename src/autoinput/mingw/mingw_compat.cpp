/**
 * @file mingw_compat.cpp
 * @brief Compatibility fixes for MinGW static linking issues.
 */

#if defined(__MINGW32__) && defined(_WIN64)

#include <cstdarg>
#include <cstdio>

extern "C" {

/**
 * Workaround for "undefined reference to __intrinsic_setjmpex"
 * This symbol is sometimes missing when linking winpthread statically on MinGW-w64.
 * We call the CRT's _setjmpex which is what it's supposed to do.
 */
extern int _setjmpex(void*, void*);

void __intrinsic_setjmpex(void* env, void* frame) {
    _setjmpex(env, frame);
}

/**
 * Workaround for "undefined reference to __ms_vsnprintf"
 */
int __ms_vsnprintf(char* s, size_t n, const char* format, va_list arg) {
    return vsnprintf(s, n, format, arg);
}

} // extern "C"

#endif
