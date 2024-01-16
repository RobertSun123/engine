// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "impeller/renderer/texture_mipmap.h"
#include "impeller/renderer/blit_pass.h"
#include "impeller/renderer/command_buffer.h"

namespace impeller {

fml::Status AddMipmapGeneration(const std::shared_ptr<Context>& context,
                                const std::shared_ptr<Texture>& texture) {
  std::shared_ptr<CommandBuffer> mip_cmd_buffer =
      context->CreateCommandBuffer();
  std::shared_ptr<BlitPass> blit_pass = mip_cmd_buffer->CreateBlitPass();
  bool success = blit_pass->GenerateMipmap(texture);
  if (!success) {
    return fml::Status(fml::StatusCode::kUnknown, "");
  }
  success = blit_pass->EncodeCommands(context->GetResourceAllocator());
  if (!success) {
    return fml::Status(fml::StatusCode::kUnknown, "");
  }
  success = mip_cmd_buffer->SubmitCommands(/*callback=*/nullptr);
  if (!success) {
    return fml::Status(fml::StatusCode::kUnknown, "");
  }
  return {};
}

}  // namespace impeller
