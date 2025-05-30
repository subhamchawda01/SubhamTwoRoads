/**
    \file Indicators/notional_traded.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#pragma once

#include "dvctrade/Indicators/common_indicator.hpp"
#include "baseinfra/TradeUtils/time_decayed_trade_info_manager.hpp"

namespace HFSAT {

/**
 * This indicator gives the exponentially decayed notional traded amount during specified duration
 *
 */
class NotionalTraded : public CommonIndicator {
 protected:
  // variables
  const SecurityMarketView& dep_market_view_;
  TimeDecayedTradeInfoManager& dep_time_decayed_trade_info_manager_;
  double dep_n2d_;

  // functions
 protected:
  NotionalTraded(DebugLogger& _dbglogger_, const Watch& _watch_, const std::string& concise_indicator_description_,
                 SecurityMarketView& _dep_market_view_, double _trade_duration_);

 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static NotionalTraded* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                           const std::vector<const char*>& _tokens_, PriceType_t _basepx_pxtype_);

  static NotionalTraded* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                           SecurityMarketView& _dep_market_view_, double _trade_duration_);

  ~NotionalTraded() {}

  inline void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_){};
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_);
  inline void OnPortfolioPriceChange(double _new_price_){};
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_){};
  inline void SubscribeDataInterrupts(MarketUpdateManager& market_update_manager_) {}

  static std::string VarName() { return "NotionalTraded"; }

  void WhyNotReady();

  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {}
  void OnMarketDataResumed(const unsigned int _security_id_) {}

 protected:
  void InitializeValues();
};
}
