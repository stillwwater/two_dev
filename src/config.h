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

#if defined(__clang__) || defined(__GNUC__)
#define TWO_FMT_PRINTF(a, b) __attribute__((__format__(printf, a, b)))
#define LIKELY(x) __builtin_expect(x, 1)
#define UNLIKELY(x) __builtin_expect(x, 0)
#else
#define TWO_FMT_PRINTF(a, b)
#define LIKELY(x) (x)
#define UNLIKELY(x) (x)
#endif

#if defined(__clang__) || defined(__GNUC__)
#define TWO_PRETTY_FUNCTION __PRETTY_FUNCTION__
#elif defined(MSVC)
#define TWO_PRETTY_FUNCTION __FUNCSIG__
#else
#define TWO_PRETTY_FUNCTION __func__
#endif

#define UNUSED(exp_) (void)(exp_)
#define MAYBE_UNUSED(exp_) UNUSED(exp_);

#endif // TWO_CONFIG_H
