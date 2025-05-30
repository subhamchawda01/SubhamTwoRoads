/**
 *      \file OptionTools/Greeks/option.hpp
 *
 *      \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
 *          Address:
 *                 Suite No 353, Evoma, #14, Bhattarhalli,
 *                 Old Madras Road, Near Garden City College,
 *                 KR Puram, Bangalore 560049, India
 *                 +91 80 4190 355
 **/

#pragma once
#include "baseinfra/OptionsUtils/distributions.hpp"
#include <stdint.h>
#include "baseinfra/OptionsUtils/lets_be_rational.hpp"
#include "baseinfra/OptionsUtils/option_object.hpp"

namespace HFSAT {
#define NUM_ITER_Z 10
// enum OptionType_t { PUT = -1, CALL = 1 };

class Option {
 public:
  Option(const OptionType_t t_option_type_, const double t_strike_price_, const double t_future_price_,
         const double t_time_to_expiry_, const double t_stdev_, const double t_interest_rate_,
         bool t_is_years_ = false) {
    double t_years_to_expiry_ = t_time_to_expiry_;
    if (!t_is_years_) {
      t_years_to_expiry_ = t_time_to_expiry_ / 365.0;
    }
    cdf_ = new CumulativeNormalDistribution();
    SetParameters(t_option_type_, t_strike_price_, t_future_price_, t_years_to_expiry_, t_stdev_, t_interest_rate_, -1);
  }

  Option(const double t_option_price_, const OptionType_t t_option_type_, const double t_strike_price_,
         const double t_future_price_, const double t_time_to_expiry_, const double t_interest_rate_,
         bool t_is_years_ = false) {
    double t_years_to_expiry_ = t_time_to_expiry_;
    if (!t_is_years_) {
      t_years_to_expiry_ = t_time_to_expiry_ / 365.0;
    }
    cdf_ = new CumulativeNormalDistribution();
    SetParameters(t_option_type_, t_strike_price_, t_future_price_, t_years_to_expiry_, -1, t_interest_rate_,
                  t_option_price_);
  }

  void SetParameters(const OptionType_t t_option_type_, const double t_strike_price_, const double t_future_price_,
                     const double t_years_to_expiry_, const double t_stdev_, const double t_interest_rate_,
                     const double t_option_price_) {
    option_type_ = t_option_type_;
    strike_price_ = t_strike_price_;
    future_price_ = t_future_price_;
    years_to_expiry_ = t_years_to_expiry_;
    stdev_ = t_stdev_;
    interest_rate_ = t_interest_rate_;
    sqrtT_ = sqrt(years_to_expiry_);  // calendar day computation for now .. change to business days if needed
    // handle boundary cases - TODO
    option_price_ = t_option_price_;
    df_ = exp(-interest_rate_ * years_to_expiry_);
    days_to_expiry_ = t_years_to_expiry_ * 365;

    if (stdev_ < 0) {
      initialize_impliedvol();
    }

    if (option_price_ < 0) {
      initialize_price();
    }

    time_to_maturity_changed_ = true;
    future_price_changed_ = true;
    implied_vol_changed_ = true;
    strike_changed_ = true;

    SetHelperParameters();
  }

  void SetHelperParameters() {
    if (time_to_maturity_changed_) {
      sqrtT_ = sqrt(years_to_expiry_);  // calendar day computation for now .. change to business days if needed
      sigmasqrtT_ = stdev_ * sqrtT_;
      df_ = exp(-interest_rate_ * years_to_expiry_);
    }

    if (time_to_maturity_changed_ || future_price_changed_ || implied_vol_changed_ || strike_changed_) {
      d1 = std::log(future_price_ / strike_price_) / sigmasqrtT_ + 0.5 * sigmasqrtT_;
      d2 = d1 - sigmasqrtT_;
      Nd1 = (*cdf_)(d1);
      Nd2 = (*cdf_)(d2);
      nd1 = cdf_->derivative(d1);
      nd2 = cdf_->derivative(d2);
    }

    time_to_maturity_changed_ = false;
    future_price_changed_ = false;
    implied_vol_changed_ = false;
    strike_changed_ = false;
  }

