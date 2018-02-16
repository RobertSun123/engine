// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_FLOW_TEXTURE_H_
#define FLUTTER_FLOW_TEXTURE_H_

#include <map>
#include "flutter/common/threads.h"
#include "lib/fxl/synchronization/waitable_event.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "flutter/flow/layers/layer.h"

namespace flow {

class Texture {
 protected:
  Texture(int64_t id);

 public:
  // Called from GPU thread.
  virtual ~Texture();

  virtual void UpdateScene(LayeredPaintContext *context, const SkRect& bounds) = 0;

  // Called from GPU thread.
  virtual void Paint(Layer::PaintContext context, const SkRect& bounds) = 0;

  // Called from GPU thread.
  virtual void OnGrContextCreated() = 0;

  // Called from GPU thread.
  virtual void OnGrContextDestroyed() = 0;

  virtual bool NeedsSystemComposite() = 0;

  int64_t Id() { return id_; }

 private:
  int64_t id_;

  FXL_DISALLOW_COPY_AND_ASSIGN(Texture);
};

class TextureRegistry {
 public:
  TextureRegistry();
  ~TextureRegistry();

  // Called from GPU thread.
  void RegisterTexture(std::shared_ptr<Texture> texture);

  // Called from GPU thread.
  void UnregisterTexture(int64_t id);

  // Called from GPU thread.
  std::shared_ptr<Texture> GetTexture(int64_t id);

  // Called from GPU thread.
  void OnGrContextCreated();

  // Called from GPU thread.
  void OnGrContextDestroyed();

 private:
  std::map<int64_t, std::shared_ptr<Texture>> mapping_;

  FXL_DISALLOW_COPY_AND_ASSIGN(TextureRegistry);
};

}  // namespace flow

#endif  // FLUTTER_FLOW_TEXTURE_H_
