/**
    \file IndicatorsCode/volume_ratio_calculator.cpp

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

#include "dvctrade/Indicators/volume_ratio_calculator.hpp"

namespace HFSAT {

void VolumeRatioCalculator::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                              std::vector<std::string>& _ors_source_needed_vec_,
                                              const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

VolumeRatioCalculator* VolumeRatioCalculator::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                                const std::vector<const char*>& r_tokens_,
                                                                PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _indep_market_view_ _num_levels_ _decay_factor_
  if (r_tokens_.size() < 5) {
    ExitVerbose(kModelCreationIndicatorLineLessArgs, t_dbglogger_,
                "VolumeRatioCalculator incorrect syntax. Should be INDICATOR _this_weight_ _indicator_string_ "
                "_indep_market_view_ _fractional_seconds_ ");
  }
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           atof(r_tokens_[4]));
}

VolumeRatioCalculator* VolumeRatioCalculator::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                                const SecurityMarketView& _indep_market_view_,
                                                                double _fractional_seconds_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _indep_market_view_.secname() << ' ' << _fractional_seconds_;
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, VolumeRatioCalculator*> concise_indicator_description_map_;

  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] = new VolumeRatioCalculator(
        t_dbglogger_, r_watch_, concise_indicator_description_, _indep_market_view_, _fractional_seconds_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

VolumeRatioCalculator::VolumeRatioCalculator(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                             const std::string& t_concise_indicator_description_,
                                             const SecurityMarketView& _indep_market_view_, double _fractional_seconds_)
    : CommonIndicator(t_dbglogger_, r_watch_, t_concise_indicator_description_),
      r_indep_market_view_(_indep_market_view_),
      p_recent_indep_volume_measure_(RecentSimpleVolumeMeasure::GetUniqueInstance(
          t_dbglogger_, r_watch_, _indep_market_view_, _fractional_seconds_)),
      fractional_seconds_(_fractional_seconds_),
      trading_volume_expected_(0),
      volume_ratio_(1.0),
      volume_ratio_listener_vec_(),
      utc_time_to_vol_map_() {
  p_recent_indep_volume_measure_->AddRecentSimpleVolumeListener(1u, this);
}

void VolumeRatioCalculator::Initialize() {
  DBGLOG_TIME_CLASS_FUNC_LINE << " Fetching SampleData  VOL " << r_indep_market_view_.shortcode() << " 60  "
                              << " st: " << trading_start_mfm_ << " end: " << trading_end_mfm_ << DBGLOG_ENDL_FLUSH;

  SampleDataUtil::GetAvgForPeriod(r_indep_market_view_.shortcode(), watch_.YYYYMMDD(), 60, trading_start_mfm_,
                                  trading_end_mfm_, "VOL", utc_time_to_vol_map_);

  for (auto it = utc_time_to_vol_map_.begin(); it != utc_time_to_vol_map_.end(); it++)
    trading_volume_expected_ += it->second;

  if (utc_time_to_vol_map_.empty()) {
    std::stringstream exit_stream_;
    exit_stream_ << "Volume Sample Data not found for: " << r_indep_market_view_.secname() << " "
                 << r_indep_market_view_.shortcode() << "\n";
    ExitVerbose(kExitErrorCodeZeroValue, dbglogger_, exit_stream_.str().c_str());
  }

  trading_volume_expected_ /= utc_time_to_vol_map_.size();  // average expected volume for 5 mins
  trading_volume_expected_ = trading_volume_expected_ * fractional_seconds_ / 300;
}

void VolumeRatioCalculator::OnVolumeUpdate(unsigned int t_index_to_send_, double _new_volume_value_) {
  if (!is_ready_) {
    if (r_indep_market_view_.is_ready_complex(2)) {
      is_ready_ = true;
      Initialize();
    }
  } else if (!data_interrupted_) {
    if (trading_volume_expected_ <= 0) {
      volume_ratio_ = 1.0;
    } else {
      volume_ratio_ = _new_volume_value_ / trading_volume_expected_;
    }
  }

  indicator_value_ = volume_ratio_;

  NotifyIndicatorListeners(indicator_value_);

  NotifyListeners();
}
void VolumeRatioCalculator::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                    const int msecs_since_last_receive_) {
  data_interrupted_ = true;
}

void VolumeRatioCalculator::OnMarketDataResumed(const unsigned int _security_id_) { data_interrupted_ = false; }
}
