/**
   \file Indicators/expression_indicator.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#ifndef BASE_INDICATORS_EXPRESSION_INDICATOR_H
#define BASE_INDICATORS_EXPRESSION_INDICATOR_H

#include <map>
#include <vector>
#include <string>
#include <tr1/unordered_map>

#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvctrade/Indicators/indicator_listener.hpp"
#include "dvctrade/Indicators/indicator_list.hpp"
#include "dvctrade/OfflineUtils/periodic_stdev_utils.hpp"

#define NUM_DAYS_HISTORY_STDEV 60

namespace HFSAT {

class ExpressionIndicator : public IndicatorListener, public CommonIndicator {
  typedef void (ExpressionIndicator::*AggregatorFunctionPtr)(const unsigned int& indicator_index,
                                                             const double& new_value);  // function pointer type

 protected:
  // variables
  std::vector<bool> is_ready_vec_;
  std::vector<double> prev_value_vec_;

  std::vector<CommonIndicator*> indicator_vec_;
  std::vector<std::vector<const char*> > indicator_tokens_vec_;
  std::string exp_type_;

  double hist_stdev_;  // to be used in cutting off trend / offline-online indicators in low stdev times
  double composite_sig_a_;
  double composite_sig_b_;
  double composite_beta_;

  int regime_;  // regime to be used in case of regime cutoff indicators
  unsigned int initial_regime_;

  std::vector<double> params_array_;
  double alpha_composite_;

  std::map<std::string, AggregatorFunctionPtr> exp_to_func_map;
  AggregatorFunctionPtr aggregate_function;

  // varaibles for EMA and Trend
  int trend_history_msecs_ = 1000;

  // computational variables
  double moving_avg_ = 0;
  double moving_square_avg_ = 0;

  int last_new_page_msecs_ = 0;
  int page_width_msecs_ = 500;

  double decay_page_factor_ = 0.95;
  std::vector<double> decay_vector_;
  double inv_decay_sum_ = 0.05;
  std::vector<double> decay_vector_sums_;

  double last_val_ = 0;
  double current_val_ = 0;
  bool is_initialized_ = false;

  // functions
  ExpressionIndicator(DebugLogger& _dbglogger_, const Watch& _watch_, const std::string& concise_indicator_description_,
                      const std::vector<const char*>& r_tokens_, PriceType_t _basepx_pxtype_);

 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static ExpressionIndicator* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                const std::vector<const char*>& _tokens_, PriceType_t _basepx_pxtype_);

  ~ExpressionIndicator() {}

  void set_start_mfm(int32_t t_start_mfm_);
  void set_end_mfm(int32_t t_end_mfm_);

  // listener interface
  inline void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {}
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {
    OnMarketUpdate(_security_id_, _market_update_info_);
  }
  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_) {}
  inline void SubscribeDataInterrupts(MarketUpdateManager& market_update_manager_) {
    for (auto i = 0u; i < indicator_vec_.size(); i++) {
      if (indicator_vec_[i] != NULL) {
        market_update_manager_.AddMarketDataInterruptedListener(indicator_vec_[i]);
      }
    }
  }
  void OnIndicatorUpdate(const unsigned int& indicator_index, const double& new_value);
  inline void OnIndicatorUpdate(const unsigned int& indicator_index, const double& new_value_decrease_,
                                const double& new_value_nochange_, const double& new_value_increase_) {
    return;
  }

  inline void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  inline void OnMarketDataResumed(const unsigned int _security_id_);
  static std::string VarName() { return "Expression"; }

  void set_basepx_pxtype(SecurityMarketView& _dep_market_view_, PriceType_t _basepx_pxtype_);

 protected:
  void InitializeValues(unsigned int);
  bool AreAllReady();
  void CheckTokensValidity();
  void CheckValidExpType();

  ////////////////Support functions
  // for EMA
  void SetTimeDecayWeights_EMA(double _fractional_seconds_);
  void InitializeValues();

  //////////////// Aggregator functions of Indicators------------------------------------
  void InitFunctionPointers();
  void ExpSum(const unsigned int& indicator_index, const double& new_value);
  void Mod(const unsigned int& indicator_index, const double& new_value);
  void Sign(const unsigned int& indicator_index, const double& new_value);
  void ExpMult(const unsigned int& indicator_index, const double& new_value);
  void ExpDiv(const unsigned int& indicator_index, const double& new_value);
  void TrendFilter(const unsigned int& indicator_index, const double& new_value);
  void Convex(const unsigned int& indicator_index, const double& new_value);
  void MultDelta(const unsigned int& indicator_index, const double& new_value);
  void MaxByMin(const unsigned int& indicator_index, const double& new_value);
  void OnlineComputed(const unsigned int& indicator_index, const double& new_value);
  void OnlineComputedCutOff(const unsigned int& indicator_index, const double& new_value);
  void StdevCutoff(const unsigned int& indicator_index, const double& new_value);
  void StdevCutoffNew(const unsigned int& indicator_index, const double& new_value);
  void StdevCutoffComplement(const unsigned int& indicator_index, const double& new_value);
  void TrendDiff(const unsigned int& indicator_index, const double& new_value);
  void RegimeCutOff(const unsigned int& indicator_index, const double& new_value);
  void Composite(const unsigned int& indicator_index, const double& new_value);
  void EMA(const unsigned int& indicator_index, const double& new_value);
  void Trend(const unsigned int& indicator_index, const double& new_value);
  void Stdev(const unsigned int& indicator_index, const double& new_value);
  void SplitRegime(const unsigned int& indicator_index, const double& new_value);
  void CutoffHigh(const unsigned int& indicator_index, const double& new_value);
  void CutoffLow(const unsigned int& indicator_index, const double& new_value);
  void WeightedIndicator(const unsigned int& indicator_index, const double& new_value);
  void ExpSig(const unsigned int& indicator_index, const double& new_value);
  void NormalizedBound(const unsigned int& indicator_index, const double& new_value);
  void NormalizedStdevBound(const unsigned int& indicator_index, const double& new_value);
};
}

#endif  // BASE_INDICATORS_EXPRESSION_INDICATOR_H
