/**
   \file Indicators/regime_turn_over_rate.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#pragma once

#include <fstream>
#include <strings.h>
#include <stdlib.h>
#include <sstream>

#include "dvccode/CDef/math_utils.hpp"
#include "baseinfra/MarketAdapter/shortcode_security_market_view_map.hpp"
#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvctrade/Indicators/turn_over_rate.hpp"
#include "dvccode/CommonTradeUtils/sample_data_util.hpp"

#define NUM_DAYS_HISTORY 20

namespace HFSAT {

class RegimeTurnOverRate : public IndicatorListener, public CommonIndicator, public TimePeriodListener {
 protected:
  // variables
  const SecurityMarketView* dep_market_view_;
  TurnOverRate* p_dep_indicator_;
  std::map<int, double> offline_tor_avg_;

  double tolerance_;
  double avg_periodic_tor_;
  std::map<int, double> periodic_tors_;
  int current_slot_;

 protected:
  RegimeTurnOverRate(DebugLogger& dbglogger, const Watch& watch, const std::string& t_concise_indicator_description,
                     const SecurityMarketView& dep_market_view, double fractional_seconds, double tolerance,
                     PriceType_t _price_type_);

 public:
  static void CollectShortCodes(std::vector<std::string>& shortcodes_affecting_this_indicator,
                                std::vector<std::string>& ors_source_needed_vec,
                                const std::vector<const char*>& tokens);

  static RegimeTurnOverRate* GetUniqueInstance(DebugLogger& dbglogger, const Watch& watch,
                                               const std::vector<const char*>& tokens, PriceType_t basepx_pxtype);

  static RegimeTurnOverRate* GetUniqueInstance(DebugLogger& dbglogger, const Watch& watch,
                                               const SecurityMarketView& dep_market_view, double fractional_seconds,
                                               double tolerance, PriceType_t price_type);

  ~RegimeTurnOverRate() {}

  // listener interface
  void OnTimePeriodUpdate(const int num_pages_to_add_);

  inline void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_){};
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_){};
  inline void OnPortfolioPriceChange(double _new_price_){};
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_){};

  void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_);
  inline void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& new_value_decrease_,
                                const double& new_value_nochange_, const double& new_value_increase_) {
    return;
  }

  // functions
  /// Used in ModelCreator to see which shortcodes are core
  bool GetReadinessRequired(const std::string& r_dep_shortcode_, const std::vector<const char*>& tokens_) const {
    std::vector<std::string> core_shortcodes_;
    GetCoreShortcodes(r_dep_shortcode_, core_shortcodes_);
    // here tokes 3 and 4 are important
    // if ( ( tokens_.size() > 3u ) &&
    // 	   ( VectorUtils::LinearSearchValue ( core_shortcodes_, std::string(tokens_[3]) ) ) )
    // 	{ return true ; }
    if ((tokens_.size() > 4u) && (VectorUtils::LinearSearchValue(core_shortcodes_, std::string(tokens_[4])))) {
      return true;
    }
    return false;
  }

  static std::string VarName() { return "RegimeTurnOverRate"; }

  void WhyNotReady();

  inline void SubscribeDataInterrupts(MarketUpdateManager& market_update_manager_) {
    if (p_dep_indicator_ != NULL) {
      market_update_manager_.AddMarketDataInterruptedListener(p_dep_indicator_);
    }
  }

  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);

 protected:
  void InitializeValues();
};
}
