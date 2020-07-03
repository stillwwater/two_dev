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

#include <string>
#include <cstdio>

#include "debug.h"
#include "two.h"

#define TWO_PERFORMANCE_PROFILER_TIMER_HACK 1

namespace two {

void Profiler::append(const TimeStamp &ts) {
    if (entries.size() < MaxEntries) {
        entries.push_back(ts);
        return;
    }
    ASSERTS_PARANOIA(false, "Profiler: too many entries");
}

void Profiler::begin_session(const char *filename) {
    if (fp != nullptr) {
        fclose(fp);
    }
    fp = fopen(filename, "w");
    fprintf(fp, "[");
}

void Profiler::save() {
    ASSERTS(fp != nullptr, "No session started by the profiler");
    for (const auto &entry : entries) {
        if (total_entries++ > 0) fprintf(fp, ",");

#if TWO_PERFORMANCE_PROFILER_TIMER_HACK
        // Prevents overlapping time data in some timeline viewers since it
        // is possible for two timers to start in the exact same microsecond
        // time.
        const int offset = total_entries % 2;
#else
        const int offset = 0;
#endif
        fprintf(fp, fmt, entry.name, entry.start + offset, entry.elapsed());
    }
}

void Profiler::end_session() {
    fprintf(fp, "]\n");
    fclose(fp);
    fp = nullptr;
}

int64_t TimeStamp::elapsed() const {
    return end - start;
}

PerformanceTimer::~PerformanceTimer() {
    using namespace std::chrono;
    auto end_tm = high_resolution_clock::now();

    int64_t start = time_point_cast<microseconds>(start_tm)
        .time_since_epoch().count();

    int64_t end = time_point_cast<microseconds>(end_tm)
        .time_since_epoch().count();

    ASSERT(two::profiler != nullptr);
    two::profiler->append(TimeStamp{name, start, end});
}

std::string sprintfs(const char *fmt, ...) {
    char buf[512];
    va_list arg_ptr;
    va_start(arg_ptr, fmt);
    vsnprintf(buf, sizeof(buf) - 1, fmt, arg_ptr);
    va_end(arg_ptr);
    buf[sizeof(buf) - 1] = '\0';
    return std::string(buf);
}

} // two
