// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMMON_PLATFORM_VIEW_H_
#define COMMON_PLATFORM_VIEW_H_

#include <memory>

#include "flutter/lib/ui/semantics/semantics_node.h"
#include "flutter/shell/common/engine.h"
#include "flutter/shell/common/shell.h"
#include "flutter/shell/common/surface.h"
#include "lib/ftl/macros.h"
#include "lib/ftl/memory/weak_ptr.h"
#include "lib/ftl/synchronization/waitable_event.h"
#include "third_party/skia/include/core/SkSize.h"
#include "third_party/skia/include/gpu/GrContext.h"

namespace shell {

class Rasterizer;

class PlatformView {
 public:
  struct SurfaceConfig {
    uint8_t red_bits = 8;
    uint8_t green_bits = 8;
    uint8_t blue_bits = 8;
    uint8_t alpha_bits = 8;
    uint8_t depth_bits = 0;
    uint8_t stencil_bits = 8;
  };

  void SetupResourceContextOnIOThread();

  virtual ~PlatformView();

  void DispatchSemanticsAction(int32_t id, blink::SemanticsAction action);
  void SetSemanticsEnabled(bool enabled);

  void ConnectToEngine(mojo::InterfaceRequest<sky::SkyEngine> request);

  void NotifyCreated(std::unique_ptr<Surface> surface);

  void NotifyCreated(std::unique_ptr<Surface> surface,
                     ftl::Closure continuation);

  void NotifyDestroyed();

  ftl::WeakPtr<PlatformView> GetWeakPtr();

  virtual bool ResourceContextMakeCurrent() = 0;

  virtual void UpdateSemantics(std::vector<blink::SemanticsNode> update);

  Rasterizer& rasterizer() { return *rasterizer_; }
  Engine& engine() { return *engine_; }

  virtual void RunFromSource(const std::string& main,
                             const std::string& packages,
                             const std::string& assets_directory) = 0;

 protected:
  explicit PlatformView(std::unique_ptr<Rasterizer> rasterizer);

  void SetupResourceContextOnIOThreadPerform(
      ftl::AutoResetWaitableEvent* event);

  SurfaceConfig surface_config_;
  std::unique_ptr<Rasterizer> rasterizer_;
  std::unique_ptr<Engine> engine_;
  SkISize size_;

 private:
  ftl::WeakPtrFactory<PlatformView> weak_factory_;

  FTL_DISALLOW_COPY_AND_ASSIGN(PlatformView);
};

}  // namespace shell

#endif  // COMMON_PLATFORM_VIEW_H_
