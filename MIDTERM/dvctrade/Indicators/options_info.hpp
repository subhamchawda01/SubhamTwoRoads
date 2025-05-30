/**
    \file Indicators/options_info.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/

#pragma once

#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvctrade/Indicators/exponential_moving_average.hpp"
#include "baseinfra/MarketAdapter/base_market_view_manager.hpp"
#include "baseinfra/OptionsUtils/option_object.hpp"

// this one prints
// r, k, type_ -> @init
// T-t -> @15OTP
// c(t) f(t) sig(t) ->OMU

// INDICATOR 1.00 OptionsInfo NSE_SBIN_C0_A 1 price_type_
// 1 -> interest_rate 2 -> strike 3 -> option_type_
// 4 -> time_to_maturity_in_years_
// 5 -> option_price_ 6 -> futures_price_ 7 -> bs_implied_vol_

namespace HFSAT {

class OptionsInfo : public CommonIndicator, public TimePeriodListener {
 protected:
  // variables
  const SecurityMarketView& option_market_view_;
  const SecurityMarketView& future_market_view_;

  const double interest_rate_;
  const double strike_price_;
  const OptionType_t option_type_;
  double days_to_expiry_;

  double opt_price_;
  double future_price_;
  double bs_implied_vol_;

  OptionObject* option_;

  bool option_interrupted_;
  bool future_interrupted_;

  int fifteen_min_counter_;
  double fifteen_min_in_days_;
  int option_info_;

  PriceType_t price_type_;

  // functions
 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static OptionsInfo* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                        const std::vector<const char*>& _tokens_, PriceType_t _basepx_pxtype_);

  static OptionsInfo* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                        const SecurityMarketView& _option_market_view_, int _option_info_,
                                        PriceType_t _price_type_);

 protected:
  OptionsInfo(DebugLogger& _dbglogger_, const Watch& _watch_, const std::string& concise_indicator_description_,
              const SecurityMarketView& _option_market_view_, int _option_info_, PriceType_t _price_type_);

 public:
  ~OptionsInfo() {}

  // listener interface
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {
    OnMarketUpdate(_security_id_, _market_update_info_);
  }

  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_) {}

  inline void OnTimePeriodUpdate(const int num_pages_to_add_);

  // functions
  static std::string VarName() { return "OptionsInfo"; }

  void WhyNotReady();

  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);

 protected:
  void InitializeValues();
};
}
