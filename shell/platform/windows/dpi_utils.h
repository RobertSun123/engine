// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "Windows.h"

#ifndef FLUTTER_SHELL_PLATFORM_WINDOWS_DPI_UTILS_H_
#define FLUTTER_SHELL_PLATFORM_WINDOWS_DPI_UTILS_H_

namespace flutter {

UINT GetDpiForHWND(HWND hwnd);
BOOL EnableNonClientDpiScaling(HWND hwnd);

}  // namespace flutter

#endif  // FLUTTER_SHELL_PLATFORM_WINDOWS_DPI_UTILS_H_
