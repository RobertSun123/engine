// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios_gl_context_guard_manager.h"

namespace flutter {

IOSGLContextGuardManager::IOSGLContextGuardManager() {
  resource_context_.reset([[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES3]);
  stored_ = fml::scoped_nsobject<NSMutableArray>([[NSMutableArray new] retain]);
  resource_context_.reset([[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES3]);
  if (resource_context_ != nullptr) {
    context_.reset([[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES3
                                         sharegroup:resource_context_.get().sharegroup]);
  } else {
    resource_context_.reset([[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2]);
    context_.reset([[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2
                                         sharegroup:resource_context_.get().sharegroup]);
  }
};

std::unique_ptr<IOSGLContextGuardManager::IOSGLContextAutoRelease> IOSGLContextGuardManager::MakeCurrent() {
  return std::make_unique<IOSGLContextGuardManager::IOSGLContextAutoRelease>(*this, context_);
}

std::unique_ptr<IOSGLContextGuardManager::IOSGLContextAutoRelease> IOSGLContextGuardManager::ResourceMakeCurrent() {
  return std::make_unique<IOSGLContextGuardManager::IOSGLContextAutoRelease>(*this, resource_context_);
}

bool IOSGLContextGuardManager::PushContext(fml::scoped_nsobject<EAGLContext> context) {
  NSLog(@"before push store %@", stored_.get());
  EAGLContext* current = [EAGLContext currentContext];
  if (current == nil) {
    [stored_.get() addObject:[NSNull null]];
  } else {
    [stored_.get() addObject:current];
  }
  NSLog(@"after push store %@", stored_.get());
  bool result = [EAGLContext setCurrentContext:context.get()];
  NSLog(@"set result %@", @(result));
  return result;
}

void IOSGLContextGuardManager::PopContext() {
  NSLog(@"before pop store %@", stored_.get());
  EAGLContext* last = [stored_.get() lastObject];
  [stored_.get() removeLastObject];
  NSLog(@"after pop store %@", stored_.get());
  if ([last isEqual:[NSNull null]]) {
    [EAGLContext setCurrentContext:nil];
    return;
  }
  [EAGLContext setCurrentContext:last];
}

}
