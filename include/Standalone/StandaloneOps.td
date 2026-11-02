#ifndef STANDALONE_OPS
#define STANDALONE_OPS

include "StandaloneDialect.td"
include "mlir/Interfaces/SideEffectInterfaces.td"
include "mlir/IR/CommonAttrConstraints.td"
include "mlir/IR/CommonTypeConstraints.td"

// One out-of-place stencil computation.
def Stencil_ApplyOp : Standalone_Op<"apply", [MemoryEffects<[MemRead, MemWrite]>]> {
    let summary = "Applies a stencil to an input memref and writes the result to an output memref.";

    let arguments = (ins
        AnyMemRef:$input,
        AnyMemRef:$output,
        IndexListArrayAttr:$offsets,
        F64Attr:$scale
    );

    let assemblyFormat = [{
        $input `,` $output
        `offsets` `=` $offsets
        `scale` `=` $scale
        attr-dict `:` type($input) `,` type($output)
    }];
}

// One in-place, lexicographically ordered stencil computation.
def Stencil_SweepOp : Standalone_Op<"sweep", [MemoryEffects<[MemRead, MemWrite]>]> {
    let summary = "Applies an in-place stencil in lexicographic order.";

    let arguments = (ins
        AnyMemRef:$memref,
        IndexListArrayAttr:$offsets,
        F64Attr:$divisor
    );

    let assemblyFormat = [{
        $memref
        `offsets` `=` $offsets
        `divisor` `=` $divisor
        attr-dict `:` type($memref)
    }];
}

#endif // STANDALONE_OPS