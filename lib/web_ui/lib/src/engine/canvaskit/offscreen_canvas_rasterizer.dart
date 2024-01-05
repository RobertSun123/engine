// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'package:ui/src/engine.dart';

/// A [Rasterizer] that uses a single GL context in an OffscreenCanvas to do
/// all the rendering. It transers bitmaps created in the OffscreenCanvas to
/// one or many on-screen <canvas> elements to actually display the scene.
class OffscreenCanvasRasterizer extends Rasterizer {
  /// This is an SkSurface backed by an OffScreenCanvas. This single Surface is
  /// used to render to many RenderCanvases to produce the rendered scene.
  final Surface offscreenSurface = Surface();

  @override
  OffscreenCanvasViewRasterizer createViewRasterizer(EngineFlutterView view) {
    return OffscreenCanvasViewRasterizer(view, this);
  }

  @override
  void setResourceCacheMaxBytes(int bytes) {
    // TODO(harryterkelsen): Implement.
  }

  @override
  void dispose() {
    // TODO(harryterkelsen): Implement.
  }
}

class OffscreenCanvasViewRasterizer extends ViewRasterizer {
  OffscreenCanvasViewRasterizer(super.view, this.rasterizer);

  final OffscreenCanvasRasterizer rasterizer;

  @override
  final OverlayCanvasFactory<RenderCanvas> overlayFactory =
      OverlayCanvasFactory<RenderCanvas>(createCanvas: () => RenderCanvas());

  /// Render the given [pictures] so it is displayed by the given [canvas].
  @override
  Future<void> rasterizeToCanvas(
      OverlayCanvas canvas, List<CkPicture> pictures) async {
    await rasterizer.offscreenSurface.rasterizeToCanvas(
      currentFrameSize,
      canvas as RenderCanvas,
      pictures,
    );
  }

  @override
  void dispose() {
    viewEmbedder.dispose();
    overlayFactory.dispose();
  }

  @override
  void prepareToDraw() {
    rasterizer.offscreenSurface.ensureSurface(currentFrameSize);
  }
}
