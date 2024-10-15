/*  +======| File Info |===============================================================+
    |                                                                                  |
    |     Subdirectory:  /src                                                          |
    |    Creation date:  7/19/2024 6:36:46 PM                                          |
    |    Last Modified:                                                                |
    |                                                                                  |
    +======================================| Copyright © Sayed Abid Hashimi |==========+  */

#include <string.h>

#if !defined(JSON_PARSE_H)

/* NOTE(abid): Parser routines. */ #define parse_err(str, ...) fprintf(stderr, "parse error: " str "\n", __VA_ARGS__)
#define parse_assert(expr, str, ...) \
    if((expr)) { } \
    else { \
        parse_err(str, ##__VA_ARGS__); \
        exit(EXIT_FAILURE); \
    }

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
 * one can `+= sizeof(json_value)` to get the actual json value. - 28.Sep.2024 */
typedef struct {
    json_value_type type;
} json_value;

typedef struct {
    char *key;
    json_value *value;
} dict_kv;

typedef struct {
    usize count; // count of `table` structure, used for hash function modulus
    dict_kv *table;
} json_dict;

typedef struct {
    usize count;
    json_value **array;
} json_list;

typedef struct {
    json_dict *json;

    mem_arena *temp_arena;
    token *token_list;
    token *current_token;
    usize global_bytes_size;
} parser_state;


/* NOTE(abid): Structure is used exclusively during the parsing process and is not part of final JSON.
 * - 14.Oct.2024 */
typedef struct json_scope json_scope;
struct json_scope {
    json_value *content;
    union {
        usize idx; // Index to be used in the context, set by routine to track dict and list free boundary.
        usize count; // Used with lexer, to keep count of a scope (dict/list)
    };
    json_scope *parent;
};
#define JSON_PARSE_H
#endif
