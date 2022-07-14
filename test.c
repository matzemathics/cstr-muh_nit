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
    MUH_ASSERT("wrong length in cstr conversion", s.length == 4);
    MUH_ASSERT("cstr conversion failed", strncmp(s.inner, "test", s.length) == 0);
    return MUH_SUCCESS;
}

MUH_NIT_CASE(test_cstring_from_char_ptr)
{
    cstring s = cstring_from("test", malloc_wrapper);
    MUH_ASSERT("wrong length in cstring conversion", s.length == 4);
    MUH_ASSERT("cstr conversion failed", strncmp(s.inner, "test", s.length) == 0);
    cstring_free(s);
    return MUH_SUCCESS;
}

MUH_NIT_CASE(test_cstr_from_string)
{
    cstring s = cstring_from("test", malloc_wrapper);
    cstr res = cstr(s);
    MUH_ASSERT("wrong length in cstr reference", res.length == s.length);
    MUH_ASSERT("cstr reference broken", s.inner == res.inner);
    cstring_free(s);
    return MUH_SUCCESS;
}

#define UNUSED(x) ((void)(x))

int main(int argc, const char **args)
{
    UNUSED(argc);
    UNUSED(args);

    muh_nit_case cases[] = MUH_CASES(
        &test_cstr_from_char_ptr,
        &test_cstring_from_char_ptr,
        &test_cstr_from_string);

    muh_test_result res = muh_nit_run(cases);
    return muh_nit_evaluate(res);
}
