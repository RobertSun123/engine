// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_DISPLAY_LIST_GEOMETRY_REGION_H_
#define FLUTTER_DISPLAY_LIST_GEOMETRY_REGION_H_

#include "third_party/skia/include/core/SkRect.h"

#include <vector>

namespace flutter {

/// Represents a region as a collection of non-overlapping rectangles.
/// Implements a subset of SkRegion functionality optimized for quickly
/// converting set of overlapping rectangles to non-overlapping rectangles.
class DlRegion {
 public:
  DlRegion();
  ~DlRegion();

  /// Adds rectangle to current region.
  /// Matches SkRegion::op(rect, SkRegion::kUnion_Op) behavior.
  void addRect(const SkIRect& rect);

  /// Returns list of non-overlapping rectangles that cover current region.
  /// If |deband| is false, each span line will result in separate rectangles,
  /// closely matching SkRegion::Iterator behavior.
  /// If |deband| is true, matching rectangles from adjacent span lines will be
  /// merged into single rectange.
  std::vector<SkIRect> getRects(bool deband = true) const;

 private:
  struct Span;
  struct SpanLine;
  typedef std::vector<Span> SpanVec;
  typedef std::vector<SpanLine> LineVec;

  void insertLine(size_t position, SpanLine line);
  LineVec::iterator removeLine(LineVec::iterator position);

  SpanLine makeLine(int32_t top,
                    int32_t bottom,
                    int32_t spanLeft,
                    int32_t spanRight);
  SpanLine makeLine(int32_t top, int32_t bottom, const SpanVec& spans);

  LineVec lines_;
  std::vector<SpanVec*> spanvec_pool_;
};

class DlRegion2 {
 public:
  void addRects(std::vector<SkIRect>&& rects);
  std::vector<SkIRect> getRects(bool deband = true) const;

 private:
  struct Span {
    int32_t left;
    int32_t right;
  };
  struct SpanLine {
    int32_t top;
    int32_t bottom;
    std::vector<Span> spans;

    void insertSpan(int32_t left, int32_t right);
  };

  std::vector<SpanLine> lines_;
};

/// Represents a region as a collection of non-overlapping rectangles.
/// Implements a subset of SkRegion functionality optimized for quickly
/// converting set of overlapping rectangles to non-overlapping rectangles.
class DlRegionSorted {
 public:
  /// Bulks adds rectangles to current region.
  /// Matches SkRegion::op(rect, SkRegion::kUnion_Op) behavior.
  void addRects(std::vector<SkIRect>&& rects);

  /// Returns list of non-overlapping rectangles that cover current region.
  /// If |deband| is false, each span line will result in separate rectangles,
  /// closely matching SkRegion::Iterator behavior.
  /// If |deband| is true, matching rectangles from adjacent span lines will be
  /// merged into single rectange.
  std::vector<SkIRect> getRects(bool deband = true) const;

 private:
  struct Span {
    int32_t left;
    int32_t right;
  };
  typedef std::vector<Span> SpanVec;
  struct SpanLine {
    int32_t top;
    int32_t bottom;
    SpanVec* spans;

    void insertSpan(int32_t left, int32_t right);
    bool spansEqual(const SpanLine& l2) const;
  };

  typedef std::vector<SpanLine> LineVec;

  std::vector<SpanLine> lines_;
  std::vector<SpanVec*> spanvec_pool_;

  void insertLine(size_t position, SpanLine line);
  LineVec::iterator removeLine(LineVec::iterator position);

  SpanLine makeLine(int32_t top,
                    int32_t bottom,
                    int32_t spanLeft,
                    int32_t spanRight);
  SpanLine makeLine(int32_t top, int32_t bottom, const SpanVec& spans);
};

}  // namespace flutter

#endif  // FLUTTER_DISPLAY_LIST_GEOMETRY_REGION_H_
