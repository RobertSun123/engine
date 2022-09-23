// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "backtrace.h"

#include "gtest/gtest.h"
#include "third_party/abseil-cpp/absl/debugging/stacktrace.h"

namespace fml {
namespace testing {

TEST(BacktraceTest, CanGatherBacktrace) {
  if (!IsCrashHandlingSupported()) {
    GTEST_SKIP();
    return;
  }
  {
    auto trace = BacktraceHere(0);
    ASSERT_GT(trace.size(), 0u);
    ASSERT_NE(trace.find("Frame 0"), std::string::npos);
    std::cout << trace << std::endl;
  }
}

TEST(BacktraceTest, BacktraceEnabled) {
  ASSERT_EQ(absl::debugging_internal::StackTraceWorksForTest(), true);
}

}  // namespace testing
}  // namespace fml
