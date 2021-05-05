// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "Path.h"
#include "Rect.h"
#include "flutter/fml/macros.h"

namespace rl {
namespace geom {

class PathBuilder {
 public:
  PathBuilder();

  ~PathBuilder();

  Path CreatePath() const;

  PathBuilder& MoveTo(Point point, bool relative = false);

  PathBuilder& Close();

  PathBuilder& LineTo(Point point, bool relative = false);

  PathBuilder& HorizontalLineTo(double x, bool relative = false);

  PathBuilder& VerticalLineTo(double y, bool relative = false);

  PathBuilder& QuadraticCurveTo(Point point,
                                Point controlPoint,
                                bool relative = false);

  PathBuilder& SmoothQuadraticCurveTo(Point point, bool relative = false);

  PathBuilder& CubicCurveTo(Point point,
                            Point controlPoint1,
                            Point controlPoint2,
                            bool relative = false);

  PathBuilder& SmoothCubicCurveTo(Point point,
                                  Point controlPoint2,
                                  bool relative = false);

  PathBuilder& AddRect(Rect rect);

  PathBuilder& AddRoundedRect(Rect rect, double radius);

  PathBuilder& AddCircle(const Point& center, double radius);

  PathBuilder& AddEllipse(const Point& center, const Size& size);

  struct RoundingRadii {
    double topLeft;
    double bottomLeft;
    double topRight;
    double bottomRight;

    RoundingRadii()
        : topLeft(0.0), bottomLeft(0.0), topRight(0.0), bottomRight(0.0) {}

    RoundingRadii(double pTopLeft,
                  double pBottomLeft,
                  double pTopRight,
                  double pBottomRight)
        : topLeft(pTopLeft),
          bottomLeft(pBottomLeft),
          topRight(pTopRight),
          bottomRight(pBottomRight) {}
  };

  PathBuilder& AddRoundedRect(Rect rect, RoundingRadii radii);

 private:
  Point subpath_start_;
  Point current_;
  Path prototype_;

  Point ReflectedQuadraticControlPoint1() const;

  Point ReflectedCubicControlPoint1() const;

  FML_DISALLOW_COPY_AND_ASSIGN(PathBuilder);
};

}  // namespace geom
}  // namespace rl
