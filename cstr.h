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
    size_t length;
    const char *inner;
} cstr;

typedef struct cstring
{
    size_t length;
    char *inner;
    size_t capacity;
    allocator alloc;
} cstring;

const allocator malloc_wrapper = {&realloc};

cstr cstr_id(cstr input) { return input; }

cstr cstr_from_char_ptr(const char *input);
cstr cstr_from_cstring(cstring input);
cstring c_string_from_cstr(cstr input, allocator alloc);

#ifndef __cplusplus

#define cstr(x) _Generic((x), char *                   \
                         : cstr_from_char_ptr, cstring \
                         : cstr_from_cstring, cstr     \
                         : cstr_id)(x)

#else

#define cstr(x) cstr_(x)

cstr cstr_(const char *input)
{
    return cstr_from_char_ptr(input);
}
cstr cstr_(cstring input) { return cstr_from_cstring(input); }
cstr cstr_(cstr input) { return input; }

#endif

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

cstring c_string_from_cstr(cstr input, allocator alloc)
{
    char *buffer = (char *)alloc.run(NULL, input.length);
    memcpy(buffer, input.inner, input.length);

    return (cstring){
        .length = input.length,
        .inner = buffer,
        .capacity = input.length,
        .alloc = alloc,
    };
}

void cstring_free(cstring string)
{
    string.alloc.run(string.inner, 0);
}

#define cstring_append(x, y) cstring_append_impl(x, cstr(y))

void cstring_append_impl(cstring *fst, cstr snd)
{
    if (fst->capacity < fst->length + snd.length)
    {
        fst->inner = (char *)fst->alloc.run(fst->inner, fst->length + snd.length);
        fst->capacity = fst->length + snd.length;
    }

    memcpy(&fst->inner[fst->length], snd.inner, snd.length);
    fst->length += snd.length;
}