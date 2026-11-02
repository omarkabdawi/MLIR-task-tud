include "mlir/Pass/PassBase.td"

def LowerStencilApply
    : Pass<"lower-stencil-apply", "mlir::ModuleOp"> {
  let summary = "Lower stencil.apply to memref subviews and linalg.generic";
}

def LowerStencilSweep
    : Pass<"lower-stencil-sweep", "mlir::ModuleOp"> {
  let summary = "Lower stencil.sweep to sequential SCF loops";
}