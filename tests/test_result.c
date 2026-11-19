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

#include "result.h"

struct opaque_handle;

enum color
{
    COLOR_RED = 10,
    COLOR_GREEN = 20,
    COLOR_BLUE = 30,
};

struct point
{
    int x;
    int y;
};

struct nested_box
{
    struct point origin;
    int width;
    int height;
};

union payload
{
    int as_int;
    double as_double;
};

struct ro_record
{
    int id;
    int value;
};

struct bitfield_flags
{
    unsigned int active : 1;
    unsigned int mode : 3;
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
DEFINE_RESULT_NAMED(struct bitfield_flags, bitfield_flags)
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

static RESULT(int) divide(int a, int b)
{
    if (b == 0)
    {
        return SET_RESULT_ERR(int, ERR_INVALID_ARGUMENT, "Division by zero");
    }
    return SET_RESULT_OK(int, a / b);
}

static RESULT(point) fail_point(void)
{
    return SET_RESULT_ERR(point, ERR_INVALID_ARGUMENT, "Point creation failed");
}

static RESULT(int) fail_null_msg(void)
{
    return SET_RESULT_ERR(int, ERR_MEMORY_ERROR, NULL);
}

static int side_effect_calls = 0;

static RESULT(int) counted_ok(int v)
{
    side_effect_calls++;
    return SET_RESULT_OK(int, v);
}

static RESULT(int) counted_err(void)
{
    side_effect_calls++;
    return SET_RESULT_ERR(int, ERR_INVALID_ARGUMENT, "counted failure");
}

static int default_eval_calls = 0;

static int compute_default(void)
{
    default_eval_calls++;
    return 999;
}

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

static void child_get_result_throws(void)
{
    int x;
    GET_RESULT_OK(int, x, divide(1, 0))
    (void)x;
}

static void child_get_result_named_throws(void)
{
    struct point pt;
    GET_RESULT_OK_NAMED(struct point, pt, fail_point(), point)
    (void)pt;
}

static void child_or_else_rethrow(void)
{
    int x;
    GET_RESULT_OK_OR_ELSE(int, x, divide(1, 0),
                          THROW_ERROR_FMT(x_result.status,
                                          "Result error msg: %s",
                                          GET_RESULT_ERR(x_result));)
    (void)x;
}

static void child_or_else_unhandled_throws(void)
{
    int x;
    /* Falling through without recovering or transferring control must throw */
    GET_RESULT_OK_OR_ELSE(int, x, divide(1, 0), { (void)0; })
    (void)x;
}

static void child_get_result_err_on_ok_throws(void)
{
    const char *err_msg;
    GET_RESULT_ERR(int, err_msg, divide(10, 2))
    (void)err_msg;
}

static void child_get_result_err_named_on_ok_throws(void)
{
    const char *err_msg;
    GET_RESULT_ERR_NAMED(struct point, err_msg,
                         SET_RESULT_OK(point, ((struct point){ 1, 2 })), point)
    (void)err_msg;
}

static void child_get_result_err_4arg_on_ok_throws(void)
{
    const char *err_msg;
    GET_RESULT_ERR(struct point, err_msg,
                   SET_RESULT_OK(point, ((struct point){ 1, 2 })), point)
    (void)err_msg;
}

static void test_define_result_all_types(void)
{
    int i_val;
    /* Primitives & multi-word & const */
    GET_RESULT_OK(int, i_val, int_result_ok(42))
    assert(i_val == 42);

    double d_val;
    GET_RESULT_OK(double, d_val, SET_RESULT_OK(double, 3.5))
    assert(d_val == 3.5);

    unsigned long ul_val;
    GET_RESULT_OK_NAMED(unsigned long, ul_val, SET_RESULT_OK(ulong, 123456UL),
                        ulong)
    assert(ul_val == 123456UL);

    int ci_val;
    GET_RESULT_OK_NAMED(int, ci_val, SET_RESULT_OK(const_int, 77), const_int)
    assert(ci_val == 77);

    struct point cp_init = { .x = 8, .y = 9 };
    struct point cp_val;
    GET_RESULT_OK_NAMED(struct point, cp_val,
                        SET_RESULT_OK(const_point, cp_init), const_point)
    assert(cp_val.x == 8 && cp_val.y == 9);

    int cp_target = 10;
    int *cip_val;
    GET_RESULT_OK_NAMED(int *const, cip_val,
                        SET_RESULT_OK(const_int_ptr, &cp_target), const_int_ptr)
    assert(cip_val == &cp_target);

    /* Unwrapping const-qualified and volatile-qualified result variables */
    const RESULT(int) const_res = SET_RESULT_OK(int, 88);
    int from_const;
    GET_RESULT_OK(int, from_const, const_res)
    assert(from_const == 88);

    volatile RESULT(int) vol_res = SET_RESULT_OK(int, 89);
    int from_vol;
    GET_RESULT_OK(int, from_vol, vol_res)
    assert(from_vol == 89);

    GET_RESULT_OK_OR(int, vol_or, vol_res, 0)
    assert(vol_or == 89);

    /* Enums & enum pointers */
    enum color col = COLOR_BLUE;
    enum color c_val;
    GET_RESULT_OK_NAMED(enum color, c_val, SET_RESULT_OK(color, COLOR_GREEN),
                        color)
    assert(c_val == COLOR_GREEN);

    enum color *cp_col;
    GET_RESULT_OK_NAMED(enum color *, cp_col, SET_RESULT_OK(color_ptr, &col),
                        color_ptr)
    assert(cp_col == &col);

    /* Structs (parenthesized compound literal & variable), nested, const member
     */
    struct point pt = { .x = 3, .y = 4 };
    struct point pt_out;
    GET_RESULT_OK_NAMED(
        struct point, pt_out,
        SET_RESULT_OK(point, ((struct point){ .x = 3, .y = 4 })), point)
    assert(pt_out.x == 3 && pt_out.y == 4);

    struct nested_box box = { .origin = { 1, 2 }, .width = 10, .height = 20 };
    struct nested_box box_out;
    GET_RESULT_OK_NAMED(struct nested_box, box_out,
                        SET_RESULT_OK(nested_box, box), nested_box)
    assert(box_out.origin.x == 1 && box_out.height == 20);

    struct ro_record ro = { .id = 42, .value = 7 };
    struct ro_record ro_out;
    GET_RESULT_OK_NAMED(struct ro_record, ro_out, SET_RESULT_OK(ro_record, ro),
                        ro_record)
    assert(ro_out.id == 42 && ro_out.value == 7);

    struct bitfield_flags bf_in = { .active = 1, .mode = 5 };
    struct bitfield_flags bf_out;
    GET_RESULT_OK_NAMED(struct bitfield_flags, bf_out,
                        SET_RESULT_OK(bitfield_flags, bf_in), bitfield_flags)
    assert(bf_out.active == 1 && bf_out.mode == 5);

    GET_RESULT_OK_NAMED_OR(union payload, pl_fallback,
                           SET_RESULT_ERR(payload, ERR_UNKNOWN, "pl"), payload,
                           ((union payload){ .as_int = 555 }))
    assert(pl_fallback.as_int == 555);

    /* Unions & union pointers */
    union payload pl = { .as_int = 99 };
    union payload pl_out;
    GET_RESULT_OK_NAMED(union payload, pl_out, SET_RESULT_OK(payload, pl),
                        payload)
    assert(pl_out.as_int == 99);

    union payload *pl_ptr_out;
    GET_RESULT_OK_NAMED(union payload *, pl_ptr_out,
                        SET_RESULT_OK(payload_ptr, &pl), payload_ptr)
    assert(pl_ptr_out == &pl);

    /* Pointers */
    int val = 123;
    int *val_p = &val;
    int **val_pp = &val_p;
    int ***val_ppp = &val_pp;

    int *ip_out;
    GET_RESULT_OK_NAMED(int *, ip_out, SET_RESULT_OK(int_ptr, val_p), int_ptr)
    assert(ip_out == val_p);

    const char *s_out;
    GET_RESULT_OK_NAMED(const char *, s_out, SET_RESULT_OK(cstr, "hello"), cstr)
    assert(strcmp(s_out, "hello") == 0);

    void *vp_out;
    GET_RESULT_OK_NAMED(void *, vp_out, SET_RESULT_OK(void_ptr, val_p),
                        void_ptr)
    assert(vp_out == (void *)val_p);

    /* Struct pointers, const struct pointers, opaque forward-declared ptr */
    struct point *pt_p = &pt;
    struct point **pt_pp = &pt_p;

    struct point *pt_p_out;
    GET_RESULT_OK_NAMED(struct point *, pt_p_out,
                        SET_RESULT_OK(point_ptr, pt_p), point_ptr)
    assert(pt_p_out == pt_p);

    const struct point *cpt_p_out;
    GET_RESULT_OK_NAMED(const struct point *, cpt_p_out,
                        SET_RESULT_OK(const_point_ptr, pt_p), const_point_ptr)
    assert(cpt_p_out == pt_p);

    struct opaque_handle *opaque = NULL;
    struct opaque_handle *op_out;
    GET_RESULT_OK_NAMED(struct opaque_handle *, op_out,
                        SET_RESULT_OK(opaque_ptr, opaque), opaque_ptr)
    assert(op_out == NULL);

    /* Pointer pointers & triple pointers */
    int **ipp_out;
    GET_RESULT_OK_NAMED(int **, ipp_out, SET_RESULT_OK(int_ptr_ptr, val_pp),
                        int_ptr_ptr)
    assert(ipp_out == val_pp && **ipp_out == 123);

    struct point **pt_pp_out;
    GET_RESULT_OK_NAMED(struct point **, pt_pp_out,
                        SET_RESULT_OK(point_ptr_ptr, pt_pp), point_ptr_ptr)
    assert(pt_pp_out == pt_pp && (**pt_pp_out).y == 4);

    int ***ippp_out;
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

static int helper_return_on_error(int denom)
{
    int q;
    GET_RESULT_OK_OR_ELSE(int, q, divide(10, denom), return -1;)
    return q;
}

static int helper_block_with_commas(int denom)
{
    int q;
    GET_RESULT_OK_OR_ELSE(int, q, divide(10, denom), {
        int a = -10, b = -5;
        return a + b;
    })
    return q;
}

static int helper_unparenthesized_commas(int denom)
{
    int q;
    GET_RESULT_OK_OR_ELSE(int, q, divide(10, denom), int a = 3, b = 4;
                          return -(a + b);)
    return q;
}

static int helper_or_else_inspect(int denom, enum error_code *out_code,
                                  const char **out_msg)
{
    int q;
    GET_RESULT_OK_OR_ELSE(int, q, divide(10, denom), {
        *out_code = q_result.status;
        *out_msg = GET_RESULT_ERR(q_result);
        return -99;
    })
    return q;
}

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

    struct point *p_zero;
    GET_RESULT_OK_NAMED_OR_ELSE(
        struct point *, p_zero,
        SET_RESULT_ERR(point_ptr, ERR_MEMORY_ERROR, "not a point"), point_ptr, {
            handler_ran = true;
            p_zero_result = SET_RESULT_OK(point_ptr, NULL);
        })
    assert(handler_ran);
    assert(p_zero == NULL);

    struct nested_box b_zero;
    GET_RESULT_OK_NAMED_OR_ELSE(
        struct nested_box, b_zero,
        SET_RESULT_ERR(nested_box, ERR_INVALID_ARGUMENT, "bad box"), nested_box,
        {
            assert(b_zero_result.status == ERR_INVALID_ARGUMENT);
            b_zero_result = SET_RESULT_OK(
                nested_box, ((struct nested_box){ { 0, 0 }, 0, 0 }));
        })
    assert(b_zero.origin.x == 0 && b_zero.origin.y == 0 && b_zero.width == 0
           && b_zero.height == 0);

    /* Explicit recovery inside OR_ELSE handler */
    int recovered;
    GET_RESULT_OK_OR_ELSE(int, recovered, divide(10, 0),
                          { recovered_result = SET_RESULT_OK(int, 123); })
    assert(recovered == 123);

    /* NULL error message preserved via GET_RESULT_ERR */
    bool saw_null = false;
    int z;
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
                           SET_RESULT_ERR(ulong, ERR_UNKNOWN, "ul"), ulong,
                           999999UL)
    assert(fallback_ul == 999999UL);

