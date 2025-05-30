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
#include "dvctrade/Indicators/recent_simple_volume_measure.hpp"
#include "dvctrade/Indicators/recent_scaled_volume_listener.hpp"
#include "dvctrade/Indicators/common_indicator.hpp"

namespace HFSAT {

class RecentScaledVolumeCalculator : public CommonIndicator,
                                     public TimePeriodListener,
                                     public RecentSimpleVolumeListener {
 protected:
  // variables
  const SecurityMarketView& r_indep_market_view_;

  RecentSimpleVolumeMeasure* const p_recent_indep_volume_measure_;

  double linear_time_scaling_factor_;
  int last_expected_volume_updated_msecs_;
  double indep_rvm_;
  double trading_volume_expected_;
  double scaled_volume_;

  std::vector<IndexedRecentScaledVolumeListener> recent_scaled_volume_listener_vec_;

  std::map<int, double> utc_time_to_vol_map_;

 protected:
  RecentScaledVolumeCalculator(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                               const std::string& concise_indicator_description_,
                               const SecurityMarketView& t_indep_market_view_, double _fractional_seconds_);

 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);
  static RecentScaledVolumeCalculator* GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                         const std::vector<const char*>& r_tokens_,
                                                         PriceType_t _basepx_pxtype_);
  static RecentScaledVolumeCalculator* GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                         const SecurityMarketView& t_indep_market_view_,
                                                         double _fractional_seconds_);

  ~RecentScaledVolumeCalculator(){};

  // listener interface
  void OnTimePeriodUpdate(const int num_pages_to_add_);
  void OnVolumeUpdate(unsigned int t_index_, double _new_volume_value_);

  // functions
  static std::string VarName() { return "RecentScaledVolumeCalculator"; }
  inline double scaled_volume() const { return scaled_volume_; }
  void Initialize();

  inline void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {}
  inline void OnMarketDataResumed(const unsigned int _security_id_) {}

  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {}
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {}
  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double _new_price_, double _old_price_, unsigned int is_data_interrupted_) {}

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
