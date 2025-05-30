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
#ifndef LETS_BE_RATIONAL_H
#define LETS_BE_RATIONAL_H

#define ENABLE_SWITCHING_THE_OUTPUT_TO_ITERATION_COUNT
#define ENABLE_CHANGING_THE_HOUSEHOLDER_METHOD_ORDER

namespace HFSAT {

double SetImpliedVolatilityMaximumIterations(double n);
double SetImpliedVolatilityOutputType(double k);
double SetImpliedVolatilityHouseholderMethodOrder(double m);
double NormalisedBlackCall(double x, double s);
double NormalisedVega(double x, double s);
double NormalisedBlack(double x, double s, double q /* q=±1 */);
double Black(double F, double K, double sigma, double T, double q /* q=±1 */);
double NormalisedImpliedVolatilityFromTransformedRationalGuessWithLimitedIterations(double beta, double x,
                                                                                    double q /* q=±1 */, int N);
double NormalisedImpliedVolatilityFromTransformedRationalGuess(double beta, double x, double q /* q=±1 */);
double ImpliedVolatilityFromTransformedRationalGuessWithLimitedIterations(double price, double F, double K, double T,
                                                                          double q /* q=±1 */, int N);
double ImpliedVolatilityFromTransformedRationalGuess(double price, double F, double K, double T, double q /* q=±1 */);
}

#endif  // NORMAL_DISTRIBUTION_H
