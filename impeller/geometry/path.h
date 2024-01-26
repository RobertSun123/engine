// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_IMPELLER_GEOMETRY_PATH_H_
#define FLUTTER_IMPELLER_GEOMETRY_PATH_H_

#include <functional>
#include <optional>
#include <set>
#include <tuple>
#include <vector>

#include "impeller/geometry/path_component.h"

namespace impeller {

enum class Cap {
  kButt,
  kRound,
  kSquare,
};

enum class Join {
  kMiter,
  kRound,
  kBevel,
};

enum class FillType {
  kNonZero,  // The default winding order.
  kOdd,
  kPositive,
  kNegative,
  kAbsGeqTwo,
};

enum class Convexity {
  kUnknown,
  kConvex,
};

//------------------------------------------------------------------------------
/// @brief      Paths are lightweight objects that describe a collection of
///             linear, quadratic, or cubic segments. These segments may be
///             broken up by move commands, which are effectively linear
///             commands that pick up the pen rather than continuing to draw.
///
///             All shapes supported by Impeller are paths either directly or
///             via approximation (in the case of circles).
///
///             Paths are externally immutable once created, Creating paths must
///             be done using a path builder.
///
class Path {
 public:
  enum class ComponentType {
    kLinear,
    kQuadratic,
    kCubic,
    kContour,
  };

  struct PolylineContour {
    struct Component {
      size_t component_start_index;
      /// Denotes whether this component is a curve.
      ///
      /// This is set to true when this component is generated from
      /// QuadraticComponent or CubicPathComponent.
      bool is_curve;
    };
    /// Index that denotes the first point of this contour.
    size_t start_index;

    /// Denotes whether the last point of this contour is connected to the first
    /// point of this contour or not.
    bool is_closed;

    /// The direction of the contour's start cap.
    Vector2 start_direction;
    /// The direction of the contour's end cap.
    Vector2 end_direction;

    /// Distinct components in this contour.
    ///
    /// If this contour is generated from multiple path components, each
    /// path component forms a component in this vector.
    std::vector<Component> components;
  };

  /// One or more contours represented as a series of points and indices in
  /// the point vector representing the start of a new contour.
  ///
  /// Polylines are ephemeral and meant to be used by the tessellator. They do
  /// not allocate their own point vectors to allow for optimizations around
  /// allocation and reuse of arenas.
  struct Polyline {
    /// The signature of a method called when it is safe to reclaim the point
    /// buffer provided to the constructor of this object.
    using PointBufferPtr = std::unique_ptr<std::vector<Point>>;
    using ReclaimPointBufferCallback = std::function<void(PointBufferPtr)>;

    /// The buffer will be cleared and returned at the destruction of this
    /// polyline.
    Polyline(PointBufferPtr point_buffer, ReclaimPointBufferCallback reclaim);

    Polyline(Polyline&& other);
    ~Polyline();

    /// Points in the polyline, which may represent multiple contours specified
    /// by indices in |contours|.
    PointBufferPtr points;

    Point& GetPoint(size_t index) const { return (*points)[index]; }

    /// Contours are disconnected pieces of a polyline, such as when a MoveTo
    /// was issued on a PathBuilder.
    std::vector<PolylineContour> contours;

    /// Convenience method to compute the start (inclusive) and end (exclusive)
    /// point of the given contour index.
    ///
    /// The contour_index parameter is clamped to contours.size().
    std::tuple<size_t, size_t> GetContourPointBounds(
        size_t contour_index) const;

   private:
    ReclaimPointBufferCallback reclaim_points_;
  };

  Path();

  ~Path();

  Path(const Path& other);
  Path(Path&& other);

  void operator=(Path&& other);
  void operator=(const Path& other);

  size_t GetComponentCount(std::optional<ComponentType> type = {}) const;

  FillType GetFillType() const;

  bool IsConvex() const;

  bool IsEmpty() const;

  template <class T>
  using Applier = std::function<void(size_t index, const T& component)>;
  void EnumerateComponents(
      const Applier<LinearPathComponent>& linear_applier,
      const Applier<QuadraticPathComponent>& quad_applier,
      const Applier<CubicPathComponent>& cubic_applier,
      const Applier<ContourComponent>& contour_applier) const;

  bool GetLinearComponentAtIndex(size_t index,
                                 LinearPathComponent& linear) const;

