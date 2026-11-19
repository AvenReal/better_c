#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "error.h"

static int run_expect_exit(void (*fn)(void), char *stderr_buf, size_t buf_size)
{
    int pipefd[2];
    assert(pipe(pipefd) == 0);

    fflush(stdout);
    fflush(stderr);

    pid_t pid = fork();
    assert(pid >= 0);

    if (pid == 0)
    {
        close(pipefd[0]);
        dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[1]);
        fn();
        _exit(0);
    }

    close(pipefd[1]);
    size_t total = 0;
    while (total + 1 < buf_size)
    {
        ssize_t n = read(pipefd[0], stderr_buf + total, buf_size - 1 - total);
        if (n <= 0)
        {
            break;
        }
        total += (size_t)n;
    }
    stderr_buf[total] = '\0';
    close(pipefd[0]);

    int status = 0;
    assert(waitpid(pid, &status, 0) == pid);
    assert(WIFEXITED(status));
    return WEXITSTATUS(status);
}

static void child_throw_plain(void)
{
    THROW_ERROR(ERR_MEMORY_ERROR, "plain message");
}

static void child_throw_fmt(void)
{
    THROW_ERROR_FMT(ERR_INVALID_ARGUMENT, "formatted code: %d", 123);
}

static void child_assert_fails(void)
{
    ASSERT(1 == 0, "assert plain message");
}

static void child_assert_fmt_fails(void)
{
    ASSERT_FMT(1 == 0, "assert fmt message: %d", 42);
}

static void child_print_stderr(void)
{
    PRINT_STDERR("plain print message");
    exit(0);
}

static void child_print_stderr_fmt(void)
{
    PRINT_STDERR_FMT("fmt print message: %s", "hello");
    exit(0);
}

static int side_effect_calls = 0;
static int get_side_effect_code(void)
{
    side_effect_calls++;
    return (side_effect_calls > 1) ? 99 : ERR_INVALID_ARGUMENT;
}

static void child_throw_side_effect(void)
{
    THROW_ERROR(get_side_effect_code(), "testing side effect single eval");
}

static void child_throw_fmt_side_effect(void)
{
    THROW_ERROR_FMT(get_side_effect_code(), "fmt side effect: %d", 1);
}

static void test_error_codes(void)
{
    assert(ERR_NO_ERROR == 0);
    assert(ERR_ASSERT_FAILED != 0);
    assert(ERR_MEMORY_ERROR != 0);
    assert(ERR_INVALID_ARGUMENT != 0);
    assert(ERR_RESULT_IS_ERR != 0);
    assert(ERR_RESULT_IS_OK != 0);
    assert(ERR_UNKNOWN != 0);
    assert(sizeof(__FILE_NAME__) > 0);
}

static void test_assert_success(void)
{
    ASSERT(1 == 1, "should not trigger");
    ASSERT(true, "should not trigger");
    ASSERT_FMT(1 + 1 == 2, "math failed: %d", 2);
}

static void test_assert_control_flow(void)
{
    int ran_else = 0;
    if (1)
        ASSERT(1 == 1, "should not fail");
    else
        ran_else = 1;
    assert(ran_else == 0);

    if (0)
        ASSERT(1 == 1, "unreachable");
    else
        ran_else = 2;
    assert(ran_else == 2);

    ran_else = 0;
    if (1)
        ASSERT_FMT(2 > 1, "fmt pass: %d", 1);
    else
        ran_else = 3;
    assert(ran_else == 0);
}

static void test_throwing_and_diagnostics(void)
{
    char buf[512];
    int code;

    /* Plain THROW_ERROR */
    code = run_expect_exit(child_throw_plain, buf, sizeof(buf));
    assert(code == ERR_MEMORY_ERROR);
    assert(strstr(buf, "plain message") != NULL);
    assert(strstr(buf, "child_throw_plain") != NULL);
    assert(strstr(buf, "Exited with error code") != NULL);

    /* Formatted THROW_ERROR_FMT */
    code = run_expect_exit(child_throw_fmt, buf, sizeof(buf));
    assert(code == ERR_INVALID_ARGUMENT);
    assert(strstr(buf, "formatted code: 123") != NULL);
    assert(strstr(buf, "child_throw_fmt") != NULL);

    /* Single evaluation of Code expression in THROW_ERROR */
    side_effect_calls = 0;
    code = run_expect_exit(child_throw_side_effect, buf, sizeof(buf));
    assert(code == ERR_INVALID_ARGUMENT);

    /* Single evaluation of Code expression in THROW_ERROR_FMT */
    side_effect_calls = 0;
    code = run_expect_exit(child_throw_fmt_side_effect, buf, sizeof(buf));
    assert(code == ERR_INVALID_ARGUMENT);

    /* Plain ASSERT failure */
    code = run_expect_exit(child_assert_fails, buf, sizeof(buf));
    assert(code == ERR_ASSERT_FAILED);
    assert(strstr(buf, "assert plain message") != NULL);

    /* Formatted ASSERT_FMT failure */
    code = run_expect_exit(child_assert_fmt_fails, buf, sizeof(buf));
    assert(code == ERR_ASSERT_FAILED);
    assert(strstr(buf, "assert fmt message: 42") != NULL);

    /* PRINT_STDERR */
    code = run_expect_exit(child_print_stderr, buf, sizeof(buf));
    assert(code == 0);
    assert(strstr(buf, "plain print message") != NULL);
    assert(strstr(buf, "child_print_stderr") != NULL);

    /* PRINT_STDERR_FMT */
    code = run_expect_exit(child_print_stderr_fmt, buf, sizeof(buf));
    assert(code == 0);
    assert(strstr(buf, "fmt print message: hello") != NULL);
    assert(strstr(buf, "child_print_stderr_fmt") != NULL);
}

int main(void)
{
    test_error_codes();
    test_assert_success();
    test_assert_control_flow();
    test_throwing_and_diagnostics();

    printf("All error.h tests passed!\n");
    return 0;
}
