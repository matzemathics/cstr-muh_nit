/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/* author: Matthias Meißner (geige.matze@gmail.com) */

#include "string.h"
#include "stdlib.h"

typedef struct allocator
{
    void *(*run)(void *ptr_to_free, size_t size);
} allocator;

typedef struct cstr
{
    int length;
    const char *inner;
} cstr;

typedef struct cstring
{
    int length;
    char *inner;
    int capacity;
    allocator allocator;
} cstring;

const allocator malloc_wrapper = {&realloc};

#define cstr(x) _Generic((x), char *                   \
                         : cstr_from_char_ptr, cstring \
                         : cstr_from_cstring)(x)

#define cstring_from(x, alloc) \
    _Generic((x),              \
             char *            \
             : c_string_from_char_ptr(x, alloc))

#define len(x) ((x).length)

cstr cstr_from_char_ptr(const char *input)
{
    return (cstr){.length = strlen(input), .inner = input};
}

cstr cstr_from_cstring(cstring input)
{
    return (cstr){.length = input.length, .inner = input.inner};
}

cstring c_string_from_char_ptr(const char *input, allocator allocator)
{
    int length = strlen(input);
    char *buffer = allocator.run(NULL, length);
    memcpy(buffer, input, length);

    return (cstring){
        .length = length,
        .inner = buffer,
        .capacity = length,
        .allocator = allocator};
}

void cstring_free(cstring string)
{
    string.allocator.run(string.inner, 0);
}