void f()
{
    // Test if math works, C23:
    static_assert((2 + 2) % 3 == 1, "Whoa dude, you knew!");
    // Pre-C23 alternative:
    _Static_assert(2 + 2 * 2 == 6, "Lucky guess!?");
/*
      FuncCall:
        ID: static_assert
        ExprList:
          BinaryOp: ==
            BinaryOp: %
              BinaryOp: +
                Constant: int, 2
                Constant: int, 2
              Constant: int, 3
            Constant: int, 1
          Constant: string, "Whoa dude, you knew!"
      StaticAssert:
        BinaryOp: ==
          BinaryOp: +
            Constant: int, 2
            BinaryOp: *
              Constant: int, 2
              Constant: int, 2
          Constant: int, 6
        Constant: string, "Lucky guess!?"
      EmptyStatement:
*/
}
