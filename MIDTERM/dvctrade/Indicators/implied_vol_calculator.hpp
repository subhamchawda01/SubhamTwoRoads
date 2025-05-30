/**
    \file Indicators/implied_vol_calculator.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_INDICATORS_IMPLIED_VOL_CALCULATOR_H
#define BASE_INDICATORS_IMPLIED_VOL_CALCULATOR_H

#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CDef/math_utils.hpp"

#include "baseinfra/MarketAdapter/base_market_view_manager.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"
#include "baseinfra/OptionsUtils/option_object.hpp"
#include "dvctrade/Indicators/common_indicator.hpp"

namespace HFSAT {

class ImpliedVolCalculator : public CommonIndicator {
 protected:
  // variables
  const SecurityMarketView& indep_market_view_;
  const SecurityMarketView& fut_market_view_;

  const PriceType_t price_type_;

  double current_indep_price_;
  double current_fut_price_;

  OptionObject* option_;

  // functions
 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static ImpliedVolCalculator* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                 const std::vector<const char*>& _tokens_, PriceType_t _basepx_pxtype_);

  static ImpliedVolCalculator* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                 const SecurityMarketView& _indep_market_view_,
                                                 PriceType_t _price_type_);

 protected:
  ImpliedVolCalculator(DebugLogger& _dbglogger_, const Watch& _watch_,
                       const std::string& concise_indicator_description_, const SecurityMarketView& _indep_market_view_,
                       PriceType_t _price_type_);

 public:
  ~ImpliedVolCalculator() {}

  // listener interface
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {
    OnMarketUpdate(_security_id_, _market_update_info_);
  }

  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_) {}

  // functions
  static std::string VarName() { return "ImpliedVolCalculator"; }

  void WhyNotReady();

  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);

 protected:
  void InitializeValues();
};
}

#endif  // BASE_INDICATORS_IMPLIED_VOL_CALCULATOR_H
