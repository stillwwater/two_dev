// Copyright (c) 2020 stillwwater
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.

#ifndef DEBUG_H
#define DEBUG_H

#include <string>
#include <vector>
#include <cstdlib>
#include <chrono>

#include "SDL_log.h"

#include "config.h"

#define TWO_ASSERT_MSG_(exp_, msg_)                                         \
    do {                                                                    \
        if (!(exp_)) {                                                      \
            two::log_error("\n\n%s: assertion failed at %s:%d\n\t'%s'\n\n", \
                __func__, __FILE__, __LINE__, msg_);                        \
            std::abort();                                                   \
        }                                                                   \
    } while (false)

// Always assert whether assertions are enabled or not
#define ASSERT_ALWAYS(exp) TWO_ASSERT_MSG_(exp, #exp)

// Always assert whether assertions are enabled or not with a message
#define ASSERTS_ALWAYS(exp, ...) \
    TWO_ASSERT_MSG_(exp, two::sprintfs(__VA_ARGS__).c_str())

#ifdef TWO_ASSERTIONS
#define ASSERT(exp) TWO_ASSERT_MSG_(exp, #exp)
// Assert with a message
#define ASSERTS(exp, ...) \
    TWO_ASSERT_MSG_(exp, two::sprintfs(__VA_ARGS__).c_str())
// Prints an error message to the console and aborts execution.
// Enabled if assertions are enabled.
#define PANIC(...) ASSERTS(false, __VA_ARGS__)
#else
#define ASSERT(exp) (void)sizeof(exp)
#define ASSERTS(exp, ...) (void)sizeof(exp)
#define PANIC(...)

#endif // TWO_ASSERTIONS

#ifdef TWO_PARANOIA
// Extra checks that may be too slow even for debug builds
#define ASSERT_PARANOIA(exp) ASSERT(exp)
#define ASSERTS_PARANOIA(exp, ...) ASSERTS(exp, __VA_ARGS__)
#else
#define ASSERT_PARANOIA(exp) (void)sizeof(exp)
#define ASSERTS_PARANOIA(exp, ...) (void)sizeof(exp)
#endif // TWO_PARANOIA

#ifdef TWO_PERFORMANCE_PROFILING
#define TWO_PROFILE_EVENT(name) two::PerformanceTimer t__##__LINE__(name)

#define TWO_PROFILE_BEGIN(name) { TWO_PROFILE_EVENT(name)
#define TWO_PROFILE_END() }

#define TWO_PROFILE_FUNC() \
    two::PerformanceTimer t__##__LINE__(TWO_PRETTY_FUNCTION)

#else
#define TWO_PROFILE_EVENT(name)
#define TWO_PROFILE_BEGIN(name)
#define TWO_PROFILE_END()
#define TWO_PROFILE_FUNC()
#endif // TWO_PERFORMANCE_PROFILING

namespace two {

// Time is gin in microseconds.
struct TimeStamp {
    const char *name;
    int64_t start, end;

    // Returns elapsed time in microseconds.
    int64_t elapsed() const;
};

class Profiler {
public:
    // Max entries that can be in memory, about 32MB.
    // This limit exists because if the profiler is not cleared every frame
    // it can very quickly have a lot of entries.
    static constexpr int MaxEntries = 1048576;

    std::vector<TimeStamp> entries;

    // Append a new entry.
    void append(const TimeStamp &ts);

    // Should be called every frame.
    inline void clear() { entries.clear(); }

    // Create a file for writting the profile data. A session is only
    // required if you are writing to a json file with `save()`.
    void begin_session(const char *filename);

    // Save time stamps to a json file created using `begin_session`.
    //
    // > Note: This function does not use the filesystem in `filesystem.h`
    // since it is designed to be used for debugging only, the native file
    // system will be used.
    void save();

    // Writes ending for the json file and closes the file.
    void end_session();

private:
    FILE *fp = nullptr;

    int total_entries = 0;

    // Format for each timestamp.
    const char *fmt = "{"
                      "\"name\":\"%s\","
                      "\"cat\":\"PERF\","
                      "\"ph\":\"X\","
                      "\"pid\":0,"
                      "\"tid\":0,"
                      "\"ts\":%lld,"
                      "\"dur\":%lld"
                      "}";
};

class PerformanceTimer {
public:
    PerformanceTimer(const char *name)
        : name{name}
        , start_tm{std::chrono::high_resolution_clock::now()} {}

    ~PerformanceTimer();
private:
    const char *name;
    std::chrono::time_point<std::chrono::system_clock> start_tm;
};

// Use for debugging only. The string will be truncated if it does
// not fit in the buffer.
std::string sprintfs(const char *fmt, ...) TWO_FMT_PRINTF(1, 2);

// Platform indipendent logging. If logging is disabled then this
// function should be inlined and optimized away, however arguments
// passed will still be evaluated.
inline void log(const char *fmt, ...) TWO_FMT_PRINTF(1, 2);

// Variant of debug::log that logs warnings to the console.
// If logging is disabled then this function should be inlined and
// optimized away, however arguments passed will still be evaluated.
inline void log_warn(const char *fmt, ...) TWO_FMT_PRINTF(1, 2);

// Variant of debug::log that logs errors to the console.
// Errors are usually enabled for both debugging and release builds.
// If logging is disabled then this function should be inlined and
// optimized away, however arguments passed will still be evaluated.
inline void log_error(const char *fmt, ...) TWO_FMT_PRINTF(1, 2);

#if TWO_LOGLEVEL >= 3
inline void log(const char *fmt, ...) {
    va_list arg_ptr;
    va_start(arg_ptr, fmt);
    SDL_LogMessageV(0, SDL_LOG_PRIORITY_INFO, fmt, arg_ptr);
    va_end(arg_ptr);
}
#else
inline void log(const char *, ...) {}
#endif

#if TWO_LOGLEVEL >= 2
inline void log_warn(const char *fmt, ...) {
    va_list arg_ptr;
    va_start(arg_ptr, fmt);
    SDL_LogMessageV(0, SDL_LOG_PRIORITY_WARN, fmt, arg_ptr);
    va_end(arg_ptr);
}
#else
inline void log_warn(const char *, ...) {}
#endif

#if TWO_LOGLEVEL >= 1
inline void log_error(const char *fmt, ...) {
    va_list arg_ptr;
    va_start(arg_ptr, fmt);
    SDL_LogMessageV(0, SDL_LOG_PRIORITY_ERROR, fmt, arg_ptr);
    va_end(arg_ptr);
}
#else
inline void log_error(const char *, ...) {}
#endif

} // two

#endif // DEBUG_H
