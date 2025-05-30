/**
    \file OptionsUtils/option_object.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#pragma once

#include "dvccode/CommonTradeUtils/watch.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "baseinfra/OptionsUtils/lets_be_rational.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "baseinfra/OptionsUtils/distributions.hpp"

namespace HFSAT {

#define NUM_ITER 10
#define SEC_IN_YEARS 3.170979e-08
#define MSEC_IN_YEARS 3.170979e-11

enum OptionType_t { PUT = -1, CALL = 1 };

struct OptionGreeks {
  double delta_;
  double gamma_;
  double vega_;
  double theta_;
};

class OptionObject {
 protected:
  DebugLogger& dbglogger_;
  const Watch& watch_;
  static std::map<std::string, OptionObject*> option_objects_map_;

  OptionObject(DebugLogger& _dbglogger_, const Watch& _watch_, std::string _option_name_);

 private:
  std::string option_name_;  // unique name
  int is_call_q_;
  double strike_;
  double interest_rate_;
  double expiry_minus_todays_midnight_in_years_;
  double current_time_minus_midnight_in_years_;

 public:
  static OptionObject* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_, std::string _option_name_);
  static unsigned int ClearOptionObjects() {
    unsigned int t_size_ = option_objects_map_.size();
    for (auto& it : option_objects_map_) {
      delete it.second;
    }
    option_objects_map_.clear();
    return t_size_;
  }

  static std::string VarName() { return "OptionObject"; }

  std::string shortcode() { return option_name_; }
  int is_call() { return is_call_q_; }
  double strike() { return strike_;}


  inline double MktImpliedVol(double _mkt_fut_px_, double _mkt_opt_px_) {
    if ((_mkt_fut_px_ <= 0) || (_mkt_opt_px_ <= 0)) return 0;

    current_time_minus_midnight_in_years_ = watch_.msecs_from_midnight() * MSEC_IN_YEARS;

    double intrinsic_value_ =
        (is_call_q_ == 1) ? std::max(0.0, _mkt_fut_px_ - strike_) : std::max(0.0, strike_ - _mkt_fut_px_);
    _mkt_opt_px_ =
        std::max(intrinsic_value_, _mkt_opt_px_);  // Bounding option price to be atleast equal to intrinsic value

    double time_to_expiry_ =
        std::max(600 * SEC_IN_YEARS, (expiry_minus_todays_midnight_in_years_ -
                                      current_time_minus_midnight_in_years_));  // Bounding this value by 10 mins

    double norm_strike_price_ = log(_mkt_fut_px_ / strike_);
    double beta_ = (_mkt_opt_px_) * (exp(interest_rate_ * (time_to_expiry_))) / (sqrt(_mkt_fut_px_ * strike_));

    double sigma_ = HFSAT::NormalisedImpliedVolatilityFromTransformedRationalGuessWithLimitedIterations(
        beta_, norm_strike_price_, is_call_q_, NUM_ITER);
    return (sigma_ / sqrt(time_to_expiry_));
  }

  inline double ModelImpliedPrice(double _model_fut_px_, double _model_iv_) {
    current_time_minus_midnight_in_years_ = watch_.msecs_from_midnight() * MSEC_IN_YEARS;

    if (dbglogger_.CheckLoggingLevel(OPTIONS_INFO)) {
      DBGLOG_TIME_CLASS_FUNC_LINE << " FutPx: " << _model_fut_px_ << " ImpledVol: " << _model_iv_
                                  << " Strike: " << strike_ << " InterestRate: " << interest_rate_
                                  << " TimeToMaturity: "
                                  << expiry_minus_todays_midnight_in_years_ - current_time_minus_midnight_in_years_
                                  << DBGLOG_ENDL_FLUSH;
    }

    double norm_strike_price_ = log(_model_fut_px_ / strike_);
    double time_to_expiry_ =
        std::max(600 * SEC_IN_YEARS, (expiry_minus_todays_midnight_in_years_ -
                                      current_time_minus_midnight_in_years_));  // Bounding this value by 10 mins
    double sigma_ = _model_iv_ * sqrt(time_to_expiry_);

    return (HFSAT::NormalisedBlack(norm_strike_price_, sigma_, is_call_q_) *
            (exp(-interest_rate_ * (time_to_expiry_))) * (sqrt(_model_fut_px_ * strike_)));
  }

  // market based
  inline void ComputeGreeks(double _mkt_fut_px_, double _mkt_opt_px_) {
    current_time_minus_midnight_in_years_ = watch_.msecs_from_midnight() * MSEC_IN_YEARS;

    double stdevA_ = MktImpliedVol(_mkt_fut_px_, _mkt_opt_px_);
    double time_to_expiry_ =
        std::max(600 * SEC_IN_YEARS, (expiry_minus_todays_midnight_in_years_ -
                                      current_time_minus_midnight_in_years_));  // Bounding this value by 10 mins
    double sqrtT_ = sqrt(time_to_expiry_);
    double sigmasqrtT_ = stdevA_ * sqrtT_;

    double d1 = std::log(_mkt_fut_px_ / strike_) / sigmasqrtT_ + 0.5 * sigmasqrtT_;
    double nd1 = cdf_->derivative(d1);
    double Nd1 = (*cdf_)(d1);

    double d2 = d1 - sigmasqrtT_;
    // double nd2 = cdf_->derivative(d2);
    double Nd2 = (*cdf_)(d2);

    double df_ = exp(-interest_rate_ * (time_to_expiry_));

    if (is_call_q_ == 1) {
      greeks_.delta_ = df_ * (Nd1);
      greeks_.theta_ = df_ * ((interest_rate_ * _mkt_fut_px_ * Nd1) - (interest_rate_ * strike_ * Nd2) -
                              (_mkt_fut_px_ * nd1 * stdevA_ / (2 * sqrtT_)));
    } else {
      greeks_.delta_ = df_ * (Nd1 - 1);
      greeks_.theta_ = df_ * ((interest_rate_ * _mkt_fut_px_ * (Nd1 - 1)) - (interest_rate_ * strike_ * (Nd2 - 1)) -
                              (_mkt_fut_px_ * nd1 * stdevA_ / (2 * sqrtT_)));
    }

    greeks_.gamma_ = df_ * nd1 / (_mkt_fut_px_ * sigmasqrtT_);
    greeks_.vega_ = df_ * _mkt_fut_px_ * sqrtT_ * nd1;
  }

  static CumulativeNormalDistribution* cdf_;
  OptionGreeks greeks_;
};
}
