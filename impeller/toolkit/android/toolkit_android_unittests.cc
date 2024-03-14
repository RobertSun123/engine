// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/fml/synchronization/waitable_event.h"
#include "flutter/testing/testing.h"
#include "impeller/toolkit/android/choreographer.h"
#include "impeller/toolkit/android/hardware_buffer.h"
#include "impeller/toolkit/android/proc_table.h"
#include "impeller/toolkit/android/surface_control.h"
#include "impeller/toolkit/android/surface_transaction.h"

namespace impeller::android::testing {

class ToolkitAndroidTest : public ::testing::Test {
 public:
  void SetUp() override {
    // The toolkit is only available on Android API levels over 29. Skip these
    // tests everywhere else.
    if (__builtin_available(android 29, *)) {
    } else {
      GTEST_SKIP() << "Platform too old for this test.";
    }
  }
};

TEST_F(ToolkitAndroidTest, CanCreateProcTable) {
  ProcTable proc_table;
  ASSERT_TRUE(proc_table.IsValid());
}

TEST_F(ToolkitAndroidTest, GuardsAgainstZeroSizedDescriptors) {
  auto desc = HardwareBufferDescriptor::MakeForSwapchainImage({0, 0});
  ASSERT_GT(desc.size.width, 0u);
  ASSERT_GT(desc.size.height, 0u);
}

TEST_F(ToolkitAndroidTest, CanCreateHardwareBuffer) {
  ASSERT_TRUE(HardwareBuffer::IsAvailableOnPlatform());
  auto desc = HardwareBufferDescriptor::MakeForSwapchainImage({100, 100});
  ASSERT_TRUE(desc.IsAllocatable());
  HardwareBuffer buffer(desc);
  ASSERT_TRUE(buffer.IsValid());
}

TEST_F(ToolkitAndroidTest, CanApplySurfaceTransaction) {
  ASSERT_TRUE(SurfaceTransaction::IsAvailableOnPlatform());
  SurfaceTransaction transaction;
  ASSERT_TRUE(transaction.IsValid());
  fml::AutoResetWaitableEvent event;
  ASSERT_TRUE(transaction.Apply([&event]() { event.Signal(); }));
  event.Wait();
}

TEST_F(ToolkitAndroidTest, SurfacControlsAreAvailable) {
  ASSERT_TRUE(SurfaceControl::IsAvailableOnPlatform());
}

TEST_F(ToolkitAndroidTest, CanAccessDeviceAPILevel) {
  // This test will be skipped otherwise.
  ASSERT_GE(GetProcTable().GetAndroidDeviceAPILevel(), 29u);
}

TEST_F(ToolkitAndroidTest, ChoreographerIsAvailable) {
  ASSERT_TRUE(Choreographer::IsAvailableOnPlatform());
}

TEST_F(ToolkitAndroidTest, CanPostAndNotWaitForFrameCallbacks) {
  const auto& choreographer = Choreographer::GetInstance();
  ASSERT_TRUE(choreographer.IsValid());
  ASSERT_TRUE(choreographer.PostFrameCallback([](auto) {}));
}

TEST_F(ToolkitAndroidTest, CanPostAndWaitForFrameCallbacks) {
  if ((true)) {
    GTEST_SKIP()
        << "Disabled till the test harness is in an Android activity. "
           "Running it without one will hang because the choreographer "
           "frame callback will never execute.";
  }
  const auto& choreographer = Choreographer::GetInstance();
  ASSERT_TRUE(choreographer.IsValid());
  fml::AutoResetWaitableEvent event;
  ASSERT_TRUE(choreographer.PostFrameCallback(
      [&event](auto point) { event.Signal(); }));
  event.Wait();
}

}  // namespace impeller::android::testing
