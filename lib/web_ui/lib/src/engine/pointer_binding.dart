// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

part of engine;

/// Set this flag to true to see all the fired events in the console.
const bool _debugLogPointerEvents = false;

/// The signature of a callback that handles pointer events.
typedef _PointerDataCallback = void Function(Iterable<ui.PointerData>);

// The mask for the bitfield of event buttons. Buttons not contained in this
// mask are cut off.
//
// In Flutter we used `kMaxUnsignedSMI`, but since that value is not available
// here, we use an already very large number (30 bits).
const int _kButtonsMask = 0x3FFFFFFF;

// Intentionally set to -1 so it doesn't conflict with other device IDs.
const int _mouseDeviceId = -1;

const int _kPrimaryMouseButton = 1;

class PointerBinding {
  /// The singleton instance of this object.
  static PointerBinding get instance => _instance;
  static PointerBinding _instance;

  static void initInstance(html.Element glassPaneElement) {
    if (_instance == null) {
      _instance = PointerBinding._(glassPaneElement);
      assert(() {
        registerHotRestartListener(() {
          _instance._adapter?.clearListeners();
          _instance._pointerDataConverter?.clearPointerState();
        });
        return true;
      }());
    }
  }

  PointerBinding._(this.glassPaneElement) {
    _pointerDataConverter = PointerDataConverter();
    _detector = const PointerSupportDetector();
    _adapter = _createAdapter();
  }

  final html.Element glassPaneElement;

  PointerSupportDetector _detector;
  _BaseAdapter _adapter;
  PointerDataConverter _pointerDataConverter;

  /// Should be used in tests to define custom detection of pointer support.
  ///
  /// ```dart
  /// // Forces PointerBinding to use mouse events.
  /// class MyTestDetector extends PointerSupportDetector {
  ///   @override
  ///   final bool hasPointerEvents = false;
  ///
  ///   @override
  ///   final bool hasTouchEvents = false;
  ///
  ///   @override
  ///   final bool hasMouseEvents = true;
  /// }
  ///
  /// PointerBinding.instance.debugOverrideDetector(MyTestDetector());
  /// ```
  void debugOverrideDetector(PointerSupportDetector newDetector) {
    newDetector ??= const PointerSupportDetector();
    // When changing the detector, we need to swap the adapter.
    if (newDetector != _detector) {
      _detector = newDetector;
      _adapter?.clearListeners();
      _adapter = _createAdapter();
      _pointerDataConverter?.clearPointerState();
    }
  }

  _BaseAdapter _createAdapter() {
    if (_detector.hasPointerEvents) {
      return _PointerAdapter(_onPointerData, glassPaneElement, _pointerDataConverter);
    }
    if (_detector.hasTouchEvents) {
      return _TouchAdapter(_onPointerData, glassPaneElement, _pointerDataConverter);
    }
    if (_detector.hasMouseEvents) {
      return _MouseAdapter(_onPointerData, glassPaneElement, _pointerDataConverter);
    }
    return null;
  }

  void _onPointerData(Iterable<ui.PointerData> data) {
    final ui.PointerDataPacket packet = ui.PointerDataPacket(data: data.toList());
    final ui.PointerDataPacketCallback callback = ui.window.onPointerDataPacket;
    if (callback != null) {
      callback(packet);
    }
  }
}

class PointerSupportDetector {
  const PointerSupportDetector();

  bool get hasPointerEvents => js_util.hasProperty(html.window, 'PointerEvent');
  bool get hasTouchEvents => js_util.hasProperty(html.window, 'TouchEvent');
  bool get hasMouseEvents => js_util.hasProperty(html.window, 'MouseEvent');

  @override
  String toString() =>
      'pointers:$hasPointerEvents, touch:$hasTouchEvents, mouse:$hasMouseEvents';
}

/// Common functionality that's shared among adapters.
abstract class _BaseAdapter {
  _BaseAdapter(this._callback, this.glassPaneElement, this._pointerDataConverter) {
    setup();
  }

  /// Listeners that are registered through dart to js api.
  static final Map<String, html.EventListener> _listeners =
    <String, html.EventListener>{};
  /// Listeners that are registered through native javascript api.
  static final Map<String, html.EventListener> _nativeListeners =
    <String, html.EventListener>{};
  final html.Element glassPaneElement;
  _PointerDataCallback _callback;
  PointerDataConverter _pointerDataConverter;

