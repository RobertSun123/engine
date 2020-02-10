// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

part of engine;

const double kEpsilon = 0.000000001;

/// An iterable collection of [PathMetric] objects describing a [Path].
///
/// A [PathMetrics] object is created by using the [Path.computeMetrics] method,
/// and represents the path as it stood at the time of the call. Subsequent
/// modifications of the path do not affect the [PathMetrics] object.
///
/// Each path metric corresponds to a segment, or contour, of a path.
///
/// For example, a path consisting of a [Path.lineTo], a [Path.moveTo], and
/// another [Path.lineTo] will contain two contours and thus be represented by
/// two [PathMetric] objects.
///
/// When iterating across a [PathMetrics]' contours, the [PathMetric] objects
/// are only valid until the next one is obtained.
class SurfacePathMetrics extends IterableBase<ui.PathMetric> implements ui.PathMetrics {
  SurfacePathMetrics._(SurfacePath path, bool forceClosed)
      : _iterator = SurfacePathMetricIterator._(_SurfacePathMeasure(path, forceClosed));

  final SurfacePathMetricIterator _iterator;

  @override
  Iterator<ui.PathMetric> get iterator => _iterator;
}

/// Maintains a single instance of computed segments for set of PathMetric
/// objects exposed through iterator.
class _SurfacePathMeasure {
  _SurfacePathMeasure(this._path, this.forceClosed) {
    _currentContourIndex = -1; // nextContour will increment this to the zero based index.
  }

  final SurfacePath _path;
  final List<_PathContourMeasure> _contours = [];

  // If the contour ends with a call to [Path.close] (which may
  // have been implied when using [Path.addRect])
  final bool forceClosed;

  int _currentContourIndex;
  int get currentContourIndex => _currentContourIndex;

  // Iterator index into [Path.subPaths]
  int _subPathIndex = 0;
  _PathContourMeasure _contourMeasure;

  double length(int contourIndex) {
    assert(contourIndex <=
        currentContourIndex, 'Iterator must be advanced before index $contourIndex can be used.');
    return _contours[contourIndex].length;
  }

  /// Computes the position of hte current contour at the given offset, and the
  /// angle of the path at that point.
  ///
  /// For example, calling this method with a distance of 1.41 for a line from
  /// 0.0,0.0 to 2.0,2.0 would give a point 1.0,1.0 and the angle 45 degrees
  /// (but in radians).
  ///
  /// Returns null if the contour has zero [length].
  ///
  /// The distance is clamped to the [length] of the current contour.
  ui.Tangent getTangentForOffset(int contourIndex, double distance) {
    return _contours[contourIndex].getTangentForOffset(distance);
  }

  bool isClosed(int contourIndex) => _contours[contourIndex].isClosed;

  // Move to the next contour in the path.
  //
  // A path can have a next contour if [Path.moveTo] was called after drawing began.
  // Return true if one exists, or false.
  bool _nextContour() {
    final bool next = _nativeNextContour();
    if (next) {
      _currentContourIndex++;
    }
    return next;
  }

  // Move to the next contour in the path.
  //
  // A path can have a next contour if [Path.moveTo] was called after drawing
  // began. Return true if one exists, or false.
  //
  // This is not exactly congruent with a regular [Iterator.moveNext].
  // Typically, [Iterator.moveNext] should be called before accessing the
  // [Iterator.current]. In this case, the [PathMetric] is valid before
  // calling `_moveNext` - `_moveNext` should be called after the first
  // iteration is done instead of before.
  bool _nativeNextContour() {
    if (_subPathIndex == (_path.subpaths.length - 1)) {
      return false;
    }
    ++_subPathIndex;
    _contourMeasure = _PathContourMeasure(_path.subpaths[_subPathIndex], forceClosed);
    _contours.add(_contourMeasure);
    return true;
  }

  ui.Path extractPath(int contourIndex, double start, double end,
      {bool startWithMoveTo = true}) {
    _contours[contourIndex].extractPath(start, end, startWithMoveTo);
  }
}

