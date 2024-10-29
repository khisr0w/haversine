/*  +======| File Info |===============================================================+
    |                                                                                  |
    |     Subdirectory:  /src                                                          |
    |    Creation date:  7/12/2024 5:11:40 PM                                          |
    |    Last Modified:  Di 15 Okt 2024 14:59:37 CEST                                  |
    |                                                                                  |
    +======================================| Copyright © Sayed Abid Hashimi |==========+  */

/* NOTE(abid):
 * Dictionary memory layout:
 * - table
 *   - void *
 *   - void *
 *   ...
 *
 * - dict_content
 * - dict_content
 *   ...
 *
   NOTE(abid): 
 * List memory layout:
 * - void *
 * - json_value
 * - void *
 * - json_value
 *   ...
 */

#include "json_parse.h"

/* NOTE(abid): Lexer routines. */
internal inline token *
buffer_push_token(token_type token_type, void *token_value, parser_state *state) {
    token *new_token = push_struct(token, state->temp_arena);
    new_token->type = token_type;
    new_token->body = token_value;

    if(state->token_list == NULL) state->token_list = new_token;
    else state->current_token->next = new_token;
    state->current_token = new_token;

    return new_token;
}
internal inline void
buffer_consume(buffer *json_buffer) { ++json_buffer->current_idx; }

inline internal char
buffer_char(buffer *json_buffer) { return json_buffer->str[json_buffer->current_idx]; }

internal inline void
buffer_consume_until(buffer *json_buffer, char char_to_stop) {
    /* NOTE(abid): Excluding the char_to_stop. */
    char current_char = buffer_char(json_buffer); 

    while(current_char != char_to_stop) {
        buffer_consume(json_buffer);
        current_char = buffer_char(json_buffer); 
    }
}

internal inline bool
buffer_is_ignore(buffer *json_buffer) {
    char current_char = json_buffer->str[json_buffer->current_idx];
    return (current_char ==  ' ') ||
           (current_char == '\n') ||
           (current_char == '\t') ||
           (current_char == '\r');
}

internal inline void
buffer_consume_ignores(buffer *json_buffer) {
    while(buffer_is_ignore(json_buffer)) buffer_consume(json_buffer);
}

internal void
buffer_to_cstring(string_value *str, buffer *json_buffer) {
    assert(buffer_char(json_buffer) == '"', "string must start with \"");
    buffer_consume(json_buffer); /* consume start quote */
    usize start_idx = json_buffer->current_idx;
    buffer_consume_until(json_buffer, '"');
    usize end_idx = json_buffer->current_idx;
    buffer_consume(json_buffer); /* consume end quote */

    str->data = json_buffer->str + start_idx;
    str->length = end_idx - start_idx;
}

internal inline bool
buffer_is_numeric(buffer *json_buffer) {
    char current_char = json_buffer->str[json_buffer->current_idx];
    return ((current_char >= '0') && (current_char <= '9')) ||
           (current_char == '-') || (current_char == '+');
}

internal bool 
buffer_consume_extract_numeric(string_value *str, buffer *json_buffer) {
    /* NOTE(abid): Assumes the current character is already a number. */
    bool is_float = false;

    usize start_idx = json_buffer->current_idx;
    while(buffer_is_numeric(json_buffer)) {
        buffer_consume(json_buffer);
        if(!is_float && json_buffer->str[json_buffer->current_idx] == '.') {
            is_float = true;
            buffer_consume(json_buffer);
        }
    }
    usize end_idx = json_buffer->current_idx;
    str->data = json_buffer->str + start_idx;
    str->length = end_idx - start_idx;

    return is_float;
}

inline internal void
scope_free_and_walk_up(json_scope **scope_p, parser_state *state) {
    /* NOTE(abid): Go up to the parent scope and add the current scope to the free list. */
    json_scope *scope = *scope_p;

    json_scope *temp = state->scope_free_list;
    state->scope_free_list = scope;
    *scope_p = scope->parent;
    state->scope_free_list->parent = temp;
}

