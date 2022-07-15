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

cstr cstr_id(cstr input) { return input; }

#define cstr(x) _Generic((x), char *                   \
                         : cstr_from_char_ptr, cstring \
                         : cstr_from_cstring, cstr     \
                         : cstr_id)(x)

#define cstring_from(x, alloc) \
    c_string_from_cstr(cstr(x), alloc)

#define len(x) ((x).length)

cstr cstr_from_char_ptr(const char *input)
{
    return (cstr){.length = strlen(input), .inner = input};
}

cstr cstr_from_cstring(cstring input)
{
    return (cstr){.length = input.length, .inner = input.inner};
}

cstring c_string_from_cstr(cstr input, allocator allocator)
{
    char *buffer = allocator.run(NULL, input.length);
    memcpy(buffer, input.inner, input.length);

    return (cstring){
        .allocator = allocator,
        .inner = buffer,
        .capacity = input.length,
        .length = input.length};
}

void cstring_free(cstring string)
{
    string.allocator.run(string.inner, 0);
}

#define cstring_append(x, y) cstring_append_impl(x, cstr(y))

void cstring_append_impl(cstring *fst, cstr snd)
{
    if (fst->capacity < fst->length + snd.length)
    {
        fst->inner = fst->allocator.run(fst->inner, fst->length + snd.length);
        fst->capacity = fst->length + snd.length;
    }

    memcpy(&fst->inner[fst->length], snd.inner, snd.length);
    fst->length += snd.length;
}