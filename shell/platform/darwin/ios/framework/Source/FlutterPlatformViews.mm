// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <UIKit/UIGestureRecognizerSubclass.h>

#import "FlutterOverlayView.h"
#import "flutter/shell/platform/darwin/ios/ios_surface.h"
#import "flutter/shell/platform/darwin/ios/ios_surface_gl.h"

#include <map>
#include <memory>
#include <string>

#include "FlutterPlatformViews_Internal.h"
#include "flutter/fml/platform/darwin/scoped_nsobject.h"
#include "flutter/shell/platform/darwin/ios/framework/Headers/FlutterChannels.h"

#pragma mark - Transforms Utils

static CGRect GetCGRectFromSkRect(const SkRect& clipSkRect) {
  return CGRectMake(clipSkRect.fLeft, clipSkRect.fTop, clipSkRect.fRight - clipSkRect.fLeft,
                    clipSkRect.fBottom - clipSkRect.fTop);
}

static void ClipRect(UIView* view, const SkRect& clipSkRect) {
  CGRect clipRect = GetCGRectFromSkRect(clipSkRect);
  CGPathRef pathRef = CGPathCreateWithRect(clipRect, nil);
  CAShapeLayer* clip = [[CAShapeLayer alloc] init];
  clip.path = pathRef;
  view.layer.mask = clip;
  CGPathRelease(pathRef);
}

static void ClipRRect(UIView* view, const SkRRect& clipSkRRect) {
  CGPathRef pathRef = nullptr;
  switch (clipSkRRect.getType()) {
    case SkRRect::kEmpty_Type: {
      break;
    }
    case SkRRect::kRect_Type: {
      ClipRect(view, clipSkRRect.rect());
      return;
    }
    case SkRRect::kOval_Type:
    case SkRRect::kSimple_Type: {
      CGRect clipRect = GetCGRectFromSkRect(clipSkRRect.rect());
      pathRef = CGPathCreateWithRoundedRect(clipRect, clipSkRRect.getSimpleRadii().x(),
                                            clipSkRRect.getSimpleRadii().y(), nil);
      break;
    }
    case SkRRect::kNinePatch_Type:
    case SkRRect::kComplex_Type: {
      CGMutablePathRef mutablePathRef = CGPathCreateMutable();
      // Complex types, we manually add cornors.
      SkRect clipSkRect = clipSkRRect.rect();
      SkVector topLeftRadii = clipSkRRect.radii(SkRRect::kUpperLeft_Corner);
      SkVector topRightRadii = clipSkRRect.radii(SkRRect::kUpperRight_Corner);
      SkVector bottomRightRadii = clipSkRRect.radii(SkRRect::kLowerRight_Corner);
      SkVector bottomLeftRadii = clipSkRRect.radii(SkRRect::kLowerLeft_Corner);

      // Start drawing RRect
      // Move point to the top left cornor adding the top left radii's x.
      CGPathMoveToPoint(mutablePathRef, nil, clipSkRect.fLeft + topLeftRadii.x(), clipSkRect.fTop);
      // Move point horizontally right to the top right cornor and add the top right curve.
      CGPathAddLineToPoint(mutablePathRef, nil, clipSkRect.fRight - topRightRadii.x(),
                           clipSkRect.fTop);
      CGPathAddCurveToPoint(mutablePathRef, nil, clipSkRect.fRight, clipSkRect.fTop,
                            clipSkRect.fRight, clipSkRect.fTop + topRightRadii.y(),
                            clipSkRect.fRight, clipSkRect.fTop + topRightRadii.y());
      // Move point vertically down to the bottom right cornor and add the bottom right curve.
      CGPathAddLineToPoint(mutablePathRef, nil, clipSkRect.fRight,
                           clipSkRect.fBottom - bottomRightRadii.y());
      CGPathAddCurveToPoint(mutablePathRef, nil, clipSkRect.fRight, clipSkRect.fBottom,
                            clipSkRect.fRight - bottomRightRadii.x(), clipSkRect.fBottom,
                            clipSkRect.fRight - bottomRightRadii.x(), clipSkRect.fBottom);
      // Move point horizontally left to the bottom left cornor and add the bottom left curve.
      CGPathAddLineToPoint(mutablePathRef, nil, clipSkRect.fLeft + bottomLeftRadii.x(),
                           clipSkRect.fBottom);
      CGPathAddCurveToPoint(mutablePathRef, nil, clipSkRect.fLeft, clipSkRect.fBottom,
                            clipSkRect.fLeft, clipSkRect.fBottom - bottomLeftRadii.y(),
                            clipSkRect.fLeft, clipSkRect.fBottom - bottomLeftRadii.y());
      // Move point vertically up to the top left cornor and add the top left curve.
      CGPathAddLineToPoint(mutablePathRef, nil, clipSkRect.fLeft,
                           clipSkRect.fTop + topLeftRadii.y());
      CGPathAddCurveToPoint(mutablePathRef, nil, clipSkRect.fLeft, clipSkRect.fTop,
                            clipSkRect.fLeft + topLeftRadii.x(), clipSkRect.fTop,
                            clipSkRect.fLeft + topLeftRadii.x(), clipSkRect.fTop);
      CGPathCloseSubpath(mutablePathRef);

      pathRef = mutablePathRef;
      break;
    }
  }
  // TODO(cyanglaz): iOS seems does not support hard edge on CAShapeLayer. It clearly stated that
  // the CAShaperLayer will be drawn antialiased. Need to figure out a way to do the hard edge
  // clipping on iOS.
  CAShapeLayer* clip = [[CAShapeLayer alloc] init];
  clip.path = pathRef;
  view.layer.mask = clip;
  CGPathRelease(pathRef);
}

