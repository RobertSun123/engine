// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_SHELL_PLATFORM_FUCHSIA_FLUTTER_TESTS_FAKES_SCENIC_FAKE_FLATLAND_H_
#define FLUTTER_SHELL_PLATFORM_FUCHSIA_FLUTTER_TESTS_FAKES_SCENIC_FAKE_FLATLAND_H_

#include <fuchsia/math/cpp/fidl.h>
#include <fuchsia/scenic/scheduling/cpp/fidl.h>
#include <fuchsia/sysmem/cpp/fidl.h>
#include <fuchsia/ui/composition/cpp/fidl.h>
#include <fuchsia/ui/composition/cpp/fidl_test_base.h>
#include <fuchsia/ui/views/cpp/fidl.h>
#include <lib/async/dispatcher.h>
#include <lib/fidl/cpp/binding.h>
#include <lib/fidl/cpp/interface_request.h>

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include "flutter/fml/logging.h"
#include "flutter/fml/macros.h"

#include "fake_flatland_types.h"

namespace flutter_runner::testing {

// A lightweight fake implementation of the Flatland API.
//
// The fake has no side effects besides mutating its own internal state
// according to the rules of interacting with the Flatland API.  It makes that
// internal state available for inspection by a test.
//
// Thus it allows tests to do a few things that would be difficult using either
// a mock implementation or the real implementation:
//   + It allows the user to hook `Present` invocations and respond with
//   stubbed-out `FuturePresentationTimes`, but more crucially it mimics the
//   real Flatland behavior of only processing commands when a `Present` is
//   invoked.
//   + It allows the user to inspect a snapshot of the scene graph at any moment
//   in time, via the `SceneGraph()` accessor.
//   + It stores the various Flatland transforms and content generated by
//   commands into a std::unordered_map, and also correctly manages the
//   transform and content lifetimes via reference counting. This allows a
//   given transform or content to stay alive if its parent still holds a
//   reference to it, in the same way the real Flatland implementation would.
//   + The resources returned by `SceneGraph()` that the test uses for
//   inspection are decoupled from the resources managed internally by the
//   `FakeFlatland` itself -- they are a snapshot of the scene graph at that
//   moment in time, with all snapshot state being cloned from the underlying
//   scene graph state.  This allows the `FakeFlatland` and test to naturally
//   use `shared_ptr` for reference counting and mimic the real Flatland
//   behavior exactly, instead of an awkward index-based API.
//   + TODO(fxb/85619):  Allow injecting {touch,mouse,focus,views} events using
//   the ViewBoundProtocols and {ParentViewport,ChildView}Watcher.
//   + TODO(fxb/85619):  Error handling / flatland disconnection is still WIP.
//   FakeFlatland will likely generate a CHECK in any place where the real
//   Flatland would disconnect or send a FlatlandError.
//
// The fake deliberately does not attempt to handle certain aspects of the real
// Flatland implemntation which are complex or burdensome to implement:
//   +Rendering/interacting with Vulkan in any way
//   +Cross-flatland links between Views and Viewports.
class FakeFlatland
    : public fuchsia::ui::composition::testing::Allocator_TestBase,
      public fuchsia::ui::composition::testing::Flatland_TestBase {
 public:
  using PresentHandler =
      std::function<void(fuchsia::ui::composition::PresentArgs)>;

  FakeFlatland();
  ~FakeFlatland() override;

  bool is_allocator_connected() const { return allocator_binding_.is_bound(); }

  bool is_flatland_connected() const { return flatland_binding_.is_bound(); }

  const std::string& debug_name() const { return debug_name_; }

  const FakeGraph& graph() { return current_graph_; }

  // Bind this instance's Flatland FIDL channel to the `dispatcher` and allow
  // processing of incoming FIDL requests.
  //
  // This can only be called once.
  fuchsia::ui::composition::AllocatorHandle ConnectAllocator(
      async_dispatcher_t* dispatcher = nullptr);

  // Bind this instance's Allocator FIDL channel to the `dispatcher` and allow
  // processing of incoming FIDL requests.
  //
  // This can only be called once.
  fuchsia::ui::composition::FlatlandHandle ConnectFlatland(
      async_dispatcher_t* dispatcher = nullptr);

  // Disconnect the Flatland's FIDL channel with an error.
  // TODO(fxb/85619): Call this internally upon command error, instead of
  // CHECK'ing.
  void Disconnect(fuchsia::ui::composition::FlatlandError error);

  // Set a handler for `Present`-related FIDL calls' return values.
  void SetPresentHandler(PresentHandler present_handler);

  // Fire an `OnNextFrameBegin` event.  Call this first after a `Present` in
  // order to give additional present tokens to the client and simulate scenic's
  // normal event flow.
  void FireOnNextFrameBeginEvent(
      fuchsia::ui::composition::OnNextFrameBeginValues
          on_next_frame_begin_values);

  // Fire an `OnFramePresented` event.  Call this second after a `Present` in
  // order to inform the client of retured frames and simulate scenic's normal
  // event flow.
  void FireOnFramePresentedEvent(
      fuchsia::scenic::scheduling::FramePresentedInfo frame_presented_info);

 private:
  struct ParentViewportWatcher : public fuchsia::ui::composition::testing::
                                     ParentViewportWatcher_TestBase {
   public:
    ParentViewportWatcher(
        fuchsia::ui::views::ViewCreationToken view_token,
        fuchsia::ui::views::ViewIdentityOnCreation view_identity,
        fuchsia::ui::composition::ViewBoundProtocols view_protocols,
        fidl::InterfaceRequest<fuchsia::ui::composition::ParentViewportWatcher>
            parent_viewport_watcher,
        async_dispatcher_t* dispatcher);
    ParentViewportWatcher(ParentViewportWatcher&& other);
    ~ParentViewportWatcher() override;

    // |fuchsia::ui::composition::testing::ParentViewportWatcher_TestBase|
    void NotImplemented_(const std::string& name) override;

    // |fuchsia::ui::composition::ParentViewportWatcher|
    void GetLayout(GetLayoutCallback callback) override;

    // |fuchsia::ui::composition::ParentViewportWatcher|
    void GetStatus(GetStatusCallback callback) override;

    fuchsia::ui::views::ViewCreationToken view_token;
    fuchsia::ui::views::ViewIdentityOnCreation view_identity;
    fuchsia::ui::composition::ViewBoundProtocols view_protocols;
    fidl::Binding<fuchsia::ui::composition::ParentViewportWatcher>
        parent_viewport_watcher;
  };

  struct ChildViewWatcher
      : public fuchsia::ui::composition::testing::ChildViewWatcher_TestBase {
   public:
    ChildViewWatcher(
        fuchsia::ui::views::ViewportCreationToken viewport_token,
        fidl::InterfaceRequest<fuchsia::ui::composition::ChildViewWatcher>
            child_view_watcher,
        async_dispatcher_t* dispatcher);
    ChildViewWatcher(ChildViewWatcher&& other);
    ~ChildViewWatcher() override;

    // |fuchsia::ui::composition::testing::ChildViewWatcher_TestBase|
    void NotImplemented_(const std::string& name) override;

    // |fuchsia::ui::composition::ChildViewWatcher|
    void GetStatus(GetStatusCallback callback) override;

    fuchsia::ui::views::ViewportCreationToken viewport_token;
    fidl::Binding<fuchsia::ui::composition::ChildViewWatcher>
        child_view_watcher;
  };

  struct ImageBinding {
    fuchsia::ui::composition::BufferCollectionImportToken import_token;
  };

  struct BufferCollectionBinding {
    fuchsia::ui::composition::BufferCollectionExportToken export_token;
    fuchsia::sysmem::BufferCollectionTokenHandle sysmem_token;

    fuchsia::ui::composition::RegisterBufferCollectionUsage usage;
  };

  struct GraphBindings {
    std::optional<std::pair<zx_koid_t, ParentViewportWatcher>> viewport_watcher;
    std::unordered_map<zx_koid_t, ChildViewWatcher> view_watchers;
    std::unordered_map<zx_koid_t, ImageBinding> images;
    std::unordered_map<zx_koid_t, BufferCollectionBinding> buffer_collections;
  };

  // |fuchsia::ui::composition::testing::Allocator_TestBase|
  // |fuchsia::ui::composition::testing::Flatland_TestBase|
  void NotImplemented_(const std::string& name) override;

  // |fuchsia::ui::composition::testing::Allocator|
  void RegisterBufferCollection(
      fuchsia::ui::composition::RegisterBufferCollectionArgs args,
      RegisterBufferCollectionCallback callback) override;

  // |fuchsia::ui::composition::Flatland|
  void Present(fuchsia::ui::composition::PresentArgs args) override;

  // |fuchsia::ui::composition::Flatland|
  void CreateView(
      fuchsia::ui::views::ViewCreationToken token,
      fidl::InterfaceRequest<fuchsia::ui::composition::ParentViewportWatcher>
          parent_viewport_watcher) override;

  // |fuchsia::ui::composition::Flatland|
  void CreateView2(
      fuchsia::ui::views::ViewCreationToken token,
      fuchsia::ui::views::ViewIdentityOnCreation view_identity,
      fuchsia::ui::composition::ViewBoundProtocols view_protocols,
      fidl::InterfaceRequest<fuchsia::ui::composition::ParentViewportWatcher>
          parent_viewport_watcher) override;

  // |fuchsia::ui::composition::Flatland|
  void CreateTransform(
      fuchsia::ui::composition::TransformId transform_id) override;

  // |fuchsia::ui::composition::Flatland|
  void SetTranslation(fuchsia::ui::composition::TransformId transform_id,
                      fuchsia::math::Vec translation) override;

  // |fuchsia::ui::composition::Flatland|
  void SetOrientation(
      fuchsia::ui::composition::TransformId transform_id,
      fuchsia::ui::composition::Orientation orientation) override;

  // |fuchsia::ui::composition::Flatland|
  void SetClipBounds(fuchsia::ui::composition::TransformId transform_id,
                     fuchsia::math::Rect clip_bounds) override;

  // TODO(fxbug.dev/89111): Re-enable once SDK rolls.
  //   // |fuchsia::ui::composition::Flatland|
  //   void SetImageOpacity(fuchsia::ui::composition::ContentId image_id,
  //                        float opacity) override;

  // |fuchsia::ui::composition::Flatland|
  void AddChild(
      fuchsia::ui::composition::TransformId parent_transform_id,
      fuchsia::ui::composition::TransformId child_transform_id) override;

  // |fuchsia::ui::composition::Flatland|
  void RemoveChild(
      fuchsia::ui::composition::TransformId parent_transform_id,
      fuchsia::ui::composition::TransformId child_transform_id) override;

  // |fuchsia::ui::composition::Flatland|
  void SetContent(fuchsia::ui::composition::TransformId transform_id,
                  fuchsia::ui::composition::ContentId content_id) override;

  // |fuchsia::ui::composition::Flatland|
  void SetRootTransform(
      fuchsia::ui::composition::TransformId transform_id) override;

  // |fuchsia::ui::composition::Flatland|
  void CreateViewport(
      fuchsia::ui::composition::ContentId viewport_id,
      fuchsia::ui::views::ViewportCreationToken token,
      fuchsia::ui::composition::ViewportProperties properties,
      fidl::InterfaceRequest<fuchsia::ui::composition::ChildViewWatcher>
          child_view_watcher) override;

  // |fuchsia::ui::composition::Flatland|
  void CreateImage(
      fuchsia::ui::composition::ContentId image_id,
      fuchsia::ui::composition::BufferCollectionImportToken import_token,
      uint32_t vmo_index,
      fuchsia::ui::composition::ImageProperties properties) override;

  // |fuchsia::ui::composition::Flatland|
  void SetImageSampleRegion(fuchsia::ui::composition::ContentId image_id,
                            fuchsia::math::RectF rect) override;

  // |fuchsia::ui::composition::Flatland|
  void SetImageDestinationSize(fuchsia::ui::composition::ContentId image_id,
                               fuchsia::math::SizeU size) override;

  // |fuchsia::ui::composition::Flatland|
  void SetImageBlendingFunction(
      fuchsia::ui::composition::ContentId image_id,
      fuchsia::ui::composition::BlendMode blend_mode) override;

  // |fuchsia::ui::composition::Flatland|
  void SetViewportProperties(
      fuchsia::ui::composition::ContentId viewport_id,
      fuchsia::ui::composition::ViewportProperties properties) override;

  // |fuchsia::ui::composition::Flatland|
  void ReleaseTransform(
      fuchsia::ui::composition::TransformId transform_id) override;

  // |fuchsia::ui::composition::Flatland|
  void ReleaseViewport(fuchsia::ui::composition::ContentId viewport_id,
                       ReleaseViewportCallback callback) override;

  // |fuchsia::ui::composition::Flatland|
  void ReleaseImage(fuchsia::ui::composition::ContentId image_id) override;

  // |fuchsia::ui::composition::Flatland|
  void SetHitRegions(
      fuchsia::ui::composition::TransformId transform_id,
      std::vector<fuchsia::ui::composition::HitRegion> regions) override;

  // |fuchsia::ui::composition::Flatland|
  void Clear() override;

  // |fuchsia::ui::composition::Flatland|
  void SetDebugName(std::string debug_name) override;

  fidl::Binding<fuchsia::ui::composition::Allocator> allocator_binding_;
  fidl::Binding<fuchsia::ui::composition::Flatland> flatland_binding_;
  GraphBindings graph_bindings_;
  PresentHandler present_handler_;

  FakeGraph pending_graph_;
  FakeGraph current_graph_;

  // This map is used to cache parent refs for `AddChild` and `RemoveChild`.
  //
  // Ideally we would like to map weak(child) -> weak(parent), but std::weak_ptr
  // cannot be the Key for an associative container. Instead we key on the raw
  // parent pointer and store pair(weak(parent), weak(child)) in the map.
  std::unordered_map<
      FakeGraph::TransformIdKey,
      std::pair<std::weak_ptr<FakeTransform>, std::weak_ptr<FakeTransform>>>
      parents_map_;

  std::string debug_name_;

  FML_DISALLOW_COPY_AND_ASSIGN(FakeFlatland);
};

}  // namespace flutter_runner::testing

#endif  // FLUTTER_SHELL_PLATFORM_FUCHSIA_FLUTTER_TESTS_FAKES_SCENIC_FAKE_FLATLAND_H_
