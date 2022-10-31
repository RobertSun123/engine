// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/android/android_surface_vulkan_impeller.h"

#include <memory>
#include <utility>

#include "flutter/fml/concurrent_message_loop.h"
#include "flutter/fml/logging.h"
#include "flutter/fml/memory/ref_ptr.h"
#include "flutter/impeller/renderer/backend/vulkan/context_vk.h"
#include "flutter/shell/gpu/gpu_surface_vulkan_impeller.h"
#include "flutter/vulkan/vulkan_native_surface_android.h"
#include "impeller/entity/vk/entity_shaders_vk.h"

namespace flutter {

std::shared_ptr<impeller::Context> CreateImpellerContext(
    const fml::RefPtr<vulkan::VulkanProcTable>& proc_table,
    const std::shared_ptr<fml::ConcurrentMessageLoop>& concurrent_loop) {
  if (!proc_table->IsValid()) {
    FML_LOG(ERROR) << "Invalid Vulkan Proc Table.";
    return nullptr;
  }

  std::vector<std::shared_ptr<fml::Mapping>> shader_mappings = {
      std::make_shared<fml::NonOwnedMapping>(impeller_entity_shaders_vk_data,
                                             impeller_entity_shaders_vk_length),
  };

  PFN_vkGetInstanceProcAddr instance_proc_addr =
      proc_table->NativeGetInstanceProcAddr();

  auto context =
      impeller::ContextVK::Create(instance_proc_addr,                //
                                  shader_mappings,                   //
                                  nullptr,                           //
                                  concurrent_loop->GetTaskRunner(),  //
                                  "Android Impeller Vulkan Lib"      //
      );

  return context;
}

AndroidSurfaceVulkanImpeller::AndroidSurfaceVulkanImpeller(
    const std::shared_ptr<AndroidContext>& android_context,
    const std::shared_ptr<PlatformViewAndroidJNI>& jni_facade)
    : AndroidSurface(android_context),
      proc_table_(fml::MakeRefCounted<vulkan::VulkanProcTable>()),
      workers_(fml::ConcurrentMessageLoop::Create()) {
  impeller_context_ = CreateImpellerContext(proc_table_, workers_);
  is_valid_ =
      proc_table_->HasAcquiredMandatoryProcAddresses() && impeller_context_;
}

AndroidSurfaceVulkanImpeller::~AndroidSurfaceVulkanImpeller() = default;

bool AndroidSurfaceVulkanImpeller::IsValid() const {
  return is_valid_;
}

void AndroidSurfaceVulkanImpeller::TeardownOnScreenContext() {
  // Nothing to do.
}

std::unique_ptr<Surface> AndroidSurfaceVulkanImpeller::CreateGPUSurface(
    GrDirectContext* gr_context) {
  if (!IsValid()) {
    return nullptr;
  }

  if (!native_window_ || !native_window_->IsValid()) {
    return nullptr;
  }

  auto vulkan_surface_android =
      std::make_unique<vulkan::VulkanNativeSurfaceAndroid>(
          native_window_->handle());

  if (!vulkan_surface_android->IsValid()) {
    return nullptr;
  }

  std::unique_ptr<GPUSurfaceVulkanImpeller> gpu_surface =
      std::make_unique<GPUSurfaceVulkanImpeller>(impeller_context_);

  if (!gpu_surface->IsValid()) {
    return nullptr;
  }

  return gpu_surface;
}

bool AndroidSurfaceVulkanImpeller::OnScreenSurfaceResize(const SkISize& size) {
  return true;
}

bool AndroidSurfaceVulkanImpeller::ResourceContextMakeCurrent() {
  FML_DLOG(ERROR) << "The vulkan backend does not support resource contexts.";
  return false;
}

bool AndroidSurfaceVulkanImpeller::ResourceContextClearCurrent() {
  FML_DLOG(ERROR) << "The vulkan backend does not support resource contexts.";
  return false;
}

bool AndroidSurfaceVulkanImpeller::SetNativeWindow(
    fml::RefPtr<AndroidNativeWindow> window) {
  native_window_ = std::move(window);

  // TODO (kaushikiska): setup the swapchain here!

  auto& context_vk = impeller::ContextVK::Cast(*impeller_context_);
  context_vk.SetupSwapchain(vk::UniqueSurfaceKHR surface);

  return native_window_ && native_window_->IsValid();
}

}  // namespace flutter
