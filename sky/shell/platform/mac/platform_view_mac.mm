// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sky/shell/platform/mac/platform_view_mac.h"

#include <AppKit/AppKit.h>

namespace sky {
namespace shell {

PlatformView* PlatformView::Create(const Config& config) {
  return new PlatformViewMac(config);
}

PlatformViewMac::PlatformViewMac(const Config& config)
    : PlatformView(config), opengl_view_(nullptr), weak_factory_(this) {}

PlatformViewMac::~PlatformViewMac() {
  weak_factory_.InvalidateWeakPtrs();
}

base::WeakPtr<sky::shell::PlatformView> PlatformViewMac::GetWeakViewPtr() {
  return weak_factory_.GetWeakPtr();
}

void PlatformViewMac::SetOpenGLView(NSOpenGLView* view) {
  opengl_view_ = view;
}

uint64_t PlatformViewMac::DefaultFramebuffer() const {
  return 0;
}

bool PlatformViewMac::ContextMakeCurrent() {
  if (!IsValid()) {
    return false;
  }

  [opengl_view_.openGLContext makeCurrentContext];
  return true;
}

bool PlatformViewMac::SwapBuffers() {
  if (!IsValid()) {
    return false;
  }

  [opengl_view_.openGLContext flushBuffer];
  return true;
}

bool PlatformViewMac::IsValid() const {
  if (opengl_view_ == nullptr) {
    return false;
  }

  auto context = opengl_view_.openGLContext;

  if (context == nullptr) {
    return false;
  }

  return true;
}

}  // namespace shell
}  // namespace sky