inline internal json_scope *
scope_new(parser_state *state) {
    json_scope *scope;
    if(state->scope_free_list) {
        /* NOTE(abid): We have a reserved scope in the free list. */
        scope = state->scope_free_list;
        scope->content = NULL;
        scope->count = 0;
        state->scope_free_list = state->scope_free_list->parent;
    } else scope = push_struct(json_scope, state->temp_arena);

    return scope;
}

internal void
jp_lexer(buffer *json_buffer, parser_state *state) {
    /* TODO(abid): Guard against using comma at the end of a scope. */
    bool comman_encountered = false;
    json_scope *scope = NULL;

    while(json_buffer->str[json_buffer->current_idx]) {
        buffer_consume_ignores(json_buffer);
        switch(json_buffer->str[json_buffer->current_idx]) {
            case '"': {
                /* TODO(abid): No support for Unicode - 24.Sep.2024 */
                // string_value *str = push_struct(string_value, memory);
                string_value *str_value = push_struct(string_value, state->temp_arena);
                buffer_to_cstring(str_value, json_buffer);
                buffer_consume_ignores(json_buffer);
                if(json_buffer->str[json_buffer->current_idx] == ':') {
                    /* NOTE(abid): We have a key. */
                    buffer_push_token(tt_key, (void *)str_value, state);
                    buffer_consume(json_buffer);
                } else {
                    buffer_push_token(tt_value_str, (void *)str_value, state);
                    state->global_bytes_size += sizeof(json_value);
                    assert(scope != NULL, "scope cannot be NULL"); ++scope->count;
                }
                state->global_bytes_size += str_value->length+1;
            } break;
            case '{': { 
                state->global_bytes_size += sizeof(json_value) + sizeof(json_dict);

                /* NOTE(abid): Increment the parent count before giving scope to child, if
                 * we are not root itself. */
                if(scope != NULL) ++scope->count;
                json_scope *this_scope = scope_new(state);
                this_scope->parent = scope;
                scope = this_scope;

                buffer_push_token(tt_dict_begin, this_scope, state);
                buffer_consume(json_buffer); 
            } break;
            case '[': { 
                state->global_bytes_size += sizeof(json_value) + sizeof(json_list);

                assert(scope != NULL, "scope cannot be NULL"); ++scope->count;
                json_scope *this_scope = scope_new(state);
                this_scope->parent = scope;
                scope = this_scope;

                buffer_push_token(tt_list_begin, this_scope, state);
                buffer_consume(json_buffer);
            } break;
            case '}': { 
                buffer_push_token(tt_dict_end, 0, state); buffer_consume(json_buffer);
                state->global_bytes_size += scope->count*sizeof(dict_kv);

                scope_free_and_walk_up(&scope, state);
            } break;
            case ']': {
                buffer_push_token(tt_list_end, 0, state); buffer_consume(json_buffer);
                state->global_bytes_size += scope->count*sizeof(json_value *);

                scope_free_and_walk_up(&scope, state);
            } break;
            case '\0': { buffer_push_token(tt_eot, 0, state); buffer_consume(json_buffer); } break;
            case '\t':
            case '\n':
            case '\r':
            case ',': { comman_encountered = true; buffer_consume(json_buffer); } break;
            case ' ': { buffer_consume_ignores(json_buffer); } break;
            default: {
                if(buffer_is_numeric(json_buffer)) {
                    string_value *str = push_struct(string_value, state->temp_arena);
                    bool is_float = buffer_consume_extract_numeric(str, json_buffer);
                    if(is_float) {
                        /* NOTE(abid): Float is 64-bit. */
                        buffer_push_token(tt_value_float, str, state);

                        state->global_bytes_size += sizeof(f64);
                    } else {
                        /* NOTE(abid): Integer is 64-bit */
                        buffer_push_token(tt_value_int, str, state);
                        state->global_bytes_size += sizeof(i64);
                    }
                    assert(scope != NULL, "scope cannot be NULL"); ++scope->count;
                    state->global_bytes_size += sizeof(json_value);
                } else assert(0, "invalid path");
            }
        }
    }

    state->current_token = state->token_list;
}

