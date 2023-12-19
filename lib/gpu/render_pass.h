// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_LIB_GPU_RENDER_PASS_H_
#define FLUTTER_LIB_GPU_RENDER_PASS_H_

#include <map>
#include <memory>
#include "flutter/lib/gpu/command_buffer.h"
#include "flutter/lib/gpu/export.h"
#include "flutter/lib/ui/dart_wrapper.h"
#include "fml/memory/ref_ptr.h"
#include "impeller/core/formats.h"
#include "impeller/core/vertex_buffer.h"
#include "impeller/renderer/command.h"
#include "impeller/renderer/render_pass.h"
#include "impeller/renderer/render_target.h"
#include "lib/gpu/device_buffer.h"
#include "lib/gpu/host_buffer.h"
#include "lib/gpu/render_pipeline.h"
#include "lib/gpu/texture.h"

namespace flutter {
namespace gpu {

class RenderPass : public RefCountedDartWrappable<RenderPass> {
  DEFINE_WRAPPERTYPEINFO();
  FML_FRIEND_MAKE_REF_COUNTED(RenderPass);

 public:
  RenderPass();

  ~RenderPass() override;

  const std::weak_ptr<const impeller::Context>& GetContext() const;

  impeller::Command& GetCommand();
  const impeller::Command& GetCommand() const;

  impeller::RenderTarget& GetRenderTarget();
  const impeller::RenderTarget& GetRenderTarget() const;

  impeller::ColorAttachmentDescriptor& GetColorAttachmentDescriptor(
      size_t color_attachment_index);

  impeller::DepthAttachmentDescriptor& GetDepthAttachmentDescriptor();

  impeller::VertexBuffer& GetVertexBuffer();

  bool Begin(flutter::gpu::CommandBuffer& command_buffer);

  void SetPipeline(fml::RefPtr<RenderPipeline> pipeline);

  /// Lookup an Impeller pipeline by building a descriptor based on the current
  /// command state.
  std::shared_ptr<impeller::Pipeline<impeller::PipelineDescriptor>>
  GetOrCreatePipeline();

  impeller::Command ProvisionRasterCommand();

  bool Draw();

 private:
  impeller::RenderTarget render_target_;
  std::shared_ptr<impeller::RenderPass> render_pass_;

  // Command encoding state.
  impeller::Command command_;
  fml::RefPtr<RenderPipeline> render_pipeline_;
  impeller::PipelineDescriptor pipeline_descriptor_;

  // Pipeline descriptor layout state. We always keep track of this state, but
  // we'll only apply it as necessary to match the RenderTarget.
  std::map<size_t, impeller::ColorAttachmentDescriptor> color_descriptors_;
  impeller::StencilAttachmentDescriptor stencil_front_desc_;
  impeller::StencilAttachmentDescriptor stencil_back_desc_;
  impeller::DepthAttachmentDescriptor depth_desc_;

  // Command state.
  impeller::VertexBuffer vertex_buffer_;

  FML_DISALLOW_COPY_AND_ASSIGN(RenderPass);
};

}  // namespace gpu
}  // namespace flutter

//----------------------------------------------------------------------------
/// Exports
///

