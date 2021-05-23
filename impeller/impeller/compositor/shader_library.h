// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <Metal/Metal.h>

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

#include "flutter/fml/hash_combine.h"
#include "flutter/fml/macros.h"
#include "impeller/compositor/comparable.h"
#include "impeller/shader_glue/shader_types.h"

namespace impeller {

class Context;

class ShaderFunction : public Comparable<ShaderFunction> {
 public:
  virtual ~ShaderFunction();

  ShaderStage GetStage() const;

  id<MTLFunction> GetMTLFunction() const;

  // Comparable<ShaderFunction>
  std::size_t GetHash() const override;

  // Comparable<ShaderFunction>
  bool IsEqual(const ShaderFunction& other) const override;

 private:
  friend class ShaderLibrary;

  id<MTLFunction> function_ = nullptr;
  ShaderStage stage_;

  ShaderFunction(id<MTLFunction> function, ShaderStage stage);

  FML_DISALLOW_COPY_AND_ASSIGN(ShaderFunction);
};

class ShaderLibrary {
 public:
  ~ShaderLibrary();

  std::shared_ptr<const ShaderFunction> GetFunction(
      const std::string_view& name,
      ShaderStage stage);

 private:
  friend class Context;

  struct ShaderKey {
    std::string name;
    ShaderStage stage = ShaderStage::kUnknown;

    ShaderKey(const std::string_view& p_name, ShaderStage p_stage)
        : name({p_name.data(), p_name.size()}), stage(p_stage) {}

    struct Hash {
      size_t operator()(const ShaderKey& key) const {
        return fml::HashCombine(key.name, key.stage);
      }
    };

    struct Equal {
      constexpr bool operator()(const ShaderKey& k1,
                                const ShaderKey& k2) const {
        return k1.stage == k2.stage && k1.name == k2.name;
      }
    };
  };

  id<MTLLibrary> library_ = nullptr;
  using Functions = std::unordered_map<ShaderKey,
                                       std::shared_ptr<const ShaderFunction>,
                                       ShaderKey::Hash,
                                       ShaderKey::Equal>;
  Functions functions_;

  ShaderLibrary(id<MTLLibrary> library);

  FML_DISALLOW_COPY_AND_ASSIGN(ShaderLibrary);
};

}  // namespace impeller
