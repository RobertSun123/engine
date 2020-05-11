// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/fml/platform/darwin/scoped_nsobject.h"

namespace fml {

id ObjcRetain(id object) {
  return [object retain];
}

id ObjcAutorelease(id object) {
  return [object autorelease];
}

void ObjcRelease(id object) {
  [object release];
}

}  // namespace fml
