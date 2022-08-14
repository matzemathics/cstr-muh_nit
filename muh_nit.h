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
#include <assert.h>

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

typedef struct muh_error
{
    muh_error_code error_code;
    int line_number;
    const char *file_name;
    const char *error_message;
} muh_error;

bool muh_contains_error(muh_error error)
{
    return error.error_code != MUH_NO_ERROR &&
           error.error_code != MUH_UNINITIALIZED_ERROR;
}

struct muh_nit_fixture;

typedef struct muh_nit_case
{
    const char *test_name;
    bool skip;
    void (*run)(muh_error *, void *);
    muh_error error;
    int fd_stdout;
    int fd_stderr;
    struct muh_nit_fixture *fixture;
} muh_nit_case;

#define MUH_CASES(...)        \
    {                         \
        __VA_ARGS__, { NULL } \
    }

typedef struct muh_nit_fixture
{
    void (*run_test_case)(muh_nit_case *);
} muh_nit_fixture;

typedef struct muh_nit_table
{
    muh_nit_fixture base;
    void *data;
    void *end;
    size_t row_width;
} muh_nit_table;

void muh_nit_table_run_test_case(muh_nit_case *test_case)
{
    muh_nit_table *self = (muh_nit_table *)test_case->fixture;
    void *current = self->data;

    while (current < self->end)
    {
        test_case->run(&test_case->error, current);
        if (muh_contains_error(test_case->error))
            break;

        current = (void *)((unsigned long)current + self->row_width);
        // TODO: reopen stream?
    }
}

#define __MUH_MK_TABLE(data, width) \
    (muh_nit_table) { {&muh_nit_table_run_test_case}, data, (void *)((unsigned long)data + sizeof(data)), width }

typedef struct muh_nit_wrapper
{
    muh_nit_fixture base;
    void *(*setup)(void);
    void (*teardown)(void *);
} muh_nit_wrapper;

void muh_nit_wrapper_run_test_case(muh_nit_case *test_case)
{
    muh_nit_wrapper *self = (muh_nit_wrapper *)test_case->fixture;
    void *data = self->setup();
    test_case->run(&test_case->error, data);
    self->teardown(data);
}

#define __MUH_MK_WRAPPER_FIXTURE(setup, teardown) \
    (muh_nit_wrapper) { {&muh_nit_wrapper_run_test_case}, (void *(*)(void))setup, (void (*)(void *))teardown }

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

void print_temp_file(int fd, const char *message)
{
    const int buffer_len = 1024;
    char buffer[buffer_len];
    size_t read_len = 0;

    lseek(fd, 0, SEEK_SET);

    if ((read_len = read(fd, buffer, buffer_len - 1)) != 0)
    {
        buffer[read_len] = 0;
        puts(message);
        do
        {
            printf("%s", buffer);
        } while (read(fd, buffer, buffer_len) != 0);
        puts("\n*****");
    }

    close(fd);
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

    print_temp_file(test_case->fd_stdout, "contents of stdout:");
    print_temp_file(test_case->fd_stderr, "contents of stderr:");
}

bool muh_nit_evaluate(muh_nit_case cases[])
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

int redirect_stream(FILE *stream)
{
    int stream_fd = fileno(stream);
    char name_template[] = "muh_test_%04d_XXXXXX";
    sprintf(name_template, name_template, stream_fd);
    int fd = mkstemp(name_template);
    dup2(fd, stream_fd);
    unlink(name_template);
    return fd;
}

