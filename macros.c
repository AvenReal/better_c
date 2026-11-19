/**
 * @file macros.c
 * @brief Demonstration and example usage of better_c Result and Error macros.
 *
 * This file illustrates how to define Result types, return OK and error values,
 * check Result predicates, and unwrap results using GET_RESULT_OK and GET_RESULT_ERR.
 */

#include <err.h>
#include <stdio.h>
#include <stdlib.h>

#include "error.h"
#include "result.h"


/**
 * @brief Sample color enumeration.
 */
enum color
{
    COLOR_RED,   /**< Red color option. */
    COLOR_GREEN, /**< Green color option. */
    COLOR_BLUE,  /**< Blue color option. */
};

/**
 * @brief 2D point structure.
 */
struct point
{
    int x; /**< X-coordinate. */
    int y; /**< Y-coordinate. */
};

/* DEFINE_RESULT works with primitives; DEFINE_RESULT_NAMED works with enums,
 * structs, pointers, struct pointers, pointer-to-pointers, and more. */
DEFINE_RESULT(int)
DEFINE_RESULT_NAMED(enum color, color)
DEFINE_RESULT_NAMED(struct point, point)
DEFINE_RESULT_NAMED(int *, int_ptr)
DEFINE_RESULT_NAMED(struct point *, point_ptr)
DEFINE_RESULT_NAMED(int **, int_ptr_ptr)

/**
 * @brief Perform integer division returning a RESULT(int).
 *
 * Divides numerator a by denominator b. Returns success with quotient if b != 0,
 * or error with ERR_INVALID_ARGUMENT.
 *
 * @param a Numerator.
 * @param b Denominator.
 * @return RESULT(int) containing quotient on success, or ERR_INVALID_ARGUMENT on division by zero.
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
 * @brief Parse an integer ID into a color Result.
 *
 * Returns success with corresponding enum color, or error with ERR_INVALID_ARGUMENT.
 *
 * @param id Integer color identifier.
 * @return RESULT(color) holding enum color on success, or ERR_INVALID_ARGUMENT on failure.
 */
static RESULT(color) parse_color(int id)
{
    if (id < COLOR_RED || id > COLOR_BLUE)
    {
        return SET_RESULT_ERR(color, ERR_INVALID_ARGUMENT, "Unknown color id");
    }
    return SET_RESULT_OK(color, (enum color)id);
}

/**
 * @brief Create a point struct Result with the given coordinates.
 *
 * Returns success with point struct holding (x, y).
 *
 * @param x X-coordinate.
 * @param y Y-coordinate.
 * @return RESULT(point) holding struct point with coordinates (x, y).
 */
static RESULT(point) make_point(int x, int y)
{
    struct point p = { .x = x, .y = y };
    return SET_RESULT_OK(point, p);
}

/**
 * @brief Dynamically allocate a point on the heap.
 *
 * Returns success with heap-allocated struct point *, or ERR_MEMORY_ERROR on failure.
 *
 * @param x X-coordinate.
 * @param y Y-coordinate.
 * @return RESULT(point_ptr) holding allocated struct point * on success, or ERR_MEMORY_ERROR on failure.
 */
static RESULT(point_ptr) alloc_point(int x, int y)
{
    struct point *p = malloc(sizeof(*p));
    if (p == NULL)
    {
        return SET_RESULT_ERR(point_ptr, ERR_MEMORY_ERROR,
                              "Failed to allocate point");
    }
    p->x = x;
    p->y = y;
    return SET_RESULT_OK(point_ptr, p);
}

/**
 * @brief Dynamically allocate a 2D integer matrix.
 *
 * Returns success with allocated int **, or error on allocation/argument failure.
 *
 * @param rows Number of rows (must be > 0).
 * @param cols Number of columns (must be > 0).
 * @return RESULT(int_ptr_ptr) holding allocated matrix pointer on success, or error on failure.
 */
