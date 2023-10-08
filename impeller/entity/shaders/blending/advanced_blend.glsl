// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <impeller/blending.glsl>
#include <impeller/color.glsl>
#include <impeller/texture.glsl>
#include <impeller/types.glsl>

uniform BlendInfo {
  float16_t dst_input_alpha;
  float16_t src_input_alpha;
  float16_t color_factor;
  f16vec4 color;  // This color input is expected to be unpremultiplied.
  float supports_decal_sampler_address_mode;
}
blend_info;

uniform f16sampler2D texture_sampler_dst;
uniform f16sampler2D texture_sampler_src;

in vec2 v_dst_texture_coords;
in vec2 v_src_texture_coords;

out f16vec4 frag_color;

f16vec4 Sample(f16sampler2D texture_sampler, vec2 texture_coords) {
  if (blend_info.supports_decal_sampler_address_mode > 0.0) {
    return texture(texture_sampler, texture_coords);
  } else {
    return IPHalfSampleDecal(texture_sampler, texture_coords);
  }
}

void main() {
  f16vec4 dst_sample = Sample(texture_sampler_dst,  // sampler
                              v_dst_texture_coords  // texture coordinates
                              ) *
                       blend_info.dst_input_alpha;

  f16vec4 dst = dst_sample;
  f16vec4 src = blend_info.color_factor > 0.0hf
                    ? blend_info.color
                    : Sample(texture_sampler_src,  // sampler
                             v_src_texture_coords  // texture coordinates
                             ) *
                          blend_info.src_input_alpha;

  f16vec4 blended = mix(src, f16vec4(Blend(dst.rgb, src.rgb), dst.a), dst.a);
  frag_color = mix(dst_sample, blended, src.a);
}
