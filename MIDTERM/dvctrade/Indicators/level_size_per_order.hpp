/**
  \file Indicators/level_size_per_order.hpp

  \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
Address:
Suite No 353, Evoma, #14, Bhattarhalli,
Old Madras Road, Near Garden City College,
KR Puram, Bangalore 560049, India
+91 80 4190 3551
 */

#ifndef BASE_INDICATORS_LEVEL_SIZE_PER_ORDER_H
#define BASE_INDICATORS_LEVEL_SIZE_PER_ORDER_H

#include "dvctrade/Indicators/common_indicator.hpp"
#include "baseinfra/MarketAdapter/base_market_view_manager.hpp"

namespace HFSAT {

/// Class returning the ask and bid sizes per order at a particular level in the order book
class LevelSizePerOrder : public CommonIndicator {
 protected:
  // variables
  SecurityMarketView& indep_market_view_;

  int book_level_;  // the level for which the size per order is required
  bool _is_ask;     // true when the ask size is required,false otherwise

  // functions
 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);
  static LevelSizePerOrder* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                              const std::vector<const char*>& _tokens_, bool _is_ask);
  static LevelSizePerOrder* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                              SecurityMarketView& _indep_market_view_, int book_level_, bool _is_ask);

 protected:
  LevelSizePerOrder(DebugLogger& _dbglogger_, const Watch& _watch_, const std::string& concise_indicator_description_,
                    SecurityMarketView& _indep_market_view_, int book_level_, bool _is_ask);

 public:
  ~LevelSizePerOrder() {}

  // listener interface
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {
    OnMarketUpdate(_security_id_, _market_update_info_);
  }

  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_) {}
  // functions
  static std::string VarName() { return "LevelSizePerOrder"; }

  void WhyNotReady();

  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);

 protected:
  void InitializeValues();
};
}

#endif  // BASE_INDICATORS_L1_SIZE_TREND_H
