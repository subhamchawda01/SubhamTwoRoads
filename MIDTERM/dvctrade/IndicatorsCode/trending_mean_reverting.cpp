/**
   \file IndicatorsCode/offline_breakout_adjusted_pairs.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#include <fstream>
#include <strings.h>
#include <stdlib.h>
#include <sstream>

#include "dvccode/CDef/math_utils.hpp"

#include "baseinfra/MarketAdapter/shortcode_security_market_view_map.hpp"

#include "dvctrade/Indicators/trending_mean_reverting.hpp"

namespace HFSAT {

void TrendingMeanReverting::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                              std::vector<std::string>& _ors_source_needed_vec_,
                                              const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

TrendingMeanReverting* TrendingMeanReverting::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                                const std::vector<const char*>& r_tokens_,
                                                                PriceType_t _basepx_pxtype_) {
  // INDICATOR  _this_weight_  _indicator_string_  _dep_market_view_  _indep_market_view_  _fractional_seconds_
  // _volume_measure_seconds_  _trend_weight_  _price_type_
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);

  if (r_tokens_.size() >= 6) {
    return GetUniqueInstance(t_dbglogger_, r_watch_,
                             *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                             atof(r_tokens_[4]), atof(r_tokens_[5]));
  } else if (r_tokens_.size() >= 5) {
    return GetUniqueInstance(t_dbglogger_, r_watch_,
                             *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                             atof(r_tokens_[4]), -1);
  } else {
    return NULL;
  }
}

TrendingMeanReverting* TrendingMeanReverting::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                                const SecurityMarketView& _dep_market_view_,
                                                                double _fractional_seconds_, double _ind_thresh_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _dep_market_view_.secname() << ' ' << _fractional_seconds_ << ' ' << _ind_thresh_;
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, TrendingMeanReverting*> concise_indicator_description_map_;

  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] = new TrendingMeanReverting(
        t_dbglogger_, r_watch_, concise_indicator_description_, _dep_market_view_, _fractional_seconds_, _ind_thresh_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

TrendingMeanReverting::TrendingMeanReverting(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                             const std::string& concise_indicator_description_,
                                             const SecurityMarketView& t_dep_market_view_, double _fractional_seconds_,
                                             double _ind_thresh_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      dep_market_view_(t_dep_market_view_),
      pred_mode_(0u),
      last_px_(0),
      last_recorded_px_(0),
      current_dep_price_(0),
      num_px_moves_(0),
      last_recorded_msecs_(0),
      width_msecs_(_fractional_seconds_ * 1000),
      indicator_threshold_(_ind_thresh_),
      trend_mean_rev_measure_(0) {
  if (!dep_market_view_.subscribe_price_type(this, kPriceTypeMidprice)) {
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " passed " << kPriceTypeMidprice << std::endl;
  }
}

void TrendingMeanReverting::OnMarketUpdate(const unsigned int _security_id_,
                                           const MarketUpdateInfo& _market_update_info_) {
  current_dep_price_ = _market_update_info_.mid_price_;
  if (!is_ready_) {
    if (dep_market_view_.is_ready_complex(2)) {
      is_ready_ = true;
      InitializeValues();
    }
  } else {
    if (watch_.msecs_from_midnight() - last_recorded_msecs_ < width_msecs_) {
      if (last_px_ != current_dep_price_) {
        num_px_moves_++;
      }
      last_px_ = current_dep_price_;
    } else {
      last_px_ = current_dep_price_;
      if (num_px_moves_ == 0 || last_px_ == last_recorded_px_) {
        trend_mean_rev_measure_ = -1;
      } else {
        trend_mean_rev_measure_ = num_px_moves_ / std::abs(last_px_ - last_recorded_px_);
      }
      if (indicator_threshold_ == -1) {
        indicator_value_ = trend_mean_rev_measure_;
      } else {
        if ((trend_mean_rev_measure_ == -1) || (trend_mean_rev_measure_ > indicator_threshold_)) {
          indicator_value_ = 2;
        } else {
          indicator_value_ = 1;
        }
      }
      NotifyIndicatorListeners(indicator_value_);
      last_recorded_msecs_ = watch_.msecs_from_midnight();
      num_px_moves_ = 0;
      last_px_ = last_recorded_px_ = current_dep_price_;
    }
  }
}

void TrendingMeanReverting::OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                                         const MarketUpdateInfo& _market_update_info_) {
  OnMarketUpdate(_security_id_, _market_update_info_);
  return;
}

void TrendingMeanReverting::WhyNotReady() {
  if (!is_ready_) {
    if (!(dep_market_view_.is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << dep_market_view_.secname() << " is_ready_complex(2) = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
  }
}

// market_interrupt_listener interface
void TrendingMeanReverting::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                    const int msecs_since_last_receive_) {
  if (_security_id_ == dep_market_view_.security_id()) {
    data_interrupted_ = true;
    trend_mean_rev_measure_ = 0;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void TrendingMeanReverting::OnMarketDataResumed(const unsigned int _security_id_) {
  if (_security_id_ == dep_market_view_.security_id()) {
    InitializeValues();
    data_interrupted_ = false;
  }
}

void TrendingMeanReverting::InitializeValues() {
  trend_mean_rev_measure_ = 0;
  indicator_value_ = 0;
  last_px_ = last_recorded_px_ = 0;
  last_recorded_msecs_ = watch_.msecs_from_midnight();
  num_px_moves_ = 0;
}
}
