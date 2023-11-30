// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_SHELL_PLATFORM_ANDROID_SURFACE_TEXTURE_EXTERNAL_TEXTURE_VK_H_
#define FLUTTER_SHELL_PLATFORM_ANDROID_SURFACE_TEXTURE_EXTERNAL_TEXTURE_VK_H_

#include <media/NdkImageReader.h>
#include <memory>

#include "flutter/shell/platform/android/surface_texture_external_texture.h"

#include "flutter/impeller/renderer/backend/vulkan/context_vk.h"
#include "flutter/impeller/renderer/backend/vulkan/texture_vk.h"
#include "impeller/renderer/backend/gles/proc_table_gles.h"
#include "impeller/toolkit/egl/display.h"
#include "impeller/toolkit/egl/surface.h"

namespace flutter {

class SurfaceTextureExternalTextureVK final
    : public SurfaceTextureExternalTexture {
 public:
  SurfaceTextureExternalTextureVK(
      const std::shared_ptr<impeller::ContextVK>& context,
      int64_t id,
      const fml::jni::ScopedJavaGlobalRef<jobject>& surface_texture,
      const std::shared_ptr<PlatformViewAndroidJNI>& jni_facade);

  ~SurfaceTextureExternalTextureVK() override;

 private:
  virtual void ProcessFrame(PaintContext& context,
                            const SkRect& bounds) override;
  virtual void Detach() override;
  void OnImageAvailable(AImageReader* reader);

  const std::shared_ptr<impeller::ContextVK> impeller_context_;
  std::shared_ptr<impeller::TextureVK> texture_;

  // Backed by ImageReader.
  std::unique_ptr<impeller::egl::Surface> surface_;

  // Proc table for GL functions.
  std::unique_ptr<impeller::ProcTableGLES> gl_;
  std::unique_ptr<impeller::egl::Display> egl_;

  FML_DISALLOW_COPY_AND_ASSIGN(SurfaceTextureExternalTextureVK);
};

}  // namespace flutter

#endif  // FLUTTER_SHELL_PLATFORM_ANDROID_SURFACE_TEXTURE_EXTERNAL_TEXTURE_VK_H_