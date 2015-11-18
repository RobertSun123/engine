// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SKY_ENGINE_TONIC_DART_IO_H_
#define SKY_ENGINE_TONIC_DART_IO_H_

#include "base/macros.h"

namespace blink {

class DartIO {
 public:
  static void InitForIsolate();

  static void SetCaptureStdout(bool value);
  static void SetCaptureStderr(bool value);

  static bool capture_stdout() {
    return capture_stdout_;
  }

 private:
  static bool capture_stdout_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(DartIO);
};

}  // namespace blink

#endif  // SKY_ENGINE_TONIC_DART_IO_H_
