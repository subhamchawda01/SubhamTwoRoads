/**
    \file IndicatorsCode/tod_tom_norm_spread.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066, India
   +91 80 4060 0717
 */
#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/tod_tom_norm_spread.hpp"
#include "baseinfra/BaseUtils/curve_utils.hpp"

namespace HFSAT {

void TodTomNormSpread::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                         std::vector<std::string>& _ors_source_needed_vec_,
                                         const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[4]);
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[5]);
}

TodTomNormSpread* TodTomNormSpread::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                      const std::vector<const char*>& r_tokens_,
                                                      PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _sec_market_view_ _price_type_
  if (r_tokens_.size() < 6) {
    ExitVerbose(
        kModelCreationIndicatorLineLessArgs, t_dbglogger_,
        "INDICATOR weight TodTomNormSpread _todtom_market_view_ _tom_market_view_ _tod_market_view_ _price_type_ ");
  }
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  return GetUniqueInstance(
      t_dbglogger_, r_watch_, *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
      *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[4])),
      *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[5])), StringToPriceType_t(r_tokens_[6]));
}

TodTomNormSpread* TodTomNormSpread::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                      const SecurityMarketView& _todtom_market_view_,
                                                      const SecurityMarketView& _tom_market_view_,
                                                      const SecurityMarketView& _tod_market_view_,
                                                      PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _todtom_market_view_.secname() << ' ' << _tom_market_view_.secname() << ' '
              << _tod_market_view_.secname() << PriceType_t_To_String(_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, TodTomNormSpread*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new TodTomNormSpread(t_dbglogger_, r_watch_, concise_indicator_description_, _todtom_market_view_,
                             _tom_market_view_, _tod_market_view_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

TodTomNormSpread::TodTomNormSpread(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                   const std::string& concise_indicator_description_,
                                   const SecurityMarketView& _todtom_market_view_,
                                   const SecurityMarketView& _tom_market_view_,
                                   const SecurityMarketView& _tod_market_view_, PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      todtom_market_view_(_todtom_market_view_),
      tod_market_view_(_tod_market_view_),
      price_type_(_price_type_),
      tom_term_(1),
      current_todtom_price_(0.0),
      current_tod_price_(0.0) {
  tom_term_ = HFSAT::CurveUtils::_get_term_(watch_.YYYYMMDD(), _tom_market_view_.secname());

  if (!todtom_market_view_.subscribe_price_type(this, price_type_) ||
      !tod_market_view_.subscribe_price_type(this, price_type_)) {
    PriceType_t t_error_price_type_ = _price_type_;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " passed " << t_error_price_type_ << std::endl;
  }
}

void TodTomNormSpread::WhyNotReady() {
  if (!is_ready_) {
    if (!(todtom_market_view_.is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << todtom_market_view_.secname() << " is_ready_complex = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
  }
}

void TodTomNormSpread::OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {
  if (_security_id_ == todtom_market_view_.security_id()) {
    current_todtom_price_ = SecurityMarketView::GetPriceFromType(price_type_, _market_update_info_);
  } else if (_security_id_ == tod_market_view_.security_id()) {
    current_tod_price_ = SecurityMarketView::GetPriceFromType(price_type_, _market_update_info_);
  } else {
    return;
  }

  if (!is_ready_) {
    if (todtom_market_view_.is_ready_complex(2)) {
      is_ready_ = true;
      InitializeValues();
    }
  } else if (!data_interrupted_) {
    double t_spread_ = current_todtom_price_ / tom_term_;
    double one_day_rate_ = t_spread_ / current_tod_price_;
    indicator_value_ = one_day_rate_ * 365;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void TodTomNormSpread::InitializeValues() { indicator_value_ = 0; }

// market_interrupt_listener interface
void TodTomNormSpread::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {
  // Emitting the same value is the best thing to do here....so doing nothing
}

void TodTomNormSpread::OnMarketDataResumed(const unsigned int _security_id_) {
  // See explanantion above in OnMarketDataInterrupted
}
}
