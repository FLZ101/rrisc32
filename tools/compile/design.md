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

  * int
  * pointer
  * array

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
    * stack
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
  * result (stack)

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
