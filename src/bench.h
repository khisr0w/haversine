/*  +======| File Info |===============================================================+
    |                                                                                  |
    |     Subdirectory:  /src                                                          |
    |    Creation date:  11/16/2024 9:35:02 PM                                         |
    |    Last Modified:                                                                |
    |                                                                                  |
    +======================================| Copyright © Sayed Abid Hashimi |==========+  */

#if !defined(BENCH_H)

#if PLT_WIN
#include <intrin.h>
#include <windows.h>
#elif PLT_LINUX
#include <x86intrin.h>
#include <sys/time.h>
#endif

internal u64
platform_os_timer_freq_get() {
    u64 result;
#if PLT_WIN
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    result = freq.QuadPart;
#elif PLT_LINUX
    result = 1000000;
#endif
    return result;
}

internal u64
platform_os_timer_get() {
    u64 result;
#if PLT_WIN
    LARGE_INTEGER timer;
    QueryPerformanceCounter(&timer);
    result = timer.QuadPart;
#elif PLT_LINUX
    struct timeval timer;
    gettimejofday(&timer, 0);
    result = platform_timer_freq_get() * (u64)timer.tv_sec + (u64)timer.tv_usec;
#endif
    return result;
}

internal inline u64
platform_cpu_timer_get() {
    return __rdtsc();
}

internal u64
platform_cpu_timer_freq_estimate_get(u64 ms_to_wait) {
    if (ms_to_wait == 0) ms_to_wait = 100;

    u64 os_freq = platform_os_timer_freq_get();

    u64 os_start = platform_os_timer_get();
    u64 cpu_start = platform_cpu_timer_get();

    u64 os_end = 0;
    u64 os_elapsed = 0;
    u64 os_wait_time = os_freq * ms_to_wait/1000;
    while(os_elapsed < os_wait_time) {
        os_end = platform_os_timer_get();
        os_elapsed = os_end - os_start;
    }

    u64 cpu_end = platform_cpu_timer_get();
    u64 cpu_elapsed = cpu_end - cpu_start;

    u64 cpu_freq = 0;
    if(os_elapsed) cpu_freq = cpu_elapsed * os_freq / os_elapsed;

    return cpu_freq;
}

#define BENCH_H
#endif