  void initialize_impliedvol() {
    if (stdev_ < 0) {
      double beta = option_price_;
      beta *= exp(interest_rate_ * years_to_expiry_);
      beta /= sqrt(future_price_ * strike_price_);

      double norm_strike_price = log(future_price_ / strike_price_);
      int q = 1;

      if (option_type_ == OptionType_t::PUT) {
        q = -1;
      }
      double variance_sqrt = HFSAT::NormalisedImpliedVolatilityFromTransformedRationalGuessWithLimitedIterations(
          beta,               // option price
          norm_strike_price,  // normalized strike px
          q,                  // 1/-1 -> call or put
          NUM_ITER_Z            // num of interations
          );

      stdev_ = variance_sqrt / sqrt(years_to_expiry_);
      implied_vol_changed_ = true;
    }
  }

  void initialize_price() {
    if (option_price_ < 0) {
      double norm_strike_price = log(future_price_ / strike_price_);
      double variance_sqrt = sqrt(stdev_ * stdev_ * years_to_expiry_);
      int q = 1;

      if (option_type_ == OptionType_t::PUT) {
        q = -1;
      }

      option_price_ = HFSAT::NormalisedBlack(norm_strike_price, variance_sqrt, q) *
                      exp(-interest_rate_ * years_to_expiry_) * sqrt(future_price_ * strike_price_);
    }
  }

  virtual ~Option() {}

  inline OptionType_t Type() const { return option_type_; }
  inline double Strike() const { return strike_price_; }
  inline double Future() const { return future_price_; }
  inline double Days() const { return years_to_expiry_ * 365; }
  inline double Stdev() {
    if (stdev_ < 0) {
      initialize_impliedvol();
    }
    return (stdev_);
  }
  inline double Rate() const { return interest_rate_; }
  inline double Price() {
    if (option_price_ < 0) {
      initialize_price();
    }
    return (option_price_);
  }

  inline double Delta() const { return delta_; }
  inline double Gamma() const { return gamma_; }
  inline double Theta() const { return theta_; }
  inline double ThetaPerDay() const { return theta_per_day_; }
  inline double Vega() const { return vega_; }
  inline double Rho() const { return rho_; }
  inline double Vanna() const { return vanna_; }
  inline double Vomma() const { return vomma_; }
  inline double Charm() const { return charm_; }

  inline void SetFuturePrice(const double t_future_price_) {
    if (future_price_ != t_future_price_) {
      future_price_ = t_future_price_;
      future_price_changed_ = true;
      SetHelperParameters();
    }
  }
  inline void SetOptionPrice(const double t_option_price_) {
    if (t_option_price_ != option_price_) {
      option_price_ = t_option_price_;
      stdev_ = -1;

      // To prevent option price from going less than intrinsic value
      if ((strike_price_ - future_price_) * (int)option_type_ > option_price_)
        option_price_ = (strike_price_ - future_price_) * (int)option_type_;

      initialize_impliedvol();
      implied_vol_changed_ = true;
      SetHelperParameters();
    }
  }
  inline void SetDaysToExpiry(const double t_days_to_expiry_) {
    if (t_days_to_expiry_ != days_to_expiry_) {
      days_to_expiry_ = t_days_to_expiry_;
      years_to_expiry_ = t_days_to_expiry_ / 365;
      time_to_maturity_changed_ = true;
      SetHelperParameters();
    }
  }
  inline void SetYearsToExpiry(const double t_years_to_expiry_) {
    if (t_years_to_expiry_ != years_to_expiry_) {
      years_to_expiry_ = t_years_to_expiry_;
      days_to_expiry_ = t_years_to_expiry_ * 365;
      time_to_maturity_changed_ = true;
      SetHelperParameters();
    }
  }
  inline void SetImpliedVol(double _implied_vol_) {
    if (_implied_vol_ != stdev_) {
      stdev_ = _implied_vol_;
      option_price_ = -1;
      initialize_price();
      implied_vol_changed_ = true;
      SetHelperParameters();
    }
  }
  inline void SetImpliedVolOnly(double _stdev_) { stdev_ = _stdev_; }
  inline void SetYearsToExpiryOnly(double _years_to_expiry_) { years_to_expiry_ = _years_to_expiry_; }

  inline void DeductTime(double _time_to_deduct_) {
    years_to_expiry_ -= _time_to_deduct_;
    days_to_expiry_ = years_to_expiry_ * 365;
    time_to_maturity_changed_ = true;
    SetHelperParameters();
  }

