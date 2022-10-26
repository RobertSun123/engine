// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fuchsia/accessibility/semantics/cpp/fidl.h>
#include <fuchsia/buildinfo/cpp/fidl.h>
#include <fuchsia/component/cpp/fidl.h>
#include <fuchsia/fonts/cpp/fidl.h>
#include <fuchsia/intl/cpp/fidl.h>
#include <fuchsia/kernel/cpp/fidl.h>
#include <fuchsia/memorypressure/cpp/fidl.h>
#include <fuchsia/metrics/cpp/fidl.h>
#include <fuchsia/net/interfaces/cpp/fidl.h>
#include <fuchsia/sysmem/cpp/fidl.h>
#include <fuchsia/tracing/provider/cpp/fidl.h>
#include <fuchsia/ui/app/cpp/fidl.h>
#include <fuchsia/ui/input/cpp/fidl.h>
#include <fuchsia/ui/policy/cpp/fidl.h>
#include <fuchsia/ui/scenic/cpp/fidl.h>
#include <fuchsia/ui/test/input/cpp/fidl.h>
#include <fuchsia/ui/test/scene/cpp/fidl.h>
#include <fuchsia/web/cpp/fidl.h>
#include <lib/async-loop/testing/cpp/real_loop.h>
#include <lib/async/cpp/task.h>
#include <lib/fidl/cpp/binding_set.h>
#include <lib/sys/component/cpp/testing/realm_builder.h>
#include <lib/sys/component/cpp/testing/realm_builder_types.h>
#include <lib/sys/cpp/component_context.h>
#include <lib/ui/scenic/cpp/resources.h>
#include <lib/ui/scenic/cpp/session.h>
#include <lib/ui/scenic/cpp/view_token_pair.h>
#include <lib/zx/clock.h>
#include <lib/zx/time.h>
#include <zircon/status.h>
#include <zircon/types.h>
#include <zircon/utc.h>

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "flutter/fml/logging.h"
#include "flutter/shell/platform/fuchsia/flutter/tests/integration/utils/color.h"
#include "flutter/shell/platform/fuchsia/flutter/tests/integration/utils/portable_ui_test.h"
#include "flutter/shell/platform/fuchsia/flutter/tests/integration/utils/screenshot.h"

// This test exercises the touch input dispatch path from Input Pipeline to a
// Scenic client. It is a multi-component test, and carefully avoids sleeping or
// polling for component coordination.
// - It runs real Root Presenter, Input Pipeline, and Scenic components.
// - It uses a fake display controller; the physical device is unused.
//
// Components involved
// - This test program
// - Input Pipeline
// - Root Presenter
// - Scenic
// - Child view, a Scenic client
//
// Touch dispatch path
// - Test program's injection -> Input Pipeline -> Scenic -> Child view
//
// Setup sequence
// - The test sets up this view hierarchy:
//   - Top level scene, owned by Root Presenter.
//   - Child view, owned by the ui client.
// - The test waits for a Scenic event that verifies the child has UI content in
// the scene graph.
// - The test injects input into Input Pipeline, emulating a display's touch
// report.
// - Input Pipeline dispatches the touch event to Scenic, which in turn
// dispatches it to the child.
// - The child receives the touch event and reports back to the test over a
// custom test-only FIDL.
// - Test waits for the child to report a touch; when the test receives the
// report, the test quits
//   successfully.
//
// This test uses the realm_builder library to construct the topology of
// components and routes services between them. For v2 components, every test
// driver component sits as a child of test_manager in the topology. Thus, the
// topology of a test driver component such as this one looks like this:
//
//     test_manager
//         |
//   touch-input-test.cml (this component)
//
// With the usage of the realm_builder library, we construct a realm during
// runtime and then extend the topology to look like:
//
//    test_manager
//         |
//   touch-input-test.cml (this component)
//         |
//   <created realm root>
//      /      \
//   scenic  input-pipeline
//
// For more information about testing v2 components and realm_builder,
// visit the following links:
//
// Testing: https://fuchsia.dev/fuchsia-src/concepts/testing/v2
// Realm Builder:
// https://fuchsia.dev/fuchsia-src/development/components/v2/realm_builder

