/**
    \file IndicatorsCode/spot_future_spread.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066, India
   +91 80 4060 0717
 */
#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/spot_future_spread.hpp"
#include "baseinfra/BaseUtils/curve_utils.hpp"

namespace HFSAT {

void SpotFutureSpread::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                         std::vector<std::string>& _ors_source_needed_vec_,
                                         const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[4]);
}

SpotFutureSpread* SpotFutureSpread::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                      const std::vector<const char*>& r_tokens_,
                                                      PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _sec_market_view_ _price_type_
  if (r_tokens_.size() < 6) {
    ExitVerbose(kModelCreationIndicatorLineLessArgs, t_dbglogger_,
                "INDICATOR weight SpotFutureSpread _fut_market_view_ _spot_market_view_ _price_type_ ");
  }
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  return GetUniqueInstance(
      t_dbglogger_, r_watch_, *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
      *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[4])), StringToPriceType_t(r_tokens_[5]));
}

SpotFutureSpread* SpotFutureSpread::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                      const SecurityMarketView& _fut_market_view_,
                                                      const SecurityMarketView& _spot_market_view_,
                                                      PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _fut_market_view_.secname() << ' ' << _spot_market_view_.secname() << ' '
              << PriceType_t_To_String(_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, SpotFutureSpread*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] = new SpotFutureSpread(
        t_dbglogger_, r_watch_, concise_indicator_description_, _fut_market_view_, _spot_market_view_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

SpotFutureSpread::SpotFutureSpread(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                   const std::string& concise_indicator_description_,
                                   const SecurityMarketView& _fut_market_view_,
                                   const SecurityMarketView& _spot_market_view_, PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      fut_market_view_(_fut_market_view_),
      spot_market_view_(_spot_market_view_),
      price_type_(_price_type_),
      fut_term_(1),
      current_fut_price_(0.0),
      current_spot_price_(0.0) {
  fut_term_ = HFSAT::CurveUtils::_get_term_(watch_.YYYYMMDD(), _fut_market_view_.secname());

  // using midprice for the spot. One it shouldn't make a big difference. Two, we need the changes for different px
  // types of fut and this will reduce noise
  if (!fut_market_view_.subscribe_price_type(this, price_type_) ||
      !spot_market_view_.subscribe_price_type(this, kPriceTypeMidprice)) {
    PriceType_t t_error_price_type_ = _price_type_;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " passed " << t_error_price_type_ << std::endl;
  }
}

void SpotFutureSpread::WhyNotReady() {
  if (!is_ready_) {
    if (!(fut_market_view_.is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << fut_market_view_.secname() << " is_ready_complex = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }

    if (!(spot_market_view_.is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << spot_market_view_.secname() << " is_ready_complex = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
  }
}

void SpotFutureSpread::OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {
  if (_security_id_ == fut_market_view_.security_id()) {
    current_fut_price_ = SecurityMarketView::GetPriceFromType(price_type_, _market_update_info_) / 1000.0;  // For Si_0
  } else if (_security_id_ == spot_market_view_.security_id()) {
    current_spot_price_ = SecurityMarketView::GetPriceFromType(price_type_, _market_update_info_);
  } else {
    return;
  }

  if (!is_ready_) {
    if (fut_market_view_.is_ready_complex(2) && spot_market_view_.is_ready_complex(2)) {
      is_ready_ = true;
      InitializeValues();
    }
  } else if (!data_interrupted_) {
    double t_spread_ = current_fut_price_ - current_spot_price_;
    double t_interest_ = t_spread_ / current_spot_price_;
    indicator_value_ = t_interest_ * 365 / fut_term_;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void SpotFutureSpread::InitializeValues() { indicator_value_ = 0; }

// market_interrupt_listener interface
void SpotFutureSpread::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {
  // Emitting the same value is the best thing to do here....so doing nothing
}

void SpotFutureSpread::OnMarketDataResumed(const unsigned int _security_id_) {
  // See explanantion above in OnMarketDataInterrupted
}
}
