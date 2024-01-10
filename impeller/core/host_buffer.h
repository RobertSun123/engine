// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_IMPELLER_CORE_HOST_BUFFER_H_
#define FLUTTER_IMPELLER_CORE_HOST_BUFFER_H_

#include <algorithm>
#include <array>
#include <functional>
#include <memory>
#include <string>
#include <type_traits>

#include "impeller/core/buffer.h"
#include "impeller/core/buffer_view.h"
#include "impeller/core/platform.h"

namespace impeller {

/// Approximately the same size as the max frames in flight.
static const constexpr size_t kHostBufferArenaSize = 3u;

/// The host buffer class manages one more 1024 Kb blocks of device buffer
/// allocations.
///
/// These are reset per-frame.
class HostBuffer {
 public:
  static std::shared_ptr<HostBuffer> Create(
      const std::shared_ptr<Allocator>& allocator);

  // |Buffer|
  virtual ~HostBuffer();

  void SetLabel(std::string label);

  //----------------------------------------------------------------------------
  /// @brief      Emplace uniform data onto the host buffer. Ensure that backend
  ///             specific uniform alignment requirements are respected.
  ///
  /// @param[in]  uniform     The uniform struct to emplace onto the buffer.
  ///
  /// @tparam     UniformType The type of the uniform struct.
  ///
  /// @return     The buffer view.
  ///
  template <class UniformType,
            class = std::enable_if_t<std::is_standard_layout_v<UniformType>>>
  [[nodiscard]] BufferView EmplaceUniform(const UniformType& uniform) {
    const auto alignment =
        std::max(alignof(UniformType), DefaultUniformAlignment());
    return Emplace(reinterpret_cast<const void*>(&uniform),  // buffer
                   sizeof(UniformType),                      // size
                   alignment                                 // alignment
    );
  }

  //----------------------------------------------------------------------------
  /// @brief      Emplace storage buffer data onto the host buffer. Ensure that
  ///             backend specific uniform alignment requirements are respected.
  ///
  /// @param[in]  uniform     The storage buffer to emplace onto the buffer.
  ///
  /// @tparam     StorageBufferType The type of the shader storage buffer.
  ///
  /// @return     The buffer view.
  ///
  template <
      class StorageBufferType,
      class = std::enable_if_t<std::is_standard_layout_v<StorageBufferType>>>
  [[nodiscard]] BufferView EmplaceStorageBuffer(
      const StorageBufferType& buffer) {
    const auto alignment =
        std::max(alignof(StorageBufferType), DefaultUniformAlignment());
    return Emplace(&buffer,                    // buffer
                   sizeof(StorageBufferType),  // size
                   alignment                   // alignment
    );
  }

  //----------------------------------------------------------------------------
  /// @brief      Emplace non-uniform data (like contiguous vertices) onto the
  ///             host buffer.
  ///
  /// @param[in]  buffer        The buffer data.
  ///
  /// @tparam     BufferType    The type of the buffer data.
  ///
  /// @return     The buffer view.
  ///
  template <class BufferType,
            class = std::enable_if_t<std::is_standard_layout_v<BufferType>>>
  [[nodiscard]] BufferView Emplace(const BufferType& buffer) {
    return Emplace(reinterpret_cast<const void*>(&buffer),  // buffer
                   sizeof(BufferType),                      // size
                   alignof(BufferType)                      // alignment
    );
  }

  [[nodiscard]] BufferView Emplace(const void* buffer,
                                   size_t length,
                                   size_t align);

  using EmplaceProc = std::function<void(uint8_t* buffer)>;

  //----------------------------------------------------------------------------
  /// @brief      Emplaces undefined data onto the managed buffer and gives the
  ///             caller a chance to update it using the specified callback. The
  ///             buffer is guaranteed to have enough space for length bytes. It
  ///             is the responsibility of the caller to not exceed the bounds
  ///             of the buffer returned in the EmplaceProc.
  ///
  /// @param[in]  cb            A callback that will be passed a ptr to the
  ///                           underlying host buffer.
  ///
  /// @return     The buffer view.
  ///
  BufferView Emplace(size_t length, size_t align, const EmplaceProc& cb);

  //----------------------------------------------------------------------------
  /// @brief Resets the contents of the HostBuffer to nothing so it can be
  ///        reused.
  void Reset();

  /// Test only internal state.
  struct TestStateQuery {
    size_t current_frame;
    size_t current_buffer;
    size_t total_buffer_count;
  };

  /// @brief Retrieve internal buffer state for test expectations.
  TestStateQuery GetStateForTest();

 private:
  struct HostBufferState {
    [[nodiscard]] std::tuple<uint8_t*, Range, std::shared_ptr<DeviceBuffer>>
    Emplace(const void* buffer, size_t length);

    std::tuple<uint8_t*, Range, std::shared_ptr<DeviceBuffer>>
    Emplace(size_t length, size_t align, const EmplaceProc& cb);

    std::tuple<uint8_t*, Range, std::shared_ptr<DeviceBuffer>>
    Emplace(const void* buffer, size_t length, size_t align);

    void Reset();

    size_t GetLength() const { return offset; }

    void MaybeCreateNewBuffer(size_t required_size);

    std::shared_ptr<DeviceBuffer> GetCurrentBuffer() {
      return device_buffers[frame_index][current_buffer];
    }

    std::shared_ptr<Allocator> allocator;
    std::array<std::vector<std::shared_ptr<DeviceBuffer>>, kHostBufferArenaSize>
        device_buffers;
    size_t current_buffer = 0u;
    size_t offset = 0u;
    size_t frame_index = 0u;
    std::string label;
  };

  std::shared_ptr<HostBufferState> state_ = std::make_shared<HostBufferState>();

  [[nodiscard]] BufferView Emplace(const void* buffer, size_t length);

  explicit HostBuffer(const std::shared_ptr<Allocator>& allocator);

  HostBuffer(const HostBuffer&) = delete;

  HostBuffer& operator=(const HostBuffer&) = delete;
};

}  // namespace impeller

#endif  // FLUTTER_IMPELLER_CORE_HOST_BUFFER_H_
