#ifndef TWO_CONFIG_H
#define TWO_CONFIG_H

// SSE instrinsics
#if defined(__SSE__)
#    define TWO_SSE
#elif defined(_MSC_VER)
#    if defined(_M_AMD64) || defined(_M_X64)
#        define TWO_SSE
#    elif _M_IX86_FP
#        define TWO_SSE
#    endif
#endif

// NEON intrinsics
#if defined(__ARM_NEON)
#    define TWO_NEON
#endif

#endif // TWO_CONFIG_H
