/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/* author: Matthias Meißner (geige.matze@gmail.com) */

#include <stdio.h>
#include <stdlib.h>

typedef struct muh_error
{
    char *error_message;
    int line_number;
    char *file_name;
    char *test_case;
    struct muh_error *next;
} muh_error;

typedef struct muh_test_result
{
    int tests_run;
    int failed_tests;
    muh_error *error_list;
} muh_test_result;

typedef muh_error *(*muh_nit_case)(void);

#define MUH_CASES(...)    \
    {                     \
        __VA_ARGS__, NULL \
    }

muh_error *muh_new_error(int line, char *file, char *error_message)
{
    muh_error *result = malloc(sizeof(muh_error));
    result->test_case = NULL;
    result->next = NULL;
    result->line_number = line;
    result->file_name = file;
    result->error_message = error_message;
    return result;
}

void muh_append_error(muh_error **list, muh_error *next)
{
    while (*list != NULL)
        list = &(*list)->next;

    *list = next;
}

void muh_free_error(muh_error *error)
{
    while (error != NULL)
    {
        muh_error *temp = error->next;
        free(error);
        error = temp;
    }
}

int muh_nit_evaluate(muh_test_result result)
{
    muh_error *temp = result.error_list;

    if (temp)
    {
        puts("\n========================================");
        printf("test case %s failed:\n"
               "[%s, line %d]: %s\n",
               temp->test_case, temp->file_name, temp->line_number, temp->error_message);
        temp = temp->next;
    }
    muh_free_error(result.error_list);

    printf("\nran %d tests with %d failures\n", result.tests_run, result.failed_tests);

    return (result.failed_tests > 0);
}

muh_test_result muh_nit_run(muh_nit_case muh_cases[])
{
    muh_test_result test_result = {0, 0, NULL};
    for (; *muh_cases != NULL; muh_cases++)
    {
        muh_error *res = (**muh_cases)();
        test_result.tests_run++;

        if (res)
        {
            test_result.failed_tests++;
            muh_append_error(&test_result.error_list, res);
        }
    }
    return test_result;
}

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

void muh_print_result(muh_error *res)
{
    if (!res)
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

#define MUH_NIT_CASE(case_ident)                 \
    muh_error *case_ident##inner(void);          \
    muh_error *case_ident(void)                  \
    {                                            \
        printf("running " #case_ident "... ");   \
        muh_error *result = case_ident##inner(); \
        muh_print_result(result);                \
        if (result)                              \
            result->test_case = #case_ident;     \
        return result;                           \
    }                                            \
    muh_error *case_ident##inner(void)

#define MUH_ASSERT(error_msg, assertion)                         \
    do                                                           \
    {                                                            \
        if (!(assertion))                                        \
            return muh_new_error(__LINE__, __FILE__, error_msg); \
    } while (0)

#define MUH_SUCCESS NULL

#define MUH_ERROR(message) muh_new_error(__LINE__, __FILE__, message)