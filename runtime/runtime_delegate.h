// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_RUNTIME_RUNTIME_DELEGATE_H_
#define FLUTTER_RUNTIME_RUNTIME_DELEGATE_H_

#include <memory>
#include <vector>

#include "flutter/flow/layers/layer_tree.h"
#include "flutter/lib/ui/semantics/custom_accessibility_action.h"
#include "flutter/lib/ui/semantics/semantics_node.h"
#include "flutter/lib/ui/text/font_collection.h"
#include "flutter/lib/ui/window/platform_message.h"
#include "third_party/dart/runtime/include/dart_api.h"

namespace flutter {

class RuntimeDelegate {
 public:
  virtual std::string DefaultRouteName() = 0;

  virtual void ScheduleFrame(bool regenerate_layer_tree = true) = 0;

  virtual void Render(std::unique_ptr<flutter::LayerTree> layer_tree) = 0;

  virtual void UpdateSemantics(SemanticsNodeUpdates update,
                               CustomAccessibilityActionUpdates actions) = 0;

  virtual void HandlePlatformMessage(fml::RefPtr<PlatformMessage> message) = 0;

  virtual FontCollection& GetFontCollection() = 0;

  virtual void UpdateIsolateDescription(const std::string isolate_name,
                                        int64_t isolate_port) = 0;

  virtual void SetNeedsReportTimings(bool value) = 0;

  virtual std::vector<std::string>& ComputePlatformResolvedLocale(
      const std::vector<std::string>& supportedLocaleData) = 0;

 protected:
  virtual ~RuntimeDelegate();
};

}  // namespace flutter

#endif  // FLUTTER_RUNTIME_RUNTIME_DELEGATE_H_
