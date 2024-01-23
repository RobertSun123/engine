// Copyright 2024 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_LIB_UI_WINDOW_PLATFORM_ISOLATE_H_
#define FLUTTER_LIB_UI_WINDOW_PLATFORM_ISOLATE_H_

#include "third_party/dart/runtime/include/dart_api.h"

namespace flutter {

class PlatformIsolateNativeApi {
 public:
  static void Spawn(Dart_Handle entry_point,
                    Dart_Handle isolate_ready_port,
                    Dart_Handle debug_name,
                    bool errors_are_fatal);

  // Using this function to verify we're on the platform thread for prototyping.
  // TODO: Need to figure out a better way of doing this.
  static bool IsRunningOnPlatformThread();
};

}  // namespace flutter

#endif  // FLUTTER_LIB_UI_WINDOW_PLATFORM_ISOLATE_H_
