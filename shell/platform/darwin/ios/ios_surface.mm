// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/darwin/ios/ios_surface.h"

#include "flutter/shell/platform/darwin/ios/ios_surface_gl.h"
#include "flutter/shell/platform/darwin/ios/ios_surface_software.h"

#if FLUTTER_SHELL_ENABLE_METAL
#include "flutter/shell/platform/darwin/ios/ios_surface_metal.h"
#endif  // FLUTTER_SHELL_ENABLE_METAL

namespace flutter {

// The name of the Info.plist flag to enable the embedded iOS views preview.
constexpr const char* kEmbeddedViewsPreview = "io.flutter.embedded_views_preview";

bool IsIosEmbeddedViewsPreviewEnabled() {
  static bool preview_enabled =
      [[[NSBundle mainBundle] objectForInfoDictionaryKey:@(kEmbeddedViewsPreview)] boolValue];
  return preview_enabled;
}

std::unique_ptr<IOSSurface> IOSSurface::Create(
    std::shared_ptr<IOSContext> context,
    fml::scoped_nsobject<CALayer> layer,
    FlutterPlatformViewsController* platform_views_controller) {
  FML_DCHECK(layer);
  FML_DCHECK(context);

  if ([layer.get() isKindOfClass:[CAEAGLLayer class]]) {
    return std::make_unique<IOSSurfaceGL>(
        fml::scoped_nsobject<CAEAGLLayer>(
            reinterpret_cast<CAEAGLLayer*>([layer.get() retain])),  // EAGL layer
        std::move(context),                                         // context
        platform_views_controller                                   // platform views controller
    );
  }

#if FLUTTER_SHELL_ENABLE_METAL
  if ([layer.get() isKindOfClass:[CAMetalLayer class]]) {
    return std::make_unique<IOSSurfaceMetal>(
        fml::scoped_nsobject<CAMetalLayer>(
            reinterpret_cast<CAMetalLayer*>([layer.get() retain])),  // Metal layer
        std::move(context),                                          // context
        platform_views_controller                                    // platform views controller
    );
  }
#endif  // FLUTTER_SHELL_ENABLE_METAL

  return std::make_unique<IOSSurfaceSoftware>(
      std::move(layer),          // layer
      std::move(context),        // context
      platform_views_controller  // platform views controller
  );
}

IOSSurface::IOSSurface(std::shared_ptr<IOSContext> ios_context,
                       FlutterPlatformViewsController* platform_views_controller)
    : ios_context_(std::move(ios_context)), platform_views_controller_(platform_views_controller) {
  FML_DCHECK(ios_context_);
}

IOSSurface::~IOSSurface() = default;

std::shared_ptr<IOSContext> IOSSurface::GetContext() const {
  return ios_context_;
}

// |ExternalViewEmbedder|
SkCanvas* IOSSurface::GetRootCanvas() {
  // On iOS, the root surface is created from the on-screen render target. Only the surfaces for the
  // various overlays are controlled by this class.
  return nullptr;
}

ExternalViewEmbedder* IOSSurface::GetExternalViewEmbedderIfEnabled() {
  if (IsIosEmbeddedViewsPreviewEnabled()) {
    return this;
  } else {
    return nullptr;
  }
}

// |ExternalViewEmbedder|
void IOSSurface::CancelFrame() {
  TRACE_EVENT0("flutter", "IOSSurface::CancelFrame");
  FML_CHECK(platform_views_controller_ != nullptr);
  platform_views_controller_->CancelFrame();
  // Committing the current transaction as |BeginFrame| will create a nested
  // CATransaction otherwise.
  if ([NSThread currentThread] == [NSThread mainThread]) {
    [CATransaction commit];
  }
}

// |ExternalViewEmbedder|
void IOSSurface::BeginFrame(SkISize frame_size, GrContext* context, double device_pixel_ratio) {
  TRACE_EVENT0("flutter", "IOSSurface::BeginFrame");
  FML_CHECK(platform_views_controller_ != nullptr);
  platform_views_controller_->SetFrameSize(frame_size);
  if ([NSThread currentThread] == [NSThread mainThread]) {
    [CATransaction begin];
  }
}

// |ExternalViewEmbedder|
void IOSSurface::PrerollCompositeEmbeddedView(int view_id,
                                              std::unique_ptr<EmbeddedViewParams> params) {
  TRACE_EVENT0("flutter", "IOSSurface::PrerollCompositeEmbeddedView");

  FML_CHECK(platform_views_controller_ != nullptr);
  platform_views_controller_->PrerollCompositeEmbeddedView(view_id, std::move(params));
}

// |ExternalViewEmbedder|
PostPrerollResult IOSSurface::PostPrerollAction(
    fml::RefPtr<fml::GpuThreadMerger> gpu_thread_merger) {
  TRACE_EVENT0("flutter", "IOSSurface::PostPrerollAction");
  FML_CHECK(platform_views_controller_ != nullptr);
  return platform_views_controller_->PostPrerollAction(gpu_thread_merger);
}

// |ExternalViewEmbedder|
std::vector<SkCanvas*> IOSSurface::GetCurrentCanvases() {
  FML_CHECK(platform_views_controller_ != nullptr);
  return platform_views_controller_->GetCurrentCanvases();
}

// |ExternalViewEmbedder|
SkCanvas* IOSSurface::CompositeEmbeddedView(int view_id) {
  TRACE_EVENT0("flutter", "IOSSurface::CompositeEmbeddedView");
  FML_CHECK(platform_views_controller_ != nullptr);
  return platform_views_controller_->CompositeEmbeddedView(view_id);
}

// |ExternalViewEmbedder|
bool IOSSurface::SubmitFrame(GrContext* context, SkCanvas* background_canvas) {
  TRACE_EVENT0("flutter", "IOSSurface::SubmitFrame");
  FML_CHECK(platform_views_controller_ != nullptr);
  bool submitted =
      platform_views_controller_->SubmitFrame(std::move(context), ios_context_, background_canvas);
  return submitted;
}

// |ExternalViewEmbedder|
void IOSSurface::EndFrame(fml::RefPtr<fml::GpuThreadMerger> gpu_thread_merger) {
  TRACE_EVENT0("flutter", "IOSSurface::EndFrame");
  FML_CHECK(platform_views_controller_ != nullptr);
  return platform_views_controller_->EndFrame(gpu_thread_merger);
}

// |ExternalViewEmbedder|
void IOSSurface::FinishFrame() {
  TRACE_EVENT0("flutter", "IOSSurface::DidSubmitFrame");
  if ([NSThread currentThread] != [NSThread mainThread]) {
    return;
  }
  [CATransaction commit];
}
}  // namespace flutter
