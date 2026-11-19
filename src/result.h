/**
 * @file result.h
 * @brief Typed Result structs, constructors, predicates, and unwrap macros.
 *
 * Provides Rust-inspired `Result` types in C without `typedef`, supporting
 * primitives, enums, structs, unions, pointers, struct pointers, and
 * pointer-to-pointers.
 */

#ifndef BETTER_C_RESULT_H
#define BETTER_C_RESULT_H

#include <stddef.h>

#include "error.h"

/**
 * @brief Expand to the Result struct type name associated with @p Name.
 *
 * Expands to `struct <Name>_result`, previously defined with
 * @ref DEFINE_RESULT or @ref DEFINE_RESULT_NAMED.
 *
 * @param Name Identifier suffix of the defined Result type.
 *
 * @code{.c}
 * static RESULT(int) divide(int a, int b);
 * static RESULT(point_ptr) alloc_point(int x, int y);
 * @endcode
 */
#define RESULT(Name) struct Name##_result

/**
 * @brief Expand to the Result payload union type name associated with @p Name.
 *
 * Expands to `union <Name>_result_union`, defined with
 * @ref DEFINE_RESULT_UNION, @ref DEFINE_RESULT, or @ref DEFINE_RESULT_NAMED.
 *
 * @param Name Identifier suffix of the defined Result type.
 *
 * @code{.c}
 * RESULT_UNION(int) u;
 * @endcode
 */
#define RESULT_UNION(Name) union Name##_result_union

/**
 * @brief Define the payload union type for a Result holding @p Type or an error message.
 *
 * Generates `union <Name>_result_union` with members:
 * - `Type ok`: Payload value on success.
 * - `const char *err`: Error message string on failure.
 *
 * @param Name Identifier suffix of the Result type.
 * @param Type C object type stored in the `.ok` member.
 *
 * @code{.c}
 * DEFINE_RESULT_UNION(int, int)
 * @endcode
 */
#define DEFINE_RESULT_UNION(Name, Type)                                        \
    RESULT_UNION(Name)                                                         \
    {                                                                          \
        Type ok;                                                               \
        const char *err;                                                       \
    };

/**
 * @brief Define a Result struct and inline constructors under an explicit name.
 *
 * Generates:
 * - `union <Name>_result_union` with `Type ok` and `const char *err` via @ref DEFINE_RESULT_UNION
 * - `struct <Name>_result` with `enum error_code status` and `RESULT_UNION(Name) value`
 * - `static inline RESULT(Name) <Name>_result_ok(Type value)`
 * - `static inline RESULT(Name) <Name>_result_err(enum error_code code, const char *error_msg)`
 *   (coercing `ERR_NO_ERROR` to `ERR_UNKNOWN`)
 *
 * Use this macro for multi-word types, enums, structs, unions, pointers,
 * struct pointers, pointer-to-pointers, and `const`-qualified types.
 *
 * @param Type C object type stored in the `.ok` member.
 * @param Name Valid C identifier used to name `struct <Name>_result` and its
 *             constructor functions.
 *
 * @note Do not place a trailing semicolon after this macro when compiling with
 *       `-Wpedantic`.
 *
 * @code{.c}
 * DEFINE_RESULT_NAMED(enum color, color)
 * DEFINE_RESULT_NAMED(struct point, point)
 * DEFINE_RESULT_NAMED(int *, int_ptr)
 * DEFINE_RESULT_NAMED(struct point *, point_ptr)
 * DEFINE_RESULT_NAMED(int **, int_ptr_ptr)
 * @endcode
 */
#define DEFINE_RESULT_NAMED(Type, Name)                                        \
    DEFINE_RESULT_UNION(Name, Type)                                            \
    RESULT(Name)                                                               \
    {                                                                          \
        enum error_code status;                                                \
        RESULT_UNION(Name) value;                                              \
    };                                                                         \
                                                                               \
    static inline RESULT(Name) Name##_result_ok(Type value)                    \
    {                                                                          \
        RESULT(Name)                                                           \
        res = { .status = ERR_NO_ERROR, .value = { .ok = value } };            \
        return res;                                                            \
    }                                                                          \
                                                                               \
    static inline RESULT(Name)                                                 \
        Name##_result_err(enum error_code code, const char *error_msg)         \
    {                                                                          \
        RESULT(Name)                                                           \
        res = { .status = (code == ERR_NO_ERROR) ? ERR_UNKNOWN : code,         \
                .value = { .err = error_msg } };                               \
        return res;                                                            \
    }

