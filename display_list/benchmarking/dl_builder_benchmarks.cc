// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/benchmarking/benchmarking.h"
#include "flutter/display_list/display_list.h"
#include "flutter/display_list/testing/dl_test_snippets.h"
#include "flutter/display_list/utils/dl_receiver_utils.h"

namespace flutter {

DlOpReceiver& DisplayListBuilderBenchmarkAccessor(DisplayListBuilder& builder) {
  return builder.asReceiver();
}

namespace {

static std::vector<testing::DisplayListInvocationGroup> allRenderingOps =
    testing::CreateAllRenderingOps();

enum class DisplayListBuilderBenchmarkType {
  kDefault,
  kBounds,
  kRtree,
  kBoundsAndRtree,
};

static void InvokeAllRenderingOps(DisplayListBuilder& builder) {
  DlOpReceiver& receiver = DisplayListBuilderBenchmarkAccessor(builder);
  for (auto& group : allRenderingOps) {
    for (size_t i = 0; i < group.variants.size(); i++) {
      auto& invocation = group.variants[i];
      invocation.Invoke(receiver);
    }
  }
}

static void Complete(DisplayListBuilder& builder,
                     DisplayListBuilderBenchmarkType type) {
  auto display_list = builder.Build();
  switch (type) {
    case DisplayListBuilderBenchmarkType::kBounds:
      display_list->bounds();
      break;
    case DisplayListBuilderBenchmarkType::kRtree:
      display_list->rtree();
      break;
    case DisplayListBuilderBenchmarkType::kBoundsAndRtree:
      display_list->bounds();
      display_list->rtree();
      break;
    case DisplayListBuilderBenchmarkType::kDefault:
      break;
  }
}

bool NeedPrepareRTree(DisplayListBuilderBenchmarkType type) {
  return type == DisplayListBuilderBenchmarkType::kRtree ||
         type == DisplayListBuilderBenchmarkType::kBoundsAndRtree;
}

}  // namespace

static void BM_DisplayListBuilderDefault(benchmark::State& state,
                                         DisplayListBuilderBenchmarkType type) {
  bool prepare_rtree = NeedPrepareRTree(type);
  while (state.KeepRunning()) {
    DisplayListBuilder builder(prepare_rtree);
    InvokeAllRenderingOps(builder);
    Complete(builder, type);
  }
}

static void BM_DisplayListBuilderWithScaleAndTranslate(
    benchmark::State& state,
    DisplayListBuilderBenchmarkType type) {
  bool prepare_rtree = NeedPrepareRTree(type);
  while (state.KeepRunning()) {
    DisplayListBuilder builder(prepare_rtree);
    builder.Scale(3.5, 3.5);
    builder.Translate(10.3, 6.9);
    InvokeAllRenderingOps(builder);
    Complete(builder, type);
  }
}

static void BM_DisplayListBuilderWithPerspective(
    benchmark::State& state,
    DisplayListBuilderBenchmarkType type) {
  bool prepare_rtree = NeedPrepareRTree(type);
  while (state.KeepRunning()) {
    DisplayListBuilder builder(prepare_rtree);
    builder.TransformFullPerspective(0, 1, 0, 12, 1, 0, 0, 33, 3, 2, 5, 29, 0,
                                     0, 0, 12);
    InvokeAllRenderingOps(builder);
    Complete(builder, type);
  }
}

static void BM_DisplayListBuilderWithClipRect(
    benchmark::State& state,
    DisplayListBuilderBenchmarkType type) {
  SkRect clip_bounds = SkRect::MakeLTRB(6.5, 7.3, 90.2, 85.7);
  bool prepare_rtree = NeedPrepareRTree(type);
  while (state.KeepRunning()) {
    DisplayListBuilder builder(prepare_rtree);
    builder.ClipRect(clip_bounds, DlCanvas::ClipOp::kIntersect, true);
    InvokeAllRenderingOps(builder);
    Complete(builder, type);
  }
}

static void BM_DisplayListBuilderWithGlobalSaveLayer(
    benchmark::State& state,
    DisplayListBuilderBenchmarkType type) {
  bool prepare_rtree = NeedPrepareRTree(type);
  while (state.KeepRunning()) {
    DisplayListBuilder builder(prepare_rtree);
    builder.Scale(3.5, 3.5);
    builder.Translate(10.3, 6.9);
    builder.SaveLayer(nullptr, nullptr);
    builder.Translate(45.3, 27.9);
    DlOpReceiver& receiver = DisplayListBuilderBenchmarkAccessor(builder);
    for (auto& group : allRenderingOps) {
      for (size_t i = 0; i < group.variants.size(); i++) {
        auto& invocation = group.variants[i];
        invocation.Invoke(receiver);
      }
    }
    builder.Restore();
    Complete(builder, type);
  }
}

static void BM_DisplayListBuilderWithSaveLayer(
    benchmark::State& state,
    DisplayListBuilderBenchmarkType type) {
  bool prepare_rtree = NeedPrepareRTree(type);
  while (state.KeepRunning()) {
    DisplayListBuilder builder(prepare_rtree);
    DlOpReceiver& receiver = DisplayListBuilderBenchmarkAccessor(builder);
    for (auto& group : allRenderingOps) {
      for (size_t i = 0; i < group.variants.size(); i++) {
        auto& invocation = group.variants[i];
        builder.SaveLayer(nullptr, nullptr);
        invocation.Invoke(receiver);
        builder.Restore();
      }
    }
    Complete(builder, type);
  }
}

static void BM_DisplayListBuilderWithSaveLayerAndImageFilter(
    benchmark::State& state,
    DisplayListBuilderBenchmarkType type) {
  DlPaint layer_paint;
  layer_paint.setImageFilter(&testing::kTestBlurImageFilter1);
  SkRect layer_bounds = SkRect::MakeLTRB(6.5, 7.3, 35.2, 42.7);
  bool prepare_rtree = NeedPrepareRTree(type);
  while (state.KeepRunning()) {
    DisplayListBuilder builder(prepare_rtree);
    DlOpReceiver& receiver = DisplayListBuilderBenchmarkAccessor(builder);
    for (auto& group : allRenderingOps) {
      for (size_t i = 0; i < group.variants.size(); i++) {
        auto& invocation = group.variants[i];
        builder.SaveLayer(&layer_bounds, &layer_paint);
        invocation.Invoke(receiver);
        builder.Restore();
      }
    }
    Complete(builder, type);
  }
}

class TextFrameDispatcher : public IgnoreAttributeDispatchHelper,
                            public IgnoreClipDispatchHelper,
                            public IgnoreDrawDispatchHelper {
 public:
  TextFrameDispatcher(DisplayList::TextFrameIterator& iterator,
                      const DlMatrix& initial_matrix)
      : iterator_(iterator), matrix_(initial_matrix) {}

  void save() override { stack_.emplace_back(matrix_); }

  void saveLayer(const SkRect& bounds,
                 const SaveLayerOptions options,
                 const DlImageFilter* backdrop) override {
    save();
  }

  void restore() override {
    matrix_ = stack_.back();
    stack_.pop_back();
  }

  void translate(SkScalar tx, SkScalar ty) override {
    matrix_ = matrix_.Translate({tx, ty});
  }

  void scale(SkScalar sx, SkScalar sy) override {
    matrix_ = matrix_.Scale({sx, sy, 1.0f});
  }

  void rotate(SkScalar degrees) override {
    matrix_ = matrix_ * DlMatrix::MakeRotationZ(DlDegrees(degrees));
  }

  void skew(SkScalar sx, SkScalar sy) override {
    matrix_ = matrix_ * DlMatrix::MakeSkew(sx, sy);
  }

  // clang-format off
  // 2x3 2D affine subset of a 4x4 transform in row major order
  void transform2DAffine(SkScalar mxx, SkScalar mxy, SkScalar mxt,
                         SkScalar myx, SkScalar myy, SkScalar myt) override {
    matrix_ = matrix_ * DlMatrix::MakeColumn(
        mxx,  myx,  0.0f, 0.0f,
        mxy,  myy,  0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        mxt,  myt,  0.0f, 1.0f
    );
  }

  // full 4x4 transform in row major order
  void transformFullPerspective(
      SkScalar mxx, SkScalar mxy, SkScalar mxz, SkScalar mxt,
      SkScalar myx, SkScalar myy, SkScalar myz, SkScalar myt,
      SkScalar mzx, SkScalar mzy, SkScalar mzz, SkScalar mzt,
      SkScalar mwx, SkScalar mwy, SkScalar mwz, SkScalar mwt) override {
    matrix_ = matrix_ * DlMatrix::MakeColumn(
        mxx, myx, mzx, mwx,
        mxy, myy, mzy, mwy,
        mxz, myz, mzz, mwz,
        mxt, myt, mzt, mwt
    );
  }
  // clang-format on

  void transformReset() override { matrix_ = DlMatrix(); }

  void drawTextFrame(const std::shared_ptr<impeller::TextFrame>& text_frame,
                     SkScalar x,
                     SkScalar y) override {
    count_++;
    iterator_(*text_frame, matrix_, x, y);
  }

  void drawDisplayList(const sk_sp<DisplayList> display_list,
                       SkScalar opacity) override {
    save();
    display_list->Dispatch(*this);
    restore();
  }

  int GetTextFrameCount() { return count_; }

 private:
  DisplayList::TextFrameIterator& iterator_;
  DlMatrix matrix_;
  std::vector<DlMatrix> stack_;
  int count_ = 0;
};

static void BM_DisplayListIterateTextFrames(
    benchmark::State& state,
    DisplayListBuilderBenchmarkType type) {
  bool prepare_rtree = NeedPrepareRTree(type);
  DisplayListBuilder builder(prepare_rtree);
  InvokeAllRenderingOps(builder);
  auto display_list = builder.Build();
  DisplayList::TextFrameIterator iterator = [](const impeller::TextFrame&,  //
                                               const DlMatrix&,             //
                                               SkScalar x, SkScalar y) {};
  while (state.KeepRunning()) {
    display_list->IterateTextFrames(iterator, DlMatrix());
  }
}

static void BM_DisplayListIterateTextFrames2(
    benchmark::State& state,
    DisplayListBuilderBenchmarkType type) {
  bool prepare_rtree = NeedPrepareRTree(type);
  DisplayListBuilder builder(prepare_rtree);
  InvokeAllRenderingOps(builder);
  auto display_list = builder.Build();
  DisplayList::TextFrameIterator iterator = [](const impeller::TextFrame&,  //
                                               const DlMatrix&,             //
                                               SkScalar x, SkScalar y) {};
  while (state.KeepRunning()) {
    display_list->IterateTextFrames2(iterator, DlMatrix());
  }
}

static void BM_DisplayListDispatchTextFrames(
    benchmark::State& state,
    DisplayListBuilderBenchmarkType type) {
  bool prepare_rtree = NeedPrepareRTree(type);
  DisplayListBuilder builder(prepare_rtree);
  InvokeAllRenderingOps(builder);
  auto display_list = builder.Build();
  DisplayList::TextFrameIterator iterator = [](const impeller::TextFrame&,  //
                                               const DlMatrix&,             //
                                               SkScalar x, SkScalar y) {};
  TextFrameDispatcher dispatcher(iterator, DlMatrix());
  while (state.KeepRunning()) {
    display_list->Dispatch(dispatcher);
  }
}

BENCHMARK_CAPTURE(BM_DisplayListIterateTextFrames,
                  kDefault,
                  DisplayListBuilderBenchmarkType::kDefault)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_CAPTURE(BM_DisplayListIterateTextFrames2,
                  kDefault,
                  DisplayListBuilderBenchmarkType::kDefault)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_CAPTURE(BM_DisplayListDispatchTextFrames,
                  kDefault,
                  DisplayListBuilderBenchmarkType::kDefault)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_CAPTURE(BM_DisplayListBuilderDefault,
                  kDefault,
                  DisplayListBuilderBenchmarkType::kDefault)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_CAPTURE(BM_DisplayListBuilderDefault,
                  kBounds,
                  DisplayListBuilderBenchmarkType::kBounds)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_CAPTURE(BM_DisplayListBuilderDefault,
                  kRtree,
                  DisplayListBuilderBenchmarkType::kRtree)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_CAPTURE(BM_DisplayListBuilderDefault,
                  kBoundsAndRtree,
                  DisplayListBuilderBenchmarkType::kBoundsAndRtree)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_CAPTURE(BM_DisplayListBuilderWithScaleAndTranslate,
                  kDefault,
                  DisplayListBuilderBenchmarkType::kDefault)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_CAPTURE(BM_DisplayListBuilderWithScaleAndTranslate,
                  kBounds,
                  DisplayListBuilderBenchmarkType::kBounds)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_CAPTURE(BM_DisplayListBuilderWithScaleAndTranslate,
                  kRtree,
                  DisplayListBuilderBenchmarkType::kRtree)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_CAPTURE(BM_DisplayListBuilderWithScaleAndTranslate,
                  kBoundsAndRtree,
                  DisplayListBuilderBenchmarkType::kBoundsAndRtree)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_CAPTURE(BM_DisplayListBuilderWithPerspective,
                  kDefault,
                  DisplayListBuilderBenchmarkType::kDefault)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_CAPTURE(BM_DisplayListBuilderWithPerspective,
                  kBounds,
                  DisplayListBuilderBenchmarkType::kBounds)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_CAPTURE(BM_DisplayListBuilderWithPerspective,
                  kRtree,
                  DisplayListBuilderBenchmarkType::kRtree)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_CAPTURE(BM_DisplayListBuilderWithPerspective,
                  kBoundsAndRtree,
                  DisplayListBuilderBenchmarkType::kBoundsAndRtree)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_CAPTURE(BM_DisplayListBuilderWithClipRect,
                  kDefault,
                  DisplayListBuilderBenchmarkType::kDefault)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_CAPTURE(BM_DisplayListBuilderWithClipRect,
                  kBounds,
                  DisplayListBuilderBenchmarkType::kBounds)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_CAPTURE(BM_DisplayListBuilderWithClipRect,
                  kRtree,
                  DisplayListBuilderBenchmarkType::kRtree)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_CAPTURE(BM_DisplayListBuilderWithClipRect,
                  kBoundsAndRtree,
                  DisplayListBuilderBenchmarkType::kBoundsAndRtree)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_CAPTURE(BM_DisplayListBuilderWithGlobalSaveLayer,
                  kDefault,
                  DisplayListBuilderBenchmarkType::kDefault)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_CAPTURE(BM_DisplayListBuilderWithGlobalSaveLayer,
                  kBounds,
                  DisplayListBuilderBenchmarkType::kBounds)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_CAPTURE(BM_DisplayListBuilderWithGlobalSaveLayer,
                  kRtree,
                  DisplayListBuilderBenchmarkType::kRtree)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_CAPTURE(BM_DisplayListBuilderWithGlobalSaveLayer,
                  kBoundsAndRtree,
                  DisplayListBuilderBenchmarkType::kBoundsAndRtree)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_CAPTURE(BM_DisplayListBuilderWithSaveLayer,
                  kDefault,
                  DisplayListBuilderBenchmarkType::kDefault)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_CAPTURE(BM_DisplayListBuilderWithSaveLayer,
                  kBounds,
                  DisplayListBuilderBenchmarkType::kBounds)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_CAPTURE(BM_DisplayListBuilderWithSaveLayer,
                  kRtree,
                  DisplayListBuilderBenchmarkType::kRtree)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_CAPTURE(BM_DisplayListBuilderWithSaveLayer,
                  kBoundsAndRtree,
                  DisplayListBuilderBenchmarkType::kBoundsAndRtree)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_CAPTURE(BM_DisplayListBuilderWithSaveLayerAndImageFilter,
                  kDefault,
                  DisplayListBuilderBenchmarkType::kDefault)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_CAPTURE(BM_DisplayListBuilderWithSaveLayerAndImageFilter,
                  kBounds,
                  DisplayListBuilderBenchmarkType::kBounds)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_CAPTURE(BM_DisplayListBuilderWithSaveLayerAndImageFilter,
                  kRtree,
                  DisplayListBuilderBenchmarkType::kRtree)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_CAPTURE(BM_DisplayListBuilderWithSaveLayerAndImageFilter,
                  kBoundsAndRtree,
                  DisplayListBuilderBenchmarkType::kBoundsAndRtree)
    ->Unit(benchmark::kMicrosecond);

}  // namespace flutter