    GET_RESULT_OK_NAMED_OR(enum color, fallback_col,
                           SET_RESULT_ERR(color, ERR_UNKNOWN, "col"), color,
                           COLOR_BLUE)
    assert(fallback_col == COLOR_BLUE);

    /* GET_RESULT_OK_NAMED_OR with const types */
    GET_RESULT_OK_NAMED_OR(const int, fallback_ci,
                           SET_RESULT_ERR(const_int, ERR_UNKNOWN, "ci"),
                           const_int, 42)
    assert(fallback_ci == 42);

    struct point fallback_cp_val_init = { .x = 10, .y = 20 };
    GET_RESULT_OK_NAMED_OR(const struct point, fallback_cp_val,
                           SET_RESULT_ERR(const_point, ERR_UNKNOWN, "cp"),
                           const_point, fallback_cp_val_init)
    assert(fallback_cp_val.x == 10 && fallback_cp_val.y == 20);

    /* GET_RESULT_OK_NAMED_OR with pointer to const types */
    struct point fallback_cp_init = { .x = 10, .y = 20 };
    GET_RESULT_OK_NAMED_OR(const struct point *, fallback_cp,
                           SET_RESULT_ERR(const_point_ptr, ERR_UNKNOWN, "cp"),
                           const_point_ptr, &fallback_cp_init)
    assert(fallback_cp->x == 10 && fallback_cp->y == 20);

