// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

part of core;

class MojoHandle {
  static const int INVALID = 0;
  static const int DEADLINE_INDEFINITE = -1;

  int _h;
  int get h => _h;

  MojoHandle(this._h, {String description}) {
    MojoHandleNatives.addOpenHandle(_h, description: description);
  }

  MojoHandle._internal(this._h);

  MojoHandle.invalid() : this._internal(INVALID);

  MojoResult close() {
    MojoHandleNatives.removeOpenHandle(_h);
    int result = MojoHandleNatives.close(_h);
    _h = INVALID;
    return new MojoResult(result);
  }

  MojoHandle pass() {
    MojoHandleNatives.removeOpenHandle(_h);
    return this;
  }

  MojoWaitResult wait(int signals, int deadline) {
    List result = MojoHandleNatives.wait(h, signals, deadline);
    var state = result[1] != null
        ? new MojoHandleSignalsState(result[1][0], result[1][1])
        : null;
    return new MojoWaitResult(new MojoResult(result[0]), state);
  }

  bool _ready(MojoHandleSignals signal) {
    MojoWaitResult mwr = wait(signal.value, 0);
    switch (mwr.result) {
      case MojoResult.OK:
        return true;
      case MojoResult.DEADLINE_EXCEEDED:
      case MojoResult.CANCELLED:
      case MojoResult.INVALID_ARGUMENT:
      case MojoResult.FAILED_PRECONDITION:
        return false;
      default:
        // Should be unreachable.
        throw new MojoInternalError("Unexpected result $mwr for wait on $h");
    }
  }

  bool get readyRead => _ready(MojoHandleSignals.PEER_CLOSED_READABLE);
  bool get readyWrite => _ready(MojoHandleSignals.WRITABLE);
  bool get isValid => (_h != INVALID);

  String toString() {
    if (!isValid) {
      return "MojoHandle(INVALID)";
    }
    var mwr = wait(MojoHandleSignals.kAll, 0);
    return "MojoHandle(h: $h, status: $mwr)";
  }

  bool operator ==(MojoHandle other) {
    return _h == other._h;
  }

  static MojoWaitManyResult waitMany(
      List<int> handles, List<int> signals, int deadline) {
    List result = MojoHandleNatives.waitMany(handles, signals, deadline);
    List states = result[2] != null
        ? result[2].map((l) => new MojoHandleSignalsState(l[0], l[1])).toList()
        : null;
    return new MojoWaitManyResult(new MojoResult(result[0]), result[1], states);
  }

  static bool registerFinalizer(MojoEventSubscription eventSubscription) {
    return MojoHandleNatives.registerFinalizer(
        eventSubscription, eventSubscription._handle.h) == MojoResult.kOk;
  }

  static bool reportLeakedHandles() => MojoHandleNatives.reportOpenHandles();
}
