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

#include "dvctrade/Indicators/simple_trend_regime.hpp"
#include "dvctrade/Indicators/simple_trend.hpp"

namespace HFSAT {

void SimpleTrendRegime::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                          std::vector<std::string>& _ors_source_needed_vec_,
                                          const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

SimpleTrendRegime* SimpleTrendRegime::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                        const std::vector<const char*>& r_tokens_,
                                                        PriceType_t _basepx_pxtype_) {
  // INDICATOR  _this_weight_  _indicator_string_  _dep_market_view_  _fractional_seconds_  _trend_ratio_thresh_
  // _price_type_
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);

  if (r_tokens_.size() >= 5) {
    return GetUniqueInstance(t_dbglogger_, r_watch_,
                             *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                             atof(r_tokens_[4]), atof(r_tokens_[5]), StringToPriceType_t(r_tokens_[6]));
    //        std::cout << "Unique instance called" <, std::endl;
  } else {
    return NULL;
  }
}

SimpleTrendRegime* SimpleTrendRegime::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                        const SecurityMarketView& _dep_market_view_,
                                                        double _fractional_seconds_, double _trend_ratio_thresh_,
                                                        PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _dep_market_view_.secname() << ' ' << _fractional_seconds_ << ' ' << ' '
              << _trend_ratio_thresh_ << ' ' << PriceType_t_To_String(_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, SimpleTrendRegime*> concise_indicator_description_map_;

  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new SimpleTrendRegime(t_dbglogger_, r_watch_, concise_indicator_description_, _dep_market_view_,
                              _fractional_seconds_, _trend_ratio_thresh_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

SimpleTrendRegime::SimpleTrendRegime(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                     const std::string& concise_indicator_description_,
                                     const SecurityMarketView& t_dep_market_view_, double _fractional_seconds_,
                                     double _trend_ratio_thresh_, PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      dep_market_view_(t_dep_market_view_),
      trend_ratio_thresh_(_trend_ratio_thresh_),
      dep_price_trend_(0),
      dep_l1_norm_ratio_(1.0),
      dep_mean_l1_norm_(LoadMeanL1Norm(r_watch_.YYYYMMDD(), dep_market_view_.shortcode(), NUM_DAYS_HISTORY)),
      pred_mode_(1u),
      p_dep_indicator_(NULL) {
  watch_.subscribe_FifteenSecondPeriod(this);  // for UpdateLRInfo and updating trend adjustment
                                               //    std::cout << " Constructor called" << std::endl;
  p_dep_indicator_ =
      SimpleTrend::GetUniqueInstance(t_dbglogger_, r_watch_, t_dep_market_view_.shortcode(), _fractional_seconds_, _price_type_);
  if (p_dep_indicator_ == NULL) {
    indicator_value_ = 0;
    is_ready_ = true;
    return;
  }

  p_dep_indicator_->add_unweighted_indicator_listener(1u, this);
}

void SimpleTrendRegime::OnTimePeriodUpdate(const int num_pages_to_add_) {
  dep_l1_norm_ratio_ = std::abs(dep_price_trend_) / dep_mean_l1_norm_;
  if (dep_l1_norm_ratio_ > trend_ratio_thresh_)
    pred_mode_ = 1u;
  else
    pred_mode_ = 0u;
}

void SimpleTrendRegime::OnIndicatorUpdate(const unsigned int& t_indicator_index_,
                                          const double& t_new_indicator_value_) {
  if (!is_ready_) {
    if (dep_market_view_.is_ready_complex(2)) {
      is_ready_ = true;
      InitializeValues();
    }
  } else {
    if (!data_interrupted_) {
      switch (t_indicator_index_) {
        case 1u: {
          dep_price_trend_ = t_new_indicator_value_;
        } break;
        default: { } break; }
      indicator_value_ = pred_mode_ + 1;
    }

    NotifyIndicatorListeners(indicator_value_);
  }
}

void SimpleTrendRegime::WhyNotReady() {
  if (!is_ready_) {
    if (!(dep_market_view_.is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << dep_market_view_.secname() << " is_ready_complex(2) = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
  }
}

// market_interrupt_listener interface
void SimpleTrendRegime::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {
  if (_security_id_ == dep_market_view_.security_id()) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void SimpleTrendRegime::OnMarketDataResumed(const unsigned int _security_id_) {
  if (_security_id_ == dep_market_view_.security_id()) {
    InitializeValues();
    data_interrupted_ = false;
  }
}

void SimpleTrendRegime::InitializeValues() { indicator_value_ = 0; }
}
