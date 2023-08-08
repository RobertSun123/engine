// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <string>

#include "flutter/fml/macros.h"
#include "flutter/fml/mapping.h"
#include "impeller/core/device_buffer_descriptor.h"
#include "impeller/core/texture_descriptor.h"
#include "impeller/geometry/size.h"

namespace impeller {

class Context;
class DeviceBuffer;
class Texture;

//------------------------------------------------------------------------------
/// @brief      An object that allocates device memory.
///
class Allocator {
 public:
  virtual ~Allocator();

  bool IsValid() const;

  std::shared_ptr<DeviceBuffer> CreateBuffer(
      const DeviceBufferDescriptor& desc);

  std::shared_ptr<Texture> CreateTexture(const TextureDescriptor& desc);

  //------------------------------------------------------------------------------
  /// @brief      Minimum value for `row_bytes` on a Texture. The row
  ///             bytes parameter of that method must be aligned to this value.
  ///
  virtual uint16_t MinimumBytesPerRow(PixelFormat format) const;

  std::shared_ptr<DeviceBuffer> CreateBufferWithCopy(const uint8_t* buffer,
                                                     size_t length);

  std::shared_ptr<DeviceBuffer> CreateBufferWithCopy(
      const fml::Mapping& mapping);

  virtual ISize GetMaxTextureSizeSupported() const = 0;

  /// @brief Increment an internal frame used to cycle through a ring buffer of
  /// allocation pools.
  virtual void DidAcquireSurfaceFrame() {
    for (auto& td : data_to_recycle_) {
      td.used_this_frame = false;
    }
  }

  virtual void DidFinishSurfaceFrame() {
    std::vector<TextureData> retain;

    for (auto td : data_to_recycle_) {
      if (td.used_this_frame) {
        retain.push_back(td);
      }
    }
    data_to_recycle_.clear();
    data_to_recycle_.insert(data_to_recycle_.end(), retain.begin(),
                            retain.end());
    FML_LOG(ERROR) << "retained: " << data_to_recycle_.size();
  }

 protected:
  Allocator();

  virtual std::shared_ptr<DeviceBuffer> OnCreateBuffer(
      const DeviceBufferDescriptor& desc) = 0;

  virtual std::shared_ptr<Texture> OnCreateTexture(
      const TextureDescriptor& desc) = 0;

 private:
  struct TextureData {
    bool used_this_frame;
    TextureDescriptor descriptor;
    std::shared_ptr<Texture> texture;
  };

  std::vector<TextureData> data_to_recycle_;

  FML_DISALLOW_COPY_AND_ASSIGN(Allocator);
};

}  // namespace impeller