static void PerformClip(UIView* view,
                        flutter::FlutterEmbededViewTransformType type,
                        const SkRect& rect,
                        const SkRRect& rrect,
                        const SkPath& path) {
  FML_CHECK(type == flutter::clip_rect || type == flutter::clip_rrect ||
            type == flutter::clip_path);
  switch (type) {
    case flutter::clip_rect:
      ClipRect(view, rect);
      break;
    case flutter::clip_rrect:
      ClipRRect(view, rrect);
      break;
    case flutter::clip_path:
      // TODO(cyanglaz): Add clip path
      break;
    default:
      break;
  }
}

static CATransform3D GetCATransform3DFromSkMatrix(const SkMatrix& matrix) {
  CGFloat screenScale = [UIScreen mainScreen].scale;
  CATransform3D transform = CATransform3DIdentity;
  transform.m11 = matrix.getScaleX();
  transform.m21 = matrix.getSkewX();
  transform.m41 = matrix.getTranslateX()/screenScale;
  transform.m14 = matrix.getPerspX();

  transform.m12 = matrix.getSkewY();
  transform.m22 = matrix.getScaleY();
  transform.m42 = matrix.getTranslateY()/screenScale;
  transform.m24 = matrix.getPerspY();
  return transform;
}

static void SetAnchor(CALayer *layer, CGPoint transformPosition) {
  NSLog(@"subview original in view %@", @(transformPosition));
  NSLog(@"current position %@", @(layer.position));
  layer.anchorPoint = CGPointMake(transformPosition.x/layer.bounds.size.width, transformPosition.y/layer.bounds.size.height);
  layer.position = transformPosition;
  NSLog(@"transform anchor %@", @(layer.anchorPoint));
  NSLog(@"transform position %@", @(layer.position));
}

