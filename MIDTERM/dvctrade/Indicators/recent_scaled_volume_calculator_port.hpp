/**
    \file Indicators/recent_scaled_volume_calclator.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#pragma once

#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvctrade/Indicators/recent_scaled_volume_listener.hpp"
#include "dvctrade/Indicators/recent_scaled_volume_calculator.hpp"

namespace HFSAT {

class RecentScaledVolumeCalculatorPort : public RecentScaledVolumeListener {
 protected:
  // variables
  DebugLogger& dbglogger_;
  const Watch& watch_;
  std::vector<SecurityMarketView*> indep_market_view_vec_;

  double scaled_volume_;
  std::vector<double> scaled_volume_vec_;
  std::vector<RecentScaledVolumeCalculator*> scaled_volume_calculator_vec_;

  std::vector<double> security_id_weight_map_;
  std::vector<IndexedRecentScaledVolumeListener> recent_scaled_volume_listener_vec_;
  std::string concise_indicator_description_;

 protected:
  RecentScaledVolumeCalculatorPort(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                   const std::string& t_concise_indicator_description_,
                                   const std::string& t_portfolio_descriptor_shortcode_, double _fractional_seconds_);

 public:
  static RecentScaledVolumeCalculatorPort* GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                             const std::string& t_portfolio_descriptor_shortcode_,
                                                             double _fractional_seconds_);

  ~RecentScaledVolumeCalculatorPort(){};

  // listener interface
  void OnScaledVolumeUpdate(const unsigned int r_security_id_, const double& r_new_scaled_volume_);

  // functions
  static std::string VarName() { return "RecentScaledVolumeCalculatorPort"; }
  inline double scaled_volume() const { return scaled_volume_; }

  inline void AddRecentScaledVolumeListener(unsigned int index_, RecentScaledVolumeListener* _new_listener_) {
    if (_new_listener_ != NULL) {
      bool found = false;
      for (auto i = 0u; i < recent_scaled_volume_listener_vec_.size(); i++) {
        if (recent_scaled_volume_listener_vec_[i].p_listener_ == _new_listener_) {
          found = true;
          break;
        }
      }
      if (!found) {
        recent_scaled_volume_listener_vec_.push_back(IndexedRecentScaledVolumeListener(index_, _new_listener_));
      }
    }
  }

  inline void RemoveRecentScaledVolumeListener(RecentScaledVolumeListener* _new_listener_) {
    auto _class_vec_iter_ = recent_scaled_volume_listener_vec_.begin();
    for (; _class_vec_iter_ != recent_scaled_volume_listener_vec_.end(); _class_vec_iter_++) {
      if (_class_vec_iter_->p_listener_ == _new_listener_) {
        _class_vec_iter_ = recent_scaled_volume_listener_vec_.erase(_class_vec_iter_);
        break;
      }
    }
  }

  inline void NotifyListeners() {
    for (auto i = 0u; i < recent_scaled_volume_listener_vec_.size(); i++) {
      (recent_scaled_volume_listener_vec_[i].p_listener_)
          ->OnScaledVolumeUpdate(recent_scaled_volume_listener_vec_[i].index_to_send_, scaled_volume_);
    }
  }
};
}
