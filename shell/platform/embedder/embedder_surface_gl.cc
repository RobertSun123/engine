// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/embedder/embedder_surface_gl.h"

#include "flutter/shell/common/shell_io_manager.h"

namespace flutter {

EmbedderSurfaceGL::EmbedderSurfaceGL(
    GLDispatchTable gl_dispatch_table,
    bool fbo_reset_after_present,
    std::shared_ptr<EmbedderExternalViewEmbedder> external_view_embedder)
    : gl_dispatch_table_(gl_dispatch_table),
      fbo_reset_after_present_(fbo_reset_after_present),
      external_view_embedder_(external_view_embedder) {
  // Make sure all required members of the dispatch table are checked.
  if (!gl_dispatch_table_.gl_make_current_callback ||
      !gl_dispatch_table_.gl_clear_current_callback ||
      !gl_dispatch_table_.gl_present_callback ||
      !gl_dispatch_table_.gl_fbo_callback ||
      !gl_dispatch_table_.gl_populate_existing_damage) {
    return;
  }

  valid_ = true;
}

EmbedderSurfaceGL::~EmbedderSurfaceGL() = default;

// |EmbedderSurface|
bool EmbedderSurfaceGL::IsValid() const {
  return valid_;
}

// |GPUSurfaceGLDelegate|
std::unique_ptr<GLContextResult> EmbedderSurfaceGL::GLContextMakeCurrent() {
  return std::make_unique<GLContextDefaultResult>(
      gl_dispatch_table_.gl_make_current_callback());
}

// |GPUSurfaceGLDelegate|
bool EmbedderSurfaceGL::GLContextClearCurrent() {
  return gl_dispatch_table_.gl_clear_current_callback();
}

// |GPUSurfaceGLDelegate|
bool EmbedderSurfaceGL::GLContextPresent(const GLPresentInfo& present_info) {
  // Pass the present information to the embedder present callback.
  return gl_dispatch_table_.gl_present_callback(present_info);
}

// |GPUSurfaceGLDelegate|
GLFBOInfo EmbedderSurfaceGL::GLContextFBO(GLFrameInfo frame_info) const {
  // Get the FBO ID using the gl_fbo_callback and then get exiting damage by
  // passing that ID to the gl_populate_existing_damage.
  return gl_dispatch_table_.gl_populate_existing_damage(
      gl_dispatch_table_.gl_fbo_callback(frame_info));
}

// |GPUSurfaceGLDelegate|
bool EmbedderSurfaceGL::GLContextFBOResetAfterPresent() const {
  return fbo_reset_after_present_;
}

// |GPUSurfaceGLDelegate|
SkMatrix EmbedderSurfaceGL::GLContextSurfaceTransformation() const {
  auto callback = gl_dispatch_table_.gl_surface_transformation_callback;
  if (!callback) {
    SkMatrix matrix;
    matrix.setIdentity();
    return matrix;
  }
  return callback();
}

// |GPUSurfaceGLDelegate|
EmbedderSurfaceGL::GLProcResolver EmbedderSurfaceGL::GetGLProcResolver() const {
  return gl_dispatch_table_.gl_proc_resolver;
}

// |GPUSurfaceGLDelegate|
SurfaceFrame::FramebufferInfo EmbedderSurfaceGL::GLContextFramebufferInfo()
    const {
  // Enable partial repaint by default on the embedders.
  auto info = SurfaceFrame::FramebufferInfo{};
  info.supports_readback = true;
  info.supports_partial_repaint =
      gl_dispatch_table_.gl_populate_existing_damage != nullptr;
  return info;
}

// |EmbedderSurface|
std::unique_ptr<Surface> EmbedderSurfaceGL::CreateGPUSurface() {
  const bool render_to_surface = !external_view_embedder_;
  return std::make_unique<GPUSurfaceGLSkia>(
      this,              // GPU surface GL delegate
      render_to_surface  // render to surface
  );
}

// |EmbedderSurface|
sk_sp<GrDirectContext> EmbedderSurfaceGL::CreateResourceContext() const {
  auto callback = gl_dispatch_table_.gl_make_resource_current_callback;
  if (callback && callback()) {
    if (auto context = ShellIOManager::CreateCompatibleResourceLoadingContext(
            GrBackend::kOpenGL_GrBackend, GetGLInterface())) {
      return context;
    } else {
      FML_LOG(ERROR)
          << "Internal error: Resource context available but could not create "
             "a compatible Skia context.";
      return nullptr;
    }
  }

  // The callback was not available or failed.
  FML_LOG(ERROR)
      << "Could not create a resource context for async texture uploads. "
         "Expect degraded performance. Set a valid make_resource_current "
         "callback on FlutterOpenGLRendererConfig.";
  return nullptr;
}

}  // namespace flutter
