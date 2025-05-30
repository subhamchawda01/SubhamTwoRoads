/**
    \file IndicatorsCode/simple_trend.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"

#include "dvctrade/Indicators/moving_avg_price_implied_vol.hpp"

namespace HFSAT {

void MovingAvgPriceImpliedVol::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                                 std::vector<std::string>& _ors_source_needed_vec_,
                                                 const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_,
                               NSESecurityDefinitions::GetFutureShortcodeFromOptionShortCode(r_tokens_[3]));
}

MovingAvgPriceImpliedVol* MovingAvgPriceImpliedVol::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                                      const std::vector<const char*>& r_tokens_,
                                                                      PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _indep_market_view_ _fractional_seconds_ _price_type_
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  ShortcodeSecurityMarketViewMap::StaticCheckValid(
      NSESecurityDefinitions::GetFutureShortcodeFromOptionShortCode(r_tokens_[3]));
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           atof(r_tokens_[4]), StringToPriceType_t(r_tokens_[5]));
}

MovingAvgPriceImpliedVol* MovingAvgPriceImpliedVol::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                                      const SecurityMarketView& _option_market_view_,
                                                                      double _fractional_seconds_,
                                                                      PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _option_market_view_.secname() << ' ' << _fractional_seconds_ << ' '
              << PriceType_t_To_String(_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  if (global_concise_indicator_description_map_.find(concise_indicator_description_) ==
      global_concise_indicator_description_map_.end()) {
    global_concise_indicator_description_map_[concise_indicator_description_] =
        new MovingAvgPriceImpliedVol(t_dbglogger_, r_watch_, concise_indicator_description_, _option_market_view_,
                                     _fractional_seconds_, _price_type_);
  }
  return dynamic_cast<MovingAvgPriceImpliedVol*>(
      global_concise_indicator_description_map_[concise_indicator_description_]);
}

MovingAvgPriceImpliedVol::MovingAvgPriceImpliedVol(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                   const std::string& concise_indicator_description_,
                                                   const SecurityMarketView& _option_market_view_,
                                                   double _fractional_seconds_, PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      option_market_view_(_option_market_view_),
      future_market_view_(*(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(
          NSESecurityDefinitions::GetFutureShortcodeFromOptionShortCode(option_market_view_.shortcode())))),
      price_history_secs_(_fractional_seconds_),
      price_type_(_price_type_),
      average_option_price_(0.0),
      average_future_price_(0.0),
      option_interrupted_(false),
      future_interrupted_(false) {
  option_exponential_moving_average_ = ExponentialMovingAverage::GetUniqueInstance(
      dbglogger_, watch_, option_market_view_, price_history_secs_, price_type_);
  option_exponential_moving_average_->add_unweighted_indicator_listener(1, this);

  future_exponential_moving_average_ = ExponentialMovingAverage::GetUniqueInstance(
      dbglogger_, watch_, future_market_view_, price_history_secs_, price_type_);
  future_exponential_moving_average_->add_unweighted_indicator_listener(2, this);

  option_ = OptionObject::GetUniqueInstance(t_dbglogger_, r_watch_, option_market_view_.shortcode());
}

void MovingAvgPriceImpliedVol::WhyNotReady() {
  if (!is_ready_) {
    if (!(future_market_view_.is_ready())) {
      DBGLOG_TIME_CLASS << future_market_view_.secname() << " is_ready_complex = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
    if (!(option_market_view_.is_ready_complex2(0))) {
      DBGLOG_TIME_CLASS << option_market_view_.secname() << " is_ready_complex2(0) = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
  }
}

void MovingAvgPriceImpliedVol::OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _average_price_) {
  if (_indicator_index_ == 1u) {
    average_option_price_ = _average_price_;
  } else if (_indicator_index_ == 2u) {
    average_future_price_ = _average_price_;
  }

  if (!is_ready_) {
    if ((option_market_view_.is_ready_complex2(0) || option_market_view_.spread_increments() <= 4) &&
        (future_market_view_.is_ready_complex2(0) || future_market_view_.spread_increments() <= 4)) {
      is_ready_ = true;
      InitializeValues();
      // both prices are set now, so why not
      indicator_value_ = option_->MktImpliedVol(average_future_price_, average_option_price_);
      if (std::isnan(indicator_value_ * 0)) {
      } else {
        NotifyIndicatorListeners(indicator_value_);
      }
    }
  } else if (!data_interrupted_) {
    indicator_value_ = option_->MktImpliedVol(average_option_price_, average_option_price_);

    if (std::isnan(indicator_value_ * 0)) {
    } else {
      NotifyIndicatorListeners(indicator_value_);
    }
  }
}

void MovingAvgPriceImpliedVol::InitializeValues() { indicator_value_ = 0; }

void MovingAvgPriceImpliedVol::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                       const int msecs_since_last_receive_) {
  if (option_market_view_.security_id() == _security_id_) {
    option_interrupted_ = true;
  } else if (future_market_view_.security_id() == _security_id_) {
    future_interrupted_ = true;
  }
  if (option_interrupted_ || future_interrupted_) {
    data_interrupted_ = true;
    indicator_value_ = 0;
  }
}

void MovingAvgPriceImpliedVol::OnMarketDataResumed(const unsigned int _security_id_) {
  if (data_interrupted_) {
    if (option_market_view_.security_id() == _security_id_) {
      option_interrupted_ = false;
    } else if (future_market_view_.security_id() == _security_id_) {
      future_interrupted_ = false;
    }
    if (!(future_interrupted_ || option_interrupted_)) {
      InitializeValues();
      data_interrupted_ = false;
    }
  }
}
}
