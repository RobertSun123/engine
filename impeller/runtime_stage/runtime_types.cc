// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "impeller/runtime_stage/runtime_types.h"

namespace impeller {

size_t RuntimeUniformDescription::GetSize() const {
  size_t size = dimensions.rows * dimensions.cols * bit_width / 8u;
  if (array_elements.value_or(0) > 0) {
    size *= array_elements.value();
  }
  return size;
}

}  // namespace impeller
