// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <memory>

#include "flutter/fml/macros.h"
#include "flutter/fml/time/time_delta.h"
#include "flutter/testing/testing.h"
#include "impeller/geometry/scalar.h"
#include "impeller/playground/playground.h"

namespace impeller {

class ComputePlaygroundTest
    : public Playground,
      public ::testing::TestWithParam<PlaygroundBackend> {
 public:
  ComputePlaygroundTest();

  virtual ~ComputePlaygroundTest();

  void SetUp() override;

  void TearDown() override;

  // |Playground|
  std::unique_ptr<fml::Mapping> OpenAssetAsMapping(
      std::string asset_name) const override;

  std::shared_ptr<RuntimeStage> OpenAssetAsRuntimeStage(
      const char* asset_name) const;

  // |Playground|
  std::string GetWindowTitle() const override;

  /// @brief Get the amount of time elapsed from the start of the playground
  ///        test's execution.
  Scalar GetSecondsElapsed() const;

 private:
  fml::TimeDelta start_time_;

  FML_DISALLOW_COPY_AND_ASSIGN(ComputePlaygroundTest);
};

#define INSTANTIATE_COMPUTE_SUITE(playground)                              \
  INSTANTIATE_TEST_SUITE_P(                                                \
      Compute, playground, ::testing::Values(PlaygroundBackend::kMetal),   \
      [](const ::testing::TestParamInfo<ComputePlaygroundTest::ParamType>& \
             info) { return PlaygroundBackendToString(info.param); });

}  // namespace impeller