  /// Each subclass is expected to override this method to attach its own event
  /// listeners and convert events into pointer events.
  void setup();

  /// Remove all active event listeners.
  void clearListeners() {
    _listeners.forEach((String eventName, html.EventListener listener) {
        glassPaneElement.removeEventListener(eventName, listener, true);
    });
    // For native listener, we will need to remove it through native javascript
    // api.
    _nativeListeners.forEach((String eventName, html.EventListener listener) {
      js_util.callMethod(
        glassPaneElement,
        'removeEventListener', <dynamic>[
          'wheel',
          listener,
        ]
      );
    });
    _listeners.clear();
    _nativeListeners.clear();
  }

  void addEventListener(String eventName, html.EventListener handler) {
    final html.EventListener loggedHandler = (html.Event event) {
      if (_debugLogPointerEvents) {
        print(event.type);
      }
      // Report the event to semantics. This information is used to debounce
      // browser gestures. Semantics tells us whether it is safe to forward
      // the event to the framework.
      if (EngineSemanticsOwner.instance.receiveGlobalEvent(event)) {
        handler(event);
      }
    };
    _listeners[eventName] = loggedHandler;
    glassPaneElement
        .addEventListener(eventName, loggedHandler, true);
  }

  /// Converts a floating number timestamp (in milliseconds) to a [Duration] by
  /// splitting it into two integer components: milliseconds + microseconds.
  static Duration _eventTimeStampToDuration(num milliseconds) {
    final int ms = milliseconds.toInt();
    final int micro =
    ((milliseconds - ms) * Duration.microsecondsPerMillisecond).toInt();
    return Duration(milliseconds: ms, microseconds: micro);
  }
}

mixin _WheelEventListenerMixin on _BaseAdapter {
  List<ui.PointerData> _convertWheelEventToPointerData(
    html.WheelEvent event
  ) {
    const int domDeltaPixel = 0x00;
    const int domDeltaLine = 0x01;
    const int domDeltaPage = 0x02;

    // Flutter only supports pixel scroll delta. Convert deltaMode values
    // to pixels.
    double deltaX = event.deltaX;
    double deltaY = event.deltaY;
    switch (event.deltaMode) {
      case domDeltaLine:
        deltaX *= 32.0;
        deltaY *= 32.0;
        break;
      case domDeltaPage:
        deltaX *= ui.window.physicalSize.width;
        deltaY *= ui.window.physicalSize.height;
        break;
      case domDeltaPixel:
      default:
        break;
    }
    final List<ui.PointerData> data = <ui.PointerData>[];
    _pointerDataConverter.convert(
      data,
      change: ui.PointerChange.hover,
      timeStamp: _BaseAdapter._eventTimeStampToDuration(event.timeStamp),
      kind: ui.PointerDeviceKind.mouse,
      signalKind: ui.PointerSignalKind.scroll,
      device: _mouseDeviceId,
      physicalX: event.client.x * ui.window.devicePixelRatio,
      physicalY: event.client.y * ui.window.devicePixelRatio,
      buttons: event.buttons,
      pressure: 1.0,
      pressureMin: 0.0,
      pressureMax: 1.0,
      scrollDeltaX: deltaX,
      scrollDeltaY: deltaY,
    );
    return data;
  }

  void _addWheelEventListener(html.EventListener handler) {
    final dynamic eventOptions = js_util.newObject();
    final html.EventListener jsHandler = js.allowInterop((html.Event event) => handler(event));
    _BaseAdapter._nativeListeners['wheel'] = jsHandler;
    js_util.setProperty(eventOptions, 'passive', false);
    js_util.callMethod(
      glassPaneElement,
      'addEventListener', <dynamic>[
        'wheel',
        jsHandler,
        eventOptions
      ]
    );
  }
}

@immutable
class _SanitizedDetails {
  const _SanitizedDetails({
    @required this.buttons,
    @required this.change,
  });

  final ui.PointerChange change;
  final int buttons;

  @override
  String toString() => '$runtimeType(change: $change, buttons: $buttons)';
}

