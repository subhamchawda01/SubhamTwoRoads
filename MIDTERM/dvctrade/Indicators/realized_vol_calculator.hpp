/**
    \file Indicators/realized_vol_calculator.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_INDICATORS_REALIZED_VOL_CALCULATOR_H
#define BASE_INDICATORS_REALIZED_VOL_CALCULATOR_H

#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvctrade/Indicators/common_indicator.hpp"

// we load daily settlement prices of last 20 days
// unfortunately we moved to use futures prices
// compute returns {r1 ... r19}
// r1 = (p20-curent_moving_avg)/p20 * (s/86400)
// r = ((p2-p1)/p1) * (1-s/86400)
// ri = (pi+1 - pi)/pi
// vol = 100 * sqrt ( 252 * [EMA(ri^2)] );

// look_back_days_ ( half_life_ is at the midpoint ) so weights [ 0.25 to 1 ]
// price_avg_timeperiod_
// nth element = alpha * decay * value
// 1st element = (1-alpha) * decay * value

namespace HFSAT {

/// Common interface extended by all classes listening to RealizedVolCalculator
/// to listen to changes in online computed stdev of the product
class RealizedVolCalculatorListener {
 public:
  virtual ~RealizedVolCalculatorListener(){};
  virtual void OnStdevUpdate(const unsigned int _security_id_, const double& _new_stdev_value_) = 0;
};

typedef std::vector<RealizedVolCalculatorListener*> RealizedVolCalculatorListenerPtrVec;
typedef std::vector<RealizedVolCalculatorListener*>::const_iterator RealizedVolCalculatorListenerPtrVecCIter_t;
typedef std::vector<RealizedVolCalculatorListener*>::iterator RealizedVolCalculatorListenerPtrVecIter_t;

class RealizedVolCalculator : public CommonIndicator, public TimePeriodListener {
 protected:
  // variables
  const SecurityMarketView& indep_market_view_;
  const unsigned int local_vol_history_;
  const unsigned int look_back_days_;

  PriceType_t price_type_;

  double current_indep_price_;
  double last_price_recorded_;

  double current_histvol_rseries_;
  unsigned int current_histvol_n_;
  double current_histvol_;

  double vol_history_decay_;
  double inverse_vol_history_decay_;

  double histvol_;
  double previous_histvol_;

  RealizedVolCalculatorListenerPtrVec realized_vol_calculator_listener_ptr_vec_;

  // functions
 public:
  static RealizedVolCalculator* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                  const SecurityMarketView& _indep_market_view_,
                                                  const unsigned int t_trend_history_msecs_,
                                                  const unsigned int t_look_back_days_, PriceType_t _price_type_);

 protected:
  RealizedVolCalculator(DebugLogger& _dbglogger_, const Watch& _watch_,
                        const std::string& concise_indicator_description_,
                        const SecurityMarketView& _indep_market_view_, const unsigned int t_trend_history_msecs_,
                        const unsigned int t_look_back_days_, PriceType_t _price_type_);

 public:
  ~RealizedVolCalculator() {}
  // listener interface
  void OnTimePeriodUpdate(const int num_pages_to_add_);

  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_){};
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {}

  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double _new_price_, double _old_price_, unsigned int is_data_interrupted_) {}
  inline void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  inline void OnMarketDataResumed(const unsigned int _security_id_);

  // functions
  static std::string VarName() { return "RealizedVolCalculator"; }

  inline void AddRealizedVolCalculatorListener(RealizedVolCalculatorListener* _new_listener_) {
    VectorUtils::UniqueVectorAdd(realized_vol_calculator_listener_ptr_vec_, _new_listener_);
  }
  inline void RemoveRealizedVolCalculatorListener(RealizedVolCalculatorListener* _new_listener_) {
    VectorUtils::UniqueVectorRemove(realized_vol_calculator_listener_ptr_vec_, _new_listener_);
  }

  static RealizedVolCalculator* GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                  const std::vector<const char*>& r_tokens_,
                                                  PriceType_t _basepx_pxtype_);
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& r_tokens_);

 protected:
  void InitializeValues();
};
}

#endif  // BASE_INDICATORS_SLOW_STDEV_CALCULATOR_H
