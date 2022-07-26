/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/* author: Matthias Meißner (geige.matze@gmail.com) */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

typedef enum terminal_color
{
    terminal_color_red,
    terminal_color_yellow,
    terminal_color_green,
    terminal_color_default
} terminal_color;

void muh_set_terminal_color(terminal_color color)
{
    switch (color)
    {
    case terminal_color_red:
        printf("\033[31m");
        break;

    case terminal_color_green:
        printf("\033[32m");
        break;

    case terminal_color_yellow:
        printf("\033[33m");
        break;

    default:
        printf("\033[0m");
        break;
    }
}

typedef enum muh_error_code
{
    MUH_UNINITIALIZED_ERROR,
    MUH_NO_ERROR,
    MUH_ASSERTION_ERROR,
    MUH_MISC_ERROR,
} muh_error_code;

#define MUH_ERROR_CODE(x) ((muh_error_code){x})

typedef struct muh_error
{
    muh_error_code error_code;
    int line_number;
    const char *file_name;
    const char *error_message;
} muh_error;

#define MUH_TEMPFILE_TEMPLATE(stream) ("muh_test_" #stream "XXXXXX")
#define MUH_TEMPFILE_TEMPLATE_LEN 22

typedef struct muh_nit_case
{
    const char *test_name;
    bool skip;
#ifdef __cplusplus
    muh_error (*run)(...);
#else
    muh_error (*run)();
#endif
    muh_error error;
    char stdout_temp_file[MUH_TEMPFILE_TEMPLATE_LEN];
    char stderr_temp_file[MUH_TEMPFILE_TEMPLATE_LEN];
} muh_nit_case;

#define MUH_CASES(...)        \
    {                         \
        __VA_ARGS__, { NULL } \
    }

void muh_mark_skip(muh_nit_case cases[], const char *skip_name)
{
    for (muh_nit_case *it = cases; it->test_name != NULL; it++)
    {
        if (strcmp(it->test_name, skip_name) == 0)
        {
            it->skip = true;
            break;
        }
    }
}

void muh_mark_only(muh_nit_case cases[], const char *only_name)
{
    for (muh_nit_case *it = cases; it->test_name != NULL; it++)
    {
        if (strcmp(it->test_name, only_name) != 0)
        {
            it->skip = true;
        }
        else
        {
            it->skip = false;
        }
    }
}

void muh_setup(int argc, const char **argv, muh_nit_case cases[])
{
    // skip executable name
    argc--;
    argv++;

    while (argc--)
    {
        if (strcmp("--skip", *argv) == 0)
        {
            if (argc == 0)
            {
                fputs("muh_nit: missing argument for --skip\n", stderr);
                exit(1);
            }
            else
            {
                argc--;
                muh_mark_skip(cases, *++argv);
            }
        }
        else if (strcmp("--only", *argv) == 0)
        {
            if (argc == 0)
            {
                fputs("muh_nit: missing argument for --only\n", stderr);
                exit(1);
            }
            else
            {
                muh_mark_only(cases, *++argv);
                break;
            }
        }
        argv++;
    }
}

bool muh_contains_error(muh_error error)
{
    return error.error_code != MUH_NO_ERROR &&
           error.error_code != MUH_UNINITIALIZED_ERROR;
}

void print_temp_file(const char *name, const char *message)
{
    FILE *stdout_tmp = fopen(name, "r");

    if (stdout_tmp != NULL)
    {
        const int buffer_len = 1024;
        char buffer[buffer_len];

        if (fgets(buffer, buffer_len, stdout_tmp) != NULL)
        {
            puts(message);
            do
            {
                printf("%s", buffer);
            } while (fgets(buffer, buffer_len, stdout_tmp) != NULL);
            puts("\n*****");
        }
    }

    fclose(stdout_tmp);
    remove(name);
}

void muh_print_error(muh_nit_case *test_case)
{
    if (!muh_contains_error(test_case->error))
        return;

    puts("\n========================================");
    printf("test case %s failed:\n"
           "[%s, line %d]: %s\n\n",
           test_case->test_name,
           test_case->error.file_name,
           test_case->error.line_number,
           test_case->error.error_message);

    print_temp_file(test_case->stdout_temp_file, "contents of stdout:");
    print_temp_file(test_case->stderr_temp_file, "contents of stderr:");
}