class _ButtonSanitizer {
  int _pressedButtons = 0;

  // Transform html.PointerEvent.buttons to Flutter's PointerEvent buttons.
  int _htmlButtonsToFlutterButtons(int buttons) {
    // Flutter's button definition conveniently matches that of JavaScript
    // from primary button (0x1) to forward button (0x10), which allows us to
    // avoid transforming it bit by bit.
    return buttons & _kButtonsMask;
  }

  List<_SanitizedDetails> sanitizeDownEvent({@required int buttons}) {
    final List<_SanitizedDetails> result = <_SanitizedDetails>[];
    // TODO(flutter_web): Remove this temporary fix for right click
    // on web platform once context gesture is implemented.
    if (_pressedButtons != 0) {
      _pressedButtons = 0;
      result.add(_SanitizedDetails(
        change: ui.PointerChange.up,
        buttons: 0,
      ));
    }
    _pressedButtons = _htmlButtonsToFlutterButtons(buttons);
    result.add(_SanitizedDetails(
      change: ui.PointerChange.down,
      buttons: _pressedButtons,
    ));
    return result;
  }

  List<_SanitizedDetails> sanitizeMoveEvent({@required int buttons}) {
    _pressedButtons = _htmlButtonsToFlutterButtons(buttons);
    return <_SanitizedDetails>[_SanitizedDetails(
      change: _pressedButtons == 0
          ? ui.PointerChange.hover
          : ui.PointerChange.move,
      buttons: _pressedButtons,
    )];
  }

  List<_SanitizedDetails> sanitizeUpEvent() {
    // The pointer could have been released by a `pointerout` event, in which
    // case `pointerup` should have no effect.
    if (_pressedButtons == 0) {
      return <_SanitizedDetails>[];
    }
    _pressedButtons = 0;
    return <_SanitizedDetails>[_SanitizedDetails(
      change: ui.PointerChange.up,
      buttons: _pressedButtons,
    )];
  }

  List<_SanitizedDetails> sanitizeCancelEvent() {
    _pressedButtons = 0;
    return <_SanitizedDetails>[_SanitizedDetails(
      change: ui.PointerChange.cancel,
      buttons: _pressedButtons,
    )];
  }
}

typedef _PointerEventListener = dynamic Function(html.PointerEvent event);

/// Adapter class to be used with browsers that support native pointer events.
class _PointerAdapter extends _BaseAdapter with _WheelEventListenerMixin {
  _PointerAdapter(
    _PointerDataCallback callback,
    html.Element glassPaneElement,
    PointerDataConverter _pointerDataConverter
  ) : super(callback, glassPaneElement, _pointerDataConverter);

  final Map<int, _ButtonSanitizer> _sanitizers = <int, _ButtonSanitizer>{};

  @visibleForTesting
  Iterable<int> debugTrackedDevices() => _sanitizers.keys;

  _ButtonSanitizer _ensureSanitizer(int device) {
    return _sanitizers.putIfAbsent(device, () => _ButtonSanitizer());
  }

  _ButtonSanitizer _getSanitizer(int device) {
    final _ButtonSanitizer sanitizer = _sanitizers[device];
    assert(sanitizer != null);
    return sanitizer;
  }

  void _removePointerIfUnhoverable(List<ui.PointerData> pointerData, html.PointerEvent event) {
    if (event.pointerType == 'touch') {
      _sanitizers.remove(event.pointerId);
      _convertEventToPointerData(
        data: pointerData,
        event: event,
        details: _SanitizedDetails(
          buttons: 0,
          change: ui.PointerChange.remove,
        ),
      );
    }
  }

  void _addPointerEventListener(String eventName, _PointerEventListener handler) {
    addEventListener(eventName, (html.Event event) {
      final html.PointerEvent pointerEvent = event;
      return handler(pointerEvent);
    });
  }