    GET_RESULT_OK_NAMED_OR(const char *, fallback_cstr,
                           SET_RESULT_ERR(cstr, ERR_UNKNOWN, "cstr"),
                           cstr, "fallback")
    assert(strcmp(fallback_cstr, "fallback") == 0);

    struct ro_record fallback_ro_init = { .id = 1, .value = 2 };
    GET_RESULT_OK_NAMED_OR(struct ro_record, fallback_ro,
                           SET_RESULT_ERR(ro_record, ERR_UNKNOWN, "ro"),
                           ro_record, fallback_ro_init)
    assert(fallback_ro.id == 1 && fallback_ro.value == 2);

    /* GET_RESULT_OK_NAMED_OR on OK result with named type */
    GET_RESULT_OK_NAMED_OR(struct point, ok_pt,
                           SET_RESULT_OK(point, ((struct point){ .x = 1, .y = 2 })),
                           point, ((struct point){ .x = 9, .y = 8 }))
    assert(ok_pt.x == 1 && ok_pt.y == 2);

    /* GET_RESULT_OK_NAMED_OR with bitfields */
    struct bitfield_flags fallback_bf_init = { .active = 0, .mode = 3 };
    GET_RESULT_OK_NAMED_OR(struct bitfield_flags, fallback_bf,
                           SET_RESULT_ERR(bitfield_flags, ERR_UNKNOWN, "bf"),
                           bitfield_flags, fallback_bf_init)
    assert(fallback_bf.active == 0 && fallback_bf.mode == 3);

