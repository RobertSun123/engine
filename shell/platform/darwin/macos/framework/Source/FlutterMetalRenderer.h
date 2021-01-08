// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>

#import "flutter/shell/platform/darwin/macos/framework/Headers/FlutterEngine.h"
#import "flutter/shell/platform/darwin/macos/framework/Source/FlutterView.h"
#import "flutter/shell/platform/embedder/embedder.h"

/**
 * Provides the renderer config needed to initialize the embedder engine and also handles external
 * texture management. This is initialized during FlutterEngine creation and then attached to the
 * FlutterView once the FlutterViewController is initializer.
 */
@interface FlutterMetalRenderer : NSObject <FlutterTextureRegistry>

@property(nonatomic, readonly, nonnull) id<MTLDevice> mtlDevice;

@property(nonatomic, readonly, nonnull) id<MTLCommandQueue> mtlCommandQueue;

/**
 * Intializes the renderer with the given FlutterEngine.
 */
- (nullable instancetype)initWithFlutterEngine:(nonnull FlutterEngine*)flutterEngine;

/**
 * Sets the FlutterView to render to.
 */
- (void)setFlutterView:(nullable FlutterView*)view;

/**
 * Creates a FlutterRendererConfig that renders using Metal.
 */
- (FlutterRendererConfig)createRendererConfig;

- (FlutterMetalTexture)createTextureForSize:(CGSize)size;

- (bool)present:(int64_t)textureId;

@end