/// Builds segments for a single contour to measure distance, compute tangent
/// and extract a sub path.
class _PathContourMeasure {
  _PathContourMeasure(this.subPath, this.forceClosed) {
    _buildSegments();
  }

  final List<_PathSegment> _segments = [];

  final Subpath subPath;
  final bool forceClosed;
  double get length => _contourLength;
  bool get isClosed => _isClosed;

  double _contourLength = 0.0;
  bool _isClosed = false;

  ui.Tangent getTangentForOffset(double distance) {
    if (distance.isNaN) {
      return null;
    }
    // Pin distance to legal range.
    if (distance < 0.0) {
      distance = 0.0;
    } else if (distance > _contourLength) {
      distance = _contourLength;
    }

    // Binary search through segments to find segment at distance.
    if (_segments.isEmpty) {
      return null;
    }
    int lo = 0;
    int hi = _segments.length - 1;
    while (lo < hi) {
      int mid = (lo + hi) >> 1;
      if (_segments[mid].distance < distance) {
        lo = mid + 1;
      } else {
        hi = mid;
      }
    }
    if (_segments[hi].distance < distance) {
      hi++;
    }
    return _getPosTan(hi, distance);
  }

  ui.Tangent _getPosTan(int segmentIndex, double distance) {
    _PathSegment segment = _segments[segmentIndex];
    // Compute distance to segment. Since distance is cumulative to find
    // t = 0..1 on the segment, we need to calculate start distance using prior
    // segment.
    final double startDistance = segmentIndex == 0 ? 0 : _segments[segmentIndex - 1].distance;
    final double totalDistance = segment.distance - startDistance;
    final double t = totalDistance < kEpsilon ? 0 :
        (distance - startDistance) / totalDistance;
    return segment.computeTangent(t);
  }

  ui.Path extractPath(double start, double end, bool startWithMoveTo) {
    throw UnimplementedError();
  }