  bool GetQuadraticComponentAtIndex(size_t index,
                                    QuadraticPathComponent& quadratic) const;

  bool GetCubicComponentAtIndex(size_t index, CubicPathComponent& cubic) const;

  bool GetContourComponentAtIndex(size_t index,
                                  ContourComponent& contour) const;

  /// Callers must provide the scale factor for how this path will be
  /// transformed.
  ///
  /// It is suitable to use the max basis length of the matrix used to transform
  /// the path. If the provided scale is 0, curves will revert to straight
  /// lines.
  Polyline CreatePolyline(
      Scalar scale,
      Polyline::PointBufferPtr point_buffer =
          std::make_unique<std::vector<Point>>(),
      Polyline::ReclaimPointBufferCallback reclaim = nullptr) const;

  std::optional<Rect> GetBoundingBox() const;

  std::optional<Rect> GetTransformedBoundingBox(const Matrix& transform) const;

  std::optional<std::pair<Point, Point>> GetMinMaxCoveragePoints() const;

 private:
  friend class PathBuilder;

  /// @brief Reserve [point_size] points and [verb_size] verbs in the
  ///        associated vectors.
  void Reserve(size_t point_size, size_t verb_size);

  void SetConvexity(Convexity value);

  void SetFillType(FillType fill);

  void SetBounds(Rect rect);

  Path& AddLinearComponent(const Point& p1, const Point& p2);

  Path& AddQuadraticComponent(const Point& p1,
                              const Point& cp,
                              const Point& p2);

  Path& AddCubicComponent(const Point& p1,
                          const Point& cp1,
                          const Point& cp2,
                          const Point& p2);

  Path& AddContourComponent(const Point& destination, bool is_closed = false);

  /// @brief Called by `PathBuilder` to compute the bounds for certain paths.
  ///
  /// `PathBuilder` may set the bounds directly, in case they come from a source
  /// with already computed bounds, such as an SkPath.
  void ComputeBounds();

  void SetContourClosed(bool is_closed);

  void Shift(Point shift);

  struct ComponentIndexPair {
    ComponentType type = ComponentType::kLinear;
    size_t index = 0;

    ComponentIndexPair() {}

    ComponentIndexPair(ComponentType a_type, size_t a_index)
        : type(a_type), index(a_index) {}
  };

  // All of the data for the path is stored in this structure which is
  // held by a shared_ptr. Since they all share the structure, the
  // copy constructor for Path is very cheap and we don't need to deal
  // with shared pointers for Path fields and method arguments.
  //
  // The PathBuilder will also share its internal prototype_'s data
  // with the results of |TakePath()| by virtue of marking the data
  // locked. If the PathBuilder is never used again then we never
  // copy the data. If it is used again, then the next call that
  // modifies the data in the prototype_ will discover that it is
  // locked when |GetModifiableData()| is called and the prototype_
  // will clone its data at that point - leaving all of the Path
  // references that referred to its most recently "taken" version
  // to share the old data.
  struct Data {
    Data() = default;
    Data(const Data& other) = default;

    ~Data() = default;

    FillType fill = FillType::kNonZero;
    Convexity convexity = Convexity::kUnknown;
    std::vector<ComponentIndexPair> components;
    std::vector<Point> points;
    std::vector<ContourComponent> contours;

    std::optional<Rect> computed_bounds;

    bool locked = false;
  };

  // The local shared reference to the data for this path. The name is
  // rather long to discourage any use of the field directly in methods
  // that aren't specifically managing construct/copy semantics. Regular
  // methods should get a reference to this structure using either
  // |GetImmutableData()| from a const method, or |GetModifiableData()|
  // from methods that mutate the path. Such mutating methods should only
  // ever be called by PathBuilder on its own prototype_ instance and
  // will copy-on-write-if-shared the structure to isolate the changes
  // from any of the "taken" paths.
  std::shared_ptr<Data> copy_on_shared_write_data_;

  const Data& GetImmutableData() const { return *copy_on_shared_write_data_; }
  Data& GetModifiableData();
};

static_assert(sizeof(Path) == sizeof(std::shared_ptr<struct Anonymous>));

}  // namespace impeller

#endif  // FLUTTER_IMPELLER_GEOMETRY_PATH_H_
