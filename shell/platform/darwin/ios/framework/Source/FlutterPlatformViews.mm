// Copyright 2018 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <map>
#include <memory>
#include <string>

#include "FlutterPlatformViews_Internal.h"
#include "flutter/fml/platform/darwin/scoped_nsobject.h"
#include "flutter/shell/platform/darwin/ios/framework/Headers/FlutterChannels.h"

#import <UIKit/UIGestureRecognizerSubclass.h>

namespace shell {

void FlutterPlatformViewsController::SetFlutterView(UIView* flutter_view) {
  flutter_view_.reset(flutter_view);
}

void FlutterPlatformViewsController::OnMethodCall(FlutterMethodCall* call, FlutterResult& result) {
  if ([[call method] isEqualToString:@"create"]) {
    OnCreate(call, result);
  } else if ([[call method] isEqualToString:@"dispose"]) {
    OnDispose(call, result);
  } else if ([[call method] isEqualToString:@"acceptGesture"]) {
    OnAcceptGesture(call, result);
  } else {
    result(FlutterMethodNotImplemented);
  }
}

void FlutterPlatformViewsController::OnCreate(FlutterMethodCall* call, FlutterResult& result) {
  if (!flutter_view_.get()) {
    // Right now we assume we have a reference to FlutterView when creating a new view.
    // TODO(amirh): support this by setting the refernce to FlutterView when it becomes available.
    // https://github.com/flutter/flutter/issues/23787
    result([FlutterError errorWithCode:@"create_failed"
                               message:@"can't create a view on a headless engine"
                               details:nil]);
    return;
  }
  NSDictionary<NSString*, id>* args = [call arguments];

  long viewId = [args[@"id"] longValue];
  std::string viewType([args[@"viewType"] UTF8String]);

  if (views_[viewId] != nil) {
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

  FlutterTouchInterceptingView* view = [[[FlutterTouchInterceptingView alloc]
      initWithEmbeddedView:[factory createWithFrame:CGRectZero
                                     viewIdentifier:viewId
                                          arguments:params]
               flutterView:flutter_view_] autorelease];
  views_[viewId] = fml::scoped_nsobject<FlutterTouchInterceptingView>([view retain]);

  result(nil);
}

void FlutterPlatformViewsController::OnDispose(FlutterMethodCall* call, FlutterResult& result) {
  NSDictionary<NSString*, id>* args = [call arguments];
  int64_t viewId = [args[@"id"] longLongValue];

  if (views_[viewId] == nil) {
    result([FlutterError errorWithCode:@"unknown_view"
                               message:@"trying to dispose an unknown"
                               details:[NSString stringWithFormat:@"view id: '%lld'", viewId]]);
    return;
  }

  UIView* view = views_[viewId].get();
  [view removeFromSuperview];
  views_.erase(viewId);
  result(nil);
}

void FlutterPlatformViewsController::OnAcceptGesture(FlutterMethodCall* call,
                                                     FlutterResult& result) {
  NSDictionary<NSString*, id>* args = [call arguments];
  int64_t viewId = [args[@"id"] longLongValue];

  if (views_[viewId] == nil) {
    result([FlutterError errorWithCode:@"unknown_view"
                               message:@"trying to set gesture state for an unknown view"
                               details:[NSString stringWithFormat:@"view id: '%lld'", viewId]]);
    return;
  }

  FlutterTouchInterceptingView* view = views_[viewId].get();
  [view releaseGesture];

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

void FlutterPlatformViewsController::CompositeEmbeddedView(int view_id,
                                                           const flow::EmbeddedViewParams& params) {
  // TODO(amirh): assert that this is running on the platform thread once we support the iOS
  // embedded views thread configuration.
  // TODO(amirh): do nothing if the params didn't change.
  CGFloat screenScale = [[UIScreen mainScreen] scale];
  CGRect rect =
      CGRectMake(params.offsetPixels.x() / screenScale, params.offsetPixels.y() / screenScale,
                 params.sizePoints.width(), params.sizePoints.height());

  UIView* view = views_[view_id];
  [view setFrame:rect];
  composition_order_.push_back(view_id);
}

void FlutterPlatformViewsController::Present() {
  if (composition_order_ == active_composition_order_) {
    composition_order_.clear();
    return;
  }
  UIView* flutter_view = flutter_view_.get();

  // This can be more efficient, instead of removing all views and then re-attaching them,
  // we should only remove the views that has been completly removed from the layer tree, and
  // reorder the views using UIView's bringSubviewToFront.
  // TODO(amirh): make this more efficient.
  // https://github.com/flutter/flutter/issues/23793
  for (UIView* sub_view in [flutter_view subviews]) {
    [sub_view removeFromSuperview];
  }

  active_composition_order_.clear();
  for (size_t i = 0; i < composition_order_.size(); i++) {
    int view_id = composition_order_[i];
    [flutter_view addSubview:views_[view_id].get()];
    active_composition_order_.push_back(view_id);
  }

  composition_order_.clear();
}

}  // namespace shell

// This recognizers delays touch events from being dispatched to the responder chain until it failed
// recognizing a gesture.
//
// We only fail this recognizer when asked to do so by the Flutter framework (which does so by
// invoking an acceptGesture method on the platform_views channel). And this is how we allow the
// Flutter framework to delay or prevent the embedded view from getting a touch sequence.
@interface DelayingGestureRecognizer : UIGestureRecognizer <UIGestureRecognizerDelegate>
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
- (instancetype)initWithTarget:(id)target flutterView:(UIView*)flutterView;
@end

@implementation FlutterTouchInterceptingView {
  fml::scoped_nsobject<DelayingGestureRecognizer> _delayingRecognizer;
}
- (instancetype)initWithEmbeddedView:(UIView*)embeddedView flutterView:(UIView*)flutterView {
  self = [super initWithFrame:embeddedView.frame];
  if (self) {
    self.multipleTouchEnabled = YES;
    embeddedView.autoresizingMask =
        (UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight);

    [self addSubview:embeddedView];

    ForwardingGestureRecognizer* forwardingRecognizer =
        [[[ForwardingGestureRecognizer alloc] initWithTarget:self
                                                 flutterView:flutterView] autorelease];

    _delayingRecognizer.reset([[DelayingGestureRecognizer alloc] initWithTarget:self action:nil]);

    [self addGestureRecognizer:_delayingRecognizer.get()];
    [self addGestureRecognizer:forwardingRecognizer];
  }
  return self;
}

- (void)releaseGesture {
  _delayingRecognizer.get().state = UIGestureRecognizerStateFailed;
}
@end

@implementation DelayingGestureRecognizer
- (instancetype)initWithTarget:(id)target action:(SEL)action {
  self = [super initWithTarget:target action:action];
  if (self) {
    self.delaysTouchesBegan = YES;
    self.delegate = self;
  }
  return self;
}

- (BOOL)gestureRecognizer:(UIGestureRecognizer*)gestureRecognizer
    shouldBeRequiredToFailByGestureRecognizer:(UIGestureRecognizer*)otherGestureRecognizer {
  return otherGestureRecognizer != self;
}

- (BOOL)gestureRecognizer:(UIGestureRecognizer*)gestureRecognizer
    shouldRequireFailureOfGestureRecognizer:(UIGestureRecognizer*)otherGestureRecognizer {
  return otherGestureRecognizer == self;
}

- (void)touchesEnded:(NSSet*)touches withEvent:(UIEvent*)event {
  // The gesture has ended, and the delaying gesture recognizer was not failed, we recognize
  // the gesture to prevent the touches from being dispatched to the embedded view.
  //
  // This doesn't work well with gestures that are recognized by the Flutter framework after
  // all pointers are up.
  //
  // TODO(amirh): explore if we can instead set this to recognized when the next touch sequence
  //  begins, or we can use a framework signal for restarting the recognizers (e.g when the
  //  gesture arena is resolved).
  self.state = UIGestureRecognizerStateRecognized;
}

- (void)touchesCancelled:(NSSet*)touches withEvent:(UIEvent*)event {
  self.state = UIGestureRecognizerStateRecognized;
}
@end

@implementation ForwardingGestureRecognizer {
  // We can't dispatch events to the framework without this back pointer.
  // This is a weak reference, the ForwardingGestureRecognizer is owned by the
  // FlutterTouchInterceptingView which is strong referenced only by the FlutterView.
  // So this is safe as when FlutterView is deallocated the reference to ForwardingGestureRecognizer
  // will go away.
  UIView* _flutterView;
}

- (instancetype)initWithTarget:(id)target flutterView:(UIView*)flutterView {
  self = [super initWithTarget:target action:nil];
  if (self) {
    self.delegate = self;
    _flutterView = flutterView;
  }
  return self;
}

- (void)touchesBegan:(NSSet*)touches withEvent:(UIEvent*)event {
  [_flutterView touchesBegan:touches withEvent:event];
}

- (void)touchesMoved:(NSSet*)touches withEvent:(UIEvent*)event {
  [_flutterView touchesMoved:touches withEvent:event];
}

- (void)touchesEnded:(NSSet*)touches withEvent:(UIEvent*)event {
  [_flutterView touchesEnded:touches withEvent:event];
  self.state = UIGestureRecognizerStateRecognized;
}

- (void)touchesCancelled:(NSSet*)touches withEvent:(UIEvent*)event {
  [_flutterView touchesCancelled:touches withEvent:event];
  self.state = UIGestureRecognizerStateRecognized;
}

- (BOOL)gestureRecognizer:(UIGestureRecognizer*)gestureRecognizer
    shouldRecognizeSimultaneouslyWithGestureRecognizer:
        (UIGestureRecognizer*)otherGestureRecognizer {
  return YES;
}
@end
