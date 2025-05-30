/**
   \file IndicatorsCode/source_switch_regime_port.cpp

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
#include "dvctrade/Indicators/source_switch_regime_port.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"

namespace HFSAT {

void SrcSwitchRegPort::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                         std::vector<std::string>& _ors_source_needed_vec_,
                                         const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
  IndicatorUtil::AddPortfolioShortCodeVec((std::string)r_tokens_[4], _shortcodes_affecting_this_indicator_);
}

SrcSwitchRegPort* SrcSwitchRegPort::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                      const std::vector<const char*>& r_tokens_,
                                                      PriceType_t _basepx_pxtype_) {
  // INDICATOR  _this_weight_  _indicator_string_  _dep_market_view_  _fractional_st_seconds_ _fractional_lt_seconds_
  // _tolerance_  _price_type_
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);

  if (r_tokens_.size() >= 11) {
    return GetUniqueInstance(t_dbglogger_, r_watch_,
                             *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                             (std::string)r_tokens_[4], atof(r_tokens_[5]), atof(r_tokens_[6]), atof(r_tokens_[7]),
                             atof(r_tokens_[8]), atof(r_tokens_[9]), StringToPriceType_t(r_tokens_[10]));
  } else {
    return NULL;
  }
}

SrcSwitchRegPort* SrcSwitchRegPort::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                      const SecurityMarketView& _dep_market_view_,
                                                      const std::string& _portfolio_descriptor_shortcode_,
                                                      double _trend_duration_, double _port_thresh_fact_,
                                                      double _dep_thresh_fact_, double _volume_exponent_,
                                                      double _tolerance_, PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _dep_market_view_.secname() << ' ' << _portfolio_descriptor_shortcode_ << ' '
              << _trend_duration_ << ' ' << _port_thresh_fact_ << ' ' << _dep_thresh_fact_ << ' ' << _volume_exponent_
              << ' ' << _tolerance_ << ' ' << PriceType_t_To_String(_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, SrcSwitchRegPort*> concise_indicator_description_map_;

  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] = new SrcSwitchRegPort(
        t_dbglogger_, r_watch_, concise_indicator_description_, _dep_market_view_, _portfolio_descriptor_shortcode_,
        _trend_duration_, _port_thresh_fact_, _dep_thresh_fact_, _volume_exponent_, _tolerance_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

SrcSwitchRegPort::SrcSwitchRegPort(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                   const std::string& concise_indicator_description_,
                                   const SecurityMarketView& t_dep_market_view_,
                                   const std::string& _portfolio_descriptor_shortcode_, double _trend_duration_,
                                   double _port_thresh_fact_, double _dep_thresh_fact_, double _volume_exponent_,
                                   double _tolerance_, PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      dep_market_view_(t_dep_market_view_),
      indep_portfolio_price_(
          *(PCAPortPrice::GetUniqueInstance(t_dbglogger_, r_watch_, _portfolio_descriptor_shortcode_, _price_type_))),
      slow_stdev_calculator_port_(*(SlowStdevCalculatorPort::GetUniqueInstance(
          t_dbglogger_, r_watch_, _portfolio_descriptor_shortcode_, _trend_duration_ * 1000u))),
      slow_stdev_calculator_(*(SlowStdevCalculator::GetUniqueInstance(
          t_dbglogger_, r_watch_, t_dep_market_view_.shortcode(), _trend_duration_ * 1000u))),
      port_stdev_(1.0),
      dep_stdev_(1.0),
      volume_ratio_calculator_(
          *(VolumeRatioCalculator::GetUniqueInstance(t_dbglogger_, r_watch_, t_dep_market_view_, 900))),
      recent_scaled_volume_calculator_(
          *(RecentScaledVolumeCalculator::GetUniqueInstance(t_dbglogger_, r_watch_, t_dep_market_view_, 900))),
      volume_ratio_calculator_port_(*(
          VolumeRatioCalculatorPort::GetUniqueInstance(t_dbglogger_, r_watch_, _portfolio_descriptor_shortcode_, 900))),
      recent_scaled_volume_calculator_port_(*(RecentScaledVolumeCalculatorPort::GetUniqueInstance(
          t_dbglogger_, r_watch_, _portfolio_descriptor_shortcode_, 900))),
      volume_exponent_(_volume_exponent_),
      current_volume_ratio_dep_(1.0),
      scaled_volume_dep_(1.0),
      volume_factor_dep_(1.0),
      current_volume_ratio_port_(1.0),
      scaled_volume_port_(1.0),
      volume_factor_port_(1.0),
      dep_mean_stdev_(0.0),
      port_mean_stdev_(0.0),
      port_thresh_(_port_thresh_fact_),
      dep_thresh_(_dep_thresh_fact_),
      tolerance_(_tolerance_) {
  //

  dep_mean_stdev_ = sqrt(_trend_duration_ / 300.0) *
                    SampleDataUtil::GetAvgForPeriod(dep_market_view_.shortcode(), r_watch_.YYYYMMDD(), NUM_DAYS_HISTORY,
                                                    trading_start_mfm_, trading_end_mfm_, "STDEV");

  port_mean_stdev_ =
      sqrt(_trend_duration_ / 300.0) * LoadMeanStdevPort(_portfolio_descriptor_shortcode_, r_watch_.YYYYMMDD(),
                                                         NUM_DAYS_HISTORY, trading_start_mfm_, trading_end_mfm_);

  watch_.subscribe_FifteenSecondPeriod(this);
  slow_stdev_calculator_port_.AddSlowStdevCalculatorPortListener(this);
  slow_stdev_calculator_.AddSlowStdevCalculatorListener(this);

  recent_scaled_volume_calculator_.AddRecentScaledVolumeListener(1u, this);
  recent_scaled_volume_calculator_port_.AddRecentScaledVolumeListener(2u, this);

  volume_ratio_calculator_.AddVolumeRatioListener(1u, this);
  volume_ratio_calculator_port_.AddVolumeRatioListener(2u, this);
}

void SrcSwitchRegPort::OnVolumeRatioUpdate(const unsigned int r_security_id_, const double& r_new_volume_ratio_) {
  if (r_security_id_ == 1) {
    current_volume_ratio_dep_ = std::pow(r_new_volume_ratio_, volume_exponent_);
    current_volume_ratio_dep_ = std::max(1.0, std::min(current_volume_ratio_dep_, 2.0));
    volume_factor_dep_ = current_volume_ratio_dep_ * scaled_volume_dep_;
  } else {
    current_volume_ratio_port_ = std::pow(r_new_volume_ratio_, volume_exponent_);
    current_volume_ratio_port_ = std::max(1.0, std::min(current_volume_ratio_dep_, 2.0));
    volume_factor_port_ = current_volume_ratio_port_ * scaled_volume_port_;
  }
}

void SrcSwitchRegPort::OnScaledVolumeUpdate(const unsigned int r_security_id_, const double& r_new_scaled_volume_) {
  if (r_security_id_ == 1) {
    scaled_volume_dep_ = std::pow(r_new_scaled_volume_, volume_exponent_);
    scaled_volume_dep_ = std::max(0.7, std::min(scaled_volume_dep_, 2.0));
    volume_factor_dep_ = current_volume_ratio_dep_ * scaled_volume_dep_;
  } else {
    scaled_volume_port_ = std::pow(r_new_scaled_volume_, volume_exponent_);
    scaled_volume_port_ = std::max(0.7, std::min(scaled_volume_port_, 2.0));
    volume_factor_port_ = current_volume_ratio_port_ * scaled_volume_port_;
  }
}

void SrcSwitchRegPort::OnStdevUpdate(const double& _new_stdev_value_) {
  port_stdev_ = _new_stdev_value_ / port_mean_stdev_;
}

void SrcSwitchRegPort::OnStdevUpdate(const unsigned int _security_id_, const double& _new_stdev_value_) {
  dep_stdev_ = _new_stdev_value_ / dep_mean_stdev_;
}

void SrcSwitchRegPort::OnTimePeriodUpdate(const int num_pages_to_add_) {
  if (!is_ready_) {
    if (dep_market_view_.is_ready_complex(2) && indep_portfolio_price_.is_ready()) {
      is_ready_ = true;
      InitializeValues();
    }
  } else {
    // std::cout << "port_stdev_ " << port_stdev_ << " volume_factor_port_: " << volume_factor_port_ << " dep_stdev_: "
    // << dep_stdev_ << " volume_factor_dep_: " << volume_factor_dep_ << std::endl;
    if ((port_stdev_ * volume_factor_port_ > port_thresh_ || volume_factor_port_ > 2.0) && indicator_value_ == 2) {
      indicator_value_ = 1;  // high external movement and/or volume
      NotifyIndicatorListeners(indicator_value_);
    } else if ((port_stdev_ * volume_factor_port_ < (1 - tolerance_) * port_thresh_ &&
                volume_factor_port_ < (1 - tolerance_) * 2.0) &&
               indicator_value_ == 1) {
      if (volume_factor_port_ < 1.25) {
        if (dep_stdev_ * volume_factor_dep_ > dep_thresh_) {
          indicator_value_ = 2;
          NotifyIndicatorListeners(indicator_value_);
        }
      } else if (dep_stdev_ >= port_stdev_ && volume_factor_dep_ > 1.25 * volume_factor_port_) {
        indicator_value_ = 2;
        NotifyIndicatorListeners(indicator_value_);
      }
    }
  }
}

void SrcSwitchRegPort::WhyNotReady() {
  if (!is_ready_) {
    if (!(dep_market_view_.is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << dep_market_view_.secname() << " is_ready_complex(2) = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
  }
}

// market_interrupt_listener interface
void SrcSwitchRegPort::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {
  if (_security_id_ == dep_market_view_.security_id()) {
    data_interrupted_ = true;
    indicator_value_ = 2;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void SrcSwitchRegPort::OnMarketDataResumed(const unsigned int _security_id_) {
  if (_security_id_ == dep_market_view_.security_id()) {
    data_interrupted_ = false;
  }
}

void SrcSwitchRegPort::InitializeValues() {
  indicator_value_ = 2;
  NotifyIndicatorListeners(indicator_value_);
}
}
