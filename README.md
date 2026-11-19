# better_c Macros Guide

`better_c` provides lightweight C11 macros for error reporting and typed `Result` values across [error.h](error.h), [result.h](result.h), and the umbrella header [better_c.h](better_c.h).

## Headers

| Header       | Purpose                                                                                          |
| ------------ | ------------------------------------------------------------------------------------------------ |
| `better_c.h` | Umbrella header that includes both `error.h` and `result.h`.                                     |
| `error.h`    | Error codes (`enum error_code`), `__FILE_NAME__` fallback, `THROW_ERROR`, and `THROW_ERROR_FMT`. |
| `result.h`   | Typed `Result` struct definitions, constructors, predicates, and unwrapping macros.              |

Include `<err.h>` (for `errx`) and the umbrella header in your `.c` files:

```c
#include <err.h>
#include "better_c.h"
```

---

## `error.h` Macros & Types

### `__FILE_NAME__`

Fallback definition to `__FILE__` when the compiler does not predefine `__FILE_NAME__`.

### `enum error_code`

Application error codes used by `THROW_ERROR` / `THROW_ERROR_FMT` and `Result` values:

| Enumerator             | Value / Meaning                                                                                    |
| ---------------------- | -------------------------------------------------------------------------------------------------- |
| `ERR_NO_ERROR`         | `0` — operation succeeded                                                                          |
| `ERR_ASSERT_FAILED`    | Assertion check failure                                                                            |
| `ERR_MEMORY_ERROR`     | Memory allocation failure                                                                          |
| `ERR_INVALID_ARGUMENT` | Invalid input argument                                                                             |
| `ERR_RESULT_IS_ERR`    | Unwrapping a `Result` that is still an error (used by `GET_RESULT_OK*` / `GET_RESULT_OK*_OR_ELSE`) |
| `ERR_RESULT_IS_OK`     | Attempting to unwrap an error from a successful `Result` (used by `GET_RESULT_ERR*`)               |
| `ERR_UNKNOWN`          | Fallback/unspecified error (also used if `SET_RESULT_ERR` is given `ERR_NO_ERROR`)                 |

### `THROW_ERROR(Code, Msg)`

Terminates the process via `errx` with file, function, line number, error code, and a plain string message `Msg` (expands to `THROW_ERROR_FMT(Code, "%s", Msg)`).

Safe for both string literals and runtime `const char *` values (such as `GET_RESULT_ERR(res)`), since `Msg` is always printed through `"%s"` rather than interpreted as a format string.

```c
THROW_ERROR(ERR_MEMORY_ERROR, "Out of memory");
THROW_ERROR(res.status, GET_RESULT_ERR(res));
```

- `Code`: Exit code / `enum error_code` value (evaluated twice by `THROW_ERROR_FMT`, so avoid expressions with side effects).
- `Msg`: Plain error message (`const char *`).

### `THROW_ERROR_FMT(Code, Fmt, ...)`

Terminates the process via `errx` with file, function, line number, error code, and a `printf`-style formatted message.

```c
THROW_ERROR_FMT(ERR_INVALID_ARGUMENT, "Invalid index: %zu", idx);
THROW_ERROR_FMT(x_result.status, "Result error msg: %s", GET_RESULT_ERR(x_result));
```

- `Code`: Exit code / `enum error_code` value (avoid expressions with side effects).
- `Fmt`: `printf`-style format **string literal** (concatenated directly onto the prefix string).
- `...`: One or more arguments matching the specifiers in `Fmt` (for messages without format arguments, use `THROW_ERROR(Code, Msg)` instead).

### `ASSERT(Cond, Msg)` and `ASSERT_FMT(Cond, Fmt, ...)`

Evaluate `Cond`. If false (`0`), terminates the process with `ERR_ASSERT_FAILED` and diagnostics:

```c
ASSERT(ptr != NULL, "Pointer must not be NULL");
ASSERT_FMT(index < size, "Index %zu out of bounds (size %zu)", index, size);
```

- `Cond`: Expression evaluating to a boolean condition.
- `Msg`: Error message string (`const char *`).
- `Fmt`: Format string literal.
- `...`: Arguments matching specifiers in `Fmt`.

---


## `result.h` Macros

### Result Struct Architecture

Under the new implementation, Result types encapsulate their payload inside a dedicated union member `.value`:

- `RESULT_UNION(Name)` (`union <Name>_result_union`) containing:
  - `Type ok;` — payload on success.
  - `const char *err;` — error description string on failure.
- `RESULT(Name)` (`struct <Name>_result`) containing:
  - `enum error_code status;` — error code (`ERR_NO_ERROR` on success).
  - `RESULT_UNION(Name) value;` — union holding either `ok` or `err`.

> **Important Rule:**
> `.ok` and `.err` fields should **NEVER** be accessed directly as struct members. All access and modification must happen exclusively through the standard macros:
> - Setting results: `SET_RESULT_OK(...)` and `SET_RESULT_ERR(...)`
> - Inspecting / unwrapping OK: `GET_RESULT_OK(...)` and its variants
> - Inspecting / unwrapping errors: `GET_RESULT_ERR(...)` and `GET_RESULT_ERR_NAMED(...)`