/* NOTE(abid): String routines. */
internal usize
cstring_length(char *c_string) {
    usize result = 0;
    while(*c_string++) ++result;

    return result;
}

internal string_value
string_make(char *c_string) {
    return (string_value) {
        .data = c_string,
        .length = cstring_length(c_string)
    };
}

internal usize
jp_hash_from_string(char *string) {
    /* NOTE(abid): Adapted from `https://stackoverflow.com/questions/7616461/generate-a-hash-from-string-in-javascript` */ 
    usize string_len = cstring_length(string);
    usize hash = 0;
    if(string_len == 0) return hash;

    for(usize idx = 0; idx < string_len; ++idx) {
        char chr = string[idx];
        hash = ((hash << 5) - hash) + chr;
    }
    return hash;
}

/* NOTE(abid): JSON list routines. */
internal void
jp_list_add(json_scope *list_scope, json_value *j_value) {
    json_list *parent_list = (json_list *)(list_scope->content+1);
    parse_assert(parent_list->count > list_scope->idx, "list index out of bounds");

    json_value **v_element = parent_list->array + list_scope->idx;
    parse_assert(*v_element == NULL, "value entry in list already filled.");
    *v_element = j_value;

    list_scope->idx++;
}

#if 0
internal json_list *
jp_list_create(usize array_count, parser_state *state) {
    /* NOTE(abid): `array_count` should be the final size of the current array (non-growable). */
    json_list *result = push_size(sizeof(json_list) + sizeof(json_value)*array_count, state->arena);
    result->array = result + 1;
    result->max_count = array_count;
    result->count = 0;

    return result;
}

internal json_value *
jp_list_get(json_list *list, usize index) {
    assert(list->count > index, "index out of bound");
    return list->array + index;
}
#endif

/* NOTE(abid): Dictionary routines. */
#if 0
inline internal json_dict *
jp_dict_create(usize table_init_count, parser_state *current_memory) {
    json_dict *result = push_size(sizeof(json_dict) + sizeof(dict_content)*table_init_count,
                                  state->arena);
    result->table = result + 1;
    result->count = table_init_count;

    return result;
}

internal void
jp_dict_add(json_dict *dict, string_value key, void *content, json_value_type type, parser_state mem) {
    /* NOTE(abid): We expect `key` and `content` be valid until end of program.
     *             i.e. we won't do the allocation here. */
    u32 hash = hash_from_string(key);
    dict_content *slot = dict->table + (hash % dict->count);

    /* NOTE(abid): Check for collision hits. */
    dict_content *next_slot = slot;
    for(;next_slot;) {
        assert(string_is_equal(ckey, next_slot->key), "adding key duplicated.");
        slot = next_slot;
        next_slot = slot->next;
    }
    slot->next = push_struct(dict_content, mem);
    slot = slot->next;

    slot->key = ckey;
    slot->value = value; 
}

internal json_value *
jp_dict_get_value(json_dict *dict, char *key) {
    u32 hash = hash_from_string(key);
    dict_content *slot = dict->table + (hash % dict->table_count);

    /* NOTE(abid): Check if object exist, as well as collision. */
    for(;slot;) {
        if(strcmp(slot->key, key) == 0) return slot->value;
        slot = slot->new;
    }

    /* NOTE(abid): If we reach here, then we've gone and done it now. */
    assert(0, "key %s not found.", key);
}

internal u32
jp_hash_from_string(string str) {
    u32 result = 0;
    for(i32 idx = 0; idx < str.length; ++idx) {
        result = ((result << 5) - result) + (u32)str.data[idx];
    }
    return result;
}

inline internal bool
string_is_equal(string str1, string str2) {
    if(str1.length != str2.length) return false;

    for(u32 idx = 0; idx < str1.length; ++idx) {
        if(str1.data[idx] != str2.data[idx]) return false;
    }

    return true;
}

