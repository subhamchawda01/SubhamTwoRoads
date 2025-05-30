/**
 *        \file OptionTools/Greeks/distribution.cpp
 *
 *         \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
 *              Address:
 *                    Suite No 353, Evoma, #14, Bhattarhalli,
 *                    Old Madras Road, Near Garden City College,
 *                    KR Puram, Bangalore 560049, India
 *                    +91 80 4190 3551
 **/

#include "baseinfra/OptionsUtils/distributions.hpp"
#define OP_EPSILON std::numeric_limits<double>::epsilon()
#define OP_MAX std::numeric_limits<double>::max()

namespace HFSAT {
double CumulativeNormalDistribution::operator()(double z) const {
  z = (z - average_) / stdev_;

  double result = 0.5 * (1.0 + err_func_(z * INV_SQRT_2));
  if (result <= 1e-8) {  // todo: investigate the threshold level
    // Asymptotic expansion for very negative z following (26.2.12)
    // on page 408 in M. Abramowitz and A. Stegun,
    // Pocketbook of Mathematical Functions, ISBN 3-87144818-4.
    double sum = 1.0, zsqr = z * z, i = 1.0, g = 1.0, x, y, a = OP_MAX, lasta;
    do {
      lasta = a;
      x = (4.0 * i - 3.0) / zsqr;
      y = x * ((4.0 * i - 1) / zsqr);
      a = g * (x - y);
      sum -= a;
      g *= y;
      ++i;
      a = std::fabs(a);
    } while (lasta > a && a >= std::fabs(sum * OP_EPSILON));
    result = -gaussian_(z) / z * sum;
  }
  return result;
}
}
