* one pass is enough

* types

  * size
  * alignment

  ---

  * void
  * int types
  * pointer
  * array
  * struct

* values

  * lvalue

    has an address (can & it)

    variables

    * global

      decl many times

      def at most one time

    * static

    * local

  * rvalue

    temporary (may only exit in a register)

    constant

* symbols

  * named types, variables, functions

* scopes

  * global

  * local

    state

    * sp

    enter

    exit

* function

  * prelogue

  * epilogue

* implicit convertion

  * Conversion as if by assignment

    * assignment
    * scalar initialization
    * function-call expression
    * return statement

  * Default argument promotions

    * a variadic function

    ---

    * integer promotion

  * Usual arithmetic conversions

    * integers

      * integer promotion
      * common type

  * Lvalue conversion

    * The value remains the same, but loses its lvalue properties (the address may no longer be taken).

  * Array to pointer conversion

    * conversion to the non-lvalue pointer to its first element

  * Function to pointer conversion

    * a conversion to the non-lvalue pointer to the function designated by the expression

* cast

* struct

  layout

  empty struct is not allowed

* initialization is translated to assignments

* max alignment is 4

* calling convention

  * arguments
    * stack (4-aligned)
  * return
    * a0, a1
  * does not support passing/returning structure

* var arg functions

  ```
  f(int, int, ...)
  ```

  ```
  va_start(va_list ap, parm_n);
  T va_arg(va_list ap, T);
  va_end(va_list ap);
  ```

* expression evaluation

  * operands
    * global, static
    * local
    * temporary (stack)
  * result
    a0, a1

  * examples

    ```
    a + b

    f(a, b)

    a + f(b)

    f(a+b, c+d)

    f(a+g(b), c+g(d))

    &o
    o.i
    p->i
    p[i]
    *p
    ```

* optimization

  ```
  push a0
  pop a0
  ```

* statements do not change sp