internal dict_content **
jp_dict_get_empty(json_dict *dict, char *string) {
    u32 hash = hash_from_string(string);
    dict_content *slot = dict->table + (hash % dict->table_count);

    /* NOTE(abid): Check for collision */
    if(slot == NULL) return slot;
    do { slot = slot->next; } while(slot);

    return &slot;
}
#endif

inline internal void
token_expect(token *tok, token_type tok_type) {
    parse_assert(tok->type == tok_type, "expected token [%s], got [%s]\n",
                  token_type_str[tok_type], token_type_str[tok->type]);
}

inline internal void
token_advance(token **tok) { *tok = (*tok)->next; }

internal token *
token_peek(token *tok, usize forward_count) {
    token *result = tok;
    for(;forward_count != 0; --forward_count) {
        result = result->next;
    }

    return result;
}

internal void
jp_dict_add_value(json_scope *dict_scope, json_value *j_value) {
    json_dict *parent_dict = (json_dict *)(dict_scope->content+1);
    dict_kv *kv_element = parent_dict->table + dict_scope->idx;
    parse_assert(kv_element && kv_element->key != NULL, "value must have associated key inside dict.");
    parse_assert(kv_element->value == NULL, "value entry in dict already filled.");
    kv_element->value = j_value;
}

inline internal void
jp_add_to_scope(json_scope *scope, json_value *j_value) {
    /* NOTE(abid): Add the json_value to the current scope (dict/list). */
    json_value *parent_value = (json_value *)(scope->content+1);
    if(parent_value->type == jvt_dict) jp_dict_add_value(scope, j_value);
    else if(parent_value->type == jvt_list) jp_list_add(scope, j_value);
}

internal char *
jp_push_token_str_to_cstr(token *current_token, mem_arena *json_arena) {
    string_value *str = (string_value *)current_token->body;
    char *c_str = push_size(str->length+1, json_arena);
    for(u32 idx = 0; idx < str->length; ++idx) c_str[idx] = str->data[idx];
    c_str[str->length] = '\0';

    return c_str;
}

