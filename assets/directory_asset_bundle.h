// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_ASSETS_DIRECTORY_ASSET_BUNDLE_H_
#define FLUTTER_ASSETS_DIRECTORY_ASSET_BUNDLE_H_

#include "flutter/assets/asset_resolver.h"
#include "flutter/fml/macros.h"
#include "flutter/fml/memory/ref_counted.h"
#include "flutter/fml/unique_fd.h"

namespace flutter {

class DirectoryAssetBundle : public AssetResolver {
 public:
  explicit DirectoryAssetBundle(fml::UniqueFD descriptor, bool should_preserve);

  ~DirectoryAssetBundle() override;

 private:
  const fml::UniqueFD descriptor_;
  bool is_valid_ = false;
  bool should_preserve_ = false;

  // |AssetResolver|
  bool IsValid() const override;

  // |AssetResolver|
  bool ShouldPreserve() const override;

  // |AssetResolver|
  std::unique_ptr<fml::Mapping> GetAsMapping(
      const std::string& asset_name) const override;

  FML_DISALLOW_COPY_AND_ASSIGN(DirectoryAssetBundle);
};

}  // namespace flutter

#endif  // FLUTTER_ASSETS_DIRECTORY_ASSET_BUNDLE_H_