  void SetImpliedVolUsingOptionPx(const double);
  void ComputeDelta();
  void ComputeGamma();
  void ComputeVega();
  void ComputeTheta();

  // compute greeks .. TODO add optional calls for smaller subset of greeks
  // TODO - have market callback for price change - should call computeGreeks perhaps post a change threshold
  // TODO - have callback for implied vol change - should call computeGreeks perhaps post a change threshold
  void computeGreeks();
  void ComputeGreeks(double t_future_price_, double t_option_price_) {
    if (future_price_ != t_future_price_) {
      future_price_ = t_future_price_;
      future_price_changed_ = true;
    }

    if (t_option_price_ != option_price_) {
      option_price_ = t_option_price_;
      stdev_ = -1;
      initialize_impliedvol();
      implied_vol_changed_ = true;
    }

    SetHelperParameters();
    computeGreeks();
  }

  static double GetOptionPriceFromImpliedVol(double _strike_price_, double _years_to_expiry_, double _future_price_,
                                             double _interest_rate_, double _stdev_, OptionType_t _option_type_) {
    if ((_future_price_ < 0) || (_stdev_ < 0)) return -1000;

    double norm_strike_price = log(_future_price_ / _strike_price_);
    double variance_sqrt = sqrt(_stdev_ * _stdev_ * _years_to_expiry_);
    int q = 1;

    if (_option_type_ == OptionType_t::PUT) {
      q = -1;
    }

    double _option_price_ = HFSAT::NormalisedBlack(norm_strike_price, variance_sqrt, q) *
                            exp(-_interest_rate_ * _years_to_expiry_) * sqrt(_future_price_ * _strike_price_);
    return _option_price_;
  }

  static double GetImpliedVolFromOptionPrice(double _strike_price_, double _years_to_expiry_, double _future_price_,
                                             double _interest_rate_, double _option_price_,
                                             OptionType_t _option_type_) {
    if ((_future_price_ < 0) || (_option_price_ < 0)) return -1000;

    double beta = _option_price_;
    beta *= exp(_interest_rate_ * _years_to_expiry_);
    beta /= sqrt(_future_price_ * _strike_price_);

    double norm_strike_price = log(_future_price_ / _strike_price_);
    int q = 1;

    if (_option_type_ == OptionType_t::PUT) {
      q = -1;
    }
    double variance_sqrt = HFSAT::NormalisedImpliedVolatilityFromTransformedRationalGuessWithLimitedIterations(
        beta,               // option price
        norm_strike_price,  // normalized strike px
        q,                  // 1/-1 -> call or put
        NUM_ITER_Z            // num of interations
        );

    double _stdev_ = variance_sqrt / sqrt(_years_to_expiry_);
    return _stdev_;
  }

  // object functions
  double GetOptionPriceFromImpliedVolatlity(double _future_price_, double _stdev_) {
    return GetOptionPriceFromImpliedVol(strike_price_, years_to_expiry_, _future_price_, interest_rate_, _stdev_,
                                        option_type_);
  }

  double GetOptionPriceFromFuturesPrice(double _future_price_) {
    return GetOptionPriceFromImpliedVol(strike_price_, years_to_expiry_, _future_price_, interest_rate_, stdev_,
                                        option_type_);
  }

  double GetImpliedVol(double _future_price_, double _option_price_) {
    return GetImpliedVolFromOptionPrice(strike_price_, years_to_expiry_, _future_price_, interest_rate_, _option_price_,
                                        option_type_);
  }

 protected:
  // Option specifications for plain vanilla options
  OptionType_t option_type_;
  double strike_price_;
  double future_price_;
  double years_to_expiry_;
  double days_to_expiry_;
  double stdev_;
  double interest_rate_;
  double option_price_;

  // Option Greeks
  double delta_;
  double gamma_;
  double theta_;
  double theta_per_day_;
  double vega_;
  double rho_;
  double vanna_;
  double vomma_;
  double charm_;

  // variables used in computing greeks
  double d1;
  double d2;
  double Nd1;
  double Nd2;
  double nd1;
  double nd2;
  double sqrtT_;
  double sigmasqrtT_;
  double df_;

  bool time_to_maturity_changed_;
  bool future_price_changed_;
  bool implied_vol_changed_;
  bool strike_changed_;

  CumulativeNormalDistribution* cdf_;
};
}
