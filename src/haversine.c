/*  +======| File Info |===============================================================+
    |                                                                                  |
    |     Subdirectory:  /src                                                          |
    |    Creation date:  Do 14 Nov 2024 03:10:29 CET                                   |
    |    Last Modified:                                                                |
    |                                                                                  |
    +======================================| Copyright © Sayed Abid Hashimi |==========+  */

/* NOTE(abid): EarthRadius is generally expected to be 6372.8 */
#define EARTH_RAIDUS 6372.8
internal f64
haversine(f64 x0, f64 y0, f64 x1, f64 y1, f64 earth_radius) {
    f64 lat1 = y0;
    f64 lat2 = y1;
    f64 lon1 = x0;
    f64 lon2 = x1;
    
    f64 dlat = radians_from_degrees(lat2 - lat1);
    f64 dlon = radians_from_degrees(lon2 - lon1);
    lat1 = radians_from_degrees(lat1);
    lat2 = radians_from_degrees(lat2);
    
    f64 a = square(sin(dlat/2.0)) + cos(lat1)*cos(lat2)*square(sinf(dlon/2));
    f64 c = 2.0*asin(sqrt(a));
    
    f64 result = earth_radius * c;
    
    return result;
}

internal void
offload_to_buffer(mem_arena *json_arena, mem_arena *result_arena, f64 y0, f64 y1, f64 x0, f64 x1,
                  bool is_last, char *json_filename, char *f64_filename) {
    u64 suffix_len = 3; /* NOTE(abid): Length of "\n]}" */

    /* NOTE(abid): We will be overestimating our memory usage since we assume each number's
     *             length to be 3 + FloatPrecision, which will not always be true. */
    u32 precision = 20;
    i32 num_chars_per_pair = 1 + 2 + 4*4 + 4 + 3 + 1 + 1 + 4*(4 + 1 + precision);
    snprintf(arena_current(json_arena), json_arena->size - json_arena->used,
              "\t{\"x0\":%.*f, \"y0\":%.*f, \"x1\":%.*f, \"y1\":%.*f}",
              precision, x0, precision, y0, precision, x1, precision, y1);
    arena_advance(json_arena, strlen(arena_current(json_arena)), char);

    if(is_last) {
        snprintf(arena_current(json_arena), suffix_len+1, /* suffix = */ "\n]}");
        arena_advance(json_arena, suffix_len, char);
    } else {
        snprintf(arena_current(json_arena), 3, ",\n");
        arena_advance(json_arena, 2, char);
    }

    /* NOTE(abid): Save the result to buffer and .f64 file. */
    f64 *dest = arena_current(result_arena);
    *dest = haversine(x0, y0, x1, y1, EARTH_RAIDUS);
    arena_advance(result_arena, 1, f64);

    /* NOTE(abid): If end of buffer, or we are out of memory for next round, then flush. */
    if(result_arena->used >= result_arena->size || is_last) {
        FILE *file_handle = fopen(f64_filename, "ab");
        usize file_size = fseek(file_handle, 0, SEEK_END);
        assert(file_handle, "cannot save to .f64 file.");
        fwrite(result_arena->ptr, sizeof(f64), result_arena->used/sizeof(f64), file_handle);
        fclose(file_handle);
        result_arena->used = 0;
    }

    /* NOTE(abid): If end of buffer, or we are out of memory for next round, then flush. */
    if((json_arena->used + num_chars_per_pair + suffix_len + 1 >= json_arena->size) || is_last) {
        FILE *file_handle = fopen(json_filename, "ab");
        assert(file_handle, "cannot save to .json file.");
        fputs(json_arena->ptr, file_handle);
        fclose(file_handle);
        json_arena->used = 0;
    }
}

