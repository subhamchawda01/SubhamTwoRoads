/**
 *        \file OptionTools/Greeks/errorfunction.hpp
 *
 *          \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
 *                 Address:
 *                         Suite No 353, Evoma, #14, Bhattarhalli,
 *                         Old Madras Road, Near Garden City College,
 *                         KR Puram, Bangalore 560049, India
 *                         +91 80 4190 3551
 **/

#pragma once
#include <limits>
// Used to compute values of CDF function.
namespace HFSAT {
class ErrorFunction {
 public:
  ErrorFunction(){};
  virtual ~ErrorFunction(){};

  double operator()(double x) const;

 private:
  static const double tiny, one, erx, efx, efx8;
  static const double pp0, pp1, pp2, pp3, pp4;
  static const double qq1, qq2, qq3, qq4, qq5;
  static const double pa0, pa1, pa2, pa3, pa4, pa5, pa6;
  static const double qa1, qa2, qa3, qa4, qa5, qa6;
  static const double ra0, ra1, ra2, ra3, ra4, ra5, ra6, ra7;
  static const double sa1, sa2, sa3, sa4, sa5, sa6, sa7, sa8;
  static const double rb0, rb1, rb2, rb3, rb4, rb5, rb6;
  static const double sb1, sb2, sb3, sb4, sb5, sb6, sb7;
};
}
