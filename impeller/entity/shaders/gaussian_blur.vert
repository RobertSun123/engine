// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

uniform FrameInfo {
  mat4 mvp;
  vec2 texture_size;
  vec2 blur_direction;
  float blur_radius;
}
frame_info;

in vec2 vertices;
in vec2 texture_coords;

out vec2 v_texture_coords;
out vec2 v_texture_size;
out vec2 v_blur_direction;
out float v_blur_radius;

void main() {
  gl_Position = frame_info.mvp * vec4(vertices, 0.0, 1.0);
  v_texture_coords = texture_coords;
  v_texture_size = frame_info.texture_size;
  v_blur_direction = frame_info.blur_direction;
  v_blur_radius = frame_info.blur_radius;
}
