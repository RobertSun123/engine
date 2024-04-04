// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_SHELL_PLATFORM_WINDOWS_COMPOSITOR_SOFTWARE_H_
#define FLUTTER_SHELL_PLATFORM_WINDOWS_COMPOSITOR_SOFTWARE_H_

#include "flutter/shell/platform/embedder/embedder.h"
#include "flutter/shell/platform/windows/compositor.h"
#include "flutter/shell/platform/windows/flutter_windows_engine.h"

namespace flutter {

// Enables the Flutter engine to render content on Windows using software
// rasterization and bitmaps.
class CompositorSoftware : public Compositor {
 public:
  CompositorSoftware(FlutterWindowsEngine* engine);

  /// |Compositor|
  bool CreateBackingStore(const FlutterBackingStoreConfig& config,
                          FlutterBackingStore* result) override;
  /// |Compositor|
  bool CollectBackingStore(const FlutterBackingStore* store) override;

  /// |Compositor|
  bool Present(FlutterViewId view_id,
               const FlutterLayer** layers,
               size_t layers_count) override;

 private:
  /// Blend layer in-place onto allocation, which already holds the previous
  /// results of composition.
  void BlendLayer(std::vector<uint32_t>& allocation,
                  const FlutterLayer& layer,
                  int x_min,
                  int y_min,
                  int width,
                  int height) const;

  FlutterWindowsEngine* engine_;
};

}  // namespace flutter

#endif  // FLUTTER_SHELL_PLATFORM_WINDOWS_COMPOSITOR_SOFTWARE_H_