/**
 * @brief Define a Result struct and inline constructors for a single-word type.
 *
 * Shorthand for `DEFINE_RESULT_NAMED(Type, Type)`.
 *
 * @param Type Single-identifier C type (e.g. `int`, `double`, `size_t`).
 *
 * @note Do not place a trailing semicolon after this macro when compiling with
 *       `-Wpedantic`.
 *
 * @code{.c}
 * DEFINE_RESULT(int)
 * DEFINE_RESULT(double)
 * @endcode
 */
#define DEFINE_RESULT(Type) DEFINE_RESULT_NAMED(Type, Type)

/**
 * @brief Construct a successful `RESULT(Name)` holding @p Value.
 *
 * @param Name  Result type name passed to @ref DEFINE_RESULT or
 *              @ref DEFINE_RESULT_NAMED.
 * @param Value Success value to store in `.value.ok`. For compound literals
 *              containing commas, use a local variable or wrap the literal in
 *              extra parentheses, e.g. `((struct point){ .x = 1, .y = 2 })`.
 * @return A `RESULT(Name)` value with `.status == ERR_NO_ERROR`.
 *
 * @code{.c}
 * return SET_RESULT_OK(int, a / b);
 * @endcode
 */
#define SET_RESULT_OK(Name, Value) Name##_result_ok(Value)

/**
 * @brief Construct an error `RESULT(Name)` with an error code and message.
 *
 * If @p Code is `ERR_NO_ERROR`, `.status` is automatically coerced to
 * `ERR_UNKNOWN`.
 *
 * @param Name    Result type name passed to @ref DEFINE_RESULT or
 *                @ref DEFINE_RESULT_NAMED.
 * @param Code    Error code (@ref error_code).
 * @param Message Error description string (`const char *`, may be `NULL`).
 * @return A `RESULT(Name)` value with `.status != ERR_NO_ERROR`.
 *
 * @code{.c}
 * return SET_RESULT_ERR(int, ERR_INVALID_ARGUMENT, "Division by zero");
 * @endcode
 */
#define SET_RESULT_ERR(Name, Code, Message) Name##_result_err((Code), (Message))

/**
 * @brief Check whether a Result value represents success.
 *
 * Variadic so compound literals containing commas can be passed directly.
 *
 * @param ... Expression evaluating to a `RESULT(...)` value.
 * @return Non-zero (`1`) if `.status == ERR_NO_ERROR`, `0` otherwise.
 *
 * @code{.c}
 * if (RESULT_IS_OK(res)) { ... }
 * @endcode
 */
#define RESULT_IS_OK(...) ((__VA_ARGS__).status == ERR_NO_ERROR)

/**
 * @brief Check whether a Result value represents an error.
 *
 * Variadic so compound literals containing commas can be passed directly.
 *
 * @param ... Expression evaluating to a `RESULT(...)` value.
 * @return Non-zero (`1`) if `.status != ERR_NO_ERROR`, `0` otherwise.
 *
 * @code{.c}
 * if (RESULT_IS_ERR(res)) { ... }
 * @endcode
 */
#define RESULT_IS_ERR(...) ((__VA_ARGS__).status != ERR_NO_ERROR)

/**
 * @brief Unwrap a named Result's error message into @p Var.
 *
 * Statement macro that evaluates @p Expr into `RESULT(Name) <Var>_result`.
 * If `<Var>_result` is an OK Result, terminates the process via @ref THROW_ERROR
 * with `ERR_RESULT_IS_OK`. Otherwise, declares `char *Var = <Var>_result.value.err;`.
 *
 * @param Type C object type of the Result's `.ok` payload.
 * @param Var  Name of the variable to declare for the unwrapped error message.
 * @param Expr Expression yielding a `RESULT(Name)`.
 * @param Name Result type name suffix passed to @ref DEFINE_RESULT_NAMED.
 *
 * @warning Expands to multiple statements; enclose in `{ ... }` inside
 *          unbraced `if`/`else`/`for`/`while` bodies, and omit a trailing `;`.
 *
 * @code{.c}
 * GET_RESULT_ERR_NAMED(struct point, err_msg, make_point_err(), point)
 * printf("Error: %s\n", err_msg);
 * @endcode
 */