  @override
  void setup() {
    _addPointerEventListener('pointerdown', (html.PointerEvent event) {
      final int device = event.pointerId;
      final List<ui.PointerData> pointerData = <ui.PointerData>[];
      final _ButtonSanitizer sanitizer = _ensureSanitizer(device);
      for (_SanitizedDetails details in sanitizer.sanitizeDownEvent(buttons: event.buttons)) {
        _convertEventToPointerData(data: pointerData, event: event, details: details);
      }
      _callback(pointerData);
    });

    _addPointerEventListener('pointermove', (html.PointerEvent event) {
      final int device = event.pointerId;
      final _ButtonSanitizer sanitizer = _ensureSanitizer(device);
      final List<ui.PointerData> pointerData = <ui.PointerData>[];
      for (html.PointerEvent expandedEvent in _expandEvents(event)) {
        for (_SanitizedDetails details in sanitizer.sanitizeMoveEvent(buttons: expandedEvent.buttons)) {
          _convertEventToPointerData(data: pointerData, event: event, details: details);
        }
      }
      _callback(pointerData);
    });

    _addPointerEventListener('pointerup', (html.PointerEvent event) {
      final int device = event.pointerId;
      final List<ui.PointerData> pointerData = <ui.PointerData>[];
      final _ButtonSanitizer sanitizer = _getSanitizer(device);
      for (_SanitizedDetails details in sanitizer.sanitizeUpEvent()) {
        _convertEventToPointerData(data: pointerData, event: event, details: details);
      }
      _removePointerIfUnhoverable(pointerData, event);
      _callback(pointerData);
    });

    // A browser fires cancel event if it concludes the pointer will no longer
    // be able to generate events (example: device is deactivated)
    // TODO(dkwingsmt): Add tests for cancel
    _addPointerEventListener('pointercancel', (html.PointerEvent event) {
      final int device = event.pointerId;
      final List<ui.PointerData> pointerData = <ui.PointerData>[];
      final _ButtonSanitizer sanitizer = _getSanitizer(device);
      for (_SanitizedDetails details in sanitizer.sanitizeCancelEvent()) {
        _convertEventToPointerData(data: pointerData, event: event, details: details);
      }
      _removePointerIfUnhoverable(pointerData, event);
      _callback(pointerData);
    });

    _addWheelEventListener((html.Event event) {
      assert(event is html.WheelEvent);
      if (_debugLogPointerEvents) {
        print(event.type);
      }
      _callback(_convertWheelEventToPointerData(event));
      // Prevent default so mouse wheel event doesn't get converted to
      // a scroll event that semantic nodes would process.
      event.preventDefault();
    });
  }

  void _convertEventToPointerData({
    @required List<ui.PointerData> data,
    @required html.PointerEvent event,
    @required _SanitizedDetails details,
  }) {
    final ui.PointerDeviceKind kind = _pointerTypeToDeviceKind(event.pointerType);
    // We force `device: _mouseDeviceId` on mouse pointers because Wheel events
    // might come before any PointerEvents, and since wheel events don't contain
    // pointerId we always assign `device: _mouseDeviceId` to them.
    final int device = kind == ui.PointerDeviceKind.mouse ? _mouseDeviceId : event.pointerId;
    _pointerDataConverter.convert(
      data,
      change: details.change,
      timeStamp: _BaseAdapter._eventTimeStampToDuration(event.timeStamp),
      kind: kind,
      signalKind: ui.PointerSignalKind.none,
      device: device,
      physicalX: event.client.x * ui.window.devicePixelRatio,
      physicalY: event.client.y * ui.window.devicePixelRatio,
      buttons: details.buttons,
      pressure: event.pressure,
      pressureMin: 0.0,
      pressureMax: 1.0,
      tilt: _computeHighestTilt(event),
    );
  }

  List<html.PointerEvent> _expandEvents(html.PointerEvent event) {
    // For browsers that don't support `getCoalescedEvents`, we fallback to
    // using the original event.
    if (js_util.hasProperty(event, 'getCoalescedEvents')) {
      final List<html.PointerEvent> coalescedEvents =
          event.getCoalescedEvents();
      // Some events don't perform coalescing, so they return an empty list. In
      // that case, we also fallback to using the original event.
      if (coalescedEvents.isNotEmpty) {
        return coalescedEvents;
      }
    }
    return <html.PointerEvent>[event];
  }

