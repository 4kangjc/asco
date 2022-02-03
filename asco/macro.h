#pragma once

#include "log.h"
#include "util.h"
#include <cassert>

// use cpp 20 [[likely]] or [[unlikely]]
// #if defined  __GNUC__ || defined __llvm__ 
// // 告诉编译器优化,条件大概率成立
// #   define ASCO_LIKELY(x)       __builtin_expect(!!(x), 1)
// // 告诉编译器优化,条件大概率不成立
// #   define ASCO_UNLIKELY(x)     __builtin_expect(!!(x), 0)
// #else
// #   define ASCO_LIKELY(x)          (x)
// #   define ASCO_UNLIKELY(x)        (x)
// #endif

#define ASCO_ASSERT(x) \
    if (!(x)) [[unlikely]] { \
        ASCO_LOG_ERROR(ASCO_LOG_ROOT()) << "ASSERTION: " << #x \
            << "\nbacktrace:\n" << asco::BacktraceToString(100, 2, "    ");  \
        assert(x); \
    }

#define ASCO_ASSERT2(x, w) \
    if (!(x)) [[unlikely]] { \
        ASCO_LOG_ERROR(ASCO_LOG_ROOT()) << "ASSERTION: " << #x << '\n' << w   \
            << "\nbacktrace:\n" << asco::BacktraceToString(100, 2, "    ");  \
        assert(x); \
    }
    