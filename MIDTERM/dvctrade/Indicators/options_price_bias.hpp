/**
    \file Indicators/options_price_bias.hpp

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
#include "baseinfra/OptionsUtils/option_object.hpp"

// model 1
#include "dvctrade/Indicators/moving_average_implied_vol.hpp"

// model 2
#include "dvctrade/Indicators/vwap_implied_vol.hpp"

namespace HFSAT {

class OptionsPriceBias : public IndicatorListener, public CommonIndicator {
 protected:
  // variables
  SecurityMarketView& option_market_view_;
  SecurityMarketView& future_market_view_;
  double opt_price_;
  double future_price_;

  OptionObject* option_;
  PriceType_t price_type_;
  double model_price_;
  CommonIndicator* implied_vol_;
  bool implied_vol_set_;
  double model_iv_;

  bool option_interrupted_;
  bool future_interrupted_;

  // functions
 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static OptionsPriceBias* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                             const std::vector<const char*>& _tokens_, PriceType_t _basepx_pxtype_);

  static OptionsPriceBias* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                             SecurityMarketView& _option_market_view_, double _moving_window_size_,
                                             int t_model_type_, PriceType_t _price_type_);

 protected:
  OptionsPriceBias(DebugLogger& _dbglogger_, const Watch& _watch_, const std::string& concise_indicator_description_,
                   SecurityMarketView& _option_market_view_, double _moving_window_size_, int t_model_type_,
                   PriceType_t _price_type_);

 public:
  ~OptionsPriceBias() {}

  // listener interface
  // futures market update
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {
    OnMarketUpdate(_security_id_, _market_update_info_);
  }

  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_) {}

  void OnIndicatorUpdate(const unsigned int& indicator_index_, const double& new_value_);
  inline void OnIndicatorUpdate(const unsigned int& indicator_index_, const double& new_value_decrease_,
                                const double& new_value_nochange_, const double& new_value_increase_) {
    return;
  }

  // functions
  static std::string VarName() { return "OptionsPriceBias"; }

  void WhyNotReady();

  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);

 protected:
  void InitializeValues();
};
}
