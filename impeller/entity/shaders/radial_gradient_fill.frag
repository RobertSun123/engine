// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <impeller/types.glsl>
#include <impeller/texture.glsl>

uniform sampler2D texture_sampler;

uniform GradientInfo {
  f16vec2 center;
  float16_t radius;
  float16_t tile_mode;
  float16_t texture_sampler_y_coord_scale;
  float16_t alpha;
  f16vec2 half_texel;
} gradient_info;

in f16vec2 v_position;

out f16vec4 frag_color;

void main() {
  float16_t len = length(v_position - gradient_info.center);
  float16_t t = len / gradient_info.radius;
  frag_color = IPSampleLinearWithTileMode(
    texture_sampler,
    f16vec2(t, 0.5hf),
    gradient_info.texture_sampler_y_coord_scale,
    gradient_info.half_texel,
    gradient_info.tile_mode);
  frag_color = f16vec4(frag_color.xyz * frag_color.a, frag_color.a) * gradient_info.alpha;
}
