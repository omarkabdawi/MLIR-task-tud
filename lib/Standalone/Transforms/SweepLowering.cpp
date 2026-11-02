#include "Standalone/Transforms/Passes.h"
#include "Standalone/StandaloneOps.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/PatternMatch.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Transforms/GreedyPatternRewriteDriver.h"

#include "llvm/ADT/SmallVector.h"

namespace mlir {

#define GEN_PASS_DEF_LOWERSTENCILSWEEP
#include "Standalone/Transforms/Passes.h.inc"

} // namespace mlir

namespace {

class LowerStencilSweepPattern
    : public mlir::OpRewritePattern<mlir::stencil::SweepOp> {
public:
  using OpRewritePattern::OpRewritePattern;

  mlir::LogicalResult
  matchAndRewrite(mlir::stencil::SweepOp op,
                  mlir::PatternRewriter &rewriter) const override {
        mlir::Location loc = op.getLoc();

    auto memrefType = mlir::cast<mlir::MemRefType>(op.getMemref().getType());

    if (memrefType.getRank() != 2 ||
        !memrefType.getElementType().isF64()) {
      return rewriter.notifyMatchFailure(op, "expected a 2D f64 memref");
    }

    mlir::ArrayAttr offsets = op.getOffsetsAttr();

    if (offsets.size() != 9) {
      return rewriter.notifyMatchFailure(op, "expected exactly nine offsets");
    }

    llvm::SmallVector<std::pair<int64_t, int64_t>> offsetValues;

    for (mlir::Attribute attr : offsets) {
      auto pair = mlir::cast<mlir::ArrayAttr>(attr);

      if (pair.size() != 2) {
        return rewriter.notifyMatchFailure(op, "expected 2D offsets");
      }

      int64_t row = mlir::cast<mlir::IntegerAttr>(pair[0]).getInt();
      int64_t col = mlir::cast<mlir::IntegerAttr>(pair[1]).getInt();

      offsetValues.emplace_back(row, col);
    }

    mlir::Value memref = op.getMemref();

    mlir::Value one =
        rewriter.create<mlir::arith::ConstantIndexOp>(loc, 1);

    mlir::Value rows =
        rewriter.create<mlir::memref::DimOp>(loc, memref, 0);
    mlir::Value cols =
        rewriter.create<mlir::memref::DimOp>(loc, memref, 1);

    mlir::Value rowsMinusOne =
        rewriter.create<mlir::arith::SubIOp>(loc, rows, one);
    mlir::Value colsMinusOne =
        rewriter.create<mlir::arith::SubIOp>(loc, cols, one);

    auto outerLoop = rewriter.create<mlir::scf::ForOp>(
        loc, one, rowsMinusOne, one);

    {
      mlir::OpBuilder::InsertionGuard outerGuard(rewriter);
      rewriter.setInsertionPointToStart(outerLoop.getBody());

      mlir::Value i = outerLoop.getInductionVar();

      auto innerLoop = rewriter.create<mlir::scf::ForOp>(
          loc, one, colsMinusOne, one);

      {
        mlir::OpBuilder::InsertionGuard innerGuard(rewriter);
        rewriter.setInsertionPointToStart(innerLoop.getBody());

        mlir::Value j = innerLoop.getInductionVar();

        llvm::SmallVector<mlir::Value> values;

        for (auto [rowOffset, colOffset] : offsetValues) {
          mlir::Value row = i;
          mlir::Value col = j;

          if (rowOffset != 0) {
            auto offset = rewriter.create<mlir::arith::ConstantIndexOp>(
                loc, rowOffset);
            row = rewriter.create<mlir::arith::AddIOp>(
                loc, i, offset);
          }

          if (colOffset != 0) {
            auto offset = rewriter.create<mlir::arith::ConstantIndexOp>(
                loc, colOffset);
            col = rewriter.create<mlir::arith::AddIOp>(
                loc, j, offset);
          }

          values.push_back(rewriter.create<mlir::memref::LoadOp>(
              loc, memref, mlir::ValueRange{row, col}));
        }

        mlir::Value sum = values[0];

        for (size_t index = 1; index < values.size(); ++index) {
          sum = rewriter.create<mlir::arith::AddFOp>(
              loc, sum, values[index]);
        }

        double divisorValue = op.getDivisorAttr().getValueAsDouble();

        mlir::Value divisor = rewriter.create<mlir::arith::ConstantOp>(
            loc, rewriter.getF64FloatAttr(divisorValue));

        mlir::Value result = rewriter.create<mlir::arith::DivFOp>(
            loc, sum, divisor);

        rewriter.create<mlir::memref::StoreOp>(
            loc, result, memref, mlir::ValueRange{i, j});
      }
    }

    rewriter.eraseOp(op);
    return mlir::success();
  }
};
class LowerStencilSweep
    : public mlir::impl::LowerStencilSweepBase<LowerStencilSweep> {
public:
  using LowerStencilSweepBase::LowerStencilSweepBase;

  void getDependentDialects(mlir::DialectRegistry &registry) const override {
    registry.insert<
        mlir::arith::ArithDialect,
        mlir::memref::MemRefDialect,
        mlir::scf::SCFDialect>();
  }

  void runOnOperation() override {
    mlir::RewritePatternSet patterns(&getContext());
    patterns.add<LowerStencilSweepPattern>(&getContext());

    if (mlir::failed(
            mlir::applyPatternsGreedily(getOperation(), std::move(patterns)))) {
      signalPassFailure();
    }
  }
};
} // namespace