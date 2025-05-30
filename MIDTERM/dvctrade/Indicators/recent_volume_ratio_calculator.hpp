/**
    \file Indicators/recent_volume_ratio_calculator.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_INDICATORS_RECENT_VOLUME_RATIO_CALCULATOR_H
#define BASE_INDICATORS_RECENT_VOLUME_RATIO_CALCULATOR_H

#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvctrade/Indicators/recent_volume_measure.hpp"

namespace HFSAT {

class RecentVolumeRatioListener {
 public:
  virtual ~RecentVolumeRatioListener(){};
  virtual void OnVolumeRatioUpdate(const double& r_new_volume_ratio_value_) = 0;
};

typedef std::vector<RecentVolumeRatioListener*> RecentVolumeRatioListenerPtrVec;
typedef std::vector<RecentVolumeRatioListener*>::const_iterator RecentVolumeRatioListenerPtrVecCIter_t;
typedef std::vector<RecentVolumeRatioListener*>::iterator RecentVolumeRatioListenerPtrVecIter_t;

class RecentVolumeRatioCalculator : public TimePeriodListener {
 protected:
  // variables
  DebugLogger& dbglogger_;
  const Watch& watch_;
  std::string concise_indicator_description_;

  const RecentVolumeMeasure* const p_recent_dep_volume_measure_;
  const RecentVolumeMeasure* const p_recent_indep_volume_measure_;

  int last_ratio_updated_msecs_;
  double recent_volume_ratio_;

  RecentVolumeRatioListenerPtrVec recent_volume_measure_listener_ptr_vec_;

 protected:
  RecentVolumeRatioCalculator(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                              const std::string& concise_indicator_description_,
                              const SecurityMarketView& t_dep_market_view_,
                              const SecurityMarketView& t_indep_market_view_, double _fractional_seconds_);

 public:
  static RecentVolumeRatioCalculator* GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                        const SecurityMarketView& t_dep_market_view_,
                                                        const SecurityMarketView& t_indep_market_view_,
                                                        double _fractional_seconds_);

  ~RecentVolumeRatioCalculator(){};

  // listener interface
  void OnTimePeriodUpdate(const int num_pages_to_add_);

  // functions
  static std::string VarName() { return "RecentVolumeRatioCalculator"; }
  inline double recent_volume_ratio() const { return recent_volume_ratio_; }

  inline void AddRecentVolumeRatioListener(RecentVolumeRatioListener* _new_listener_) {
    if (_new_listener_ != NULL) {
      VectorUtils::UniqueVectorAdd(recent_volume_measure_listener_ptr_vec_, _new_listener_);
    }
  }

  inline void RemoveRecentVolumeRatioListener(RecentVolumeRatioListener* _new_listener_) {
    VectorUtils::UniqueVectorRemove(recent_volume_measure_listener_ptr_vec_, _new_listener_);
  }

  inline void NotifyListeners() {
    for (auto i = 0u; i < recent_volume_measure_listener_ptr_vec_.size(); i++) {
      recent_volume_measure_listener_ptr_vec_[i]->OnVolumeRatioUpdate(recent_volume_ratio_);
    }
  }
};
}

#endif  // BASE_INDICATORS_RECENT_VOLUME_RATIO_CALCULATOR_H
