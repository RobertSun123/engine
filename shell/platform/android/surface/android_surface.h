// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_SHELL_PLATFORM_ANDROID_ANDROID_SURFACE_H_
#define FLUTTER_SHELL_PLATFORM_ANDROID_ANDROID_SURFACE_H_

#include <memory>
#include "flutter/flow/embedded_views.h"
#include "flutter/flow/surface.h"
#include "flutter/fml/macros.h"
#include "flutter/shell/platform/android/context/android_context.h"
#include "flutter/shell/platform/android/jni/platform_view_android_jni.h"
#include "flutter/shell/platform/android/surface/android_native_window.h"
#include "third_party/skia/include/core/SkSize.h"

namespace flutter {

class AndroidExternalViewEmbedder;

class AndroidSurface {
 public:
  virtual ~AndroidSurface();

  virtual bool IsValid() const = 0;

  virtual void TeardownOnScreenContext() = 0;

  virtual std::unique_ptr<Surface> CreateGPUSurface(
      GrDirectContext* gr_context = nullptr) = 0;

  virtual bool OnScreenSurfaceResize(const SkISize& size) = 0;

  virtual bool ResourceContextMakeCurrent() = 0;

  virtual bool ResourceContextClearCurrent() = 0;

  virtual bool SetNativeWindow(fml::RefPtr<AndroidNativeWindow> window) = 0;

  virtual std::unique_ptr<Surface> CreatePbufferSurface();

 protected:
  explicit AndroidSurface(
      const std::shared_ptr<AndroidContext>& android_context);
  std::shared_ptr<AndroidContext> android_context_;
};

class AndroidSurfaceFactory {
 public:
  AndroidSurfaceFactory() = default;

  virtual ~AndroidSurfaceFactory() = default;

  virtual std::unique_ptr<AndroidSurface> CreateSurface() = 0;
};

}  // namespace flutter

#endif  // FLUTTER_SHELL_PLATFORM_ANDROID_ANDROID_SURFACE_H_
