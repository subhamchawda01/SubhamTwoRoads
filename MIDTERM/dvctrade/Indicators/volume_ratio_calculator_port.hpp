/**
    \file Indicators/volume_ratio_calculator_port.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#pragma once

#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvctrade/Indicators/volume_ratio_listener.hpp"
#include "dvctrade/Indicators/volume_ratio_calculator.hpp"

namespace HFSAT {

class VolumeRatioCalculatorPort : public VolumeRatioListener {
 protected:
  // variables
  DebugLogger& dbglogger_;
  const Watch& watch_;
  std::vector<SecurityMarketView*> indep_market_view_vec_;

  double volume_ratio_;
  std::vector<double> volume_ratio_vec_;
  std::vector<VolumeRatioCalculator*> volume_ratio_calculator_vec_;

  std::vector<double> security_id_weight_map_;
  std::vector<IndexedVolumeRatioListener> volume_ratio_listener_vec_;

  std::string concise_indicator_description_;

 protected:
  VolumeRatioCalculatorPort(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                            const std::string& concise_indicator_description_,
                            const std::string& portfolio_descriptor_shortcode_, double _fractional_seconds_);

 public:
  static VolumeRatioCalculatorPort* GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                      const std::string& portfolio_descriptor_shortcode_,
                                                      double _fractional_seconds_);

  ~VolumeRatioCalculatorPort(){};

  // listener interface
  void OnVolumeRatioUpdate(const unsigned int _security_id_, const double& _new_volume_ratio_);

  // functions
  static std::string VarName() { return "VolumeRatioCalculatorPort"; }
  inline double volume_ratio() const { return volume_ratio_; }

  inline void AddVolumeRatioListener(unsigned int index_, VolumeRatioListener* _new_listener_) {
    if (_new_listener_ != NULL) {
      bool found = false;
      for (auto i = 0u; i < volume_ratio_listener_vec_.size(); i++) {
        if (volume_ratio_listener_vec_[i].p_listener_ == _new_listener_) {
          found = true;
          break;
        }
      }
      if (!found) {
        volume_ratio_listener_vec_.push_back(IndexedVolumeRatioListener(index_, _new_listener_));
      }
    }
  }

  inline void RemoveVolumeRatioListener(VolumeRatioListener* _new_listener_) {
    auto _class_vec_iter_ = volume_ratio_listener_vec_.begin();
    for (; _class_vec_iter_ != volume_ratio_listener_vec_.end(); _class_vec_iter_++) {
      if (_class_vec_iter_->p_listener_ == _new_listener_) {
        _class_vec_iter_ = volume_ratio_listener_vec_.erase(_class_vec_iter_);
        break;
      }
    }
  }

  inline void NotifyListeners() {
    for (auto i = 0u; i < volume_ratio_listener_vec_.size(); i++) {
      (volume_ratio_listener_vec_[i].p_listener_)
          ->OnVolumeRatioUpdate(volume_ratio_listener_vec_[i].index_to_send_, volume_ratio_);
    }
  }
};
}
