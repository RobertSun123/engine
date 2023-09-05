// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_SHELL_PLATFORM_WINDOWS_FLUTTER_WINDOW_H_
#define FLUTTER_SHELL_PLATFORM_WINDOWS_FLUTTER_WINDOW_H_

#include <string>
#include <vector>

#include "flutter/fml/macros.h"
#include "flutter/shell/platform/common/geometry.h"
#include "flutter/shell/platform/embedder/embedder.h"
#include "flutter/shell/platform/windows/flutter_windows_view.h"
#include "flutter/shell/platform/windows/window.h"
#include "flutter/shell/platform/windows/window_binding_handler.h"

namespace flutter {

// A win32 flutter child window used as implementations for flutter view.  In
// the future, there will likely be a CoreWindow-based FlutterWindow as well.
// At the point may make sense to dependency inject the native window rather
// than inherit.
class FlutterWindow : public KeyboardManager::WindowDelegate, public WindowBindingHandler {
 public:
  // Create flutter Window for use as child window
  FlutterWindow(int width, int height, std::unique_ptr<WindowsProcTable> windows_proc_table = nullptr, std::unique_ptr<TextInputManager> text_input_manager = nullptr);

  virtual ~FlutterWindow();

  // Initializes as a child window with size using |width| and |height| and
  // |title| to identify the windowclass.  Does not show window, window must be
  // parented into window hierarchy by caller.
  void InitializeChild(const char* title,
                       unsigned int width,
                       unsigned int height);

  HWND GetWindowHandle();

  // |KeyboardManager::WindowDelegate|
  virtual BOOL Win32PeekMessage(LPMSG lpMsg,
                                UINT wMsgFilterMin,
                                UINT wMsgFilterMax,
                                UINT wRemoveMsg) override;

  // |KeyboardManager::WindowDelegate|
  virtual uint32_t Win32MapVkToChar(uint32_t virtual_key) override;

  // |KeyboardManager::WindowDelegate|
  virtual UINT Win32DispatchMessage(UINT Msg,
                                    WPARAM wParam,
                                    LPARAM lParam) override;

  // |Window|
  void OnDpiScale(unsigned int dpi);

  // |Window|
  void OnResize(unsigned int width, unsigned int height);

  // |Window|
  void OnPaint();

  // |Window|
  void OnPointerMove(double x,
                     double y,
                     FlutterPointerDeviceKind device_kind,
                     int32_t device_id,
                     int modifiers_state);

  // |Window|
  void OnPointerDown(double x,
                     double y,
                     FlutterPointerDeviceKind device_kind,
                     int32_t device_id,
                     UINT button);

  // |Window|
  void OnPointerUp(double x,
                   double y,
                   FlutterPointerDeviceKind device_kind,
                   int32_t device_id,
                   UINT button);

  // |Window|
  void OnPointerLeave(double x,
                      double y,
                      FlutterPointerDeviceKind device_kind,
                      int32_t device_id);

  // |Window|
  void OnSetCursor();

  // |Window|
  void OnText(const std::u16string& text) override;

  // |Window|
  void OnKey(int key,
             int scancode,
             int action,
             char32_t character,
             bool extended,
             bool was_down,
             KeyEventCallback callback) override;

  // |Window|
  void OnComposeBegin();

  // |Window|
  void OnComposeCommit();

  // |Window|
  void OnComposeEnd();

  // |Window|
  void OnComposeChange(const std::u16string& text, int cursor_pos);

  // |FlutterWindowBindingHandler|
  void OnCursorRectUpdated(const Rect& rect) override;

  // |FlutterWindowBindingHandler|
  void OnResetImeComposing() override;

  // |Window|
  void OnUpdateSemanticsEnabled(bool enabled);

  // |Window|
  void OnScroll(double delta_x,
                double delta_y,
                FlutterPointerDeviceKind device_kind,
                int32_t device_id);

  // |Window|
  gfx::NativeViewAccessible GetNativeViewAccessible();

  // |FlutterWindowBindingHandler|
  void SetView(WindowBindingHandlerDelegate* view) override;

  // |FlutterWindowBindingHandler|
  WindowsRenderTarget GetRenderTarget() override;

  // |FlutterWindowBindingHandler|
  PlatformWindow GetPlatformWindow() override;

