/**
   \file Indicators/current_time.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 351, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/

#pragma once

#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvctrade/Indicators/simple_trend.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"

namespace HFSAT {

class CurrentTime : public CommonIndicator, public TimePeriodListener {
 protected:
  CurrentTime(DebugLogger& t_dbglogger, const Watch& t_watch, const std::string& t_concise_indicator_description);

 public:
  static void CollectShortCodes(std::vector<std::string>& t_shortcodes_affecting_this_indicator,
                                std::vector<std::string>& t_ors_source_needed_vec,
                                const std::vector<const char*>& t_tokens);

  static CurrentTime* GetUniqueInstance(DebugLogger& t_dbglogger, const Watch& t_watch,
                                        const std::vector<const char*>& t_tokens, PriceType_t t_basepx_pxtype);

  static CurrentTime* GetUniqueInstance(DebugLogger& t_dbglogger, const Watch& t_watch);

  ~CurrentTime() {}

  // listener interface
  void OnTimePeriodUpdate(const int t_num_pages_to_add);

  void OnMarketUpdate(const unsigned int t_security_id, const MarketUpdateInfo& t_market_update_info);
  void OnTradePrint(const unsigned int t_security_id, const TradePrintInfo& t_trade_print_info,
                    const MarketUpdateInfo& t_market_update_info);
  inline void OnPortfolioPriceChange(double t_new_price){};
  inline void OnPortfolioPriceReset(double t_new_price, double t_old_price, unsigned int t_is_data_interrupted){};

  void OnIndicatorUpdate(const unsigned int& t_indicator_index, const double& t_new_value);
  inline void OnIndicatorUpdate(const unsigned int& t_indicator_index, const double& t_new_value_decrease,
                                const double& t_new_value_nochange, const double& t_new_value_increase) {
    return;
  }

  // functions
  /// Used in ModelCreator to see which shortcodes are core
  bool GetReadinessRequired(const std::string& r_dep_shortcode_, const std::vector<const char*>& tokens_) const {
    return false;
  }

  static std::string VarName() { return "CurrentTime"; }

  inline void SubscribeDataInterrupts(MarketUpdateManager& market_update_manager_) {
    market_update_manager_.AddMarketDataInterruptedListener(this);
  }

  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);
};
}
