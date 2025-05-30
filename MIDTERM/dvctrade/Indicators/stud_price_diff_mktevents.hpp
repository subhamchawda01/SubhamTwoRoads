/**
    \file Indicators/stud_price_diff_mktevents.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 162, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */

#ifndef BASE_INDICATORS_STUD_PRICE_DIFF_MKTEVENTS_H
#define BASE_INDICATORS_STUD_PRICE_DIFF_MKTEVENTS_H

#include "dvctrade/Indicators/common_indicator.hpp"
#include "baseinfra/MarketAdapter/base_market_view_manager.hpp"
#include "dvctrade/Indicators/slow_stdev_calculator.hpp"
#include "dvctrade/Indicators/offline_returns_lrdb.hpp"

namespace HFSAT {

class StudPriceDiffMktEvents : public CommonIndicator, public SlowStdevCalculatorListener {
 protected:
  // variables
  const SecurityMarketView& dep_market_view_;
  const SecurityMarketView& indep_market_view_;

  const PriceType_t price_type_;

  double dep_decay_page_factor_;
  double dep_inv_decay_sum_;

  double indep_decay_page_factor_;
  double indep_inv_decay_sum_;

  double moving_avg_dep_;
  double current_dep_price_;

  double moving_avg_indep_;
  double current_indep_price_;

  double stdev_dep_;
  double stdev_indep_;

  bool dep_interrupted_;
  bool indep_interrupted_;

  int lrdb_sign_;

  // functions
 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static StudPriceDiffMktEvents* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                   const std::vector<const char*>& _tokens_,
                                                   PriceType_t _basepx_pxtype_);

  static StudPriceDiffMktEvents* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                   const SecurityMarketView& _dep_market_view_,
                                                   const SecurityMarketView& _indep_market_view_,
                                                   double _fractional_seconds_, double _lrdb_sign_,
                                                   PriceType_t _price_type_);

 protected:
  StudPriceDiffMktEvents(DebugLogger& _dbglogger_, const Watch& _watch_,
                         const std::string& concise_indicator_description_, const SecurityMarketView& _dep_market_view_,
                         const SecurityMarketView& _indep_market_view_, double _fractional_seconds_, double _lrdb_sign_,
                         PriceType_t _price_type_);

 public:
  ~StudPriceDiffMktEvents() {}

  // listener interface
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {
    OnMarketUpdate(_security_id_, _market_update_info_);
  }

  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_) {}

  // functions
  static std::string VarName() { return "StudPriceDiffMktEvents"; }

  void WhyNotReady();

  void OnStdevUpdate(const unsigned int _security_id_, const double& _new_stdev_value_);

  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);

 protected:
  void InitializeValues();
};
}

#endif  // BASE_INDICATORS_STUD_PRICE_DIFF_MKTEVENTS_H
