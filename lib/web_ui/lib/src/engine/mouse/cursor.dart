// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import '../dom.dart';

/// Controls the mouse cursor in the given [element].
class MouseCursor {
  MouseCursor(this.element);

  final DomElement element;

  // Map from Flutter's kind values to CSS's cursor values.
  //
  // This map must be kept in sync with Flutter framework's
  // rendering/mouse_cursor.dart.
  static const Map<String, String> _kindToCssValueMap = <String, String>{
    'alias': 'alias',
    'allScroll': 'all-scroll',
    'basic': 'default',
    'cell': 'cell',
    'click': 'pointer',
    'contextMenu': 'context-menu',
    'copy': 'copy',
    'forbidden': 'not-allowed',
    'grab': 'grab',
    'grabbing': 'grabbing',
    'help': 'help',
    'move': 'move',
    'none': 'none',
    'noDrop': 'no-drop',
    'precise': 'crosshair',
    'progress': 'progress',
    'text': 'text',
    'resizeColumn': 'col-resize',
    'resizeDown': 's-resize',
    'resizeDownLeft': 'sw-resize',
    'resizeDownRight': 'se-resize',
    'resizeLeft': 'w-resize',
    'resizeLeftRight': 'ew-resize',
    'resizeRight': 'e-resize',
    'resizeRow': 'row-resize',
    'resizeUp': 'n-resize',
    'resizeUpDown': 'ns-resize',
    'resizeUpLeft': 'nw-resize',
    'resizeUpRight': 'ne-resize',
    'resizeUpLeftDownRight': 'nwse-resize',
    'resizeUpRightDownLeft': 'nesw-resize',
    'verticalText': 'vertical-text',
    'wait': 'wait',
    'zoomIn': 'zoom-in',
    'zoomOut': 'zoom-out',
  };

  static String _mapKindToCssValue(String? kind) {
    return _kindToCssValueMap[kind] ?? 'default';
  }

  void activateSystemCursor(String? kind) {
    element.style.cursor = _mapKindToCssValue(kind);
  }
}
