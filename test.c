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
}

MUH_NIT_CASE(test_cstring_from_char_ptr)
{
    cstring s = cstring_from("test", malloc_wrapper);
    MUH_ASSERT("wrong length in cstring conversion", len(s) == 4);
    MUH_ASSERT("cstr conversion failed", strncmp(s.inner, "test", len(s)) == 0);
    cstring_free(s);
}

MUH_NIT_CASE(test_cstr_from_string)
{
    cstring s = cstring_from("test", malloc_wrapper);
    cstr res = cstr(s);
    MUH_ASSERT("wrong length in cstr reference", len(res) == len(s));
    MUH_ASSERT("cstr reference broken", s.inner == res.inner);
    cstring_free(s);
}

MUH_NIT_CASE(test_cstring_from_cstr)
{
    cstring s = cstring_from(cstr("test"), malloc_wrapper);
    MUH_ASSERT("wrong length in cstring conversion", len(s) == 4);
    MUH_ASSERT("cstring conversion failed", strncmp(s.inner, "test", len(s)) == 0);
    cstring_free(s);
}

MUH_NIT_CASE(test_cstring_append)
{
    cstring s = cstring_from(cstr("hello"), malloc_wrapper);
    cstring_append(&s, " world");
    MUH_ASSERT("wrong length in cstring append", len(cstr("hello world")) == len(s));
    MUH_ASSERT("cstring append failed", strncmp(s.inner, "hello world", len(s)) == 0);
    MUH_ASSERT("capacity invariant broken", s.capacity >= len(s));
    cstring_free(s);
}

MUH_NIT_CASE(test_cstr_match)
{
    cstr a = cstr("test"), b = cstr("test"), c = cstr("cccc"), d = cstr("d");
    MUH_ASSERT("equal strings do not match", cstr_match(a, b));
    MUH_ASSERT("unequal strings match", !cstr_match(a, c));
    MUH_ASSERT("unequal strings match", !cstr_match(a, d));
}

MUH_NIT_CASE(test_find_first)
{
    cstr a = cstr("tesettingsre");
    MUH_ASSERT("fake finding", len(cstr_find_first(a, cstr("test"))) == 0);
    MUH_ASSERT("find failed", cstr_match(cstr_find_first(a, cstr("setting")), cstr("setting")));
}

MUH_NIT_CASE(test_contains)
{
    cstr a = cstr("tesettingsere");
    MUH_ASSERT("contains found fake", !cstr_contains(a, cstr("test")));
    MUH_ASSERT("contains found not", cstr_contains(a, cstr("setting")));
    MUH_ASSERT("contains found not", cstr_contains(a, cstr("ser")));
}

MUH_NIT_CASE(test_for_word_space)
{
    cstr sentence = cstr("some simple sentence for testing");
    const char *ref = ptr(sentence);
    int i = 0;

    FOR_ITER_CSTR(word, sentence, " ")
    {
        MUH_ASSERT("to many words", i < 6);

        int j = 0;
        while (ref + j < end(sentence) && ref[j] != ' ')
            j++;

        MUH_ASSERT("wrong word length", len(word) == j);
        MUH_ASSERT("wrong word", ptr(word) == ref);

        i++;
        ref = &ref[j + 1];
    }

    MUH_ASSERT("skipped words", i == 5);
}

MUH_NIT_CASE(test_for_word_sep)
{
    cstr sentence = cstr("some--simple--sentence--for--testing");
    const char *ref = ptr(sentence);
    int i = 0;

    FOR_ITER_CSTR(word, sentence, "--")
    {
        MUH_ASSERT("to many words", i < 6);

        int j = 0;
        while (ref + j < end(sentence) && ref[j] != '-')
            j++;

        MUH_ASSERT("wrong word length", len(word) == j);
        MUH_ASSERT("wrong word", ptr(word) == ref);

        i++;
        ref = &ref[j + 2];
    }

    MUH_ASSERT("skipped words", i == 5);
}

MUH_NIT_CASE(dumb_test, SKIP)
{
    MUH_FAIL("this test always fails");
}

MUH_NIT_FIXTURE(test_fixture, TABLE(int, float, const char *),
                {1, 0, "blah"},
                {2, 5, "blub"}, )

MUH_NIT_CASE(fixture_test, FIXTURE(test_fixture))
{
    static int i;
    printf("%d: ", i);
    MUH_FIXTURE_BIND(test_fixture, ROW(a, b, msg));
    printf("%d %f %s\n", a, b, msg);
    i++;

    if (a == 1)
    {
        MUH_ASSERT("b is wrong", b == 0);
        MUH_ASSERT("msg is wrong", strncmp("blah", msg, 4) == 0);
        return;
    }
    if (a == 2)
    {
        MUH_ASSERT("b is wrong", b == 5);
        MUH_ASSERT("msg is wrong", strncmp("blub", msg, 4) == 0);
        puts("2 alright");
        return;
    }

    MUH_FAIL("unreachable");
}

const char *setup_test(void)
{
    return "Hello World";
}

void teardown_test(const char *input)
{
    assert(strcmp("Hello World", input) == 0);
}

MUH_NIT_FIXTURE(wrap_fixture, WRAPPER(const char *, setup_test, teardown_test))

MUH_NIT_CASE(wrapper_test, FIXTURE(wrap_fixture))
{
    MUH_FIXTURE_BIND(wrap_fixture, message);
    MUH_ASSERT("doesn't work", strcmp("Hello World", message) == 0);
}

int main(int argc, const char **args)
{
    muh_nit_case cases[] = MUH_CASES(
        test_cstr_from_char_ptr,
        test_cstring_from_char_ptr,
        test_cstr_from_string,
        test_cstring_from_cstr,
        test_cstring_append,
        test_cstr_match,
        test_find_first,
        test_contains,
        test_for_word_space,
        test_for_word_sep,
        dumb_test,
        fixture_test,
        wrapper_test);

    muh_setup(argc, args, cases);
    muh_nit_run(cases);
    return muh_nit_evaluate(cases);
}
