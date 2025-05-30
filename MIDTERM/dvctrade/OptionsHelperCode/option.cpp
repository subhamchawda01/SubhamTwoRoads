/**
 *       \file OptionTools/Greeks/option.hpp
 *
 *        \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
 *             Address:
 *                   Suite No 353, Evoma, #14, Bhattarhalli,
 *                   Old Madras Road, Near Garden City College,
 *                   KR Puram, Bangalore 560049, India
 *                   +91 80 4190 355
 **/

#include "dvctrade/OptionsHelper/option.hpp"
#include <stdio.h>

namespace HFSAT {

void Option::computeGreeks() {
  //    printf( " stdev = %f, isqrtT = %f, fp = %f, sp = %f, ir = %f \n", stdev_, sqrtT_, future_price_, strike_price_,
  //    interest_rate_ );

  if ((future_price_ < 0) || (stdev_ < 0)) {
    delta_ = 0;
    gamma_ = 0;
    theta_ = 0;
    vega_ = 0;
    rho_ = 0;
    return;
  }

  if (option_type_ == CALL) {
    delta_ = df_ * (Nd1);
    gamma_ = df_ * nd1 / (future_price_ * sigmasqrtT_);
    theta_ = df_ * ((interest_rate_ * future_price_ * Nd1) - (interest_rate_ * strike_price_ * Nd2) -
                    (future_price_ * nd1 * stdev_ / (2 * sqrtT_)));

    vega_ = df_ * future_price_ * sqrtT_ * nd1;
    rho_ = df_ * strike_price_ * years_to_expiry_ * Nd2;
    vanna_ = -df_ * nd1 * d2 / stdev_;
    vomma_ = df_ * future_price_ * nd1 * sqrtT_ * d1 * d2 / stdev_;
    charm_ = df_ * ((interest_rate_ * Nd1) +
                    (nd1 * (log(future_price_ / strike_price_) / (stdev_ * 2 * pow(years_to_expiry_, 3 / 2)) -
                            (0.25 * stdev_ / sqrtT_))));

  } else {
    delta_ = df_ * (Nd1 - 1);
    gamma_ = df_ * nd1 / (future_price_ * sigmasqrtT_);
    theta_ = df_ * ((interest_rate_ * future_price_ * (Nd1 - 1)) - (interest_rate_ * strike_price_ * (Nd2 - 1)) -
                    (future_price_ * nd1 * stdev_ / (2 * sqrtT_)));

    vega_ = df_ * future_price_ * sqrtT_ * nd1;
    rho_ = df_ * strike_price_ * years_to_expiry_ * (Nd2 - 1);
    vanna_ = -df_ * nd1 * d2 / stdev_;
    vomma_ = df_ * future_price_ * nd1 * sqrtT_ * d1 * d2 / stdev_;
    charm_ = df_ * ((interest_rate_ * (Nd1 - 1)) +
                    (nd1 * (log(future_price_ / strike_price_) / (stdev_ * 2 * pow(years_to_expiry_, 3 / 2)) -
                            (0.25 * stdev_ / sqrtT_))));
  }
  theta_per_day_ = theta_ / 365;  // change if we move to business days
}

void Option::SetImpliedVolUsingOptionPx(const double t_option_price_) { SetOptionPrice(t_option_price_); }

void Option::ComputeDelta() {
  sigmasqrtT_ = stdev_ * sqrtT_;
  d1 = std::log(future_price_ / strike_price_) / sigmasqrtT_ + 0.5 * sigmasqrtT_;
  Nd1 = (*cdf_)(d1);
  if (option_type_ == CALL) {
    delta_ = df_ * (Nd1);
  } else {
    delta_ = df_ * (Nd1 - 1);
  }
}

void Option::ComputeGamma() {
  sigmasqrtT_ = stdev_ * sqrtT_;
  d1 = std::log(future_price_ / strike_price_) / sigmasqrtT_ + 0.5 * sigmasqrtT_;
  nd1 = cdf_->derivative(d1);
  gamma_ = df_ * nd1 / (future_price_ * sigmasqrtT_);
}

void Option::ComputeVega() {
  sigmasqrtT_ = stdev_ * sqrtT_;
  d1 = std::log(future_price_ / strike_price_) / sigmasqrtT_ + 0.5 * sigmasqrtT_;
  nd1 = cdf_->derivative(d1);
  vega_ = df_ * future_price_ * sqrtT_ * nd1;
}

void Option::ComputeTheta() {
  sigmasqrtT_ = stdev_ * sqrtT_;
  d1 = std::log(future_price_ / strike_price_) / sigmasqrtT_ + 0.5 * sigmasqrtT_;
  d2 = d1 - sigmasqrtT_;
  Nd1 = (*cdf_)(d1);
  Nd2 = (*cdf_)(d2);
  nd1 = cdf_->derivative(d1);
  nd2 = cdf_->derivative(d2);

  if (option_type_ == CALL) {
    theta_ = df_ * ((interest_rate_ * future_price_ * Nd1) - (interest_rate_ * strike_price_ * Nd2) -
                    (future_price_ * nd1 * stdev_ / (2 * sqrtT_)));
  } else {
    theta_ = df_ * ((interest_rate_ * future_price_ * (Nd1 - 1)) - (interest_rate_ * strike_price_ * (Nd2 - 1)) -
                    (future_price_ * nd1 * stdev_ / (2 * sqrtT_)));
  }
}
}
