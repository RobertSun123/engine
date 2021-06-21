// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <cstddef>

#include "flutter/fml/build_config.h"
#include "flutter/fml/macros.h"

namespace impeller {

constexpr size_t DefaultUniformAlignment() {
#if OS_IOS
  return 16u;
#elif OS_MACOSX
  return 256u;
#else
#error "Unsupported platform".
#endif
}

}  // namespace impeller
