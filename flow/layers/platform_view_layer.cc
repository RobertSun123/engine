// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/flow/layers/platform_view_layer.h"

namespace flutter {

PlatformViewLayer::PlatformViewLayer(const SkPoint& offset,
                                     const SkSize& size,
                                     int64_t view_id)
    : offset_(offset), size_(size), view_id_(view_id) {}

void PlatformViewLayer::Preroll(PrerollContext* context,
                                const SkMatrix& matrix) {
  set_paint_bounds(SkRect::MakeXYWH(offset_.x(), offset_.y(), size_.width(),
                                    size_.height()));

  if (context->view_embedder == nullptr) {
    FML_LOG(ERROR) << "Trying to embed a platform view but the PrerollContext "
                      "does not support embedding";
    return;
  }
  matrix_ = matrix;
  context->has_platform_view = true;
  std::unique_ptr<EmbeddedViewParams> params =
      std::make_unique<EmbeddedViewParams>();
  params->offsetPixels =
      SkPoint::Make(matrix.getTranslateX(), matrix.getTranslateY());
  params->sizePoints = size_;
  params->mutatorsStack = context->mutators_stack;
  context->view_embedder->PrerollCompositeEmbeddedView(view_id_,
                                                       std::move(params));
}

void PlatformViewLayer::Paint(PaintContext& context) const {
  if (context.view_embedder == nullptr) {
    FML_LOG(ERROR) << "Trying to embed a platform view but the PaintContext "
                      "does not support embedding";
    return;
  }
  SkCanvas* canvas = context.view_embedder->CompositeEmbeddedView(view_id_);
  context.leaf_nodes_canvas = canvas;
  // Don't draw subsequent drawings on the area covered by the platform view,
  // since these drawings are render on an overlay on top of the platform view.
  // This prevent visible drawings on the background canvas when the platform
  // view has opacity.
  // context.background_canvas->resetMatrix();
  // context.background_canvas->clipRect(
  //     context.view_embedder->GetPlatformViewRect(view_id_),
  //     SkClipOp::kDifference);
  // context.background_canvas->setMatrix(matrix_);
}
}  // namespace flutter
