// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file is transformed during the build process into a single library with
// part files (`dart:_engine`) by performing the following:
//
//  - Replace all exports with part directives.
//  - Rewrite the libraries into `part of` part files without imports.
//  - Add imports to this file sufficient to cover the needs of `dart:_engine`.
//
// The code that performs the transformations lives in:
//
//  - https://github.com/flutter/engine/blob/main/web_sdk/sdk_rewriter.dart

library engine;

export 'engine/alarm_clock.dart';
export 'engine/app_bootstrap.dart';
export 'engine/assets.dart';
export 'engine/browser_detection.dart';
export 'engine/canvas_pool.dart';
export 'engine/canvaskit/canvas.dart';
export 'engine/canvaskit/canvaskit_api.dart';
export 'engine/canvaskit/canvaskit_canvas.dart';
export 'engine/canvaskit/color_filter.dart';
export 'engine/canvaskit/embedded_views.dart';
export 'engine/canvaskit/embedded_views_diff.dart';
export 'engine/canvaskit/font_fallbacks.dart';
export 'engine/canvaskit/fonts.dart';
export 'engine/canvaskit/image.dart';
export 'engine/canvaskit/image_filter.dart';
export 'engine/canvaskit/image_wasm_codecs.dart';
export 'engine/canvaskit/image_web_codecs.dart';
export 'engine/canvaskit/initialization.dart';
export 'engine/canvaskit/interval_tree.dart';
export 'engine/canvaskit/layer.dart';
export 'engine/canvaskit/layer_scene_builder.dart';
export 'engine/canvaskit/layer_tree.dart';
export 'engine/canvaskit/mask_filter.dart';
export 'engine/canvaskit/n_way_canvas.dart';
export 'engine/canvaskit/painting.dart';
export 'engine/canvaskit/path.dart';
export 'engine/canvaskit/path_metrics.dart';
export 'engine/canvaskit/picture.dart';
export 'engine/canvaskit/picture_recorder.dart';
export 'engine/canvaskit/raster_cache.dart';
export 'engine/canvaskit/rasterizer.dart';
export 'engine/canvaskit/shader.dart';
export 'engine/canvaskit/skia_object_cache.dart';
export 'engine/canvaskit/surface.dart';
export 'engine/canvaskit/surface_factory.dart';
export 'engine/canvaskit/text.dart';
export 'engine/canvaskit/util.dart';
export 'engine/canvaskit/vertices.dart';
export 'engine/clipboard.dart';
export 'engine/color_filter.dart';
export 'engine/configuration.dart';
export 'engine/dom.dart';
export 'engine/embedder.dart';
export 'engine/engine_canvas.dart';
export 'engine/font_change_util.dart';
export 'engine/frame_reference.dart';
export 'engine/host_node.dart';
export 'engine/html/backdrop_filter.dart';
export 'engine/html/bitmap_canvas.dart';
export 'engine/html/canvas.dart';
export 'engine/html/clip.dart';
export 'engine/html/color_filter.dart';
export 'engine/html/debug_canvas_reuse_overlay.dart';
export 'engine/html/dom_canvas.dart';
export 'engine/html/image_filter.dart';
export 'engine/html/offset.dart';
export 'engine/html/opacity.dart';
export 'engine/html/painting.dart';
export 'engine/html/path/conic.dart';
export 'engine/html/path/cubic.dart';
export 'engine/html/path/path.dart';
export 'engine/html/path/path_iterator.dart';
export 'engine/html/path/path_metrics.dart';
export 'engine/html/path/path_ref.dart';
export 'engine/html/path/path_to_svg.dart';
export 'engine/html/path/path_utils.dart';
export 'engine/html/path/path_windings.dart';
export 'engine/html/path/tangent.dart';
export 'engine/html/path_to_svg_clip.dart';
export 'engine/html/picture.dart';
export 'engine/html/platform_view.dart';
export 'engine/html/recording_canvas.dart';
export 'engine/html/render_vertices.dart';
export 'engine/html/scene.dart';
export 'engine/html/scene_builder.dart';
export 'engine/html/shader_mask.dart';
export 'engine/html/shaders/image_shader.dart';
export 'engine/html/shaders/normalized_gradient.dart';
export 'engine/html/shaders/shader.dart';
export 'engine/html/shaders/shader_builder.dart';
export 'engine/html/shaders/vertex_shaders.dart';
export 'engine/html/surface.dart';
export 'engine/html/surface_stats.dart';
export 'engine/html/transform.dart';
export 'engine/html_image_codec.dart';
export 'engine/initialization.dart';
export 'engine/js_interop/js_loader.dart';
export 'engine/js_interop/js_promise.dart';
export 'engine/key_map.g.dart';
export 'engine/keyboard.dart';
export 'engine/keyboard_binding.dart';
export 'engine/keyboard_layout_detector.dart';
export 'engine/mouse_cursor.dart';
export 'engine/navigation/history.dart';
export 'engine/navigation/js_url_strategy.dart';
export 'engine/navigation/url_strategy.dart';
export 'engine/onscreen_logging.dart';
export 'engine/picture.dart';
export 'engine/platform_dispatcher.dart';
export 'engine/platform_views.dart';
export 'engine/platform_views/content_manager.dart';
export 'engine/platform_views/message_handler.dart';
export 'engine/platform_views/slots.dart';
export 'engine/plugins.dart';
export 'engine/pointer_binding.dart';
export 'engine/pointer_converter.dart';
export 'engine/profiler.dart';
export 'engine/rrect_renderer.dart';
export 'engine/safe_browser_api.dart';
export 'engine/semantics/accessibility.dart';
export 'engine/semantics/checkable.dart';
export 'engine/semantics/image.dart';
export 'engine/semantics/incrementable.dart';
export 'engine/semantics/label_and_value.dart';
export 'engine/semantics/live_region.dart';
export 'engine/semantics/scrollable.dart';
export 'engine/semantics/semantics.dart';
export 'engine/semantics/semantics_helper.dart';
export 'engine/semantics/tappable.dart';
export 'engine/semantics/text_field.dart';
export 'engine/services/buffers.dart';
export 'engine/services/message_codec.dart';
export 'engine/services/message_codecs.dart';
export 'engine/services/serialization.dart';
export 'engine/shadow.dart';
export 'engine/svg.dart';
export 'engine/test_embedding.dart';
export 'engine/text/canvas_paragraph.dart';
export 'engine/text/font_collection.dart';
export 'engine/text/layout_service.dart';
export 'engine/text/line_break_properties.dart';
export 'engine/text/line_breaker.dart';
export 'engine/text/measurement.dart';
export 'engine/text/paint_service.dart';
export 'engine/text/paragraph.dart';
export 'engine/text/ruler.dart';
export 'engine/text/text_direction.dart';
export 'engine/text/unicode_range.dart';
export 'engine/text/word_break_properties.dart';
export 'engine/text/word_breaker.dart';
export 'engine/text_editing/autofill_hint.dart';
export 'engine/text_editing/composition_aware_mixin.dart';
export 'engine/text_editing/input_type.dart';
export 'engine/text_editing/text_capitalization.dart';
export 'engine/text_editing/text_editing.dart';
export 'engine/util.dart';
export 'engine/validators.dart';
export 'engine/vector_math.dart';
export 'engine/window.dart';
