// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <map>
#include <memory>
#include <optional>
#include <string>

#include "flutter/fml/logging.h"
#include "flutter/fml/macros.h"
#include "impeller/core/buffer_view.h"
#include "impeller/core/formats.h"
#include "impeller/core/resource_binder.h"
#include "impeller/core/sampler.h"
#include "impeller/core/shader_types.h"
#include "impeller/core/texture.h"
#include "impeller/core/vertex_buffer.h"
#include "impeller/geometry/rect.h"
#include "impeller/renderer/pipeline.h"
#include "impeller/renderer/vertex_buffer_builder.h"
#include "impeller/tessellator/tessellator.h"

namespace impeller {

template <class T>
struct Resource {
  using ResourceType = T;
  ResourceType resource;

  Resource() {}

  Resource(const ShaderMetadata* metadata, ResourceType p_resource)
      : resource(p_resource), metadata_(metadata) {}

  Resource(std::shared_ptr<const ShaderMetadata>& metadata,
           ResourceType p_resource)
      : resource(p_resource), dynamic_metadata_(metadata) {}

  const ShaderMetadata* GetMetadata() const {
    return dynamic_metadata_ ? dynamic_metadata_.get() : metadata_;
  }

 private:
  // Static shader metadata (typically generated by ImpellerC).
  const ShaderMetadata* metadata_ = nullptr;

  // Dynamically generated shader metadata.
  std::shared_ptr<const ShaderMetadata> dynamic_metadata_;
};

using BufferResource = Resource<BufferView>;
using TextureResource = Resource<std::shared_ptr<const Texture>>;
using SamplerResource = Resource<std::shared_ptr<const Sampler>>;

struct Bindings {
  std::map<size_t, ShaderUniformSlot> uniforms;
  std::map<size_t, SampledImageSlot> sampled_images;
  std::map<size_t, BufferResource> buffers;
  std::map<size_t, TextureResource> textures;
  std::map<size_t, SamplerResource> samplers;
};

//------------------------------------------------------------------------------
/// @brief      An object used to specify work to the GPU along with references
///             to resources the GPU will used when doing said work.
///
///             To construct a valid command, follow these steps:
///             * Specify a valid pipeline.
///             * Specify vertex information via a call `BindVertices`
///             * Specify any stage bindings.
///             * (Optional) Specify a debug label.
///
///             Command are very lightweight objects and can be created
///             frequently and on demand. The resources referenced in commands
///             views into buffers managed by other allocators and resource
///             managers.
///
struct Command : public ResourceBinder {
  //----------------------------------------------------------------------------
  /// The pipeline to use for this command.
  ///
  std::shared_ptr<Pipeline<PipelineDescriptor>> pipeline;
  //----------------------------------------------------------------------------
  /// The buffer, texture, and sampler bindings used by the vertex pipeline
  /// stage.
  ///
  Bindings vertex_bindings;
  //----------------------------------------------------------------------------
  /// The buffer, texture, and sampler bindings used by the fragment pipeline
  /// stage.
  ///
  Bindings fragment_bindings;
  //----------------------------------------------------------------------------
  /// The index buffer binding used by the vertex shader stage. Instead of
  /// setting this directly, it usually easier to specify the vertex and index
  /// buffer bindings directly via a single call to `BindVertices`.
  ///
  /// @see         `BindVertices`
  ///
  BufferView index_buffer;
  //----------------------------------------------------------------------------
  /// The number of vertices to draw.
  ///
  /// If the index_type is `IndexType::kNone`, this is a count into the vertex
  /// buffer. Otherwise, it is a count into the index buffer. Set the vertex and
  /// index buffers as well as the index count using a call to `BindVertices`.
  ///
  /// @see         `BindVertices`
  ///
  size_t vertex_count = 0u;
  //----------------------------------------------------------------------------
  /// The type of indices in the index buffer. The indices must be tightly
  /// packed in the index buffer.
  ///
  IndexType index_type = IndexType::kUnknown;
  //----------------------------------------------------------------------------
  /// The debugging label to use for the command.
  ///
  std::string label;
  //----------------------------------------------------------------------------
  /// The reference value to use in stenciling operations. Stencil configuration
  /// is part of pipeline setup and can be read from the pipelines descriptor.
  ///
  /// @see         `Pipeline`
  /// @see         `PipelineDescriptor`
  ///
  uint32_t stencil_reference = 0u;
  //----------------------------------------------------------------------------
  /// The offset used when indexing into the vertex buffer.
  ///
  uint64_t base_vertex = 0u;
  //----------------------------------------------------------------------------
  /// The viewport coordinates that the rasterizer linearly maps normalized
  /// device coordinates to.
  /// If unset, the viewport is the size of the render target with a zero
  /// origin, znear=0, and zfar=1.
  ///
  std::optional<Viewport> viewport;
  //----------------------------------------------------------------------------
  /// The scissor rect to use for clipping writes to the render target. The
  /// scissor rect must lie entirely within the render target.
  /// If unset, no scissor is applied.
  ///
  std::optional<IRect> scissor;
  //----------------------------------------------------------------------------
  /// The number of instances of the given set of vertices to render. Not all
  /// backends support rendering more than one instance at a time.
  ///
  /// @warning      Setting this to more than one will limit the availability of
  ///               backends to use with this command.
  ///
  size_t instance_count = 1u;

  size_t last_vertex_index = VertexDescriptor::kReservedVertexBufferIndex;

  //----------------------------------------------------------------------------
  /// @brief      Specify the vertex and index buffer to use for this command.
  ///
  /// @param[in]  buffer  The vertex and index buffer definition.
  ///
  /// @return     returns if the binding was updated.
  ///
  bool BindVertices(const VertexBuffer& buffer);

  void BindIndexBuffer(IndexType index_type, BufferView index_buffer);

  template <size_t N>
  void BindVertexBuffers(
      std::array<BufferView, N> vertex_buffers) {
    for (auto i = 0u; i < N; i++) {
      vertex_bindings
          .buffers[VertexDescriptor::kReservedVertexBufferIndex - i] = {
          nullptr, vertex_buffers[i]};
    }
    last_vertex_index = VertexDescriptor::kReservedVertexBufferIndex - N + 1;
  };

  // |ResourceBinder|
  bool BindResource(ShaderStage stage,
                    const ShaderUniformSlot& slot,
                    const ShaderMetadata& metadata,
                    const BufferView& view) override;

  bool BindResource(ShaderStage stage,
                    const ShaderUniformSlot& slot,
                    const std::shared_ptr<const ShaderMetadata>& metadata,
                    const BufferView& view);

  // |ResourceBinder|
  bool BindResource(ShaderStage stage,
                    const SampledImageSlot& slot,
                    const ShaderMetadata& metadata,
                    const std::shared_ptr<const Texture>& texture) override;

  // |ResourceBinder|
  bool BindResource(ShaderStage stage,
                    const SampledImageSlot& slot,
                    const ShaderMetadata& metadata,
                    const std::shared_ptr<const Sampler>& sampler) override;

  // |ResourceBinder|
  bool BindResource(ShaderStage stage,
                    const SampledImageSlot& slot,
                    const ShaderMetadata& metadata,
                    const std::shared_ptr<const Texture>& texture,
                    const std::shared_ptr<const Sampler>& sampler) override;

  BufferView GetVertexBuffer() const;

  constexpr operator bool() const { return pipeline && pipeline->IsValid(); }

 private:
  template <class T>
  bool DoBindResource(ShaderStage stage,
                      const ShaderUniformSlot& slot,
                      T metadata,
                      const BufferView& view);
};

}  // namespace impeller
