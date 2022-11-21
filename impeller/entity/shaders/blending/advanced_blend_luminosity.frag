// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <impeller/types.glsl>
#include <impeller/blending.glsl>

f16vec3 Blend(f16vec3 dst, f16vec3 src) {
  return IPBlendLuminosity(dst, src);
}

#include "advanced_blend.glsl"
