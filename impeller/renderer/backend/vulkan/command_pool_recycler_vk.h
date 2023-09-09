// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <memory>
#include <optional>
#include "fml/macros.h"
#include "impeller/base/thread.h"
#include "impeller/renderer/backend/vulkan/context_vk.h"
#include "impeller/renderer/backend/vulkan/vk.h"  // IWYU pragma: keep.

namespace impeller {

class CommandPoolRecyclerVK;

//------------------------------------------------------------------------------
/// @brief      Manages the lifecycle of a single |vk::CommandPool|.
///
/// A |vk::CommandPool| is expensive to create and reset. This class manages
/// the lifecycle of a single |vk::CommandPool| by returning to the origin
/// (|CommandPoolRecyclerVK|) when it is destroyed to be reused.
///
/// @warning    This class is not thread-safe.
///
/// @see        |CommandPoolRecyclerVK|
class CommandPoolResourceVK final {
 public:
  CommandPoolResourceVK(CommandPoolResourceVK&&) = default;
  ~CommandPoolResourceVK();

  /// @brief      Creates a resource that manages the life of a command pool.
  ///
  /// @param[in]  pool      The command pool to manage.
  /// @param[in]  recycler  The recycler that will be notified on destruction.
  explicit CommandPoolResourceVK(
      vk::UniqueCommandPool&& pool,
      const std::weak_ptr<CommandPoolRecyclerVK>& recycler)
      : pool_(std::move(pool)), recycler_(recycler) {}

  /// @brief      Returns a reference to the underlying |vk::CommandPool|.
  const vk::CommandPool& Get() const { return *pool_; }

 private:
  FML_DISALLOW_COPY_AND_ASSIGN(CommandPoolResourceVK);

  vk::UniqueCommandPool pool_;
  const std::weak_ptr<CommandPoolRecyclerVK>& recycler_;
};

//------------------------------------------------------------------------------
/// @brief      Creates and manages the lifecycle of |vk::CommandPool| objects.
///
/// A |vk::CommandPool| is expensive to create and reset. This class manages
/// the lifecycle of |vk::CommandPool| objects by creating and recycling them;
/// or in other words, a pool for command pools.
///
/// A single instance should be created per |ContextVK|.
///
/// Every "frame", a single |CommandPoolResourceVk| is made available for each
/// thread that calls |Get|. After calling |Recycle|, the current thread's pool
/// is moved to a background thread, reset, and made available for the next time
/// |Get| is called and needs to create a command pool.
///
/// @note       This class is thread-safe.
///
/// @see        |vk::CommandPoolResourceVk|
/// @see        |ContextVK|
/// @see
/// https://arm-software.github.io/vulkan_best_practice_for_mobile_developers/samples/performance/command_buffer_usage/command_buffer_usage_tutorial.html
class CommandPoolRecyclerVK final
    : public std::enable_shared_from_this<CommandPoolRecyclerVK> {
 public:
  ~CommandPoolRecyclerVK();

  /// @brief      Creates a recycler for the given |ContextVK|.
  ///
  /// @param[in]  context The context to create the recycler for.
  explicit CommandPoolRecyclerVK(const ContextVK& context);

  /// @brief      Gets a command pool for the current thread.
  ///
  /// @warning    Returns a |nullptr| if a pool could not be created.
  std::shared_ptr<CommandPoolResourceVK> Get();

  /// @brief      Returns a command pool to be reset on a background thread.
  ///
  /// @param[in]  pool The pool to recycle.
  void Reclaim(vk::UniqueCommandPool&& pool);

  /// @brief      Clears all recycled command pools to let them be reclaimed.
  void Recycle();

 private:
  FML_DISALLOW_COPY_AND_ASSIGN(CommandPoolRecyclerVK);

  /// @brief      Creates a new |vk::CommandPool|.
  ///
  /// @returns    Returns a |std::nullopt| if a pool could not be created.
  std::optional<vk::UniqueCommandPool> Create();

  /// @brief      Reuses a recycled |vk::CommandPool|, if available.
  ///
  /// @returns    Returns a |std::nullopt| if a pool was not available.
  std::optional<vk::UniqueCommandPool> Reuse();

  const std::weak_ptr<ContextVK> context_;

  Mutex recycled_mutex_;
  std::vector<vk::UniqueCommandPool> recycled_ IPLR_GUARDED_BY(recycled_mutex_);
};

}  // namespace impeller
