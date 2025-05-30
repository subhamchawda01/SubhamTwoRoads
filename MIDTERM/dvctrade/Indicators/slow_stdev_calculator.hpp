/**
    \file Indicators/slow_stdev_calculator.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_INDICATORS_SLOW_STDEV_CALCULATOR_H
#define BASE_INDICATORS_SLOW_STDEV_CALCULATOR_H

#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvctrade/Indicators/price_portfolio.hpp"

namespace HFSAT {

/// Common interface extended by all classes listening to SlowStdevCalculator
/// to listen to changes in online computed stdev of the product
class SlowStdevCalculatorListener {
 public:
  virtual ~SlowStdevCalculatorListener(){};
  virtual void OnStdevUpdate(const unsigned int _security_id_, const double& _new_stdev_value_) = 0;
};

typedef std::vector<SlowStdevCalculatorListener*> SlowStdevCalculatorListenerPtrVec;
typedef std::vector<SlowStdevCalculatorListener*>::const_iterator SlowStdevCalculatorListenerPtrVecCIter_t;
typedef std::vector<SlowStdevCalculatorListener*>::iterator SlowStdevCalculatorListenerPtrVecIter_t;

/// Class used currently in StableScaledTrend to get the value of stdev_value_
/// for the product computed over alonger period than the period in which the
/// price_change ( or pseudo price_change ... px - expw_movavg ) is computed.
/// The value is expected to be update every 10 seconds and then it nmotifies it's listeners
class SlowStdevCalculator : public CommonIndicator, public IndicatorListener {
 protected:
  // variables
  const SecurityMarketView* indep_market_view_;
  std::string shortcode_;
  PricePortfolio* price_portfolio_;

  PriceType_t price_type_;

  // computational variables
  double moving_avg_price_;
  double moving_avg_squared_price_;

  double last_price_recorded_;
  double stdev_value_;

  double current_indep_price_;
  /// since sqrt of positive number unbiased_l2_norm_ is a denominator, we should have a min threshold
  /// below which there has been so little movement that a BollingerBand sort of math would not make sense
  /// and hence the movement is best described as independant of unbiased_l2_norm_
  double min_unbiased_l2_norm_;
  bool is_price_portfolio_;

  SlowStdevCalculatorListenerPtrVec slow_stdev_calculator_listener_ptr_vec_;

  // functions
 public:
  static SlowStdevCalculator* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                const std::string& shortcode,
                                                const unsigned int t_trend_history_msecs_ = 200u * 1000u,
                                                double t_min_stdev_value_factor = 1.0);

 protected:
  SlowStdevCalculator(DebugLogger& _dbglogger_, const Watch& _watch_, const std::string& concise_indicator_description_,
                      const std::string& shortcode, const unsigned int t_trend_history_msecs_,
                      const double t_min_stdev_value_factor);

 public:
  ~SlowStdevCalculator() {}

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

  void OnIndicatorUpdate(const unsigned int& indicator_index, const double& new_value);
  void OnIndicatorUpdate(const unsigned int& indicator_index, const double& new_value_decrease,
                         const double& new_value_nochange, const double& new_value_increase) {}

  // functions
  static std::string VarName() { return "SlowStdevCalculator"; }
  inline double stdev_value() const { return stdev_value_; }
  inline unsigned int trend_history_msecs() const { return trend_history_msecs_; }

  inline void AddSlowStdevCalculatorListener(SlowStdevCalculatorListener* _new_listener_) {
    VectorUtils::UniqueVectorAdd(slow_stdev_calculator_listener_ptr_vec_, _new_listener_);
  }
  inline void RemoveSlowStdevCalculatorListener(SlowStdevCalculatorListener* _new_listener_) {
    VectorUtils::UniqueVectorRemove(slow_stdev_calculator_listener_ptr_vec_, _new_listener_);
  }

  static SlowStdevCalculator* GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                const std::vector<const char*>& r_tokens_, PriceType_t _basepx_pxtype_);
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& r_tokens_);

 protected:
  void InitializeValues();
  void UpdateComputedVariables();
};
}

#endif  // BASE_INDICATORS_SLOW_STDEV_CALCULATOR_H
