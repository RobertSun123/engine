// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_SHELL_PLATFORM_FUCHSIA_FOCUS_DELEGATE_H_
#define FLUTTER_SHELL_PLATFORM_FUCHSIA_FOCUS_DELEGATE_H_

#include <fuchsia/sys/cpp/fidl.h>
#include <fuchsia/ui/views/cpp/fidl.h>

#include "flutter/fml/macros.h"
#include "flutter/lib/ui/window/platform_message.h"
#include "third_party/rapidjson/include/rapidjson/document.h"

namespace flutter_runner {

class FocusDelegate {
 public:
  FocusDelegate(fidl::InterfaceHandle<fuchsia::ui::views::ViewRefFocused>
                    view_ref_focused,
                fidl::InterfaceHandle<fuchsia::ui::views::Focuser> focuser)
      : view_ref_focused_(view_ref_focused.Bind()), focuser_(focuser.Bind()) {}

  virtual ~FocusDelegate() = default;

  /// Continuously watches the host viewRef for focus events, invoking a
  /// callback each time.
  ///
  /// This can only be called once.
  virtual void WatchLoop(std::function<void(bool)> callback);

  /// Completes the platform message request with the FocusDelegate's most
  /// recent focus state.
  virtual void CompleteCurrentFocusState(
      fml::RefPtr<flutter::PlatformMessageResponse> response);

  /// Completes the platform message request with the FocusDelegate's next focus
  /// state.
  ///
  /// Only one outstanding request may exist at a time. Any others will be
  /// completed empty.
  virtual void CompleteNextFocusState(
      fml::RefPtr<flutter::PlatformMessageResponse> response);

  /// Completes a platform message request by attempting to give focus for a
  /// given viewRef.
  virtual void RequestFocus(
      rapidjson::Value request,
      fml::RefPtr<flutter::PlatformMessageResponse> response);

 private:
  fuchsia::ui::views::ViewRefFocusedPtr view_ref_focused_;
  fuchsia::ui::views::FocuserPtr focuser_;

  std::function<void(fuchsia::ui::views::FocusState)> watch_loop_;
  bool is_focused_;
  fml::RefPtr<flutter::PlatformMessageResponse> next_focus_request_;

  void Complete(fml::RefPtr<flutter::PlatformMessageResponse> response,
                bool value);

  FML_DISALLOW_COPY_AND_ASSIGN(FocusDelegate);
};

}  // namespace flutter_runner
#endif  // FLUTTER_SHELL_PLATFORM_FUCHSIA_FOCUS_DELEGATE_H_
