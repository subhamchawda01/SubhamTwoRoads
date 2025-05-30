/**
    \file Indicators/self_position.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#pragma once

#include "dvctrade/Indicators/common_indicator.hpp"

namespace HFSAT {

/// Risk based indicator. Tries to change trading either through the target price
///  or through the strategy or ExecLogic
/// Returns our(self) position on a product (shortcode) that is likely to be very correlated to the
/// the dependant of the model or the priduct traded
/// Perhaps getting a good coefficient for this indicator in the model is going to be impractical from modeling logic
/// It might best be incporporated by trial and error in strategy.
class SelfPosition : public CommonIndicator {
 protected:
 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static SelfPosition* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                         const std::vector<const char*>& _tokens_, PriceType_t _basepx_pxtype_);

  static SelfPosition* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                         PromOrderManager& _indep_prom_order_manager_, PriceType_t _basepx_pxtype_);

 protected:
  SelfPosition(DebugLogger& _dbglogger_, const Watch& _watch_, const std::string& concise_indicator_description_,
               PromOrderManager& _indep_prom_order_manager_);

 public:
  ~SelfPosition() {}

  // listener interface
  inline void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {}
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {}

  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_) {}

  void OnGlobalPositionChange(const unsigned int _security_id_, int _new_global_position_);

  inline void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {}
  inline void OnMarketDataResumed(const unsigned int _security_id_) {}
  // functions

  // functions
  static std::string VarName() { return "SelfPosition"; }

 protected:
};
}
