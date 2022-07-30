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

typedef struct muh_fixture_table
{
    void *data;
    size_t row_width;
    size_t data_size;
} muh_fixture_table;

#define MUH_TEMPFILE_TEMPLATE(stream) ("muh_test_" #stream "XXXXXX")
#define MUH_TEMPFILE_TEMPLATE_LEN 22

typedef struct muh_nit_case
{
    const char *test_name;
    bool skip;
    void (*run)(muh_error *, void *);
    muh_error error;
    char stdout_temp_file[MUH_TEMPFILE_TEMPLATE_LEN];
    char stderr_temp_file[MUH_TEMPFILE_TEMPLATE_LEN];
    muh_fixture_table *fixture_table;
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

    if (test_case->fixture_table != NULL)
    {
        void *current = test_case->fixture_table->data;
        void *end = (void *)((unsigned long)current + test_case->fixture_table->data_size);

        while (current != end)
        {
            test_case->run(&test_case->error, current);

            if (muh_contains_error(test_case->error))
                break;

            freopen(test_case->stdout_temp_file, "w", stdout);
            freopen(test_case->stderr_temp_file, "w", stderr);

            current = (void *)((unsigned long)current + test_case->fixture_table->row_width);
        }
    }
    else
    {
        test_case->run(&test_case->error, NULL);
        if (test_case->error.error_code == MUH_UNINITIALIZED_ERROR)
            test_case->error.error_code = MUH_NO_ERROR;
    }

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

#define FST(x, ...) x
#define SND(x, y, ...) y
#define MACRO_APP(f, args) f args

#define FIXTURE_PARAMETER __fixture_data
#define ERROR_PARAMETER __muh_error_res

#define MUH_NIT_CASE(case_ident, ...)                  \
    void case_ident##__inner_fun(muh_error *, void *); \
    static muh_nit_case case_ident = {                 \
        #case_ident,                                   \
        EVAL(FIND_SKIP(__VA_ARGS__)),                  \
        &case_ident##__inner_fun,                      \
        {MUH_ERROR_CODE(MUH_UNINITIALIZED_ERROR)},     \
        MUH_TEMPFILE_TEMPLATE(stdout),                 \
        MUH_TEMPFILE_TEMPLATE(stderr),                 \
        EVAL(FIND_FIXTURE(__VA_ARGS__)),               \
    };                                                 \
    void case_ident##__inner_fun(muh_error *ERROR_PARAMETER, void *FIXTURE_PARAMETER)

#define MUH_ASSERT(message, assertion)               \
    do                                               \
    {                                                \
        if (!(assertion))                            \
        {                                            \
            *ERROR_PARAMETER = ((muh_error){         \
                MUH_ERROR_CODE(MUH_ASSERTION_ERROR), \
                __LINE__,                            \
                __FILE__,                            \
                message,                             \
            });                                      \
            return;                                  \
        }                                            \
    } while (0)

#define MUH_FAIL(message)                   \
    do                                      \
    {                                       \
        *ERROR_PARAMETER = ((muh_error){    \
            MUH_ERROR_CODE(MUH_MISC_ERROR), \
            __LINE__,                       \
            __FILE__,                       \
            message,                        \
        });                                 \
        return;                             \
    } while (0)

#define EVAL(...) EVAL1(EVAL1(EVAL1(__VA_ARGS__)))
#define EVAL1(...) EVAL2(EVAL2(EVAL2(__VA_ARGS__)))
#define EVAL2(...) EVAL3(EVAL3(EVAL3(__VA_ARGS__)))
#define EVAL3(...) EVAL4(EVAL4(EVAL4(__VA_ARGS__)))
#define EVAL4(...) EVAL5(EVAL5(EVAL5(__VA_ARGS__)))
#define EVAL5(...) __VA_ARGS__

#define EMPTY(...)
#define DEFER(...) __VA_ARGS__ EMPTY()
#define OBSTRUCT(...) __VA_ARGS__ DEFER(EMPTY)()
#define EXPAND(...) __VA_ARGS__

#define CAT(a, ...) PRIMITIVE_CAT(a, __VA_ARGS__)
#define PRIMITIVE_CAT(a, ...) a##__VA_ARGS__

#define INDIRECT_SND(...) SND(__VA_ARGS__)
#define IS_NON_EMPTY(x, ...) EXPAND(INDIRECT_SND(PRIMITIVE_CAT(IS_NON_EMPTY_, x), 1))
#define IS_NON_EMPTY_ ~, 0

#define WHEN(c) CAT(WHEN_, c)
#define WHEN_0 EMPTY
#define WHEN_1 EXPAND

#define INC(x) CAT(INC_, x)
#define INC_0 1
#define INC_1 2
#define INC_2 3
#define INC_3 4
#define INC_4 5
#define INC_5 6
#define INC_6 7
#define INC_7 8
#define INC_8 9

#define FIXTURE_FIELD(counter) CAT(__fixture_field_, counter)
#define FIXTURE_FIELD_TYPE(name, counter) CAT(CAT(__, name), CAT(_field, counter))

#define TABLE_FIELDS_ID() TABLE_FIELDS
#define TABLE_FIELDS(counter, x, ...) \
    WHEN(IS_NON_EMPTY(x))             \
    (x FIXTURE_FIELD(counter);        \
     OBSTRUCT(TABLE_FIELDS_ID)()(INC(counter), __VA_ARGS__))
#define TABLE(...) (struct {TABLE_FIELDS(0, __VA_ARGS__)}, __VA_ARGS__)

#define TABLE_TYPES_ID() TABLE_TYPES
#define TABLE_TYPES_IND(...) TABLE_TYPES(__VA_ARGS__)
#define TABLE_TYPES(name, counter, x, ...)        \
    WHEN(IS_NON_EMPTY(x))                         \
    (typedef x FIXTURE_FIELD_TYPE(name, counter); \
     OBSTRUCT(TABLE_TYPES_ID)()(name, INC(counter), __VA_ARGS__))

#define OR(x) CAT(OR_, x)
#define OR_0(y) y
#define OR_1(_) 1

#define NOT(x) CAT(NOT_, x)
#define NOT_1 0
#define NOT_0 1

#define IS_EMPTY(x) NOT(IS_NON_EMPTY(x))

#define IIF(x) CAT(IIF_, x)
#define IIF_0(_, x) x
#define IIF_1(x, _) x

#define FIND_SKIP_ID() FIND_SKIP
#define FIND_SKIP(x, ...)            \
    IIF(OR(IS_SKIP(x))(IS_EMPTY(x))) \
    (IS_SKIP(x), OBSTRUCT(FIND_SKIP_ID)()(__VA_ARGS__))
#define IS_SKIP(x) INDIRECT_SND(CAT(IS_SKIP_, x), 0)
#define IS_SKIP_SKIP ~, 1

#define FIND_FIXTURE_ID() FIND_FIXTURE
#define FIND_FIXTURE(x, ...)            \
    IIF(OR(IS_EMPTY(x))(IS_FIXTURE(x))) \
    (FIXTURE_PARAM(x), OBSTRUCT(FIND_FIXTURE_ID)()(__VA_ARGS__))
#define IS_FIXTURE(x) INDIRECT_SND(CAT(IS_FIXTURE_, x), 0)
#define IS_FIXTURE_FIXTURE(...) ~, 1
#define FIXTURE_PARAM(x) INDIRECT_SND(CAT(FIXTURE_PARAM_, x), NULL)
#define FIXTURE_PARAM_FIXTURE(p) ~, &p

#define STR(x) STR_INNER(x)
#define STR_INNER(x, ...) #x
#define TAIL(x, ...) __VA_ARGS__

#define MUH_NIT_FIXTURE(name, layout, ...)                        \
    typedef EVAL(MACRO_APP(FST, layout)) name##__fixture_struct;  \
    EVAL(TABLE_TYPES_IND(name, 0, MACRO_APP(TAIL, layout)));      \
    static name##__fixture_struct name##__data[] = {__VA_ARGS__}; \
    static muh_fixture_table name = {                             \
        name##__data,                                             \
        sizeof(name##__fixture_struct),                           \
        sizeof(name##__data),                                     \
    };

#define FIXTURE_INIT_ID() FIXTURE_INIT
#define FIXTURE_INIT(name, counter, arg, ...)                                          \
    WHEN(IS_NON_EMPTY(arg))                                                            \
    (FIXTURE_FIELD_TYPE(name, counter) arg = __muh_fixture_tmp.FIXTURE_FIELD(counter); \
     OBSTRUCT(FIXTURE_INIT_ID)()(name, INC(counter), __VA_ARGS__))

#define MUH_FIXTURE_BIND(name, ...)                                                          \
    name##__fixture_struct __muh_fixture_tmp = *(name##__fixture_struct *)FIXTURE_PARAMETER; \
    EVAL(FIXTURE_INIT(name, 0, __VA_ARGS__))                                                 \
    while (false)
