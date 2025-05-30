/**
    \file Indicators/diff_price_type.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_INDICATORS_EVENT_OWP_DIFF_PRICE_L1_H
#define BASE_INDICATORS_EVENT_OWP_DIFF_PRICE_L1_H

#include "baseinfra/MarketAdapter/security_market_view.hpp"
#include "dvctrade/Indicators/common_indicator.hpp"
#include "baseinfra/TradeUtils/time_decayed_trade_info_manager.hpp"

namespace HFSAT {

/// Indicator returns indep_market_view_::price_type_ - indep_market_view_::basepx_pxtype_
/// Also basepx_pxtype_ changes by set_basepx_pxtype ( ) call
class EventOwpDiffPriceL1 : public CommonIndicator, public TimePeriodListener {
 protected:
  TimeDecayedTradeInfoManager& time_decayed_trade_info_manager_;
  SecurityMarketView& indep_market_view_;
  PriceType_t price_type_;
  const Watch& watch_;
  unsigned int last_new_page_msecs_;
  unsigned int page_width_msecs_;
  double alpha_;
  unsigned long long last_recorded_l1events_;
  unsigned int current_l1events_avg_;
  bool first_time_;
  int avg_l1size_;
  double trade_price_avg_;
  double avg_l1_events_per_sec_;
  double k_factor_;
  int max_significant_trade_sz_;
  int trade_hist_msecs_;
  double dyn_factor_;
  bool hist_l1_info_found_;
  unsigned long long last_level_shift_events_;
  int last_bid_int_price_;
  int last_ask_int_price_;

 public:
  // functions

  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static EventOwpDiffPriceL1* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                const std::vector<const char*>& _tokens_, PriceType_t _basepx_pxtype_);

  static EventOwpDiffPriceL1* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                SecurityMarketView& _indep_market_view_, double _fractional_seconds_,
                                                PriceType_t _price_type_);

  EventOwpDiffPriceL1(DebugLogger& _dbglogger_, const Watch& _watch_, const std::string& concise_indicator_description_,
                      SecurityMarketView& _indep_market_view_, double _fractional_seconds_, PriceType_t _price_type_);

  ~EventOwpDiffPriceL1() {}

  void Initialize();
  // listener interface
  void OnTimePeriodUpdate(const int num_pages_to_add_);
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);
  void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                    const MarketUpdateInfo& _market_update_info_);
  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_){};

  // functions
  static std::string VarName() { return "EventOwpDiffPriceL1"; }
  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);

 protected:
};
}

#endif  // BASE_INDICATORS_EVENT_OWP_DIFF_PRICE_L1_H