void muh_nit_run_case(muh_nit_case *test_case)
{
    printf("running %s... ", test_case->test_name);
    fflush(stdout);

    if (test_case->skip)
    {
        muh_set_terminal_color(terminal_color_yellow);
        puts("skipped");
        muh_set_terminal_color(terminal_color_default);
        return;
    }

    test_case->fd_stdout = redirect_stream(stdout);
    test_case->fd_stderr = redirect_stream(stderr);

    if (test_case->fixture != NULL)
        test_case->fixture->run_test_case(test_case);
    else
        test_case->run(&test_case->error, NULL);

    if (test_case->error.error_code == MUH_UNINITIALIZED_ERROR)
        test_case->error.error_code = MUH_NO_ERROR;

    freopen("/dev/tty", "w", stdout);
    freopen("/dev/tty", "w", stderr);

    if (!muh_contains_error(test_case->error))
    {
        close(test_case->fd_stdout);
        close(test_case->fd_stderr);

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

#define __MUH_FIX_DATA_ARG __fixture_data
#define __MUH_ERR_ARG __muh_error_res

#define MUH_NIT_CASE(case_ident, ...)                    \
    void case_ident##__inner_fun(muh_error *, void *);   \
    static muh_nit_case case_ident = {                   \
        #case_ident,                                     \
        __MUH_HLP_EVAL(__MUH_FIND_SKIP(__VA_ARGS__)),    \
        &case_ident##__inner_fun,                        \
        {MUH_UNINITIALIZED_ERROR},                       \
        -1, /* stdout file descriptor */                 \
        -1, /* stderr file descriptor */                 \
        __MUH_HLP_EVAL(__MUH_FIND_FIXTURE(__VA_ARGS__)), \
    };                                                   \
    void case_ident##__inner_fun(muh_error *__MUH_ERR_ARG, void *__MUH_FIX_DATA_ARG)

#define MUH_ASSERT(message, assertion)     \
    do                                     \
    {                                      \
        if (!(assertion))                  \
        {                                  \
            *__MUH_ERR_ARG = ((muh_error){ \
                MUH_ASSERTION_ERROR,       \
                __LINE__,                  \
                __FILE__,                  \
                message,                   \
            });                            \
            return;                        \
        }                                  \
    } while (0)

#define MUH_FAIL(message)              \
    do                                 \
    {                                  \
        *__MUH_ERR_ARG = ((muh_error){ \
            MUH_MISC_ERROR,            \
            __LINE__,                  \
            __FILE__,                  \
            message,                   \
        });                            \
        return;                        \
    } while (0)

#define __MUH_HLP_EVAL(...) __MUH_HLP_EVAL1(__MUH_HLP_EVAL1(__MUH_HLP_EVAL1(__VA_ARGS__)))
#define __MUH_HLP_EVAL1(...) __MUH_HLP_EVAL2(__MUH_HLP_EVAL2(__MUH_HLP_EVAL2(__VA_ARGS__)))
#define __MUH_HLP_EVAL2(...) __MUH_HLP_EVAL3(__MUH_HLP_EVAL3(__MUH_HLP_EVAL3(__VA_ARGS__)))
#define __MUH_HLP_EVAL3(...) __MUH_HLP_EVAL4(__MUH_HLP_EVAL4(__MUH_HLP_EVAL4(__VA_ARGS__)))
#define __MUH_HLP_EVAL4(...) __MUH_HLP_EVAL5(__MUH_HLP_EVAL5(__MUH_HLP_EVAL5(__VA_ARGS__)))
#define __MUH_HLP_EVAL5(...) __VA_ARGS__

#define __MUH_HLP_EMPTY(...)
#define __MUH_HLP_DEFER(...) __VA_ARGS__ __MUH_HLP_EMPTY()
#define __MUH_HLP_OBSTRUCT(...) __VA_ARGS__ __MUH_HLP_DEFER(__MUH_HLP_EMPTY)()

#define __MUH_HLP_CAT(a, ...) __MUH_HLP_PRIM_CAT(a, __VA_ARGS__)
#define __MUH_HLP_PRIM_CAT(a, ...) a##__VA_ARGS__

#define __MUH_HLP_SND(x, y, ...) y
#define __MUH_HLP_NON_EMPTY(x, ...) __MUH_HLP_DEFER(__MUH_HLP_SND)(__MUH_HLP_PRIM_CAT(__MUH_HLP_NON_EMPTY_, x), 1)
#define __MUH_HLP_NON_EMPTY_ ~, 0

#define __MUH_HLP_INC(x) __MUH_HLP_CAT(__MUH_HLP_INC_, x)
#define __MUH_HLP_INC_0 1
#define __MUH_HLP_INC_1 2
#define __MUH_HLP_INC_2 3
#define __MUH_HLP_INC_3 4
#define __MUH_HLP_INC_4 5
#define __MUH_HLP_INC_5 6
#define __MUH_HLP_INC_6 7
#define __MUH_HLP_INC_7 8
#define __MUH_HLP_INC_8 9

#define __MUH_HLP_OR(x) __MUH_HLP_CAT(__MUH_HLP_OR_, x)
#define __MUH_HLP_OR_0(y) y
#define __MUH_HLP_OR_1(_) 1

#define __MUH_HLP_NOT(x) __MUH_HLP_CAT(__MUH_HLP_NOT_, x)
#define __MUH_HLP_NOT_1 0
#define __MUH_HLP_NOT_0 1

#define __MUH_HLP_IF(x) __MUH_HLP_CAT(__MUH_HLP_IF_, x)
#define __MUH_HLP_IF_0(_, x) x
#define __MUH_HLP_IF_1(x, _) x

#define __MUH_FIXTURE_FIELD(counter) __MUH_HLP_CAT(__fixture_field_, counter)
#define __MUH_FIXTURE_FIELD_TYPE(name, counter) __MUH_HLP_CAT(__##name##_field_, counter)

#define __MUH_TABLE_FIELDS_ID() __MUH_TABLE_FIELDS
#define __MUH_TABLE_FIELDS(counter, x, ...) \
    __MUH_HLP_IF(__MUH_HLP_NON_EMPTY(x))    \
    (x __MUH_FIXTURE_FIELD(counter);        \
     __MUH_HLP_OBSTRUCT(__MUH_TABLE_FIELDS_ID)()(__MUH_HLP_INC(counter), __VA_ARGS__), )

#define __MUH_FIX_TYPE_TABLE(...)          \
    struct                                 \
    {                                      \
        __MUH_TABLE_FIELDS(0, __VA_ARGS__) \
    }

#define __MUH_TYPES_TABLE(...) __VA_ARGS__

#define __MUH_TABLE_TYPES_ID() __MUH_TABLE_TYPES
#define __MUH_TABLE_TYPES_IND(...) __MUH_TABLE_TYPES(__VA_ARGS__)
#define __MUH_TABLE_TYPES(name, counter, x, ...)        \
    __MUH_HLP_IF(__MUH_HLP_NON_EMPTY(x))                \
    (typedef x __MUH_FIXTURE_FIELD_TYPE(name, counter); \
     __MUH_HLP_OBSTRUCT(__MUH_TABLE_TYPES_ID)()(name, __MUH_HLP_INC(counter), __VA_ARGS__), )

#define __MUH_FIND_SKIP_ID() __MUH_FIND_SKIP
#define __MUH_FIND_SKIP(x, ...)                                                         \
    __MUH_HLP_IF(__MUH_HLP_OR(__MUH_IS_SKIP(x))(__MUH_HLP_NOT(__MUH_HLP_NON_EMPTY(x)))) \
    (__MUH_IS_SKIP(x), __MUH_HLP_OBSTRUCT(__MUH_FIND_SKIP_ID)()(__VA_ARGS__))
#define __MUH_IS_SKIP(x) __MUH_HLP_DEFER(__MUH_HLP_SND)(__MUH_HLP_CAT(__MUH_IS_SKIP_, x), 0)
#define __MUH_IS_SKIP_SKIP ~, 1

#define __MUH_FIND_FIXTURE_ID() __MUH_FIND_FIXTURE
#define __MUH_FIND_FIXTURE(x, ...)                                                         \
    __MUH_HLP_IF(__MUH_HLP_OR(__MUH_HLP_NOT(__MUH_HLP_NON_EMPTY(x)))(__MUH_IS_FIXTURE(x))) \
    (__MUH_FIXTURE_PARAM(x), __MUH_HLP_OBSTRUCT(__MUH_FIND_FIXTURE_ID)()(__VA_ARGS__))
#define __MUH_IS_FIXTURE(x) __MUH_HLP_DEFER(__MUH_HLP_SND)(__MUH_HLP_CAT(__MUH_IS_FIXTURE_, x), 0)
#define __MUH_IS_FIXTURE_FIXTURE(...) ~, 1
#define __MUH_FIXTURE_PARAM(x) __MUH_HLP_DEFER(__MUH_HLP_SND)(__MUH_HLP_CAT(__MUH_FIXTURE_PARAM_, x), NULL)
#define __MUH_FIXTURE_PARAM_FIXTURE(p) ~, (muh_nit_fixture *)&p

#define __DBG_STR(x) __DBG_STR_INNER(x)
#define __DBG_STR_INNER(x, ...) #x

#define __MUH_CASE_TABLE(name, layout, ...)                                 \
    typedef __MUH_HLP_EVAL(__MUH_FIX_TYPE_##layout) name##__fixture_struct; \
    __MUH_HLP_EVAL(__MUH_TABLE_TYPES_IND(name, 0, __MUH_TYPES_##layout));   \
    static name##__fixture_struct name##__data[] = {__VA_ARGS__};           \
    static muh_nit_table name = __MUH_MK_TABLE(name##__data, sizeof(name##__fixture_struct));

#define __MUH_TYPE_WRAPPER(type, ...) type
#define __MUH_SETUP_WRAPPER(type, setup, ...) &setup
#define __MUH_TEARDOWN_WRAPPER(...) __MUH_TEARDOWN_WRAPPER_INNER(__VA_ARGS__, )
#define __MUH_TEARDOWN_WRAPPER_INNER(type, setup, teardown, ...) \
    __MUH_HLP_IF(__MUH_HLP_NON_EMPTY(teardown))                  \
    (&teardown, NULL)
#define __MUH_WRAPPER_FIXTURE(name, args, ...)      \
    typedef __MUH_TYPE_##args name##__fixture_type; \
    static muh_nit_wrapper name = __MUH_MK_WRAPPER_FIXTURE(__MUH_SETUP_##args, __MUH_TEARDOWN_##args)

#define __MUH_LAYOUT_SWITCH_TABLE(...) __MUH_CASE_TABLE
#define __MUH_LAYOUT_SWITCH_WRAPPER(...) __MUH_WRAPPER_FIXTURE
#define MUH_NIT_FIXTURE(name, layout, ...) __MUH_LAYOUT_SWITCH_##layout(name, layout, __VA_ARGS__);

#define __MUH_FIXTURE_INIT_ID() __MUH_FIXTURE_INIT
#define __MUH_FIXTURE_INIT(name, counter, arg, ...)                                                \
    __MUH_HLP_IF(__MUH_HLP_NON_EMPTY(arg))                                                         \
    (__MUH_FIXTURE_FIELD_TYPE(name, counter) arg = __muh_fixture_tmp.__MUH_FIXTURE_FIELD(counter); \
     __MUH_HLP_OBSTRUCT(__MUH_FIXTURE_INIT_ID)()(name, __MUH_HLP_INC(counter), __VA_ARGS__), )

#define __MUH_BIND_IS_ROW(arg) __MUH_HLP_DEFER(__MUH_HLP_SND)(__MUH_HLP_CAT(__MUH_FIXTURE_ROW_BIND_, arg), 0)
#define MUH_FIXTURE_BIND(name, arg)                     \
    __MUH_HLP_IF(__MUH_BIND_IS_ROW(arg))                \
    (__MUH_HLP_EVAL(__MUH_FIXTURE_BIND_ROW(name, arg)), \
     name##__fixture_type arg = (name##__fixture_type)__MUH_FIX_DATA_ARG;)

#define __MUH_ARGS_ROW(...) __VA_ARGS__
#define __MUH_FIXTURE_BIND_ROW(name, arg)                                                     \
    name##__fixture_struct __muh_fixture_tmp = *(name##__fixture_struct *)__MUH_FIX_DATA_ARG; \
    __MUH_HLP_EVAL(__MUH_HLP_DEFER(__MUH_FIXTURE_INIT)(name, 0, __MUH_ARGS_##arg))            \
    if (true)

#define __MUH_FIXTURE_ROW_BIND_ROW(...) ~, 1