namespace flutter {

void FlutterPlatformViewsController::SetFlutterView(UIView* flutter_view) {
  flutter_view_.reset([flutter_view retain]);
}

void FlutterPlatformViewsController::SetFlutterViewController(
    UIViewController* flutter_view_controller) {
  flutter_view_controller_.reset([flutter_view_controller retain]);
}

void FlutterPlatformViewsController::OnMethodCall(FlutterMethodCall* call, FlutterResult& result) {
  if ([[call method] isEqualToString:@"create"]) {
    OnCreate(call, result);
  } else if ([[call method] isEqualToString:@"dispose"]) {
    OnDispose(call, result);
  } else if ([[call method] isEqualToString:@"acceptGesture"]) {
    OnAcceptGesture(call, result);
  } else if ([[call method] isEqualToString:@"rejectGesture"]) {
    OnRejectGesture(call, result);
  } else {
    result(FlutterMethodNotImplemented);
  }
}

void FlutterPlatformViewsController::OnCreate(FlutterMethodCall* call, FlutterResult& result) {
  if (!flutter_view_.get()) {
    // Right now we assume we have a reference to FlutterView when creating a new view.
    // TODO(amirh): support this by setting the reference to FlutterView when it becomes available.
    // https://github.com/flutter/flutter/issues/23787
    result([FlutterError errorWithCode:@"create_failed"
                               message:@"can't create a view on a headless engine"
                               details:nil]);
    return;
  }
  NSDictionary<NSString*, id>* args = [call arguments];

  long viewId = [args[@"id"] longValue];
  std::string viewType([args[@"viewType"] UTF8String]);

  if (views_.count(viewId) != 0) {
    result([FlutterError errorWithCode:@"recreating_view"
                               message:@"trying to create an already created view"
                               details:[NSString stringWithFormat:@"view id: '%ld'", viewId]]);
  }

  NSObject<FlutterPlatformViewFactory>* factory = factories_[viewType].get();
  if (factory == nil) {
    result([FlutterError errorWithCode:@"unregistered_view_type"
                               message:@"trying to create a view with an unregistered type"
                               details:[NSString stringWithFormat:@"unregistered view type: '%@'",
                                                                  args[@"viewType"]]]);
    return;
  }

  id params = nil;
  if ([factory respondsToSelector:@selector(createArgsCodec)]) {
    NSObject<FlutterMessageCodec>* codec = [factory createArgsCodec];
    if (codec != nil && args[@"params"] != nil) {
      FlutterStandardTypedData* paramsData = args[@"params"];
      params = [codec decode:paramsData.data];
    }
  }

  NSObject<FlutterPlatformView>* embedded_view = [factory createWithFrame:CGRectZero
                                                           viewIdentifier:viewId
                                                                arguments:params];
  views_[viewId] = fml::scoped_nsobject<NSObject<FlutterPlatformView>>([embedded_view retain]);

  FlutterTouchInterceptingView* touch_interceptor = [[[FlutterTouchInterceptingView alloc]
       initWithEmbeddedView:embedded_view.view
      flutterViewController:flutter_view_controller_.get()] autorelease];

  touch_interceptors_[viewId] =
      fml::scoped_nsobject<FlutterTouchInterceptingView>([touch_interceptor retain]);
  root_views_[viewId] = fml::scoped_nsobject<UIView>([touch_interceptor retain]);

  result(nil);
}

void FlutterPlatformViewsController::OnDispose(FlutterMethodCall* call, FlutterResult& result) {
  NSNumber* arg = [call arguments];
  int64_t viewId = [arg longLongValue];

  if (views_.count(viewId) == 0) {
    result([FlutterError errorWithCode:@"unknown_view"
                               message:@"trying to dispose an unknown"
                               details:[NSString stringWithFormat:@"view id: '%lld'", viewId]]);
    return;
  }
  // We wait for next submitFrame to dispose views.
  views_to_dispose_.insert(viewId);
  result(nil);
}

void FlutterPlatformViewsController::OnAcceptGesture(FlutterMethodCall* call,
                                                     FlutterResult& result) {
  NSDictionary<NSString*, id>* args = [call arguments];
  int64_t viewId = [args[@"id"] longLongValue];

  if (views_.count(viewId) == 0) {
    result([FlutterError errorWithCode:@"unknown_view"
                               message:@"trying to set gesture state for an unknown view"
                               details:[NSString stringWithFormat:@"view id: '%lld'", viewId]]);
    return;
  }

  FlutterTouchInterceptingView* view = touch_interceptors_[viewId].get();
  [view releaseGesture];

  result(nil);
}

void FlutterPlatformViewsController::OnRejectGesture(FlutterMethodCall* call,
                                                     FlutterResult& result) {
  NSDictionary<NSString*, id>* args = [call arguments];
  int64_t viewId = [args[@"id"] longLongValue];

  if (views_.count(viewId) == 0) {
    result([FlutterError errorWithCode:@"unknown_view"
                               message:@"trying to set gesture state for an unknown view"
                               details:[NSString stringWithFormat:@"view id: '%lld'", viewId]]);
    return;
  }

  FlutterTouchInterceptingView* view = touch_interceptors_[viewId].get();
  [view blockGesture];

  result(nil);
}

void FlutterPlatformViewsController::RegisterViewFactory(
    NSObject<FlutterPlatformViewFactory>* factory,
    NSString* factoryId) {
  std::string idString([factoryId UTF8String]);
  FML_CHECK(factories_.count(idString) == 0);
  factories_[idString] =
      fml::scoped_nsobject<NSObject<FlutterPlatformViewFactory>>([factory retain]);
}

void FlutterPlatformViewsController::SetFrameSize(SkISize frame_size) {
  frame_size_ = frame_size;
}

void FlutterPlatformViewsController::PrerollCompositeEmbeddedView(int view_id) {
  picture_recorders_[view_id] = std::make_unique<SkPictureRecorder>();
  picture_recorders_[view_id]->beginRecording(SkRect::Make(frame_size_));
  picture_recorders_[view_id]->getRecordingCanvas()->clear(SK_ColorTRANSPARENT);
  composition_order_.push_back(view_id);
}

NSObject<FlutterPlatformView>* FlutterPlatformViewsController::GetPlatformViewByID(int view_id) {
  if (views_.empty()) {
    return nil;
  }
  return views_[view_id].get();
}

std::vector<SkCanvas*> FlutterPlatformViewsController::GetCurrentCanvases() {
  std::vector<SkCanvas*> canvases;
  for (size_t i = 0; i < composition_order_.size(); i++) {
    int64_t view_id = composition_order_[i];
    canvases.push_back(picture_recorders_[view_id]->getRecordingCanvas());
  }
  return canvases;
}

void FlutterPlatformViewsController::CompositeWithParams(
    int view_id,
    const flutter::EmbeddedViewParams& params) {
  UIView* touch_interceptor = touch_interceptors_[view_id].get();

  CGFloat screenScale = [UIScreen mainScreen].scale;
  CGRect frame = CGRectMake(0, 0, params.sizePoints.width(), params.sizePoints.height());
  touch_interceptor.frame = frame;
  UIView* lastView = touch_interceptor;
  CGPoint transformAnchor = CGPointZero;
  lastView.layer.transform = CATransform3DIdentity;
  SetAnchor(lastView.layer, transformAnchor);
  // Reverse transform based on screen scale.
  // We needed to do that because in our layer tree, we have a transform layer at the root to transform based on the sreen scale.
  std::vector<FlutterEmbededViewTransformElement>::reverse_iterator iter = params.transformStack->rbegin();
  int64_t clipCount = 0;
  while (iter != params.transformStack->rend()) {
    switch (iter->type()) {
      case transform: {
        CATransform3D transform = GetCATransform3DFromSkMatrix(iter->matrix());
        lastView.layer.transform = CATransform3DConcat(lastView.layer.transform, transform);
        break;
      }
      case clip_rect:
      case clip_rrect:
      case clip_path: {
        UIView* view = lastView.superview;
        // if we need more clips operations than last time, create a new view.
        ++clipCount;
        if (clipCount > clip_count_[view_id]) {
          [lastView removeFromSuperview];
          view = [[UIView alloc] initWithFrame:lastView.bounds];
          [view addSubview:lastView];
        }
        PerformClip(view, iter->type(), iter->rect(), iter->rrect(), iter->path());

        CGPoint transformPosition = [view convertPoint:touch_interceptor.bounds.origin fromView:touch_interceptor];
        SetAnchor(view.layer, transformPosition);

        lastView = view;
        break;
      }
    }
    NSLog(@"last view frame %@", @(lastView.frame));
    ++iter;
  }
  lastView.layer.transform = CATransform3DMakeScale(1/screenScale, 1/screenScale, 1);

  // If we have less cilp operations this time, remove unnecessary views.
  // We skip this process if we have more clip operations this time.
  // TODO TEST(cyanglaz):
  int64_t extraClipsFromLastTime = clip_count_[view_id] - clipCount;
  if (extraClipsFromLastTime > 0 && lastView.superview) {
    [lastView removeFromSuperview];
    UIView* superview = lastView.superview;
    while (extraClipsFromLastTime > 0) {
      UIView* superSuperView = superview.superview;
      [superview removeFromSuperview];
      [superview release];
      superview = superSuperView;
    }
    [flutter_view_.get() addSubview:lastView];
  }
  clip_count_[view_id] = clipCount;
  // If we have clips, replace root view with the top parent view.
  if (lastView != touch_interceptor) {
    root_views_[view_id] = fml::scoped_nsobject<UIView>([lastView retain]);
  }
}

SkCanvas* FlutterPlatformViewsController::CompositeEmbeddedView(
    int view_id,
    const flutter::EmbeddedViewParams& params) {
  // TODO(amirh): assert that this is running on the platform thread once we support the iOS
  // embedded views thread configuration.

  // Do nothing if the params didn't change.
  if (composite_params_.count(view_id) == 1 && composite_params_[view_id] == params) {
    return picture_recorders_[view_id]->getRecordingCanvas();
  }
  composite_params_[view_id] = params;
  CompositeWithParams(view_id, params);

  return picture_recorders_[view_id]->getRecordingCanvas();
}

void FlutterPlatformViewsController::Reset() {
  UIView* flutter_view = flutter_view_.get();
  for (UIView* sub_view in [flutter_view subviews]) {
    [sub_view removeFromSuperview];
  }
  views_.clear();
  overlays_.clear();
  composition_order_.clear();
  active_composition_order_.clear();
  picture_recorders_.clear();
  composite_params_.clear();
  clip_count_.clear();
}

bool FlutterPlatformViewsController::SubmitFrame(bool gl_rendering,
                                                 GrContext* gr_context,
                                                 std::shared_ptr<IOSGLContext> gl_context) {
  DisposeViews();

  bool did_submit = true;
  for (size_t i = 0; i < composition_order_.size(); i++) {
    int64_t view_id = composition_order_[i];
    if (gl_rendering) {
      EnsureGLOverlayInitialized(view_id, gl_context, gr_context);
    } else {
      EnsureOverlayInitialized(view_id);
    }
    auto frame = overlays_[view_id]->surface->AcquireFrame(frame_size_);
    SkCanvas* canvas = frame->SkiaCanvas();
    canvas->drawPicture(picture_recorders_[view_id]->finishRecordingAsPicture());
    canvas->flush();
    did_submit &= frame->Submit();
  }
  picture_recorders_.clear();
  if (composition_order_ == active_composition_order_) {
    composition_order_.clear();
    return did_submit;
  }
  DetachUnusedLayers();
  active_composition_order_.clear();
  UIView* flutter_view = flutter_view_.get();

  for (size_t i = 0; i < composition_order_.size(); i++) {
    int view_id = composition_order_[i];
    UIView* root = root_views_[view_id].get();
    UIView* overlay = overlays_[view_id]->overlay_view;
    FML_CHECK(root.superview == overlay.superview);
    if (root.superview == flutter_view) {
      [flutter_view bringSubviewToFront:root];
      [flutter_view bringSubviewToFront:overlay];
    } else {
      [flutter_view addSubview:root];
      [flutter_view addSubview:overlay];
      NSLog(@"root view frame %@", @(root.frame));
    }

    active_composition_order_.push_back(view_id);
  }
  composition_order_.clear();
  return did_submit;
}

void FlutterPlatformViewsController::DetachUnusedLayers() {
  std::unordered_set<int64_t> composition_order_set;

  for (int64_t view_id : composition_order_) {
    composition_order_set.insert(view_id);
  }

  for (int64_t view_id : active_composition_order_) {
    if (composition_order_set.find(view_id) == composition_order_set.end()) {
      if (root_views_.find(view_id) == root_views_.end()) {
        continue;
      }
      UIView* root = root_views_[view_id].get();
      [root removeFromSuperview];
      [overlays_[view_id]->overlay_view.get() removeFromSuperview];
    }
  }
}

void FlutterPlatformViewsController::DisposeViews() {
  if (views_to_dispose_.empty()) {
    return;
  }

  for (int64_t viewId : views_to_dispose_) {
    UIView* root_view = root_views_[viewId].get();
    [root_view removeFromSuperview];
    views_.erase(viewId);
    touch_interceptors_.erase(viewId);
    root_views_.erase(viewId);
    overlays_.erase(viewId);
    composite_params_.erase(viewId);
    clip_count_.erase(viewId);
  }
  views_to_dispose_.clear();
}

void FlutterPlatformViewsController::EnsureOverlayInitialized(int64_t overlay_id) {
  if (overlays_.count(overlay_id) != 0) {
    return;
  }
  FlutterOverlayView* overlay_view = [[FlutterOverlayView alloc] init];
  overlay_view.frame = flutter_view_.get().bounds;
  overlay_view.autoresizingMask =
      (UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight);
  std::unique_ptr<IOSSurface> ios_surface = overlay_view.createSoftwareSurface;
  std::unique_ptr<Surface> surface = ios_surface->CreateGPUSurface();
  overlays_[overlay_id] = std::make_unique<FlutterPlatformViewLayer>(
      fml::scoped_nsobject<UIView>(overlay_view), std::move(ios_surface), std::move(surface));
}

void FlutterPlatformViewsController::EnsureGLOverlayInitialized(
    int64_t overlay_id,
    std::shared_ptr<IOSGLContext> gl_context,
    GrContext* gr_context) {
  if (overlays_.count(overlay_id) != 0) {
    if (gr_context != overlays_gr_context_) {
      overlays_gr_context_ = gr_context;
      // The overlay already exists, but the GrContext was changed so we need to recreate
      // the rendering surface with the new GrContext.
      IOSSurfaceGL* ios_surface_gl = (IOSSurfaceGL*)overlays_[overlay_id]->ios_surface.get();
      std::unique_ptr<Surface> surface = ios_surface_gl->CreateSecondaryGPUSurface(gr_context);
      overlays_[overlay_id]->surface = std::move(surface);
    }
    return;
  }
  auto contentsScale = flutter_view_.get().layer.contentsScale;
  FlutterOverlayView* overlay_view =
      [[FlutterOverlayView alloc] initWithContentsScale:contentsScale];
  overlay_view.frame = flutter_view_.get().bounds;
  overlay_view.autoresizingMask =
      (UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight);
  std::unique_ptr<IOSSurfaceGL> ios_surface =
      [overlay_view createGLSurfaceWithContext:std::move(gl_context)];
  std::unique_ptr<Surface> surface = ios_surface->CreateSecondaryGPUSurface(gr_context);
  overlays_[overlay_id] = std::make_unique<FlutterPlatformViewLayer>(
      fml::scoped_nsobject<UIView>(overlay_view), std::move(ios_surface), std::move(surface));
  overlays_gr_context_ = gr_context;
}

}  // namespace flutter