  void _buildSegments() {
    assert(_segments.isEmpty, '_buildSegments should be called once');
    _isClosed = false;
    double distance = 0.0;
    bool haveSeenMoveTo = false;

    final List<PathCommand> commands = subPath.commands;
    double currentX = 0.0, currentY = 0.0;
    final Function lineToHandler = (double x, double y) {
      final double dx = currentX - x;
      final double dy = currentY - y;
      final double prevDistance = distance;
      distance += math.sqrt(dx * dx + dy * dy);
      // As we accumulate distance, we have to check that the result of +=
      // actually made it larger, since a very small delta might be > 0, but
      // still have no effect on distance (if distance >>> delta).
      if (distance > prevDistance) {
        _segments.add(_PathSegment(PathCommandTypes.lineTo, distance,
            [currentX, currentY, x, y]));
      }
      currentX = x;
      currentY = y;
    };
    _EllipseSegmentResult ellipseResult;
    for (PathCommand command in commands) {
      switch (command.type) {
        case PathCommandTypes.moveTo:
          final MoveTo moveTo = command;
          currentX = moveTo.x;
          currentY = moveTo.y;
          haveSeenMoveTo = true;
          break;
        case PathCommandTypes.lineTo:
          assert(haveSeenMoveTo);
          final LineTo lineTo = command;
          lineToHandler(lineTo.x, lineTo.y);
          break;
        case PathCommandTypes.bezierCurveTo:
          assert(haveSeenMoveTo);
          final BezierCurveTo curve = command;
          // Compute cubic curve distance.
          distance = _computeCubicSegments(
              currentX,
              currentY,
              curve.x1,
              curve.y1,
              curve.x2,
              curve.y2,
              curve.x3,
              curve.y3,
              distance,
              0,
              _kMaxTValue,
              _segments);
          break;
        case PathCommandTypes.quadraticCurveTo:
          assert(haveSeenMoveTo);
          final QuadraticCurveTo quadraticCurveTo = command;
          // Compute quad curve distance.
          distance = _computeQuadSegments(
              currentX,
              currentY,
              quadraticCurveTo.x1,
              quadraticCurveTo.y1,
              quadraticCurveTo.x2,
              quadraticCurveTo.y2,
              distance,
              0,
              _kMaxTValue);
          break;
        case PathCommandTypes.close:
          break;
        case PathCommandTypes.ellipse:
          final Ellipse ellipse = command;
          ellipseResult ??= _EllipseSegmentResult();
          _computeEllipseSegments(
              currentX,
              currentY,
              distance,
              ellipse.x,
              ellipse.y,
              ellipse.startAngle,
              ellipse.endAngle,
              ellipse.rotation,
              ellipse.radiusX,
              ellipse.radiusY,
              ellipse.anticlockwise,
              ellipseResult,
              _segments);
          distance = ellipseResult.distance;
          currentX = ellipseResult.endPointX;
          currentY = ellipseResult.endPointY;
          _isClosed = true;
          break;
        case PathCommandTypes.rRect:
          final RRectCommand rrectCommand = command;
          final ui.RRect rrect = rrectCommand.rrect;
          RRectMetricsRenderer(moveToCallback: (double x, double y) {
            currentX = x;
            currentY = y;
            _isClosed = true;
            haveSeenMoveTo = true;
          }, lineToCallback: (double x, double y) {
            lineToHandler(x, y);
          }, ellipseCallback: (double centerX,
              double centerY,
              double radiusX,
              double radiusY,
              double rotation,
              double startAngle,
              double endAngle,
              bool antiClockwise) {
            ellipseResult ??= _EllipseSegmentResult();
            _computeEllipseSegments(
                currentX,
                currentY,
                distance,
                centerX,
                centerY,
                startAngle,
                endAngle,
                rotation,
                radiusX,
                radiusY,
                antiClockwise,
                ellipseResult,
                _segments);
            distance = ellipseResult.distance;
            currentX = ellipseResult.endPointX;
            currentY = ellipseResult.endPointY;
          }).render(rrect);
          _isClosed = true;
          break;
        case PathCommandTypes.rect:
          final RectCommand rectCommand = command;
          final double x = rectCommand.x;
          final double y = rectCommand.y;
          final double width = rectCommand.width;
          final double height = rectCommand.height;
          currentX = x;
          currentY = y;
          lineToHandler(x + width, y);
          lineToHandler(x + width, y + height);
          lineToHandler(x, y + height);
          lineToHandler(x, y);
          _isClosed = true;
          break;
        default:
          throw UnimplementedError('Unknown path command $command');
      }
    }
    if (!_isClosed && forceClosed && _segments.isNotEmpty) {
      _PathSegment firstSegment = _segments.first;
      lineToHandler(firstSegment.points[0], firstSegment.points[1]);
    }
    _contourLength = distance;
  }

  static bool _tspanBigEnough(int tSpan) => (tSpan >> 10) != 0;

  static bool _cubicTooCurvy(double x0, double y0, double x1, double y1,
      double x2, double y2, double x3, double y3) {
    // Measure distance from start-end line at 1/3 and 2/3rds to control
    // points. If distance is less than _fTolerance we should continue
    // subdividing curve. Uses approx distance for speed.
    //
    // p1 = point 1/3rd between start,end points.
    final double p1x = (x0 * 2 / 3) + (x3 / 3);
    final double p1y = (y0 * 2 / 3) + (y3 / 3);
    if ((p1x - x1).abs() > _fTolerance) {
      return true;
    }
    if ((p1y - y1).abs() > _fTolerance) {
      return true;
    }
    // p2 = point 2/3rd between start,end points.
    final double p2x = (x0 / 3) + (x3 * 2 / 3);
    final double p2y = (y0 / 3) + (y3 * 2 / 3);
    if ((p2x - x2).abs() > _fTolerance) {
      return true;
    }
    if ((p2y - y2).abs() > _fTolerance) {
      return true;
    }
    return false;
  }