namespace touch_input_test::testing {
namespace {
// Types imported for the realm_builder library.
using component_testing::ChildRef;
using component_testing::ConfigValue;
using component_testing::LocalComponent;
using component_testing::LocalComponentHandles;
using component_testing::ParentRef;
using component_testing::Protocol;
using component_testing::Realm;
using component_testing::RealmRoot;
using component_testing::Route;

using fuchsia_test_utils::PortableUITest;

using RealmBuilder = component_testing::RealmBuilder;

// Max timeout in failure cases.
// Set this as low as you can that still works across all test platforms.
constexpr zx::duration kTimeout = zx::min(1);
// Timeout for Scenic's |TakeScreenshot| FIDL call.
constexpr zx::duration kScreenshotTimeout = zx::sec(10);

constexpr auto kTestUIStackUrl =
    "fuchsia-pkg://fuchsia.com/gfx-root-presenter-test-ui-stack#meta/test-ui-stack.cm";

constexpr auto kMockTouchInputListener = "touch_input_listener";
constexpr auto kMockTouchInputListenerRef = ChildRef{kMockTouchInputListener};

constexpr auto kTouchInputView = "touch-input-view";
constexpr auto kTouchInputViewRef = ChildRef{kTouchInputView};
constexpr auto kTouchInputViewUrl =
    "fuchsia-pkg://fuchsia.com/touch-input-view#meta/touch-input-view.cm";
constexpr auto kEmbeddingFlutterView = "embedding-flutter-view";
constexpr auto kEmbeddingFlutterViewRef = ChildRef{kEmbeddingFlutterView};
constexpr auto kEmbeddingFlutterViewUrl =
    "fuchsia-pkg://fuchsia.com/embedding-flutter-view#meta/embedding-flutter-view.cm";

bool CompareDouble(double f0, double f1, double epsilon) {
  return std::abs(f0 - f1) <= epsilon;
}

// This component implements the TouchInput protocol
// and the interface for a RealmBuilder LocalComponent. A LocalComponent
// is a component that is implemented here in the test, as opposed to
// elsewhere in the system. When it's inserted to the realm, it will act
// like a proper component. This is accomplished, in part, because the
// realm_builder library creates the necessary plumbing. It creates a manifest
// for the component and routes all capabilities to and from it.
// LocalComponent:
// https://fuchsia.dev/fuchsia-src/development/testing/components/realm_builder#mock-components
class TouchInputListenerServer
    : public fuchsia::ui::test::input::TouchInputListener,
      public LocalComponent {
 public:
  explicit TouchInputListenerServer(async_dispatcher_t* dispatcher)
      : dispatcher_(dispatcher) {}

  // |fuchsia::ui::test::input::TouchInputListener|
  void ReportTouchInput(
      fuchsia::ui::test::input::TouchInputListenerReportTouchInputRequest
          request) override {
    FML_LOG(INFO) << "Received ReportTouchInput event";
    events_received_.push_back(std::move(request));
  }

  // |LocalComponent::Start|
  // When the component framework requests for this component to start, this
  // method will be invoked by the realm_builder library.
  void Start(std::unique_ptr<LocalComponentHandles> local_handles) override {
    FML_LOG(INFO) << "Starting TouchInputListenerServer";
    // When this component starts, add a binding to the
    // protocol to this component's outgoing directory.
    ASSERT_EQ(ZX_OK, local_handles->outgoing()->AddPublicService(
                         fidl::InterfaceRequestHandler<
                             fuchsia::ui::test::input::TouchInputListener>(
                             [this](auto request) {
                               bindings_.AddBinding(this, std::move(request),
                                                    dispatcher_);
                             })));
    local_handles_.emplace_back(std::move(local_handles));
  }

  const std::vector<
      fuchsia::ui::test::input::TouchInputListenerReportTouchInputRequest>&
  events_received() {
    return events_received_;
  }

 private:
  async_dispatcher_t* dispatcher_ = nullptr;
  std::vector<std::unique_ptr<LocalComponentHandles>> local_handles_;
  fidl::BindingSet<fuchsia::ui::test::input::TouchInputListener> bindings_;
  std::vector<
      fuchsia::ui::test::input::TouchInputListenerReportTouchInputRequest>
      events_received_;
};

class FlutterTapTestBase : public PortableUITest,
                       public ::testing::Test,
                       public ::testing::WithParamInterface<std::string> {
 protected:
  ~FlutterTapTestBase() override {
    FML_CHECK(touch_injection_request_count() > 0)
        << "Injection expected but didn't happen.";
  }

  void SetUp() override {
    PortableUITest::SetUp();

    // Post a "just in case" quit task, if the test hangs.
    async::PostDelayedTask(
        dispatcher(),
        [] {
          FML_LOG(FATAL)
              << "\n\n>> Test did not complete in time, terminating.  <<\n\n";
        },
        kTimeout);
  }

  bool LastEventReceivedMatches(float expected_x,
                                float expected_y,
                                std::string component_name) {
    const auto& events_received =
        touch_input_listener_server_->events_received();

    if (events_received.empty()) {
      return false;
    }

    const auto& last_event = events_received.back();

    auto pixel_scale = last_event.has_device_pixel_ratio()
                           ? last_event.device_pixel_ratio()
                           : 1;

    auto actual_x = pixel_scale * last_event.local_x();
    auto actual_y = pixel_scale * last_event.local_y();
    auto actual_component = last_event.component_name();

    bool last_event_matches = CompareDouble(actual_x, expected_x, pixel_scale) &&
           CompareDouble(actual_y, expected_y, pixel_scale) &&
           last_event.component_name() == component_name;

    if (last_event_matches) {
      FML_LOG(INFO) << "Received event for component " << component_name
                    << " at (" << expected_x << ", " << expected_y << ")";
    } else {
      FML_LOG(WARNING) << "Expecting event for component " << component_name
                    << " at (" << expected_x << ", " << expected_y << "). "
                    << "Instead received event for component " << actual_component
                    << " at (" << actual_x << ", " << actual_y
                    << "), accounting for pixel scale of " << pixel_scale;
    }

    return last_event_matches;
  }

  // Guaranteed to be initialized after SetUp().
  uint32_t display_width() const { return display_width_; }
  uint32_t display_height() const { return display_height_; }

  ParamType GetTestUIStackUrl() override { return GetParam(); };

  std::unique_ptr<TouchInputListenerServer> touch_input_listener_server_;
};

class FlutterTapTest : public FlutterTapTestBase {
  private:
   void ExtendRealm() override {
     FML_LOG(INFO) << "Extending realm";
     // Key part of service setup: have this test component vend the
     // |TouchInputListener| service in the constructed realm.
     touch_input_listener_server_ =
         std::make_unique<TouchInputListenerServer>(dispatcher());
     realm_builder()->AddLocalChild(kMockTouchInputListener,
                                    touch_input_listener_server_.get());

     // Add touch-input-view to the Realm
     realm_builder()->AddChild(kTouchInputView, kTouchInputViewUrl,
                               component_testing::ChildOptions{
                                   .environment = kFlutterRunnerEnvironment,
                               });

     // Route the TouchInput protocol capability to the Dart component
     realm_builder()->AddRoute(
         Route{.capabilities = {Protocol{
                   fuchsia::ui::test::input::TouchInputListener::Name_}},
               .source = kMockTouchInputListenerRef,
               .targets = {kFlutterJitRunnerRef, kTouchInputViewRef}});

     realm_builder()->AddRoute(Route{
         .capabilities = {Protocol{fuchsia::ui::app::ViewProvider::Name_}},
         .source = kTouchInputViewRef,
         .targets = {ParentRef()}});
   }
};

class FlutterEmbedTapTest : public FlutterTapTestBase {
  protected:
    // Takes a screenshot and waits for child view to embed itself to the parent view
    bool WaitForEmbed() {
      return RunLoopWithTimeoutOrUntil([this] {
        constexpr fuchsia_test_utils::Color kChildBackgroundColor = {
            0xFF, 0x00, 0xFF, 0xFF};  // Pink
        fuchsia::ui::scenic::ScreenshotData screenshot_out;
        scenic_->TakeScreenshot(
            [this, &screenshot_out](
                fuchsia::ui::scenic::ScreenshotData screenshot, bool status) {
              EXPECT_TRUE(status) << "Failed to take screenshot";
              screenshot_out = std::move(screenshot);
              QuitLoop();
            });
        EXPECT_FALSE(RunLoopWithTimeout(kScreenshotTimeout))
            << "Timed out waiting for screenshot.";

        auto screenshot = fuchsia_test_utils::Screenshot(screenshot_out);
        auto histogram = screenshot.Histogram();

        bool color_found = histogram[kChildBackgroundColor] > 0;
        return color_found;
      },
      kTimeout);
    }

