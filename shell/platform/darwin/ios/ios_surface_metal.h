// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_SHELL_PLATFORM_DARWIN_IOS_IOS_SURFACE_METAL_H_
#define FLUTTER_SHELL_PLATFORM_DARWIN_IOS_IOS_SURFACE_METAL_H_

#include "flutter/fml/macros.h"
#include "flutter/shell/gpu/gpu_surface_delegate.h"
#include "flutter/shell/platform/darwin/ios/ios_surface.h"

@class CAMetalLayer;

namespace flutter {

#if TARGET_IPHONE_SIMULATOR
class API_AVAILABLE(ios(13.0)) IOSSurfaceMetal final : public IOSSurface,
                                                       public GPUSurfaceDelegate {
#else
class IOSSurfaceMetal final : public IOSSurface, public GPUSurfaceDelegate {
#endif  // TARGET_IPHONE_SIMULATOR
 public:
  IOSSurfaceMetal(fml::scoped_nsobject<CAMetalLayer> layer,
                  std::shared_ptr<IOSContext> context,
                  FlutterPlatformViewsController* platform_views_controller);

  // |IOSSurface|
  ~IOSSurfaceMetal() override;

 private:
  fml::scoped_nsobject<CAMetalLayer> layer_;
  bool is_valid_ = false;

  // |IOSSurface|
  bool IsValid() const override;

  // |IOSSurface|
  void UpdateStorageSizeIfNecessary() override;

  // |IOSSurface|
  std::unique_ptr<Surface> CreateGPUSurface(GrContext* gr_context) override;

  // |GPUSurfaceDelegate|
  ExternalViewEmbedder* GetExternalViewEmbedder() override;

  FML_DISALLOW_COPY_AND_ASSIGN(IOSSurfaceMetal);
};

}  // namespace flutter

#endif  // FLUTTER_SHELL_PLATFORM_DARWIN_IOS_IOS_SURFACE_METAL_H_
