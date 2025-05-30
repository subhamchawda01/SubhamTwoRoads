/**
    \file Indicators/volume_ratio_calclator.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#pragma once
#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvctrade/Indicators/recent_simple_volume_measure.hpp"
#include "dvctrade/Indicators/volume_ratio_listener.hpp"

namespace HFSAT {

class VolumeRatioCalculator : public CommonIndicator, public RecentSimpleVolumeListener {
 protected:
  // variables
  const SecurityMarketView& r_indep_market_view_;

  RecentSimpleVolumeMeasure* const p_recent_indep_volume_measure_;

  double fractional_seconds_;
  double trading_volume_expected_;
  double volume_ratio_;

  std::vector<IndexedVolumeRatioListener> volume_ratio_listener_vec_;

  std::map<int, double> utc_time_to_vol_map_;

 protected:
  VolumeRatioCalculator(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                        const std::string& concise_indicator_description_,
                        const SecurityMarketView& t_indep_market_view_, double _fractional_seconds_);

 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);
  static VolumeRatioCalculator* GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                  const std::vector<const char*>& r_tokens_,
                                                  PriceType_t _basepx_pxtype_);
  static VolumeRatioCalculator* GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                  const SecurityMarketView& t_indep_market_view_,
                                                  double _fractional_seconds_);

  ~VolumeRatioCalculator(){};

  // listener interface
  void OnVolumeUpdate(unsigned int t_index_to_send_, double _new_volume_value_);

  // functions
  static std::string VarName() { return "VolumeRatioCalculator"; }
  inline double volume_ratio() const { return volume_ratio_; }

  inline void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  inline void OnMarketDataResumed(const unsigned int _security_id_);

  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {}
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {}
  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double _new_price_, double _old_price_, unsigned int is_data_interrupted_) {}

  void Initialize();

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
