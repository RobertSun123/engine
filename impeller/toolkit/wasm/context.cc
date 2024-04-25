// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/impeller/toolkit/wasm/context.h"

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <emscripten.h>

#include "impeller/base/validation.h"
#include "impeller/renderer/backend/gles/surface_gles.h"

namespace impeller::wasm {

static void ClearDepthEmulated(GLdouble depth) {}

static void DepthRangeEmulated(GLclampf nearVal, GLclampf farVal) {}

Context::Context() {
  if (!display_.IsValid()) {
    VALIDATION_LOG << "Could not create EGL display connection.";
    return;
  }

  egl::ConfigDescriptor desc;
  desc.api = egl::API::kOpenGLES2;
  desc.samples = egl::Samples::kOne;
  desc.color_format = egl::ColorFormat::kRGBA8888;
  desc.stencil_bits = egl::StencilBits::kZero;
  desc.depth_bits = egl::DepthBits::kZero;
  desc.surface_type = egl::SurfaceType::kWindow;

  auto config = display_.ChooseConfig(desc);
  if (!config) {
    VALIDATION_LOG << "Could not choose an EGL config.";
    return;
  }

  auto context = display_.CreateContext(*config, nullptr);
  if (!context) {
    VALIDATION_LOG << "Could not create EGL context.";
    return;
  }

  // The native window type is NULL for emscripten as documented in
  // https://emscripten.org/docs/porting/multimedia_and_graphics/EGL-Support-in-Emscripten.html
  auto surface = display_.CreateWindowSurface(*config, NULL);
  if (!surface) {
    VALIDATION_LOG << "Could not create EGL surface.";
    return;
  }

  if (!context->MakeCurrent(*surface)) {
    VALIDATION_LOG << "Could not make context current.";
    return;
  }

  std::map<std::string, void*> gl_procs;

  gl_procs["glGetError"] = (void*)&glGetError;
  gl_procs["glClearDepthf"] = (void*)&ClearDepthEmulated;
  gl_procs["glDepthRangef"] = (void*)&DepthRangeEmulated;

#define IMPELLER_PROC(name) gl_procs["gl" #name] = (void*)&gl##name;
  FOR_EACH_IMPELLER_PROC(IMPELLER_PROC);
#undef IMPELLER_PROC

  auto gl = std::make_unique<ProcTableGLES>(
      [&gl_procs](const char* function_name) -> void* {
        auto found = gl_procs.find(function_name);
        if (found == gl_procs.end()) {
          return nullptr;
        }
        return found->second;
      });

  if (!gl->IsValid()) {
    VALIDATION_LOG << "Could not setup GL proc table.";
    return;
  }

  auto renderer_context = ContextGLES::Create(std::move(gl),  // proc table
                                              {},    // shader libraries
                                              false  // enable tracing
  );
  if (!renderer_context) {
    VALIDATION_LOG << "Could not create GL context.";
    return;
  }

  surface_ = std::move(surface);
  context_ = std::move(context);
  renderer_context_ = std::move(renderer_context);
  is_valid_ = true;
}

Context::~Context() = default;

bool Context::IsValid() const {
  return is_valid_;
}

bool Context::RenderFrame() {
  if (!IsValid()) {
    VALIDATION_LOG << "Context was invalid.";
    return false;
  }

  if (!context_->MakeCurrent(*surface_)) {
    VALIDATION_LOG << "Could not make the context current.";
    return false;
  }

  SurfaceGLES::SwapCallback swap_callback = [surface = surface_]() -> bool {
    return surface->Present();
  };
  auto surface =
      SurfaceGLES::WrapFBO(renderer_context_,               // context
                           swap_callback,                   // swap callback
                           0u,                              // fbo
                           PixelFormat::kR8G8B8A8UNormInt,  // pixel format
                           GetWindowSize()                  // surface size
      );

  if (!surface || !surface->IsValid()) {
    VALIDATION_LOG << "Could not warp onscreen surface.";
    return false;
  }

  if (!surface->Present()) {
    VALIDATION_LOG << "Could not present the surface.";
    return false;
  }

  return true;
}

ISize Context::GetWindowSize() const {
  int width = 0;
  int height = 0;
  int fullscreen = 0;
  emscripten_get_canvas_size(&width, &height, &fullscreen);
  return ISize::MakeWH(std::max<uint32_t>(0u, width),
                       std::max<uint32_t>(0u, height));
}

}  // namespace impeller::wasm
