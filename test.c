/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/* author: Matthias Meißner (geige.matze@gmail.com) */

#include "muh_nit.h"
#include "cstr.h"

#include "string.h"

MUH_NIT_CASE(test_cstr_from_char_ptr)
{
    cstr s = cstr("test");
    MUH_ASSERT("wrong length in cstr conversion", len(s) == 4);
    MUH_ASSERT("cstr conversion failed", strncmp(s.inner, "test", len(s)) == 0);
    return MUH_SUCCESS;
}

MUH_NIT_CASE(test_cstring_from_char_ptr)
{
    cstring s = cstring_from("test", malloc_wrapper);
    MUH_ASSERT("wrong length in cstring conversion", len(s) == 4);
    MUH_ASSERT("cstr conversion failed", strncmp(s.inner, "test", len(s)) == 0);
    cstring_free(s);
    return MUH_SUCCESS;
}

MUH_NIT_CASE(test_cstr_from_string)
{
    cstring s = cstring_from("test", malloc_wrapper);
    cstr res = cstr(s);
    MUH_ASSERT("wrong length in cstr reference", len(res) == len(s));
    MUH_ASSERT("cstr reference broken", s.inner == res.inner);
    cstring_free(s);
    return MUH_SUCCESS;
}

MUH_NIT_CASE(test_cstring_from_cstr)
{
    cstring s = cstring_from(cstr("test"), malloc_wrapper);
    MUH_ASSERT("wrong length in cstring conversion", len(s) == 4);
    MUH_ASSERT("cstring conversion failed", strncmp(s.inner, "test", len(s)) == 0);
    cstring_free(s);
    return MUH_SUCCESS;
}

MUH_NIT_CASE(test_cstring_append)
{
    cstring s = cstring_from(cstr("hello"), malloc_wrapper);
    cstring_append(&s, " world");
    MUH_ASSERT("wrong length in cstring append", len(cstr("hello world")) == len(s));
    MUH_ASSERT("cstring append failed", strncmp(s.inner, "hello world", len(s)) == 0);
    MUH_ASSERT("capacity invariant broken", s.capacity >= len(s));
    cstring_free(s);
    return MUH_SUCCESS;
}

MUH_NIT_CASE(test_cstr_match)
{
    cstr a = cstr("test"), b = cstr("test"), c = cstr("cccc"), d = cstr("d");
    MUH_ASSERT("equal strings do not match", cstr_match(a, b));
    MUH_ASSERT("unequal strings match", !cstr_match(a, c));
    MUH_ASSERT("unequal strings match", !cstr_match(a, d));
    return MUH_SUCCESS;
}

#define UNUSED(x) ((void)(x))

MUH_NIT_CASE(test_find_first)
{
    cstr a = cstr("tesettingsre");
    MUH_ASSERT("fake finding", len(cstr_find_first(a, cstr("test"))) == 0);
    MUH_ASSERT("find failed", cstr_match(cstr_find_first(a, cstr("setting")), cstr("setting")));
    return MUH_SUCCESS;
}

int main(int argc, const char **args)
{
    UNUSED(argc);
    UNUSED(args);

    muh_nit_case cases[] = MUH_CASES(
        &test_cstr_from_char_ptr,
        &test_cstring_from_char_ptr,
        &test_cstr_from_string,
        &test_cstring_from_cstr,
        &test_cstring_append,
        &test_cstr_match,
        &test_find_first);

    muh_test_result res = muh_nit_run(cases);
    return muh_nit_evaluate(res);
}
