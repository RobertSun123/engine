// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <cstdint>

#include "flutter/fml/build_config.h"

#include "impeller/geometry/color.h"
#include "impeller/geometry/point.h"
#include "impeller/geometry/scalar.h"
#include "impeller/geometry/vector.h"

#if defined(FML_OS_WIN) || defined(FML_OS_ANDROID)
using InternalHalf = uint16_t;
#else
using InternalHalf = _Float16;
#endif

namespace impeller {

/// @brief Convert a scalar to a half precision float.
///
/// See also: https://clang.llvm.org/docs/LanguageExtensions.html
/// This is not currently supported on Windows on some Android toolchains.
inline constexpr InternalHalf ScalarToHalf(Scalar f) {
#if defined(FML_OS_WIN) || defined(FML_OS_ANDROID)
  uint32_t x = static_cast<uint32_t>(f);
  uint32_t sign = static_cast<unsigned short>(x >> 31);
  uint32_t mantissa = 0;
  uint32_t exp = 0;
  uint16_t hf = 0;

  mantissa = x & ((1 << 23) - 1);
  exp = x & (0xFF << 23);
  if (exp >= 0x47800000) {
    // check if the original number is a NaN
    if (mantissa && (exp == (0xFF << 23))) {
      // single precision NaN
      mantissa = (1 << 23) - 1;
    } else {
      // half-float will be Inf
      mantissa = 0;
    }
    hf = ((static_cast<uint16_t>(sign)) << 15) |
         static_cast<uint16_t>((0x1F << 10)) |
         static_cast<uint16_t>(mantissa >> 13);
  }
  // check if exponent is <= -15
  else if (exp <= 0x38000000) {
    hf = 0;  // too small to be represented
  } else {
    hf = ((static_cast<uint16_t>(sign)) << 15) |
         static_cast<uint16_t>((exp - 0x38000000) >> 13) |
         static_cast<uint16_t>(mantissa >> 13);
  }

  return hf;
#else
  return static_cast<InternalHalf>(f);
#endif
}

/// @brief A storage only class for half precision floating point.
struct Half {
  InternalHalf x = 0;

  constexpr Half() {}

  constexpr Half(double value) : x(ScalarToHalf(static_cast<Scalar>(value))) {}

  constexpr Half(Scalar value) : x(ScalarToHalf(value)) {}

  constexpr Half(int value) : x(ScalarToHalf(static_cast<Scalar>(value))) {}

  constexpr Half(InternalHalf x) : x(x) {}

  constexpr bool operator==(const Half& v) const { return v.x == x; }

  constexpr bool operator!=(const Half& v) const { return v.x != x; }
};

/// @brief A storage only class for half precision floating point vector 4.
struct HalfVector4 {
  union {
    struct {
      InternalHalf x = 0;
      InternalHalf y = 0;
      InternalHalf z = 0;
      InternalHalf w = 0;
    };
    InternalHalf e[4];
  };

  constexpr HalfVector4() {}

  constexpr HalfVector4(const Color& a)
      : x(ScalarToHalf(a.red)),
        y(ScalarToHalf(a.green)),
        z(ScalarToHalf(a.blue)),
        w(ScalarToHalf(a.alpha)) {}

  constexpr HalfVector4(const Vector4& a)
      : x(ScalarToHalf(a.x)),
        y(ScalarToHalf(a.y)),
        z(ScalarToHalf(a.z)),
        w(ScalarToHalf(a.w)) {}

  constexpr HalfVector4(InternalHalf x,
                        InternalHalf y,
                        InternalHalf z,
                        InternalHalf w)
      : x(x), y(y), z(z), w(w) {}

  constexpr bool operator==(const HalfVector4& v) const {
    return v.x == x && v.y == y && v.z == z && v.w == w;
  }

  constexpr bool operator!=(const HalfVector4& v) const {
    return v.x != x || v.y != y || v.z != z || v.w != w;
  }
};

/// @brief A storage only class for half precision floating point vector 3.
struct HalfVector3 {
  union {
    struct {
      InternalHalf x = 0;
      InternalHalf y = 0;
      InternalHalf z = 0;
    };
    InternalHalf e[3];
  };

  constexpr HalfVector3() {}

  constexpr HalfVector3(const Vector3& a)
      : x(ScalarToHalf(a.x)), y(ScalarToHalf(a.y)), z(ScalarToHalf(a.z)) {}

  constexpr HalfVector3(InternalHalf x, InternalHalf y, InternalHalf z)
      : x(x), y(y), z(z) {}

  constexpr bool operator==(const HalfVector3& v) const {
    return v.x == x && v.y == y && v.z == z;
  }

  constexpr bool operator!=(const HalfVector3& v) const {
    return v.x != x || v.y != y || v.z != z;
  }
};

/// @brief A storage only class for half precision floating point vector 2.
struct HalfVector2 {
  union {
    struct {
      InternalHalf x = 0;
      InternalHalf y = 0;
    };
    InternalHalf e[2];
  };

  constexpr HalfVector2() {}

  constexpr HalfVector2(const Vector2& a)
      : x(ScalarToHalf(a.x)), y(ScalarToHalf(a.y)) {}

  constexpr HalfVector2(InternalHalf x, InternalHalf y) : x(x), y(y){};

  constexpr bool operator==(const HalfVector2& v) const {
    return v.x == x && v.y == y;
  }

  constexpr bool operator!=(const HalfVector2& v) const {
    return v.x != x || v.y != y;
  }
};

static_assert(sizeof(Half) == sizeof(uint16_t));
static_assert(sizeof(HalfVector2) == 2 * sizeof(Half));
static_assert(sizeof(HalfVector3) == 3 * sizeof(Half));
static_assert(sizeof(HalfVector4) == 4 * sizeof(Half));

}  // namespace impeller

namespace std {

inline std::ostream& operator<<(std::ostream& out, const impeller::Half& p) {
  out << "(" << static_cast<impeller::Scalar>(p.x) << ")";
  return out;
}

inline std::ostream& operator<<(std::ostream& out,
                                const impeller::HalfVector2& p) {
  out << "(" << static_cast<impeller::Scalar>(p.x) << ", "
      << static_cast<impeller::Scalar>(p.y) << ")";
  return out;
}

inline std::ostream& operator<<(std::ostream& out,
                                const impeller::HalfVector3& p) {
  out << "(" << static_cast<impeller::Scalar>(p.x) << ", "
      << static_cast<impeller::Scalar>(p.y) << ", "
      << static_cast<impeller::Scalar>(p.z) << ")";
  return out;
}

inline std::ostream& operator<<(std::ostream& out,
                                const impeller::HalfVector4& p) {
  out << "(" << static_cast<impeller::Scalar>(p.x) << ", "
      << static_cast<impeller::Scalar>(p.y) << ", "
      << static_cast<impeller::Scalar>(p.z) << ", "
      << static_cast<impeller::Scalar>(p.w) << ")";
  return out;
}

}  // namespace std