  private:
   void ExtendRealm() override {
     FML_LOG(INFO) << "Extending realm";
     // Key part of service setup: have this test component vend the
     // |TouchInputListener| service in the constructed realm.
     touch_input_listener_server_ =
         std::make_unique<TouchInputListenerServer>(dispatcher());
     realm_builder()->AddLocalChild(kMockTouchInputListener,
                                    touch_input_listener_server_.get());

     // Add touch-input-view to the Realm
     realm_builder()->AddChild(kTouchInputView, kTouchInputViewUrl,
                               component_testing::ChildOptions{
                                   .environment = kFlutterRunnerEnvironment,
                               });
     // Add embedding-flutter-view to the Realm
     // This component will embed touch-input-view as a child view
     realm_builder()->AddChild(kEmbeddingFlutterView, kEmbeddingFlutterViewUrl,
                               component_testing::ChildOptions{
                                   .environment = kFlutterRunnerEnvironment,
                               });

     // Route the TouchInput protocol capability to the Dart component
     realm_builder()->AddRoute(
         Route{.capabilities = {Protocol{
                   fuchsia::ui::test::input::TouchInputListener::Name_}},
               .source = kMockTouchInputListenerRef,
               .targets = {kFlutterJitRunnerRef, kTouchInputViewRef,
                           kEmbeddingFlutterViewRef}});

     realm_builder()->AddRoute(Route{
         .capabilities = {Protocol{fuchsia::ui::app::ViewProvider::Name_}},
         .source = kEmbeddingFlutterViewRef,
         .targets = {ParentRef()}});
     realm_builder()->AddRoute(Route{
         .capabilities = {Protocol{fuchsia::ui::app::ViewProvider::Name_}},
         .source = kTouchInputViewRef,
         .targets = {kEmbeddingFlutterViewRef}});
   }
};

// Makes use of gtest's parameterized testing, allowing us
// to test different combinations of test-ui-stack + runners. Currently, there
// is just one combination. Documentation:
// http://go/gunitadvanced#value-parameterized-tests
INSTANTIATE_TEST_SUITE_P(
    FlutterTapTestParameterized,
    FlutterTapTest,
    ::testing::Values(kTestUIStackUrl));

TEST_P(FlutterTapTest, FlutterTap) {
  // Launch client view, and wait until it's rendering to proceed with the test.
  FML_LOG(INFO) << "Initializing scene";
  LaunchClient();
  FML_LOG(INFO) << "Client launched";

  // touch-input-view logical coordinate space doesn't match the fake touch
  // screen injector's coordinate space, which spans [-1000, 1000] on both axes.
  // Scenic handles figuring out where in the coordinate space
  //  to inject a touch event (this is fixed to a display's bounds).
  InjectTap(-500, -500);
  // For a (-500 [x], -500 [y]) tap, we expect a touch event in the middle of
  // the upper-left quadrant of the screen.
  RunLoopUntil([this] {
    return LastEventReceivedMatches(
        /*expected_x=*/static_cast<float>(display_width() / 4.0f),
        /*expected_y=*/static_cast<float>(display_height() / 4.0f),
        /*component_name=*/"touch-input-view");
  });
}

INSTANTIATE_TEST_SUITE_P(
    FlutterEmbedTapTestParameterized,
    FlutterEmbedTapTest,
    ::testing::Values(kTestUIStackUrl));

TEST_P(FlutterEmbedTapTest, FlutterEmbedTap) {
  // Launch view
  FML_LOG(INFO) << "Initializing scene";
  LaunchClient();
  FML_LOG(INFO) << "Client launched";
  // Wait for child view to embed and render to proceed with the test.
  WaitForEmbed();
  FML_LOG(INFO) << "Embedded child view has rendered";

  {
    // Embedded child view takes up the left side of the screen
    // Expect a response from the child view if we inject a tap there
    InjectTap(-500, -500);
    RunLoopUntil([this] {
      return LastEventReceivedMatches(
          /*expected_x=*/static_cast<float>(display_width() / 4.0f),
          /*expected_y=*/static_cast<float>(display_height() / 4.0f),
          /*component_name=*/"touch-input-view");
    });
  }

  {
    // Parent view takes up the right side of the screen
    // Validate that parent can still receive taps
    InjectTap(500, 500);
    RunLoopUntil([this] {
      return LastEventReceivedMatches(
          /*expected_x=*/static_cast<float>(display_width() / (4.0f / 3.0f)),
          /*expected_y=*/static_cast<float>(display_height() / (4.0f / 3.0f)),
          /*component_name=*/"embedding-flutter-view");
    });
  }

  // There should be 2 injected taps
  ASSERT_EQ(touch_injection_request_count(), 2);
}

}  // namespace
}  // namespace touch_input_test::testing
