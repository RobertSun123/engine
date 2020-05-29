// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_SHELL_PLATFORM_ANDROID_ANDROID_CONTEXT_H_
#define FLUTTER_SHELL_PLATFORM_ANDROID_ANDROID_CONTEXT_H_

#include "flutter/fml/macros.h"
#include "flutter/fml/memory/ref_counted.h"
#include "flutter/fml/memory/ref_ptr.h"
#include "flutter/shell/common/platform_view.h"

namespace flutter {

enum class AndroidRenderingAPI {
  kSoftware,
  kOpenGLES,
  kVulkan,
};

//------------------------------------------------------------------------------
/// @brief      Holds state that is shared across android surfaces.
///
class AndroidContext : public fml::RefCountedThreadSafe<AndroidContext> {
 public:
  AndroidContext(AndroidRenderingAPI rendering_api);

  ~AndroidContext();

  static std::shared_ptr<AndroidContext> Create(
      AndroidRenderingAPI rendering_api);

  AndroidRenderingAPI RenderingApi();

  bool IsValid();

 private:
  AndroidRenderingAPI rendering_api_;

  FML_DISALLOW_COPY_AND_ASSIGN(AndroidContext);
};

}  // namespace flutter

#endif  // FLUTTER_SHELL_PLATFORM_ANDROID_ANDROID_CONTEXT_H_
