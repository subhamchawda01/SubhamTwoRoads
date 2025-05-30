/**
        \file Indicators/common_indicator.hpp

        \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
_Address:
Suite No 353, Evoma, #14, Bhattarhalli,
Old Madras Road, Near Garden City College,
KR Puram, Bangalore 560049, India
+91 80 4190 3551
*/
#ifndef BASE_INDICATORS_COMMON_INDICATOR_H
#define BASE_INDICATORS_COMMON_INDICATOR_H

#include <vector>

#include <string>
#include <iostream>
#include <map>
#include <utility>

#include <sstream>
#include <stdexcept>

#include "dvccode/CDef/debug_logger.hpp"

#include "dvccode/CommonDataStructures/vector_utils.hpp"

#include "dvccode/CommonTradeUtils/watch.hpp"
#include "baseinfra/TradeUtils/market_update_manager_listener.hpp"

#include "baseinfra/MarketAdapter/security_market_view_change_listener.hpp"
#include "baseinfra/MarketAdapter/security_market_view.hpp"
#include "baseinfra/TradeUtils/market_update_manager.hpp"
#include "dvctrade/Indicators/core_shortcodes.hpp"
#include "dvctrade/Indicators/indicator_listener.hpp"
#include "dvccode/CommonTradeUtils/sample_data_util.hpp"
#include "dvctrade/Indicators/portfolio_price_change_listener.hpp"
#include "dvctrade/Indicators/time_decay_calculator.hpp"

#include "baseinfra/OrderRouting/order_manager_listeners.hpp"
#include "dvccode/Profiler/cpucycle_profiler.hpp"

#define EQUITY_INDICATORS_ALWAYS_READY false