  ui.PointerDeviceKind _pointerTypeToDeviceKind(String pointerType) {
    switch (pointerType) {
      case 'mouse':
        return ui.PointerDeviceKind.mouse;
      case 'pen':
        return ui.PointerDeviceKind.stylus;
      case 'touch':
        return ui.PointerDeviceKind.touch;
      default:
        return ui.PointerDeviceKind.unknown;
    }
  }

  /// Tilt angle is -90 to + 90. Take maximum deflection and convert to radians.
  double _computeHighestTilt(html.PointerEvent e) =>
      (e.tiltX.abs() > e.tiltY.abs() ? e.tiltX : e.tiltY).toDouble() /
      180.0 *
      math.pi;
}

typedef _TouchEventListener = dynamic Function(html.TouchEvent event);

/// Adapter to be used with browsers that support touch events.
class _TouchAdapter extends _BaseAdapter {
  _TouchAdapter(
    _PointerDataCallback callback,
    html.Element glassPaneElement,
    PointerDataConverter _pointerDataConverter
  ) : super(callback, glassPaneElement, _pointerDataConverter);

  final Set<int> _pressedTouches = <int>{};
  bool _isTouchPressed(int identifier) => _pressedTouches.contains(identifier);
  void _pressTouch(int identifier) { _pressedTouches.add(identifier); }
  void _unpressTouch(int identifier) { _pressedTouches.remove(identifier); }

  void _addTouchEventListener(String eventName, _TouchEventListener handler) {
    addEventListener(eventName, (html.Event event) {
      final html.TouchEvent touchEvent = event;
      return handler(touchEvent);
    });
  }

  @override
  void setup() {
    _addTouchEventListener('touchstart', (html.TouchEvent event) {
      final Duration timeStamp = _BaseAdapter._eventTimeStampToDuration(event.timeStamp);
      final List<ui.PointerData> pointerData = <ui.PointerData>[];
      for (html.Touch touch in event.changedTouches) {
        final nowPressed = _isTouchPressed(touch.identifier);
        if (!nowPressed) {
          _pressTouch(touch.identifier);
          _convertEventToPointerData(
            data: pointerData,
            change: ui.PointerChange.down,
            touch: touch,
            pressed: true,
            timeStamp: timeStamp,
          );
        }
      }
      _callback(pointerData);
    });

    _addTouchEventListener('touchmove', (html.TouchEvent event) {
      event.preventDefault(); // Prevents standard overscroll on iOS/Webkit.
      final Duration timeStamp = _BaseAdapter._eventTimeStampToDuration(event.timeStamp);
      final List<ui.PointerData> pointerData = <ui.PointerData>[];
      for (html.Touch touch in event.changedTouches) {
        final nowPressed = _isTouchPressed(touch.identifier);
        if (nowPressed) {
          _convertEventToPointerData(
            data: pointerData,
            change: ui.PointerChange.move,
            touch: touch,
            pressed: true,
            timeStamp: timeStamp,
          );
        }
      }
      _callback(pointerData);
    });

    _addTouchEventListener('touchend', (html.TouchEvent event) {
      // On Safari Mobile, the keyboard does not show unless this line is
      // added.
      event.preventDefault();
      final Duration timeStamp = _BaseAdapter._eventTimeStampToDuration(event.timeStamp);
      final List<ui.PointerData> pointerData = <ui.PointerData>[];
      for (html.Touch touch in event.changedTouches) {
        final nowPressed = _isTouchPressed(touch.identifier);
        if (nowPressed) {
          _unpressTouch(touch.identifier);
          _convertEventToPointerData(
            data: pointerData,
            change: ui.PointerChange.up,
            touch: touch,
            pressed: false,
            timeStamp: timeStamp,
          );
          _convertEventToPointerData(
            data: pointerData,
            change: ui.PointerChange.remove,
            touch: touch,
            pressed: false,
            timeStamp: timeStamp,
          );
        }
      }
      _callback(pointerData);
    });

    // TODO(dkwingsmt): Add tests for cancel
    _addTouchEventListener('touchcancel', (html.TouchEvent event) {
      final Duration timeStamp = _BaseAdapter._eventTimeStampToDuration(event.timeStamp);
      final List<ui.PointerData> pointerData = <ui.PointerData>[];
      for (html.Touch touch in event.changedTouches) {
        final nowPressed = _isTouchPressed(touch.identifier);
        if (nowPressed) {
          _unpressTouch(touch.identifier);
          _convertEventToPointerData(
            data: pointerData,
            change: ui.PointerChange.cancel,
            touch: touch,
            pressed: false,
            timeStamp: timeStamp,
          );
          _convertEventToPointerData(
            data: pointerData,
            change: ui.PointerChange.remove,
            touch: touch,
            pressed: false,
            timeStamp: timeStamp,
          );
        }
      }
    });
  }

