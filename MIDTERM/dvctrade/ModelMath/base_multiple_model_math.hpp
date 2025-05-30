/**
        \file ModelMath/base_multiple_model_math.hpp

        \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
Address:
Suite No 353, Evoma, #14, Bhattarhalli,
Old Madras Road, Near Garden City College,
KR Puram, Bangalore 560049, India
+91 80 4190 3551
 */
#ifndef BASE_MODELMATH_MULTIPLE_MODEL_MATH_H
#define BASE_MODELMATH_MULTIPLE_MODEL_MATH_H

#include <vector>

#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/email_utils.hpp"
#include "dvccode/CommonTradeUtils/watch.hpp"
#include "baseinfra/OrderRouting/order_manager_listeners.hpp"
#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvctrade/Indicators/diff_price_type.hpp"
#include "dvctrade/Indicators/regime_slow_stdev.hpp"
#include "dvctrade/Indicators/base_implied_price.hpp"
#include "baseinfra/TradeUtils/market_update_manager.hpp"
#include "dvccode/CDef/control_messages.hpp"
#include "dvccode/ORSMessages/control_message_listener.hpp"
#include "dvctrade/ModelMath/non_linear_wrapper.hpp"
#include "baseinfra/MarketAdapter/book_interface.hpp"
#include "dvctrade/ModelMath/multiple_model_math_listener.hpp"
#include "dvccode/CDef/nse_security_definition.hpp"

#include "dvccode/Profiler/cpucycle_profiler.hpp"

#define INDICATORS_NOT_READY_ALERT_PROCESS_RUNTIME 600
#define SHOW_IND_MSECS_TIMEOUT 500