### Defining Result Types

#### `RESULT_UNION(Name)` and `DEFINE_RESULT_UNION(Name, Type)`

`RESULT_UNION(Name)` expands to `union <Name>_result_union`.

`DEFINE_RESULT_UNION(Name, Type)` defines the union storing the `ok` payload and `const char *err` string:
```c
union Name##_result_union
{
    Type ok;
    const char *err;
};
```
This is called automatically by `DEFINE_RESULT_NAMED(Type, Name)`.

#### `DEFINE_RESULT(Type)`


Defines a `Result` struct, union, and inline constructors for a single-identifier type (`int`, `double`, `size_t`, etc.). Expands to `DEFINE_RESULT_NAMED(Type, Type)`.

```c
DEFINE_RESULT(int)
DEFINE_RESULT(double)
```

#### `DEFINE_RESULT_NAMED(Type, Name)`

Defines a `Result` struct, union, and inline constructors for any type that cannot be pasted directly into a C identifier (enums, structs, unions, multi-word types, pointers, struct pointers, pointer-to-pointers, `const`-qualified types).

```c
DEFINE_RESULT_NAMED(unsigned long, ulong)
DEFINE_RESULT_NAMED(enum color, color)
DEFINE_RESULT_NAMED(struct point, point)
DEFINE_RESULT_NAMED(int *, int_ptr)
DEFINE_RESULT_NAMED(struct point *, point_ptr)
DEFINE_RESULT_NAMED(int **, int_ptr_ptr)
```

For a given `(Type, Name)`, `DEFINE_RESULT_NAMED` generates:

- `RESULT_UNION(Name)` (`union <Name>_result_union { Type ok; const char *err; };`)
- `RESULT(Name)` (`struct <Name>_result { enum error_code status; RESULT_UNION(Name) value; };`)
- `static inline RESULT(Name) <Name>_result_ok(Type value)`
- `static inline RESULT(Name) <Name>_result_err(enum error_code code, const char *error_msg)`

> **Notes:**
>
> - Do not put a trailing `;` after `DEFINE_RESULT(...)` or `DEFINE_RESULT_NAMED(...)` when compiling with `-Wpedantic`.
> - Raw declarator types where the identifier must appear inside the type (such as `int (*)(int, int)` or `int (*)[4]`) and inline anonymous struct definitions are not directly supported by `Type ok;`.

---

### Result Type Name, Constructors & Predicates

#### `RESULT(Name)`

Expands to `struct <Name>_result`. Use it in function return types and variable declarations.

```c
static RESULT(int) divide(int a, int b);
static RESULT(point_ptr) alloc_point(int x, int y);
```

#### `SET_RESULT_OK(Name, Value)`

Constructs a successful `RESULT(Name)` with `status = ERR_NO_ERROR` and `value.ok = Value`.

```c
return SET_RESULT_OK(int, a / b);
return SET_RESULT_OK(point_ptr, p);
```

> **Compound literals with commas:** Because `SET_RESULT_OK(Name, Value)` takes two fixed macro arguments, unparenthesized commas inside `{ ... }` are split by the preprocessor. Assign to a local variable first or wrap the compound literal in extra parentheses:
>
> ```c
> struct point p = { .x = x, .y = y };
> return SET_RESULT_OK(point, p);
>
> /* or */
> return SET_RESULT_OK(point, ((struct point){ .x = x, .y = y }));
> ```

#### `SET_RESULT_ERR(Name, Code, Message)`

Constructs an error `RESULT(Name)` with `status = Code` and `value.err = Message`. If `Code == ERR_NO_ERROR`, `status` is coerced to `ERR_UNKNOWN` so an error result can never masquerade as `OK`.

```c
return SET_RESULT_ERR(int, ERR_INVALID_ARGUMENT, "Division by zero");
```

#### `RESULT_IS_OK(...)` and `RESULT_IS_ERR(...)`

Predicates that check whether a `Result` expression succeeded (`status == ERR_NO_ERROR`) or failed (`status != ERR_NO_ERROR`). Both are variadic so compound literals containing commas work directly.

```c
RESULT(int) r = divide(10, 0);
if (RESULT_IS_ERR(r))
{
    fprintf(stderr, "Failed (%d): %s\n", (int)r.status, GET_RESULT_ERR(r));
}
```

---

### Unwrapping Results

All `GET_RESULT_OK*` and statement-form `GET_RESULT_ERR*` macros are **statement macros** (not expressions): they evaluate `Expr` once into a local result variable and declare unwrapped output variables in the current scope.

Because they expand to multiple statements, always wrap them in `{ ... }` inside an `if`, `else`, `for`, or `while` body, and omit a trailing semicolon.

#### `GET_RESULT_OK(Type, Var, Expr)` and `GET_RESULT_OK_NAMED(Type, Var, Expr, Name)`

