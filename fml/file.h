// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_FML_FILE_H_
#define FLUTTER_FML_FILE_H_

#include <initializer_list>
#include <string>
#include <vector>

#include "flutter/fml/macros.h"
#include "flutter/fml/unique_fd.h"

#ifdef ERROR
#undef ERROR
#endif

namespace fml {

class Mapping;

enum class FilePermission {
  kRead,
  kWrite,
  kReadWrite,
};

std::string CreateTemporaryDirectory();

fml::UniqueFD OpenFile(const char* path,
                       bool create_if_necessary,
                       FilePermission permission);

fml::UniqueFD OpenFile(const fml::UniqueFD& base_directory,
                       const char* path,
                       bool create_if_necessary,
                       FilePermission permission);

fml::UniqueFD OpenDirectory(const char* path,
                            bool create_if_necessary,
                            FilePermission permission);

fml::UniqueFD OpenDirectory(const fml::UniqueFD& base_directory,
                            const char* path,
                            bool create_if_necessary,
                            FilePermission permission);

fml::UniqueFD Duplicate(fml::UniqueFD::element_type descriptor);

bool IsDirectory(const fml::UniqueFD& directory);

// Returns whether the given path is a file.
bool IsFile(const std::string& path);

bool TruncateFile(const fml::UniqueFD& file, size_t size);

bool FileExists(const fml::UniqueFD& base_directory, const char* path);

bool UnlinkDirectory(const char* path);

bool UnlinkDirectory(const fml::UniqueFD& base_directory, const char* path);

bool UnlinkFile(const char* path);

bool UnlinkFile(const fml::UniqueFD& base_directory, const char* path);

fml::UniqueFD CreateDirectory(const fml::UniqueFD& base_directory,
                              const std::vector<std::string>& components,
                              FilePermission permission);

bool WriteAtomically(const fml::UniqueFD& base_directory,
                     const char* file_name,
                     const Mapping& mapping);

using FileVisitor = std::function<void(const fml::UniqueFD& directory,
                                       const std::string& filename)>;

/// Call `visitor` on all files inside the `directory` non-recursively. If
/// recursive visiting is needed, call `VisitFiles` inside the `visitor`.
/// The trivial file "." and ".." will not be visited.
void VisitFiles(const fml::UniqueFD& directory, const FileVisitor& visitor);

class ScopedTemporaryDirectory {
 public:
  ScopedTemporaryDirectory();

  ~ScopedTemporaryDirectory();

  const UniqueFD& fd() { return dir_fd_; }

 private:
  std::string path_;
  UniqueFD dir_fd_;
};

}  // namespace fml

#endif  // FLUTTER_FML_FILE_H_