namespace HFSAT {

/// The common parent class of all classes
/// that encode some way of taking outputs of indicators
/// and combining them to make the target price.
/// There is typically a unique BaseModelMath obect corresponding to a model_filename_
/// hence model_filename_ is stored in here.
/// Control reaches here from SMV, and PromOM depending on
/// if the indicator is a market indicator or an ors data indicator.
/// For SMV we call OnReady when all indicators have been updated
/// For PromOM we call Position change since all indicators are based on that.
class BaseMultipleModelMath : public IndicatorListener,
                              public SecurityMarketViewOnReadyListener,
                              public TimePeriodListener,
                              public ControlMessageListener {
 protected:
  DebugLogger& dbglogger_;
  const Watch& watch_;
  int32_t t_trading_start_utc_mfm_;
  int32_t t_trading_end_utc_mfm_;
  int32_t runtime_id_;
  const std::string& param_filename_;

  // Keeps track of values related to every indicator of every shc
  std::vector<std::vector<double>> prev_individual_indicator_value_vec_;
  std::vector<double> value_vec_;
  std::vector<double> last_propagated_value_vec_;
  std::vector<int> last_individual_indicators_debug_print_;

  std::vector<double> prev_global_indicator_value_vec_;
  double common_sum_vars_;
  int last_common_indicators_debug_print_;
  int index_to_propagate_;

  /// internal variable for computation of status of global products
  std::vector<bool> is_ready_vec_;
  bool is_ready_;
  bool last_is_ready_;

  /// internal variable for computation of status of individual products
  std::vector<std::vector<bool>> is_ready_vec_product_;
  std::vector<bool> is_ready_product_;
  std::vector<bool> last_is_ready_product_;

  MultipleModelMathListener* model_math_listener__;
  int modelmath_index_;

  std::string underlying_shc_;
  PriceType_t dep_baseprice_type_;
  std::vector<std::string> shortcodes_list_;

  SecurityMarketView* underlying_smv_;
  std::vector<SecurityMarketView*> const_smv_list_;

  /// storing the indicators in the format (Individual Indicators SHC1 Individual Indicators SHC2 Individual Indicators
  /// SHC3 Global Indicators)
  std::vector<CommonIndicator*> global_indicator_vec_;
  std::vector<std::vector<CommonIndicator*>> individual_indicator_vec_;
  std::map<unsigned int, std::vector<double>> global_indicators_weights_;

  unsigned int num_shortcodes_;
  unsigned int num_individual_indicators_;
  int current_indicator_index_;  // Keeps track of indicator that has been subscribed
                                 // option_product_id -> vector of weights ( individual_indicators + global_indicators )
  // std::vector<unsigned int, double> indicator_weights_;

  bool indicator_alert_has_already_been_sent_;
  struct timeval query_start_time_;

  bool is_global_indicator_added_;

  std::vector<std::string> shortcodes_affecting_model_;
  std::vector<std::string> ors_shc_needed_;

 public:
  BaseMultipleModelMath(DebugLogger& _dbglogger_, const Watch& _watch_, SecurityMarketView* _underlying_smv_,
                        const std::string& _param_filename_, PriceType_t _dep_baseprice_type_)
      : dbglogger_(_dbglogger_),
        watch_(_watch_),
        t_trading_start_utc_mfm_(-1),
        t_trading_end_utc_mfm_(-1),
        runtime_id_(-1),
        param_filename_(_param_filename_),
        prev_individual_indicator_value_vec_(),
        value_vec_(),
        last_propagated_value_vec_(),
        last_individual_indicators_debug_print_(),
        prev_global_indicator_value_vec_(),
        common_sum_vars_(0.0),
        last_common_indicators_debug_print_(0),
        index_to_propagate_(-1),
        is_ready_vec_(),
        is_ready_(false),
        last_is_ready_(false),
        is_ready_vec_product_(),
        is_ready_product_(),
        last_is_ready_product_(),
        model_math_listener__(NULL),
        modelmath_index_(0),
        underlying_shc_(_underlying_smv_->shortcode()),
        dep_baseprice_type_(_dep_baseprice_type_),
        shortcodes_list_(),
        underlying_smv_(_underlying_smv_),
        const_smv_list_(),
        global_indicator_vec_(),
        individual_indicator_vec_(),
        global_indicators_weights_(),
        num_shortcodes_(0),
        num_individual_indicators_(0),
        indicator_alert_has_already_been_sent_(true),
        query_start_time_(),
        is_global_indicator_added_(false),
        shortcodes_affecting_model_(),
        ors_shc_needed_() {
    watch_.subscribe_FifteenSecondPeriod(this);
    current_indicator_index_ = 0;
    gettimeofday(&query_start_time_, NULL);
  }

  virtual ~BaseMultipleModelMath() {}

  void SetModelMathIndex(int _modelmath_index_) { modelmath_index_ = _modelmath_index_; }

  virtual inline SecurityMarketView* GetUnderyingSMV() { return underlying_smv_; }
  virtual inline std::vector<SecurityMarketView*> GetConstituentsSMV() { return const_smv_list_; }

  virtual inline unsigned int NumGlobalIndicatorsInModel() { return global_indicator_vec_.size(); }
  virtual inline unsigned int NumIndividualIndicatorsInModel() { return individual_indicator_vec_[0].size(); }

  virtual void AddIndividualIndicator(CommonIndicator* _this_indicator_, const double& _this_weight_,
                                      bool _readiness_required_, int _product_index_);
  virtual void AddGlobalIndicator(CommonIndicator* _this_indicator_, const double& _this_weight_,
                                  bool _readiness_required_);
  virtual void AddGlobalIndicator(CommonIndicator* _this_indicator_, const std::vector<double>& _this_weight_vector_,
                                  bool _readiness_required_);

  void AddShortCode(SecurityMarketView* _smv_);
  void SetNumIndividualIndicators(int _num_individual_indicators_) {
    num_individual_indicators_ = _num_individual_indicators_;
  }

  std::vector<std::string> GetShortCodeAffectingModel() { return shortcodes_affecting_model_; }
  std::vector<std::string> GetORSShortCodeNeeded() { return ors_shc_needed_; }

  void SetShortCodeAffectingModel(std::vector<std::string> _shortcodes_affecting_model_) {
    shortcodes_affecting_model_ = _shortcodes_affecting_model_;
  }
  void SetORSShortCodeNeeded(std::vector<std::string> _ors_shc_needed_) { ors_shc_needed_ = _ors_shc_needed_; }

  void SubscribeMarketInterrupts(MarketUpdateManager& market_update_manager_) {
    for (auto i = 0u; i < global_indicator_vec_.size(); i++) {
      if (global_indicator_vec_[i] != NULL) {
        market_update_manager_.AddMarketDataInterruptedListener(global_indicator_vec_[i]);
        global_indicator_vec_[i]->SubscribeDataInterrupts(market_update_manager_);
      }
    }
    for (auto i = 0u; i < num_shortcodes_; i++) {
      for (unsigned int j = 0; j < individual_indicator_vec_[i].size(); j++) {
        if (individual_indicator_vec_[i][j] != NULL) {
          market_update_manager_.AddMarketDataInterruptedListener(individual_indicator_vec_[i][j]);
          individual_indicator_vec_[i][j]->SubscribeDataInterrupts(market_update_manager_);
        }
      }
    }
  }

  bool is_ready() { return is_ready_; }

  void SetTradingTimeAndQueryId(int32_t const& start_trading_time, int32_t const& end_trading_time,
                                int32_t const& query_id) {
    t_trading_start_utc_mfm_ = start_trading_time;
    t_trading_end_utc_mfm_ = end_trading_time;
    runtime_id_ = query_id;
    HFSAT::CommonIndicator::set_global_start_mfm(start_trading_time);
    HFSAT::CommonIndicator::set_global_end_mfm(end_trading_time);
  }

  int32_t start_trading_time() { return t_trading_start_utc_mfm_; }
  int32_t runtime_id() { return runtime_id_; }
  int32_t end_trading_time() { return t_trading_end_utc_mfm_; }

  // Called from inherited classes - when they see indicators are not ready
  // The function will choose to send conditional alerts
  void AlertIndicatorsNotReady(std::string const& alert_body) {
    // Historical Mode, Reusing variables instead of adding new live check
    if (runtime_id_ == -1 || t_trading_start_utc_mfm_ == -1) return;

    struct timeval current_time;
    gettimeofday(&current_time, NULL);

    // Let's check if query has been running for sometime and still indicators are not ready
    if (current_time.tv_sec - query_start_time_.tv_sec > INDICATORS_NOT_READY_ALERT_PROCESS_RUNTIME) {
      indicator_alert_has_already_been_sent_ = false;
    }

    // Time to send alert
    if (false == indicator_alert_has_already_been_sent_ && watch_.msecs_from_midnight() > t_trading_start_utc_mfm_ &&
        // LiveTrading check
        (fabs(current_time.tv_sec - watch_.tv().tv_sec) < 300)) {
      char hostname[128];
      hostname[127] = '\0';
      gethostname(hostname, 127);

      std::ostringstream sub_stream;
      sub_stream << "<TRADING_ERROR> Query Not Getting Ready To Trade On Machine " << hostname;

      HFSAT::Email e;
      e.setSubject(sub_stream.str());
      e.addRecepient("nseall@tworoads.co.in");
      e.addSender("TradingError@circulumvite.com");

      e.content_stream << "Query Id : " << runtime_id_ << " <br/>";
      e.content_stream << "Last Time Checked It Was Not Ready Was At : " << query_start_time_ << " <br/>";
      e.content_stream << "Below Indicators Are Not Ready"
                       << " <br/>";
      e.content_stream << alert_body << " <br/>";
      e.sendMail();

      indicator_alert_has_already_been_sent_ = true;

      DBGLOG_CLASS_FUNC_LINE_ERROR << " SENDING EMAIL AS QUERY NOT READY : " << current_time.tv_sec
                                   << " START : " << query_start_time_.tv_sec
                                   << " THRESHOLD : " << INDICATORS_NOT_READY_ALERT_PROCESS_RUNTIME
                                   << " Watch : " << watch_.msecs_from_midnight()
                                   << " START TIME : " << t_trading_start_utc_mfm_ << DBGLOG_ENDL_FLUSH;
    }

    query_start_time_ = current_time;
  }

  virtual void AddListener(MultipleModelMathListener* _new_model_math_listener_, int modelmath_index_ = 0);
  virtual void set_basepx_pxtype() = 0;
  virtual void set_start_end_mfm();

  virtual void DumpIndicatorValues() = 0;
  virtual void FinishCreation() = 0;

  virtual void ForceIndicatorReady(const unsigned int t_indicator_index_, int product_index_){};
  virtual void ForceIndicatorReadyGlobal(const unsigned int t_indicator_index_){};

  virtual void ForceAllIndicatorReady() {
    for (auto i = 0u; i < num_shortcodes_; i++) {
      for (unsigned int j = 0; j < individual_indicator_vec_[i].size(); j++) {
        ForceIndicatorReady(j, i);
      }
    }

    for (auto i = 0u; i < global_indicator_vec_.size(); i++) {
      ForceIndicatorReadyGlobal(i);
    }
  };

  virtual void OnControlUpdate(const ControlMessage& _control_message_, const char* symbol_, const int trader_id) = 0;

  virtual void ShowIndicatorValues() {}

 protected:
  inline void PropagateNewTargetPrice(const double& _new_target_price_, const double& _new_sum_vars_,
                                      int _product_index_) {
#if CCPROFILING_TRADEINIT
    HFSAT::CpucycleProfiler::GetUniqueInstance().End(25);
    HFSAT::CpucycleProfiler::GetUniqueInstance().Start(26);
    HFSAT::CpucycleProfiler::GetUniqueInstance().Start(27);
#endif
    if (model_math_listener__->UpdateTarget(_new_target_price_, _new_sum_vars_, modelmath_index_, _product_index_)) {
      // DumpIndicatorValues();
    }
  }

  inline void PropagateNotReady(int _product_index_) {
    model_math_listener__->TargetNotReady(modelmath_index_, _product_index_);
  }
};
}
#endif  // BASE_MODELMATH_MULTIPLE_MODEL_MATH_H