  // |FlutterWindowBindingHandler|
  float GetDpiScale() override;

  // |FlutterWindowBindingHandler|
  bool IsVisible() override;

  // |FlutterWindowBindingHandler|
  PhysicalWindowBounds GetPhysicalWindowBounds() override;

  // |FlutterWindowBindingHandler|
  void UpdateFlutterCursor(const std::string& cursor_name) override;

  // |FlutterWindowBindingHandler|
  void SetFlutterCursor(HCURSOR cursor) override;

  // |FlutterWindowBindingHandler|
  void OnWindowResized() override;

  // |FlutterWindowBindingHandler|
  bool OnBitmapSurfaceUpdated(const void* allocation,
                              size_t row_bytes,
                              size_t height) override;

  // |FlutterWindowBindingHandler|
  PointerLocation GetPrimaryPointerLocation() override;

  // |Window|
  void OnThemeChange();

  // |WindowBindingHandler|
  void SendInitialAccessibilityFeatures() override;

  // |WindowBindingHandler|
  AlertPlatformNodeDelegate* GetAlertDelegate() override;

  // |WindowBindingHandler|
  ui::AXPlatformNodeWin* GetAlert() override;

  // |WindowBindingHandler|
  bool NeedsVSync() override;

  // |Window|
  ui::AXFragmentRootDelegateWin* GetAxFragmentRootDelegate();

  // |Window|
  virtual void OnWindowStateEvent(WindowStateEvent event);

 protected:
  // Win32's DefWindowProc.
  //
  // Used as the fallback behavior of HandleMessage. Exposed for dependency
  // injection.
  virtual LRESULT Win32DefWindowProc(HWND hWnd,
                                     UINT Msg,
                                     WPARAM wParam,
                                     LPARAM lParam);
  // Release OS resources associated with the window.
  void Destroy();

  // Converts a c string to a wide unicode string.
  std::wstring NarrowToWide(const char* source);

  // Registers a window class with default style attributes, cursor and
  // icon.
  WNDCLASS RegisterWindowClass(std::wstring& title);

  // OS callback called by message pump.  Handles the WM_NCCREATE message which
  // is passed when the non-client area is being created and enables automatic
  // non-client DPI scaling so that the non-client area automatically
  // responsponds to changes in DPI.  All other messages are handled by
  // MessageHandler.
  static LRESULT CALLBACK WndProc(HWND const window,
                                  UINT const message,
                                  WPARAM const wparam,
                                  LPARAM const lparam) noexcept;

  // Processes and route salient window messages for mouse handling,
  // size change and DPI.  Delegates handling of these to member overloads that
  // inheriting classes can handle.
  LRESULT HandleMessage(UINT const message,
                        WPARAM const wparam,
                        LPARAM const lparam) noexcept;

  // Called when the OS requests a COM object.
  //
  // The primary use of this function is to supply Windows with wrapped
  // semantics objects for use by Windows accessibility.
  virtual LRESULT OnGetObject(UINT const message,
                              WPARAM const wparam,
                              LPARAM const lparam);

  // Called when a window is activated in order to configure IME support for
  // multi-step text input.
  virtual void OnImeSetContext(UINT const message,
                               WPARAM const wparam,
                               LPARAM const lparam);

  // Called when multi-step text input begins when using an IME.
  virtual void OnImeStartComposition(UINT const message,
                                     WPARAM const wparam,
                                     LPARAM const lparam);

  // Called when edits/commit of multi-step text input occurs when using an IME.
  virtual void OnImeComposition(UINT const message,
                                WPARAM const wparam,
                                LPARAM const lparam);

  // Called when multi-step text input ends when using an IME.
  virtual void OnImeEndComposition(UINT const message,
                                   WPARAM const wparam,
                                   LPARAM const lparam);

  // Called when the user triggers an IME-specific request such as input
  // reconversion, where an existing input sequence is returned to composing
  // mode to select an alternative candidate conversion.
  virtual void OnImeRequest(UINT const message,
                            WPARAM const wparam,
                            LPARAM const lparam);

  // Called when the app ends IME composing, such as when the text input client
  // is cleared or changed.
  virtual void AbortImeComposing();

