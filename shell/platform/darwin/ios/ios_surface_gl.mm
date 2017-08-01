// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/darwin/ios/ios_surface_gl.h"

#include "flutter/shell/gpu/gpu_surface_gl.h"

namespace shell {

IOSSurfaceGL::IOSSurfaceGL(
    PlatformView::SurfaceConfig surface_config,
    CAEAGLLayer* layer,
    ftl::Closure firstFrameCallback)
    : IOSSurface(surface_config, reinterpret_cast<CALayer*>(layer), firstFrameCallback),
      context_(surface_config, layer) {}

IOSSurfaceGL::~IOSSurfaceGL() = default;

bool IOSSurfaceGL::IsValid() const {
  return context_.IsValid();
}

bool IOSSurfaceGL::ResourceContextMakeCurrent() {
  return IsValid() ? context_.ResourceMakeCurrent() : false;
}

void IOSSurfaceGL::UpdateStorageSizeIfNecessary() {
  if (IsValid()) {
    context_.UpdateStorageSizeIfNecessary();
  }
}

std::unique_ptr<Surface> IOSSurfaceGL::CreateGPUSurface() {
  return std::make_unique<GPUSurfaceGL>(this);
}

intptr_t IOSSurfaceGL::GLContextFBO() const {
  return IsValid() ? context_.framebuffer() : GL_NONE;
}

bool IOSSurfaceGL::GLContextMakeCurrent() {
  return IsValid() ? context_.MakeCurrent() : false;
}

bool IOSSurfaceGL::GLContextClearCurrent() {
  [EAGLContext setCurrentContext:nil];
  return true;
}

bool IOSSurfaceGL::GLContextPresent() {
  TRACE_EVENT0("flutter", "IOSSurfaceGL::GLContextPresent");
  if (!IsValid()) {
    return false;
  }

  if (context_.PresentRenderBuffer()) {
    NotifyFirstFrameOnce();
    return true;
  }

  return false;
}

}  // namespace shell
