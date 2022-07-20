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
    muh_error (*run)(void);
    muh_error error;
    char stdout_temp_file[MUH_TEMPFILE_TEMPLATE_LEN];
    char stderr_temp_file[MUH_TEMPFILE_TEMPLATE_LEN];
} muh_nit_case;

#define MUH_CASES(...)        \
    {                         \
        __VA_ARGS__, { NULL } \
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

void muh_print_result(muh_error res)
{
    if (!muh_contains_error(res))
    {
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

void muh_nit_run_case(muh_nit_case *test_case)
{
    close(mkstemp(test_case->stdout_temp_file));
    close(mkstemp(test_case->stderr_temp_file));
    printf("running %s... ", test_case->test_name);

    freopen(test_case->stdout_temp_file, "w", stdout);
    freopen(test_case->stderr_temp_file, "w", stderr);
    test_case->error = test_case->run();
    freopen("/dev/tty", "w", stdout);
    freopen("/dev/tty", "w", stderr);

    if (!muh_contains_error(test_case->error))
    {
        remove(test_case->stderr_temp_file);
        remove(test_case->stdout_temp_file);
    }

    muh_print_result(test_case->error);
}

void muh_nit_run(muh_nit_case muh_cases[])
{
    for (muh_nit_case *it = muh_cases; it->test_name != NULL; it++)
    {
        muh_nit_run_case(it);
    }
}

#define MUH_NIT_CASE(case_ident)                   \
    muh_error case_ident##__inner_fun(void);       \
    static muh_nit_case case_ident = {             \
        #case_ident,                               \
        &case_ident##__inner_fun,                  \
        {MUH_ERROR_CODE(MUH_UNINITIALIZED_ERROR)}, \
        MUH_TEMPFILE_TEMPLATE(stdout),             \
        MUH_TEMPFILE_TEMPLATE(stderr),             \
    };                                             \
    muh_error case_ident##__inner_fun(void)

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
