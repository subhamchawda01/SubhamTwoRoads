/**
        \file ModelMath/base_model_math.hpp

        \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
Address:
Suite No 353, Evoma, #14, Bhattarhalli,
Old Madras Road, Near Garden City College,
KR Puram, Bangalore 560049, India
+91 80 4190 3551
 */
#ifndef BASE_MODELMATH_BASE_MODEL_MATH_H
#define BASE_MODELMATH_BASE_MODEL_MATH_H

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
#include "dvctrade/ModelMath/model_math_listener.hpp"
#include "dvctrade/ModelMath/cancel_model_listener.hpp"
#include "dvctrade/ModelMath/non_linear_wrapper.hpp"
#include "baseinfra/MarketAdapter/book_interface.hpp"

#include "dvccode/Profiler/cpucycle_profiler.hpp"

#define SHOW_IND_MSECS_TIMEOUT 500
#define INDICATORS_NOT_READY_ALERT_PROCESS_RUNTIME 600

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
class BaseModelMath : public IndicatorListener,
                      public SecurityMarketViewOnReadyListener,
                      public GlobalPositionChangeListener,
                      public ControlMessageListener {
 protected:
  DebugLogger& dbglogger_;
  const Watch& watch_;
  int32_t t_trading_start_utc_mfm_;
  int32_t t_trading_end_utc_mfm_;
  int32_t runtime_id_;
  std::string model_filename_;

  /// internal variable for computation of status of model
  std::vector<bool> is_ready_vec_;
  std::vector<bool> readiness_required_vec_;
  bool is_ready_;
  bool last_is_ready_;
  StrategyType this_strategy_type_;
  double model_stdev_;  // used in position based exec logi
  /// typically there is expected to be a single listener
  /// the solitary strategy class like PriceBasedTrading object that gets the target price
  /// in the case that there are more than one listeners
  /// like two strategies running with the same model
  /// single_model_math_listener__ will be nullptr
  ModelMathListener* single_model_math_listener__;
  /// and model_math_listener_vec_ shall be used to store the listeners
  /// and otify them
  std::vector<ModelMathListener*> model_math_listener_vec_;

  CancelModelListener* single_cancel_model_listener__;
  std::vector<CancelModelListener*> cancel_model_listener_vec_;

  /// storing the indicators, primarily for access in MultiplyIndicatorNodeValuesBy
  std::vector<CommonIndicator*> indicator_vec_;
  // This indicator should always return an int (index to the sumvars vector)
  CommonIndicator* p_dep_indep_based_regime_;
  // This is to be used in DAT when the Base Price isn't Mid
  DiffPriceType* p_diff_price_indicator_;
  // This is to be used in DAT for deciding whether to use base as mid or not
  RegimeSlowStdev* p_regime_indicator_for_dat_;
  // for spread/fly trading, where dep_smv_ don't provide the desired
  const BookInterface* target_price_reporter_;
  // price
  bool use_mid_price_base_;
  BaseImpliedPrice* p_implied_price_indicator_;
  bool use_implied_price_;
  bool use_own_base_px_;
  PriceType_t own_base_px_;
  std::vector<int> modelmath_index_vec_;
  int single_modelmath_index_;

  double min_ticks_move_;
  double min_ticks_move_ratio_;

  bool indicator_alert_has_already_been_sent_;
  struct timeval query_start_time_;
  int huge_value_threshold_;
  bool is_cancellation_;

 public:
  BaseModelMath(DebugLogger& _dbglogger_, const Watch& _watch_, const std::string& _model_filename_)
      : dbglogger_(_dbglogger_),
        watch_(_watch_),
        t_trading_start_utc_mfm_(-1),
        t_trading_end_utc_mfm_(-1),
        runtime_id_(-1),
        model_filename_(_model_filename_),
        is_ready_vec_(),
        readiness_required_vec_(),
        is_ready_(false),
        last_is_ready_(false),
        this_strategy_type_(kPriceBasedAggressiveTrading),
        model_stdev_(1.00),
        single_model_math_listener__(nullptr),
        model_math_listener_vec_(),
        single_cancel_model_listener__(nullptr),
        cancel_model_listener_vec_(),
        indicator_vec_(),
        p_dep_indep_based_regime_(nullptr),
        p_diff_price_indicator_(nullptr),
        p_regime_indicator_for_dat_(nullptr),
        target_price_reporter_(nullptr),
        use_mid_price_base_(false),
        p_implied_price_indicator_(nullptr),
        use_implied_price_(false),
        use_own_base_px_(false),
        own_base_px_(kPriceTypeMax),
        modelmath_index_vec_(),
        single_modelmath_index_(0),
        min_ticks_move_(0.015),
        // min_ticks_move_(-0.1),
        min_ticks_move_ratio_(0),
        indicator_alert_has_already_been_sent_(true),
        query_start_time_(),
        huge_value_threshold_(10),
        is_cancellation_(false) {
    model_math_listener_vec_.clear();
    indicator_vec_.clear();
    is_ready_vec_.clear();
    readiness_required_vec_.clear();

    // Process Start Time
    gettimeofday(&query_start_time_, nullptr);
    listener_type = kAggregator;
  }

  virtual ~BaseModelMath() {}

  void GetHugeValueThreshold(const std::string& _dep_shortcode_) {
    std::ifstream ifs;
    ifs.open("/spare/local/tradeinfo/huge_value_thresholds.txt", std::ifstream::in);
    if (!ifs.is_open()) {
      return;
    }
    const unsigned kLineLen = 256;
    char buffer_[kLineLen];
    while (ifs.good()) {
      bzero(buffer_, kLineLen);
      ifs.getline(buffer_, kLineLen);
      PerishableStringTokenizer st_(buffer_, kLineLen);
      const std::vector<const char*>& tokens_ = st_.GetTokens();
      if (tokens_.size() < 2) {
        continue;
      }
      if (strcmp(tokens_[0], _dep_shortcode_.c_str()) == 0) {
        huge_value_threshold_ = std::max(10, atoi(tokens_[1]));
        break;
      }
    }
    ifs.close();
  }

  void SetCancellation() { is_cancellation_ = true; }

  bool CheckCancellation() { return is_cancellation_; }

  void SetPropagateThresholds(const std::string& _dep_shortcode_) {
    std::ifstream ifs;
    ifs.open("/spare/local/tradeinfo/sumvars_propagate_thresholds.txt", std::ifstream::in);
    if (!ifs.is_open()) {
      return;
    }
    const unsigned kLineLen = 256;
    char buffer_[kLineLen];
    while (ifs.good()) {
      bzero(buffer_, kLineLen);
      ifs.getline(buffer_, kLineLen);
      PerishableStringTokenizer st_(buffer_, kLineLen);
      const std::vector<const char*>& tokens_ = st_.GetTokens();
      if (tokens_.size() < 3) {
        continue;
      }
      if (strcmp(tokens_[0], _dep_shortcode_.c_str()) == 0) {
        min_ticks_move_ = atof(tokens_[1]);
        min_ticks_move_ratio_ = atof(tokens_[2]);
        // std::cout << min_ticks_move_ << " " << min_ticks_move_ratio_ << "\n";
        break;
      }
    }
    ifs.close();
  }

  void SetTargetPriceReporter(BookInterface* _target_price_reporter_) {
    target_price_reporter_ = _target_price_reporter_;
  }
  void SetModelMathIndex(int _modelmath_index_) {
    if (modelmath_index_vec_.size() == 0) {
      single_modelmath_index_ = _modelmath_index_;
    }
    modelmath_index_vec_.push_back(_modelmath_index_);
  }

  void SetOwnBasePx(PriceType_t t_own_base_px_) {
    use_own_base_px_ = true;
    own_base_px_ = t_own_base_px_;
  }
  virtual void GetBasePx(PriceType_t& _dep_baseprice_type) {}

  virtual void SetRegimeIndicator(CommonIndicator* regime_based_indictor_, bool siglr_ = false) {
    p_dep_indep_based_regime_ = regime_based_indictor_;
    if (!siglr_)
      AddIndicator(p_dep_indep_based_regime_, 1.0, false);
    else
      AddIndicator(p_dep_indep_based_regime_, 1.0, 1.0, false);
  }

  virtual void SetModelWeightIndicator(CommonIndicator* p_submodel_weight_indicator_, bool siglr_ = false) {}

  virtual inline unsigned int NumIndicatorsInModel() { return indicator_vec_.size(); }

  bool IsNULLRegime() {
    if (p_dep_indep_based_regime_ == nullptr) return true;
    return false;
  }

  void SetDiffPriceIndicator(DiffPriceType* _diff_price_type_) {
    p_diff_price_indicator_ = _diff_price_type_;
    use_mid_price_base_ = true;
  }

  void SetRegimeIndicatorForDat(RegimeSlowStdev* _regime_indicator_) {
    if (use_mid_price_base_) p_regime_indicator_for_dat_ = _regime_indicator_;
  }

  void SetImpliedPriceIndicator(BaseImpliedPrice* _implied_price_type_) {
    p_implied_price_indicator_ = _implied_price_type_;
    use_implied_price_ = true;
  }

  const std::string& model_filename() const { return model_filename_; }

  // virtual void OnIndicatorUpdate ( const unsigned int & _indicator_index_, const double & _new_value_ ) = 0;
  // virtual void SMVOnReady ( ) = 0;

  // only needed in logistic
  virtual void AddIntercept(const double& _this_weight_decrease_, const double& _this_weight_nochange_,
                            const double& _this_weight_increase_) {
    return;
  }
  virtual void AddIntercept(const double& _this_weight_constant_) {}

  virtual void AddTree(double _beta_) {}
  virtual void AddTreeLine(const int tree_index_, const int tree_line_index_, const int t_indicator_index_,
                           const int t_left_daughter_, const int t_right_daughter_, const double t_split_value_,
                           const double t_prediction_, const bool t_is_leaf_) {}
  virtual void AddLeftChild(const int boosting_level_, const int tree_class_, std::vector<int>& left_child,
                            const int bid_or_ask) {}
  virtual void AddRightChild(const int boosting_level_, const int tree_class_, std::vector<int>& right_child,
                             const int bid_or_ask) {}
  virtual void AddFeature(const int boosting_level_, const int tree_class_, std::vector<int>& feature,
                          const int bid_or_ask) {}
  virtual void AddThreshold(const int boosting_level_, const int tree_class_, std::vector<double>& threshold,
                            const int bid_or_ask) {}
  virtual void AddValue(const int boosting_level_, const int tree_class_, std::vector<double>& value,
                        const int bid_or_ask) {}
  virtual void AddIndicator(CommonIndicator* _this_indicator_, bool _readiness_required_) {}
  virtual void AddIndicator(CommonIndicator* _this_indicator_, const double& _this_weight_, bool _readiness_required_);
  virtual void AddIndicator(CommonIndicator* _this_indicator_, const double& _this_weight_,
                            const unsigned int& _this_id_, bool _readiness_required_) {}
  virtual void AddIndicator(CommonIndicator* _this_indicator_, const double& _this_alpha_, const double& _this_beta_,
                            const unsigned int& _this_id_, bool _readiness_required_) {}
  virtual void AddNonLinearComponent(HFSAT::NonLinearWrapper* _this_non_linear_component_) {}
  virtual int GetIndicatorIndexFromConciseDescription(const std::string& this_indicator_concise_description_) {
    return -1;
  }
  virtual void AddIndicator(CommonIndicator* _this_indicator_, const double& _this_weight_decrease_,
                            const double& _this_weight_nochange_, const double& _this_weight_increase_,
                            bool _readiness_required_);
  virtual void AddIndicator(CommonIndicator* _this_indicator_, const double& _this_alpha_, const double& _this_beta_,
                            bool readiness_required_) {}
  virtual void SetRegimeModelMath(BaseModelMath* _base_model_math_, unsigned _index_) {}
  virtual void SetSubModelMath(BaseModelMath* _base_model_math_){};

  void SubscribeMarketInterrupts(MarketUpdateManager& market_update_manager_) {
    for (auto i = 0u; i < indicator_vec_.size(); i++) {
      if (indicator_vec_[i] != nullptr) {
        market_update_manager_.AddMarketDataInterruptedListener(indicator_vec_[i]);
        indicator_vec_[i]->SubscribeDataInterrupts(market_update_manager_);
      }
    }
  }

  void set_model_stdev(double t_stdev_) { model_stdev_ = t_stdev_; }
  double model_stdev() { return model_stdev_; }

  bool is_ready() { return is_ready_; }

  void SetTradingTimeAndQueryId(int32_t const& start_trading_time, int32_t const& end_trading_time,
                                int32_t const& query_id) {
    t_trading_start_utc_mfm_ = start_trading_time;
    t_trading_end_utc_mfm_ = end_trading_time;
    runtime_id_ = query_id;
    HFSAT::CommonIndicator::set_global_start_mfm(start_trading_time);
    HFSAT::CommonIndicator::set_global_end_mfm(end_trading_time);

    if (p_dep_indep_based_regime_) {
      p_dep_indep_based_regime_->set_start_mfm(start_trading_time);
      p_dep_indep_based_regime_->set_end_mfm(end_trading_time);
    }
    if (p_regime_indicator_for_dat_) {
      p_regime_indicator_for_dat_->set_start_mfm(start_trading_time);
      p_regime_indicator_for_dat_->set_end_mfm(end_trading_time);
    }
    if (p_implied_price_indicator_) {
      p_implied_price_indicator_->set_start_mfm(start_trading_time);
      p_implied_price_indicator_->set_end_mfm(end_trading_time);
    }
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
    gettimeofday(&current_time, nullptr);

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

  virtual void AddListener(ModelMathListener* _new_model_math_listener_, int modelmath_index_ = 0);
  virtual void AddCancellationListener(CancelModelListener* p_new_cancel_model_listener_);

  void SetStrategyType(std::string _strategy_name_);
  virtual void set_basepx_pxtype() = 0;

  virtual void set_start_end_mfm();
  /// probably more sophisticated for more complex model math
  virtual void FinishCreation() {}

  virtual void FinishCreation(bool all_models_finished_) {}

  virtual void StartCreation() {}

  virtual void StartCreation(double beta_) {}

  virtual void InterpretModelParameters(const std::vector<const char*> _tokens_) = 0;

  virtual void DumpIndicatorValues() = 0;
  // a virtual function for IndicatorINFO overwriiten by linear_model_aggregator for debug code INDICATOR_INFO
  virtual void DumpIndicatorContribution() { dbglogger_ << "\n"; }

  virtual void ForceIndicatorReady(const unsigned int t_indicator_index_){};

  virtual void ForceAllIndicatorReady() {
    for (auto i = 0u; i < indicator_vec_.size(); i++) {
      ForceIndicatorReady(i);
    }
  };

  virtual void OnControlUpdate(const ControlMessage& _control_message_, const char* symbol_, const int trader_id) = 0;

  virtual void ShowIndicatorValues() {}

 protected:
  inline void PropagateCancelSignal(double predicted_class_prob_, int bid_or_ask) {
    if (single_cancel_model_listener__) {
      single_cancel_model_listener__->UpdateCancelSignal(predicted_class_prob_, bid_or_ask);
    } else {
      for (auto i = 0u; i < cancel_model_listener_vec_.size(); i++) {
        cancel_model_listener_vec_[i]->UpdateCancelSignal(predicted_class_prob_, bid_or_ask);
      }
    }
  }

  inline void PropagateNewTargetPrice(const double& _new_target_price_, const double& _new_sum_vars_) {
#if CCPROFILING_TRADEINIT
    HFSAT::CpucycleProfiler::GetUniqueInstance().End(25);
    HFSAT::CpucycleProfiler::GetUniqueInstance().Start(26);
    HFSAT::CpucycleProfiler::GetUniqueInstance().Start(27);
#endif
    if (single_model_math_listener__) {
      if (single_model_math_listener__->UpdateTarget(_new_target_price_, _new_sum_vars_, single_modelmath_index_)) {
        DumpIndicatorValues();
      }
    } else {
      bool to_dump_indicator_values_ = false;
      for (auto i = 0u; i < model_math_listener_vec_.size(); i++) {
        to_dump_indicator_values_ |=
            (model_math_listener_vec_[i]->UpdateTarget(_new_target_price_, _new_sum_vars_, modelmath_index_vec_[i]));
      }
      if (to_dump_indicator_values_) {
        DumpIndicatorValues();
      }
    }
  }

  inline void PropagateNewTargetPrice(const double& _prob_0_, const double& _prob_1_, const double& _prob_2_) {
#if CCPROFILING_TRADEINIT
    HFSAT::CpucycleProfiler::GetUniqueInstance().End(25);
    HFSAT::CpucycleProfiler::GetUniqueInstance().Start(26);
    HFSAT::CpucycleProfiler::GetUniqueInstance().Start(27);
#endif
    if (single_model_math_listener__) {
      if (single_model_math_listener__->UpdateTarget(_prob_0_, _prob_1_, _prob_2_, single_modelmath_index_)) {
        DumpIndicatorValues();
      }
    } else {
      bool to_dump_indicator_values_ = false;
      for (auto i = 0u; i < model_math_listener_vec_.size(); i++) {
        to_dump_indicator_values_ |=
            (model_math_listener_vec_[i]->UpdateTarget(_prob_0_, _prob_1_, _prob_2_, modelmath_index_vec_[i]));
      }
      if (to_dump_indicator_values_) {
        DumpIndicatorValues();
      }
    }
  }

  inline void PropagateNotReady() {
    if (single_model_math_listener__) {
      single_model_math_listener__->TargetNotReady();
    } else {
      for (auto i = 0u; i < model_math_listener_vec_.size(); i++) {
        model_math_listener_vec_[i]->TargetNotReady();
      }
    }
  }
};
}
#endif  // BASE_MODELMATH_BASE_MODEL_MATH_H
