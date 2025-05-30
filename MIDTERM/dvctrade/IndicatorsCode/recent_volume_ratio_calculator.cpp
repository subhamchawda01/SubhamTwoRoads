/**
    \file IndicatorsCode/recent_volume_ratio_calculator.cpp

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

#include "dvctrade/Indicators/recent_volume_ratio_calculator.hpp"

namespace HFSAT {

RecentVolumeRatioCalculator* RecentVolumeRatioCalculator::GetUniqueInstance(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, const SecurityMarketView& _dep_market_view_,
    const SecurityMarketView& _indep_market_view_, double _fractional_seconds_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _dep_market_view_.secname() << ' ' << _dep_market_view_.secname() << ' '
              << _indep_market_view_.secname() << ' ' << _fractional_seconds_;
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, RecentVolumeRatioCalculator*> concise_indicator_description_map_;

  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new RecentVolumeRatioCalculator(t_dbglogger_, r_watch_, concise_indicator_description_, _dep_market_view_,
                                        _indep_market_view_, _fractional_seconds_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

RecentVolumeRatioCalculator::RecentVolumeRatioCalculator(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                         const std::string& t_concise_indicator_description_,
                                                         const SecurityMarketView& _dep_market_view_,
                                                         const SecurityMarketView& _indep_market_view_,
                                                         double _fractional_seconds_)
    : dbglogger_(t_dbglogger_),
      watch_(r_watch_),
      concise_indicator_description_(t_concise_indicator_description_),
      p_recent_dep_volume_measure_(
          RecentVolumeMeasure::GetUniqueInstance(t_dbglogger_, r_watch_, _dep_market_view_, _fractional_seconds_)),
      p_recent_indep_volume_measure_(
          RecentVolumeMeasure::GetUniqueInstance(t_dbglogger_, r_watch_, _indep_market_view_, _fractional_seconds_)),
      last_ratio_updated_msecs_(0),
      recent_volume_ratio_(1.0) {
  r_watch_.subscribe_BigTimePeriod(this);  // for UpdateLRVolumeInfo and updating volume adjustment
  NotifyListeners();
}

void RecentVolumeRatioCalculator::OnTimePeriodUpdate(const int num_pages_to_add_) {
  if ((last_ratio_updated_msecs_ == 0) ||
      (watch_.msecs_from_midnight() - last_ratio_updated_msecs_ > TWOMINUTESMSECS)) {
    double dep_rvm_ = p_recent_dep_volume_measure_->recent_volume();
    double indep_rvm_ = p_recent_indep_volume_measure_->recent_volume();

#define MIN_RATIO 0.02
#define MAX_RATIO 50.0

    if ((dep_rvm_ > 0) && (indep_rvm_ > 0)) {
      recent_volume_ratio_ = sqrt(std::max(MIN_RATIO, std::min(MAX_RATIO, (indep_rvm_ / dep_rvm_))));
      NotifyListeners();

      if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        DBGLOG_TIME << concise_indicator_description_ << " Current : " << recent_volume_ratio_ << " Dep: " << dep_rvm_
                    << " Indep: " << indep_rvm_ << DBGLOG_ENDL_FLUSH;
      }
    }

#undef MAX_RATIO
#undef MIN_RATIO

    last_ratio_updated_msecs_ = watch_.msecs_from_midnight() - (watch_.msecs_from_midnight() % TWOMINUTESMSECS);
  }
}
}
