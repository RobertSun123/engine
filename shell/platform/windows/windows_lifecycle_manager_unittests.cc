// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/windows/windows_lifecycle_manager.h"

#include "flutter/shell/platform/windows/testing/windows_test.h"
#include "gtest/gtest.h"

namespace flutter{
namespace testing{

class WindowsLifecycleManagerTest : public WindowsTest {};

TEST_F(WindowsLifecycleManagerTest, StateTransitions) {
  WindowsLifecycleManager manager(nullptr);
  HWND win1 = (HWND)1;
  HWND win2 = (HWND)2;

  // Hidden to inactive upon window shown.
  manager.SetLifecycleState(AppLifecycleState::kHidden);
  manager.OnWindowStateEvent(win1, SHOW);
  EXPECT_EQ(manager.GetLifecycleState(), AppLifecycleState::kInactive);

  // Showing a second window does not change state.
  manager.OnWindowStateEvent(win2, SHOW);
  EXPECT_EQ(manager.GetLifecycleState(), AppLifecycleState::kInactive);

  // Inactive to resumed upon window focus.
  manager.OnWindowStateEvent(win2, FOCUS);
  EXPECT_EQ(manager.GetLifecycleState(), AppLifecycleState::kResumed);

  // Showing a second window does not change state.
  manager.OnWindowStateEvent(win1, FOCUS);
  EXPECT_EQ(manager.GetLifecycleState(), AppLifecycleState::kResumed);

  // Unfocusing one window does not change state while another is focused.
  manager.OnWindowStateEvent(win1, UNFOCUS);
  EXPECT_EQ(manager.GetLifecycleState(), AppLifecycleState::kResumed);

  // Unfocusing final remaining focused window transitions to inactive.
  manager.OnWindowStateEvent(win2, UNFOCUS);
  EXPECT_EQ(manager.GetLifecycleState(), AppLifecycleState::kInactive);

  // Hiding one of two visible windows does not change state.
  manager.OnWindowStateEvent(win2, HIDE);
  EXPECT_EQ(manager.GetLifecycleState(), AppLifecycleState::kInactive);

  // Hiding only visible window transitions to hidden.
  manager.OnWindowStateEvent(win1, HIDE);
  EXPECT_EQ(manager.GetLifecycleState(), AppLifecycleState::kHidden);

  // Transition directly from resumed to hidden when the window is hidden.
  manager.OnWindowStateEvent(win1, SHOW);
  manager.OnWindowStateEvent(win1, FOCUS);
  manager.OnWindowStateEvent(win1, HIDE);
  EXPECT_EQ(manager.GetLifecycleState(), AppLifecycleState::kHidden);
}

}
}
