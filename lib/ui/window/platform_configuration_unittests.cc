// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#define FML_USED_ON_EMBEDDER

#include <memory>

#include "flutter/lib/ui/window/platform_configuration.h"

#include "flutter/common/task_runners.h"
#include "flutter/fml/synchronization/waitable_event.h"
#include "flutter/lib/ui/painting/vertices.h"
#include "flutter/runtime/dart_vm.h"
#include "flutter/shell/common/shell_test.h"
#include "flutter/shell/common/thread_host.h"
#include "flutter/testing/testing.h"

namespace flutter {
namespace testing {

class DummyPlatformConfigurationClient : public PlatformConfigurationClient {
 public:
  DummyPlatformConfigurationClient() {
    std::vector<uint8_t> data;
    isolate_data_.reset(new ::fml::DataMapping(data));
  }
  virtual std::string DefaultRouteName() { return "TestRoute"; }
  virtual void ScheduleFrame() {}
  virtual void Render(Scene* scene) {}
  virtual void UpdateSemantics(SemanticsUpdate* update) {}
  virtual void HandlePlatformMessage(fml::RefPtr<PlatformMessage> message) {}
  virtual FontCollection& GetFontCollection() { return font_collection_; }
  virtual void UpdateIsolateDescription(const std::string isolate_name,
                                        int64_t isolate_port) {}
  virtual void SetNeedsReportTimings(bool value) {}
  virtual std::shared_ptr<const fml::Mapping> GetPersistentIsolateData() {
    return isolate_data_;
  }
  virtual std::unique_ptr<std::vector<std::string>>
  ComputePlatformResolvedLocale(
      const std::vector<std::string>& supported_locale_data) {
    return nullptr;
  };

 private:
  FontCollection font_collection_;
  std::shared_ptr<const fml::Mapping> isolate_data_;
};

TEST_F(ShellTest, PlatformConfigurationInitialization) {
  fml::AutoResetWaitableEvent message_latch;

  auto nativeValidateConfiguration = [&](Dart_NativeArguments args) {
    PlatformConfiguration* configuration =
        UIDartState::Current()->platform_configuration();
    ASSERT_NE(configuration->window(), nullptr);
    ASSERT_EQ(configuration->window()->viewport_metrics().device_pixel_ratio,
              1.0);
    ASSERT_EQ(configuration->window()->viewport_metrics().physical_width, 0.0);
    ASSERT_EQ(configuration->window()->viewport_metrics().physical_height, 0.0);

    message_latch.Signal();
  };

  Settings settings = CreateSettingsForFixture();
  TaskRunners task_runners("test",                  // label
                           GetCurrentTaskRunner(),  // platform
                           CreateNewThread(),       // raster
                           CreateNewThread(),       // ui
                           CreateNewThread()        // io
  );

  AddNativeCallback("ValidateConfiguration",
                    CREATE_NATIVE_ENTRY(nativeValidateConfiguration));

  std::unique_ptr<Shell> shell =
      CreateShell(std::move(settings), std::move(task_runners));

  ASSERT_TRUE(shell->IsSetup());
  auto run_configuration = RunConfiguration::InferFromSettings(settings);
  run_configuration.SetEntrypoint("validateConfiguration");

  shell->RunEngine(std::move(run_configuration), [&](auto result) {
    ASSERT_EQ(result, Engine::RunStatus::Success);
  });

  message_latch.Wait();
  DestroyShell(std::move(shell), std::move(task_runners));
}

TEST_F(ShellTest, PlatformConfigurationWindowMetricsUpdate) {
  fml::AutoResetWaitableEvent message_latch;

  auto nativeValidateConfiguration = [&](Dart_NativeArguments args) {
    PlatformConfiguration* configuration =
        UIDartState::Current()->platform_configuration();

    ASSERT_NE(configuration->window(), nullptr);
    configuration->window()->UpdateWindowMetrics(
        ViewportMetrics{2.0, 10.0, 20.0});
    ASSERT_EQ(configuration->window()->viewport_metrics().device_pixel_ratio,
              2.0);
    ASSERT_EQ(configuration->window()->viewport_metrics().physical_width, 10.0);
    ASSERT_EQ(configuration->window()->viewport_metrics().physical_height,
              20.0);

    message_latch.Signal();
  };

  Settings settings = CreateSettingsForFixture();
  TaskRunners task_runners("test",                  // label
                           GetCurrentTaskRunner(),  // platform
                           CreateNewThread(),       // raster
                           CreateNewThread(),       // ui
                           CreateNewThread()        // io
  );

  AddNativeCallback("ValidateConfiguration",
                    CREATE_NATIVE_ENTRY(nativeValidateConfiguration));

  std::unique_ptr<Shell> shell =
      CreateShell(std::move(settings), std::move(task_runners));

  ASSERT_TRUE(shell->IsSetup());
  auto run_configuration = RunConfiguration::InferFromSettings(settings);
  run_configuration.SetEntrypoint("validateConfiguration");

  shell->RunEngine(std::move(run_configuration), [&](auto result) {
    ASSERT_EQ(result, Engine::RunStatus::Success);
  });

  message_latch.Wait();
  DestroyShell(std::move(shell), std::move(task_runners));
}

}  // namespace testing
}  // namespace flutter