static RESULT(int_ptr_ptr) alloc_matrix(size_t rows, size_t cols)
{
    if (rows == 0 || cols == 0)
    {
        return SET_RESULT_ERR(int_ptr_ptr, ERR_INVALID_ARGUMENT,
                              "Matrix dimensions must be non-zero");
    }

    int **mat = malloc(rows * sizeof(*mat));
    if (mat == NULL)
    {
        return SET_RESULT_ERR(int_ptr_ptr, ERR_MEMORY_ERROR,
                              "Failed to allocate matrix rows");
    }

    for (size_t i = 0; i < rows; i++)
    {
        mat[i] = malloc(cols * sizeof(*mat[i]));
        if (mat[i] == NULL)
        {
            for (size_t j = 0; j < i; j++)
            {
                free(mat[j]);
            }
            free(mat);
            return SET_RESULT_ERR(int_ptr_ptr, ERR_MEMORY_ERROR,
                                  "Failed to allocate matrix column");
        }
        for (size_t j = 0; j < cols; j++)
        {
            mat[i][j] = (int)(i * cols + j);
        }
    }

    return SET_RESULT_OK(int_ptr_ptr, mat);
}

/**
 * @brief Main entry point demonstrating macro usage.
 *
 * @return 0 on success, non-zero on failure.
 */
int main(void)
{
    /* Predicate checks before unwrapping */
    RESULT(int) probe = divide(10, 0);
    if (RESULT_IS_OK(probe) || !RESULT_IS_ERR(probe))
    {
        THROW_ERROR(ERR_UNKNOWN, "Expected division by zero to fail");
    }

    /* Simple unwrap: throws ERR_RESULT_IS_ERR automatically on error */
    GET_RESULT_OK(int, quotient, divide(10, 2))
    GET_RESULT_OK_NAMED(enum color, c, parse_color(COLOR_GREEN), color)
    GET_RESULT_OK_NAMED(struct point, pt, make_point(3, 4), point)

    /* Fallback values on error (single-word and named types) */
    GET_RESULT_OK_OR(int, safe_div, divide(10, 0), -1)
    GET_RESULT_OK_NAMED_OR(enum color, fallback_color, parse_color(99), color,
                           COLOR_RED)

    /* Unwrapping error messages directly without accessing .err */
    GET_RESULT_ERR(int, err_msg, divide(10, 0))
    GET_RESULT_ERR_NAMED(enum color, col_err_msg, parse_color(99), color)
    (void)err_msg;
    (void)col_err_msg;


    /* Custom error handler for single-word type */
    GET_RESULT_OK_OR_ELSE(int, checked_div, divide(20, 4), {
        THROW_ERROR(checked_div_result.status,
                    GET_RESULT_ERR(checked_div_result));
    })

    /* Custom error handler with explicit error info for named types */
    GET_RESULT_OK_NAMED_OR_ELSE(
        struct point *, heap_pt, alloc_point(7, 8), point_ptr, {
            fprintf(stderr, "alloc_point failed (%d): %s\n",
                    (int)heap_pt_result.status,
                    GET_RESULT_ERR(heap_pt_result));
            return 1;
        })

    /* Pointer-to-pointer result */
    GET_RESULT_OK_NAMED_OR_ELSE(int **, matrix, alloc_matrix(2, 2), int_ptr_ptr,
                                {
                                    free(heap_pt);
                                    return 1;
                                })

    /* Standard allocation with THROW_ERROR on OOM */
    int *numbers = malloc(4 * sizeof(*numbers));
    if (numbers == NULL)
    {
        free(matrix[0]);
        free(matrix[1]);
        free(matrix);
        free(heap_pt);
        THROW_ERROR(ERR_MEMORY_ERROR, "Failed to allocate numbers array");
    }
    for (int i = 0; i < 4; i++)
    {
        numbers[i] = i * 10;
    }

    printf("quotient=%d safe_div=%d checked_div=%d color=%d fallback_color=%d "
           "pt=(%d,%d) heap_pt=(%d,%d) mat[1][1]=%d numbers[3]=%d\n",
           quotient, safe_div, checked_div, (int)c, (int)fallback_color, pt.x,
           pt.y, heap_pt->x, heap_pt->y, matrix[1][1], numbers[3]);

    free(numbers);
    free(matrix[0]);
    free(matrix[1]);
    free(matrix);
    free(heap_pt);
    return 0;
}
