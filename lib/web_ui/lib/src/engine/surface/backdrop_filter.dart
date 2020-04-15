// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// @dart = 2.6
part of engine;

/// A surface that applies an image filter to background.
class PersistedBackdropFilter extends PersistedContainerSurface
    implements ui.BackdropFilterEngineLayer {
  PersistedBackdropFilter(PersistedBackdropFilter oldLayer, this.filter)
      : super(oldLayer);

  final EngineImageFilter filter;

  /// The dedicated child container element that's separate from the
  /// [rootElement] is used to host child in front of [filterElement] that
  /// is transformed to cover background.
  @override
  html.Element get childContainer => _childContainer;
  html.Element _childContainer;
  html.Element _filterElement;
  // Cached inverted transform for _transform.
  Matrix4 _invertedTransform;
  // Reference to transform last used to cache [_invertedTransform].
  Matrix4 _previousTransform;

  @override
  void adoptElements(PersistedBackdropFilter oldSurface) {
    super.adoptElements(oldSurface);
    _childContainer = oldSurface._childContainer;
    _filterElement = oldSurface._filterElement;
    oldSurface._childContainer = null;
  }

  @override
  html.Element createElement() {
    final html.Element element = defaultCreateElement('flt-backdrop')
      ..style.transformOrigin = '0 0 0';
    _childContainer = html.Element.tag('flt-backdrop-interior');
    _childContainer.style.position = 'absolute';
    if (_debugExplainSurfaceStats) {
      // This creates an additional interior element. Count it too.
      _surfaceStatsFor(this).allocatedDomNodeCount++;
    }
    _filterElement = defaultCreateElement('flt-backdrop-filter');
    _filterElement.style.transformOrigin = '0 0 0';
    element..append(_filterElement)..append(_childContainer);
    return element;
  }

  @override
  void discard() {
    super.discard();
    // Do not detach the child container from the root. It is permanently
    // attached. The elements are reused together and are detached from the DOM
    // together.
    _childContainer = null;
    _filterElement = null;
  }

  @override
  void apply() {
    if (_previousTransform != _transform) {
      _invertedTransform = Matrix4.inverted(_transform);
      _previousTransform = _transform;
    }
    // https://api.flutter.dev/flutter/widgets/BackdropFilter-class.html
    // Defines the effective area as the parent/ancestor clip or if not
    // available, the whole screen.
    //
    // The CSS backdrop-filter will use isolation boundary defined in
    // https://drafts.fxtf.org/filter-effects-2/#BackdropFilterProperty
    // Therefore we need to use parent clip element bounds for
    // backdrop boundary.
    final double dpr = ui.window.devicePixelRatio;
    ui.Rect rect = transformLTRB(_invertedTransform, 0, 0,
        ui.window.physicalSize.width * dpr, ui.window.physicalSize.height);
    PersistedSurface parentSurface = parent;
    String left = '{rect.left}px';
    String top = '{rect.top}px';
    String width = '${rect.width}px';
    String height = '${rect.height}px';
    while (parentSurface != null) {
      html.HtmlElement parentElement = parentSurface.rootElement;
      if (parentElement.tagName == 'flt-clip' || parentElement.tagName == 'FLT-CLIP') {
        final html.CssStyleDeclaration parentElementStyle = parentElement.style;
        left = parentElementStyle.left;
        top = parentElementStyle.top;
        width = parentElementStyle.width;
        height = parentElementStyle.height;
        break;
      }
      parentSurface = parentSurface.parent;
    }
    final html.CssStyleDeclaration filterElementStyle = _filterElement.style;
    filterElementStyle
      ..position = 'absolute'
      ..left = left
      ..top = top
      ..width = width
      ..height = height;
    if (browserEngine == BrowserEngine.firefox) {
      // For FireFox for now render transparent black background.
      // TODO(flutter_web): Switch code to use filter when
      // See https://caniuse.com/#feat=css-backdrop-filter.
      filterElementStyle
        ..backgroundColor = '#000'
        ..opacity = '0.2';
    } else {
      // CSS uses pixel radius for blur. Flutter & SVG use sigma parameters. For
      // Gaussian blur with standard deviation (normal distribution),
      // the blur will fall within 2 * sigma pixels.
      if (browserEngine == BrowserEngine.webkit) {
        domRenderer.setElementStyle(_filterElement, '-webkit-backdrop-filter',
            _imageFilterToCss(filter));
      }
      domRenderer.setElementStyle(_filterElement, 'backdrop-filter', _imageFilterToCss(filter));
    }
  }

  @override
  void update(PersistedBackdropFilter oldSurface) {
    super.update(oldSurface);
    if (filter != oldSurface.filter) {
      apply();
    }
  }
}
