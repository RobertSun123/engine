// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'package:test/bootstrap/browser.dart';
import 'package:test/test.dart';
import 'package:ui/src/engine.dart';

import '../../common/matchers.dart';

void main() {
  internalBootstrapBrowserTest(() => doTests);
}

void doTests() {
  group('DomManager', () {
    test('fromFlutterViewEmbedderDEPRECATED', () {
      final FlutterViewEmbedder embedder = FlutterViewEmbedder();
      final DomManager domManager =
          DomManager.fromFlutterViewEmbedderDEPRECATED(embedder);

      expect(domManager.rootElement, embedder.flutterViewElementDEPRECATED);
      expect(domManager.renderingHost, embedder.glassPaneShadowDEPRECATED);
      expect(domManager.platformViewsHost, embedder.glassPaneElementDEPRECATED);
      expect(domManager.textEditingHost, embedder.textEditingHostNodeDEPRECATED);
      expect(domManager.semanticsHost, embedder.semanticsHostElementDEPRECATED);
    });
  });

  group('StyleManager', () {
    test('styleSceneHost', () {
      expect(
        () => StyleManager.styleSceneHost(createDomHTMLDivElement()),
        throwsAssertionError,
      );

      final DomElement sceneHost = createDomElement('flt-scene-host');
      StyleManager.styleSceneHost(sceneHost);
      expect(sceneHost.style.pointerEvents, 'none');
      expect(sceneHost.style.opacity, isEmpty);

      final DomElement sceneHost2 = createDomElement('flt-scene-host');
      StyleManager.styleSceneHost(sceneHost2, debugShowSemanticsNodes: true);
      expect(sceneHost2.style.pointerEvents, 'none');
      expect(sceneHost2.style.opacity, isNotEmpty);
    });

    test('styleSemanticsHost', () {
      expect(
        () => StyleManager.styleSemanticsHost(createDomHTMLDivElement(), 1.0),
        throwsAssertionError,
        reason: 'Only accepts a <flt-semantics-host> element.'
      );

      final DomElement semanticsHost = createDomElement('flt-semantics-host');
      StyleManager.styleSemanticsHost(semanticsHost, 4.0);
      expect(semanticsHost.style.transform, 'scale(0.25)');
      expect(semanticsHost.style.position, 'absolute');
      expect(semanticsHost.style.transformOrigin, anyOf('0px 0px 0px', '0px 0px'));
    });

    test('scaleSemanticsHost', () {
      expect(
        () => StyleManager.scaleSemanticsHost(createDomHTMLDivElement(), 1.0),
        throwsAssertionError,
        reason: 'Only accepts a <flt-semantics-host> element.'
      );

      final DomElement semanticsHost = createDomElement('flt-semantics-host');
      StyleManager.scaleSemanticsHost(semanticsHost, 5.0);
      expect(semanticsHost.style.transform, 'scale(0.2)');
      // Didn't set other styles.
      expect(semanticsHost.style.position, isEmpty);
      expect(semanticsHost.style.transformOrigin, isEmpty);
    });
  });
}
