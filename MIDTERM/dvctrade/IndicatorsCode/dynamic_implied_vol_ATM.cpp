/**
    \file IndicatorsCode/dynamic_implied_vol_ATM.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/

#include "dvctrade/Indicators/dynamic_implied_vol_ATM.hpp"

namespace HFSAT {

void DynamicImpliedVolATM::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                             std::vector<std::string>& _ors_source_needed_vec_,
                                             const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_,
                               NSESecurityDefinitions::LoadOptionsList((std::string)r_tokens_[3], 3));
}

DynamicImpliedVolATM* DynamicImpliedVolATM::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                              const std::vector<const char*>& r_tokens_,
                                                              PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _indep_market_view_ _fractional_seconds_ _price_type_
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           _basepx_pxtype_);
}

DynamicImpliedVolATM* DynamicImpliedVolATM::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                              const SecurityMarketView& _indep_market_view_,
                                                              PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _indep_market_view_.secname() << ' ' << PriceType_t_To_String(_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, DynamicImpliedVolATM*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] = new DynamicImpliedVolATM(
        t_dbglogger_, r_watch_, concise_indicator_description_, _indep_market_view_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

DynamicImpliedVolATM::DynamicImpliedVolATM(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                           const std::string& concise_indicator_description_,
                                           const SecurityMarketView& _indep_market_view_, PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      indep_market_view_(_indep_market_view_),
      price_type_(_price_type_),
      interest_rate_(NSESecurityDefinitions::GetInterestRate(watch_.YYYYMMDD())),
      days_to_expire_(0.0),
      step_value_(NSESecurityDefinitions::GetStepValueFromShortCode(indep_market_view_.shortcode())),
      last_msecs_days_to_expire_update_(0) {
  days_to_expire_ = difftime(DateTime::GetTimeUTC(
                                 NSESecurityDefinitions::GetExpiryFromShortCode(indep_market_view_.shortcode()), 1000),
                             DateTime::GetTimeMidnightUTC(watch_.YYYYMMDD())) /
                    (3600 * 24);
  DBGLOG_TIME_CLASS << indep_market_view_.secname() << indep_market_view_.secname() << " " << interest_rate_ << " "
                    << days_to_expire_ << " " << DBGLOG_ENDL_FLUSH;
  DBGLOG_DUMP;

  watch_.subscribe_FifteenSecondPeriod(this);
}

void DynamicImpliedVolATM::WhyNotReady() {
  if (!is_ready_) {
    if (!(indep_market_view_.is_ready())) {
      DBGLOG_TIME_CLASS << indep_market_view_.secname() << " is_ready_complex = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
  }
}

void DynamicImpliedVolATM::InitializeValues() {
  indicator_value_ = 0;
}

void DynamicImpliedVolATM::OnTimePeriodUpdate(const int num_pages_to_add_) {
  if ((watch_.msecs_from_midnight() - last_msecs_days_to_expire_update_) > 15 * 60 * 1000) {
    days_to_expire_ -= (double)(watch_.msecs_from_midnight() - last_msecs_days_to_expire_update_) / (1000 * 3600 * 24);
    last_msecs_days_to_expire_update_ = watch_.msecs_from_midnight();
  }

  if (!is_ready_) {
    if ((indep_market_view_.is_ready())) {
      is_ready_ = true;
      InitializeValues();
    }
  } else if (!data_interrupted_) {
    double current_indep_price_ = (indep_market_view_.bestbid_price() + indep_market_view_.bestask_price()) / 2;
    double strike_put_ = std::floor(current_indep_price_ / step_value_) * step_value_;
    double strike_call_ = std::ceil(current_indep_price_ / step_value_) * step_value_;

    std::string call_shc_ = NSESecurityDefinitions::GetShortCodeFromStrikePrice(
        strike_call_, indep_market_view_.shortcode(), OptionType_t::CALL);
    std::string put_shc_ = NSESecurityDefinitions::GetShortCodeFromStrikePrice(
        strike_put_, indep_market_view_.shortcode(), OptionType_t::PUT);

    const SecurityMarketView* call_smv_ = ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(call_shc_);
    const SecurityMarketView* put_smv_ = ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(put_shc_);

    if ((call_smv_ == NULL) || (put_smv_ == NULL)) {
      return;
    }

    double call_price_ = (call_smv_->bestbid_price() + call_smv_->bestask_price()) / 2;
    double put_price_ = (put_smv_->bestbid_price() + put_smv_->bestask_price()) / 2;

    double implied_vol_call_ = Option::GetImpliedVolFromOptionPrice(
        strike_call_, days_to_expire_ / (365.0), current_indep_price_, interest_rate_, call_price_, OptionType_t::CALL);
    double implied_vol_put_ = Option::GetImpliedVolFromOptionPrice(
        strike_put_, days_to_expire_ / (365.0), current_indep_price_, interest_rate_, put_price_, OptionType_t::PUT);

    double weight_put_ = log(strike_call_) - log(current_indep_price_);
    double weight_call_ = log(current_indep_price_) - log(strike_put_);

    indicator_value_ =
        (implied_vol_call_ * weight_call_ + implied_vol_put_ * weight_put_) / (weight_call_ + weight_put_);
    indicator_value_ *= 100;  // Changing the value to percentage

    if (indicator_value_ > 0) NotifyIndicatorListeners(indicator_value_);
  }
}

// market_interrupt_listener interface
void DynamicImpliedVolATM::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                   const int msecs_since_last_receive_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void DynamicImpliedVolATM::OnMarketDataResumed(const unsigned int _security_id_) {
  if (indep_market_view_.security_id() == _security_id_) {
    InitializeValues();
    data_interrupted_ = false;
  }
}
}
