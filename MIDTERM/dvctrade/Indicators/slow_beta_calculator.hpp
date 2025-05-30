/**
    \file Indicators/slow_beta_calculator.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
*/
#ifndef BASE_INDICATORS_SLOW_BETA_CALCULATOR_H
#define BASE_INDICATORS_SLOW_BETA_CALCULATOR_H

#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvctrade/Indicators/common_indicator.hpp"

namespace HFSAT {

/// Common interface extended by all classes listening to SlowBetaCalculator
/// to listen to changes in online computed beta of the product
class SlowBetaCalculatorListener {
 public:
  virtual ~SlowBetaCalculatorListener(){};
  virtual void OnBetaUpdate(const unsigned int _security_id_, const double& _new_beta_value_) = 0;
};

typedef std::vector<SlowBetaCalculatorListener*> SlowBetaCalculatorListenerPtrVec;
typedef std::vector<SlowBetaCalculatorListener*>::const_iterator SlowBetaCalculatorListenerPtrVecCIter_t;
typedef std::vector<SlowBetaCalculatorListener*>::iterator SlowBetaCalculatorListenerPtrVecIter_t;

class SlowBetaCalculator : public CommonIndicator {
 protected:
  // variables
  const SecurityMarketView& dep_market_view_;
  const SecurityMarketView& indep_market_view_;

  PriceType_t price_type_;

  // computational variables
  double moving_dep_avg_price_;
  double moving_dep_avg_squared_price_;

  double moving_indep_avg_price_;
  double moving_indep_avg_squared_price_;

  double moving_dep_indep_avg_price_;

  double last_dep_price_recorded_;
  double last_indep_price_recorded_;
  double last_dep_indep_price_recoreded_;
  double beta_value_;

  double current_dep_price_;
  double current_indep_price_;
  double current_dep_indep_price_;
  /// since sqrt of positive number unbiased_l2_norm_ is a denominator, we should have a min threshold
  /// below which there has been so little movement that a BollingerBand sort of math would not make sense
  /// and hence the movement is best described as independant of unbiased_l2_norm_
  double min_dep_unbiased_l2_norm_;
  double min_indep_unbiased_l2_norm_;

  SlowBetaCalculatorListenerPtrVec slow_beta_calculator_listener_ptr_vec_;

  // functions
 public:
  static SlowBetaCalculator* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                               const SecurityMarketView& _dep_market_view_,
                                               const SecurityMarketView& _indep_market_view_,
                                               const unsigned int t_trend_history_msecs_ = 200u * 1000u);

 protected:
  SlowBetaCalculator(DebugLogger& _dbglogger_, const Watch& _watch_, const std::string& concise_indicator_description_,
                     const SecurityMarketView& _dep_market_view_, const SecurityMarketView& _indep_market_view_,
                     const unsigned int t_trend_history_msecs_);

 public:
  ~SlowBetaCalculator() {}

  // listener interface
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {
    OnMarketUpdate(_security_id_, _market_update_info_);
  }
  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double _new_price_, double _old_price_, unsigned int is_data_interrupted_) {}

  inline void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  inline void OnMarketDataResumed(const unsigned int _security_id_);

  // functions
  static std::string VarName() { return "SlowBetaCalculator"; }
  inline double beta_value() const { return beta_value_; }

  inline void AddSlowBetaCalculatorListener(SlowBetaCalculatorListener* _new_listener_) {
    VectorUtils::UniqueVectorAdd(slow_beta_calculator_listener_ptr_vec_, _new_listener_);
  }
  inline void RemoveSlowBetaCalculatorListener(SlowBetaCalculatorListener* _new_listener_) {
    VectorUtils::UniqueVectorRemove(slow_beta_calculator_listener_ptr_vec_, _new_listener_);
  }
  static SlowBetaCalculator* GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                               const std::vector<const char*>& r_tokens_, PriceType_t _basepx_pxtype_);
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& r_tokens_);

 protected:
  void InitializeValues();
};
}

#endif  // BASE_INDICATORS_SLOW_BETA_CALCULATOR_H
