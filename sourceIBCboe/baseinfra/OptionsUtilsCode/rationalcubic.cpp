//
// This source code resides at www.jaeckel.org/LetsBeRational.7z .
//
// ======================================================================================
// Copyright © 2013-2014 Peter Jäckel.
//
// Permission to use, copy, modify, and distribute this software is freely granted,
// provided that this notice is preserved.
//
// WARRANTY DISCLAIMER
// The Software is provided "as is" without warranty of any kind, either express or implied,
// including without limitation any implied warranties of condition, uninterrupted use,
// merchantability, fitness for a particular purpose, or non-infringement.
// ======================================================================================
//

#include "baseinfra/OptionsUtils/rationalcubic.hpp"

#include <cfloat>
#include <cmath>
#include <algorithm>

// Based on
//
//    “Shape preserving piecewise rational interpolation”, R. Delbourgo, J.A. Gregory - SIAM journal on scientific and
//    statistical computing, 1985 - SIAM.
//    http://dspace.brunel.ac.uk/bitstream/2438/2200/1/TR_10_83.pdf  [caveat emptor: there are some typographical errors
//    in that draft version]
//

namespace HFSAT {

const double minimum_rational_cubic_control_parameter_value = -(1 - sqrt(DBL_EPSILON));
const double maximum_rational_cubic_control_parameter_value = 2 / (DBL_EPSILON * DBL_EPSILON);
inline bool is_zero(double x) { return fabs(x) < DBL_MIN; }

double RationalCubicInterpolation(double x, double x_l, double x_r, double y_l, double y_r, double d_l, double d_r,
                                  double r) {
  const double h = (x_r - x_l);
  if (fabs(h) <= 0) return 0.5 * (y_l + y_r);
  // r should be greater than -1. We do not use  assert(r > -1)  here in order to allow values such as NaN to be
  // propagated as they should.
  const double t = (x - x_l) / h;
  if (!(r >= maximum_rational_cubic_control_parameter_value)) {
    const double t = (x - x_l) / h, omt = 1 - t, t2 = t * t, omt2 = omt * omt;
    // Formula (2.4) divided by formula (2.5)
    return (y_r * t2 * t + (r * y_r - h * d_r) * t2 * omt + (r * y_l + h * d_l) * t * omt2 + y_l * omt2 * omt) /
           (1 + (r - 3) * t * omt);
  }
  // Linear interpolation without over-or underflow.
  return y_r * t + y_l * (1 - t);
}

double RationalCubicControlParameterToFitSecondDerivativeAtLeftSide(double x_l, double x_r, double y_l, double y_r,
                                                                    double d_l, double d_r,
                                                                    double second_derivative_l) {
  const double h = (x_r - x_l), numerator = 0.5 * h * second_derivative_l + (d_r - d_l);
  if (is_zero(numerator)) return 0;
  const double denominator = (y_r - y_l) / h - d_l;
  if (is_zero(denominator))
    return numerator > 0 ? maximum_rational_cubic_control_parameter_value
                         : minimum_rational_cubic_control_parameter_value;
  return numerator / denominator;
}

double RationalCubicControlParameterToFitSecondDerivativeAtRightSide(double x_l, double x_r, double y_l, double y_r,
                                                                     double d_l, double d_r,
                                                                     double second_derivative_r) {
  const double h = (x_r - x_l), numerator = 0.5 * h * second_derivative_r + (d_r - d_l);
  if (is_zero(numerator)) return 0;
  const double denominator = d_r - (y_r - y_l) / h;
  if (is_zero(denominator))
    return numerator > 0 ? maximum_rational_cubic_control_parameter_value
                         : minimum_rational_cubic_control_parameter_value;
  return numerator / denominator;
}

double MinimumRationalCubicControlParameter(double d_l, double d_r, double s,
                                            bool preferShapePreservationOverSmoothness) {
  const bool monotonic = d_l * s >= 0 && d_r * s >= 0, convex = d_l <= s && s <= d_r, concave = d_l >= s && s >= d_r;
  if (!monotonic && !convex && !concave)  // If 3==r_non_shape_preserving_target, this means revert to standard cubic.
    return minimum_rational_cubic_control_parameter_value;
  const double d_r_m_d_l = d_r - d_l, d_r_m_s = d_r - s, s_m_d_l = s - d_l;
  double r1 = -DBL_MAX, r2 = r1;
  // If monotonicity on this interval is possible, set r1 to satisfy the monotonicity condition (3.8).
  if (monotonic) {
    if (!is_zero(s))                                 // (3.8), avoiding division by zero.
      r1 = (d_r + d_l) / s;                          // (3.8)
    else if (preferShapePreservationOverSmoothness)  // If division by zero would occur, and shape preservation is
                                                     // preferred, set value to enforce linear interpolation.
      r1 = maximum_rational_cubic_control_parameter_value;  // This value enforces linear interpolation.
  }
  if (convex || concave) {
    if (!(is_zero(s_m_d_l) || is_zero(d_r_m_s)))  // (3.18), avoiding division by zero.
      r2 = std::max(fabs(d_r_m_d_l / d_r_m_s), fabs(d_r_m_d_l / s_m_d_l));
    else if (preferShapePreservationOverSmoothness)
      r2 = maximum_rational_cubic_control_parameter_value;  // This value enforces linear interpolation.
  } else if (monotonic && preferShapePreservationOverSmoothness)
    r2 = maximum_rational_cubic_control_parameter_value;  // This enforces linear interpolation along segments that are
                                                          // inconsistent with the slopes on the boundaries, e.g., a
                                                          // perfectly horizontal segment that has negative slopes on
                                                          // either edge.
  return std::max(minimum_rational_cubic_control_parameter_value, std::max(r1, r2));
}

double ConvexRationalCubicControlParameterToFitSecondDerivativeAtLeftSide(double x_l, double x_r, double y_l,
                                                                          double y_r, double d_l, double d_r,
                                                                          double second_derivative_l,
                                                                          bool preferShapePreservationOverSmoothness) {
  const double r =
      RationalCubicControlParameterToFitSecondDerivativeAtLeftSide(x_l, x_r, y_l, y_r, d_l, d_r, second_derivative_l);
  const double r_min =
      MinimumRationalCubicControlParameter(d_l, d_r, (y_r - y_l) / (x_r - x_l), preferShapePreservationOverSmoothness);
  return std::max(r, r_min);
}

double ConvexRationalCubicControlParameterToFitSecondDerivativeAtRightSide(double x_l, double x_r, double y_l,
                                                                           double y_r, double d_l, double d_r,
                                                                           double second_derivative_r,
                                                                           bool preferShapePreservationOverSmoothness) {
  const double r =
      RationalCubicControlParameterToFitSecondDerivativeAtRightSide(x_l, x_r, y_l, y_r, d_l, d_r, second_derivative_r);
  const double r_min =
      MinimumRationalCubicControlParameter(d_l, d_r, (y_r - y_l) / (x_r - x_l), preferShapePreservationOverSmoothness);
  return std::max(r, r_min);
}
}
