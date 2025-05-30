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
#ifndef RATIONAL_CUBIC_H
#define RATIONAL_CUBIC_H

// Based on
//
//    “Shape preserving piecewise rational interpolation”, R. Delbourgo, J.A. Gregory - SIAM journal on scientific and
//    statistical computing, 1985 - SIAM.
//    http://dspace.brunel.ac.uk/bitstream/2438/2200/1/TR_10_83.pdf  [caveat emptor: there are some typographical errors
//    in that draft version]
//

namespace HFSAT {

double RationalCubicInterpolation(double x, double x_l, double x_r, double y_l, double y_r, double d_l, double d_r,
                                  double r);

double RationalCubicControlParameterToFitSecondDerivativeAtLeftSide(double x_l, double x_r, double y_l, double y_r,
                                                                    double d_l, double d_r, double second_derivative_l);

double RationalCubicControlParameterToFitSecondDerivativeAtRightSide(double x_l, double x_r, double y_l, double y_r,
                                                                     double d_l, double d_r,
                                                                     double second_derivative_r);

double MinimumRationalCubicControlParameter(double d_l, double d_r, double s,
                                            bool preferShapePreservationOverSmoothness);

double ConvexRationalCubicControlParameterToFitSecondDerivativeAtLeftSide(double x_l, double x_r, double y_l,
                                                                          double y_r, double d_l, double d_r,
                                                                          double second_derivative_l,
                                                                          bool preferShapePreservationOverSmoothness);

double ConvexRationalCubicControlParameterToFitSecondDerivativeAtRightSide(double x_l, double x_r, double y_l,
                                                                           double y_r, double d_l, double d_r,
                                                                           double second_derivative_r,
                                                                           bool preferShapePreservationOverSmoothness);
}

#endif  // RATIONAL_CUBIC_H
