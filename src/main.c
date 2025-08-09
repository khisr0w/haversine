/*  +======| File Info |===============================================================+
    |                                                                                  |
    |     Subdirectory:  /src                                                          |
    |    Creation date:  6/28/2024 1:23:31 AM                                          |
    |    Last Modified:  9/22/2024 11:03:22 PM                                         |
    |                                                                                  |
    +======================================| Copyright © Sayed Abid Hashimi |==========+  */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define BENCH_ON
#include "types.h"
#include "utils.c"
#include "bench.h"
#include "random.c"
#include "stat.c"
#include "haversine.c"
#include "json_parse.c"

typedef struct {
    f64 *f64_buffer;
    json_dict *json;
} haversine_files;
internal haversine_files
load_json_f64_files(char *filename) {
    bench_function_begin();
    /* NOTE(abid): `filename` should be without extension. */

    usize filename_len = strlen(filename);
    char *temp = malloc(strlen(filename) + 5 + 1);
    char *json_extension = ".json";
    char *f64_extension = ".f64";
    usize json_extension_len = strlen(json_extension);
    usize f64_extension_len = strlen(f64_extension);
    memcpy(temp, filename, filename_len);

    /* NOTE(abid): Load JSON. */
    u32 idx;
    for(idx = 0; idx < json_extension_len; ++idx)
        temp[idx + filename_len] = json_extension[idx];
    temp[idx + filename_len] = '\0';
    json_dict *json = jp_load(temp);

    /* NOTE(abid): Load .f64 file. */
    for(idx = 0; idx < f64_extension_len; ++idx)
        temp[idx + filename_len] = f64_extension[idx];
    temp[idx + filename_len] = '\0';
    f64 *f64_buffer = read_file(temp, sizeof(f64));

    free(temp);

    return (haversine_files) {
        .json = json,
        .f64_buffer = f64_buffer
    };

    bench_function_end();
}

internal void
test_json_f64_difference(char *filename) {
    /* NOTE(abid): Testing, using .f64, whether json parser parses values correctly. */
    haversine_files loaded_files = load_json_f64_files(filename);

    bench_block_no_return_begin(calculate_diff);
    json_list *pairs = jp_get_dict_value(loaded_files.json, "pairs", json_list);
    f64 difference_sum = 0;
    for(u64 idx = 0; idx < pairs->count; ++idx) {
        json_dict *elem = jp_get_list_elem(pairs, idx, json_dict);
        f64 x0 = *jp_get_dict_value(elem, "x0", f64);
        f64 x1 = *jp_get_dict_value(elem, "x1", f64);
        f64 y0 = *jp_get_dict_value(elem, "y0", f64);
        f64 y1 = *jp_get_dict_value(elem, "y1", f64);
        f64 stored_value = loaded_files.f64_buffer[idx];
        f64 calc_value = haversine(x0, y0, x1, y1, EARTH_RADIUS);
        f64 difference = fabs(stored_value - calc_value);
        if(difference != 0.0f) {
            int value = 3;
            value = 2;
        }
        difference_sum += difference;
        // printf("%llu. stored = %f, calculated = %f, difference = %f\n",
        //        idx+1, stored_value, calc_value, difference);
    }
    bench_block_no_return_end(calculate_diff);
    // printf("\nTotal difference: %f\n", difference_sum);
}

internal void
generate_and_check_difference(u64 num_pairs, u64 num_clusters, char *filename, u64 seed) {
    stat_f64 generation_stat = generate_haversine_json(num_pairs, num_clusters, filename);
    printf("Seed: %llu\nPair Count: %llu\nExpected Sum: %f\n\n",
            seed, num_pairs, generation_stat.Sum);

    test_json_f64_difference(filename);
}

#if 1
i32 main(i32 argc, char* argv[]) {

    if(argc != 5) {
        printf("Usage: [seed] [number of pairs] [number of clusters] [file name]\n");
        return -1;
    }

    u64 seed = atoll(argv[1]);
    // u64 number_pairs = atoll(argv[2]);
    // u64 num_clusters = atoll(argv[3]);
    char* filename = argv[4];

    rand_seed(seed);

    bench_begin();

    // generate_haversine_json(number_pairs, num_clusters, filename);
    test_json_f64_difference(filename);

    bench_end();

    return 0;
}
#else

internal void func(u32);
internal void
func2(u32 recurse) {
    bench_function_begin()

    for(i32 idx = 0; idx < 10000000; ++idx) { idx = idx; }

    if(recurse) {
        func(recurse-1);
    }

    bench_function_end();
}

internal void
func(u32 recurse) {
    bench_function_begin()

    for(i32 idx = 0; idx < 10000000; ++idx) { idx = idx; }

    if(recurse) {
        func2(recurse-1);
    }

    bench_function_end();
}

i32 main() {
    bench_begin();

    func(4);

    bench_end();

    return 0;
}
#endif