  // Recursively subdivides cubic and adds segments.
  static double _computeCubicSegments(
      double x0,
      double y0,
      double x1,
      double y1,
      double x2,
      double y2,
      double x3,
      double y3,
      double distance,
      int tMin,
      int tMax,
      List<_PathSegment> segments) {
    if (_tspanBigEnough(tMax - tMin) &&
        _cubicTooCurvy(x0, y0, x1, y1, x2, y2, x3, y3)) {
      // Chop cubic into two halves (De Cateljau's algorithm)
      // See https://en.wikipedia.org/wiki/De_Casteljau%27s_algorithm
      final double abX = (x0 + x1) / 2;
      final double abY = (y0 + y1) / 2;
      final double bcX = (x1 + x2) / 2;
      final double bcY = (y1 + y2) / 2;
      final double cdX = (x2 + x3) / 2;
      final double cdY = (y2 + y3) / 2;
      final double abcX = (abX + bcX) / 2;
      final double abcY = (abY + bcY) / 2;
      final double bcdX = (bcX + cdX) / 2;
      final double bcdY = (bcY + cdY) / 2;
      final double abcdX = (abcX + bcdX) / 2;
      final double abcdY = (abcY + bcdY) / 2;
      final int tHalf = (tMin + tMax) >> 1;
      distance = _computeCubicSegments(
          x0, y0, abX, abY, abcX, abcY, abcdX, abcdY, distance, tMin, tHalf, segments);
      distance = _computeCubicSegments(
          abcdX, abcdY, bcdX, bcdY, cdX, cdY, x3, y3, distance, tHalf, tMax, segments);
    } else {
      final double dx = x0 - x3;
      final double dy = y0 - y3;
      final double startToEndDistance = math.sqrt(dx * dx + dy * dy);
      final double prevDistance = distance;
      distance += startToEndDistance;
      if (distance > prevDistance) {
        segments.add(_PathSegment(PathCommandTypes.bezierCurveTo,
            distance, <double>[x0, y0, x1, y1, x2, y2, x3, y3]));
      }
    }
    return distance;
  }

  static bool _quadTooCurvy(
      double x0, double y0, double x1, double y1, double x2, double y2) {
    // (a/4 + b/2 + c/4) - (a/2 + c/2)  =  -a/4 + b/2 - c/4
    final double dx = (x1 / 2) - (x0 + x2) / 4;
    if (dx.abs() > _fTolerance) {
      return true;
    }
    final double dy = (y1 / 2) - (y0 + y2) / 4;
    if (dy.abs() > _fTolerance) {
      return true;
    }
    return false;
  }

  double _computeQuadSegments(double x0, double y0, double x1, double y1,
      double x2, double y2, double distance, int tMin, int tMax) {
    if (_tspanBigEnough(tMax - tMin) && _quadTooCurvy(x0, y0, x1, y1, x2, y2)) {
      final double p01x = (x0 + x1) / 2;
      final double p01y = (y0 + y1) / 2;
      final double p12x = (x1 + x2) / 2;
      final double p12y = (y1 + y2) / 2;
      final double p012x = (p01x + p12x) / 2;
      final double p012y = (p01y + p12y) / 2;
      final int tHalf = (tMin + tMax) >> 1;
      distance = _computeQuadSegments(
          x0, y0, p01x, p01y, p012x, p012y, distance, tMin, tHalf);
      distance = _computeQuadSegments(
          p012x, p012y, p12x, p12y, x2, y2, distance, tMin, tHalf);
    } else {
      final double dx = x0 - x2;
      final double dy = y0 - y2;
      final double startToEndDistance = math.sqrt(dx * dx + dy * dy);
      final double prevDistance = distance;
      distance += startToEndDistance;
      if (distance > prevDistance) {
        _segments.add(_PathSegment(PathCommandTypes.quadraticCurveTo,
            distance, <double>[x0, y0, x1, y1, x2, y2]));
      }
    }
    return distance;
  }

