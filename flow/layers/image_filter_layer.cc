// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/flow/layers/image_filter_layer.h"

namespace flutter {

ImageFilterLayer::ImageFilterLayer(sk_sp<SkImageFilter> filter)
    : filter_(std::move(filter)), render_count_(1) {}

void ImageFilterLayer::Preroll(PrerollContext* context,
                               const SkMatrix& matrix) {
  TRACE_EVENT0("flutter", "ImageFilterLayer::Preroll");

  Layer::AutoPrerollSaveLayerState save =
      Layer::AutoPrerollSaveLayerState::Create(context);

  child_paint_bounds_ = SkRect::MakeEmpty();
  PrerollChildren(context, matrix, &child_paint_bounds_);
  if (filter_) {
    const SkIRect filter_input_bounds = child_paint_bounds_.roundOut();
    SkIRect filter_output_bounds =
        filter_->filterBounds(filter_input_bounds, SkMatrix::I(),
                              SkImageFilter::kForward_MapDirection);
    set_paint_bounds(SkRect::Make(filter_output_bounds));
  } else {
    set_paint_bounds(child_paint_bounds_);
  }

  if (render_count_ >= kMinimumRendersBeforeCachingFilterLayer) {
    TryToPrepareRasterCache(context, this, matrix);
  } else {
    render_count_++;
    context->raster_cache->Prepare(context, GetCacheableChild(), matrix);
  }
}

void ImageFilterLayer::Paint(PaintContext& context) const {
  TRACE_EVENT0("flutter", "ImageFilterLayer::Paint");
  FML_DCHECK(needs_painting());

  if (context.raster_cache) {
    if (context.raster_cache->Draw(this, *context.leaf_nodes_canvas)) {
      FML_LOG(ERROR) << "Rendered filtered output from cache";
      return;
    }
    const SkMatrix& ctm = context.leaf_nodes_canvas->getTotalMatrix();
    sk_sp<SkImageFilter> transformed_filter = filter_->makeWithLocalMatrix(ctm);
    if (transformed_filter) {
      SkPaint paint;
      paint.setImageFilter(transformed_filter);

      if (context.raster_cache->Draw(GetCacheableChild(),
                                     *context.leaf_nodes_canvas, &paint)) {
        FML_LOG(ERROR) << "Filtered from cached child";
        return;
      }
    }
  }

  FML_LOG(ERROR) << "Applying filter to child on the fly";
  SkPaint paint;
  paint.setImageFilter(filter_);

  // Normally a save_layer is sized to the current layer bounds, but in this
  // case the bounds of the child may not be the same as the filtered version
  // so we use the child_paint_bounds_ which were snapshotted from the
  // Preroll on the children before we adjusted them based on the filter.
  Layer::AutoSaveLayer save_layer =
      Layer::AutoSaveLayer::Create(context, child_paint_bounds_, &paint);
  PaintChildren(context);
}

}  // namespace flutter