int muh_nit_evaluate(muh_nit_case cases[])
{
    int failed_tests = 0;
    int skipped_tests = 0;
    int passed_tests = 0;

    for (muh_nit_case *it = cases; it->test_name != NULL; it++)
        switch (it->error.error_code)
        {
        case MUH_NO_ERROR:
            passed_tests++;
            break;

        case MUH_UNINITIALIZED_ERROR:
            skipped_tests++;
            break;

        default:
            muh_print_error(it);
            failed_tests++;
            break;
        }

    printf("\n%d passed, %d failures, %d skipped\n",
           passed_tests, failed_tests, skipped_tests);

    return (failed_tests > 0);
}

void muh_nit_run_case(muh_nit_case *test_case)
{
    printf("running %s... ", test_case->test_name);

    if (test_case->skip)
    {
        muh_set_terminal_color(terminal_color_yellow);
        puts("skipped");
        muh_set_terminal_color(terminal_color_default);
        return;
    }

    close(mkstemp(test_case->stdout_temp_file));
    close(mkstemp(test_case->stderr_temp_file));

    freopen(test_case->stdout_temp_file, "w", stdout);
    freopen(test_case->stderr_temp_file, "w", stderr);
    test_case->error = test_case->run();
    freopen("/dev/tty", "w", stdout);
    freopen("/dev/tty", "w", stderr);

    if (!muh_contains_error(test_case->error))
    {
        remove(test_case->stderr_temp_file);
        remove(test_case->stdout_temp_file);

        muh_set_terminal_color(terminal_color_green);
        puts("ok");
        muh_set_terminal_color(terminal_color_default);
    }
    else
    {
        muh_set_terminal_color(terminal_color_red);
        puts("failed");
        muh_set_terminal_color(terminal_color_default);
    }
}

void muh_nit_run(muh_nit_case muh_cases[])
{
    for (muh_nit_case *it = muh_cases; it->test_name != NULL; it++)
    {
        muh_nit_run_case(it);
    }
}

#define SKIP true, (NULL), true
#define TAIL(x, ...) __VA_ARGS__
#define FST(x, ...) x
#define SND(x, y, ...) y
#define THRD(x, y, z, ...) z
#define MACRO_APP(f, args) f args

#define FIXTURE(name, ...) , (name, __VA_ARGS__)

#ifdef __cplusplus

#define MUH_NIT_CASE(case_ident, ...)                                               \
    muh_error case_ident##__inner_fun(MACRO_APP(TAIL, SND(__VA_ARGS__, (NULL, )))); \
    static muh_nit_case case_ident = {                                              \
        #case_ident,                                                                \
        THRD(__VA_ARGS__, false, false),                                            \
        (muh_error(*)(...)) & case_ident##__inner_fun,                              \
        {MUH_ERROR_CODE(MUH_UNINITIALIZED_ERROR)},                                  \
        MUH_TEMPFILE_TEMPLATE(stdout),                                              \
        MUH_TEMPFILE_TEMPLATE(stderr),                                              \
    };                                                                              \
    muh_error case_ident##__inner_fun(MACRO_APP(TAIL, SND(__VA_ARGS__, (NULL, ))))

#else

#define MUH_NIT_CASE(case_ident, ...)                                               \
    muh_error case_ident##__inner_fun(MACRO_APP(TAIL, SND(__VA_ARGS__, (NULL, )))); \
    static muh_nit_case case_ident = {                                              \
        #case_ident,                                                                \
        THRD(__VA_ARGS__, false, false),                                            \
        (muh_error(*)()) & case_ident##__inner_fun,                                 \
        {MUH_ERROR_CODE(MUH_UNINITIALIZED_ERROR)},                                  \
        MUH_TEMPFILE_TEMPLATE(stdout),                                              \
        MUH_TEMPFILE_TEMPLATE(stderr),                                              \
    };                                                                              \
    muh_error case_ident##__inner_fun(MACRO_APP(TAIL, SND(__VA_ARGS__, (NULL, ))))

#endif

#define MUH_ASSERT(message, assertion)               \
    do                                               \
    {                                                \
        if (!(assertion))                            \
            return ((muh_error){                     \
                MUH_ERROR_CODE(MUH_ASSERTION_ERROR), \
                __LINE__,                            \
                __FILE__,                            \
                message,                             \
            });                                      \
    } while (0)

#define MUH_SUCCESS ((muh_error){MUH_ERROR_CODE(MUH_NO_ERROR)})

#define MUH_ERROR(message) ((muh_error){ \
    MUH_ERROR_CODE(MUH_MISC_ERROR),      \
    __LINE__,                            \
    __FILE__,                            \
    message,                             \
})