  // Create segments by converting arc to cubics.
  // See http://www.w3.org/TR/SVG/implnote.html#ArcConversionEndpointToCenter.
  static void _computeEllipseSegments(
      double startX,
      double startY,
      double distance,
      double cx,
      double cy,
      double startAngle,
      double endAngle,
      double rotation,
      double radiusX,
      double radiusY,
      bool anticlockwise,
      _EllipseSegmentResult result,
      List<_PathSegment> segments) {
    final double endX = cx + (radiusX * math.cos(endAngle));
    final double endY = cy + (radiusY * math.sin(endAngle));
    result.endPointX = endX;
    result.endPointY = endY;
    // Check for http://www.w3.org/TR/SVG/implnote.html#ArcOutOfRangeParameters
    // Treat as line segment from start to end if arc has zero radii.
    // If start and end point are the same treat as zero length path.
    if ((radiusX == 0 || radiusY == 0) || (startX == endX && startY == endY)) {
      result.distance = distance;
      return;
    }
    final double rxAbs = radiusX.abs();
    final double ryAbs = radiusY.abs();

    final double theta1 = startAngle;
    final double theta2 = endAngle;
    final double thetaArc = theta2 - theta1;

    // Add 0.01f to make sure we have enough segments when thetaArc is close
    // to pi/2.
    final int numSegments = (thetaArc / ((math.pi / 2.0) + 0.01)).abs().ceil();
    double x0 = startX;
    double y0 = startY;
    for (int segmentIndex = 0; segmentIndex < numSegments; segmentIndex++) {
      final double startTheta =
          theta1 + (segmentIndex * thetaArc / numSegments);
      final double endTheta =
          theta1 + ((segmentIndex + 1) * thetaArc / numSegments);
      final double t = (4.0 / 3.0) * math.tan((endTheta - startTheta) / 4);
      if (!t.isFinite) {
        result.distance = distance;
        return;
      }
      final double sinStartTheta = math.sin(startTheta);
      final double cosStartTheta = math.cos(startTheta);
      final double sinEndTheta = math.sin(endTheta);
      final double cosEndTheta = math.cos(endTheta);

      // Compute cubic segment start, control point and end (target).
      final double p1x = rxAbs * (cosStartTheta - t * sinStartTheta) + cx;
      final double p1y = ryAbs * (sinStartTheta + t * cosStartTheta) + cy;
      final double targetPointX = rxAbs * cosEndTheta + cx;
      final double targetPointY = ryAbs * sinEndTheta + cy;
      final double p2x = targetPointX + rxAbs * (t * sinEndTheta);
      final double p2y = targetPointY + ryAbs * (-t * cosEndTheta);

      distance = _computeCubicSegments(x0, y0, p1x, p1y, p2x, p2y, targetPointX,
          targetPointY, distance, 0, _kMaxTValue, segments);
      x0 = targetPointX;
      y0 = targetPointY;
    }
    result.distance = distance;
  }
}

/// Tracks iteration from one segment of a path to the next for measurement.
class SurfacePathMetricIterator implements Iterator<ui.PathMetric> {
  SurfacePathMetricIterator._(this._pathMeasure) : assert(_pathMeasure != null);

  SurfacePathMetric _pathMetric;
  _SurfacePathMeasure _pathMeasure;
  bool _firstTime = true;

  @override
  SurfacePathMetric get current => _pathMetric;

  @override
  bool moveNext() {
    if (_pathMeasure._nextContour()) {
      _pathMetric = SurfacePathMetric._(_pathMeasure);
      return true;
    }
    _pathMetric = null;
    return false;
  }
}

// Maximum range value used in curve subdivision using Casteljau algorithm.
const int _kMaxTValue = 0x3FFFFFFF;
// Distance at which we stop subdividing cubic and quadratic curves.
const double _fTolerance = 0.5;

/// Utilities for measuring a [Path] and extracting sub-paths.
///
/// Iterate over the object returned by [Path.computeMetrics] to obtain
/// [PathMetric] objects. Callers that want to randomly access elements or
/// iterate multiple times should use `path.computeMetrics().toList()`, since
/// [PathMetrics] does not memoize.
///
/// Once created, the metrics are only valid for the path as it was specified
/// when [Path.computeMetrics] was called. If additional contours are added or
/// any contours are updated, the metrics need to be recomputed. Previously
/// created metrics will still refer to a snapshot of the path at the time they
/// were computed, rather than to the actual metrics for the new mutations to
/// the path.
///
/// Implementation is based on
/// https://github.com/google/skia/blob/master/src/core/SkContourMeasure.cpp
/// to maintain consistency with native platforms.
class SurfacePathMetric implements ui.PathMetric {