// This recognizers delays touch events from being dispatched to the responder chain until it failed
// recognizing a gesture.
//
// We only fail this recognizer when asked to do so by the Flutter framework (which does so by
// invoking an acceptGesture method on the platform_views channel). And this is how we allow the
// Flutter framework to delay or prevent the embedded view from getting a touch sequence.
@interface DelayingGestureRecognizer : UIGestureRecognizer <UIGestureRecognizerDelegate>
- (instancetype)initWithTarget:(id)target
                        action:(SEL)action
          forwardingRecognizer:(UIGestureRecognizer*)forwardingRecognizer;
@end

// While the DelayingGestureRecognizer is preventing touches from hitting the responder chain
// the touch events are not arriving to the FlutterView (and thus not arriving to the Flutter
// framework). We use this gesture recognizer to dispatch the events directly to the FlutterView
// while during this phase.
//
// If the Flutter framework decides to dispatch events to the embedded view, we fail the
// DelayingGestureRecognizer which sends the events up the responder chain. But since the events
// are handled by the embedded view they are not delivered to the Flutter framework in this phase
// as well. So during this phase as well the ForwardingGestureRecognizer dispatched the events
// directly to the FlutterView.
@interface ForwardingGestureRecognizer : UIGestureRecognizer <UIGestureRecognizerDelegate>
- (instancetype)initWithTarget:(id)target
         flutterViewController:(UIViewController*)flutterViewController;
