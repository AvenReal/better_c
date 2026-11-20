/**
 * @file error.h
 * @brief Error codes and fatal error-reporting macros.
 *
 * Provides @ref error_code along with @ref THROW_ERROR,
 * @ref THROW_ERROR_FMT, @ref ASSERT, and @ref ASSERT_FMT for terminating
 * the process with file, function, line, and error-code diagnostics.
 */

#ifndef BETTER_C_ERROR_H
#define BETTER_C_ERROR_H

#include <err.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef __FILE_NAME__
/**
 * @brief Fallback basename macro when the compiler does not predefine
 *        `__FILE_NAME__`.
 */
#    define __FILE_NAME__ __FILE__
#endif /* !__FILE_NAME__ */

/**
 * @brief Application error codes shared by @ref THROW_ERROR and Result types.
 */
enum error_code
{
    ERR_NO_ERROR = 0, /**< Operation succeeded (no error). */
    ERR_ASSERT_FAILED, /**< Assert failure. */
    ERR_MEMORY_ERROR, /**< Memory allocation failure. */
    ERR_INVALID_ARGUMENT, /**< Invalid argument passed to a function. */
    ERR_RESULT_IS_ERR, /**< Attempted to unwrap ok of an error Result. */
    ERR_RESULT_IS_OK, /**< Attempted to unwrap error of an ok Result. */
    ERR_UNKNOWN, /**< Unspecified or fallback error code. */
};

/**
 * @brief Print a formatted diagnostic message to `stderr` with source location.
 *
 * Prints source file name (`__FILE_NAME__`), function name (`__func__`),
 * line number (`__LINE__`), and the formatted message to `stderr`.
 *
 * @param Fmt printf-style format string.
 * @param ... Format arguments matching @p Fmt.
 *
 * @code{.c}
 * PRINT_STDERR_FMT("Invalid value: %d\n", val);
 * @endcode
 */
#define PRINT_STDERR_FMT(Fmt, ...)                                             \
    fprintf(stderr, "Error message from `%s` -> %s:%d : " Fmt, __FILE_NAME__,  \
            __func__, __LINE__, __VA_ARGS__)

/**
 * @brief Print a plain diagnostic message to `stderr` with source location.
 *
 * Expands to `PRINT_STDERR_FMT("%s", Msg)` to print messages safely without
 * format string interpretation.
 *
 * @param Msg Null-terminated message string to print.
 *
 * @code{.c}
 * PRINT_STDERR("Something went wrong\n");
 * @endcode
 */
#define PRINT_STDERR(Msg) PRINT_STDERR_FMT("%s", Msg)

/**
 * @brief Terminate the process with a `printf`-style formatted error message.
 *
 * Prints the source file name, function name, line number, exit code, and
 * formatted message to `stderr`, then terminates the process with @p Code via
 * `exit()`.
 *
 * @param Code Exit status or @ref error_code value.
 * @param Fmt  `printf`-style format string literal.
 * @param ...  One or more arguments corresponding to format specifiers in
 *             @p Fmt.
 *
 * @note For plain messages without format arguments, use @ref THROW_ERROR.
 *
 * @code{.c}
 * THROW_ERROR_FMT(ERR_INVALID_ARGUMENT, "Invalid index: %zu", idx);
 * THROW_ERROR_FMT(res.status, "Operation failed: %s", GET_RESULT_ERR(res));
 * @endcode
 */
#define THROW_ERROR_FMT(Code, Fmt, ...)                                        \
    PRINT_STDERR_FMT("Exited with error code %d:\n\t" Fmt "\n", (Code),        \
                     __VA_ARGS__);                                             \
    exit((Code));

/**
 * @brief Terminate the process with a plain error message string.
 *
 * Expands to `THROW_ERROR_FMT(Code, "%s", Msg)` so runtime strings are
 * printed safely without format-string interpretation.
 *
 * @param Code Exit status or @ref error_code value.
 * @param Msg  Null-terminated error message string (`const char *`).
 *
 * @code{.c}
 * THROW_ERROR(ERR_MEMORY_ERROR, "Out of memory");
 * THROW_ERROR(res.status, GET_RESULT_ERR(res));
 * @endcode
 */
#define THROW_ERROR(Code, Msg) THROW_ERROR_FMT(Code, "%s", Msg)

/**
 * @brief Assert that a condition is true, terminating with formatted
 * diagnostics if false.
 *
 * If @p Cond evaluates to false (zero), prints diagnostics to `stderr` and
 * terminates the process with @ref ERR_ASSERT_FAILED via @ref THROW_ERROR_FMT.
 *
 * @param Cond Expression evaluating to a boolean condition.
 * @param Fmt  `printf`-style format string literal describing the assertion
 * failure.
 * @param ...  Arguments corresponding to format specifiers in @p Fmt.
 *
 * @code{.c}
 * ASSERT_FMT(ptr != NULL, "Pointer %s was NULL", "ptr");
 * @endcode
 */
#define ASSERT_FMT(Cond, Fmt, ...)                                             \
    if (!(Cond))                                                               \
    {                                                                          \
        THROW_ERROR_FMT(ERR_ASSERT_FAILED,                                     \
                        "Assertion: (" #Cond ") failed: " Fmt, __VA_ARGS__);   \
    }

/**
 * @brief Assert that a condition is true, terminating with a message string if
 * false.
 *
 * Shorthand for `ASSERT_FMT(Cond, "%s", Msg)`.
 *
 * @param Cond Expression evaluating to a boolean condition.
 * @param Msg  Null-terminated error message string describing the failure.
 *
 * @code{.c}
 * ASSERT(len > 0, "Length must be strictly positive");
 * @endcode
 */
#define ASSERT(Cond, Msg) ASSERT_FMT(Cond, "%s", Msg)

#endif /* !BETTER_C_ERROR_H */
