/**
    \file Indicators/di_pricing_indicator_mktevents.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#ifndef BASE_INDICATORS_DI_PRICING_INDICATOR_MKTEVENTS_HPP
#define BASE_INDICATORS_DI_PRICING_INDICATOR_MKTEVENTS_HPP

#include "baseinfra/MarketAdapter/security_market_view.hpp"
#include "dvctrade/Indicators/common_indicator.hpp"
#include "baseinfra/BaseUtils/curve_utils.hpp"
#include "dvctrade/Indicators/simple_trend_mktevents.hpp"

#define RATEFACTOR 25200

namespace HFSAT {

class DIPricingIndicatorMktEvents : public IndicatorListener, public CommonIndicator {
  // DIPricingIndicatorMktEvents dep_shc_ indep_1_ indep_2_ dep_pxtype_ indep_1_pxtype_ indep_2_pxtype_
  // _num_events_halflife_
  // specific to DI's ie dep / indeps should be spot rates
  // order of expiries should be : indep_1_ < dep_ < indep_2_
  // assumption : fwd rates between  [indep_1_ <-> dep_] and [dep_ <-> indep_2_] are same

 protected:
  const SecurityMarketView& dep_market_view_;
  const SecurityMarketView& indep_1_market_view_;
  const SecurityMarketView& indep_2_market_view_;

  const PriceType_t t_pxtype_;

  const unsigned int dep_security_id_;
  const unsigned int indep_1_security_id_;
  const unsigned int indep_2_security_id_;

  double prev_value_dep_;      // dependent is spread
  double prev_value_indep_1_;  // indep1
  double prev_value_indep_2_;  // indep2

  bool dep_ready_;
  bool indep_1_ready_;
  bool indep_2_ready_;

  double indep_1_trend_;
  double indep_2_trend_;

  double dep_p_;
  double indep_1_p_;
  double indep_2_p_;

  SimpleTrendMktEvents* indep_1_trend_indicator_;
  SimpleTrendMktEvents* indep_2_trend_indicator_;

 public:
  static DIPricingIndicatorMktEvents* GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                        const std::vector<const char*>& r_tokens_,
                                                        PriceType_t _base_price_type_);

  static DIPricingIndicatorMktEvents* GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                        SecurityMarketView& t_dep_market_view_,
                                                        SecurityMarketView& t_indep_1_market_view_,
                                                        SecurityMarketView& t_indep_2_market_view_,
                                                        unsigned int _num_events_halflife_, PriceType_t t_pxtype_);

  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& r_tokens_);
  static std::string VarName() { return "DIPricingIndicatorMktEvents"; }

  ~DIPricingIndicatorMktEvents() {}

  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);

  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_);

  inline void OnPortfolioPriceChange(double _new_price_) {}

  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_) {}

  inline void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);

  inline void OnMarketDataResumed(const unsigned int _security_id_);

  void WhyNotReady();

  void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_);

  inline void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& new_value_decrease_,
                                const double& new_value_nochange_, const double& new_value_increase_);

 protected:
  DIPricingIndicatorMktEvents(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                              const std::string& concise_indicator_description_, SecurityMarketView& t_dep_market_view_,
                              SecurityMarketView& t_indep_1_market_view_, SecurityMarketView& t_indep_2_market_view_,
                              unsigned int _num_events_halflife_, PriceType_t t_dep_pxtype_);
};
}

#endif /* BASE_INDICATORS_DI_PRICING_INDICATOR_MKTEVENTS_HPP */
