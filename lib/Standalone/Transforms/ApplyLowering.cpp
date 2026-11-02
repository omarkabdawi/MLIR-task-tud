
#include "Standalone/Transforms/Passes.h"
#include "Standalone/StandaloneOps.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Linalg/IR/Linalg.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/IR/AffineMap.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/PatternMatch.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Transforms/GreedyPatternRewriteDriver.h"

#include "llvm/ADT/SmallVector.h"

namespace mlir {

#define GEN_PASS_DEF_LOWERSTENCILAPPLY
#include "Standalone/Transforms/Passes.h.inc"

} // namespace mlir

namespace {

class LowerStencilApplyPattern
    : public mlir::OpRewritePattern<mlir::stencil::ApplyOp> {
public:
  using OpRewritePattern::OpRewritePattern;

  mlir::LogicalResult
  matchAndRewrite(mlir::stencil::ApplyOp op,
                  mlir::PatternRewriter &rewriter) const override {

    mlir::Location loc = op.getLoc();

    auto inputType = mlir::cast<mlir::MemRefType>(op.getInput().getType());
    auto outputType = mlir::cast<mlir::MemRefType>(op.getOutput().getType());

    if (inputType.getRank() != 2 || outputType.getRank() != 2 ||
        !inputType.getElementType().isF64() ||
        !outputType.getElementType().isF64())
      return rewriter.notifyMatchFailure(op, "expected 2D f64 memrefs");

    mlir::ArrayAttr offsets = op.getOffsetsAttr();

    if (offsets.empty())
      return rewriter.notifyMatchFailure(op, "stencil has no offsets");

    llvm::SmallVector<std::pair<int64_t, int64_t>> offsetValues;

    for (mlir::Attribute attr : offsets) {
      auto pair = mlir::cast<mlir::ArrayAttr>(attr);

      if (pair.size() != 2)
        return rewriter.notifyMatchFailure(op, "expected 2D offsets");

      int64_t row = mlir::cast<mlir::IntegerAttr>(pair[0]).getInt();
      int64_t col = mlir::cast<mlir::IntegerAttr>(pair[1]).getInt();

      offsetValues.emplace_back(row, col);
    }

    mlir::Value input = op.getInput();
    mlir::Value output = op.getOutput();

    mlir::Value two =
        rewriter.create<mlir::arith::ConstantIndexOp>(loc, 2);

    llvm::SmallVector<mlir::OpFoldResult> sizes;

    for (int64_t dim = 0; dim < 2; ++dim) {
      mlir::Value dimension =
          rewriter.create<mlir::memref::DimOp>(loc, output, dim);

      mlir::Value interiorSize =
          rewriter.create<mlir::arith::SubIOp>(loc, dimension, two);

      sizes.push_back(interiorSize);
    }

    llvm::SmallVector<mlir::OpFoldResult> strides = {
        rewriter.getIndexAttr(1), rewriter.getIndexAttr(1)};

    llvm::SmallVector<mlir::OpFoldResult> outputOffsets = {
        rewriter.getIndexAttr(1), rewriter.getIndexAttr(1)};

    auto outputSubview = mlir::memref::SubViewOp::create(
        rewriter, loc, output, outputOffsets, sizes, strides);

    llvm::SmallVector<mlir::Value> inputSubviews;

    for (auto [row, col] : offsetValues) {
      llvm::SmallVector<mlir::OpFoldResult> inputOffsets = {
          rewriter.getIndexAttr(1 + row),
          rewriter.getIndexAttr(1 + col)};

      auto inputSubview = mlir::memref::SubViewOp::create(
          rewriter, loc, input, inputOffsets, sizes, strides);

      inputSubviews.push_back(inputSubview.getResult());
    }

    mlir::AffineMap identity =
        mlir::AffineMap::getMultiDimIdentityMap(2, rewriter.getContext());

    llvm::SmallVector<mlir::AffineMap> indexingMaps(
        inputSubviews.size() + 1, identity);

    llvm::SmallVector<mlir::utils::IteratorType> iteratorTypes(
        2, mlir::utils::IteratorType::parallel);

    double scaleValue = op.getScaleAttr().getValueAsDouble();

    llvm::SmallVector<mlir::Value> outputBuffers = {
        outputSubview.getResult()};

    mlir::linalg::GenericOp::create(
        rewriter, loc,
        mlir::ValueRange(inputSubviews),
        mlir::ValueRange(outputBuffers),
        indexingMaps,
        iteratorTypes,
        [&](mlir::OpBuilder &builder, mlir::Location bodyLoc,
            mlir::ValueRange blockArgs) {

          mlir::Value sum = blockArgs[0];

          for (size_t i = 1; i < offsetValues.size(); ++i) {
            sum = builder.create<mlir::arith::AddFOp>(
                bodyLoc, sum, blockArgs[i]);
          }

          auto scale = builder.create<mlir::arith::ConstantOp>(
              bodyLoc, builder.getF64FloatAttr(scaleValue));

          mlir::Value result = builder.create<mlir::arith::MulFOp>(
              bodyLoc, sum, scale);

          mlir::ImplicitLocOpBuilder implicitBuilder(bodyLoc, builder);
          mlir::linalg::YieldOp::create(implicitBuilder, result);
        });

    rewriter.eraseOp(op);
    return mlir::success();
  }
};

class LowerStencilApply
    : public mlir::impl::LowerStencilApplyBase<LowerStencilApply> {
public:
  using LowerStencilApplyBase::LowerStencilApplyBase;

  void getDependentDialects(mlir::DialectRegistry &registry) const override {
    registry.insert<
        mlir::arith::ArithDialect,
        mlir::linalg::LinalgDialect,
        mlir::memref::MemRefDialect>();
}

  void runOnOperation() override {
        mlir::RewritePatternSet patterns(&getContext());
         patterns.add<LowerStencilApplyPattern>(&getContext());
    if (mlir::failed(
            mlir::applyPatternsGreedily(getOperation(), std::move(patterns)))) {
      signalPassFailure();
    }
  }
};

} // namespace
