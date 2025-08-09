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
platform_get_os_timer_freq() {
#if PLT_WIN
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    return freq.QuadPart;
#elif PLT_LINUX
    return 1000000;
#endif
}

internal u64
platform_get_os_timer() {
#if PLT_WIN
    LARGE_INTEGER timer;
    QueryPerformanceCounter(&timer);
    return timer.QuadPart;
#elif PLT_LINUX
    struct timeval timer;
    gettimejofday(&timer, 0);
    return platform_get_os_timer_freq() * (u64)timer.tv_sec + (u64)timer.tv_usec;
#endif
}

internal inline u64
platform_get_cpu_timer() {
    return __rdtsc();
}

typedef struct { u64 ms_to_wait; } __platform_get_cpu_timer_freq_estimate_impl_opt_args;
#define __platform_get_cpu_timer_freq_estimate_impl_opt_args_default .ms_to_wait = 100
#define platform_get_cpu_timer_freq_estimate(...) \
    __platform_get_cpu_timer_freq_estimate_impl( \
        (__platform_get_cpu_timer_freq_estimate_impl_opt_args){ \
            __platform_get_cpu_timer_freq_estimate_impl_opt_args_default, __VA_ARGS__ \
        } \
    )
internal u64 __platform_get_cpu_timer_freq_estimate_impl(
    __platform_get_cpu_timer_freq_estimate_impl_opt_args opt
) {
    u64 os_freq = platform_get_os_timer_freq();

    u64 os_start = platform_get_os_timer();
    u64 cpu_start = platform_get_cpu_timer();

    u64 os_end = 0;
    u64 os_elapsed = 0;
    u64 os_wait_time = os_freq * opt.ms_to_wait/1000;
    while(os_elapsed < os_wait_time) {
        os_end = platform_get_os_timer();
        os_elapsed = os_end - os_start;
    }

    u64 cpu_end = platform_get_cpu_timer();
    u64 cpu_elapsed = cpu_end - cpu_start;

    u64 cpu_freq = 0;
    if(os_elapsed) cpu_freq = cpu_elapsed * os_freq / os_elapsed;

    return cpu_freq;
}

typedef struct {
    char *name;
    u64 tsc_elapsed_inclusive; /* NOTE(abid): Includes the timing of the children. */
    u64 tsc_elapsed_exclusive;
    u64 tsc_elapsed_at_root;
    u64 hit_count;

    u64 parent_idx;
} bench_anchor;

#define ANCHOR_ARRAY_SIZE 4096
typedef struct {
    u64 start_tsc;
    u64 end_tsc;
    bench_anchor anchors[ANCHOR_ARRAY_SIZE];
} profiler_state;
global_var profiler_state __GLOBAL_profiler = {0};
global_var u64 __GLOBAL_profiler_parent_idx = 0;

typedef struct {
    u64 start_tsc;
    u64 idx;
    u64 parent_idx;
    u64 old_tsc_elapsed_inclusive;
} bench_block;

internal bench_block
__bench_block_begin(u64 index) {
    assert(index + 1 < ANCHOR_ARRAY_SIZE, "out of bench anchors.");

    bench_block block = {0};
    block.idx = index;
    block.old_tsc_elapsed_inclusive = __GLOBAL_profiler.anchors[index].tsc_elapsed_inclusive;
    block.parent_idx = __GLOBAL_profiler_parent_idx;
    __GLOBAL_profiler_parent_idx = index;

    block.start_tsc = platform_get_cpu_timer();

    return block;
}

internal void
__bench_block_end(bench_block block, char *name) {
    u64 tsc_elapsed = platform_get_cpu_timer() - block.start_tsc;

    bench_anchor *anchor = __GLOBAL_profiler.anchors + block.idx;
    anchor->name = name;
    anchor->tsc_elapsed_inclusive = block.old_tsc_elapsed_inclusive + tsc_elapsed;
    anchor->tsc_elapsed_exclusive += tsc_elapsed;
    ++anchor->hit_count;

    bench_anchor *parent = __GLOBAL_profiler.anchors + block.parent_idx;
    parent->tsc_elapsed_exclusive -= tsc_elapsed;
    __GLOBAL_profiler_parent_idx = block.parent_idx;
}

#ifdef BENCH_ON

/* WARNING(abid): The block of code under this cannot have a return call (early exit). */
#define bench_block_no_return_begin(name) bench_block __ ## name = __bench_block_begin(__COUNTER__ + 1);
#define bench_block_no_return_end(name) __bench_block_end(__ ## name, "block-" #name);

/* TODO(abid): This only works for msvc. Use the better `__attribute__` when in gcc/clang. */
/* NOTE(abid): Although this catches early returns. In msvc, this will make all vars local. */
#define bench_block_begin(name) bench_block_no_return_begin(name) __try {
#define bench_block_end(name) } __finally { bench_block_no_return_end(name) }

#define bench_function_begin() bench_block __bench_block = __bench_block_begin(__COUNTER__ + 1); __try {
#define bench_function_end() } __finally { __bench_block_end(__bench_block, __func__); }

#else

#define bench_block_no_return_begin(name)
#define bench_block_no_return_end(name)
#define bench_block_begin(name)
#define bench_block_end(name)

#define bench_function_begin()
#define bench_function_end()

#endif

#define bench_begin() __bench_begin()
internal void
__bench_begin() { __GLOBAL_profiler.start_tsc = platform_get_cpu_timer(); }
#define bench_end(...) \
    assert_static(__COUNTER__ < ANCHOR_ARRAY_SIZE); \
    __bench_end((__bench_end_opt){__bench_end_opt_default, __VA_ARGS__})

#define __bench_end_opt_default .print = true, .reset_profiler = true
typedef struct { bool print; bool reset_profiler; } __bench_end_opt;
internal void
__bench_end(__bench_end_opt opt) {
    __GLOBAL_profiler.end_tsc = platform_get_cpu_timer();
    u64 cpu_freq = platform_get_cpu_timer_freq_estimate();
    u64 total_tsc_elapsed = __GLOBAL_profiler.end_tsc - __GLOBAL_profiler.start_tsc;

    if(opt.print) {
        printf(
            "\nTotal time: %fms (CPU Freq: %.2fGhz)\n",
            1000*(f64)total_tsc_elapsed / (f64)cpu_freq, (f64)cpu_freq / 1e9
        );

        for(u64 idx = 1; idx < array_size(__GLOBAL_profiler.anchors); ++idx) {
            bench_anchor *anchor = __GLOBAL_profiler.anchors + idx;
            if(anchor->tsc_elapsed_exclusive) {
                printf(
                    "  %s[%llu]: %llu (%.2f%%", anchor->name, anchor->hit_count, anchor->tsc_elapsed_exclusive,
                    100.0*(f64)anchor->tsc_elapsed_exclusive/(f64)total_tsc_elapsed
                );

                if(anchor->tsc_elapsed_inclusive != anchor->tsc_elapsed_exclusive) {
                    printf(
                        ", %.2f%% w/children", 100.0*(f64)(anchor->tsc_elapsed_inclusive) / (f64)total_tsc_elapsed
                    );
                }
                printf(")\n");
            }
        }
        printf("\n");
    }

    if(opt.reset_profiler) { __GLOBAL_profiler = (profiler_state){0}; }
}

#define BENCH_H
#endif