#define GET_RESULT_ERR_NAMED(Type, Var, Expr, Name)                            \
    RESULT(Name) Var##_result = (Expr);                                        \
    if (RESULT_IS_OK(Var##_result))                                            \
    {                                                                          \
        THROW_ERROR(ERR_RESULT_IS_OK,                                          \
                    "Tried to get err of " #Expr " but it's an ok.");          \
    }                                                                          \
    const char *Var = Var##_result.value.err;

/** @cond INTERNAL */
#define _GET_RESULT_ERR_1(Res) ((Res).value.err)
#define _GET_RESULT_ERR_3(Type, Var, Expr)                                     \
    GET_RESULT_ERR_NAMED(Type, Var, Expr, Type)
#define _GET_RESULT_ERR_4(Type, Var, Expr, Name)                               \
    GET_RESULT_ERR_NAMED(Type, Var, Expr, Name)
#define _GET_RESULT_ERR_CHOOSER(_1, _2, _3, _4, NAME, ...) NAME
/** @endcond */

/**
 * @brief Retrieve or unwrap the error message from a Result.
 *
 * When passed a single argument `GET_RESULT_ERR(Res)`, retrieves the `const char *`
 * error string from the result variable without accessing the union member directly.
 * When passed three arguments `GET_RESULT_ERR(Type, Var, Expr)` or four arguments
 * `GET_RESULT_ERR(Type, Var, Expr, Name)`, evaluates `Expr` and unwraps the error
 * message into `const char *Var`, throwing `ERR_RESULT_IS_OK` if `Expr` was successful.
 *
 * @param ... Variable arguments: either `(Res)` for 1-arg inspection, or `(Type, Var, Expr)`
 *            / `(Type, Var, Expr, Name)` for statement unwrapping.
 */
#define GET_RESULT_ERR(...)                                                    \
    _GET_RESULT_ERR_CHOOSER(__VA_ARGS__, _GET_RESULT_ERR_4,                    \
                            _GET_RESULT_ERR_3, _unused,                        \
                            _GET_RESULT_ERR_1, 0)(__VA_ARGS__)

/**
 * @brief Unwrap a named Result into @p Var, running a custom handler on error.
 *
 * Statement macro that evaluates @p Expr once into `RESULT(Name) <Var>_result`,
 * executes `...` if `<Var>_result` is an error, asserts that `<Var>_result` is
 * no longer an error (terminating via @ref THROW_ERROR with
 * `ERR_RESULT_IS_ERR` otherwise), and declares `Type Var = <Var>_result.value.ok;`.
 *
 * Inside the error handler, `<Var>_result.status` and `<Var>_result.value.err` are
 * available. The handler must either transfer control (`return`, `break`,
 * `continue`, `goto`, @ref THROW_ERROR, @ref THROW_ERROR_FMT) or recover by
 * setting `<Var>_result.status = ERR_NO_ERROR` and `<Var>_result.value.ok = ...`.
 *
 * @param Type C type of the unwrapped variable @p Var.
 * @param Var  Name of the variable to declare for the unwrapped value.
 * @param Expr Expression returning `RESULT(Name)`.
 * @param Name Result type name passed to @ref DEFINE_RESULT_NAMED.
 * @param ...  Error-handling statement or block executed when @p Expr fails.
 *
 * @warning Expands to multiple statements; enclose in `{ ... }` inside
 *          unbraced `if`/`else`/`for`/`while` bodies, and omit a trailing `;`.
 *
 * @code{.c}
 * GET_RESULT_OK_NAMED_OR_ELSE(struct point *, p, alloc_point(1, 2), point_ptr,
 * {
 *     fprintf(stderr, "Error (%d): %s\n", (int)p_result.status, p_result.value.err);
 *     return 1;
 * })
 * @endcode
 */
#define GET_RESULT_OK_NAMED_OR_ELSE(Type, Var, Expr, Name, ...)                \
    RESULT(Name) Var##_result = (Expr);                                        \
    if (RESULT_IS_ERR(Var##_result))                                           \
    {                                                                          \
        __VA_ARGS__                                                            \
    }                                                                          \
    if (RESULT_IS_ERR(Var##_result))                                           \
    {                                                                          \
        THROW_ERROR(ERR_RESULT_IS_ERR,                                         \
                    "" #Var                                                    \
                    "_result is still an error type but should not be.");      \
    }                                                                          \
    Type Var = Var##_result.value.ok;

/**
 * @brief Unwrap a single-word Result into @p Var, running a custom handler on
 *        error.
 *
 * Shorthand for `GET_RESULT_OK_NAMED_OR_ELSE(Type, Var, Expr, Type, ...)`.
 *
 * @param Type Single-identifier C type of @p Var and the Result name.
 * @param Var  Name of the variable to declare for the unwrapped value.
 * @param Expr Expression returning `RESULT(Type)`.
 * @param ...  Error-handling statement or block executed when @p Expr fails.
 *
 * @warning Expands to multiple statements; enclose in `{ ... }` inside
 *          unbraced `if`/`else`/`for`/`while` bodies, and omit a trailing `;`.
 *
 * @code{.c}
 * GET_RESULT_OK_OR_ELSE(int, q, divide(10, 0), {
 *     THROW_ERROR_FMT(q_result.status, "divide failed: %s", q_result.value.err);
 * })
 * @endcode
 */
#define GET_RESULT_OK_OR_ELSE(Type, Var, Expr, ...)                            \
    GET_RESULT_OK_NAMED_OR_ELSE(Type, Var, Expr, Type, __VA_ARGS__)

/**
 * @brief Unwrap a named Result into @p Var, or use a fallback expression on
 *        error.
 *
 * Statement macro that evaluates @p Expr once into `RESULT(Name) <Var>_result`
 * and initializes `Type Var` with `<Var>_result.value.ok` on success or the lazily
 * evaluated fallback expression `...` on error.
 *
 * @param Type C type of the unwrapped variable @p Var.
 * @param Var  Name of the variable to declare for the unwrapped value.
 * @param Expr Expression returning `RESULT(Name)`.
 * @param Name Result type name passed to @ref DEFINE_RESULT_NAMED.
 * @param ...  Fallback expression evaluated only if @p Expr returns an error.
 *
 * @warning Expands to multiple statements; enclose in `{ ... }` inside
 *          unbraced `if`/`else`/`for`/`while` bodies, and omit a trailing `;`.
 *
 * @code{.c}
 * GET_RESULT_OK_NAMED_OR(struct point, pt, make_point_err(), point,
 *                     (struct point){ .x = 0, .y = 0 })
 * @endcode
 */
#define GET_RESULT_OK_NAMED_OR(Type, Var, Expr, Name, ...)                     \
    RESULT(Name) Var##_result = (Expr);                                        \
    Type Var = RESULT_IS_OK(Var##_result) ? Var##_result.value.ok             \
                                          : (__VA_ARGS__);
/**
 * @brief Unwrap a single-word Result into @p Var, or use a fallback expression
 *        on error.
 *
 * Shorthand for `GET_RESULT_OK_NAMED_OR(Type, Var, Expr, Type, ...)`.
 *
 * @param Type Single-identifier C type of @p Var and the Result name.
 * @param Var  Name of the variable to declare for the unwrapped value.
 * @param Expr Expression returning `RESULT(Type)`.
 * @param ...  Fallback expression evaluated only if @p Expr returns an error.
 *
 * @warning Expands to multiple statements; enclose in `{ ... }` inside
 *          unbraced `if`/`else`/`for`/`while` bodies, and omit a trailing `;`.
 *
 * @code{.c}
 * GET_RESULT_OK_OR(int, safe_div, divide(10, 0), -1)
 * @endcode
 */
#define GET_RESULT_OK_OR(Type, Var, Expr, ...)                                 \
    GET_RESULT_OK_NAMED_OR(Type, Var, Expr, Type, __VA_ARGS__)

/**
 * @brief Unwrap a named Result into @p Var, terminating the process on error.
 *
 * Calls @ref THROW_ERROR with `ERR_RESULT_IS_ERR` if @p Expr returns an error.
 *
 * @param Type C type of the unwrapped variable @p Var.
 * @param Var  Name of the variable to declare for the unwrapped value.
 * @param Expr Expression returning `RESULT(Name)`.
 * @param Name Result type name passed to @ref DEFINE_RESULT_NAMED.
 *
 * @warning Expands to multiple statements; enclose in `{ ... }` inside
 *          unbraced `if`/`else`/`for`/`while` bodies, and omit a trailing `;`.
 *
 * @code{.c}
 * GET_RESULT_OK_NAMED(struct point, pt, make_point(3, 4), point)
 * @endcode
 */
#define GET_RESULT_OK_NAMED(Type, Var, Expr, Name)                             \
    GET_RESULT_OK_NAMED_OR_ELSE(                                               \
        Type, Var, Expr, Name,                                                 \
        THROW_ERROR(ERR_RESULT_IS_ERR,                                         \
                    "Tried to get result value but " #Expr                     \
                    " returned an error.");)

/**
 * @brief Unwrap a single-word Result into @p Var, terminating the process on
 *        error.
 *
 * Shorthand for `GET_RESULT_OK_NAMED(Type, Var, Expr, Type)`.
 *
 * @param Type Single-identifier C type of @p Var and the Result name.
 * @param Var  Name of the variable to declare for the unwrapped value.
 * @param Expr Expression returning `RESULT(Type)`.
 *
 * @warning Expands to multiple statements; enclose in `{ ... }` inside
 *          unbraced `if`/`else`/`for`/`while` bodies, and omit a trailing `;`.
 *
 * @code{.c}
 * GET_RESULT_OK(int, quotient, divide(10, 2))
 * @endcode
 */
#define GET_RESULT_OK(Type, Var, Expr)                                         \
    GET_RESULT_OK_NAMED(Type, Var, Expr, Type)

#endif /* !BETTER_C_RESULT_H */
