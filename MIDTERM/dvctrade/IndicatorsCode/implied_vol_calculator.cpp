/**
    \file IndicatorsCode/simple_trend.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/

#include "dvctrade/Indicators/implied_vol_calculator.hpp"

#define MAX_INT_SPREAD_TO_CONSIDER 50

namespace HFSAT {

void ImpliedVolCalculator::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                             std::vector<std::string>& _ors_source_needed_vec_,
                                             const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_,
                               NSESecurityDefinitions::GetFutureShortcodeFromOptionShortCode(r_tokens_[3]));
}

ImpliedVolCalculator* ImpliedVolCalculator::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                              const std::vector<const char*>& r_tokens_,
                                                              PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _indep_market_view_ _fractional_seconds_ _price_type_
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  ShortcodeSecurityMarketViewMap::StaticCheckValid(
      NSESecurityDefinitions::GetFutureShortcodeFromOptionShortCode(r_tokens_[3]));
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           StringToPriceType_t(r_tokens_[4]));
}

ImpliedVolCalculator* ImpliedVolCalculator::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                              const SecurityMarketView& _indep_market_view_,
                                                              PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _indep_market_view_.secname() << ' ' << PriceType_t_To_String(_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  //  static std::map<std::string, ImpliedVolCalculator*> concise_indicator_description_map_;
  if (global_concise_indicator_description_map_.find(concise_indicator_description_) ==
      global_concise_indicator_description_map_.end()) {
    global_concise_indicator_description_map_[concise_indicator_description_] = new ImpliedVolCalculator(
        t_dbglogger_, r_watch_, concise_indicator_description_, _indep_market_view_, _price_type_);
  }
  return dynamic_cast<ImpliedVolCalculator*>(global_concise_indicator_description_map_[concise_indicator_description_]);
}

ImpliedVolCalculator::ImpliedVolCalculator(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                           const std::string& concise_indicator_description_,
                                           const SecurityMarketView& _indep_market_view_, PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      indep_market_view_(_indep_market_view_),
      fut_market_view_(*(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(
          NSESecurityDefinitions::GetFutureShortcodeFromOptionShortCode(indep_market_view_.shortcode())))),
      price_type_(_price_type_),
      current_indep_price_(0),
      current_fut_price_(0) {
  if (!indep_market_view_.subscribe_price_type(this, _price_type_)) {
    PriceType_t t_error_price_type_ = _price_type_;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " passed " << t_error_price_type_ << std::endl;
  }

  if (!fut_market_view_.subscribe_price_type(this, _price_type_)) {
    PriceType_t t_error_price_type_ = _price_type_;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " passed " << t_error_price_type_ << std::endl;
  }

  option_ = OptionObject::GetUniqueInstance(t_dbglogger_, r_watch_, indep_market_view_.shortcode());
}

void ImpliedVolCalculator::WhyNotReady() {
  if (!is_ready_) {
    if (!(indep_market_view_.is_ready_complex2(0))) {
      DBGLOG_TIME_CLASS << indep_market_view_.secname() << " is_ready_complex2 = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
  }
}
// never notify 0
void ImpliedVolCalculator::OnMarketUpdate(const unsigned int _security_id_,
                                          const MarketUpdateInfo& _market_update_info_) {
  if (_security_id_ == indep_market_view_.security_id()) {
    current_indep_price_ = SecurityMarketView::GetPriceFromType(price_type_, _market_update_info_);
  } else {
    current_fut_price_ = SecurityMarketView::GetPriceFromType(price_type_, _market_update_info_);
  }

  if (!is_ready_) {
    if ((indep_market_view_.is_ready_complex2(0) || indep_market_view_.spread_increments() <= 4) &&
        (fut_market_view_.is_ready_complex2(0) || fut_market_view_.spread_increments() <= 4)) {
      is_ready_ = true;
      InitializeValues();
      // both prices are set now, so why not
      indicator_value_ = option_->MktImpliedVol(current_fut_price_, current_indep_price_);
      if (std::isnan(indicator_value_ * 0)) {
      } else {
        NotifyIndicatorListeners(indicator_value_);
      }
    }
  } else if (!data_interrupted_) {
    indicator_value_ = option_->MktImpliedVol(current_fut_price_, current_indep_price_);

    if (std::isnan(indicator_value_ * 0)) {
    } else {
      NotifyIndicatorListeners(indicator_value_);
    }
  }
}

void ImpliedVolCalculator::InitializeValues() { indicator_value_ = 0; }

// market_interrupt_listener interface
void ImpliedVolCalculator::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                   const int msecs_since_last_receive_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = true;
    // indicator_value_ = 0;
    // NotifyIndicatorListeners(indicator_value_);
  }
}

void ImpliedVolCalculator::OnMarketDataResumed(const unsigned int _security_id_) {
  if (indep_market_view_.security_id() == _security_id_) {
    // InitializeValues();
    data_interrupted_ = false;
  }
}
}
