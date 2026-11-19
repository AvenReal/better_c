# better_c

## Goal of the Project

`better_c` is a lightweight, header-only C11 macro library designed to bring modern, robust language ergonomics to C projects. Its primary goal is to provide expressive fatal error reporting, location-aware assertions, and Rust-inspired typed `Result` error handling without requiring `typedef` definitions, heap allocations, or external runtime dependencies.

## Documentation

Full API documentation generated with Doxygen is available at:
https://avenreal.github.io/better_c/

---

## Headers Overview

The library consists of two modular headers located in `src/`:

- [`src/error.h`](src/error.h): Diagnostic error codes, fatal error termination macros (`THROW_ERROR`, `THROW_ERROR_FMT`), assertion checks (`ASSERT`, `ASSERT_FMT`), and stderr logging utilities.
- [`src/result.h`](src/result.h): Type-safe `Result` definitions, constructors (`SET_RESULT_OK`, `SET_RESULT_ERR`), predicate checks (`RESULT_IS_OK`, `RESULT_IS_ERR`), and unwrapping macros (`GET_RESULT_OK`, `GET_RESULT_OK_OR`, `GET_RESULT_OK_OR_ELSE`, `GET_RESULT_ERR`).

---

## 1. `src/error.h`

### What it adds

- **`enum error_code`**: Standard error codes shared across fatal assertions and Result types:
  - `ERR_NO_ERROR` (`0`) — Successful operation.
  - `ERR_ASSERT_FAILED` — Assertion check failed.
  - `ERR_MEMORY_ERROR` — Memory allocation failed.
  - `ERR_INVALID_ARGUMENT` — Invalid parameter provided.
  - `ERR_RESULT_IS_ERR` — Attempted to unwrap an error Result as OK.
  - `ERR_RESULT_IS_OK` — Attempted to unwrap an OK Result as an error.
  - `ERR_UNKNOWN` — Unspecified or fallback error.
- **`THROW_ERROR(Code, Msg)` & `THROW_ERROR_FMT(Code, Fmt, ...)`**: Prints source file name, function name, line number, exit code, and error message to `stderr`, then terminates the process with `exit(Code)`.
- **`ASSERT(Cond, Msg)` & `ASSERT_FMT(Cond, Fmt, ...)`**: Evaluates `Cond`. If false (`0`), terminates the process with `ERR_ASSERT_FAILED` and detailed location diagnostics.
- **`PRINT_STDERR(Msg)` & `PRINT_STDERR_FMT(Fmt, ...)`**: Diagnostic printing to `stderr` with file, function, and line prefix without terminating.
- **`__FILE_NAME__`**: Fallback to `__FILE__` if the compiler does not predefine `__FILE_NAME__`.

### Simple Example

```c
#include <stdio.h>
#include "error.h"

static void process_age(int age)
{
    ASSERT(age >= 0, "Age cannot be negative");

    if (age > 150)
    {
        THROW_ERROR_FMT(ERR_INVALID_ARGUMENT, "Unrealistic age: %d", age);
    }

    printf("Age is valid: %d\n", age);
}

int main(void)
{
    process_age(25);
    return 0;
}
```

---

## 2. `src/result.h`

### What it adds

- **Type Definition Macros**:
  - `DEFINE_RESULT(Type)`: Generates `RESULT(Type)` (`struct Type_result`), `RESULT_UNION(Type)`, and inline constructors `Type_result_ok` / `Type_result_err` for single-word types (`int`, `double`, `size_t`).
  - `DEFINE_RESULT_NAMED(Type, Name)`: Generates `RESULT(Name)` (`struct Name_result`), `RESULT_UNION(Name)`, and constructors for complex types (pointers `int *`, structs `struct point`, enums, qualifiers).
- **Type Specifiers**:
  - `RESULT(Name)`: Expands to `struct Name_result`.
  - `RESULT_UNION(Name)`: Expands to `union Name_result_union`.
- **Constructors**:
  - `SET_RESULT_OK(Name, Value)`: Constructs a successful Result holding `Value` with `ERR_NO_ERROR`.
  - `SET_RESULT_ERR(Name, Code, Message)`: Constructs an error Result holding `Message` (`const char *`) with `Code`. Coerces `ERR_NO_ERROR` to `ERR_UNKNOWN`.
- **Predicates**:
  - `RESULT_IS_OK(...)`: Checks if `.status == ERR_NO_ERROR`.
  - `RESULT_IS_ERR(...)`: Checks if `.status != ERR_NO_ERROR`.