    /* GET_RESULT_ERR statement unwrap test */
    const char *unwrap_err_msg;
    GET_RESULT_ERR(int, unwrap_err_msg, divide(10, 0))
    assert(strcmp(unwrap_err_msg, "Division by zero") == 0);

    const char *unwrap_named_err;
    GET_RESULT_ERR_NAMED(struct point, unwrap_named_err, fail_point(), point)
    assert(strcmp(unwrap_named_err, "Point creation failed") == 0);

    const char *unwrap_4arg_err;
    GET_RESULT_ERR(struct point, unwrap_4arg_err, fail_point(), point)
    assert(strcmp(unwrap_4arg_err, "Point creation failed") == 0);

    /* Single evaluation of Expr */
    side_effect_calls = 0;
    int once_ok;
    GET_RESULT_OK(int, once_ok, counted_ok(11))
    assert(once_ok == 11 && side_effect_calls == 1);

    side_effect_calls = 0;
    GET_RESULT_OK_OR(int, once_err, counted_err(), 55)
    assert(once_err == 55 && side_effect_calls == 1);

    /* Single evaluation of Expr in GET_RESULT_ERR */
    side_effect_calls = 0;
    const char *once_err_eval;
    GET_RESULT_ERR(int, once_err_eval, counted_err())
    assert(strcmp(once_err_eval, "counted failure") == 0 && side_effect_calls == 1);

    /* Control flow: continue, break, and goto inside handlers */
    int sum = 0;
    for (int i = 0; i < 4; i++)
    {
        int d = (i == 1) ? 0 : 2;

        int v;
        GET_RESULT_OK_OR_ELSE(int, v, divide(10, d), continue;)

        sum += v;
    }
    assert(sum == 15);

    int stopped_at = -1;
    for (int i = 0; i < 5; i++)
    {
        stopped_at = i;

        int v;
        GET_RESULT_OK_OR_ELSE(int, v, divide(10, i == 2 ? 0 : 1), break;)

        (void)v;
    }
    assert(stopped_at == 2);

    bool jumped = false;
    {
        int v;
        GET_RESULT_OK_OR_ELSE(int, v, divide(10, 0), goto after_err;)

        (void)v;
        assert(false);
    }
after_err:
    jumped = true;
    assert(jumped);
}

static void test_throwing_paths(void)
{
    char buf[512];
    int code;

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

    code = run_expect_exit(child_get_result_err_named_on_ok_throws, buf,
                           sizeof(buf));
    assert(code == ERR_RESULT_IS_OK);
    assert(strstr(buf, "to get err of") != NULL);

    code = run_expect_exit(child_get_result_err_4arg_on_ok_throws, buf,
                           sizeof(buf));
    assert(code == ERR_RESULT_IS_OK);
    assert(strstr(buf, "to get err of") != NULL);
}

int main(void)
{
    test_define_result_all_types();
    test_get_result_variants();
    test_throwing_paths();

    printf("All result.h tests passed!\n");
    return 0;
}
