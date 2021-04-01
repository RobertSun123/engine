// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_SHELL_PLATFORM_WINDOWS_WINDOW_BINDING_HANDLER_H_
#define FLUTTER_SHELL_PLATFORM_WINDOWS_WINDOW_BINDING_HANDLER_H_

#include <windows.h>

#include <string>
#include <variant>

#include "flutter/shell/platform/common/geometry.h"
#include "flutter/shell/platform/windows/public/flutter_windows.h"
#include "flutter/shell/platform/windows/window_binding_handler_delegate.h"

#ifdef WINUWP
#include <third_party/cppwinrt/generated/winrt/Windows.UI.Composition.h>
#endif

namespace flutter {

class FlutterWindowsView;

// Structure containing physical bounds of a Window
struct PhysicalWindowBounds {
  size_t width;
  size_t height;
};

#ifdef WINUWP
using WindowsRenderTarget =
    std::variant<winrt::Windows::UI::Composition::SpriteVisual,
                 winrt::Windows::UI::Core::CoreWindow,
                 HWND>;
#else
using WindowsRenderTarget = std::variant<HWND>;
#endif

// Abstract class for binding Windows platform windows to Flutter views.
class WindowBindingHandler {
 public:
  virtual ~WindowBindingHandler() = default;

  // Sets the delegate used to communicate state changes from window to view
  // such as key presses, mouse position updates etc.
  virtual void SetView(WindowBindingHandlerDelegate* view) = 0;

  // Returns a valid WindowsRenderTarget representing the backing
  // window.
  virtual WindowsRenderTarget GetRenderTarget() = 0;

  // Returns the scale factor for the backing window.
  virtual float GetDpiScale() = 0;

  // Returns the bounds of the backing window in physical pixels.
  virtual PhysicalWindowBounds GetPhysicalWindowBounds() = 0;

  // Invoked after the window has been resized.
  virtual void OnWindowResized() = 0;

  // Sets the cursor that should be used when the mouse is over the Flutter
  // content. See mouse_cursor.dart for the values and meanings of cursor_name.
  virtual void UpdateFlutterCursor(const std::string& cursor_name) = 0;

  // Invoked when the cursor/composing rect has been updated in the framework.
  virtual void OnCursorRectUpdated(const Rect& rect) = 0;

  // Invoked when the Embedder provides us with new bitmap data for the contents
  // of this Flutter view.
  //
  // Returns whether the surface was successfully updated or not.
  virtual bool OnBitmapSurfaceUpdated(const void* allocation,
                                      size_t row_bytes,
                                      size_t height) = 0;
};

}  // namespace flutter

#endif  // FLUTTER_SHELL_PLATFORM_WINDOWS_WINDOW_BINDING_HANDLER_H_
