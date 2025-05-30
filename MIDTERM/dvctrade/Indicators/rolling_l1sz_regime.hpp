/**
   \file Indicators/rolling_l1sz_regime.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#pragma once

#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvctrade/Indicators/l1_size_trend.hpp"
#include "dvctrade/Indicators/indicator_listener.hpp"

namespace HFSAT {

/// regime based on rolling avg l1 size as computed by l1_size_trend.
/// parameter is duration for averaging and threshold for regime ( currently two regimes supported )
class RollingL1SzRegime : public CommonIndicator, public IndicatorListener {
 protected:
  // variables
  const SecurityMarketView& dep_market_view_;
  L1SizeTrend* l1_trend_var_;
  double tolerance_;  // to induce hysterisis in indicator value if desired
  double threshold_;  // threshold value for l1size

 protected:
  RollingL1SzRegime(DebugLogger& _dbglogger_, const Watch& _watch_, const std::string& concise_indicator_description_,
                    const SecurityMarketView& _dep_market_view_, double time_length_, double thresh_, double tolerance_,
                    PriceType_t basepx_pxtype_);

 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static RollingL1SzRegime* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                              const std::vector<const char*>& _tokens_, PriceType_t _basepx_pxtype_);

  static RollingL1SzRegime* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                              const SecurityMarketView& _dep_market_view_, double time_length_,
                                              double thresh_, double tolerance_, PriceType_t basepx_pxtype_);

  ~RollingL1SzRegime() {}

  // listener interface
  void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_);

  // functions
  /// Used in ModelCreator to see which shortcodes are core
  bool GetReadinessRequired(const std::string& r_dep_shortcode_, const std::vector<const char*>& tokens_) const {
    std::vector<std::string> core_shortcodes_;
    GetCoreShortcodes(r_dep_shortcode_, core_shortcodes_);
    // here tokes 3 and 4 are important
    // if ( ( tokens_.size() > 3u ) &&
    // 	   ( VectorUtils::LinearSearchValue ( core_shortcodes_, std::string(tokens_[3]) ) ) )
    // 	{ return true ; }
    if ((tokens_.size() > 4u) && (VectorUtils::LinearSearchValue(core_shortcodes_, std::string(tokens_[4])))) {
      return true;
    }
    return false;
  }

  static std::string VarName() { return "RollingL1SzRegime"; }

  inline void SubscribeDataInterrupts(MarketUpdateManager& market_update_manager_) {
    market_update_manager_.AddMarketDataInterruptedListener(this);
  }

  /// following are not important for this indicator
  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  ;
  void OnMarketDataResumed(const unsigned int _security_id_);
  ;
  void OnPortfolioPriceChange(double _new_price_){};
  void OnPortfolioPriceReset(double t_new_value_, double t_old_price_, unsigned int is_data_interrupted_){};
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_){};
  void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                    const MarketUpdateInfo& _market_update_info_){};
  inline void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& new_value_decrease_,
                                const double& new_value_nochange_, const double& new_value_increase_) {
    return;
  }

 protected:
  void InitializeValues();
};
}
