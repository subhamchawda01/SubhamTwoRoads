/**
    \file Indicators/diff_pair_price_type.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_INDICATORS_DIFF_PAIR_PRICE_TYPE_H
#define BASE_INDICATORS_DIFF_PAIR_PRICE_TYPE_H

#include "baseinfra/MarketAdapter/security_market_view.hpp"
#include "dvctrade/Indicators/common_indicator.hpp"

namespace HFSAT {

/// Indicator returns indep_market_view_::price_type_ - indep_market_view_::basepx_pxtype_
/// Also basepx_pxtype_ changes by set_basepx_pxtype ( ) call
class DiffPairPriceType : public CommonIndicator {
 protected:
  SecurityMarketView& indep_market_view_;

  PriceType_t base_price_type_;
  PriceType_t target_price_type_;

 public:
  // functions

  static void CollectShortCodes(std::vector<std::string>& t_shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& t_ors_source_needed_vec_,
                                const std::vector<const char*>& t_tokens_);

  static DiffPairPriceType* GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& t_watch_,
                                              const std::vector<const char*>& t_tokens_, PriceType_t _basepx_pxtype_);

  static DiffPairPriceType* GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& t_watch_,
                                              SecurityMarketView& t_indep_market_view_, PriceType_t t_base_price_type_,
                                              PriceType_t t_target_price_type_);

  DiffPairPriceType(DebugLogger& t_dbglogger_, const Watch& t_watch_, const std::string& concise_indicator_description_,
                    SecurityMarketView& t_indep_market_view_, PriceType_t t_base_price_type_,
                    PriceType_t t_target_price_type_);

  ~DiffPairPriceType() {}

  // listener interface
  void OnMarketUpdate(const unsigned int t_security_id_, const MarketUpdateInfo& t_market_update_info_);
  inline void OnTradePrint(const unsigned int t_security_id_, const TradePrintInfo& t_trade_print_info_,
                           const MarketUpdateInfo& t_market_update_info_) {
    OnMarketUpdate(t_security_id_, t_market_update_info_);
  }
  inline void OnPortfolioPriceChange(double t_new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_){};

  // functions
  static std::string VarName() { return "DiffPairPriceType"; }
  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);
};
}

#endif  // BASE_INDICATORS_DIFF_PAIR_PRICE_TYPE_H
