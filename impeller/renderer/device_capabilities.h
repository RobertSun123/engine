// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <memory>

#include "flutter/fml/macros.h"

namespace impeller {

class DeviceCapabilities {
 public:
  ~DeviceCapabilities();

  bool HasThreadingRestrictions() const;

  bool SupportsOffscreenMSAA() const;

  bool SupportsSSBO() const;

 private:
  DeviceCapabilities(bool threading_restrictions,
                     bool offscreen_msaa,
                     bool supports_ssbo);

  friend class DeviceCapabilitiesBuilder;

  bool threading_restrictions_ = false;
  bool offscreen_msaa_ = false;
  bool supports_ssbo_ = false;

  FML_DISALLOW_COPY_AND_ASSIGN(DeviceCapabilities);
};

class DeviceCapabilitiesBuilder {
 public:
  DeviceCapabilitiesBuilder();

  ~DeviceCapabilitiesBuilder();

  DeviceCapabilitiesBuilder& HasThreadingRestrictions(bool value);

  DeviceCapabilitiesBuilder& SupportsOffscreenMSAA(bool value);

  DeviceCapabilitiesBuilder& SupportsSSBO(bool value);

  std::unique_ptr<DeviceCapabilities> Build();

 private:
  bool threading_restrictions_ = false;
  bool offscreen_msaa_ = false;
  bool supports_ssbo_ = false;

  FML_DISALLOW_COPY_AND_ASSIGN(DeviceCapabilitiesBuilder);
};

}  // namespace impeller
