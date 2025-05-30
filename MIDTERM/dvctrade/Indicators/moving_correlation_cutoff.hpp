/**
   \file Indicators/moving_correlation_cutoff.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 351, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#ifndef BASE_INDICATORS_MOVING_CORRELATION_CUTOFF_H
#define BASE_INDICATORS_MOVING_CORRELATION_CUTOFF_H

#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvctrade/Indicators/slow_stdev_calculator.hpp"

namespace HFSAT {

/// Common interface extended by all classes listening to MovingCorrelationCutOff
/// to listen to changes in online computed correlation of the products
class MovingCorrelationCutOffListener : public SlowStdevCalculatorListener {
 public:
  virtual ~MovingCorrelationCutOffListener(){};
};

// Regime Indicator: If value is above threshold then returns 2, else 1;
// Calculation :
// Numerator : Exponentially weighted Summation of {(delta price / std_dev  of dependent) * ((delta price / std_dev  of
// independent))}
// Denominator : Exponentially weighted Summation of {(delta price / std_dev  of dependent)^2 + (delta price / std_dev
// of independent)^2}

typedef std::vector<MovingCorrelationCutOffListener*> MovingCorrelationCutOffListenerPtrVec;
typedef std::vector<MovingCorrelationCutOffListener*>::const_iterator MovingCorrelationCutOffListenerPtrVecCIter_t;
typedef std::vector<MovingCorrelationCutOffListener*>::iterator MovingCorrelationCutOffListenerPtrVecIter_t;

class MovingCorrelationCutOff : public CommonIndicator, public SlowStdevCalculatorListener {
 protected:
  // variables
  const SecurityMarketView& dep_market_view_;
  const SecurityMarketView& indep_market_view_;

  PriceType_t price_type_;

  // computational variables
  double moving_sum_of_product_of_normalised_change_in_price_;
  double moving_sum_of_square_of_normalised_price_;

  SlowStdevCalculator& dep_slow_stdev_calculator_;
  SlowStdevCalculator& indep_slow_stdev_calculator_;

  double dep_price_change_;
  double indep_price_change_;

  double dep_last_price_change_;
  double indep_last_price_change_;

  double dep_std_dev_;
  double indep_std_dev_;

  bool dep_updated;
  bool indep_updated;
  double cutoff_threshold;
  bool update_threshold;

  double last_dep_price_recorded_;
  double last_indep_price_recorded_;

  PriceType_t t_price_type_;
  double current_dep_price_;

  double current_indep_price_;
  double dep_moving_avg_price_;
  double indep_moving_avg_price_;
  double _indicator_return_type_;
  // for dynamic threshold
  double decay_page_factor2_;
  std::vector<double> decay_vector2_;
  std::vector<double> decay_vector_sums2_;
  double inv_decay_sum2_;

  double dep_price_change2_;
  double indep_price_change2_;

  double dep_last_price_change2_;
  double indep_last_price_change2_;
  double moving_sum_of_product_of_normalised_change_in_price2_;
  double moving_sum_of_square_of_normalised_price2_;
  double dep_moving_avg_price2_;
  double indep_moving_avg_price2_;
  int last_new_page_msecs2_;
  int page_width_msecs2_;

  MovingCorrelationCutOffListenerPtrVec moving_correlation_cutoff_listener_ptr_vec_;

  // functions
 public:
  static MovingCorrelationCutOff* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                    const SecurityMarketView& _dep_market_view_,
                                                    const SecurityMarketView& _indep_market_view_,
                                                    const unsigned int t_trend_history_msecs_,
                                                    PriceType_t _t_price_type_, double indicator_threshold_,
                                                    int _indicator_return_type_);

 protected:
  MovingCorrelationCutOff(DebugLogger& _dbglogger_, const Watch& _watch_,
                          const std::string& concise_indicator_description_,
                          const SecurityMarketView& _dep_market_view_, const SecurityMarketView& _indep_market_view_,
                          const unsigned int t_trend_history_msecs_, PriceType_t _t_price_type_,
                          double indicator_threshold_, int _indicator_return_type_);

 public:
  ~MovingCorrelationCutOff() {}

  // listener interface
  void OnStdevUpdate(const unsigned int _security_id_, const double& _new_stdev_value_);
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
  static std::string VarName() { return "MovingCorrelationCutOff"; }
  inline unsigned int trend_history_msecs() const { return trend_history_msecs_; }

  inline void AddMovingCorrelationCutOffListener(MovingCorrelationCutOffListener* _new_listener_) {
    VectorUtils::UniqueVectorAdd(moving_correlation_cutoff_listener_ptr_vec_, _new_listener_);
  }
  inline void RemoveMovingCorrelationCutOffListener(MovingCorrelationCutOffListener* _new_listener_) {
    VectorUtils::UniqueVectorRemove(moving_correlation_cutoff_listener_ptr_vec_, _new_listener_);
  }

  static MovingCorrelationCutOff* GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                    const std::vector<const char*>& r_tokens_,
                                                    PriceType_t _basepx_pxtype_);
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& r_tokens_);

 protected:
  void SetThresholdTimeDecayWeights();
  void InitializeValues();
  void InitializeValues2();
};
}

#endif  // BASE_INDICATORS_MOVING_CORRELATION_CUTOFF_H
