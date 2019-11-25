// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/flow/layers/container_layer.h"

namespace flutter {

ContainerLayer::ContainerLayer()
    : child_needs_screen_readback_(false), renders_to_save_layer_(false) {}

ContainerLayer::~ContainerLayer() = default;

void ContainerLayer::Add(std::shared_ptr<Layer> layer) {
  Layer* the_layer = layer.get();
  the_layer->set_parent(this);
  layers_.push_back(std::move(layer));
  if (the_layer->tree_reads_surface()) {
    UpdateChildReadback(the_layer);
  }
}

void ContainerLayer::ClearChildren() {
  layers_.clear();
  if (child_needs_screen_readback_) {
    child_needs_screen_readback_ = false;
    UpdateTreeReadsSurface();
  }
}

void ContainerLayer::set_renders_to_save_layer(bool protects) {
  if (renders_to_save_layer_ != protects) {
    renders_to_save_layer_ = protects;
    UpdateTreeReadsSurface();
  }
}

void ContainerLayer::UpdateChildReadback(const Layer* layer) {
  if (child_needs_screen_readback_ == layer->tree_reads_surface()) {
    return;
  }
  if (child_needs_screen_readback_) {
    // Transitioning to false, but only if there are no other children
    // that read from the surface.
    for (auto& child : layers_) {
      if (child->tree_reads_surface()) {
        return;
      }
    }
    child_needs_screen_readback_ = false;
  } else {
    child_needs_screen_readback_ = true;
  }
  UpdateTreeReadsSurface();
}

bool ContainerLayer::ComputeTreeReadsSurface() const {
  return layer_reads_surface() ||
         (!renders_to_save_layer_ && child_needs_screen_readback_);
}

void ContainerLayer::Preroll(PrerollContext* context, const SkMatrix& matrix) {
  TRACE_EVENT0("flutter", "ContainerLayer::Preroll");

  SkRect child_paint_bounds = SkRect::MakeEmpty();
  PrerollChildren(context, matrix, &child_paint_bounds);
  set_paint_bounds(child_paint_bounds);
}

void ContainerLayer::PrerollChildren(PrerollContext* context,
                                     const SkMatrix& child_matrix,
                                     SkRect* child_paint_bounds) {
  // Platform views have no children, so context->has_platform_view should
  // always be false.
  FML_DCHECK(!context->has_platform_view);
  bool child_has_platform_view = false;
  for (auto& layer : layers_) {
    // Reset context->has_platform_view to false so that layers aren't treated
    // as if they have a platform view based on one being previously found in a
    // sibling tree.
    context->has_platform_view = false;

    layer->Preroll(context, child_matrix);

    if (layer->needs_system_composite()) {
      set_needs_system_composite(true);
    }
    child_paint_bounds->join(layer->paint_bounds());

    child_has_platform_view =
        child_has_platform_view || context->has_platform_view;
  }

  context->has_platform_view = child_has_platform_view;
}

void ContainerLayer::PaintChildren(PaintContext& context) const {
  FML_DCHECK(needs_painting());

  // Intentionally not tracing here as there should be no self-time
  // and the trace event on this common function has a small overhead.
  for (auto& layer : layers_) {
    if (layer->needs_painting()) {
      layer->Paint(context);
    }
  }
}

#if defined(OS_FUCHSIA)

void ContainerLayer::UpdateScene(SceneUpdateContext& context) {
  UpdateSceneChildren(context);
}

void ContainerLayer::UpdateSceneChildren(SceneUpdateContext& context) {
  FML_DCHECK(needs_system_composite());

  // Paint all of the layers which need to be drawn into the container.
  // These may be flattened down to a containing
  for (auto& layer : layers_) {
    if (layer->needs_system_composite()) {
      layer->UpdateScene(context);
    }
  }
}

#endif  // defined(OS_FUCHSIA)

}  // namespace flutter