Unwraps `Expr` into `Type Var`. If `Expr` returns an error, immediately terminates the process via `THROW_ERROR(ERR_RESULT_IS_ERR, ...)`.

```c
GET_RESULT_OK(int, quotient, divide(10, 2))
GET_RESULT_OK_NAMED(enum color, c, parse_color(COLOR_GREEN), color)
GET_RESULT_OK_NAMED(struct point, pt, make_point(3, 4), point)
```

#### `GET_RESULT_ERR(...)` and `GET_RESULT_ERR_NAMED(Type, Var, Expr, Name)`

Retrieves or unwraps the error message from a Result without accessing `.err` directly:

1. **Accessor form (`GET_RESULT_ERR(Res)`):** Evaluates to the `const char *` error string from an existing Result variable:
   ```c
   const char *msg = GET_RESULT_ERR(q_result);
   ```

2. **Unwrap form (`GET_RESULT_ERR(Type, Var, Expr)` / `GET_RESULT_ERR_NAMED(Type, Var, Expr, Name)`):**
   Evaluates `Expr` and unwraps the error message string into `const char *Var`. If `Expr` was actually an OK result, terminates the process with `ERR_RESULT_IS_OK`:
   ```c
   GET_RESULT_ERR(int, err_msg, divide(10, 0))
   GET_RESULT_ERR_NAMED(struct point, pt_err, fail_point(), point)
   ```

#### `GET_RESULT_OK_OR(Type, Var, Expr, ...)` and `GET_RESULT_OK_NAMED_OR(Type, Var, Expr, Name, ...)`

Unwraps `Expr` into `Type Var`, or initializes `Var` with the fallback expression `...` if `Expr` returns an error.

- The fallback expression is **lazily evaluated** (only evaluated on error).
- Assigns the fallback value to the result (`Var##_result.value.ok = (...)`) and sets `status = ERR_NO_ERROR`.
- Works with mutable types (primitives, mutable structs, pointers, enums). For `const`-qualified types or structs containing `const` members, use `GET_RESULT_OK` / `GET_RESULT_OK_NAMED` since C does not permit assignment to const objects.

```c
GET_RESULT_OK_OR(int, safe_div, divide(10, 0), -1)
GET_RESULT_OK_NAMED_OR(enum color, c, parse_color(99), color, COLOR_RED)
GET_RESULT_OK_NAMED_OR(struct point, pt, make_point_err(), point,
                       ((struct point){ .x = 0, .y = 0 }))
```

#### `GET_RESULT_OK_OR_ELSE(Type, Var, Expr, ...)` and `GET_RESULT_OK_NAMED_OR_ELSE(Type, Var, Expr, Name, ...)`

Unwraps `Expr` into `Type Var`, executing the custom error handler `...` if `Expr` returns an error.

Inside the error handler, `<Var>_result` is in scope:

- `<Var>_result.status` holds the `enum error_code`.
- Error message should be accessed via `GET_RESULT_ERR(<Var>_result)`.

The handler **must** handle the error in one of two ways:

1. **Transfer control** (`return`, `break`, `continue`, `goto`, `THROW_ERROR(...)`, `THROW_ERROR_FMT(...)`, etc.).
2. **Recover explicitly** using `SET_RESULT_OK`:
   `<Var>_result = SET_RESULT_OK(Name, <value>);`
   (never access `.ok` directly).

If the handler falls through while `<Var>_result` is still an error, a second `RESULT_IS_ERR(<Var>_result)` guard automatically terminates the process via `THROW_ERROR(ERR_RESULT_IS_ERR, "<Var>_result is still an error type but should not be.")`, preventing accidental reads of the inactive `.ok` union member.

```c
/* Return on error after logging */
GET_RESULT_OK_NAMED_OR_ELSE(struct point *, heap_pt, alloc_point(7, 8),
                         point_ptr, {
    fprintf(stderr, "alloc_point failed (%d): %s\n",
            (int)heap_pt_result.status, GET_RESULT_ERR(heap_pt_result));
    return 1;
})

/* Re-throw with a formatted message */
GET_RESULT_OK_OR_ELSE(int, x, divide(1, 0),
                   THROW_ERROR_FMT(x_result.status, "Result error msg: %s",
                                   GET_RESULT_ERR(x_result));)

/* Explicit recovery using SET_RESULT_OK */
GET_RESULT_OK_OR_ELSE(int, recovered, divide(10, 0), {
    recovered_result = SET_RESULT_OK(int, 123);
})

/* Loop control flow */
for (int i = 0; i < 4; i++)
{
    GET_RESULT_OK_OR_ELSE(int, v, divide(10, i), continue;)
    sum += v;
}
```

---

## Complete Example

See [macros.c](macros.c) for a full runnable example and [test_better_c.c](test_better_c.c) for the test suite.

```sh
make        # builds ./macros
make test   # builds and runs ./test_better_c and ./macros
make clean  # removes built binaries
```
