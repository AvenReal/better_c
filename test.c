/**
 * @file test.c
 * @brief Simple demonstration of better_c divide function.
 */

#include <stdio.h>

#include "better_c.h"

DEFINE_RESULT(int)

/**
 * @brief Divide two integers and return a Result.
 *
 * Divides numerator a by denominator b. Returns success with quotient or
 * error with ERR_INVALID_ARGUMENT.
 *
 * @param a Numerator.
 * @param b Denominator.
 * @return RESULT(int) holding quotient on success or error on division by zero.
 */
static RESULT(int) divide(int a, int b)
{
    if (b == 0)
    {
        return SET_RESULT_ERR(int, ERR_INVALID_ARGUMENT, "Cannot divide by 0.");
    }

    return SET_RESULT_OK(int, a / b);
}

/**
 * @brief Main function.
 *
 * @return 0 on success.
 */
int main(void)
{
    GET_RESULT_OK(int, res, divide(5, 1))

    printf("%d\n", res);
    return 0;
}
