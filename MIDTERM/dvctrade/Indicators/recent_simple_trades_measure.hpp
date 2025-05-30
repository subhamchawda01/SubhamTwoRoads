/**
    \file Indicators/recent_simple_trades_measure.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#pragma once

#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvctrade/Indicators/common_indicator.hpp"

namespace HFSAT {

class RecentSimpleTradesMeasure : public CommonIndicator, public TimePeriodListener {
 protected:
  // variables
  const SecurityMarketView& indep_market_view_;
  int pages_since_last_broadcast_;
  unsigned int num_pages_to_recall_;
  int new_page_value_;
  std::vector<int> past_tradess_;

  bool read_first_secs_;

 protected:
  RecentSimpleTradesMeasure(DebugLogger& _dbglogger_, const Watch& _watch_,
                            const std::string& concise_indicator_description_,
                            const SecurityMarketView& _indep_market_view_, double _fractional_seconds_);

 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);
  static RecentSimpleTradesMeasure* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                      const std::vector<const char*>& _tokens_,
                                                      PriceType_t _basepx_pxtype_);
  static RecentSimpleTradesMeasure* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                      const SecurityMarketView& _indep_market_view_,
                                                      double _fractional_seconds_);

  ~RecentSimpleTradesMeasure() {}

  // listener interface
  void OnTimePeriodUpdate(const int num_pages_to_add_);
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {}
  void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                    const MarketUpdateInfo& _market_update_info_);
  inline void OnPortfolioPriceChange(double _new_price_){};
  inline void OnPortfolioPriceReset(double _new_price_, double _old_price_, unsigned int is_data_interrupted_){};
  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);
  // functions

  // functions
  static std::string VarName() { return "RecentSimpleTradesMeasure"; }
  inline int recent_trades() const { return indicator_value_; }
};
}
