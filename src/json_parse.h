/*  +======| File Info |===============================================================+
    |                                                                                  |
    |     Subdirectory:  /src                                                          |
    |    Creation date:  7/19/2024 6:36:46 PM                                          |
    |    Last Modified:                                                                |
    |                                                                                  |
    +======================================| Copyright © Sayed Abid Hashimi |==========+  */

#include <string.h>

#if !defined(JSON_PARSE_H)

typedef struct {
    json_value *content;
    linked_list *parent;
} linked_list;

typedef struct {
    char *data;
    usize length;
} string_value;

typedef struct {
    char *str;
    usize current_idx;
} buffer;

#define TOKEN_TYPES \
    X(key)          \
    X(value_float)  \
    X(value_int)    \
    X(value_str)    \
    X(list_begin)   \
    X(list_end)     \
    X(dict_begin)   \
    X(dict_end)     \
    X(comma)        \
    X(eot)

typedef enum {
#define X(value) tt_ ## value,
    TOKEN_TYPES
#undef X
} token_type;

char *token_type_str[] = {
#define X(value) #value,
    TOKEN_TYPES
#undef X
};
#undef TOKEN_TYPES

typedef struct token token;
struct token {
    token_type type;
    void *body;

    token *next;
};

typedef enum {
    jvt_dict,
    jvt_list,

    jvt_str,
    jvt_float,
    jvt_int,
} json_value_type;

/* NOTE(abid): This is just a stub used to define the type of the value. Once the type is known
 * one can `+= sizeof(json_value)` to get the actual content. - 28.Sep.2024 */
typedef struct {
    json_value_type type;
} json_value;

/* NOTE(abid): At the moment, we are putting key and value close in memory, since most of our ops
 * will be looking up a key and then immediately getting the value. So this reduces cache misses.
 * - 28.Sep.2024 */
typedef struct dict_content dict_content;
struct {
    char *key;
    json_value *value;

    dict_content *next;
} dict_content;

typedef struct {
    usize content_size; // size of the content of `table` in bytes, used for jumping over this structure

    usize table_count = 1024*1024; // count of `table` structure, used for hash function modulus
    dict_content *table[1024*1024];
} json_dict;

typedef struct {
    usize count;
    usize content_size; // size of `array` in bytes, used for jumping over this structure

    json_value *array;
} json_list;

typedef struct {
    mem_arena *arena;

    token *token_list;
    token *current_token;
    usize global_bytes_size;
} parser_state;

#define JSON_PARSE_H
#endif