@end

@implementation FlutterTouchInterceptingView {
  fml::scoped_nsobject<DelayingGestureRecognizer> _delayingRecognizer;
}
- (instancetype)initWithEmbeddedView:(UIView*)embeddedView
               flutterViewController:(UIViewController*)flutterViewController {
  self = [super initWithFrame:embeddedView.frame];
  if (self) {
    self.multipleTouchEnabled = YES;
    embeddedView.autoresizingMask =
        (UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight);

    [self addSubview:embeddedView];

    ForwardingGestureRecognizer* forwardingRecognizer =
        [[[ForwardingGestureRecognizer alloc] initWithTarget:self
                                       flutterViewController:flutterViewController] autorelease];

    _delayingRecognizer.reset([[DelayingGestureRecognizer alloc]
              initWithTarget:self
                      action:nil
        forwardingRecognizer:forwardingRecognizer]);

    [self addGestureRecognizer:_delayingRecognizer.get()];
    [self addGestureRecognizer:forwardingRecognizer];
  }
  return self;
}

- (void)releaseGesture {
  _delayingRecognizer.get().state = UIGestureRecognizerStateFailed;
}

- (void)blockGesture {
  _delayingRecognizer.get().state = UIGestureRecognizerStateEnded;
}

// We want the intercepting view to consume the touches and not pass the touches up to the parent
// view. Make the touch event method not call super will not pass the touches up to the parent view.
// Hence we overide the touch event methods and do nothing.
- (void)touchesBegan:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event {
}

