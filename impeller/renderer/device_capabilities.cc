// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "impeller/renderer/device_capabilities.h"

namespace impeller {

DeviceCapabilities::DeviceCapabilities(bool threading_restrictions,
                                       bool offscreen_msaa,
                                       bool supports_ssbo)
    : threading_restrictions_(threading_restrictions),
      offscreen_msaa_(offscreen_msaa),
      supports_ssbo_(supports_ssbo) {}

DeviceCapabilities::~DeviceCapabilities() = default;

bool DeviceCapabilities::HasThreadingRestrictions() const {
  return threading_restrictions_;
}

bool DeviceCapabilities::SupportsOffscreenMSAA() const {
  return offscreen_msaa_;
}

bool DeviceCapabilities::SupportsSSBO() const {
  return supports_ssbo_;
}

DeviceCapabilitiesBuilder::DeviceCapabilitiesBuilder() = default;

DeviceCapabilitiesBuilder::~DeviceCapabilitiesBuilder() = default;

DeviceCapabilitiesBuilder& DeviceCapabilitiesBuilder::HasThreadingRestrictions(
    bool value) {
  threading_restrictions_ = value;
  return *this;
}

DeviceCapabilitiesBuilder& DeviceCapabilitiesBuilder::SupportsOffscreenMSAA(
    bool value) {
  offscreen_msaa_ = value;
  return *this;
}

DeviceCapabilitiesBuilder& DeviceCapabilitiesBuilder::SupportsSSBO(bool value) {
  supports_ssbo_ = value;
  return *this;
}

std::unique_ptr<DeviceCapabilities> DeviceCapabilitiesBuilder::Build() {
  DeviceCapabilities* capabilities = new DeviceCapabilities(
      threading_restrictions_, offscreen_msaa_, supports_ssbo_);
  return std::unique_ptr<DeviceCapabilities>(capabilities);
}

}  // namespace impeller
