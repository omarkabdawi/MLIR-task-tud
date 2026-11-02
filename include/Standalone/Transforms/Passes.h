#ifndef STENCIL_PASSES_H
#define STENCIL_PASSES_H

#include "mlir/Pass/Pass.h"

namespace mlir {

#define GEN_PASS_DECL
#include "Standalone/Transforms/Passes.h.inc"

#define GEN_PASS_REGISTRATION
#include "Standalone/Transforms/Passes.h.inc"

} // namespace mlir

#endif