extern "C" {

FLUTTER_GPU_EXPORT
extern void InternalFlutterGpu_RenderPass_Initialize(Dart_Handle wrapper);

FLUTTER_GPU_EXPORT
extern Dart_Handle InternalFlutterGpu_RenderPass_SetColorAttachment(
    flutter::gpu::RenderPass* wrapper,
    int color_attachment_index,
    int load_action,
    int store_action,
    int clear_color,
    flutter::gpu::Texture* texture,
    Dart_Handle resolve_texture_wrapper);

FLUTTER_GPU_EXPORT
extern Dart_Handle InternalFlutterGpu_RenderPass_SetDepthStencilAttachment(
    flutter::gpu::RenderPass* wrapper,
    int depth_load_action,
    int depth_store_action,
    float depth_clear_value,
    int stencil_load_action,
    int stencil_store_action,
    int stencil_clear_value,
    flutter::gpu::Texture* texture);

FLUTTER_GPU_EXPORT
extern Dart_Handle InternalFlutterGpu_RenderPass_Begin(
    flutter::gpu::RenderPass* wrapper,
    flutter::gpu::CommandBuffer* command_buffer);

FLUTTER_GPU_EXPORT
extern void InternalFlutterGpu_RenderPass_BindPipeline(
    flutter::gpu::RenderPass* wrapper,
    flutter::gpu::RenderPipeline* pipeline);

FLUTTER_GPU_EXPORT
extern void InternalFlutterGpu_RenderPass_BindVertexBufferDevice(
    flutter::gpu::RenderPass* wrapper,
    flutter::gpu::DeviceBuffer* device_buffer,
    int offset_in_bytes,
    int length_in_bytes,
    int vertex_count);

FLUTTER_GPU_EXPORT
extern void InternalFlutterGpu_RenderPass_BindVertexBufferHost(
    flutter::gpu::RenderPass* wrapper,
    flutter::gpu::HostBuffer* host_buffer,
    int offset_in_bytes,
    int length_in_bytes,
    int vertex_count);

FLUTTER_GPU_EXPORT
extern void InternalFlutterGpu_RenderPass_BindIndexBufferDevice(
    flutter::gpu::RenderPass* wrapper,
    flutter::gpu::DeviceBuffer* device_buffer,
    int offset_in_bytes,
    int length_in_bytes,
    int index_type,
    int index_count);

FLUTTER_GPU_EXPORT
extern void InternalFlutterGpu_RenderPass_BindIndexBufferHost(
    flutter::gpu::RenderPass* wrapper,
    flutter::gpu::HostBuffer* host_buffer,
    int offset_in_bytes,
    int length_in_bytes,
    int index_type,
    int index_count);

FLUTTER_GPU_EXPORT
extern bool InternalFlutterGpu_RenderPass_BindUniformDevice(
    flutter::gpu::RenderPass* wrapper,
    flutter::gpu::Shader* shader,
    Dart_Handle uniform_name_handle,
    flutter::gpu::DeviceBuffer* device_buffer,
    int offset_in_bytes,
    int length_in_bytes);

FLUTTER_GPU_EXPORT
extern bool InternalFlutterGpu_RenderPass_BindUniformHost(
    flutter::gpu::RenderPass* wrapper,
    flutter::gpu::Shader* shader,
    Dart_Handle uniform_name_handle,
    flutter::gpu::HostBuffer* host_buffer,
    int offset_in_bytes,
    int length_in_bytes);

FLUTTER_GPU_EXPORT
extern bool InternalFlutterGpu_RenderPass_BindTexture(
    flutter::gpu::RenderPass* wrapper,
    flutter::gpu::Shader* shader,
    Dart_Handle uniform_name_handle,
    flutter::gpu::Texture* texture,
    int min_filter,
    int mag_filter,
    int mip_filter,
    int width_address_mode,
    int height_address_mode);

FLUTTER_GPU_EXPORT
extern void InternalFlutterGpu_RenderPass_ClearBindings(
    flutter::gpu::RenderPass* wrapper);

FLUTTER_GPU_EXPORT
extern void InternalFlutterGpu_RenderPass_SetColorBlendEnable(
    flutter::gpu::RenderPass* wrapper,
    int color_attachment_index,
    bool enable);

FLUTTER_GPU_EXPORT
extern void InternalFlutterGpu_RenderPass_SetColorBlendEquation(
    flutter::gpu::RenderPass* wrapper,
    int color_attachment_index,
    int color_blend_operation,
    int source_color_blend_factor,
    int destination_color_blend_factor,
    int alpha_blend_operation,
    int source_alpha_blend_factor,
    int destination_alpha_blend_factor);

FLUTTER_GPU_EXPORT
extern void InternalFlutterGpu_RenderPass_SetDepthWriteEnable(
    flutter::gpu::RenderPass* wrapper,
    bool enable);

FLUTTER_GPU_EXPORT
extern void InternalFlutterGpu_RenderPass_SetDepthCompareOperation(
    flutter::gpu::RenderPass* wrapper,
    int compare_operation);

FLUTTER_GPU_EXPORT
extern bool InternalFlutterGpu_RenderPass_Draw(
    flutter::gpu::RenderPass* wrapper);

}  // extern "C"

#endif  // FLUTTER_LIB_GPU_RENDER_PASS_H_