- **Unwrapping & Pattern Matching**:
  - `GET_RESULT_OK(Type, Var, Expr)` / `GET_RESULT_OK_NAMED(...)`: Unwraps into `Type Var`, immediately terminating with `ERR_RESULT_IS_ERR` on error.
  - `GET_RESULT_OK_OR(Type, Var, Expr, Fallback)`: Unwraps into `Type Var`, lazily evaluating `Fallback` on error.
  - `GET_RESULT_OK_OR_ELSE(Type, Var, Expr, Handler)`: Unwraps into `Type Var`, executing `Handler` on error with `<Var>_result` in scope for inspection or recovery via `SET_RESULT_OK`.
  - `GET_RESULT_ERR(Res)` / `GET_RESULT_ERR(Type, Var, Expr)`: Safely inspects or unwraps error message string into `const char *Var`.

### Simple Example

```c
#include <stdio.h>
#include "result.h"

/* Define RESULT(int) */
DEFINE_RESULT(int)

static RESULT(int) divide(int a, int b)
{
    if (b == 0)
    {
        return SET_RESULT_ERR(int, ERR_INVALID_ARGUMENT, "Cannot divide by zero");
    }
    return SET_RESULT_OK(int, a / b);
}

int main(void)
{
    /* 1. Unwrapping success */
    GET_RESULT_OK(int, quotient, divide(10, 2))
    printf("10 / 2 = %d\n", quotient);

    /* 2. Fallback on error */
    GET_RESULT_OK_OR(int, safe_div, divide(10, 0), -1)
    printf("10 / 0 with fallback = %d\n", safe_div);

    /* 3. Handling error with custom logic */
    GET_RESULT_OK_OR_ELSE(int, handled, divide(10, 0), {
        printf("Division error caught (%d): %s\n",
               handled_result.status, GET_RESULT_ERR(handled_result));
        handled_result = SET_RESULT_OK(int, 0); /* recover with 0 */
    })
    printf("Recovered value: %d\n", handled);

    return 0;
}
```

---

## Detailed Macros Reference

### Error Handling (`src/error.h`)

#### `THROW_ERROR(Code, Msg)`
Terminates the process with file, function, line number, exit code, and plain string `Msg` to `stderr`. Safe for runtime strings since `Msg` is printed through `"%s"`.

#### `THROW_ERROR_FMT(Code, Fmt, ...)`
Terminates the process with file, function, line number, exit code, and a formatted message matching `Fmt`.

#### `ASSERT(Cond, Msg)` and `ASSERT_FMT(Cond, Fmt, ...)`
Evaluates `Cond`. If false (`0`), terminates the process with `ERR_ASSERT_FAILED` and diagnostics.

---

### Typed Results (`src/result.h`)

#### Result Struct Architecture
Result types encapsulate their payload inside a dedicated union member `.value`:
- `RESULT_UNION(Name)` (`union Name_result_union`):
  - `Type ok;` — payload on success.
  - `const char *err;` — error message on failure.
- `RESULT(Name)` (`struct Name_result`):
  - `enum error_code status;` — error status code.
  - `RESULT_UNION(Name) value;` — union holding either `ok` or `err`.

*Note:* `.ok` and `.err` should not be accessed directly as struct members; access them via the standard macros (`SET_RESULT_OK`, `SET_RESULT_ERR`, `GET_RESULT_OK*`, `GET_RESULT_ERR*`).

#### Defining Types
```c
DEFINE_RESULT(int)
DEFINE_RESULT(double)
DEFINE_RESULT_NAMED(unsigned long, ulong)
DEFINE_RESULT_NAMED(struct point, point)
DEFINE_RESULT_NAMED(int *, int_ptr)
```

#### Unwrapping Variations
- **`GET_RESULT_OK(Type, Var, Expr)`**: Evaluates `Expr`. On error, terminates via `THROW_ERROR(ERR_RESULT_IS_ERR, ...)`.
- **`GET_RESULT_OK_OR(Type, Var, Expr, Fallback)`**: Evaluates `Expr`. If error, assigns `Fallback` expression lazily.
- **`GET_RESULT_OK_OR_ELSE(Type, Var, Expr, Handler)`**: Evaluates `Expr`. On error, executes `Handler`. The handler must either transfer control (`return`, `break`, `continue`, `goto`, `THROW_ERROR`) or recover explicitly with `<Var>_result = SET_RESULT_OK(Name, value);`.
- **`GET_RESULT_ERR(Res)`**: Returns `const char *` error string from an existing result variable.
- **`GET_RESULT_ERR(Type, Var, Expr)`**: Evaluates `Expr` and unwraps error message into `const char *Var`. If `Expr` succeeded, terminates with `ERR_RESULT_IS_OK`.

---

## Building and Testing

The project uses `make` and standard C11 (`gcc` or `clang`):

```sh
make        # builds macros demonstration and test binaries
make test   # builds and runs test_error, test_result, and macros
make docs   # generates Doxygen documentation in docs/ (for src/ only)
make clean  # removes compiled binaries
```

Full runnable demonstration code can be found in [macros.c](macros.c).
Separate test suites are located in [tests/test_error.c](tests/test_error.c) and [tests/test_result.c](tests/test_result.c).
