// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "impeller/renderer/backend/vulkan/compute_pass_vk.h"

#include "flutter/fml/trace_event.h"
#include "impeller/renderer/backend/vulkan/compute_pipeline_vk.h"
#include "impeller/renderer/backend/vulkan/sampler_vk.h"
#include "impeller/renderer/backend/vulkan/texture_vk.h"

namespace impeller {

ComputePassVK::ComputePassVK(std::weak_ptr<const Context> context,
                             std::weak_ptr<CommandEncoderVK> encoder)
    : ComputePass(std::move(context)), encoder_(std::move(encoder)) {
  is_valid_ = true;
}

ComputePassVK::~ComputePassVK() = default;

bool ComputePassVK::IsValid() const {
  return is_valid_;
}

void ComputePassVK::OnSetLabel(const std::string& label) {
  if (label.empty()) {
    return;
  }
  label_ = label;
}

static bool UpdateBindingLayouts(const Bindings& bindings,
                                 const vk::CommandBuffer& buffer) {
  LayoutTransition transition;
  transition.cmd_buffer = buffer;
  transition.src_access = vk::AccessFlagBits::eColorAttachmentWrite |
                          vk::AccessFlagBits::eTransferWrite;
  transition.src_stage = vk::PipelineStageFlagBits::eColorAttachmentOutput |
                         vk::PipelineStageFlagBits::eTransfer;
  transition.dst_access = vk::AccessFlagBits::eShaderRead;
  transition.dst_stage = vk::PipelineStageFlagBits::eFragmentShader;

  transition.new_layout = vk::ImageLayout::eShaderReadOnlyOptimal;

  for (const auto& [_, texture] : bindings.textures) {
    if (!TextureVK::Cast(*texture.resource).SetLayout(transition)) {
      return false;
    }
  }
  return true;
}

static bool UpdateBindingLayouts(const ComputeCommand& command,
                                 const vk::CommandBuffer& buffer) {
  return UpdateBindingLayouts(command.bindings, buffer);
}

static bool UpdateBindingLayouts(const std::vector<ComputeCommand>& commands,
                                 const vk::CommandBuffer& buffer) {
  for (const auto& command : commands) {
    if (!UpdateBindingLayouts(command, buffer)) {
      return false;
    }
  }
  return true;
}

static bool AllocateAndBindDescriptorSets(const ContextVK& context,
                                          const ComputeCommand& command,
                                          CommandEncoderVK& encoder,
                                          const ComputePipelineVK& pipeline) {
  auto desc_set =
      encoder.AllocateDescriptorSet(pipeline.GetDescriptorSetLayout());
  if (!desc_set) {
    return false;
  }

  auto& allocator = *context.GetResourceAllocator();

  std::unordered_map<uint32_t, vk::DescriptorBufferInfo> buffers;
  std::unordered_map<uint32_t, vk::DescriptorImageInfo> images;
  std::vector<vk::WriteDescriptorSet> writes;

  auto bind_images = [&encoder,  //
                      &images,   //
                      &writes,   //
                      &desc_set  //
  ](const Bindings& bindings) -> bool {
    for (const auto& [index, sampler_handle] : bindings.samplers) {
      if (bindings.textures.find(index) == bindings.textures.end()) {
        return false;
      }

      auto texture = bindings.textures.at(index).resource;
      const auto& texture_vk = TextureVK::Cast(*texture);
      const SamplerVK& sampler = SamplerVK::Cast(*sampler_handle.resource);

      if (!encoder.Track(texture) ||
          !encoder.Track(sampler.GetSharedSampler())) {
        return false;
      }

      const SampledImageSlot& slot = bindings.sampled_images.at(index);

      vk::DescriptorImageInfo image_info;
      image_info.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
      image_info.sampler = sampler.GetSampler();
      image_info.imageView = texture_vk.GetImageView();

      vk::WriteDescriptorSet write_set;
      write_set.dstSet = desc_set.value();
      write_set.dstBinding = slot.binding;
      write_set.descriptorCount = 1u;
      write_set.descriptorType = vk::DescriptorType::eCombinedImageSampler;
      write_set.pImageInfo = &(images[slot.binding] = image_info);

      writes.push_back(write_set);
    }

    return true;
  };

  auto bind_buffers = [&allocator,  //
                       &encoder,    //
                       &buffers,    //
                       &writes,     //
                       &desc_set    //
  ](const Bindings& bindings) -> bool {
    for (const auto& [buffer_index, view] : bindings.buffers) {
      const auto& buffer_view = view.resource.buffer;

      auto device_buffer = buffer_view->GetDeviceBuffer(allocator);
      if (!device_buffer) {
        VALIDATION_LOG << "Failed to get device buffer for vertex binding";
        return false;
      }

      auto buffer = DeviceBufferVK::Cast(*device_buffer).GetBuffer();
      if (!buffer) {
        return false;
      }

      if (!encoder.Track(device_buffer)) {
        return false;
      }

      uint32_t offset = view.resource.range.offset;

      vk::DescriptorBufferInfo buffer_info;
      buffer_info.buffer = buffer;
      buffer_info.offset = offset;
      buffer_info.range = view.resource.range.length;

      const ShaderUniformSlot& uniform = bindings.uniforms.at(buffer_index);

      vk::WriteDescriptorSet write_set;
      write_set.dstSet = desc_set.value();
      write_set.dstBinding = uniform.binding;
      write_set.descriptorCount = 1u;
      write_set.descriptorType = vk::DescriptorType::eUniformBuffer;
      write_set.pBufferInfo = &(buffers[uniform.binding] = buffer_info);

      writes.push_back(write_set);
    }
    return true;
  };

  if (!bind_buffers(command.bindings) || !bind_images(command.bindings)) {
    return false;
  }

  context.GetDevice().updateDescriptorSets(writes, {});

  encoder.GetCommandBuffer().bindDescriptorSets(
      vk::PipelineBindPoint::eCompute,  // bind point
      pipeline.GetPipelineLayout(),     // layout
      0,                                // first set
      {vk::DescriptorSet{*desc_set}},   // sets
      nullptr                           // offsets
  );
  return true;
}

bool ComputePassVK::OnEncodeCommands(const Context& context,
                                     const ISize& grid_size,
                                     const ISize& thread_group_size) const {
  TRACE_EVENT0("impeller", "ComputePassVK::EncodeCommands");
  if (!IsValid()) {
    return false;
  }

  FML_DCHECK(!grid_size.IsEmpty() && !thread_group_size.IsEmpty());

  const auto& vk_context = ContextVK::Cast(context);
  auto encoder = encoder_.lock();
  if (!encoder) {
    VALIDATION_LOG << "Command encoder died before commands could be encoded.";
    return false;
  }

  fml::ScopedCleanupClosure pop_marker(
      [&encoder]() { encoder->PopDebugGroup(); });
  if (!label_.empty()) {
    encoder->PushDebugGroup(label_.c_str());
  } else {
    pop_marker.Release();
  }

  auto cmd_buffer = encoder->GetCommandBuffer();

  if (!UpdateBindingLayouts(commands_, cmd_buffer)) {
    VALIDATION_LOG << "Could not update binding layouts for compute pass.";
    return false;
  }

  {
    TRACE_EVENT0("impeller", "EncodeComputePassCommands");

    vk::CommandBufferBeginInfo pass_info;
    pass_info.sType = vk::StructureType::eCommandBufferBeginInfo;

    auto begin_result = cmd_buffer.begin(pass_info);
    if (begin_result != vk::Result::eSuccess) {
      VALIDATION_LOG << "Failed to begin compute pass: "
                     << vk::to_string(begin_result) << ".";
      return false;
    }

    fml::ScopedCleanupClosure end_compute_pass([cmd_buffer]() {
      auto end_result = cmd_buffer.end();
      if (end_result != vk::Result::eSuccess) {
        VALIDATION_LOG << "Failed to end compute pass: "
                       << vk::to_string(end_result) << ".";
      }
    });

    for (const auto& command : commands_) {
      if (!command.pipeline) {
        continue;
      }

      const auto& pipeline_vk = ComputePipelineVK::Cast(*command.pipeline);

      cmd_buffer.bindPipeline(vk::PipelineBindPoint::eCompute,
                              pipeline_vk.GetPipeline());
      if (!AllocateAndBindDescriptorSets(vk_context,  //
                                         command,     //
                                         *encoder,    //
                                         pipeline_vk  //
                                         )) {
        return false;
      }

      cmd_buffer.dispatch(grid_size.width, grid_size.height, 1);
      vk::Result result = cmd_buffer.end();
      if (result != vk::Result::eSuccess) {
        return false;
      }
    }
  }

  return true;
}

}  // namespace impeller
