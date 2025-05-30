/**
    \file Indicators/amplify_level_change.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#pragma once

#include "dvctrade/Indicators/common_indicator.hpp"
#include "baseinfra/MarketAdapter/base_market_view_manager.hpp"

namespace HFSAT {

/// Class returning projected future price minus current price
/// Currently the projection is very rule based
class AmplifyLevelChange : public CommonIndicator {
 protected:
  // variables
  const SecurityMarketView& indep_market_view_;

  int stability_msecs_;  // the number of msecs since newlevel at which point things are most probably stable
  int growth_size_;      // the size that if crossed while increasing from 0 will accelerate to stable_value_
  int depletion_size_;   // the size that if crossed while falling from mid will accelerate to 0
  double stable_value_;  // the price above bid and ask to push the size to, when accelerating up from 0

  struct SizeIntPrice_t {
    SizeIntPrice_t(int a, int b, unsigned int c) : size_(a), int_price_(b), time_stamp_(c) {}
    int size_;
    int int_price_;
    int time_stamp_;
  };
  std::vector<SizeIntPrice_t> bid_quotes_;
  std::vector<SizeIntPrice_t> ask_quotes_;

  unsigned bid_start_index_;
  unsigned bid_end_index_;
  unsigned ask_start_index_;
  unsigned ask_end_index_;

  // functions
 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static AmplifyLevelChange* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                               const std::vector<const char*>& _tokens_, PriceType_t _basepx_pxtype_);

  static AmplifyLevelChange* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                               const SecurityMarketView& _indep_market_view_,
                                               double _fractional_seconds_, int t_growth_size_, int t_depletion_size_,
                                               double t_stable_value_);

 protected:
  AmplifyLevelChange(DebugLogger& _dbglogger_, const Watch& _watch_, const std::string& concise_indicator_description_,
                     const SecurityMarketView& _indep_market_view_, double _fractional_seconds_, int t_growth_size_,
                     int t_depletion_size_, double t_stable_value_);

 public:
  ~AmplifyLevelChange() {}

  // listener interface
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {
    OnMarketUpdate(_security_id_, _market_update_info_);
  }

  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_) {}

  // functions
  static std::string VarName() { return "AmplifyLevelChange"; }

  void WhyNotReady();

  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);
  void UpdateQuoteVec();
  void SetEndIndex();

 protected:
};
}