  SurfacePathMetric._(this._measure)
      : assert(_measure != null),
        length = _measure.length(_measure.currentContourIndex),
        isClosed = _measure.isClosed(_measure.currentContourIndex),
        contourIndex = _measure.currentContourIndex;

  /// Return the total length of the current contour.
  @override
  final double length;

  /// Whether the contour is closed.
  ///
  /// Returns true if the contour ends with a call to [Path.close] (which may
  /// have been implied when using methods like [Path.addRect]) or if
  /// `forceClosed` was specified as true in the call to [Path.computeMetrics].
  /// Returns false otherwise.
  final bool isClosed;

  /// The zero-based index of the contour.
  ///
  /// [Path] objects are made up of zero or more contours. The first contour is
  /// created once a drawing command (e.g. [Path.lineTo]) is issued. A
  /// [Path.moveTo] command after a drawing command may create a new contour,
  /// although it may not if optimizations are applied that determine the move
  /// command did not actually result in moving the pen.
  ///
  /// This property is only valid with reference to its original iterator and
  /// the contours of the path at the time the path's metrics were computed. If
  /// additional contours were added or existing contours updated, this metric
  /// will be invalid for the current state of the path.
  final int contourIndex;

  final _SurfacePathMeasure _measure;

  /// Computes the position of the current contour at the given offset, and the
  /// angle of the path at that point.
  ///
  /// For example, calling this method with a distance of 1.41 for a line from
  /// 0.0,0.0 to 2.0,2.0 would give a point 1.0,1.0 and the angle 45 degrees
  /// (but in radians).
  ///
  /// Returns null if the contour has zero [length].
  ///
  /// The distance is clamped to the [length] of the current contour.
  @override
  ui.Tangent getTangentForOffset(double distance) {
    return _measure.getTangentForOffset(contourIndex, distance);
  }

  /// Given a start and stop distance, return the intervening segment(s).
  ///
  /// `start` and `end` are pinned to legal values (0..[length])
  /// Returns null if the segment is 0 length or `start` > `stop`.
  /// Begin the segment with a moveTo if `startWithMoveTo` is true.
  ui.Path extractPath(double start, double end, {bool startWithMoveTo = true}) {
    return _measure.extractPath(contourIndex, start, end, startWithMoveTo: startWithMoveTo);
  }

  @override
  String toString() => 'PathMetric';
}

class _EllipseSegmentResult {
  double endPointX;
  double endPointY;
  double distance;
  _EllipseSegmentResult();
}

// Given a vector dx, dy representing slope, normalize and return as [ui.Offset].
ui.Offset _normalizeSlope(double dx, double dy) {
  final double length = math.sqrt(dx * dx + dy * dy);
  return length < kEpsilon ? ui.Offset(0.0, 0.0) : ui.Offset(dx / length, dy / length);
}

class _PathSegment {
  _PathSegment(this.segmentType, this.distance, this.points);

  final int segmentType;
  final double distance;
  final List<double> points;

  ui.Tangent computeTangent(double t) {
    switch (segmentType) {
      case PathCommandTypes.lineTo:
        // Simple line. Position is simple interpolation from start to end point.
        final double xAtDistance = (points[2] * t) + (points[0] * (1.0 - t));
        final double yAtDistance = (points[3] * t) + (points[1] * (1.0 - t));
        return ui.Tangent(ui.Offset(xAtDistance, yAtDistance),
            _normalizeSlope(points[2] - points[0], points[3] - points[1]));
      case PathCommandTypes.bezierCurveTo:
        return tangentForCubicAt(t, points[0], points[1], points[2], points[3], points[4], points[5], points[6], points[7]);
      case PathCommandTypes.quadraticCurveTo:
        return tangentForQuadAt(t, points[0], points[1], points[2], points[3], points[4], points[5]);
      default:
        throw UnsupportedError('Invalid segment type');
    }
  }

