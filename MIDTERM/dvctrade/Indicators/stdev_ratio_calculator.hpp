/**
    \file Indicators/stdev_ratio_calclator.hpp

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
#include "dvctrade/Indicators/slow_stdev_calculator.hpp"
#include "dvctrade/Indicators/stdev_ratio_listener.hpp"

namespace HFSAT {

class StdevRatioCalculator : public CommonIndicator, public SlowStdevCalculatorListener, public TimePeriodListener {
 protected:
  // variables
  const SecurityMarketView& indep_market_view_;

  SlowStdevCalculator* slow_stdev_calculator_;

  double scaling_factor_;
  double stdev_expected_;
  double stdev_ratio_;

  int last_expected_stdev_updated_msecs_;

  bool scaled_vol_;
  std::vector<IndexedStdevRatioListener> stdev_ratio_listener_vec_;

  std::map<int, double> utc_time_to_stdev_map_;

 protected:
  StdevRatioCalculator(DebugLogger& dbglogger, const Watch& watch, const std::string& concise_indicator_description,
                       const SecurityMarketView& indep_market_view, double fractional_seconds, bool scaled_vol);

 public:
  static void CollectShortCodes(std::vector<std::string>& shortcodes_affecting_this_indicator,
                                std::vector<std::string>& ors_source_needed_vec,
                                const std::vector<const char*>& tokens);
  static StdevRatioCalculator* GetUniqueInstance(DebugLogger& dbglogger, const Watch& watch,
                                                 const std::vector<const char*>& tokens, PriceType_t basepx_pxtype);
  static StdevRatioCalculator* GetUniqueInstance(DebugLogger& dbglogger, const Watch& watch,
                                                 const SecurityMarketView& indep_market_view, double fractional_seconds,
                                                 bool scaled_vol);

  ~StdevRatioCalculator(){};

  // listener interface
  void OnStdevUpdate(const unsigned int security_id, const double& new_stdev_value);

  // functions
  static std::string VarName() { return "StdevRatioCalculator"; }
  inline double stdev_ratio() const { return stdev_ratio_; }

  void Initialize();

  inline void OnMarketDataInterrupted(const unsigned int security_id, const int msecs_since_last_receive);
  inline void OnMarketDataResumed(const unsigned int security_id);

  void OnTimePeriodUpdate(const int num_pages_to_add);
  void OnMarketUpdate(const unsigned int security_id, const MarketUpdateInfo& market_update_info) {}
  inline void OnTradePrint(const unsigned int security_id, const TradePrintInfo& trade_print_info,
                           const MarketUpdateInfo& market_update_info) {}
  inline void OnPortfolioPriceChange(double new_price) {}
  inline void OnPortfolioPriceReset(double new_price, double old_price, unsigned int is_data_interrupted_) {}

  inline void AddStdevRatioListener(unsigned int index, StdevRatioListener* new_listener) {
    if (new_listener != NULL) {
      bool found = false;
      for (auto i = 0u; i < stdev_ratio_listener_vec_.size(); i++) {
        if (stdev_ratio_listener_vec_[i].p_listener_ == new_listener) {
          found = true;
          break;
        }
      }
      if (!found) {
        stdev_ratio_listener_vec_.push_back(IndexedStdevRatioListener(index, new_listener));
      }
    }
  }

  inline void RemoveStdevRatioListener(StdevRatioListener* new_listener) {
    auto class_vec_iter = stdev_ratio_listener_vec_.begin();
    for (; class_vec_iter != stdev_ratio_listener_vec_.end(); class_vec_iter++) {
      if (class_vec_iter->p_listener_ == new_listener) {
        class_vec_iter = stdev_ratio_listener_vec_.erase(class_vec_iter);
        break;
      }
    }
  }

  inline void NotifyListeners() {
    for (auto i = 0u; i < stdev_ratio_listener_vec_.size(); i++) {
      (stdev_ratio_listener_vec_[i].p_listener_)
          ->OnStdevRatioUpdate(stdev_ratio_listener_vec_[i].index_to_send_, stdev_ratio_);
    }
  }
};
}
