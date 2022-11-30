// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
part of dart.ui;

void addModelLayer(
  SceneBuilder builder,
  int viewId, {
  Offset offset = Offset.zero,
  double width = 0.0,
  double height = 0.0,
}) {
  assert(offset != null, 'Offset argument was null');
  builder._addModelLayer(offset.dx, offset.dy, width, height, viewId);
}