  ui.Tangent tangentForQuadAt(double t, double x0, double y0, double x1, double y1, double x2, double y2) {
    assert(t >= 0 && t <= 1);
    final _SkQuadCoefficients _quadEval = _SkQuadCoefficients(x0, y0, x1, y1, x2, y2);
    final ui.Offset pos = ui.Offset(_quadEval.evalX(t), _quadEval.evalY(t));
    // Derivative of quad curve is 2(b - a + (a - 2b + c)t).
    // If control point is at start or end point, this yields 0 for t = 0 and
    // t = 1. In that case use the quad end points to compute tangent instead
    // of derivative.
    final ui.Offset tangentVector =
     ((t == 0 && x0 == x1 && y0 == y1) || (t == 0 && x1 == x2 && y1 == y2))
      ? _normalizeSlope(x2 - x0, y2 - y0)
      : _normalizeSlope(2 * ((x2 - x0) * t + (x1 - x0)), 2 * ((y2 - y0) * t + (y1 - y0)));
    return ui.Tangent(pos, tangentVector);
  }

  ui.Tangent tangentForCubicAt(double t, double x0, double y0, double x1, double y1, double x2, double y2, double x3, double y3) {
    assert(t >= 0 && t <= 1);
    final _SkCubicCoefficients _cubicEval = _SkCubicCoefficients(x0, y0, x1, y1, x2, y2, x3, y3);
    final ui.Offset pos = ui.Offset(_cubicEval.evalX(t), _cubicEval.evalY(t));
    // Derivative of cubic is zero when t = 0 or 1 and adjacent control point
    // is on the start or end point of curve. Use the other control point
    // to compute the tangent or if both control points are on end points
    // use end points for tangent.
    final bool tAtZero = t == 0;
    ui.Offset tangentVector;
    if ((tAtZero && x0 == x1 && y0 == y1) || (t == 1 && x2 == x3 && y2 == y3)) {
      double dx = tAtZero ? x2 - x0 : x3 - x1;
      double dy = tAtZero ? y2 - y0 : y3 - y1;
      if (dx == 0 && dy == 0) {
        dx = x3 - x0;
        dy = y3 - y0;
      }
      tangentVector = _normalizeSlope(dx, dy);
    } else {
      final double ax = x3 + (3 * (x1 - x2)) - x0;
      final double ay = y3 + (3 * (y1 - y2)) - y0;
      final double bx = 2 * (x2 - (2 * x1) + x0);
      final double by = 2 * (y2 - (2 * y1) + y0);
      final double cx = x1 - x0;
      final double cy = y1 - y0;
      final double tx = (ax * t + bx) * t + cx;
      final double ty = (ay * t + by) * t + cy;
      tangentVector = _normalizeSlope(tx, ty);
    }
  }
}

/// Evaluates A * t^2 + B * t + C = 0 for quadratic curve.
class _SkQuadCoefficients {
  _SkQuadCoefficients(double x0, double y0, double x1, double y1, double x2, double y2)
      : cx = x0, cy = y0, bx = 2 * (x1 - x0), by = 2 * (y1 - y0),
        ax = x2 - (2 * x1) + x0, ay = y2 - (2 * y1) + y0;
  final double ax, ay, bx, by, cx, cy;

  double evalX(double t) => (ax * t + bx) * t + cx;

  double evalY(double t) => (ay * t + by) * t + cy;
}

// Evaluates A * t^3 + B * t^2 + Ct + D = 0 for cubic curve.
class _SkCubicCoefficients {
  final double ax, ay, bx, by, cx, cy, dx, dy;
  _SkCubicCoefficients(double x0, double y0, double x1, double y1, double x2, double y2, double x3, double y3) :
      ax = x3 + (3 * (x1 - x2)) - x0,
      ay = y3 + (3 * (y1 - y2)) - y0,
      bx = 3 * (x2 - (2 * x1) + x0),
      by = 3 * (y2 - (2 * y1) + y0),
      cx = 3 * (x1 - x0),
      cy = 3 * (y1 - y0),
      dx = x0,
      dy = y0;

  double evalX(double t) => (((ax * t + bx) * t) + cx) * t + dx;

  double evalY(double t) => (((ay * t + by) * t) + cy) * t + dy;
}
