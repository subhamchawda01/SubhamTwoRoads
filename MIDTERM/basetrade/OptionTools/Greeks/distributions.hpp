/**
 *      \file OptionTools/Greeks/distribution.hpp
 *
 *         \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
 *              Address:
 *                      Suite No 353, Evoma, #14, Bhattarhalli,
 *                      Old Madras Road, Near Garden City College,
 *                      KR Puram, Bangalore 560049, India
 *                      +91 80 4190 3551
 **/
#pragma once
#include <cmath>
#include "OptionTools/Greeks/errorfunction.hpp"

#define INV_SQRT_2 0.7071067811865475244008443621048490392848359376887
#define INV_SQRT_PI 0.564189583547756286948
#define INV_SQRT_2PI 0.3989422804014326779398898791969837179346539135327

namespace HFSAT {
class NormalDistribution {
 public:
  NormalDistribution(double t_average_ = 0, double t_stdev_ = 1);

  virtual ~NormalDistribution() {}

  double operator()(double x) const;
  double derivative(double x) const;

 private:
  double average_;
  double stdev_;
  double normalizationFactor_;
  double derNormalizationFactor_;
  double denominator_;
};

class CumulativeNormalDistribution {
 public:
  CumulativeNormalDistribution(double t_average_ = 0, double t_stdev_ = 1);

  virtual ~CumulativeNormalDistribution() {}

  double operator()(double x) const;
  double derivative(double x) const;

 private:
  NormalDistribution gaussian_;
  ErrorFunction err_func_;
  double average_;
  double stdev_;
};

inline NormalDistribution::NormalDistribution(double t_average_, double t_stdev_) {
  average_ = t_average_;
  stdev_ = t_stdev_;
  normalizationFactor_ = INV_SQRT_2PI / stdev_;
  denominator_ = 2 * stdev_ * stdev_;
  derNormalizationFactor_ = 1 / (stdev_ * stdev_);
}

inline double NormalDistribution::operator()(double x) const {
  double deltax_ = (x - average_);
  double exponent_ = -(deltax_ * deltax_) / denominator_;
  return normalizationFactor_ * std::exp(exponent_);
}

inline double NormalDistribution::derivative(double x) const {
  return ((*this)(x) * (average_ - x) * derNormalizationFactor_);
}

inline CumulativeNormalDistribution::CumulativeNormalDistribution(double t_average_, double t_stdev_) {
  average_ = t_average_;
  stdev_ = t_stdev_;
}

inline double CumulativeNormalDistribution::derivative(double x) const {
  double xmod_ = (x - average_) / stdev_;
  return gaussian_(xmod_) / stdev_;
}
}
