/**
 \file Indicators/moving_avg_bid_ask_ivspread.hpp

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
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CDef/math_utils.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/OptionsHelper/option.hpp"


namespace HFSAT {

class MovingAvgBidAskIVSpread : public CommonIndicator {
 protected:
  // variables
  const SecurityMarketView& dep_market_view_;
  const SecurityMarketView& fut_market_view_;

  PriceType_t price_type_;
  OptionObject* dep_option_;
  double dep_bid_implied_vol_;
  double dep_ask_implied_vol_;
  double fut_price_;

  // computational variables
  double last_spread_;
  double current_spread_;
  double moving_average_spread_;

 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static MovingAvgBidAskIVSpread* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                    const std::vector<const char*>& _tokens_,
                                                    PriceType_t _basepx_pxtype_);

  static MovingAvgBidAskIVSpread* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                    const SecurityMarketView& _dep_market_view_, int _history_seconds_,
                                                    PriceType_t _price_type_);

 protected:
  MovingAvgBidAskIVSpread(DebugLogger& _dbglogger_, const Watch& _watch_,
                          const std::string& concise_indicator_description_,
                          const SecurityMarketView& _dep_market_view_, int _history_seconds_, PriceType_t _price_type_);

 public:
  ~MovingAvgBidAskIVSpread() {}

  // listener interface
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);

  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {
    OnMarketUpdate(_security_id_, _market_update_info_);
  }

  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_) {}

  // functions
  static std::string VarName() { return "MovingAvgBidAskIVSpread"; }

  void WhyNotReady();

  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);

 protected:
  void InitializeValues();
};
}