  // Called when the cursor rect has been updated.
  //
  // |rect| is in Win32 window coordinates.
  virtual void UpdateCursorRect(const Rect& rect);

  UINT GetCurrentDPI();

  UINT GetCurrentWidth();

  UINT GetCurrentHeight();

  // Returns the current pixel per scroll tick value.
  virtual float GetScrollOffsetMultiplier();

  // Check if the high contrast feature is enabled on the OS
  virtual bool GetHighContrastEnabled();

  // Creates the ax_fragment_root_, alert_delegate_ and alert_node_ if they do
  // not yet exist.
  // Once set, they are not reset to nullptr.
  void CreateAxFragmentRoot();

  // Delegate to a alert_node_ used to set the announcement text.
  std::unique_ptr<AlertPlatformNodeDelegate> alert_delegate_;

  // Accessibility node that represents an alert.
  std::unique_ptr<ui::AXPlatformNodeWin> alert_node_;

  // Handles running DirectManipulation on the window to receive trackpad
  // gestures.
  std::unique_ptr<DirectManipulationOwner> direct_manipulation_owner_;

 private:
  // A pointer to a FlutterWindowsView that can be used to update engine
  // windowing and input state.
  WindowBindingHandlerDelegate* binding_handler_delegate_;

  // The last cursor set by Flutter. Defaults to the arrow cursor.
  HCURSOR current_cursor_;

  // The cursor rect set by Flutter.
  RECT cursor_rect_;

  // The window receives resize and focus messages before its view is set, so
  // these values cache the state of the window in the meantime so that the
  // proper application lifecycle state can be updated once the view is set.
  bool restored_ = false;
  bool focused_ = false;
  // Activates tracking for a "mouse leave" event.
  void TrackMouseLeaveEvent(HWND hwnd);

  // Stores new width and height and calls |OnResize| to notify inheritors
  void HandleResize(UINT width, UINT height);

  // Retrieves a class instance pointer for |window|
  static FlutterWindow* GetThisFromHandle(HWND const window) noexcept;

  // Updates the cached scroll_offset_multiplier_ value based off OS settings.
  void UpdateScrollOffsetMultiplier();

  int current_dpi_ = 0;
  int current_width_ = 0;
  int current_height_ = 0;

  // Holds the conversion factor from lines scrolled to pixels scrolled.
  float scroll_offset_multiplier_;

  // WM_DPICHANGED_BEFOREPARENT defined in more recent Windows
  // SDK
  const static long kWmDpiChangedBeforeParent = 0x02E2;

  // Member variable to hold window handle.
  HWND window_handle_ = nullptr;

  // Member variable to hold the window title.
  std::wstring window_class_name_;

  // Set to true to be notified when the mouse leaves the window.
  bool tracking_mouse_leave_ = false;

  // Keeps track of the last key code produced by a WM_KEYDOWN or WM_SYSKEYDOWN
  // message.
  int keycode_for_char_message_ = 0;

  // Keeps track of the last mouse coordinates by a WM_MOUSEMOVE message.
  double mouse_x_ = 0;
  double mouse_y_ = 0;

  // Abstracts Windows APIs that may not be available on all supported versions
  // of Windows.
  std::unique_ptr<WindowsProcTable> windows_proc_table_;

  // Manages IME state.
  std::unique_ptr<TextInputManager> text_input_manager_;

  // Manages IME state.
  std::unique_ptr<KeyboardManager> keyboard_manager_;

  // Used for temporarily storing the WM_TOUCH-provided touch points.
  std::vector<TOUCHINPUT> touch_points_;

  // Generates touch point IDs for touch events.
  SequentialIdGenerator touch_id_generator_;

  // Timer identifier for DirectManipulation gesture polling.
  const static int kDirectManipulationTimer = 1;

  // Implements IRawElementProviderFragmentRoot when UIA is enabled.
  std::unique_ptr<ui::AXFragmentRootWin> ax_fragment_root_;

  // Allow WindowAXFragmentRootDelegate to access protected method.
  friend class WindowAXFragmentRootDelegate;

  FML_DISALLOW_COPY_AND_ASSIGN(FlutterWindow);
};

}  // namespace flutter

#endif  // FLUTTER_SHELL_PLATFORM_WINDOWS_FLUTTER_WINDOW_H_