namespace HFSAT {

/// Common base class of alll indicators
/// since they are
/// (i) SecurityMarketViewChangeListener
/// (ii) PortfolioPriceChangeListener
/// (iii) GlobalPositionChangeListener
/// they all send their indicator values to IndicatorListenerPair/UnweightedIndicatorListenerPair objects
class CommonIndicator : public SecurityMarketViewChangeListener,
                        public SecurityMarketViewStatusListener,
                        public PortfolioPriceChangeListener,
                        public GlobalPositionChangeListener,
                        public MarketDataInterruptedListener,
                        public OrderExecutedListener,
                        public OrderConfirmedListener,
                        public OrderCanceledListener {
 protected:
  // variables
  DebugLogger& dbglogger_;
  const Watch& watch_;
  std::string concise_indicator_description_;

  std::vector<IndicatorListenerPair> indicator_listener_pairs_;
  std::vector<IndicatorListenerPairLogit> indicator_listener_pairs_logit_;
  std::vector<IndicatorListenerPairSigmoid> indicator_listener_pairs_sigmoid_;
  std::vector<UnweightedIndicatorListenerPair>
      unweighted_indicator_listener_pairs_;  ///< to avoid a multiplication in combos etc where the weight will be 1.00

  // used to indicate the pricetype used for the baseprice of the dependant
  PriceType_t basepx_pxtype_;
  int32_t trading_start_mfm_;
  int32_t trading_end_mfm_;
  // computational variables
  int trend_history_msecs_;
  int last_new_page_msecs_;
  int page_width_msecs_;

  double decay_page_factor_;
  std::vector<double> decay_vector_;
  double inv_decay_sum_;
  std::vector<double> decay_vector_sums_;

  bool is_ready_;
  bool data_interrupted_;
  double indicator_value_;
  double tolerance_value_;
  double prev_indicator_value_;
  MktStatus_t last_mkt_status_;

  // this is new ( because we want to destroy objects for running multiple days )
  // ideally we should also have dumpstate initstate and removeinstance
  // but for now initstate == 0
 public:
  static int32_t global_trading_start_mfm_;
  static int32_t global_trading_end_mfm_;
  static std::map<std::string, CommonIndicator*> global_concise_indicator_description_map_;

  // functions
 public:
  CommonIndicator(DebugLogger& _dbglogger_, const Watch& _watch_, const std::string& r_concise_indicator_description_);
  static unsigned int ClearIndicatorMap() {
    unsigned int t_size_ = global_concise_indicator_description_map_.size();
    for (auto& it : global_concise_indicator_description_map_) {
      if (!it.second) {
        delete it.second;
      }
    }
    global_concise_indicator_description_map_.clear();
    return t_size_;
  }

  virtual ~CommonIndicator() {}

 public:
  inline virtual void set_basepx_pxtype(SecurityMarketView& t_dep_market_view_, PriceType_t t_basepx_pxtype_) {
    basepx_pxtype_ = t_basepx_pxtype_;
  }

  virtual void set_start_mfm(int32_t t_start_mfm_);
  virtual void set_end_mfm(int32_t t_end_mfm_);
  double GetPageWidth();
  double GetDecayPageFactor();
  static void set_global_start_mfm(int32_t t_global_start_mfm_);
  static void set_global_end_mfm(int32_t t_global_end_mfm_);

  inline virtual void OnGlobalPositionChange(const unsigned int _security_id_, int _new_global_position_){};

  inline const std::string& concise_indicator_description() const { return concise_indicator_description_; }
  inline bool IsIndicatorReady() const { return is_ready_; }
  inline bool IsDataInterrupted() const { return data_interrupted_; }
  inline double indicator_value(bool& _is_ready_) const {
    if (is_ready_) {
      return indicator_value_;
    } else {
      _is_ready_ = false;
      return 0;
    }
  }

  virtual void WhyNotReady(){};

  virtual void PrintDebugInfo() const {};

  // called in model_creator. expected to be false for non core indicators
  virtual bool GetReadinessRequired(const std::string& r_dep_shortcode_,
                                    const std::vector<const char*>& tokens_) const {
    std::vector<std::string> core_shortcodes_;
    GetCoreShortcodes(r_dep_shortcode_, core_shortcodes_);
    if (VectorUtils::LinearSearchValue(
            core_shortcodes_,
            std::string(tokens_[3])))  // making an assumption that this string refers to the shortcode
    {
      return true;
    }
    return false;
  }

  inline virtual void SubscribeDataInterrupts(MarketUpdateManager& market_update_manager_) { return; }

  void add_indicator_listener(unsigned int _indicator_index_, IndicatorListener* _indicator_listener__,
                              double _node_value_);
  void add_indicator_listener(unsigned int _indicator_index_, IndicatorListener* _indicator_listener__,
                              double _node_value_, bool _check_existance_);
  void add_indicator_listener(unsigned int _indicator_index_, IndicatorListener* _indicator_listener__,
                              double _node_value_decrease_, double _node_value_nochange_, double _node_value_increase_);
  void add_indicator_listener(unsigned int _indicator_index_, IndicatorListener* _indicator_listener__,
                              double _node_alpha_, double _node_beta_);

  void UpdateIndicatorListenerWeight(IndicatorListener* _indicator_listener__, double _node_value_);

  void MultiplyIndicatorListenerWeight(IndicatorListener* _indicator_listener__, double _node_value_mult_factor_);

  double GetIndicatorListenerWeight(IndicatorListener* _indicator_listener__);

  bool IsWeightedListenerPresent(IndicatorListener* _indicator_listener__);

  void add_unweighted_indicator_listener(unsigned int _indicator_index_, IndicatorListener* _indicator_listener__);

  bool IsUnweightedListenerPresent(IndicatorListener* _indicator_listener__);

  void SetTimeDecayWeights();

  inline void NotifyIndicatorListeners(double _indicator_value_) {
#if CCPROFILING_TRADEINIT
    HFSAT::CpucycleProfiler::GetUniqueInstance().End(23);
#endif
    for (auto& t_indicator_listener_pair_ : indicator_listener_pairs_) {

#if CCPROFILING_TRADEINIT
      if (t_indicator_listener_pair_.indicator_listener__->listener_type == IndicatorListener::LISTENER_TYPE::kAggregator) {
        HFSAT::CpucycleProfiler::GetUniqueInstance().Start(24);
      } else {
        HFSAT::CpucycleProfiler::GetUniqueInstance().Start(23);
      }
#endif
      t_indicator_listener_pair_.OnIndicatorUpdate(_indicator_value_);

#if CCPROFILING_TRADEINIT
      if (t_indicator_listener_pair_.indicator_listener__->listener_type == IndicatorListener::LISTENER_TYPE::kAggregator) {
        HFSAT::CpucycleProfiler::GetUniqueInstance().End(24);
      } else {
        HFSAT::CpucycleProfiler::GetUniqueInstance().End(23);
      }
#endif
    }
    for (auto& t_unweighted_indicator_listener_pair_ : unweighted_indicator_listener_pairs_) {
#if CCPROFILING_TRADEINIT
      if (t_unweighted_indicator_listener_pair_.indicator_listener__->listener_type == IndicatorListener::LISTENER_TYPE::kAggregator) {
        HFSAT::CpucycleProfiler::GetUniqueInstance().Start(24);
      } else {
        HFSAT::CpucycleProfiler::GetUniqueInstance().Start(23);
      }
#endif
      t_unweighted_indicator_listener_pair_.OnIndicatorUpdate(_indicator_value_);
#if CCPROFILING_TRADEINIT
      if (t_unweighted_indicator_listener_pair_.indicator_listener__->listener_type == IndicatorListener::LISTENER_TYPE::kAggregator) {
        HFSAT::CpucycleProfiler::GetUniqueInstance().End(24);

      } else {
        HFSAT::CpucycleProfiler::GetUniqueInstance().End(23);
      }
#endif
    }
    for (auto& t_indicator_listener_pairs_logit_ : indicator_listener_pairs_logit_) {
// These listeners are aggregator specific only
#if CCPROFILING_TRADEINIT
      HFSAT::CpucycleProfiler::GetUniqueInstance().Start(24);
#endif
      t_indicator_listener_pairs_logit_.OnIndicatorUpdate(_indicator_value_);
#if CCPROFILING_TRADEINIT
      HFSAT::CpucycleProfiler::GetUniqueInstance().End(24);
#endif
    }
    for (auto& t_indicator_listener_pairs_sigmoid_ : indicator_listener_pairs_sigmoid_) {
// These listeners are aggregator specific only
#if CCPROFILING_TRADEINIT
      HFSAT::CpucycleProfiler::GetUniqueInstance().Start(24);
#endif
      t_indicator_listener_pairs_sigmoid_.OnIndicatorUpdate(_indicator_value_);
#if CCPROFILING_TRADEINIT
      HFSAT::CpucycleProfiler::GetUniqueInstance().End(24);
#endif
    }
  }

  inline virtual void OrderExecuted(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                                    const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                                    const double _price_, const TradeType_t r_buysell_, const int _size_remaining_,
                                    const int _size_executed_, const int _client_position_, const int _global_position_,
                                    const int r_int_price_, const int32_t server_assigned_message_sequence,
                                    const uint64_t exchange_order_id, const ttime_t time_set_by_server){};

  inline virtual void OrderConfirmed(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                                     const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                                     const double _price_, const TradeType_t r_buysell_, const int _size_remaining_,
                                     const int _size_executed_, const int _client_position_,
                                     const int _global_position_, const int r_int_price_,
                                     const int32_t server_assigned_message_sequence, const uint64_t exchange_order_id,
                                     const ttime_t time_set_by_server){};

  inline virtual void OrderORSConfirmed(const int t_server_assigned_client_id_,
                                        const int _client_assigned_order_sequence_,
                                        const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                                        const double _price_, const TradeType_t r_buysell_, const int _size_remaining_,
                                        const int _size_executed_, const int r_int_price_,
                                        const int32_t server_assigned_message_sequence,
                                        const uint64_t exchange_order_id, const ttime_t time_set_by_server){};

  virtual void OrderCanceled(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                             const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                             const double _price_, const TradeType_t r_buysell_, const int _size_remaining_,
                             const int _client_position_, const int _global_position_, const int r_int_price_,
                             const int32_t server_assigned_message_sequence, const uint64_t exchange_order_id,
                             const ttime_t time_set_by_server){};

  virtual void OrderCancelRejected(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                                   const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                                   const double _price_, const TradeType_t t_buysell_, const int _size_remaining_,
                                   const int _rejection_reason_, const int t_client_position_,
                                   const int t_global_position_, const int r_int_price_,
                                   const uint64_t exchange_order_id, const ttime_t time_set_by_server){};

  //   inline void OnMarketDataInterrupted ( const unsigned int _security_id_, const int msecs_since_last_receive_) { }
  //  inline void OnMarketDataResumed (const unsigned int _security_id_) { }

  inline void OnIndicatorUpdate(const unsigned int& indicator_index_, const std::vector<double> _new_values_) {
    return;
  }  // TODO see the need for this. Since this does nto extend any base class with this virtual function
  virtual void OnMarketStatusChange(const unsigned int _security_id_, const MktStatus_t _new_market_status_);
  void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_) {return; }
};
}
#endif  // BASE_INDICATORS_COMMON_INDICATOR_H
