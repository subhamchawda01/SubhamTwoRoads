/**
    \file Indicators/amplify_level_change_l1.hpp

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
#include "dvctrade/Indicators/l1_size_trend.hpp"

namespace HFSAT {

/// Class returning projected future price minus current price
/// Currently the projection is very rule based
class AmplifyLevelChangeL1 : public CommonIndicator, public TimePeriodListener {
 protected:
  // variables
  const SecurityMarketView& indep_market_view_;

  int stability_msecs_;  // the number of msecs since newlevel at which point things are most probably stable
  double growth_size_factor_;
  double depletion_size_factor_;

  int growth_size_;      // the size that if crossed while increasing from 0 will accelerate to stable_value_
  int depletion_size_;   // the size that if crossed while falling from mid will accelerate to 0
  double stable_value_;  // the price above bid and ask to push the size to, when accelerating up from 0
  L1SizeTrend* l1_size_ind_;

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
  static void CollectShortCodes(std::vector<std::string>& shortcodes_affecting_this_indicator,
                                std::vector<std::string>& ors_source_needed_vec,
                                const std::vector<const char*>& tokens);

  static AmplifyLevelChangeL1* GetUniqueInstance(DebugLogger& r_dbglogger, const Watch& r_watch,
                                                 const std::vector<const char*>& tokens, PriceType_t basepx_pxtype);

  static AmplifyLevelChangeL1* GetUniqueInstance(DebugLogger& r_dbglogger, const Watch& r_watch,
                                                 const SecurityMarketView& r_indep_market_view,
                                                 double fractional_seconds, double t_growth_size_factor,
                                                 double t_depletion_size_factor, double t_stable_value);

 protected:
  AmplifyLevelChangeL1(DebugLogger& _dbglogger_, const Watch& _watch_,
                       const std::string& concise_indicator_description_, const SecurityMarketView& indep_market_view,
                       double fractional_seconds, double t_growth_size_factor, double t_depletion_size_factor,
                       double t_stable_value);

 public:
  ~AmplifyLevelChangeL1() {}

  void OnTimePeriodUpdate(const int num_pages_to_add);

  // listener interface
  void OnMarketUpdate(const unsigned int t_security_id, const MarketUpdateInfo& market_update_info);
  inline void OnTradePrint(const unsigned int t_security_id, const TradePrintInfo& trade_print_info,
                           const MarketUpdateInfo& market_update_info) {
    OnMarketUpdate(t_security_id, market_update_info);
  }

  inline void OnPortfolioPriceChange(double new_price) {}
  inline void OnPortfolioPriceReset(double t_new_price, double t_old_price, unsigned int is_data_interrupted_) {}

  // functions
  static std::string VarName() { return "AmplifyLevelChangeL1"; }

  void WhyNotReady();

  void OnMarketDataInterrupted(const unsigned int t_security_id, const int msecs_since_last_receive);
  void OnMarketDataResumed(const unsigned int t_security_id);

  void UpdateQuoteVec();
  void SetEndIndex();

 protected:
};
}
