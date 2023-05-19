// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "flutter/shell/platform/darwin/ios/ios_surface_metal_skia.h"

#include "flutter/shell/gpu/gpu_surface_metal_delegate.h"
#include "flutter/shell/gpu/gpu_surface_metal_skia.h"
#include "flutter/shell/platform/darwin/ios/ios_context_metal_skia.h"

namespace flutter {

static IOSContextMetalSkia* CastToMetalContext(const std::shared_ptr<IOSContext>& context) {
  return reinterpret_cast<IOSContextMetalSkia*>(context.get());
}

IOSSurfaceMetalSkia::IOSSurfaceMetalSkia(const fml::scoped_nsobject<CAMetalLayer>& layer,
                                         std::shared_ptr<IOSContext> context)
    : IOSSurface(std::move(context)),
      GPUSurfaceMetalDelegate(MTLRenderTargetType::kCAMetalLayer),
      layer_(layer) {
  is_valid_ = layer_;
  auto metal_context = CastToMetalContext(GetContext());
  auto darwin_context = metal_context->GetDarwinContext().get();
  command_queue_ = darwin_context.commandQueue;
  device_ = darwin_context.device;
}

// |IOSSurface|
IOSSurfaceMetalSkia::~IOSSurfaceMetalSkia() = default;

// |IOSSurface|
bool IOSSurfaceMetalSkia::IsValid() const {
  return is_valid_;
}

// |IOSSurface|
void IOSSurfaceMetalSkia::UpdateStorageSizeIfNecessary() {
  // Nothing to do.
}

// |IOSSurface|
std::unique_ptr<Surface> IOSSurfaceMetalSkia::CreateGPUSurface(GrDirectContext* context) {
  FML_DCHECK(context);
  return std::make_unique<GPUSurfaceMetalSkia>(this,                               // delegate
                                               sk_ref_sp(context),                 // context
                                               GetContext()->GetMsaaSampleCount()  // sample count
  );
}

// |GPUSurfaceMetalDelegate|
GPUCAMetalLayerHandle IOSSurfaceMetalSkia::GetCAMetalLayer(const SkISize& frame_info) const {
  CAMetalLayer* layer = layer_.get();
  layer.device = device_;

  layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
  // Flutter needs to read from the color attachment in cases where there are effects such as
  // backdrop filters. Flutter plugins that create platform views may also read from the layer.
  layer.framebufferOnly = NO;

  const auto drawable_size = CGSizeMake(frame_info.width(), frame_info.height());
  if (!CGSizeEqualToSize(drawable_size, layer.drawableSize)) {
    layer.drawableSize = drawable_size;
  }

  // When there are platform views in the scene, the drawable needs to be presented in the same
  // transaction as the one created for platform views. When the drawable are being presented from
  // the raster thread, there is no such transaction.
  layer.presentsWithTransaction = [[NSThread currentThread] isMainThread];

  return layer;
}

// |GPUSurfaceMetalDelegate|
bool IOSSurfaceMetalSkia::AllowsDrawingWhenGpuDisabled() const {
  return false;
}

}  // namespace flutter