internal stat_f64
generate_haversine_json(u64 number_pairs, u64 num_clusters, char *filename) {
    mem_arena *temp_arena = arena_create(kilobyte(1), gigabyte(10));
    mem_arena *json_arena = arena_create(megabyte(1), terabyte(10));
    mem_arena *result_arena = arena_create(megabyte(1), terabyte(10));

    stat_f64 haversine_stat = {0};

    u64 filename_len = strlen(filename); // Yes, I ain't using strlen
    char *json_extension = ".json";
    char *f64_extension = ".f64";
    u64 json_extension_len = strlen(json_extension);
    u64 f64_extension_len = strlen(f64_extension);
    char *json_filename = push_size(filename_len + json_extension_len + 1, temp_arena);
    char *f64_filename = push_size(filename_len + f64_extension_len + 1, temp_arena);

    for(u64 Idx = 0; Idx < filename_len; ++Idx) {
        json_filename[Idx] = filename[Idx];
        f64_filename[Idx] = filename[Idx];
    }
    for(u64 Idx = 0; Idx < json_extension_len; ++Idx) json_filename[filename_len+Idx] = json_extension[Idx];
    json_filename[json_extension_len + filename_len] = '\0';
    for(u64 idx = 0; idx < f64_extension_len; ++idx) f64_filename[filename_len+idx] = f64_extension[idx];
    f64_filename[f64_extension_len + filename_len] = '\0';


    /* NOTE(abid): Alignment to the 8 bytes of f64. */
    usize alignment = sizeof(f64);
    assert(result_arena->size >= 2*sizeof(f64)*alignment, "minimum size requirement for result buffer not met.");
    usize alignment_mask = alignment - 1;
    if((usize)result_arena->ptr & alignment_mask) {
        usize aligment_offset = alignment - ((usize)result_arena->ptr & alignment_mask);
        result_arena->ptr = (void *)((usize)result_arena->ptr + aligment_offset);
        result_arena->size -= sizeof(f64);
    }

    char *prefix = "{\"pairs\":[\n";
    u64 prefix_len = 11;
    for(; json_arena->used < prefix_len; arena_advance(json_arena, 1, char)) {
        char *dest = arena_current(json_arena);
        *dest = prefix[json_arena->used];
    }

    if(num_clusters) {
        // NumClusters = (NumClusters) ? NumClusters : RandRangeU64(20, 300);
        u64 num_pair_per_cluster = (u64)(number_pairs / num_clusters);
        u64 pair_remainder = number_pairs % num_clusters;
        if(pair_remainder) ++num_clusters;
        for(u64 cluster_idx = 0; cluster_idx < num_clusters; cluster_idx++) {
            f64 lat1_size = rand_range_f64(10., 100.);
            f64 lon1_size = rand_range_f64(20., 200.);
            f64 lat1_start = rand_range_f64(-90., 90. - lat1_size);
            f64 lon1_start = rand_range_f64(-180., 180. - lon1_size);

            f64 lat2_size = rand_range_f64(10., 100.);
            f64 lon2_size = rand_range_f64(20., 200.);
            f64 lat2_start = rand_range_f64(-90., 90. - lat2_size);
            f64 lon2_start = rand_range_f64(-180., 180. - lon2_size);

            /* NOTE(abid): In case we have remainders left. */
            u64 num_to_generate = ((cluster_idx == num_clusters-1) && pair_remainder) ? pair_remainder
                                                                                   : num_pair_per_cluster;
            for(u64 pair_idx = 0; pair_idx < num_to_generate; pair_idx++) {
                f64 lat1 = rand_range_f64(lat1_start, lat1_start+lat1_size);
                f64 lat2 = rand_range_f64(lat2_start, lat2_start+lat2_size);
                f64 lon1 = rand_range_f64(lon1_start, lon1_start+lon1_size);
                f64 lon2 = rand_range_f64(lon2_start, lon2_start+lon2_size);
                stat_f64_accumulate(lat1, &haversine_stat);
                stat_f64_accumulate(lat2, &haversine_stat);
                stat_f64_accumulate(lon1, &haversine_stat);
                stat_f64_accumulate(lon2, &haversine_stat);
                offload_to_buffer(json_arena, result_arena, lat1, lat2, lon1, lon2,
                                  cluster_idx*num_pair_per_cluster + pair_idx + 1 == number_pairs,
                                  json_filename, f64_filename);
            }
        }
    } else {
        for(u64 idx = 0; idx < number_pairs; idx++) {
            f64 lat1 = rand_range_f64(-90., 90.);
            f64 lat2 = rand_range_f64(-90., 90.);
            f64 lon1 = rand_range_f64(-180., 180.);
            f64 lon2 = rand_range_f64(-180., 180.);
            stat_f64_accumulate(lat1, &haversine_stat);
            stat_f64_accumulate(lat2, &haversine_stat);
            stat_f64_accumulate(lon1, &haversine_stat);
            stat_f64_accumulate(lon2, &haversine_stat);

            offload_to_buffer(json_arena, result_arena, lat1, lat2, lon1, lon2,
                              idx+1 == number_pairs, json_filename, f64_filename);
        }
    }

    return haversine_stat;
}