internal void 
jp_parser(parser_state *state) {
    token *current_token = state->token_list;
    /* NOTE(abid): The initial tokens must dict_begin followed by a key. */
    token_expect(current_token, tt_dict_begin);
    token_expect(token_peek(current_token, 1), tt_key);

    /* NOTE(abid): Push the entire required memory for json object at once. */
    mem_arena *json_arena = arena_create(state->global_bytes_size, state->global_bytes_size);
    state->json = json_arena->ptr;

    /* NOTE(abid): Current scope of the container we are in [json_list | json_dict]. */
    json_scope *scope = NULL;
    while(current_token->type != tt_eot) {
        switch(current_token->type) {
            case tt_dict_begin: {
                json_value *j_value = push_size(sizeof(json_value) + sizeof(json_dict), json_arena);
                j_value->type = jvt_dict;
                json_dict *dict = (json_dict *)(j_value+1);

                json_scope *l_scope = (json_scope *)current_token->body;
                dict->count = l_scope->count;
                dict->table = push_size(dict->count*sizeof(dict_kv), json_arena);

                /* NOTE(abid): If are not at the root dictionary, then must add to parent. */
                if(scope != NULL) jp_add_to_scope(scope, j_value);
                json_scope *this_scope = scope_new(state);
                this_scope->content = j_value;
                this_scope->parent = scope;
                this_scope->idx = 0;
                scope = this_scope;
            } break;
            case tt_list_begin: {
                parse_assert(scope != NULL, "a list cannot be the first scope in JSON");

                json_value *j_value = push_size(sizeof(json_value) + sizeof(json_list), json_arena);
                j_value->type = jvt_list;
                json_list *list = (json_list *)(j_value+1);

                json_scope *l_scope = (json_scope *)current_token->body;
                list->count = l_scope->count;
                list->array = push_size(list->count*sizeof(json_value *), json_arena);

                jp_add_to_scope(scope, j_value);
                json_scope *this_scope = scope_new(state);
                this_scope->content = j_value;
                this_scope->parent = scope;
                this_scope->idx = 0;
                scope = this_scope;
            } break;
            case tt_list_end: 
            case tt_dict_end: {
                parse_assert(scope != NULL, "unexpected closing of scope, did you enter an extra }/]?");
                scope_free_and_walk_up(&scope, state);
            } break;
            case tt_key: {
                parse_assert(scope && scope->content->type == jvt_dict,
                             "key cannot exist outside dictionary scope");
                json_dict *parent_dict = (json_dict *)(scope->content+1);
                // dict_kv *kv_element = parent_dict->table + scope->idx;

                char *key = jp_push_token_str_to_cstr(current_token, json_arena);
                usize hash = jp_hash_from_string(key);
                usize original_idx = (hash % parent_dict->count);

                dict_kv *kv_element = NULL;
                /* NOTE(abid): Check for collision hits. */
                usize potential_idx = original_idx;
                while(true) {
                    kv_element = parent_dict->table + potential_idx;
                    if(kv_element->key == NULL) break;

                    potential_idx = (potential_idx+1) % parent_dict->count;
                    parse_assert(original_idx != potential_idx, "count not find empty entry in dict.");
                }
                kv_element->key = key;
                scope->idx = potential_idx;
            } break;
            case tt_value_float: {
                parse_assert(scope, "float value cannot exist outside a scope");

                json_value *j_value = push_size(sizeof(json_value) + sizeof(f64), json_arena);
                j_value->type = jvt_float;
                f64 *value = (f64 *)(j_value+1);

                char *end;
                char *float_str = jp_push_token_str_to_cstr(current_token, json_arena);
                *value = strtod(float_str, &end);

                jp_add_to_scope(scope, j_value);
            } break;
            case tt_value_int: {
                parse_assert(scope, "integer value cannot exist outside a scope");

                json_value *j_value = push_size(sizeof(json_value) + sizeof(i64), json_arena);
                j_value->type = jvt_int;
                i64 *value = (i64 *)(j_value+1);

                char *int_str = jp_push_token_str_to_cstr(current_token, json_arena);
                *value = atol(int_str);

                jp_add_to_scope(scope, j_value);
            } break;
            case tt_value_str: {
                parse_assert(scope, "string value cannot exist outside a scope");

                json_value *j_value = push_size(sizeof(json_value) + sizeof(char *), json_arena);
                j_value->type = jvt_str;
                char *value = (char *)(j_value+1);
                value = jp_push_token_str_to_cstr(current_token, json_arena);
            } break;

            default: assert(0, "invalid code path in parser");
        }
        token_advance(&current_token);
    }
}

internal buffer 
read_text_file(char *Filename) {
    buffer Result = {0};
    FILE *FileHandle = fopen(Filename, "rb");

    /* NOTE(abid): Get the size of the File */
    fseek(FileHandle, 0, SEEK_END);
    usize FileSize = ftell(FileHandle);
    rewind(FileHandle);

    /* NOTE(abid): Read into the start of the main memory. */
    char* Content = platform_allocate(FileSize);
    if (Content == NULL) {
        fclose(FileHandle);
        Assert(false, "could not allocate memory");
    }

    size_t BytesRead = fread(Content, 1, FileSize, FileHandle);
    if (BytesRead != FileSize) {
        fclose(FileHandle);
        free(Content);
        Assert(false, "could not load file into memory");
    }
    fclose(FileHandle);

    Result.str = Content;
    // Result.str.length = BytesRead - 1; // -1 for the \0 at the end of the buffer
    Result.current_idx = 0;

    return Result;
}

internal json_value *
jp_load(char *Filename) {
    buffer Buffer = read_text_file(Filename);
    usize physical_mem_max_size = platform_ram_size_get();
    parser_state state = {
        .json = NULL,
        .temp_arena = arena_create(megabyte(10), (u64)(physical_mem_max_size/2)),
        .token_list = NULL,
        .current_token = NULL
    };

    jp_lexer(&Buffer, &state);
    jp_parser(&state);

    arena_free(state.temp_arena);

    return state.json;
}
