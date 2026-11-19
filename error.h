/**
 * @file error.h
 * @brief Error codes and fatal error-reporting macros.
 *
 * Provides @ref error_code along with @ref THROW_ERROR and
 * @ref THROW_ERROR_FMT for terminating the process with file, function, line,
 * and error-code diagnostics via `errx()`.
 */

#ifndef BETTER_C_ERROR_H
#define BETTER_C_ERROR_H

#include <err.h>
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

#define PRINT_STDERR_FMT(Fmt, ...)                                             \
    fprintf(stderr, "Error message from `%s` -> %s:%d : " Fmt, __FILE_NAME__,  \
            __func__, __LINE__, __VA_ARGS__)

#define PRINT_STDERR(Msg) PRINT_STDERR_FMT("%s", Msg)

/**
 * @brief Terminate the process with a `printf`-style formatted error message.
 *
 * Prints the source file name, function name, line number, error code, and
 * formatted message to `stderr`, then exits with @p Code via `errx()`.
 *
 * @param Code Exit status / @ref error_code value. Evaluated twice; avoid
 *             expressions with side effects.
 * @param Fmt  `printf`-style format string literal.
 * @param ...  One or more arguments corresponding to format specifiers in
 *             @p Fmt.
 *
 * @note Callers must include `<err.h>` for `errx()`.
 * @note For plain messages without format arguments, use @ref THROW_ERROR.
 *
 * @code{.c}
 * THROW_ERROR_FMT(ERR_INVALID_ARGUMENT, "Invalid index: %zu", idx);
 * THROW_ERROR_FMT(res.status, "Operation failed: %s", res.err);
 * @endcode
 */
#define THROW_ERROR_FMT(Code, Fmt, ...)                                        \
    PRINT_STDERR_FMT("Exited with error code %d:\n\t" Fmt, (Code),             \
                     __VA_ARGS__);                                             \
    exit(Code)

/**
 * @brief Terminate the process with a plain error message string.
 *
 * Expands to `THROW_ERROR_FMT(Code, "%s", Msg)` so runtime strings (such as
 * `res.err`) are printed safely without format-string interpretation.
 *
 * @param Code Exit status / @ref error_code value. Evaluated twice; avoid
 *             expressions with side effects.
 * @param Msg  Null-terminated error message string (`const char *`).
 *
 * @note Callers must include `<err.h>` for `errx()`.
 *
 * @code{.c}
 * THROW_ERROR(ERR_MEMORY_ERROR, "Out of memory");
 * THROW_ERROR(res.status, res.err);
 * @endcode
 */
#define THROW_ERROR(Code, Msg) THROW_ERROR_FMT(Code, "%s", Msg)

#define ASSERT_FMT(Cond, Fmt, ...)                                             \
    if (!(Cond))                                                               \
        THROW_ERROR_FMT(ERR_ASSERT_FAILED, Fmt, __VA_ARGS__);

#define ASSERT(Cond, Msg) ASSERT_FMT(Cond, "%s", Msg)

#endif /* !BETTER_C_ERROR_H */
