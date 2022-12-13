// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/display_list/display_list_builder.h"

#include "flutter/display_list/display_list.h"
#include "flutter/display_list/display_list_blend_mode.h"
#include "flutter/display_list/display_list_canvas_dispatcher.h"
#include "flutter/display_list/display_list_color_source.h"
#include "flutter/display_list/display_list_ops.h"

namespace flutter {

#define DL_BUILDER_PAGE 4096

// CopyV(dst, src,n, src,n, ...) copies any number of typed srcs into dst.
static void CopyV(void* dst) {}

template <typename S, typename... Rest>
static void CopyV(void* dst, const S* src, int n, Rest&&... rest) {
  FML_DCHECK(((uintptr_t)dst & (alignof(S) - 1)) == 0)
      << "Expected " << dst << " to be aligned for at least " << alignof(S)
      << " bytes.";
  sk_careful_memcpy(dst, src, n * sizeof(S));
  CopyV(SkTAddOffset<void>(dst, n * sizeof(S)), std::forward<Rest>(rest)...);
}

template <typename T, typename... Args>
void* DisplayListBuilder::Push(size_t pod, int op_inc, Args&&... args) {
  size_t size = SkAlignPtr(sizeof(T) + pod);
  FML_DCHECK(size < (1 << 24));
  if (used_ + size > allocated_) {
    static_assert(SkIsPow2(DL_BUILDER_PAGE),
                  "This math needs updating for non-pow2.");
    // Next greater multiple of DL_BUILDER_PAGE.
    allocated_ = (used_ + size + DL_BUILDER_PAGE) & ~(DL_BUILDER_PAGE - 1);
    storage_.realloc(allocated_);
    FML_DCHECK(storage_.get());
    memset(storage_.get() + used_, 0, allocated_ - used_);
  }
  FML_DCHECK(used_ + size <= allocated_);
  auto op = reinterpret_cast<T*>(storage_.get() + used_);
  used_ += size;
  new (op) T{std::forward<Args>(args)...};
  op->type = T::kType;
  op->size = size;
  op_count_ += op_inc;
  return op + 1;
}

sk_sp<DisplayList> DisplayListBuilder::Build() {
  while (layer_stack_.size() > 1) {
    restore();
  }
  size_t bytes = used_;
  int count = op_count_;
  size_t nested_bytes = nested_bytes_;
  int nested_count = nested_op_count_;
  used_ = allocated_ = op_count_ = 0;
  nested_bytes_ = nested_op_count_ = 0;
  storage_.realloc(bytes);
  bool compatible = layer_stack_.back().is_group_opacity_compatible();
  return sk_sp<DisplayList>(
      new DisplayList(storage_.release(), bytes, count, nested_bytes,
                      nested_count, bounds(), cull_rect_, compatible, rtree()));
}

DisplayListBuilder::DisplayListBuilder(const SkRect& cull_rect,
                                       bool need_produce_rtree)
    : cull_rect_(cull_rect), need_produce_rtree_(need_produce_rtree) {
  layer_stack_.emplace_back(SkM44(), SkMatrix::I(), cull_rect);
  current_layer_ = &layer_stack_.back();
}

DisplayListBuilder::~DisplayListBuilder() {
  uint8_t* ptr = storage_.get();
  if (ptr) {
    DisplayList::DisposeOps(ptr, ptr + used_);
  }
}

void DisplayListBuilder::onSetAntiAlias(bool aa) {
  current_.setAntiAlias(aa);
  Push<SetAntiAliasOp>(0, 0, aa);
}
void DisplayListBuilder::onSetDither(bool dither) {
  current_.setDither(dither);
  Push<SetDitherOp>(0, 0, dither);
}
void DisplayListBuilder::onSetInvertColors(bool invert) {
  current_.setInvertColors(invert);
  Push<SetInvertColorsOp>(0, 0, invert);
  UpdateCurrentOpacityCompatibility();
}
void DisplayListBuilder::onSetStrokeCap(DlStrokeCap cap) {
  current_.setStrokeCap(cap);
  Push<SetStrokeCapOp>(0, 0, cap);
}
void DisplayListBuilder::onSetStrokeJoin(DlStrokeJoin join) {
  current_.setStrokeJoin(join);
  Push<SetStrokeJoinOp>(0, 0, join);
}
void DisplayListBuilder::onSetStyle(DlDrawStyle style) {
  current_.setDrawStyle(style);
  Push<SetStyleOp>(0, 0, style);
}
void DisplayListBuilder::onSetStrokeWidth(float width) {
  current_.setStrokeWidth(width);
  Push<SetStrokeWidthOp>(0, 0, width);
}
void DisplayListBuilder::onSetStrokeMiter(float limit) {
  current_.setStrokeMiter(limit);
  Push<SetStrokeMiterOp>(0, 0, limit);
}
void DisplayListBuilder::onSetColor(DlColor color) {
  current_.setColor(color);
  Push<SetColorOp>(0, 0, color);
}
void DisplayListBuilder::onSetBlendMode(DlBlendMode mode) {
  current_blender_ = nullptr;
  current_.setBlendMode(mode);
  Push<SetBlendModeOp>(0, 0, mode);
  UpdateCurrentOpacityCompatibility();
}

void DisplayListBuilder::onSetBlender(sk_sp<SkBlender> blender) {
  // setBlender(nullptr) should be redirected to setBlendMode(SrcOver)
  // by the set method, if not then the following is inefficient but works
  FML_DCHECK(blender);
  SkPaint p;
  p.setBlender(blender);
  if (p.asBlendMode()) {
    setBlendMode(ToDl(p.asBlendMode().value()));
  } else {
    // |current_blender_| supersedes any value of |current_blend_mode_|
    (current_blender_ = blender)  //
        ? Push<SetBlenderOp>(0, 0, std::move(blender))
        : Push<ClearBlenderOp>(0, 0);
    UpdateCurrentOpacityCompatibility();
  }
}

void DisplayListBuilder::onSetColorSource(const DlColorSource* source) {
  if (source == nullptr) {
    current_.setColorSource(nullptr);
    Push<ClearColorSourceOp>(0, 0);
  } else {
    current_.setColorSource(source->shared());
    switch (source->type()) {
      case DlColorSourceType::kColor: {
        const DlColorColorSource* color_source = source->asColor();
        current_.setColorSource(nullptr);
        setColor(color_source->color());
        break;
      }
      case DlColorSourceType::kImage: {
        const DlImageColorSource* image_source = source->asImage();
        FML_DCHECK(image_source);
        Push<SetImageColorSourceOp>(0, 0, image_source);
        break;
      }
      case DlColorSourceType::kLinearGradient: {
        const DlLinearGradientColorSource* linear = source->asLinearGradient();
        FML_DCHECK(linear);
        void* pod = Push<SetPodColorSourceOp>(linear->size(), 0);
        new (pod) DlLinearGradientColorSource(linear);
        break;
      }
      case DlColorSourceType::kRadialGradient: {
        const DlRadialGradientColorSource* radial = source->asRadialGradient();
        FML_DCHECK(radial);
        void* pod = Push<SetPodColorSourceOp>(radial->size(), 0);
        new (pod) DlRadialGradientColorSource(radial);
        break;
      }
      case DlColorSourceType::kConicalGradient: {
        const DlConicalGradientColorSource* conical =
            source->asConicalGradient();
        FML_DCHECK(conical);
        void* pod = Push<SetPodColorSourceOp>(conical->size(), 0);
        new (pod) DlConicalGradientColorSource(conical);
        break;
      }
      case DlColorSourceType::kSweepGradient: {
        const DlSweepGradientColorSource* sweep = source->asSweepGradient();
        FML_DCHECK(sweep);
        void* pod = Push<SetPodColorSourceOp>(sweep->size(), 0);
        new (pod) DlSweepGradientColorSource(sweep);
        break;
      }
      case DlColorSourceType::kRuntimeEffect: {
        const DlRuntimeEffectColorSource* effect = source->asRuntimeEffect();
        FML_DCHECK(effect);
        Push<SetRuntimeEffectColorSourceOp>(0, 0, effect);
        break;
      }
      case DlColorSourceType::kUnknown:
        Push<SetSkColorSourceOp>(0, 0, source->skia_object());
        break;
    }
  }
}
void DisplayListBuilder::onSetImageFilter(const DlImageFilter* filter) {
  if (filter == nullptr) {
    current_.setImageFilter(nullptr);
    Push<ClearImageFilterOp>(0, 0);
  } else {
    current_.setImageFilter(filter->shared());
    switch (filter->type()) {
      case DlImageFilterType::kBlur: {
        const DlBlurImageFilter* blur_filter = filter->asBlur();
        FML_DCHECK(blur_filter);
        void* pod = Push<SetPodImageFilterOp>(blur_filter->size(), 0);
        new (pod) DlBlurImageFilter(blur_filter);
        break;
      }
      case DlImageFilterType::kDilate: {
        const DlDilateImageFilter* dilate_filter = filter->asDilate();
        FML_DCHECK(dilate_filter);
        void* pod = Push<SetPodImageFilterOp>(dilate_filter->size(), 0);
        new (pod) DlDilateImageFilter(dilate_filter);
        break;
      }
      case DlImageFilterType::kErode: {
        const DlErodeImageFilter* erode_filter = filter->asErode();
        FML_DCHECK(erode_filter);
        void* pod = Push<SetPodImageFilterOp>(erode_filter->size(), 0);
        new (pod) DlErodeImageFilter(erode_filter);
        break;
      }
      case DlImageFilterType::kMatrix: {
        const DlMatrixImageFilter* matrix_filter = filter->asMatrix();
        FML_DCHECK(matrix_filter);
        void* pod = Push<SetPodImageFilterOp>(matrix_filter->size(), 0);
        new (pod) DlMatrixImageFilter(matrix_filter);
        break;
      }
      case DlImageFilterType::kComposeFilter:
      case DlImageFilterType::kLocalMatrixFilter:
      case DlImageFilterType::kColorFilter: {
        Push<SetSharedImageFilterOp>(0, 0, filter);
        break;
      }
      case DlImageFilterType::kUnknown: {
        Push<SetSkImageFilterOp>(0, 0, filter->skia_object());
        break;
      }
    }
  }
}
void DisplayListBuilder::onSetColorFilter(const DlColorFilter* filter) {
  if (filter == nullptr) {
    current_.setColorFilter(nullptr);
    Push<ClearColorFilterOp>(0, 0);
  } else {
    current_.setColorFilter(filter->shared());
    switch (filter->type()) {
      case DlColorFilterType::kBlend: {
        const DlBlendColorFilter* blend_filter = filter->asBlend();
        FML_DCHECK(blend_filter);
        void* pod = Push<SetPodColorFilterOp>(blend_filter->size(), 0);
        new (pod) DlBlendColorFilter(blend_filter);
        break;
      }
      case DlColorFilterType::kMatrix: {
        const DlMatrixColorFilter* matrix_filter = filter->asMatrix();
        FML_DCHECK(matrix_filter);
        void* pod = Push<SetPodColorFilterOp>(matrix_filter->size(), 0);
        new (pod) DlMatrixColorFilter(matrix_filter);
        break;
      }
      case DlColorFilterType::kSrgbToLinearGamma: {
        void* pod = Push<SetPodColorFilterOp>(filter->size(), 0);
        new (pod) DlSrgbToLinearGammaColorFilter();
        break;
      }
      case DlColorFilterType::kLinearToSrgbGamma: {
        void* pod = Push<SetPodColorFilterOp>(filter->size(), 0);
        new (pod) DlLinearToSrgbGammaColorFilter();
        break;
      }
      case DlColorFilterType::kUnknown: {
        Push<SetSkColorFilterOp>(0, 0, filter->skia_object());
        break;
      }
    }
  }
  UpdateCurrentOpacityCompatibility();
}
void DisplayListBuilder::onSetPathEffect(const DlPathEffect* effect) {
  if (effect == nullptr) {
    current_.setPathEffect(nullptr);
    Push<ClearPathEffectOp>(0, 0);
  } else {
    current_.setPathEffect(effect->shared());
    switch (effect->type()) {
      case DlPathEffectType::kDash: {
        const DlDashPathEffect* dash_effect = effect->asDash();
        void* pod = Push<SetPodPathEffectOp>(dash_effect->size(), 0);
        new (pod) DlDashPathEffect(dash_effect);
        break;
      }
      case DlPathEffectType::kUnknown: {
        Push<SetSkPathEffectOp>(0, 0, effect->skia_object());
        break;
      }
    }
  }
}
void DisplayListBuilder::onSetMaskFilter(const DlMaskFilter* filter) {
  if (filter == nullptr) {
    current_.setMaskFilter(nullptr);
    Push<ClearMaskFilterOp>(0, 0);
  } else {
    current_.setMaskFilter(filter->shared());
    switch (filter->type()) {
      case DlMaskFilterType::kBlur: {
        const DlBlurMaskFilter* blur_filter = filter->asBlur();
        FML_DCHECK(blur_filter);
        void* pod = Push<SetPodMaskFilterOp>(blur_filter->size(), 0);
        new (pod) DlBlurMaskFilter(blur_filter);
        break;
      }
      case DlMaskFilterType::kUnknown:
        Push<SetSkMaskFilterOp>(0, 0, filter->skia_object());
        break;
    }
  }
}

void DisplayListBuilder::setAttributesFromDlPaint(
    const DlPaint& paint,
    const DisplayListAttributeFlags flags) {
  if (flags.applies_anti_alias()) {
    setAntiAlias(paint.isAntiAlias());
  }
  if (flags.applies_dither()) {
    setDither(paint.isDither());
  }
  if (flags.applies_alpha_or_color()) {
    setColor(paint.getColor().argb);
  }
  if (flags.applies_blend()) {
    setBlendMode(paint.getBlendMode());
  }
  if (flags.applies_style()) {
    setStyle(paint.getDrawStyle());
  }
  if (flags.is_stroked(paint.getDrawStyle())) {
    setStrokeWidth(paint.getStrokeWidth());
    setStrokeMiter(paint.getStrokeMiter());
    setStrokeCap(paint.getStrokeCap());
    setStrokeJoin(paint.getStrokeJoin());
  }
  if (flags.applies_shader()) {
    setColorSource(paint.getColorSource().get());
  }
  if (flags.applies_color_filter()) {
    setInvertColors(paint.isInvertColors());
    setColorFilter(paint.getColorFilter().get());
  }
  if (flags.applies_image_filter()) {
    setImageFilter(paint.getImageFilter().get());
  }
  if (flags.applies_path_effect()) {
    setPathEffect(paint.getPathEffect().get());
  }
  if (flags.applies_mask_filter()) {
    setMaskFilter(paint.getMaskFilter().get());
  }
}

void DisplayListBuilder::setAttributesFromPaint(
    const SkPaint& paint,
    const DisplayListAttributeFlags flags) {
  if (flags.applies_anti_alias()) {
    setAntiAlias(paint.isAntiAlias());
  }
  if (flags.applies_dither()) {
    setDither(paint.isDither());
  }
  if (flags.applies_alpha_or_color()) {
    setColor(paint.getColor());
  }
  if (flags.applies_blend()) {
    std::optional<SkBlendMode> mode_optional = paint.asBlendMode();
    if (mode_optional) {
      setBlendMode(ToDl(mode_optional.value()));
    } else {
      setBlender(sk_ref_sp(paint.getBlender()));
    }
  }
  if (flags.applies_style()) {
    setStyle(ToDl(paint.getStyle()));
  }
  if (flags.is_stroked(ToDl(paint.getStyle()))) {
    setStrokeWidth(paint.getStrokeWidth());
    setStrokeMiter(paint.getStrokeMiter());
    setStrokeCap(ToDl(paint.getStrokeCap()));
    setStrokeJoin(ToDl(paint.getStrokeJoin()));
  }
  if (flags.applies_shader()) {
    SkShader* shader = paint.getShader();
    setColorSource(DlColorSource::From(shader).get());
  }
  if (flags.applies_color_filter()) {
    // invert colors is a Flutter::Paint thing, not an SkPaint thing
    // we must clear it because it is a second potential color filter
    // that is composed with the paint's color filter.
    setInvertColors(false);
    SkColorFilter* color_filter = paint.getColorFilter();
    setColorFilter(DlColorFilter::From(color_filter).get());
  }
  if (flags.applies_image_filter()) {
    setImageFilter(DlImageFilter::From(paint.getImageFilter()).get());
  }
  if (flags.applies_path_effect()) {
    SkPathEffect* path_effect = paint.getPathEffect();
    setPathEffect(DlPathEffect::From(path_effect).get());
  }
  if (flags.applies_mask_filter()) {
    SkMaskFilter* mask_filter = paint.getMaskFilter();
    setMaskFilter(DlMaskFilter::From(mask_filter).get());
  }
}

void DisplayListBuilder::checkForDeferredSave() {
  if (current_layer_->has_deferred_save_op_) {
    Push<SaveOp>(0, 1);
    current_layer_->has_deferred_save_op_ = false;
  }
}

void DisplayListBuilder::save() {
  layer_stack_.emplace_back(current_layer_);
  current_layer_ = &layer_stack_.back();
  current_layer_->has_deferred_save_op_ = true;
  accumulator()->save();
}

void DisplayListBuilder::restore() {
  if (layer_stack_.size() > 1) {
    if (!current_layer_->has_deferred_save_op_) {
      Push<RestoreOp>(0, 1);
    }
    // Grab the current layer info before we push the restore
    // on the stack.
    LayerInfo layer_info = layer_stack_.back();

    layer_stack_.pop_back();
    current_layer_ = &layer_stack_.back();
    bool is_unbounded = layer_info.is_unbounded();

    // Before we pop_back we will get the current layer bounds from the
    // current accumulator and adjust ot as required based on the filter.
    std::shared_ptr<const DlImageFilter> filter = layer_info.filter();
    const SkRect* clip = &current_layer_->clip_bounds();
    if (filter) {
      if (!accumulator()->restore(
              [filter = filter, matrix = getTransform()](const SkRect& input,
                                                         SkRect& output) {
                SkIRect output_bounds;
                bool ret = filter->map_device_bounds(input.roundOut(), matrix,
                                                     output_bounds);
                output.set(output_bounds);
                return ret;
              },
              clip)) {
        is_unbounded = true;
      }
    } else {
      accumulator()->restore();
    }

    if (is_unbounded) {
      AccumulateUnbounded();
    }

    if (layer_info.has_layer()) {
      if (layer_info.is_group_opacity_compatible()) {
        // We are now going to go back and modify the matching saveLayer
        // call to add the option indicating it can distribute an opacity
        // value to its children.
        //
        // Note that this operation cannot and does not change the size
        // or structure of the SaveLayerOp record. It only sets an option
        // flag on an existing field.
        //
        // Note that these kinds of modification operations on data already
        // in the DisplayList are only allowed *during* the build phase.
        // Once built, the DisplayList records must remain read only to
        // ensure consistency of rendering and |Equals()| behavior.
        SaveLayerOp* op = reinterpret_cast<SaveLayerOp*>(
            storage_.get() + layer_info.save_layer_offset());
        op->options = op->options.with_can_distribute_opacity();
      }
    } else {
      // For regular save() ops there was no protecting layer so we have to
      // accumulate the values into the enclosing layer.
      if (layer_info.cannot_inherit_opacity()) {
        current_layer_->mark_incompatible();
      } else if (layer_info.has_compatible_op()) {
        current_layer_->add_compatible_op();
      }
    }
  }
}
void DisplayListBuilder::restoreToCount(int restore_count) {
  FML_DCHECK(restore_count <= getSaveCount());
  while (restore_count < getSaveCount() && getSaveCount() > 1) {
    restore();
  }
}
void DisplayListBuilder::saveLayer(const SkRect* bounds,
                                   const SaveLayerOptions in_options,
                                   const DlImageFilter* backdrop) {
  SaveLayerOptions options = in_options.without_optimizations();
  size_t save_layer_offset = used_;
  if (backdrop) {
    bounds  //
        ? Push<SaveLayerBackdropBoundsOp>(0, 1, *bounds, options, backdrop)
        : Push<SaveLayerBackdropOp>(0, 1, options, backdrop);
  } else {
    bounds  //
        ? Push<SaveLayerBoundsOp>(0, 1, *bounds, options)
        : Push<SaveLayerOp>(0, 1, options);
  }
  CheckLayerOpacityCompatibility(options.renders_with_attributes());

  if (options.renders_with_attributes()) {
    // The actual flood of the outer layer clip will occur after the
    // (eventual) corresponding restore is called, but rather than
    // remember this information in the LayerInfo until the restore
    // method is processed, we just mark the unbounded state up front.
    if (!paint_nops_on_transparency()) {
      // We will fill the clip of the outer layer when we restore
      AccumulateUnbounded();
    }
    layer_stack_.emplace_back(current_layer_, save_layer_offset, true,
                              current_.getImageFilter());
  } else {
    layer_stack_.emplace_back(current_layer_, save_layer_offset, true, nullptr);
  }
  accumulator()->save();
  current_layer_ = &layer_stack_.back();
  if (options.renders_with_attributes()) {
    // |current_opacity_compatibility_| does not take an ImageFilter into
    // account because an individual primitive with an ImageFilter can apply
    // opacity on top of it. But, if the layer is applying the ImageFilter
    // then it cannot pass the opacity on.
    if (!current_opacity_compatibility_ ||
        current_.getImageFilter() != nullptr) {
      UpdateLayerOpacityCompatibility(false);
    }
  }

  // Even though Skia claims that the bounds are only a hint, they actually
  // use them as the temporary layer bounds during rendering the layer, so
  // we set them as if a clip operation were performed.
  if (bounds) {
    intersect(*bounds);
  }
  if (backdrop) {
    // A backdrop will affect up to the entire surface, bounded by the clip
    AccumulateUnbounded();
  }
}
void DisplayListBuilder::saveLayer(const SkRect* bounds,
                                   const DlPaint* paint,
                                   const DlImageFilter* backdrop) {
  if (paint != nullptr) {
    setAttributesFromDlPaint(*paint,
                             DisplayListOpFlags::kSaveLayerWithPaintFlags);
    saveLayer(bounds, SaveLayerOptions::kWithAttributes, backdrop);
  } else {
    saveLayer(bounds, SaveLayerOptions::kNoAttributes, backdrop);
  }
}

void DisplayListBuilder::translate(SkScalar tx, SkScalar ty) {
  if (SkScalarIsFinite(tx) && SkScalarIsFinite(ty) &&
      (tx != 0.0 || ty != 0.0)) {
    checkForDeferredSave();
    Push<TranslateOp>(0, 1, tx, ty);
    current_layer_->matrix().preTranslate(tx, ty);
    current_layer_->update_matrix33();
  }
}
void DisplayListBuilder::scale(SkScalar sx, SkScalar sy) {
  if (SkScalarIsFinite(sx) && SkScalarIsFinite(sy) &&
      (sx != 1.0 || sy != 1.0)) {
    checkForDeferredSave();
    Push<ScaleOp>(0, 1, sx, sy);
    current_layer_->matrix().preScale(sx, sy);
    current_layer_->update_matrix33();
  }
}
void DisplayListBuilder::rotate(SkScalar degrees) {
  if (SkScalarMod(degrees, 360.0) != 0.0) {
    checkForDeferredSave();
    Push<RotateOp>(0, 1, degrees);
    current_layer_->matrix().preConcat(SkMatrix::RotateDeg(degrees));
    current_layer_->update_matrix33();
  }
}
void DisplayListBuilder::skew(SkScalar sx, SkScalar sy) {
  if (SkScalarIsFinite(sx) && SkScalarIsFinite(sy) &&
      (sx != 0.0 || sy != 0.0)) {
    checkForDeferredSave();
    Push<SkewOp>(0, 1, sx, sy);
    current_layer_->matrix().preConcat(SkMatrix::Skew(sx, sy));
    current_layer_->update_matrix33();
  }
}

// clang-format off

// 2x3 2D affine subset of a 4x4 transform in row major order
void DisplayListBuilder::transform2DAffine(
    SkScalar mxx, SkScalar mxy, SkScalar mxt,
    SkScalar myx, SkScalar myy, SkScalar myt) {
  if (SkScalarsAreFinite(mxx, myx) &&
      SkScalarsAreFinite(mxy, myy) &&
      SkScalarsAreFinite(mxt, myt) &&
      !(mxx == 1 && mxy == 0 && mxt == 0 &&
        myx == 0 && myy == 1 && myt == 0)) {
    checkForDeferredSave();
    Push<Transform2DAffineOp>(0, 1,
                              mxx, mxy, mxt,
                              myx, myy, myt);
    current_layer_->matrix().preConcat(SkM44(mxx, mxy,  0,  mxt,
                                           myx, myy,  0,  myt,
                                            0,   0,   1,   0,
                                            0,   0,   0,   1));
    current_layer_->update_matrix33();
  }
}
// full 4x4 transform in row major order
void DisplayListBuilder::transformFullPerspective(
    SkScalar mxx, SkScalar mxy, SkScalar mxz, SkScalar mxt,
    SkScalar myx, SkScalar myy, SkScalar myz, SkScalar myt,
    SkScalar mzx, SkScalar mzy, SkScalar mzz, SkScalar mzt,
    SkScalar mwx, SkScalar mwy, SkScalar mwz, SkScalar mwt) {
  if (                        mxz == 0 &&
                              myz == 0 &&
      mzx == 0 && mzy == 0 && mzz == 1 && mzt == 0 &&
      mwx == 0 && mwy == 0 && mwz == 0 && mwt == 1) {
    transform2DAffine(mxx, mxy, mxt,
                      myx, myy, myt);
  } else if (SkScalarsAreFinite(mxx, mxy) && SkScalarsAreFinite(mxz, mxt) &&
             SkScalarsAreFinite(myx, myy) && SkScalarsAreFinite(myz, myt) &&
             SkScalarsAreFinite(mzx, mzy) && SkScalarsAreFinite(mzz, mzt) &&
             SkScalarsAreFinite(mwx, mwy) && SkScalarsAreFinite(mwz, mwt)) {
    checkForDeferredSave();
    Push<TransformFullPerspectiveOp>(0, 1,
                                     mxx, mxy, mxz, mxt,
                                     myx, myy, myz, myt,
                                     mzx, mzy, mzz, mzt,
                                     mwx, mwy, mwz, mwt);
    current_layer_->matrix().preConcat(SkM44(mxx, mxy, mxz, mxt,
                                           myx, myy, myz, myt,
                                           mzx, mzy, mzz, mzt,
                                           mwx, mwy, mwz, mwt));
    current_layer_->update_matrix33();
  }
}
// clang-format on
void DisplayListBuilder::transformReset() {
  checkForDeferredSave();
  Push<TransformResetOp>(0, 0);
  current_layer_->matrix().setIdentity();
  current_layer_->update_matrix33();
}
void DisplayListBuilder::transform(const SkMatrix* matrix) {
  if (matrix != nullptr) {
    transform(SkM44(*matrix));
  }
}
void DisplayListBuilder::transform(const SkM44* m44) {
  if (m44 != nullptr) {
    transformFullPerspective(
        m44->rc(0, 0), m44->rc(0, 1), m44->rc(0, 2), m44->rc(0, 3),
        m44->rc(1, 0), m44->rc(1, 1), m44->rc(1, 2), m44->rc(1, 3),
        m44->rc(2, 0), m44->rc(2, 1), m44->rc(2, 2), m44->rc(2, 3),
        m44->rc(3, 0), m44->rc(3, 1), m44->rc(3, 2), m44->rc(3, 3));
  }
}

void DisplayListBuilder::clipRect(const SkRect& rect,
                                  SkClipOp clip_op,
                                  bool is_aa) {
  if (!rect.isFinite()) {
    return;
  }
  checkForDeferredSave();
  switch (clip_op) {
    case SkClipOp::kIntersect:
      Push<ClipIntersectRectOp>(0, 1, rect, is_aa);
      intersect(rect);
      break;
    case SkClipOp::kDifference:
      Push<ClipDifferenceRectOp>(0, 1, rect, is_aa);
      break;
  }
}
void DisplayListBuilder::clipRRect(const SkRRect& rrect,
                                   SkClipOp clip_op,
                                   bool is_aa) {
  if (rrect.isRect()) {
    clipRect(rrect.rect(), clip_op, is_aa);
  } else {
    checkForDeferredSave();
    switch (clip_op) {
      case SkClipOp::kIntersect:
        Push<ClipIntersectRRectOp>(0, 1, rrect, is_aa);
        intersect(rrect.getBounds());
        break;
      case SkClipOp::kDifference:
        Push<ClipDifferenceRRectOp>(0, 1, rrect, is_aa);
        break;
    }
  }
}
void DisplayListBuilder::clipPath(const SkPath& path,
                                  SkClipOp clip_op,
                                  bool is_aa) {
  if (!path.isInverseFillType()) {
    SkRect rect;
    if (path.isRect(&rect)) {
      this->clipRect(rect, clip_op, is_aa);
      return;
    }
    SkRRect rrect;
    if (path.isOval(&rect)) {
      rrect.setOval(rect);
      this->clipRRect(rrect, clip_op, is_aa);
      return;
    }
    if (path.isRRect(&rrect)) {
      this->clipRRect(rrect, clip_op, is_aa);
      return;
    }
  }
  checkForDeferredSave();
  switch (clip_op) {
    case SkClipOp::kIntersect:
      Push<ClipIntersectPathOp>(0, 1, path, is_aa);
      if (!path.isInverseFillType()) {
        intersect(path.getBounds());
      }
      break;
    case SkClipOp::kDifference:
      Push<ClipDifferencePathOp>(0, 1, path, is_aa);
      // Map "kDifference of inverse path" to "kIntersect of the original path".
      if (path.isInverseFillType()) {
        intersect(path.getBounds());
      }
      break;
  }
}
void DisplayListBuilder::intersect(const SkRect& rect) {
  SkRect dev_clip_bounds = getTransform().mapRect(rect);
  if (!current_layer_->clip_bounds().intersect(dev_clip_bounds)) {
    current_layer_->clip_bounds().setEmpty();
  }
}
SkRect DisplayListBuilder::getLocalClipBounds() {
  SkM44 inverse;
  if (current_layer_->matrix().invert(&inverse)) {
    SkRect dev_bounds;
    current_layer_->clip_bounds().roundOut(&dev_bounds);
    return inverse.asM33().mapRect(dev_bounds);
  }
  return kMaxCullRect;
}

bool DisplayListBuilder::quickReject(const SkRect& bounds) const {
  if (bounds.isEmpty()) {
    return true;
  }
  SkMatrix matrix = getTransform();
  // We don't need the inverse, but this method tells us if the matrix
  // is singular in which case we can reject all rendering.
  if (!matrix.invert(nullptr)) {
    return true;
  }
  SkRect dev_bounds;
  matrix.mapRect(bounds).roundOut(&dev_bounds);
  return !current_layer_->clip_bounds().intersects(dev_bounds);
}

void DisplayListBuilder::drawPaint() {
  Push<DrawPaintOp>(0, 1);
  CheckLayerOpacityCompatibility();
  AccumulateUnbounded();
}
void DisplayListBuilder::drawPaint(const DlPaint& paint) {
  setAttributesFromDlPaint(paint, DisplayListOpFlags::kDrawPaintFlags);
  drawPaint();
}
void DisplayListBuilder::drawColor(DlColor color, DlBlendMode mode) {
  Push<DrawColorOp>(0, 1, color, mode);
  CheckLayerOpacityCompatibility(mode);
  AccumulateUnbounded();
}
void DisplayListBuilder::drawLine(const SkPoint& p0, const SkPoint& p1) {
  Push<DrawLineOp>(0, 1, p0, p1);
  CheckLayerOpacityCompatibility();
  SkRect bounds = SkRect::MakeLTRB(p0.fX, p0.fY, p1.fX, p1.fY).makeSorted();
  DisplayListAttributeFlags flags =
      (bounds.width() > 0.0f && bounds.height() > 0.0f) ? kDrawLineFlags
                                                        : kDrawHVLineFlags;
  AccumulateOpBounds(bounds, flags);
}
void DisplayListBuilder::drawLine(const SkPoint& p0,
                                  const SkPoint& p1,
                                  const DlPaint& paint) {
  setAttributesFromDlPaint(paint, DisplayListOpFlags::kDrawLineFlags);
  drawLine(p0, p1);
}
void DisplayListBuilder::drawRect(const SkRect& rect) {
  Push<DrawRectOp>(0, 1, rect);
  CheckLayerOpacityCompatibility();
  AccumulateOpBounds(rect, kDrawRectFlags);
}
void DisplayListBuilder::drawRect(const SkRect& rect, const DlPaint& paint) {
  setAttributesFromDlPaint(paint, DisplayListOpFlags::kDrawRectFlags);
  drawRect(rect);
}
void DisplayListBuilder::drawOval(const SkRect& bounds) {
  Push<DrawOvalOp>(0, 1, bounds);
  CheckLayerOpacityCompatibility();
  AccumulateOpBounds(bounds, kDrawOvalFlags);
}
void DisplayListBuilder::drawOval(const SkRect& bounds, const DlPaint& paint) {
  setAttributesFromDlPaint(paint, DisplayListOpFlags::kDrawOvalFlags);
  drawOval(bounds);
}
void DisplayListBuilder::drawCircle(const SkPoint& center, SkScalar radius) {
  Push<DrawCircleOp>(0, 1, center, radius);
  CheckLayerOpacityCompatibility();
  AccumulateOpBounds(SkRect::MakeLTRB(center.fX - radius, center.fY - radius,
                                      center.fX + radius, center.fY + radius),
                     kDrawCircleFlags);
}
void DisplayListBuilder::drawCircle(const SkPoint& center,
                                    SkScalar radius,
                                    const DlPaint& paint) {
  setAttributesFromDlPaint(paint, DisplayListOpFlags::kDrawCircleFlags);
  drawCircle(center, radius);
}
void DisplayListBuilder::drawRRect(const SkRRect& rrect) {
  if (rrect.isRect()) {
    drawRect(rrect.rect());
  } else if (rrect.isOval()) {
    drawOval(rrect.rect());
  } else {
    Push<DrawRRectOp>(0, 1, rrect);
    CheckLayerOpacityCompatibility();
    AccumulateOpBounds(rrect.getBounds(), kDrawRRectFlags);
  }
}
void DisplayListBuilder::drawRRect(const SkRRect& rrect, const DlPaint& paint) {
  setAttributesFromDlPaint(paint, DisplayListOpFlags::kDrawRRectFlags);
  drawRRect(rrect);
}
void DisplayListBuilder::drawDRRect(const SkRRect& outer,
                                    const SkRRect& inner) {
  Push<DrawDRRectOp>(0, 1, outer, inner);
  CheckLayerOpacityCompatibility();
  AccumulateOpBounds(outer.getBounds(), kDrawDRRectFlags);
}
void DisplayListBuilder::drawDRRect(const SkRRect& outer,
                                    const SkRRect& inner,
                                    const DlPaint& paint) {
  setAttributesFromDlPaint(paint, DisplayListOpFlags::kDrawDRRectFlags);
  drawDRRect(outer, inner);
}
void DisplayListBuilder::drawPath(const SkPath& path) {
  Push<DrawPathOp>(0, 1, path);
  CheckLayerOpacityHairlineCompatibility();
  if (path.isInverseFillType()) {
    AccumulateUnbounded();
  } else {
    AccumulateOpBounds(path.getBounds(), kDrawPathFlags);
  }
}
void DisplayListBuilder::drawPath(const SkPath& path, const DlPaint& paint) {
  setAttributesFromDlPaint(paint, DisplayListOpFlags::kDrawPathFlags);
  drawPath(path);
}

void DisplayListBuilder::drawArc(const SkRect& bounds,
                                 SkScalar start,
                                 SkScalar sweep,
                                 bool useCenter) {
  Push<DrawArcOp>(0, 1, bounds, start, sweep, useCenter);
  if (useCenter) {
    CheckLayerOpacityHairlineCompatibility();
  } else {
    CheckLayerOpacityCompatibility();
  }
  // This could be tighter if we compute where the start and end
  // angles are and then also consider the quadrants swept and
  // the center if specified.
  AccumulateOpBounds(bounds,
                     useCenter  //
                         ? kDrawArcWithCenterFlags
                         : kDrawArcNoCenterFlags);
}
void DisplayListBuilder::drawArc(const SkRect& bounds,
                                 SkScalar start,
                                 SkScalar sweep,
                                 bool useCenter,
                                 const DlPaint& paint) {
  setAttributesFromDlPaint(
      paint, useCenter ? kDrawArcWithCenterFlags : kDrawArcNoCenterFlags);
  drawArc(bounds, start, sweep, useCenter);
}
void DisplayListBuilder::drawPoints(SkCanvas::PointMode mode,
                                    uint32_t count,
                                    const SkPoint pts[]) {
  if (count == 0) {
    return;
  }

  void* data_ptr;
  FML_DCHECK(count < kMaxDrawPointsCount);
  int bytes = count * sizeof(SkPoint);
  RectBoundsAccumulator ptBounds;
  for (size_t i = 0; i < count; i++) {
    ptBounds.accumulate(pts[i]);
  }
  SkRect point_bounds = ptBounds.bounds();
  switch (mode) {
    case SkCanvas::PointMode::kPoints_PointMode:
      data_ptr = Push<DrawPointsOp>(bytes, 1, count);
      AccumulateOpBounds(point_bounds, kDrawPointsAsPointsFlags);
      break;
    case SkCanvas::PointMode::kLines_PointMode:
      data_ptr = Push<DrawLinesOp>(bytes, 1, count);
      AccumulateOpBounds(point_bounds, kDrawPointsAsLinesFlags);
      break;
    case SkCanvas::PointMode::kPolygon_PointMode:
      data_ptr = Push<DrawPolygonOp>(bytes, 1, count);
      AccumulateOpBounds(point_bounds, kDrawPointsAsPolygonFlags);
      break;
    default:
      FML_DCHECK(false);
      return;
  }
  CopyV(data_ptr, pts, count);
  // drawPoints treats every point or line (or segment of a polygon)
  // as a completely separate operation meaning we cannot ensure
  // distribution of group opacity without analyzing the mode and the
  // bounds of every sub-primitive.
  // See: https://fiddle.skia.org/c/228459001d2de8db117ce25ef5cedb0c
  UpdateLayerOpacityCompatibility(false);
}
void DisplayListBuilder::drawPoints(SkCanvas::PointMode mode,
                                    uint32_t count,
                                    const SkPoint pts[],
                                    const DlPaint& paint) {
  const DisplayListAttributeFlags* flags;
  switch (mode) {
    case SkCanvas::PointMode::kPoints_PointMode:
      flags = &DisplayListOpFlags::kDrawPointsAsPointsFlags;
      break;
    case SkCanvas::PointMode::kLines_PointMode:
      flags = &DisplayListOpFlags::kDrawPointsAsLinesFlags;
      break;
    case SkCanvas::PointMode::kPolygon_PointMode:
      flags = &DisplayListOpFlags::kDrawPointsAsPolygonFlags;
      break;
    default:
      FML_DCHECK(false);
      return;
  }
  setAttributesFromDlPaint(paint, *flags);
  drawPoints(mode, count, pts);
}
void DisplayListBuilder::drawSkVertices(const sk_sp<SkVertices> vertices,
                                        SkBlendMode mode) {
  Push<DrawSkVerticesOp>(0, 1, vertices, mode);
  // DrawVertices applies its colors to the paint so we have no way
  // of controlling opacity using the current paint attributes.
  // Although, examination of the |mode| might find some predictable
  // cases.
  UpdateLayerOpacityCompatibility(false);
  AccumulateOpBounds(vertices->bounds(), kDrawVerticesFlags);
}
void DisplayListBuilder::drawVertices(const DlVertices* vertices,
                                      DlBlendMode mode) {
  void* pod = Push<DrawVerticesOp>(vertices->size(), 1, mode);
  new (pod) DlVertices(vertices);
  // DrawVertices applies its colors to the paint so we have no way
  // of controlling opacity using the current paint attributes.
  // Although, examination of the |mode| might find some predictable
  // cases.
  UpdateLayerOpacityCompatibility(false);
  AccumulateOpBounds(vertices->bounds(), kDrawVerticesFlags);
}
void DisplayListBuilder::drawVertices(const DlVertices* vertices,
                                      DlBlendMode mode,
                                      const DlPaint& paint) {
  setAttributesFromDlPaint(paint, DisplayListOpFlags::kDrawVerticesFlags);
  drawVertices(vertices, mode);
}

void DisplayListBuilder::drawImage(const sk_sp<DlImage> image,
                                   const SkPoint point,
                                   DlImageSampling sampling,
                                   bool render_with_attributes) {
  render_with_attributes
      ? Push<DrawImageWithAttrOp>(0, 1, image, point, sampling)
      : Push<DrawImageOp>(0, 1, image, point, sampling);
  CheckLayerOpacityCompatibility(render_with_attributes);
  SkRect bounds = SkRect::MakeXYWH(point.fX, point.fY,  //
                                   image->width(), image->height());
  DisplayListAttributeFlags flags = render_with_attributes  //
                                        ? kDrawImageWithPaintFlags
                                        : kDrawImageFlags;
  AccumulateOpBounds(bounds, flags);
}
void DisplayListBuilder::drawImage(const sk_sp<DlImage>& image,
                                   const SkPoint point,
                                   DlImageSampling sampling,
                                   const DlPaint* paint) {
  if (paint != nullptr) {
    setAttributesFromDlPaint(*paint,
                             DisplayListOpFlags::kDrawImageWithPaintFlags);
    drawImage(image, point, sampling, true);
  } else {
    drawImage(image, point, sampling, false);
  }
}
void DisplayListBuilder::drawImageRect(const sk_sp<DlImage> image,
                                       const SkRect& src,
                                       const SkRect& dst,
                                       DlImageSampling sampling,
                                       bool render_with_attributes,
                                       SkCanvas::SrcRectConstraint constraint) {
  Push<DrawImageRectOp>(0, 1, image, src, dst, sampling, render_with_attributes,
                        constraint);
  CheckLayerOpacityCompatibility(render_with_attributes);
  DisplayListAttributeFlags flags = render_with_attributes
                                        ? kDrawImageRectWithPaintFlags
                                        : kDrawImageRectFlags;
  AccumulateOpBounds(dst, flags);
}
void DisplayListBuilder::drawImageRect(const sk_sp<DlImage>& image,
                                       const SkRect& src,
                                       const SkRect& dst,
                                       DlImageSampling sampling,
                                       const DlPaint* paint,
                                       SkCanvas::SrcRectConstraint constraint) {
  if (paint != nullptr) {
    setAttributesFromDlPaint(*paint,
                             DisplayListOpFlags::kDrawImageRectWithPaintFlags);
    drawImageRect(image, src, dst, sampling, true, constraint);
  } else {
    drawImageRect(image, src, dst, sampling, false, constraint);
  }
}
void DisplayListBuilder::drawImageNine(const sk_sp<DlImage> image,
                                       const SkIRect& center,
                                       const SkRect& dst,
                                       DlFilterMode filter,
                                       bool render_with_attributes) {
  render_with_attributes
      ? Push<DrawImageNineWithAttrOp>(0, 1, image, center, dst, filter)
      : Push<DrawImageNineOp>(0, 1, image, center, dst, filter);
  CheckLayerOpacityCompatibility(render_with_attributes);
  DisplayListAttributeFlags flags = render_with_attributes
                                        ? kDrawImageNineWithPaintFlags
                                        : kDrawImageNineFlags;
  AccumulateOpBounds(dst, flags);
}
void DisplayListBuilder::drawImageNine(const sk_sp<DlImage>& image,
                                       const SkIRect& center,
                                       const SkRect& dst,
                                       DlFilterMode filter,
                                       const DlPaint* paint) {
  if (paint != nullptr) {
    setAttributesFromDlPaint(*paint,
                             DisplayListOpFlags::kDrawImageNineWithPaintFlags);
    drawImageNine(image, center, dst, filter, true);
  } else {
    drawImageNine(image, center, dst, filter, false);
  }
}
void DisplayListBuilder::drawImageLattice(const sk_sp<DlImage> image,
                                          const SkCanvas::Lattice& lattice,
                                          const SkRect& dst,
                                          DlFilterMode filter,
                                          bool render_with_attributes) {
  int x_div_count = lattice.fXCount;
  int y_div_count = lattice.fYCount;
  FML_DCHECK((lattice.fRectTypes == nullptr) || (lattice.fColors != nullptr));
  int cell_count = lattice.fRectTypes && lattice.fColors
                       ? (x_div_count + 1) * (y_div_count + 1)
                       : 0;
  size_t bytes =
      (x_div_count + y_div_count) * sizeof(int) +
      cell_count * (sizeof(SkColor) + sizeof(SkCanvas::Lattice::RectType));
  SkIRect src = lattice.fBounds ? *lattice.fBounds : image->bounds();
  void* pod = this->Push<DrawImageLatticeOp>(bytes, 1, image, x_div_count,
                                             y_div_count, cell_count, src, dst,
                                             filter, render_with_attributes);
  CopyV(pod, lattice.fXDivs, x_div_count, lattice.fYDivs, y_div_count,
        lattice.fColors, cell_count, lattice.fRectTypes, cell_count);
  CheckLayerOpacityCompatibility(render_with_attributes);
  DisplayListAttributeFlags flags = render_with_attributes
                                        ? kDrawImageLatticeWithPaintFlags
                                        : kDrawImageLatticeFlags;
  AccumulateOpBounds(dst, flags);
}
void DisplayListBuilder::drawAtlas(const sk_sp<DlImage> atlas,
                                   const SkRSXform xform[],
                                   const SkRect tex[],
                                   const DlColor colors[],
                                   int count,
                                   DlBlendMode mode,
                                   DlImageSampling sampling,
                                   const SkRect* cull_rect,
                                   bool render_with_attributes) {
  int bytes = count * (sizeof(SkRSXform) + sizeof(SkRect));
  void* data_ptr;
  if (colors != nullptr) {
    bytes += count * sizeof(DlColor);
    if (cull_rect != nullptr) {
      data_ptr =
          Push<DrawAtlasCulledOp>(bytes, 1, atlas, count, mode, sampling, true,
                                  *cull_rect, render_with_attributes);
    } else {
      data_ptr = Push<DrawAtlasOp>(bytes, 1, atlas, count, mode, sampling, true,
                                   render_with_attributes);
    }
    CopyV(data_ptr, xform, count, tex, count, colors, count);
  } else {
    if (cull_rect != nullptr) {
      data_ptr =
          Push<DrawAtlasCulledOp>(bytes, 1, atlas, count, mode, sampling, false,
                                  *cull_rect, render_with_attributes);
    } else {
      data_ptr = Push<DrawAtlasOp>(bytes, 1, atlas, count, mode, sampling,
                                   false, render_with_attributes);
    }
    CopyV(data_ptr, xform, count, tex, count);
  }
  // drawAtlas treats each image as a separate operation so we cannot rely
  // on it to distribute the opacity without overlap without checking all
  // of the transforms and texture rectangles.
  UpdateLayerOpacityCompatibility(false);

  SkPoint quad[4];
  RectBoundsAccumulator atlasBounds;
  for (int i = 0; i < count; i++) {
    const SkRect& src = tex[i];
    xform[i].toQuad(src.width(), src.height(), quad);
    for (int j = 0; j < 4; j++) {
      atlasBounds.accumulate(quad[j]);
    }
  }
  if (atlasBounds.is_not_empty()) {
    DisplayListAttributeFlags flags = render_with_attributes  //
                                          ? kDrawAtlasWithPaintFlags
                                          : kDrawAtlasFlags;
    AccumulateOpBounds(atlasBounds.bounds(), flags);
  }
}
void DisplayListBuilder::drawAtlas(const sk_sp<DlImage>& atlas,
                                   const SkRSXform xform[],
                                   const SkRect tex[],
                                   const DlColor colors[],
                                   int count,
                                   DlBlendMode mode,
                                   DlImageSampling sampling,
                                   const SkRect* cull_rect,
                                   const DlPaint* paint) {
  if (paint != nullptr) {
    setAttributesFromDlPaint(*paint,
                             DisplayListOpFlags::kDrawAtlasWithPaintFlags);
    drawAtlas(atlas, xform, tex, colors, count, mode, sampling, cull_rect,
              true);
  } else {
    drawAtlas(atlas, xform, tex, colors, count, mode, sampling, cull_rect,
              false);
  }
}

void DisplayListBuilder::drawPicture(const sk_sp<SkPicture> picture,
                                     const SkMatrix* matrix,
                                     bool render_with_attributes) {
  // TODO(flar) cull rect really cannot be trusted in general, but it will
  // work for SkPictures generated from our own PictureRecorder or any
  // picture captured with an SkRTreeFactory or accurate bounds estimate.
  SkRect bounds = picture->cullRect();
  if (matrix) {
    matrix->mapRect(&bounds);
  }
  DisplayListAttributeFlags flags = render_with_attributes  //
                                        ? kDrawPictureWithPaintFlags
                                        : kDrawPictureFlags;
  AccumulateOpBounds(bounds, flags);

  matrix  //
      ? Push<DrawSkPictureMatrixOp>(0, 1, picture, *matrix,
                                    render_with_attributes)
      : Push<DrawSkPictureOp>(0, 1, picture, render_with_attributes);
  // The non-nested op count accumulated in the |Push| method will include
  // this call to |drawPicture| for non-nested op count metrics.
  // But, for nested op count metrics we want the |drawPicture| call itself
  // to be transparent. So we subtract 1 from our accumulated nested count to
  // balance out against the 1 that was accumulated into the regular count.
  // This behavior is identical to the way SkPicture computes nested op counts.
  nested_op_count_ += picture->approximateOpCount(true) - 1;
  nested_bytes_ += picture->approximateBytesUsed();
  CheckLayerOpacityCompatibility(render_with_attributes);
}
void DisplayListBuilder::drawDisplayList(
    const sk_sp<DisplayList> display_list) {
  const SkRect bounds = display_list->bounds();
  switch (accumulator()->type()) {
    case BoundsAccumulatorType::kRect:
      AccumulateOpBounds(bounds, kDrawDisplayListFlags);
      break;
    case BoundsAccumulatorType::kRTree:
      auto rtree = display_list->rtree();
      if (rtree) {
        std::list<SkRect> rects = rtree->searchNonOverlappingDrawnRects(bounds);
        for (const SkRect& rect : rects) {
          // TODO (https://github.com/flutter/flutter/issues/114919): Attributes
          // are not necessarily `kDrawDisplayListFlags`.
          AccumulateOpBounds(rect, kDrawDisplayListFlags);
        }
      } else {
        AccumulateOpBounds(bounds, kDrawDisplayListFlags);
      }
      break;
  }
  Push<DrawDisplayListOp>(0, 1, display_list);
  // The non-nested op count accumulated in the |Push| method will include
  // this call to |drawDisplayList| for non-nested op count metrics.
  // But, for nested op count metrics we want the |drawDisplayList| call itself
  // to be transparent. So we subtract 1 from our accumulated nested count to
  // balance out against the 1 that was accumulated into the regular count.
  // This behavior is identical to the way SkPicture computes nested op counts.
  nested_op_count_ += display_list->op_count(true) - 1;
  nested_bytes_ += display_list->bytes(true);
  UpdateLayerOpacityCompatibility(display_list->can_apply_group_opacity());
}
void DisplayListBuilder::drawTextBlob(const sk_sp<SkTextBlob> blob,
                                      SkScalar x,
                                      SkScalar y) {
  AccumulateOpBounds(blob->bounds().makeOffset(x, y), kDrawTextBlobFlags);
  Push<DrawTextBlobOp>(0, 1, blob, x, y);
  CheckLayerOpacityCompatibility();
}
void DisplayListBuilder::drawTextBlob(const sk_sp<SkTextBlob>& blob,
                                      SkScalar x,
                                      SkScalar y,
                                      const DlPaint& paint) {
  setAttributesFromDlPaint(paint, DisplayListOpFlags::kDrawTextBlobFlags);
  drawTextBlob(blob, x, y);
}
void DisplayListBuilder::drawShadow(const SkPath& path,
                                    const DlColor color,
                                    const SkScalar elevation,
                                    bool transparent_occluder,
                                    SkScalar dpr) {
  SkRect shadow_bounds = DisplayListCanvasDispatcher::ComputeShadowBounds(
      path, elevation, dpr, getTransform());
  AccumulateOpBounds(shadow_bounds, kDrawShadowFlags);

  transparent_occluder  //
      ? Push<DrawShadowTransparentOccluderOp>(0, 1, path, color, elevation, dpr)
      : Push<DrawShadowOp>(0, 1, path, color, elevation, dpr);
  UpdateLayerOpacityCompatibility(false);
}

bool DisplayListBuilder::ComputeFilteredBounds(SkRect& bounds,
                                               const DlImageFilter* filter) {
  if (filter) {
    if (!filter->map_local_bounds(bounds, bounds)) {
      return false;
    }
  }
  return true;
}

bool DisplayListBuilder::AdjustBoundsForPaint(SkRect& bounds,
                                              DisplayListAttributeFlags flags) {
  if (flags.ignores_paint()) {
    return true;
  }

  if (flags.is_geometric()) {
    // Path effect occurs before stroking...
    DisplayListSpecialGeometryFlags special_flags =
        flags.WithPathEffect(current_.getPathEffectPtr());
    if (current_.getPathEffect()) {
      auto effect_bounds = current_.getPathEffect()->effect_bounds(bounds);
      if (!effect_bounds.has_value()) {
        return false;
      }
      bounds = effect_bounds.value();
    }

    if (flags.is_stroked(current_.getDrawStyle())) {
      // Determine the max multiplier to the stroke width first.
      SkScalar pad = 1.0f;
      if (current_.getStrokeJoin() == DlStrokeJoin::kMiter &&
          special_flags.may_have_acute_joins()) {
        pad = std::max(pad, current_.getStrokeMiter());
      }
      if (current_.getStrokeCap() == DlStrokeCap::kSquare &&
          special_flags.may_have_diagonal_caps()) {
        pad = std::max(pad, SK_ScalarSqrt2);
      }
      SkScalar min_stroke_width = 0.01;
      pad *= std::max(getStrokeWidth() * 0.5f, min_stroke_width);
      bounds.outset(pad, pad);
    }
  }

  if (flags.applies_mask_filter()) {
    if (current_.getMaskFilter()) {
      const DlBlurMaskFilter* blur_filter = current_.getMaskFilter()->asBlur();
      if (blur_filter) {
        SkScalar mask_sigma_pad = blur_filter->sigma() * 3.0;
        bounds.outset(mask_sigma_pad, mask_sigma_pad);
      } else {
        SkPaint p;
        p.setMaskFilter(current_.getMaskFilter()->skia_object());
        if (!p.canComputeFastBounds()) {
          return false;
        }
        bounds = p.computeFastBounds(bounds, &bounds);
      }
    }
  }

  if (flags.applies_image_filter()) {
    return ComputeFilteredBounds(bounds, current_.getImageFilter().get());
  }

  return true;
}

void DisplayListBuilder::AccumulateUnbounded() {
  accumulator()->accumulate(current_layer_->clip_bounds());
}

void DisplayListBuilder::AccumulateOpBounds(SkRect& bounds,
                                            DisplayListAttributeFlags flags) {
  if (AdjustBoundsForPaint(bounds, flags)) {
    AccumulateBounds(bounds);
  } else {
    AccumulateUnbounded();
  }
}
void DisplayListBuilder::AccumulateBounds(SkRect& bounds) {
  getTransform().mapRect(&bounds);
  if (bounds.intersect(current_layer_->clip_bounds())) {
    accumulator()->accumulate(bounds);
  }
}

bool DisplayListBuilder::paint_nops_on_transparency() {
  // SkImageFilter::canComputeFastBounds tests for transparency behavior
  // This test assumes that the blend mode checked down below will
  // NOP on transparent black.
  if (current_.getImageFilter() &&
      current_.getImageFilter()->modifies_transparent_black()) {
    return false;
  }

  // We filter the transparent black that is used for the background of a
  // saveLayer and make sure it returns transparent black. If it does, then
  // the color filter will leave all area surrounding the contents of the
  // save layer untouched out to the edge of the output surface.
  // This test assumes that the blend mode checked down below will
  // NOP on transparent black.
  if (current_.getColorFilter() &&
      current_.getColorFilter()->modifies_transparent_black()) {
    return false;
  }

  if (!getBlendMode()) {
    return false;  // can we query other blenders for this?
  }
  // Unusual blendmodes require us to process a saved layer
  // even with operations outisde the clip.
  // For example, DstIn is used by masking layers.
  // https://code.google.com/p/skia/issues/detail?id=1291
  // https://crbug.com/401593
  switch (getBlendMode().value()) {
    // For each of the following transfer modes, if the source
    // alpha is zero (our transparent black), the resulting
    // blended pixel is not necessarily equal to the original
    // destination pixel.
    // Mathematically, any time in the following equations where
    // the result is not d assuming source is 0
    case DlBlendMode::kClear:     // r = 0
    case DlBlendMode::kSrc:       // r = s
    case DlBlendMode::kSrcIn:     // r = s * da
    case DlBlendMode::kDstIn:     // r = d * sa
    case DlBlendMode::kSrcOut:    // r = s * (1-da)
    case DlBlendMode::kDstATop:   // r = d*sa + s*(1-da)
    case DlBlendMode::kModulate:  // r = s*d
      return false;
      break;

    // And in these equations, the result must be d if the
    // source is 0
    case DlBlendMode::kDst:         // r = d
    case DlBlendMode::kSrcOver:     // r = s + (1-sa)*d
    case DlBlendMode::kDstOver:     // r = d + (1-da)*s
    case DlBlendMode::kDstOut:      // r = d * (1-sa)
    case DlBlendMode::kSrcATop:     // r = s*da + d*(1-sa)
    case DlBlendMode::kXor:         // r = s*(1-da) + d*(1-sa)
    case DlBlendMode::kPlus:        // r = min(s + d, 1)
    case DlBlendMode::kScreen:      // r = s + d - s*d
    case DlBlendMode::kOverlay:     // multiply or screen, depending on dest
    case DlBlendMode::kDarken:      // rc = s + d - max(s*da, d*sa),
                                    // ra = kSrcOver
    case DlBlendMode::kLighten:     // rc = s + d - min(s*da, d*sa),
                                    // ra = kSrcOver
    case DlBlendMode::kColorDodge:  // brighten destination to reflect source
    case DlBlendMode::kColorBurn:   // darken destination to reflect source
    case DlBlendMode::kHardLight:   // multiply or screen, depending on source
    case DlBlendMode::kSoftLight:   // lighten or darken, depending on source
    case DlBlendMode::kDifference:  // rc = s + d - 2*(min(s*da, d*sa)),
                                    // ra = kSrcOver
    case DlBlendMode::kExclusion:   // rc = s + d - two(s*d), ra = kSrcOver
    case DlBlendMode::kMultiply:    // r = s*(1-da) + d*(1-sa) + s*d
    case DlBlendMode::kHue:         // ra = kSrcOver
    case DlBlendMode::kSaturation:  // ra = kSrcOver
    case DlBlendMode::kColor:       // ra = kSrcOver
    case DlBlendMode::kLuminosity:  // ra = kSrcOver
      return true;
      break;
  }
}
}  // namespace flutter
