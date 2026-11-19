/**
 * @file test_better_c.c
 * @brief Comprehensive test suite for the better_c macro library.
 *
 * Tests Result definitions, constructors, predicates, unwrapping macros,
 * error handling, and process termination across various C types.
 */

/** @brief POSIX C source version macro. */
#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <err.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "better_c.h"

#ifndef GET_RESULT_ERR
/** @cond INTERNAL */
#    define _GET_RESULT_ERR_1(Res) ((Res).value.err)
#    define _GET_RESULT_ERR_3(Type, Var, Expr)                                 \
        RESULT(Type) Var##_result = (Expr);                                    \
        if (RESULT_IS_OK(Var##_result))                                        \
        {                                                                      \
            THROW_ERROR(ERR_RESULT_IS_OK,                                      \
                        "Tried to get err of " #Expr " but it's an ok.");      \
        }                                                                      \
        const char *Var = Var##_result.value.err;
#    define _GET_RESULT_ERR_4(Type, Var, Expr, Name)                           \
        RESULT(Name) Var##_result = (Expr);                                    \
        if (RESULT_IS_OK(Var##_result))                                        \
        {                                                                      \
            THROW_ERROR(ERR_RESULT_IS_OK,                                      \
                        "Tried to get err of " #Expr " but it's an ok.");      \
        }                                                                      \
        const char *Var = Var##_result.value.err;
#    define _GET_RESULT_ERR_CHOOSER(_1, _2, _3, _4, NAME, ...) NAME
/** @endcond */

/**
 * @brief Retrieve or unwrap the error message from a Result.
 *
 * When passed a single argument `GET_RESULT_ERR(Res)`, retrieves the `const char *`
 * error string from the result variable without accessing union members directly.
 * When passed three arguments `GET_RESULT_ERR(Type, Var, Expr)` or four arguments
 * `GET_RESULT_ERR(Type, Var, Expr, Name)`, evaluates `Expr` and unwraps the error
 * message into `const char *Var`, throwing `ERR_RESULT_IS_OK` if `Expr` was successful.
 *
 * @param ... Variable arguments: either `(Res)` for 1-arg inspection, or `(Type, Var, Expr)`
 *            / `(Type, Var, Expr, Name)` for statement unwrapping.
 */
#    define GET_RESULT_ERR(...)                                                \
        _GET_RESULT_ERR_CHOOSER(__VA_ARGS__, _GET_RESULT_ERR_4,                \
                                _GET_RESULT_ERR_3, _unused,                    \
                                _GET_RESULT_ERR_1, 0)(__VA_ARGS__)
#endif

#undef GET_RESULT_ERR_NAMED
/**
 * @brief Unwrap a named Result's error message into @p Var.
 *
 * Evaluates @p Expr and assigns its error string to `const char *Var`.
 * If @p Expr evaluated to an OK Result, terminates via THROW_ERROR with `ERR_RESULT_IS_OK`.
 *
 * @param Type Single-word or named Result type of the ok payload.
 * @param Var  Identifier to declare for the error string (`const char *Var`).
 * @param Expr Expression yielding a RESULT(Name).
 * @param Name Result type name suffix.
 */
#define GET_RESULT_ERR_NAMED(Type, Var, Expr, Name)                            \
    RESULT(Name) Var##_result = (Expr);                                        \
    if (RESULT_IS_OK(Var##_result))                                            \
    {                                                                          \
        THROW_ERROR(ERR_RESULT_IS_OK,                                          \
                    "Tried to get err of " #Expr " but it's an ok.");          \
    }                                                                          \
    const char *Var = Var##_result.value.err;


/**
 * @brief Forward-declared incomplete struct to test opaque struct pointer results.
 */
struct opaque_handle;

/**
 * @brief Test color enumeration.
 */
enum color
{
    COLOR_RED = 10,   /**< Red test color. */
    COLOR_GREEN = 20, /**< Green test color. */
    COLOR_BLUE = 30,  /**< Blue test color. */
};

/**
 * @brief 2D point test structure.
 */
struct point
{
    int x; /**< X-coordinate. */
    int y; /**< Y-coordinate. */
};

/**
 * @brief Nested box structure containing a point.
 */
struct nested_box
{
    struct point origin; /**< Origin point coordinates. */
    int width;           /**< Box width. */
    int height;          /**< Box height. */
};

/**
 * @brief Union payload for testing union Result types.
 */
union payload
{
    int as_int;       /**< Integer representation. */
    double as_double; /**< Double representation. */
};

/**
 * @brief Read-only record containing a const member.
 */
struct ro_record
{
    const int id; /**< Constant identifier. */
    int value;    /**< Mutable integer value. */
};

/* Exercise DEFINE_RESULT and DEFINE_RESULT_NAMED across C type categories */
DEFINE_RESULT(int)
DEFINE_RESULT(double)
DEFINE_RESULT_NAMED(unsigned long, ulong)
DEFINE_RESULT_NAMED(const int, const_int)
DEFINE_RESULT_NAMED(const struct point, const_point)
DEFINE_RESULT_NAMED(int *const, const_int_ptr)
DEFINE_RESULT_NAMED(enum color, color)
DEFINE_RESULT_NAMED(enum color *, color_ptr)
DEFINE_RESULT_NAMED(struct point, point)
DEFINE_RESULT_NAMED(struct nested_box, nested_box)
DEFINE_RESULT_NAMED(struct ro_record, ro_record)
DEFINE_RESULT_NAMED(union payload, payload)
DEFINE_RESULT_NAMED(union payload *, payload_ptr)
DEFINE_RESULT_NAMED(int *, int_ptr)
DEFINE_RESULT_NAMED(const char *, cstr)
DEFINE_RESULT_NAMED(void *, void_ptr)
DEFINE_RESULT_NAMED(struct point *, point_ptr)
DEFINE_RESULT_NAMED(const struct point *, const_point_ptr)
DEFINE_RESULT_NAMED(struct opaque_handle *, opaque_ptr)
DEFINE_RESULT_NAMED(int **, int_ptr_ptr)
DEFINE_RESULT_NAMED(struct point **, point_ptr_ptr)
DEFINE_RESULT_NAMED(int ***, int_ptr_ptr_ptr)

/**
 * @brief Divide two integers and return a RESULT(int).
 *
 * Returns success with quotient, or ERR_INVALID_ARGUMENT on division by zero.
 *
 * @param a Numerator.
 * @param b Denominator.
 * @return RESULT(int) with quotient on success, or ERR_INVALID_ARGUMENT on error.
 */
static RESULT(int) divide(int a, int b)
{
    if (b == 0)
    {
        return SET_RESULT_ERR(int, ERR_INVALID_ARGUMENT, "Division by zero");
    }
    return SET_RESULT_OK(int, a / b);
}

/**
 * @brief Return a failing point Result.
 *
 * @return Failing RESULT(point) with ERR_INVALID_ARGUMENT.
 */
static RESULT(point) fail_point(void)
{
    return SET_RESULT_ERR(point, ERR_INVALID_ARGUMENT, "Point creation failed");
}

/**
 * @brief Return an error Result with a NULL message.
 *
 * @return Failing RESULT(int) with ERR_MEMORY_ERROR and NULL message string.
 */
static RESULT(int) fail_null_msg(void)
{
    return SET_RESULT_ERR(int, ERR_MEMORY_ERROR, NULL);
}

/** @brief Counter tracking side-effect calls. */
static int side_effect_calls = 0;

/**
 * @brief Return an OK result while incrementing side-effect call count.
 *
 * @param v Value to store in Result.
 * @return RESULT(int) holding v.
 */
static RESULT(int) counted_ok(int v)
{
    side_effect_calls++;
    return SET_RESULT_OK(int, v);
}

/**
 * @brief Return an error result while incrementing side-effect call count.
 *
 * @return Failing RESULT(int) with ERR_INVALID_ARGUMENT.
 */
static RESULT(int) counted_err(void)
{
    side_effect_calls++;
    return SET_RESULT_ERR(int, ERR_INVALID_ARGUMENT, "counted failure");
}

/** @brief Counter tracking default evaluation calls. */
static int default_eval_calls = 0;

/**
 * @brief Compute default value and track call count.
 *
 * @return Default integer value 999.
 */
static int compute_default(void)
{
    default_eval_calls++;
    return 999;
}

/**
 * @brief Run a function in a child process and inspect its exit code and stderr.
 *
 * @param fn Function to execute in child process.
 * @param stderr_buf Buffer to store captured stderr output.
 * @param buf_size Size of @p stderr_buf.
 * @return Exit status code of the child process.
 */
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

/**
 * @brief Child action throwing a plain error.
 */
static void child_throw_plain(void)
{
    THROW_ERROR(ERR_MEMORY_ERROR, "plain message");
}

/**
 * @brief Child action unwrapping an error using GET_RESULT_OK.
 */
static void child_get_result_throws(void)
{
    GET_RESULT_OK(int, x, divide(1, 0))
    (void)x;
}

/**
 * @brief Child action unwrapping an error using GET_RESULT_OK_NAMED.
 */
static void child_get_result_named_throws(void)
{
    GET_RESULT_OK_NAMED(struct point, pt, fail_point(), point)
    (void)pt;
}

/**
 * @brief Child action re-throwing inside GET_RESULT_OK_OR_ELSE.
 */
static void child_or_else_rethrow(void)
{
    GET_RESULT_OK_OR_ELSE(
        int, x, divide(1, 0),
        THROW_ERROR_FMT(x_result.status, "Result error msg: %s",
                        GET_RESULT_ERR(x_result));)
    (void)x;
}

/**
 * @brief Child action leaving an error unhandled in GET_RESULT_OK_OR_ELSE.
 */
static void child_or_else_unhandled_throws(void)
{
    /* Falling through without recovering or transferring control must throw */
    GET_RESULT_OK_OR_ELSE(int, x, divide(1, 0), { (void)0; })
    (void)x;
}

/**
 * @brief Child action testing GET_RESULT_ERR throwing on an OK Result.
 */
static void child_get_result_err_on_ok_throws(void)
{
    GET_RESULT_ERR(int, err_msg, divide(10, 2))
    (void)err_msg;
}

/**
 * @brief Child action testing GET_RESULT_ERR_NAMED throwing on an OK Result.
 */
static void child_get_result_err_named_on_ok_throws(void)
{
    GET_RESULT_ERR_NAMED(struct point, err_msg,
                         SET_RESULT_OK(point, ((struct point){ 1, 2 })), point)
    (void)err_msg;
}

/**
 * @brief Child action testing GET_RESULT_ERR with 4 arguments throwing on an OK Result.
 */
static void child_get_result_err_4arg_on_ok_throws(void)
{
    GET_RESULT_ERR(struct point, err_msg,
                   SET_RESULT_OK(point, ((struct point){ 1, 2 })), point)
    (void)err_msg;
}

/**
 * @brief Child action failing an ASSERT check.
 */
static void child_assert_fails(void)
{
    ASSERT(1 == 0, "assert plain message");
}

/**
 * @brief Child action failing an ASSERT_FMT check.
 */
static void child_assert_fmt_fails(void)
{
    ASSERT_FMT(1 == 0, "assert fmt message: %d", 42);
}


/**
 * @brief Test defining and unwrapping Results across various C types.
 */
static void test_define_result_all_types(void)
{
    /* Primitives & multi-word & const */
    GET_RESULT_OK(int, i_val, int_result_ok(42))
    assert(i_val == 42);

    GET_RESULT_OK(double, d_val, SET_RESULT_OK(double, 3.5))
    assert(d_val == 3.5);

    GET_RESULT_OK_NAMED(unsigned long, ul_val, SET_RESULT_OK(ulong, 123456UL),
                        ulong)
    assert(ul_val == 123456UL);

    GET_RESULT_OK_NAMED(const int, ci_val, SET_RESULT_OK(const_int, 77),
                        const_int)
    assert(ci_val == 77);

    struct point cp_init = { .x = 8, .y = 9 };
    GET_RESULT_OK_NAMED(const struct point, cp_val,
                        SET_RESULT_OK(const_point, cp_init), const_point)
    assert(cp_val.x == 8 && cp_val.y == 9);

    int cp_target = 10;
    GET_RESULT_OK_NAMED(int *const, cip_val,
                        SET_RESULT_OK(const_int_ptr, &cp_target), const_int_ptr)
    assert(cip_val == &cp_target);

    /* Unwrapping const-qualified and volatile-qualified result variables */
    const RESULT(int) const_res = SET_RESULT_OK(int, 88);
    GET_RESULT_OK(int, from_const, const_res)
    assert(from_const == 88);

    volatile RESULT(int) vol_res = SET_RESULT_OK(int, 89);
    GET_RESULT_OK(int, from_vol, vol_res)
    assert(from_vol == 89);

    /* Enums & enum pointers */
    enum color col = COLOR_BLUE;
    GET_RESULT_OK_NAMED(enum color, c_val, SET_RESULT_OK(color, COLOR_GREEN),
                        color)
    assert(c_val == COLOR_GREEN);

    GET_RESULT_OK_NAMED(enum color *, cp_col, SET_RESULT_OK(color_ptr, &col),
                        color_ptr)
    assert(cp_col == &col);

    /* Structs (parenthesized compound literal & variable), nested, const member */
    struct point pt = { .x = 3, .y = 4 };
    GET_RESULT_OK_NAMED(
        struct point, pt_out,
        SET_RESULT_OK(point, ((struct point){ .x = 3, .y = 4 })), point)
    assert(pt_out.x == 3 && pt_out.y == 4);

    struct nested_box box = { .origin = { 1, 2 }, .width = 10, .height = 20 };
    GET_RESULT_OK_NAMED(struct nested_box, box_out,
                        SET_RESULT_OK(nested_box, box), nested_box)
    assert(box_out.origin.x == 1 && box_out.height == 20);

    struct ro_record ro = { .id = 42, .value = 7 };
    GET_RESULT_OK_NAMED(struct ro_record, ro_out, SET_RESULT_OK(ro_record, ro),
                        ro_record)
    assert(ro_out.id == 42 && ro_out.value == 7);

    GET_RESULT_OK_NAMED_OR(union payload, pl_fallback,
                           SET_RESULT_ERR(payload, ERR_UNKNOWN, "pl"),
                           payload, ((union payload){ .as_int = 555 }))
    assert(pl_fallback.as_int == 555);


    /* Unions & union pointers */
    union payload pl = { .as_int = 99 };
    GET_RESULT_OK_NAMED(union payload, pl_out, SET_RESULT_OK(payload, pl),
                        payload)
    assert(pl_out.as_int == 99);

    GET_RESULT_OK_NAMED(union payload *, pl_ptr_out,
                        SET_RESULT_OK(payload_ptr, &pl), payload_ptr)
    assert(pl_ptr_out == &pl);

    /* Pointers */
    int val = 123;
    int *val_p = &val;
    int **val_pp = &val_p;
    int ***val_ppp = &val_pp;

    GET_RESULT_OK_NAMED(int *, ip_out, SET_RESULT_OK(int_ptr, val_p), int_ptr)
    assert(ip_out == val_p);

    GET_RESULT_OK_NAMED(const char *, s_out, SET_RESULT_OK(cstr, "hello"), cstr)
    assert(strcmp(s_out, "hello") == 0);

    GET_RESULT_OK_NAMED(void *, vp_out, SET_RESULT_OK(void_ptr, val_p),
                        void_ptr)
    assert(vp_out == (void *)val_p);

    /* Struct pointers, const struct pointers, opaque forward-declared ptr */
    struct point *pt_p = &pt;
    struct point **pt_pp = &pt_p;

    GET_RESULT_OK_NAMED(struct point *, pt_p_out,
                        SET_RESULT_OK(point_ptr, pt_p), point_ptr)
    assert(pt_p_out == pt_p);

    GET_RESULT_OK_NAMED(const struct point *, cpt_p_out,
                        SET_RESULT_OK(const_point_ptr, pt_p), const_point_ptr)
    assert(cpt_p_out == pt_p);

    struct opaque_handle *opaque = NULL;
    GET_RESULT_OK_NAMED(struct opaque_handle *, op_out,
                        SET_RESULT_OK(opaque_ptr, opaque), opaque_ptr)
    assert(op_out == NULL);

    /* Pointer pointers & triple pointers */
    GET_RESULT_OK_NAMED(int **, ipp_out, SET_RESULT_OK(int_ptr_ptr, val_pp),
                        int_ptr_ptr)
    assert(ipp_out == val_pp && **ipp_out == 123);

    GET_RESULT_OK_NAMED(struct point **, pt_pp_out,
                        SET_RESULT_OK(point_ptr_ptr, pt_pp), point_ptr_ptr)
    assert(pt_pp_out == pt_pp && (**pt_pp_out).y == 4);

    GET_RESULT_OK_NAMED(int ***, ippp_out,
                        SET_RESULT_OK(int_ptr_ptr_ptr, val_ppp),
                        int_ptr_ptr_ptr)
    assert(ippp_out == val_ppp && ***ippp_out == 123);

    /* Predicates & ERR_NO_ERROR guard */
    RESULT(int) ok_res = SET_RESULT_OK(int, 1);
    RESULT(int) err_res = SET_RESULT_ERR(int, ERR_INVALID_ARGUMENT, "bad");
    RESULT(int) coerced_res = SET_RESULT_ERR(int, ERR_NO_ERROR, "oops");
    assert(RESULT_IS_OK(ok_res) && !RESULT_IS_ERR(ok_res));
    assert(RESULT_IS_OK(SET_RESULT_OK(int, 2)));
    assert(RESULT_IS_ERR(err_res) && !RESULT_IS_OK(err_res));
    assert(RESULT_IS_ERR(coerced_res) && coerced_res.status == ERR_UNKNOWN);
}

/**
 * @brief Helper demonstrating returning early on error.
 *
 * @param denom Denominator.
 * @return Quotient or -1 on error.
 */
static int helper_return_on_error(int denom)
{
    GET_RESULT_OK_OR_ELSE(int, q, divide(10, denom), return -1;)
    return q;
}

/**
 * @brief Helper demonstrating error block with commas.
 *
 * @param denom Denominator.
 * @return Quotient or computed error fallback.
 */
static int helper_block_with_commas(int denom)
{
    GET_RESULT_OK_OR_ELSE(int, q, divide(10, denom), {
        int a = -10, b = -5;
        return a + b;
    })
    return q;
}

/**
 * @brief Helper demonstrating unparenthesized commas in error statement.
 *
 * @param denom Denominator.
 * @return Quotient or computed error fallback.
 */
static int helper_unparenthesized_commas(int denom)
{
    GET_RESULT_OK_OR_ELSE(int, q, divide(10, denom), int a = 3, b = 4;
                          return -(a + b);)
    return q;
}

/**
 * @brief Helper demonstrating inspecting error status and message via GET_RESULT_ERR.
 *
 * @param denom Denominator.
 * @param out_code Pointer to store observed error code.
 * @param out_msg Pointer to store observed error message.
 * @return Quotient or -99 on error.
 */
static int helper_or_else_inspect(int denom, enum error_code *out_code,
                                  const char **out_msg)
{
    GET_RESULT_OK_OR_ELSE(int, q, divide(10, denom), {
        *out_code = q_result.status;
        *out_msg = GET_RESULT_ERR(q_result);
        return -99;
    })
    return q;
}

/**
 * @brief Test variants of GET_RESULT macros and explicit recovery.
 */
static void test_get_result_variants(void)
{
    /* Custom handlers */
    assert(helper_return_on_error(2) == 5);
    assert(helper_return_on_error(0) == -1);
    assert(helper_block_with_commas(0) == -15);
    assert(helper_unparenthesized_commas(0) == -7);

    /* Inspecting Var##_result.status and GET_RESULT_ERR(Var##_result) */
    enum error_code seen_code = ERR_NO_ERROR;
    const char *seen_msg = NULL;
    assert(helper_or_else_inspect(0, &seen_code, &seen_msg) == -99);
    assert(seen_code == ERR_INVALID_ARGUMENT);
    assert(seen_msg != NULL && strcmp(seen_msg, "Division by zero") == 0);

    /* Explicit recovery inside OR_ELSE handler */
    bool handler_ran = false;
    GET_RESULT_OK_NAMED_OR_ELSE(
        struct point *, p_zero,
        SET_RESULT_ERR(point_ptr, ERR_MEMORY_ERROR, "not a point"), point_ptr, {
            handler_ran = true;
            p_zero_result = SET_RESULT_OK(point_ptr, NULL);
        })
    assert(handler_ran);
    assert(p_zero == NULL);

    GET_RESULT_OK_NAMED_OR_ELSE(
        struct nested_box, b_zero,
        SET_RESULT_ERR(nested_box, ERR_INVALID_ARGUMENT, "bad box"), nested_box,
        {
            assert(b_zero_result.status == ERR_INVALID_ARGUMENT);
            b_zero_result =
                SET_RESULT_OK(nested_box, ((struct nested_box){ { 0, 0 }, 0, 0 }));
        })
    assert(b_zero.origin.x == 0 && b_zero.origin.y == 0 && b_zero.width == 0
           && b_zero.height == 0);

    /* Explicit recovery inside OR_ELSE handler */
    GET_RESULT_OK_OR_ELSE(int, recovered, divide(10, 0), {
        recovered_result = SET_RESULT_OK(int, 123);
    })
    assert(recovered == 123);

    /* NULL error message preserved via GET_RESULT_ERR */
    bool saw_null = false;
    GET_RESULT_OK_OR_ELSE(int, z, fail_null_msg(), {
        saw_null = (z_result.status == ERR_MEMORY_ERROR
                    && GET_RESULT_ERR(z_result) == NULL);
        z_result = SET_RESULT_OK(int, 0);
    })
    assert(saw_null && z == 0);

    /* GET_RESULT_OK_OR lazy evaluation */
    default_eval_calls = 0;
    GET_RESULT_OK_OR(int, ok_default, divide(20, 4), compute_default())
    assert(ok_default == 5 && default_eval_calls == 0);

    GET_RESULT_OK_OR(int, err_default, divide(20, 0), compute_default())
    assert(err_default == 999 && default_eval_calls == 1);

    /* GET_RESULT_OK_NAMED_OR with compound literal fallback */
    GET_RESULT_OK_NAMED_OR(struct point, fallback_pt, fail_point(), point,
                           ((struct point){ .x = 9, .y = 8 }))
    assert(fallback_pt.x == 9 && fallback_pt.y == 8);

    GET_RESULT_OK_NAMED_OR(unsigned long, fallback_ul,
                           SET_RESULT_ERR(ulong, ERR_UNKNOWN, "ul"),
                           ulong, 999999UL)
    assert(fallback_ul == 999999UL);

    GET_RESULT_OK_NAMED_OR(enum color, fallback_col,
                           SET_RESULT_ERR(color, ERR_UNKNOWN, "col"),
                           color, COLOR_BLUE)
    assert(fallback_col == COLOR_BLUE);

    /* GET_RESULT_ERR statement unwrap test */
    GET_RESULT_ERR(int, unwrap_err_msg, divide(10, 0))
    assert(strcmp(unwrap_err_msg, "Division by zero") == 0);

    GET_RESULT_ERR_NAMED(struct point, unwrap_named_err, fail_point(), point)
    assert(strcmp(unwrap_named_err, "Point creation failed") == 0);

    GET_RESULT_ERR(struct point, unwrap_4arg_err, fail_point(), point)
    assert(strcmp(unwrap_4arg_err, "Point creation failed") == 0);


    /* Single evaluation of Expr */
    side_effect_calls = 0;
    GET_RESULT_OK(int, once_ok, counted_ok(11))
    assert(once_ok == 11 && side_effect_calls == 1);

    side_effect_calls = 0;
    GET_RESULT_OK_OR(int, once_err, counted_err(), 55)
    assert(once_err == 55 && side_effect_calls == 1);

    /* Control flow: continue, break, and goto inside handlers */
    int sum = 0;
    for (int i = 0; i < 4; i++)
    {
        int d = (i == 1) ? 0 : 2;
        GET_RESULT_OK_OR_ELSE(int, v, divide(10, d), continue;)
        sum += v;
    }
    assert(sum == 15);

    int stopped_at = -1;
    for (int i = 0; i < 5; i++)
    {
        stopped_at = i;
        GET_RESULT_OK_OR_ELSE(int, v, divide(10, i == 2 ? 0 : 1), break;)
        (void)v;
    }
    assert(stopped_at == 2);

    bool jumped = false;
    {
        GET_RESULT_OK_OR_ELSE(int, v, divide(10, 0), goto after_err;)
        (void)v;
        assert(false);
    }
after_err:
    jumped = true;
    assert(jumped);
}

/**
 * @brief Test paths that throw errors and exit processes.
 */
static void test_throwing_paths(void)
{
    char buf[512];
    int code;

    /* ASSERT failure paths */
    code = run_expect_exit(child_assert_fails, buf, sizeof(buf));

    assert(code == ERR_ASSERT_FAILED);
    assert(strstr(buf, "assert plain message") != NULL);

    code = run_expect_exit(child_assert_fmt_fails, buf, sizeof(buf));
    assert(code == ERR_ASSERT_FAILED);
    assert(strstr(buf, "assert fmt message: 42") != NULL);

    code = run_expect_exit(child_throw_plain, buf, sizeof(buf));
    assert(code == ERR_MEMORY_ERROR);
    assert(strstr(buf, "plain message") != NULL);
    assert(strstr(buf, "child_throw_plain") != NULL);

    code = run_expect_exit(child_get_result_throws, buf, sizeof(buf));
    assert(code == ERR_RESULT_IS_ERR);
    assert(strstr(buf, "returned an error") != NULL);

    code = run_expect_exit(child_get_result_named_throws, buf, sizeof(buf));
    assert(code == ERR_RESULT_IS_ERR);
    assert(strstr(buf, "returned an error") != NULL);

    code = run_expect_exit(child_or_else_rethrow, buf, sizeof(buf));
    assert(code == ERR_INVALID_ARGUMENT);
    assert(strstr(buf, "Division by zero") != NULL);

    code = run_expect_exit(child_or_else_unhandled_throws, buf, sizeof(buf));
    assert(code == ERR_RESULT_IS_ERR);
    assert(strstr(buf, "is still an error type but should not be") != NULL);

    code = run_expect_exit(child_get_result_err_on_ok_throws, buf, sizeof(buf));
    assert(code == ERR_RESULT_IS_OK);
    assert(strstr(buf, "to get err of") != NULL);

    code = run_expect_exit(child_get_result_err_named_on_ok_throws, buf, sizeof(buf));
    assert(code == ERR_RESULT_IS_OK);
    assert(strstr(buf, "to get err of") != NULL);

    code = run_expect_exit(child_get_result_err_4arg_on_ok_throws, buf, sizeof(buf));
    assert(code == ERR_RESULT_IS_OK);
    assert(strstr(buf, "to get err of") != NULL);
}

/**
 * @brief Main test runner.
 *
 * @return 0 on success.
 */
int main(void)
{
    test_define_result_all_types();
    test_get_result_variants();
    test_throwing_paths();

    printf("All better_c tests passed!\n");
    return 0;
}
