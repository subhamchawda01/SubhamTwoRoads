/**
    \file Indicators/curve_adjusted_price.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_INDICATORS_CURVE_ADJUSTED_PRICE_H
#define BASE_INDICATORS_CURVE_ADJUSTED_PRICE_H

#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "baseinfra/MarketAdapter/security_market_view.hpp"
#include "dvctrade/Indicators/common_indicator.hpp"
#include "baseinfra/BaseUtils/curve_utils.hpp"

namespace HFSAT {

class CurveAdjustedPrice : public CommonIndicator {
 protected:
  const SecurityMarketView& dep_market_view_;
  const SecurityMarketView& indep1_market_view_;
  const SecurityMarketView& indep2_market_view_;

  //    DI1Utils curve_utils_ ;
  const PriceType_t price_type_t;

  double current_dep_price_;
  double current_indep1_price_;
  double current_indep2_price_;

  // computational variables
  double dep_term_;
  double indep1_term_;
  double indep2_term_;

  bool dep_interrupted_;
  bool indep1_interrupted_;
  bool indep2_interrupted_;

  double alpha1_;
  double alpha2_;
  double current_predicted_price_;

  CurveAdjustedPrice(DebugLogger& _dbglogger_, const Watch& _watch_, const std::string& _concise_indicator_description_,
                     const SecurityMarketView& _dep_market_view_, const SecurityMarketView& _indep1_market_view_,
                     const SecurityMarketView& _indep2_market_view_, PriceType_t _price_type_);

 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static CurveAdjustedPrice* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                               const std::vector<const char*>& _tokens_, PriceType_t _base_price_type_);

  static CurveAdjustedPrice* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                               const SecurityMarketView& _dep_market_view_,
                                               const SecurityMarketView& _indep1_market_view_,
                                               const SecurityMarketView& _indep2_market_view_,
                                               PriceType_t t_price_type_);

  ~CurveAdjustedPrice() {}

  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {
    OnMarketUpdate(_security_id_, _market_update_info_);
  }

  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_){};

  inline void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  inline void OnMarketDataResumed(const unsigned int _security_id_);
  // functions

  // functions
  static std::string VarName() { return "CurveAdjustedPrice"; }

 protected:
  void InitializeValues();
};
}

#endif  // BASE_INDICATORS_CURVE_ADJUSTED_PRICE_H
