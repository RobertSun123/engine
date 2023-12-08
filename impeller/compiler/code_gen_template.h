// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string_view>

namespace impeller {
namespace compiler {

constexpr std::string_view kReflectionHeaderTemplate =
    R"~~(// THIS FILE IS GENERATED BY impellerc.
// DO NOT EDIT OR CHECK THIS INTO SOURCE CONTROL

#pragma once

{# Note: The nogncheck decorations are only to make GN not mad at the template#}
{# this file is generated from. There are no GN rule violations in the generated#}
{# file itself and the no-check declarations will be stripped in generated files.#}
#include "impeller/core/buffer_view.h"                {# // nogncheck #}

#include "impeller/core/sampler.h"                    {# // nogncheck #}

#include "impeller/core/shader_types.h"               {# // nogncheck #}

#include "impeller/core/texture.h"                    {# // nogncheck #}


namespace impeller {

struct {{camel_case(shader_name)}}{{camel_case(shader_stage)}}Shader {
  // ===========================================================================
  // Stage Info ================================================================
  // ===========================================================================
  static constexpr std::string_view kLabel = "{{camel_case(shader_name)}}";
  static constexpr std::string_view kEntrypointName = "{{entrypoint}}";
  static constexpr ShaderStage kShaderStage = {{to_shader_stage(shader_stage)}};
  // The generator used to prepare these bindings. Metal generators may be used
  // by GLES backends but GLES generators are unsuitable for the metal backend.
  static constexpr std::string_view kGeneratorName = "{{get_generator_name()}}";
{% if length(struct_definitions) > 0 %}
  // ===========================================================================
  // Struct Definitions ========================================================
  // ===========================================================================
{% for def in struct_definitions %}

{% if last(def.members).array_elements == 0 %}
  template <size_t FlexCount>
{% endif %}
  struct {{def.name}} {
{% for member in def.members %}
{{"    "}}{% if member.element_padding > 0 %}Padded<{{member.type}}, {{member.element_padding}}>{% else %}{{member.type}}{% endif %}{{" " + member.name}}{% if member.array_elements != "std::nullopt" %}[{% if member.array_elements == 0 %}FlexCount{% else %}{{member.array_elements}}{% endif %}]{% endif %}; // (offset {{member.offset}}, size {{member.byte_length}})
{% endfor %}
  }; // struct {{def.name}} (size {{def.byte_length}})
{% endfor %}
{% endif %}
{% if length(buffers) > 0 %}

  // ===========================================================================
  // Stage Uniform & Storage Buffers ===========================================
  // ===========================================================================
{% for buffer in buffers %}

  static constexpr auto kResource{{camel_case(buffer.name)}} = ShaderUniformSlot { // {{buffer.name}}
    "{{buffer.name}}",     // name
    {{buffer.ext_res_0}}u, // ext_res_0
    {{buffer.set}}u,       // set
    {{buffer.binding}}u,   // binding
  };
  static ShaderMetadata kMetadata{{camel_case(buffer.name)}};
{% endfor %}
{% endif %}

  // ===========================================================================
  // Stage Inputs ==============================================================
  // ===========================================================================
{% if length(stage_inputs) > 0 %}
{% for stage_input in stage_inputs %}

  static constexpr auto kInput{{camel_case(stage_input.name)}} = ShaderStageIOSlot { // {{stage_input.name}}
    "{{stage_input.name}}",             // name
    {{stage_input.location}}u,          // attribute location
    {{stage_input.descriptor_set}}u,    // attribute set
    {{stage_input.binding}}u,           // attribute binding
    {{stage_input.type.type_name}},     // type
    {{stage_input.type.bit_width}}u,    // bit width of type
    {{stage_input.type.vec_size}}u,     // vec size
    {{stage_input.type.columns}}u,      // number of columns
    {{stage_input.offset}}u,            // offset for interleaved layout
  };
{% endfor %}
{% endif %}

  static constexpr std::array<const ShaderStageIOSlot*, {{length(stage_inputs)}}> kAllShaderStageInputs = {
{% for stage_input in stage_inputs %}
    &kInput{{camel_case(stage_input.name)}}, // {{stage_input.name}}
{% endfor %}
  };

{% if shader_stage == "vertex" %}
  static constexpr auto kInterleavedLayout = ShaderStageBufferLayout {
{% if length(stage_inputs) > 0 %}
    sizeof(PerVertexData),                 // stride for interleaved layout
{% else %}
    0u,
{% endif %}
    0u,                                    // attribute binding
  };
  static constexpr std::array<const ShaderStageBufferLayout*, 1> kInterleavedBufferLayout = {
    &kInterleavedLayout
  };
{% endif %}

{% if length(sampled_images) > 0 %}
  // ===========================================================================
  // Sampled Images ============================================================
  // ===========================================================================
{% for sampled_image in sampled_images %}

  static constexpr auto kResource{{camel_case(sampled_image.name)}} = SampledImageSlot { // {{sampled_image.name}}
    "{{sampled_image.name}}",      // name
    {{sampled_image.ext_res_0}}u,  // ext_res_0
    {{sampled_image.set}}u,        // set
    {{sampled_image.binding}}u,    // binding
  };
  static ShaderMetadata kMetadata{{camel_case(sampled_image.name)}};
{% endfor %}
{% endif %}
  // ===========================================================================
  // Stage Outputs =============================================================
  // ===========================================================================
{% if length(stage_outputs) > 0 %}
{% for stage_output in stage_outputs %}
  static constexpr auto kOutput{{camel_case(stage_output.name)}} = ShaderStageIOSlot { // {{stage_output.name}}
    "{{stage_output.name}}",             // name
    {{stage_output.location}}u,          // attribute location
    {{stage_output.descriptor_set}}u,    // attribute set
    {{stage_output.binding}}u,           // attribute binding
    {{stage_output.type.type_name}},     // type
    {{stage_output.type.bit_width}}u,    // bit width of type
    {{stage_output.type.vec_size}}u,     // vec size
    {{stage_output.type.columns}}u       // number of columns
  };
{% endfor %}
  static constexpr std::array<const ShaderStageIOSlot*, {{length(stage_outputs)}}> kAllShaderStageOutputs = {
{% for stage_output in stage_outputs %}
    &kOutput{{camel_case(stage_output.name)}}, // {{stage_output.name}}
{% endfor %}
  };
{% endif %}

  // ===========================================================================
  // Resource Binding Utilities ================================================
  // ===========================================================================

{% for proto in bind_prototypes %}
  /// {{proto.docstring}}
  static {{proto.return_type}} Bind{{proto.name}}({% for arg in proto.args %}
{{arg.type_name}} {{arg.argument_name}}{% if not loop.is_last %}, {% endif %}
{% endfor %}) {
    return {{proto.return_type}}({% for arg in proto.args %}
  {% if loop.is_first %}
{{to_shader_stage(shader_stage)}}, kResource{{ proto.name }}, kMetadata{{ proto.name }}, std::move({{ arg.argument_name }}){% if not loop.is_last %}, {% endif %} {% else %}
std::move({{ arg.argument_name }}){% if not loop.is_last %}, {% endif %}
  {% endif %}
  {% endfor %});
  }

{% endfor %}

  // ===========================================================================
  // Metadata for Vulkan =======================================================
  // ===========================================================================
  static constexpr std::array<DescriptorSetLayout,{{length(buffers)+length(sampled_images)+length(subpass_inputs)}}> kDescriptorSetLayouts{
{% for subpass_input in subpass_inputs %}
    DescriptorSetLayout{
      {{subpass_input.binding}}, // binding = {{subpass_input.binding}}
      {{subpass_input.descriptor_type}}, // descriptor_type = {{subpass_input.descriptor_type}}
      {{to_shader_stage(shader_stage)}}, // shader_stage = {{to_shader_stage(shader_stage)}}
    },
{% endfor %}
{% for buffer in buffers %}
    DescriptorSetLayout{
      {{buffer.binding}}, // binding = {{buffer.binding}}
      {{buffer.descriptor_type}}, // descriptor_type = {{buffer.descriptor_type}}
      {{to_shader_stage(shader_stage)}}, // shader_stage = {{to_shader_stage(shader_stage)}}
    },
{% endfor %}
{% for sampled_image in sampled_images %}
    DescriptorSetLayout{
      {{sampled_image.binding}}, // binding = {{sampled_image.binding}}
      {{sampled_image.descriptor_type}}, // descriptor_type = {{sampled_image.descriptor_type}}
      {{to_shader_stage(shader_stage)}}, // shader_stage = {{to_shader_stage(shader_stage)}}
    },
{% endfor %}
  };

};  // struct {{camel_case(shader_name)}}{{camel_case(shader_stage)}}Shader

}  // namespace impeller
)~~";

constexpr std::string_view kReflectionCCTemplate =
    R"~~(// THIS FILE IS GENERATED BY impellerc.
// DO NOT EDIT OR CHECK THIS INTO SOURCE CONTROL

#include "{{header_file_name}}"

#include <type_traits>

namespace impeller {

using Shader = {{camel_case(shader_name)}}{{camel_case(shader_stage)}}Shader;

{% for def in struct_definitions %}
// Sanity checks for {{def.name}}
{% if last(def.members).array_elements == 0 %}
static_assert(std::is_standard_layout_v<Shader::{{def.name}}<0>>);
{% for member in def.members %}
static_assert(offsetof(Shader::{{def.name}}<0>, {{member.name}}) == {{member.offset}});
{% endfor %}
{% else %}
static_assert(std::is_standard_layout_v<Shader::{{def.name}}>);
static_assert(sizeof(Shader::{{def.name}}) == {{def.byte_length}});
{% for member in def.members %}
static_assert(offsetof(Shader::{{def.name}}, {{member.name}}) == {{member.offset}});
{% endfor %}
{% endif %}
{% endfor %}

{% for buffer in buffers %}
ShaderMetadata Shader::kMetadata{{camel_case(buffer.name)}} = {
  "{{buffer.name}}",    // name
  std::vector<ShaderStructMemberMetadata> {
    {% for member in buffer.type.members %}
      ShaderStructMemberMetadata {
        {{ member.base_type }},      // type
        "{{ member.name }}",         // name
        {{ member.offset }},         // offset
        {{ member.size }},           // size
        {{ member.byte_length }},    // byte_length
        {{ member.array_elements }}, // array_elements
      },
    {% endfor %}
  } // members
};
{% endfor %}

{% for sampled_image in sampled_images %}
ShaderMetadata Shader::kMetadata{{camel_case(sampled_image.name)}} = {
    "{{sampled_image.name}}",    // name
    std::vector<ShaderStructMemberMetadata> {}, // 0 members
};
{% endfor %}

}  // namespace impeller
)~~";

}  // namespace compiler
}  // namespace impeller
