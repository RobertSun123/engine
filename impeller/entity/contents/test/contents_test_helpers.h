// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "impeller/renderer/command.h"
#include "impeller/renderer/render_pass.h"

namespace impeller {

/// @brief Retrieve the [VertInfo] struct data from the provided [command].
template <typename T>
typename T::VertInfo* GetVertInfo(const Command& command,
                                  const RenderPass& render_pass) {
  // auto resource = std::find_if(command.vertex_bindings.buffers.begin(),
  //                              command.vertex_bindings.buffers.end(),
  //                              [](const BoundBuffer& data) {
  //                                return data.slot.ext_res_0 == 0u;
  //                              });
  // if (resource == command.vertex_bindings.buffers.end()) {
  //   return nullptr;
  // }

  // auto data =
  //     (resource->view.resource.contents +
  //     resource->view.resource.range.offset);
  // return reinterpret_cast<typename T::VertInfo*>(data);
  return nullptr;  // TODO
}

/// @brief Retrieve the [FragInfo] struct data from the provided [command].
template <typename T>
typename T::FragInfo* GetFragInfo(const Command& command,
                                  const RenderPass& render_pass) {
  // auto resource = std::find_if(command.fragment_bindings.buffers.begin(),
  //                              command.fragment_bindings.buffers.end(),
  //                              [](const BoundBuffer& data) {
  //                                return data.slot.ext_res_0 == 0u;
  //                              });
  // if (resource == command.fragment_bindings.buffers.end()) {
  //   return nullptr;
  // }

  // auto data =
  //     (resource->view.resource.contents +
  //     resource->view.resource.range.offset);
  // return reinterpret_cast<typename T::FragInfo*>(data);
  return nullptr;  // TODO
}

}  // namespace impeller
