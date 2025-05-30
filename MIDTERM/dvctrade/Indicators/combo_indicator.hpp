/**
   \file Indicators/mult_mkt_complex_order_price_combo.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#ifndef BASE_INDICATORS_COMBO_INDICATOR_H
#define BASE_INDICATORS_COMBO_INDICATOR_H

#include <map>
#include <vector>
#include <string>

#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvctrade/Indicators/indicator_listener.hpp"
#include "dvctrade/Indicators/indicator_list.hpp"

namespace HFSAT {

/// Indicator simple averaging of MultMktComplexOrderPrice indicators
class ComboIndicator : public IndicatorListener, public CommonIndicator {
 protected:
  // variables
  std::vector<bool> is_ready_vec_;
  std::vector<double> prev_value_vec_;

  std::vector<CommonIndicator*> indicator_vec_;
  std::vector<std::string> portfolio_constituents_;
  std::string dep_short_code_;  // only if relative its valid
  //
  OfflineReturnsLRDB* lrdb_;
  bool is_relative_, is_offline_, is_trend_;

  // functions
  ComboIndicator(DebugLogger& _dbglogger_, const Watch& _watch_, const std::string& concise_indicator_description_,
                 const std::vector<const char*>& r_tokens_, PriceType_t _basepx_pxtype_);

 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static ComboIndicator* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                           const std::vector<const char*>& _tokens_, PriceType_t _basepx_pxtype_);

  ~ComboIndicator() {}

  // listener interface
  inline void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {}
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {
    OnMarketUpdate(_security_id_, _market_update_info_);
  }
  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_) {}
  inline void SubscribeDataInterrupts(MarketUpdateManager& market_update_manager_) {
    for (auto i = 0u; i < indicator_vec_.size(); i++) {
      if (indicator_vec_[i] != NULL) {
        market_update_manager_.AddMarketDataInterruptedListener(indicator_vec_[i]);
      }
    }
  }
  void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_);
  inline void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& new_value_decrease_,
                                const double& new_value_nochange_, const double& new_value_increase_) {
    return;
  }

  inline void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {}
  inline void OnMarketDataResumed(const unsigned int _security_id_) {}
  // functions
  // functions
  static std::string VarName() { return "ComboIndicator"; }

  void set_basepx_pxtype(SecurityMarketView& _dep_market_view_, PriceType_t _basepx_pxtype_);

 protected:
  void InitializeValues(unsigned int);
  void InitializeWeights();
  bool IsAnyoneReady();
  // Indicator specific handling.. has to be done..:P
  void GetIndicatorWeight(std::string _portfolio_descriptor_shortcode_, std::vector<double>& this_indicator_weight_);

  static bool isOfflineCombo(std::string varname_) { return (varname_.find("Offline") != varname_.npos); }

  static bool isOnlineCombo(std::string varname_) { return (varname_.find("Online") != varname_.npos); }

  static bool isRelativeCombo(std::string varname_) {
    // if ( is_offline_ != -1 ) return ( is_offline_==1 );
    return (varname_.find("Offline") != varname_.npos || varname_.find("Online") != varname_.npos ||
            varname_.find("StudPrice") != varname_.npos);
    // {
    // 	//is_offline_ = 1;
    // 	return true;
    // }
    // is_offline_ = 0;
    // return false ;
  }

  static bool isTrendCombo(std::string varname_) {  // TO - CHECk
    // if ( is_trend_ != -1 ) return ( is_trend_==1 );
    return ((varname_.find("Trend") != varname_.npos) || (varname_.find("Derivative") != varname_.npos));
    // {
    // 	is_trend_ = 1;
    // 	return true;
    // }
    // is_trend_ = 0;
    // return false;
  }

  static bool isCombIndicator(std::string varname_) {  // these are Comb not Combo.. BeCareful
    return (varname_.find("Comb") == 0);
  }
};
}

#endif  // BASE_INDICATORS_COMBO_INDICATOR_H
