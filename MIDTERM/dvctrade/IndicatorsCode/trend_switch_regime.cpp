/**
   \file IndicatorsCode/trend_switch_regime.cpp

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

#include "dvctrade/Indicators/trend_switch_regime.hpp"

namespace HFSAT {

void TrendSwitchRegime::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                          std::vector<std::string>& _ors_source_needed_vec_,
                                          const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

TrendSwitchRegime* TrendSwitchRegime::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                        const std::vector<const char*>& r_tokens_,
                                                        PriceType_t _basepx_pxtype_) {
  // INDICATOR  _this_weight_  _indicator_string_  _dep_market_view_  _fractional_st_seconds_ _fractional_lt_seconds_
  // _tolerance_  _price_type_
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);

  if ((r_tokens_.size() >= 8) && (atof(r_tokens_[4]) <= atof(r_tokens_[5]))) {
    return GetUniqueInstance(
        t_dbglogger_, r_watch_, *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
        atof(r_tokens_[4]), atof(r_tokens_[5]), atof(r_tokens_[6]), StringToPriceType_t(r_tokens_[7]));
  } else {
    return NULL;
  }
}

TrendSwitchRegime* TrendSwitchRegime::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                        const SecurityMarketView& _dep_market_view_,
                                                        double _fractional_st_seconds_, double _fractional_lt_seconds_,
                                                        double _tolerance_, PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _dep_market_view_.secname() << ' ' << _fractional_st_seconds_ << ' ' << ' '
              << _fractional_lt_seconds_ << ' ' << ' ' << _tolerance_ << PriceType_t_To_String(_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, TrendSwitchRegime*> concise_indicator_description_map_;

  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new TrendSwitchRegime(t_dbglogger_, r_watch_, concise_indicator_description_, _dep_market_view_,
                              _fractional_st_seconds_, _fractional_lt_seconds_, _tolerance_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

TrendSwitchRegime::TrendSwitchRegime(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                     const std::string& concise_indicator_description_,
                                     const SecurityMarketView& t_dep_market_view_, double _fractional_st_seconds_,
                                     double _fractional_lt_seconds_, double _tolerance_, PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      dep_market_view_(t_dep_market_view_),
      st_trend_(0.0),
      lt_trend_(0.0),
      tolerance_(_tolerance_) {
  p_st_indicator_ =
      SimpleTrend::GetUniqueInstance(t_dbglogger_, r_watch_, dep_market_view_.shortcode(), _fractional_st_seconds_, _price_type_);
  p_st_indicator_->add_indicator_listener(0, this, 1.00);

  double ratio_lt_st_ = std::max(1.00, sqrt(_fractional_lt_seconds_ / _fractional_st_seconds_));

  p_lt_indicator_ =
      SimpleTrend::GetUniqueInstance(t_dbglogger_, r_watch_, dep_market_view_.shortcode(), _fractional_lt_seconds_, _price_type_);
  p_lt_indicator_->add_indicator_listener(1, this, (1.00 / ratio_lt_st_));
}

void TrendSwitchRegime::OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_) {
  if (!is_ready_) {
    if (dep_market_view_.is_ready_complex(2)) {
      is_ready_ = true;
      InitializeValues();
    }
  } else {
    if (_indicator_index_ == 0u) {
      st_trend_ = _new_value_;
    } else {
      lt_trend_ = _new_value_;
    }

    if (p_st_indicator_->IsDataInterrupted()) {
      indicator_value_ = 1;
      NotifyIndicatorListeners(indicator_value_);
    } else {
      if (st_trend_ * lt_trend_ < 0 && indicator_value_ != 1) {
        if (((st_trend_ + (1 - tolerance_) * lt_trend_) > 0 && indicator_value_ == 2) ||
            (indicator_value_ == 3 && (st_trend_ + (1 - tolerance_) * lt_trend_) < 0)) {
          indicator_value_ = 1;
          NotifyIndicatorListeners(indicator_value_);
        }
      } else {
        if ((lt_trend_ < 0 && st_trend_ < (1 - tolerance_) * lt_trend_ && indicator_value_ == 1) ||
            (lt_trend_ < 0 && st_trend_ < (1 + tolerance_) * lt_trend_ && indicator_value_ == 3)) {
          indicator_value_ = 2;  // trending low
          NotifyIndicatorListeners(indicator_value_);
        }

        if ((lt_trend_ > 0 && st_trend_ > (1 - tolerance_) * lt_trend_ && indicator_value_ == 1) ||
            (lt_trend_ > 0 && st_trend_ > (1 + tolerance_) * lt_trend_ && indicator_value_ == 2)) {
          indicator_value_ = 3;  // trending high
          NotifyIndicatorListeners(indicator_value_);
        }
      }
    }
  }
}

void TrendSwitchRegime::WhyNotReady() {
  if (!is_ready_) {
    if (!(dep_market_view_.is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << dep_market_view_.secname() << " is_ready_complex(2) = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
  }
}

// market_interrupt_listener interface
void TrendSwitchRegime::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {
  if (_security_id_ == dep_market_view_.security_id()) {
    data_interrupted_ = true;
    indicator_value_ = 1;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void TrendSwitchRegime::OnMarketDataResumed(const unsigned int _security_id_) {
  if (_security_id_ == dep_market_view_.security_id()) {
    data_interrupted_ = false;
  }
}

void TrendSwitchRegime::InitializeValues() {
  indicator_value_ = 1;
  NotifyIndicatorListeners(indicator_value_);
}
}