- (void)touchesMoved:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event {
}

- (void)touchesCancelled:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event {
}

- (void)touchesEnded:(NSSet*)touches withEvent:(UIEvent*)event {
}

@end

@implementation DelayingGestureRecognizer {
  fml::scoped_nsobject<UIGestureRecognizer> _forwardingRecognizer;
}

- (instancetype)initWithTarget:(id)target
                        action:(SEL)action
          forwardingRecognizer:(UIGestureRecognizer*)forwardingRecognizer {
  self = [super initWithTarget:target action:action];
  if (self) {
    self.delaysTouchesBegan = YES;
    self.delegate = self;
    _forwardingRecognizer.reset([forwardingRecognizer retain]);
  }
  return self;
}

- (BOOL)gestureRecognizer:(UIGestureRecognizer*)gestureRecognizer
    shouldBeRequiredToFailByGestureRecognizer:(UIGestureRecognizer*)otherGestureRecognizer {
  // The forwarding gesture recognizer should always get all touch events, so it should not be
  // required to fail by any other gesture recognizer.
  return otherGestureRecognizer != _forwardingRecognizer.get() && otherGestureRecognizer != self;
}

- (BOOL)gestureRecognizer:(UIGestureRecognizer*)gestureRecognizer
    shouldRequireFailureOfGestureRecognizer:(UIGestureRecognizer*)otherGestureRecognizer {
  return otherGestureRecognizer == self;
}