  void _convertEventToPointerData({
    @required List<ui.PointerData> data,
    @required ui.PointerChange change,
    @required html.Touch touch,
    @required bool pressed,
    @required Duration timeStamp,
  }) {
    _pointerDataConverter.convert(
      data,
      change: change,
      timeStamp: timeStamp,
      kind: ui.PointerDeviceKind.touch,
      signalKind: ui.PointerSignalKind.none,
      device: touch.identifier,
      physicalX: touch.client.x * ui.window.devicePixelRatio,
      physicalY: touch.client.y * ui.window.devicePixelRatio,
      buttons: pressed ? _kPrimaryMouseButton : 0,
      pressure: 1.0,
      pressureMin: 0.0,
      pressureMax: 1.0,
    );
  }
}

typedef _MouseEventListener = dynamic Function(html.MouseEvent event);

/// Adapter to be used with browsers that support mouse events.
class _MouseAdapter extends _BaseAdapter with _WheelEventListenerMixin {
  _MouseAdapter(
    _PointerDataCallback callback,
    html.Element glassPaneElement,
    PointerDataConverter _pointerDataConverter
  ) : super(callback, glassPaneElement, _pointerDataConverter);

  final _ButtonSanitizer _sanitizer = _ButtonSanitizer();

  void _addMouseEventListener(String eventName, _MouseEventListener handler) {
    addEventListener(eventName, (html.Event event) {
      final html.MouseEvent mouseEvent = event;
      return handler(mouseEvent);
    });
  }

  @override
  void setup() {
    _addMouseEventListener('mousedown', (html.MouseEvent event) {
      final List<ui.PointerData> pointerData = <ui.PointerData>[];
      for (_SanitizedDetails details in _sanitizer.sanitizeDownEvent(buttons: event.buttons)) {
        _convertEventToPointerData(data: pointerData, event: event, details: details);
      }
      _callback(pointerData);
    });

    _addMouseEventListener('mousemove', (html.MouseEvent event) {
      final List<ui.PointerData> pointerData = <ui.PointerData>[];
      for (_SanitizedDetails details in _sanitizer.sanitizeMoveEvent(buttons: event.buttons)) {
        _convertEventToPointerData(data: pointerData, event: event, details: details);
      }
      _callback(pointerData);
    });

    _addMouseEventListener('mouseup', (html.MouseEvent event) {
      final List<ui.PointerData> pointerData = <ui.PointerData>[];
      for (_SanitizedDetails details in _sanitizer.sanitizeUpEvent()) {
        _convertEventToPointerData(data: pointerData, event: event, details: details);
      }
      _callback(pointerData);
    });

    _addWheelEventListener((html.Event event) {
      assert(event is html.WheelEvent);
      if (_debugLogPointerEvents) {
        print(event.type);
      }
      _callback(_convertWheelEventToPointerData(event));
      // Prevent default so mouse wheel event doesn't get converted to
      // a scroll event that semantic nodes would process.
      event.preventDefault();
    });
  }

  void _convertEventToPointerData({
    @required List<ui.PointerData> data,
    @required html.MouseEvent event,
    @required _SanitizedDetails details,
  }) {
    assert(event != null);
    assert(details != null);
    _pointerDataConverter.convert(
      data,
      change: details.change,
      timeStamp: _BaseAdapter._eventTimeStampToDuration(event.timeStamp),
      kind: ui.PointerDeviceKind.mouse,
      signalKind: ui.PointerSignalKind.none,
      device: _mouseDeviceId,
      physicalX: event.client.x * ui.window.devicePixelRatio,
      physicalY: event.client.y * ui.window.devicePixelRatio,
      buttons: details.buttons,
      pressure: 1.0,
      pressureMin: 0.0,
      pressureMax: 1.0,
    );
  }
}
