// =====================================================================================
//
//       Filename:  cxl_fast_orders.hpp
//
//    Description:  Uses Order Cancels for small orders
//
//        Version:  1.0
//        Created:  01/06/2015 03:41:47 PM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
//
//        Address:  Suite No 162, Evoma, #14, Bhattarhalli,
//                  Old Madras Road, Near Garden City College,
//                  KR Puram, Bangalore 560049, India
//          Phone:  +91 80 4190 3551
//
// =====================================================================================

#pragma once
#include "dvctrade/Indicators/common_indicator.hpp"
#include "baseinfra/MarketAdapter/base_market_view_manager.hpp"
#include "baseinfra/BaseUtils/eobi_fast_order_manager.hpp"

namespace HFSAT {
class FastOrdersDiffPx : public CommonIndicator {
 protected:
  const SecurityMarketView& indep_market_view_;
  HFSAT::SecurityNameIndexer& sec_name_indexer_;
  int fast_orders_size_;
  int window_usecs_;
  int num_fast_orders_;
  double min_price_increment_;
  double mid_price_;
  int fast_best_bid_int_price_;
  int fast_best_ask_int_price_;
  int fast_best_bid_size_;
  int fast_best_ask_size_;
  double fast_mkt_price_;
  double last_bid_price_;
  double last_ask_price_;
  PriceType_t base_price_type_;

 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);
  static FastOrdersDiffPx* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                             const std::vector<const char*>& _tokens_, PriceType_t _basepx_pxtype_);
  static FastOrdersDiffPx* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                             const SecurityMarketView& _indep_market_view_, int _window_msecs_,
                                             int _num_fast_orders_, int _orders_size_,
                                             PriceType_t t_target_price_type_);

 protected:
  FastOrdersDiffPx(DebugLogger& _dbglogger_, const Watch& _watch_, const std::string& concise_indicator_description_,
                   const SecurityMarketView& _indep_market_view_, int _window_msecs_, int _num_fast_orders_,
                   int _orders_size_, PriceType_t base_price_type_);

 public:
  ~FastOrdersDiffPx() {}

  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);

  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {
    OnMarketUpdate(_security_id_, _market_update_info_);
  }

  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_) {}

  static std::string VarName() { return "FastOrdersDiffPx"; }

  void WhyNotReady();
  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);

 protected:
  void InitializeValues();
};
}