- (void)touchesCancelled:(NSSet*)touches withEvent:(UIEvent*)event {
  self.state = UIGestureRecognizerStateFailed;
}
@end

@implementation ForwardingGestureRecognizer {
  // We can't dispatch events to the framework without this back pointer.
  // This is a weak reference, the ForwardingGestureRecognizer is owned by the
  // FlutterTouchInterceptingView which is strong referenced only by the FlutterView,
  // which is strongly referenced by the FlutterViewController.
  // So this is safe as when FlutterView is deallocated the reference to ForwardingGestureRecognizer
  // will go away.
  UIViewController* _flutterViewController;
  // Counting the pointers that has started in one touch sequence.
  NSInteger _currentTouchPointersCount;
}

- (instancetype)initWithTarget:(id)target
         flutterViewController:(UIViewController*)flutterViewController {
  self = [super initWithTarget:target action:nil];
  if (self) {
    self.delegate = self;
    _flutterViewController = flutterViewController;
    _currentTouchPointersCount = 0;
  }
  return self;
}

- (void)touchesBegan:(NSSet*)touches withEvent:(UIEvent*)event {
  [_flutterViewController touchesBegan:touches withEvent:event];
  _currentTouchPointersCount += touches.count;
}

- (void)touchesMoved:(NSSet*)touches withEvent:(UIEvent*)event {
  [_flutterViewController touchesMoved:touches withEvent:event];
}

- (void)touchesEnded:(NSSet*)touches withEvent:(UIEvent*)event {
  [_flutterViewController touchesEnded:touches withEvent:event];
  _currentTouchPointersCount -= touches.count;
  // Touches in one touch sequence are sent to the touchesEnded method separately if different
  // fingers stop touching the screen at different time. So one touchesEnded method triggering does
  // not necessarially mean the touch sequence has ended. We Only set the state to
  // UIGestureRecognizerStateFailed when all the touches in the current touch sequence is ended.
  if (_currentTouchPointersCount == 0) {
    self.state = UIGestureRecognizerStateFailed;
  }
}

- (void)touchesCancelled:(NSSet*)touches withEvent:(UIEvent*)event {
  [_flutterViewController touchesCancelled:touches withEvent:event];
  _currentTouchPointersCount = 0;
  self.state = UIGestureRecognizerStateFailed;
}

- (BOOL)gestureRecognizer:(UIGestureRecognizer*)gestureRecognizer
    shouldRecognizeSimultaneouslyWithGestureRecognizer:
        (UIGestureRecognizer*)otherGestureRecognizer {
  return YES;
}
@end
