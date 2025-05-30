/**
    \file Indicators/slow_stdev_calculator_port.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#pragma once

#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvctrade/Indicators/pcaport_price.hpp"

namespace HFSAT {

/// Common interface extended by all classes listening to SlowStdevCalculatorPort
/// to listen to changes in online computed stdev of the product
class SlowStdevCalculatorPortListener {
 public:
  virtual ~SlowStdevCalculatorPortListener(){};
  virtual void OnStdevUpdate(const double& _new_stdev_value_) = 0;
};

typedef std::vector<SlowStdevCalculatorPortListener*> SlowStdevCalculatorPortListenerPtrVec;
typedef std::vector<SlowStdevCalculatorPortListener*>::const_iterator SlowStdevCalculatorPortListenerPtrVecCIter_t;
typedef std::vector<SlowStdevCalculatorPortListener*>::iterator SlowStdevCalculatorPortListenerPtrVecIter_t;

/// Class used currently in StableScaledTrend to get the value of stdev_value_
/// for the product computed over alonger period than the period in which the
/// price_change ( or pseudo price_change ... px - expw_movavg ) is computed.
/// The value is expected to be update every 10 seconds and then it nmotifies it's listeners
class SlowStdevCalculatorPort : public CommonIndicator {
 protected:
  // variables
  PCAPortPrice* const indep_portfolio_price__;

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

  SlowStdevCalculatorPortListenerPtrVec slow_stdev_calculator_listener_ptr_vec_;

  // functions
 public:
  static SlowStdevCalculatorPort* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                    const std::string& _portfolio_descriptor_shortcode_,
                                                    const unsigned int t_trend_history_msecs_ = 200u * 1000u,
                                                    double t_min_stdev_value_ = 1.0);

 protected:
  SlowStdevCalculatorPort(DebugLogger& _dbglogger_, const Watch& _watch_,
                          const std::string& concise_indicator_description_,
                          const std::string& _portfolio_descriptor_shortcode_,
                          const unsigned int t_trend_history_msecs_, double t_min_stdev_value_);

 public:
  ~SlowStdevCalculatorPort() {}

  // listener interface
  inline void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {}
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {}

  void OnPortfolioPriceChange(double _new_price_);
  void OnPortfolioPriceReset(double _new_price_, double _old_price_, unsigned int is_data_interrupted_);

  inline void SubscribeDataInterrupts(MarketUpdateManager& market_update_manager_) {
    market_update_manager_.AddMarketDataInterruptedListener(indep_portfolio_price__);
  }

  inline void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  inline void OnMarketDataResumed(const unsigned int _security_id_);

  // functions
  static std::string VarName() { return "SlowStdevCalculatorPort"; }
  inline double stdev_value() const { return stdev_value_; }

  inline void AddSlowStdevCalculatorPortListener(SlowStdevCalculatorPortListener* _new_listener_) {
    VectorUtils::UniqueVectorAdd(slow_stdev_calculator_listener_ptr_vec_, _new_listener_);
  }
  inline void RemoveSlowStdevCalculatorPortListener(SlowStdevCalculatorPortListener* _new_listener_) {
    VectorUtils::UniqueVectorRemove(slow_stdev_calculator_listener_ptr_vec_, _new_listener_);
  }

  static SlowStdevCalculatorPort* GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                    const std::vector<const char*>& r_tokens_,
                                                    PriceType_t _basepx_pxtype_);
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& r_tokens_);

 protected:
  void InitializeValues();
};
}
