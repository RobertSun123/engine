// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_FML_BASE32_H_
#define FLUTTER_FML_BASE32_H_

#include <string_view>
#include <utility>

#include "flutter/fml/logging.h"

namespace fml {

template <int from_length, int to_length, int buffer_length>
class BitConverter {
 public:
  // Although member functions in headers are by default inline, we put extra
  // "inline" keywords to strongly suggest the compiler to make these frequently
  // called functions inline.
  //
  // TODO(liyuqian): Ask Chinmay "Can we use SK_WALWAYS_INLINE in fml?".
  inline void Append(int bits) {
    FML_DCHECK(bits < (1 << from_length));
    FML_DCHECK(CanAppend());
    lower_free_bits_ -= from_length;
    buffer_ |= (bits << lower_free_bits_);
  }

  inline int Extract() {
    FML_DCHECK(CanExtract());
    int result = Peek();
    buffer_ = (buffer_ << to_length) & mask_;
    lower_free_bits_ += to_length;
    return result;
  }

  inline int Peek() const { return (buffer_ >> (buffer_length - to_length)); }
  inline int BitsAvailable() const { return buffer_length - lower_free_bits_; }
  inline bool CanAppend() const { return lower_free_bits_ >= from_length; }
  inline bool CanExtract() const { return BitsAvailable() >= to_length; }

 private:
  static_assert(buffer_length >= 2 * from_length);
  static_assert(buffer_length >= 2 * to_length);
  static_assert(buffer_length < sizeof(int) * 8);

  static constexpr int mask_ = (1 << buffer_length) - 1;

  int buffer_ = 0;
  int lower_free_bits_ = buffer_length;
};

using Base32DecodeConverter = BitConverter<5, 8, 16>;
using Base32EncodeConverter = BitConverter<8, 5, 16>;

std::pair<bool, std::string> Base32Encode(std::string_view input);
std::pair<bool, std::string> Base32Decode(const std::string& input);

}  // namespace fml

#endif  // FLUTTER_FML_BASE32_H_
