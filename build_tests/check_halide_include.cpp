#include <cstdint>

#include "Halide.h"
#include "HalideBuffer.h"

int main()
{
    Halide::Var x;
    Halide::Expr expr = Halide::cast<int>(x + 1);
    (void)expr;

    Halide::Runtime::Buffer<uint8_t> scratch(1, 1, 4);
    scratch.fill(0);
    return scratch.dimensions() == 3 ? 0 : 1;
}
