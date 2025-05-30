#pragma once

#include "dvccode/CDef/math_utils.hpp"
#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvctrade/Indicators/pcaport_price.hpp"
#include "dvctrade/Indicators/offline_returns_lrdb.hpp"
#include "dvctrade/Indicators/simple_trend.hpp"
#include "dvctrade/Indicators/dynamic_weight_port_trend.hpp"

namespace HFSAT {

class OfflineComputedPairsDynamicWeightPort : public IndicatorListener,
                                              public CommonIndicator,
                                              public TimePeriodListener {
 protected:
  // variables
  const SecurityMarketView& dep_market_view_;

  PCAPortPrice& indep_portfolio_price_;

  double dep_price_trend_;
  double indep_price_trend_;

  SimpleTrend* p_dep_indicator_;
  DynamicWeightPortTrend* p_indep_indicator_;

  OfflineReturnsLRDB& lrdb_;
  int last_lrinfo_updated_msecs_;
  LRInfo current_lrinfo_;
  double current_projection_multiplier_;
  double current_projected_trend_;
  bool indep_interrupted_;
  bool dep_interrupted_;
  // functions
 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static OfflineComputedPairsDynamicWeightPort* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                                  const std::vector<const char*>& _tokens_,
                                                                  PriceType_t _basepx_pxtype_);

  static OfflineComputedPairsDynamicWeightPort* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                                  SecurityMarketView& _dep_market_view_,
                                                                  std::string _portfolio_descriptor_shortcode_,
                                                                  std::string _stdev_vol_, double _ratio_duration_,
                                                                  double _trend_duration_, PriceType_t _price_type_);

 protected:
  OfflineComputedPairsDynamicWeightPort(DebugLogger& _dbglogger_, const Watch& _watch_,
                                        const std::string& concise_indicator_description_,
                                        SecurityMarketView& _dep_market_view_,
                                        std::string _portfolio_descriptor_shortcode_, std::string _stdev_vol_,
                                        double _ratio_duration_, double _trend_duration_, PriceType_t _price_type_);

 public:
  ~OfflineComputedPairsDynamicWeightPort() {}

  // listener interface
  void OnTimePeriodUpdate(const int num_pages_to_add_) { UpdateLRInfo(); }

  inline void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_){};
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_){};

  inline void OnPortfolioPriceChange(double _new_price_){};
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_){};

  inline void SubscribeDataInterrupts(MarketUpdateManager& market_update_manager_) {
    market_update_manager_.AddMarketDataInterruptedListener(&indep_portfolio_price_);
    if (p_indep_indicator_ != NULL) {
      market_update_manager_.AddMarketDataInterruptedListener(p_indep_indicator_);
    }
  }
  void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_);
  inline void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& new_value_decrease_,
                                const double& new_value_nochange_, const double& new_value_increase_) {
    return;
  }

  inline void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  inline void OnMarketDataResumed(const unsigned int _security_id_);
  // functions
  /// Used in ModelCreator to see which variable is in the model file
  static std::string VarName() { return "OfflineComputedPairsDynamicWeightPort"; }

 protected:
  void InitializeValues();
  void UpdateLRInfo();

  inline void ComputeMultiplier() { current_projection_multiplier_ = current_lrinfo_.lr_coeff_; }
};
}
