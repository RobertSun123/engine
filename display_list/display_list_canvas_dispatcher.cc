// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/display_list/display_list_canvas_dispatcher.h"

#include "flutter/display_list/display_list_blend_mode.h"
#include "flutter/fml/trace_event.h"

namespace flutter {

const SkScalar kLightHeight = 600;
const SkScalar kLightRadius = 800;

const SkPaint* DisplayListCanvasDispatcher::safe_paint(RenderWith with) {
  switch (with) {
    case RenderWith::kDlPaint:
      // The accumulated SkPaint object will already have incorporated
      // any attribute overrides.
      return &paint();
    case RenderWith::kAlpha:
      temp_paint_.setColor(paint().getColor());
      return &temp_paint_;
    case RenderWith::kDefaults:
      return nullptr;
  }
  FML_DCHECK(false);
}

void DisplayListCanvasDispatcher::save() {
  canvas_->save();
}
void DisplayListCanvasDispatcher::restore() {
  canvas_->restore();
}
void DisplayListCanvasDispatcher::saveLayer(const SkRect* bounds,
                                            RenderWith with,
                                            const DlImageFilter* backdrop,
                                            int optimizations) {
  TRACE_EVENT0("flutter", "Canvas::saveLayer");
  const SkPaint* paint = safe_paint(with);
  const sk_sp<SkImageFilter> sk_backdrop =
      backdrop ? backdrop->skia_object() : nullptr;
  canvas_->saveLayer(
      SkCanvas::SaveLayerRec(bounds, paint, sk_backdrop.get(), 0));
}

void DisplayListCanvasDispatcher::translate(SkScalar tx, SkScalar ty) {
  canvas_->translate(tx, ty);
}
void DisplayListCanvasDispatcher::scale(SkScalar sx, SkScalar sy) {
  canvas_->scale(sx, sy);
}
void DisplayListCanvasDispatcher::rotate(SkScalar degrees) {
  canvas_->rotate(degrees);
}
void DisplayListCanvasDispatcher::skew(SkScalar sx, SkScalar sy) {
  canvas_->skew(sx, sy);
}
// clang-format off
// 2x3 2D affine subset of a 4x4 transform in row major order
void DisplayListCanvasDispatcher::transform2DAffine(
    SkScalar mxx, SkScalar mxy, SkScalar mxt,
    SkScalar myx, SkScalar myy, SkScalar myt) {
  // Internally concat(SkMatrix) gets redirected to concat(SkM44)
  // so we just jump directly to the SkM44 version
  canvas_->concat(SkM44(mxx, mxy, 0, mxt,
                        myx, myy, 0, myt,
                         0,   0,  1,  0,
                         0,   0,  0,  1));
}
// full 4x4 transform in row major order
void DisplayListCanvasDispatcher::transformFullPerspective(
    SkScalar mxx, SkScalar mxy, SkScalar mxz, SkScalar mxt,
    SkScalar myx, SkScalar myy, SkScalar myz, SkScalar myt,
    SkScalar mzx, SkScalar mzy, SkScalar mzz, SkScalar mzt,
    SkScalar mwx, SkScalar mwy, SkScalar mwz, SkScalar mwt) {
  canvas_->concat(SkM44(mxx, mxy, mxz, mxt,
                        myx, myy, myz, myt,
                        mzx, mzy, mzz, mzt,
                        mwx, mwy, mwz, mwt));
}
// clang-format on
void DisplayListCanvasDispatcher::transformReset() {
  canvas_->resetMatrix();
}

void DisplayListCanvasDispatcher::clipRect(const SkRect& rect,
                                           SkClipOp clip_op,
                                           bool is_aa) {
  canvas_->clipRect(rect, clip_op, is_aa);
}
void DisplayListCanvasDispatcher::clipRRect(const SkRRect& rrect,
                                            SkClipOp clip_op,
                                            bool is_aa) {
  canvas_->clipRRect(rrect, clip_op, is_aa);
}
void DisplayListCanvasDispatcher::clipPath(const SkPath& path,
                                           SkClipOp clip_op,
                                           bool is_aa) {
  canvas_->clipPath(path, clip_op, is_aa);
}

void DisplayListCanvasDispatcher::drawPaint() {
  const SkPaint& sk_paint = paint();
  SkImageFilter* filter = sk_paint.getImageFilter();
  if (filter && !filter->asColorFilter(nullptr)) {
    // drawPaint does an implicit saveLayer if an SkImageFilter is
    // present that cannot be replaced by an SkColorFilter.
    TRACE_EVENT0("flutter", "Canvas::saveLayer");
  }
  canvas_->drawPaint(sk_paint);
}
void DisplayListCanvasDispatcher::drawColor(DlColor color, DlBlendMode mode) {
  canvas_->drawColor(color, ToSk(mode));
}
void DisplayListCanvasDispatcher::drawLine(const SkPoint& p0,
                                           const SkPoint& p1) {
  canvas_->drawLine(p0, p1, paint());
}
void DisplayListCanvasDispatcher::drawRect(const SkRect& rect) {
  canvas_->drawRect(rect, paint());
}
void DisplayListCanvasDispatcher::drawOval(const SkRect& bounds) {
  canvas_->drawOval(bounds, paint());
}
void DisplayListCanvasDispatcher::drawCircle(const SkPoint& center,
                                             SkScalar radius) {
  canvas_->drawCircle(center, radius, paint());
}
void DisplayListCanvasDispatcher::drawRRect(const SkRRect& rrect) {
  canvas_->drawRRect(rrect, paint());
}
void DisplayListCanvasDispatcher::drawDRRect(const SkRRect& outer,
                                             const SkRRect& inner) {
  canvas_->drawDRRect(outer, inner, paint());
}
void DisplayListCanvasDispatcher::drawPath(const SkPath& path) {
  canvas_->drawPath(path, paint());
}
void DisplayListCanvasDispatcher::drawArc(const SkRect& bounds,
                                          SkScalar start,
                                          SkScalar sweep,
                                          bool useCenter) {
  canvas_->drawArc(bounds, start, sweep, useCenter, paint());
}
void DisplayListCanvasDispatcher::drawPoints(SkCanvas::PointMode mode,
                                             uint32_t count,
                                             const SkPoint pts[]) {
  canvas_->drawPoints(mode, count, pts, paint());
}
void DisplayListCanvasDispatcher::drawSkVertices(
    const sk_sp<SkVertices> vertices,
    SkBlendMode mode) {
  canvas_->drawVertices(vertices, mode, paint());
}
void DisplayListCanvasDispatcher::drawVertices(const DlVertices* vertices,
                                               DlBlendMode mode) {
  canvas_->drawVertices(vertices->skia_object(), ToSk(mode), paint());
}
void DisplayListCanvasDispatcher::drawImage(const sk_sp<DlImage> image,
                                            const SkPoint point,
                                            DlImageSampling sampling,
                                            RenderWith with) {
  canvas_->drawImage(image ? image->skia_image() : nullptr, point.fX, point.fY,
                     ToSk(sampling), safe_paint(with));
}
void DisplayListCanvasDispatcher::drawImageRect(
    const sk_sp<DlImage> image,
    const SkRect& src,
    const SkRect& dst,
    DlImageSampling sampling,
    RenderWith with,
    SkCanvas::SrcRectConstraint constraint) {
  canvas_->drawImageRect(image ? image->skia_image() : nullptr, src, dst,
                         ToSk(sampling), safe_paint(with), constraint);
}
void DisplayListCanvasDispatcher::drawImageNine(const sk_sp<DlImage> image,
                                                const SkIRect& center,
                                                const SkRect& dst,
                                                DlFilterMode filter,
                                                RenderWith with) {
  if (!image) {
    return;
  }
  auto skia_image = image->skia_image();
  if (!skia_image) {
    return;
  }
  canvas_->drawImageNine(skia_image.get(), center, dst, ToSk(filter),
                         safe_paint(with));
}
void DisplayListCanvasDispatcher::drawImageLattice(
    const sk_sp<DlImage> image,
    const SkCanvas::Lattice& lattice,
    const SkRect& dst,
    DlFilterMode filter,
    RenderWith with) {
  if (!image) {
    return;
  }
  auto skia_image = image->skia_image();
  if (!skia_image) {
    return;
  }
  canvas_->drawImageLattice(skia_image.get(), lattice, dst, ToSk(filter),
                            safe_paint(with));
}
void DisplayListCanvasDispatcher::drawAtlas(const sk_sp<DlImage> atlas,
                                            const SkRSXform xform[],
                                            const SkRect tex[],
                                            const DlColor colors[],
                                            int count,
                                            DlBlendMode mode,
                                            DlImageSampling sampling,
                                            const SkRect* cullRect,
                                            RenderWith with) {
  if (!atlas) {
    return;
  }
  auto skia_atlas = atlas->skia_image();
  if (!skia_atlas) {
    return;
  }
  const SkColor* sk_colors = reinterpret_cast<const SkColor*>(colors);
  canvas_->drawAtlas(skia_atlas.get(), xform, tex, sk_colors, count, ToSk(mode),
                     ToSk(sampling), cullRect, safe_paint(with));
}
void DisplayListCanvasDispatcher::drawPicture(const sk_sp<SkPicture> picture,
                                              const SkMatrix* matrix,
                                              RenderWith with) {
  const SkPaint* paint = safe_paint(with);
  if (paint) {
    // drawPicture does an implicit saveLayer if an SkPaint is supplied.
    TRACE_EVENT0("flutter", "Canvas::saveLayer");
    canvas_->drawPicture(picture, matrix, paint);
  } else {
    canvas_->drawPicture(picture, matrix, nullptr);
  }
}
void DisplayListCanvasDispatcher::drawDisplayList(
    const sk_sp<DisplayList> display_list,
    SkScalar opacity) {
  int save_count = canvas_->save();
  display_list->RenderTo(canvas_, opacity);
  canvas_->restoreToCount(save_count);
}
void DisplayListCanvasDispatcher::drawTextBlob(const sk_sp<SkTextBlob> blob,
                                               SkScalar x,
                                               SkScalar y) {
  canvas_->drawTextBlob(blob, x, y, paint());
}

SkRect DisplayListCanvasDispatcher::ComputeShadowBounds(const SkPath& path,
                                                        float elevation,
                                                        SkScalar dpr,
                                                        const SkMatrix& ctm) {
  SkRect shadow_bounds(path.getBounds());
  SkShadowUtils::GetLocalBounds(
      ctm, path, SkPoint3::Make(0, 0, dpr * elevation),
      SkPoint3::Make(0, -1, 1), kLightRadius / kLightHeight,
      SkShadowFlags::kDirectionalLight_ShadowFlag, &shadow_bounds);
  return shadow_bounds;
}

void DisplayListCanvasDispatcher::DrawShadow(SkCanvas* canvas,
                                             const SkPath& path,
                                             DlColor color,
                                             float elevation,
                                             bool transparentOccluder,
                                             SkScalar dpr) {
  const SkScalar kAmbientAlpha = 0.039f;
  const SkScalar kSpotAlpha = 0.25f;

  uint32_t flags = transparentOccluder
                       ? SkShadowFlags::kTransparentOccluder_ShadowFlag
                       : SkShadowFlags::kNone_ShadowFlag;
  flags |= SkShadowFlags::kDirectionalLight_ShadowFlag;
  SkColor in_ambient = SkColorSetA(color, kAmbientAlpha * SkColorGetA(color));
  SkColor in_spot = SkColorSetA(color, kSpotAlpha * SkColorGetA(color));
  SkColor ambient_color, spot_color;
  SkShadowUtils::ComputeTonalColors(in_ambient, in_spot, &ambient_color,
                                    &spot_color);
  SkShadowUtils::DrawShadow(canvas, path, SkPoint3::Make(0, 0, dpr * elevation),
                            SkPoint3::Make(0, -1, 1),
                            kLightRadius / kLightHeight, ambient_color,
                            spot_color, flags);
}

void DisplayListCanvasDispatcher::drawShadow(const SkPath& path,
                                             const DlColor color,
                                             const SkScalar elevation,
                                             bool transparent_occluder,
                                             SkScalar dpr) {
  DrawShadow(canvas_, path, color, elevation, transparent_occluder, dpr);
}

}  // namespace flutter
