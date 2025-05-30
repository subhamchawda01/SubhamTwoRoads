/**
    \file Indicators/offline_price_pair_diff.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
 */

#ifndef BASE_INDICATORS_OFFLINE_PRICE_PAIR_DIFF_H
#define BASE_INDICATORS_OFFLINE_PRICE_PAIR_DIFF_H

#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"
#include "baseinfra/MarketAdapter/security_market_view.hpp"

namespace HFSAT {
class OfflinePricePairDiff : public CommonIndicator {
 protected:
  // variables
  const SecurityMarketView& dep_market_view_;
  const SecurityMarketView& indep_market_view_;

  // computational variables
  double alpha_;
  double beta_;

  double current_dep_price_;
  double current_indep_price_;

  bool dep_interrupted_;
  bool indep_interrupted_;

  PriceType_t price_type_;

  // functions
 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static OfflinePricePairDiff* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                 const std::vector<const char*>& _tokens_, PriceType_t _basepx_pxtype_);

  static OfflinePricePairDiff* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                 const SecurityMarketView& _dep_market_view_,
                                                 const SecurityMarketView& _indep_market_view_, double _alpha_,
                                                 double _beta_, PriceType_t _price_type_);

 protected:
  OfflinePricePairDiff(DebugLogger& _dbglogger_, const Watch& _watch_,
                       const std::string& concise_indicator_description_, const SecurityMarketView& _dep_market_view_,
                       const SecurityMarketView& _indep_market_view_vec_, double _alpha_, double _beta_,
                       PriceType_t _price_type_);

 public:
  ~OfflinePricePairDiff() {}

  // listener interface
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {
    OnMarketUpdate(_security_id_, _market_update_info_);
  }
  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_) {}

  static std::string VarName() { return "OfflinePricePairDiff"; }

  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);

 protected:
  void InitializeValues();
};
}

#endif /* */
