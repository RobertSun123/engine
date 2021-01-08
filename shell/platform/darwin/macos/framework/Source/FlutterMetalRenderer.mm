
// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "flutter/shell/platform/darwin/macos/framework/Source/FlutterMetalRenderer.h"

#import "flutter/shell/platform/darwin/macos/framework/Source/FlutterEngine_Internal.h"
#import "flutter/shell/platform/darwin/macos/framework/Source/FlutterView.h"
#include "flutter/shell/platform/embedder/embedder.h"

#pragma mark - Static callbacks that require the engine.

static FlutterMetalTexture OnGetNextDrawable(FlutterEngine* engine,
                                             const FlutterFrameInfo* frameInfo) {
  CGSize size = CGSizeMake(frameInfo->size.width, frameInfo->size.height);
  return [engine.metalRenderer createTextureForSize:size];
}

static bool OnPresentDrawable(FlutterEngine* engine, const FlutterMetalTexture* texture) {
  return [engine.metalRenderer present:texture->texture_id];
}

#pragma mark - FlutterMetalRenderer implementation

@implementation FlutterMetalRenderer {
  FlutterEngine* _engine;

  FlutterView* _flutterView;
}

- (instancetype)initWithFlutterEngine:(nonnull FlutterEngine*)flutterEngine {
  self = [super init];
  if (self) {
    _engine = flutterEngine;

    _mtlDevice = MTLCreateSystemDefaultDevice();
    if (!_mtlDevice) {
      NSLog(@"Could not acquire Metal device.");
      return nil;
    }

    _mtlCommandQueue = [_mtlDevice newCommandQueue];
    if (!_mtlCommandQueue) {
      NSLog(@"Could not create Metal command queue.");
      return nil;
    }
  }
  return self;
}

- (void)setFlutterView:(FlutterView*)view {
  _flutterView = view;
}

/**
 * Creates a FlutterRendererConfig that renders using Metal.
 */
- (FlutterRendererConfig)createRendererConfig {
  FlutterRendererConfig config = {
      .type = FlutterRendererType::kMetal,
      .metal.struct_size = sizeof(FlutterMetalRendererConfig),
      .metal.device = (__bridge FlutterMetalDeviceHandle)_mtlDevice,
      .metal.present_command_queue = (__bridge FlutterMetalCommandQueueHandle)_mtlCommandQueue,
      .metal.get_next_drawable_callback =
          reinterpret_cast<FlutterMetalTextureCallback>(OnGetNextDrawable),
      .metal.present_drawable_callback =
          reinterpret_cast<FlutterMetalPresentCallback>(OnPresentDrawable),
  };
  return config;
}

#pragma mark - Embedder callback implementations.

- (FlutterMetalTexture)createTextureForSize:(CGSize)size {
  FlutterBackingStoreDescriptor* backingStore = [_flutterView backingStoreForSize:size];
  id<MTLTexture> mtlTexture = [backingStore metalTexture];
  FlutterMetalTexture embedderTexture;
  embedderTexture.struct_size = sizeof(FlutterMetalTexture);
  embedderTexture.texture = (__bridge void*)mtlTexture;
  embedderTexture.texture_id = reinterpret_cast<int64_t>(mtlTexture);
  return embedderTexture;
}

- (bool)present:(int64_t)textureId {
  if (!_mtlCommandQueue || !_flutterView) {
    return false;
  }
  [_flutterView present];
  return true;
}

#pragma mark - FlutterTextureRegistrar methods.

- (int64_t)registerTexture:(id<FlutterTexture>)texture {
  NSAssert(NO, @"External textures aren't supported when using Metal on macOS.");
  return 0;
}

- (void)textureFrameAvailable:(int64_t)textureID {
  NSAssert(NO, @"External textures aren't supported when using Metal on macOS.");
}

- (void)unregisterTexture:(int64_t)textureID {
  NSAssert(NO, @"External textures aren't supported when using Metal on macOS.");
}

@end
