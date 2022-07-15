/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/* author: Matthias Meißner (geige.matze@gmail.com) */

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

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
bool cstr_match(cstr a, cstr b);
bool cstr_contains(cstr haystack, cstr needle);
cstr cstr_find_first(cstr haystack, cstr needle);
cstring c_string_from_cstr(cstr input, allocator alloc);
void cstring_append_impl(cstring *, cstr);
void cstring_free(cstring string);

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

bool cstr_match(cstr a, cstr b)
{
    if (a.length != b.length)
        return false;

    const char *end = a.inner + a.length;

    while (a.inner < end)
        if (*a.inner++ != *b.inner++)
            return false;

    return true;
}

bool cstr_contains(cstr haystack, cstr needle)
{
    if (len(needle) == 0)
        return true;

    return len(cstr_find_first(haystack, needle)) != 0;
}

cstr cstr_find_first(cstr haystack, cstr needle)
{
    if (len(needle) == 0)
        return (cstr){.length = 0, .inner = NULL};

    int left = 0, right = 0;
    int shift_table[needle.length];
    shift_table[0] = -1;

    while (++right < needle.length)
    {
        if (needle.inner[left] == needle.inner[right])
            shift_table[right] = shift_table[left];
        else
            shift_table[right] = left;

        while (left >= 0 && needle.inner[right] != needle.inner[left])
            left = shift_table[left];

        left++;
    }

    int needle_pos = 0;
    const char *haystack_end = haystack.inner + haystack.length;

    while (haystack.inner < haystack_end)
    {
        while (needle_pos >= 0 && needle.inner[needle_pos] != *haystack.inner)
            needle_pos = shift_table[needle_pos];

        needle_pos++;
        haystack.inner++;

        if (needle_pos == len(needle))
            return (cstr){
                .length = len(needle),
                .inner = haystack.inner - len(needle)};
    }

    return (cstr){.length = 0, .inner = NULL};
}