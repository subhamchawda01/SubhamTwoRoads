#pragma once

#include <assert.h>

#include "dvccode/Profiler/cpucycle_profiler.hpp"

#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvctrade/ModelMath/base_model_math.hpp"
#include "dvctrade/ModelMath/decision_tree.hpp"

namespace HFSAT {
class GradientBoostingClassifierModelAggregator : public BaseModelMath {
 protected:
  SecurityMarketView& dep_market_view_;
  PriceType_t dep_baseprice_type_;
  int last_indicators_debug_print_;
  std::vector<double> prev_value_vec_;
  bool new_data_arrived_;

  std::map<std::pair<int, int>, std::vector<int> > left_children_bid;
  std::map<std::pair<int, int>, std::vector<int> > left_children_ask;
  std::map<std::pair<int, int>, std::vector<int> > right_children_bid;
  std::map<std::pair<int, int>, std::vector<int> > right_children_ask;
  std::map<std::pair<int, int>, std::vector<int> > feature_bid;
  std::map<std::pair<int, int>, std::vector<int> > feature_ask;
  std::map<std::pair<int, int>, std::vector<double> > threshold_bid;
  std::map<std::pair<int, int>, std::vector<double> > threshold_ask;
  std::map<std::pair<int, int>, std::vector<double> > value_bid;
  std::map<std::pair<int, int>, std::vector<double> > value_ask;
  /*std::vector<std::vector<std::vector<int> > > left_children_bid;
  std::vector<std::vector<std::vector<int> > > right_children_bid;
  std::vector<std::vector<std::vector<int> > > feature_bid;
  std::vector<std::vector<std::vector<double> > > threshold_bid;
  std::vector<std::vector<std::vector<double> > > value_bid;
  std::vector<std::vector<std::vector<int> > > left_children_ask;
  std::vector<std::vector<std::vector<int> > > right_children_ask;
  std::vector<std::vector<std::vector<int> > > feature_ask;
  std::vector<std::vector<std::vector<double> > > threshold_ask;
  std::vector<std::vector<std::vector<double> > > value_ask;*/
  int num_levels_;
  int num_trees_at_level_;
  int num_classes_;
  double scale_;
  double predicted_class_bid_;
  double predicted_class_ask_;
  double avg;

 public:
  GradientBoostingClassifierModelAggregator(DebugLogger& _dbglogger_, const Watch& _watch_,
                                            const std::string& _model_filename_, SecurityMarketView& _dep_market_view_,
                                            PriceType_t _dep_baseprice_type_)
      : BaseModelMath(_dbglogger_, _watch_, _model_filename_),
        dep_market_view_(_dep_market_view_),
        dep_baseprice_type_(_dep_baseprice_type_),
        last_indicators_debug_print_(0),
        new_data_arrived_(false),
        num_levels_(0),
        num_trees_at_level_(0),
        num_classes_(0),
        scale_(0.0),
        predicted_class_bid_(0),
        predicted_class_ask_(0),
        avg(0.0) {
    SetPropagateThresholds(dep_market_view_.shortcode());
    GetHugeValueThreshold(dep_market_view_.shortcode());
    dbglogger_ << "=================================================================" << DBGLOG_ENDL_FLUSH;
  }

  virtual ~GradientBoostingClassifierModelAggregator() {}

  inline void ForceIndicatorReady(const unsigned int t_indicator_index_) {
    if (t_indicator_index_ < is_ready_vec_.size() && t_indicator_index_ < indicator_vec_.size() &&
        !is_ready_vec_[t_indicator_index_] && !indicator_vec_[t_indicator_index_]->IsIndicatorReady()) {
      if (!dbglogger_.IsNoLogs()) {
        DBGLOG_TIME_CLASS_FUNC << "Indicator Not Ready [ " << t_indicator_index_ << " ] "
                               << indicator_vec_[t_indicator_index_]->concise_indicator_description()
                               << DBGLOG_ENDL_FLUSH;
        DBGLOG_DUMP;
        indicator_vec_[t_indicator_index_]->WhyNotReady();
      }
      is_ready_vec_[t_indicator_index_] = true;
      if (!dbglogger_.IsNoLogs()) {
        DBGLOG_TIME_CLASS_FUNC << "Indicator Forced Ready [ " << t_indicator_index_ << " ] "
                               << indicator_vec_[t_indicator_index_]->concise_indicator_description()
                               << DBGLOG_ENDL_FLUSH;
        DBGLOG_DUMP;
      }
      is_ready_ = AreAllReady();
      if (is_ready_) {  // Check for is_ready_ again , maybe this indicator was the last one.
        if (!dbglogger_.IsNoLogs()) {
          DBGLOG_TIME_CLASS_FUNC << "All Indicators Ready " << DBGLOG_ENDL_FLUSH;
          DBGLOG_DUMP;
        }
        last_is_ready_ = true;
      }
    }
  }

  inline void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& new_value_decrease_,
                                const double& new_value_nochange_, const double& new_value_increase_) {
    return;
  }

  inline void OnControlUpdate(const ControlMessage& _control_message_, const char* symbol_, const int trader_id) {
    switch (_control_message_.message_code_) {
      case kControlMessageCodeShowIndicators: {
        ShowIndicatorValues();
      } break;
      default: { } break; }
  }

  inline void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_) {
    if (!is_ready_) {
      is_ready_vec_[_indicator_index_] = true;
      is_ready_ = AreAllReady();

      if ((!is_ready_) && (VectorUtils::CheckAllForValue(is_ready_vec_, false) == false) &&
          ((last_indicators_debug_print_ == 0) ||
           (watch_.msecs_from_midnight() > last_indicators_debug_print_ + 2000))) {
        last_indicators_debug_print_ = watch_.msecs_from_midnight();

        std::ostringstream alert_indicators_not_ready_stream;

        for (auto i = 0u; i < indicator_vec_.size(); i++) {
          if (!is_ready_vec_[i]) {
            if (indicator_vec_[i]->IsIndicatorReady()) {
              is_ready_vec_[i] = indicator_vec_[i]->IsIndicatorReady();
              is_ready_ = AreAllReady();
            } else {
              if (!dbglogger_.IsNoLogs()) {
                DBGLOG_TIME_CLASS_FUNC << "Indicator Not Ready [ " << i << " ] "
                                       << indicator_vec_[i]->concise_indicator_description() << DBGLOG_ENDL_FLUSH;
                DBGLOG_DUMP;

                alert_indicators_not_ready_stream << "Indicator Not Ready [ " << i << " ] "
                                                  << indicator_vec_[i]->concise_indicator_description() << " <br/> ";

                indicator_vec_[i]->WhyNotReady();
              }
            }
          }
        }

        if (!is_ready_) {
          AlertIndicatorsNotReady(alert_indicators_not_ready_stream.str());
        }
      }

      if (is_ready_) {
        if (!dbglogger_.IsNoLogs()) {
          DBGLOG_TIME_CLASS_FUNC << "All Indicators Ready " << DBGLOG_ENDL_FLUSH;
          DBGLOG_DUMP;
        }
        last_is_ready_ = true;
      }
    } else {
      if (std::isnan(_new_value_)) {
        if (!dbglogger_.IsNoLogs()) {
          std::cerr << watch_.tv() << ' ' << typeid(*this).name() << ':' << __func__ << ' '
                    << "nan in sum_vars_/ last updated : " << _indicator_index_ << " "
                    << indicator_vec_[_indicator_index_]->concise_indicator_description() << std::endl;
          DBGLOG_TIME_CLASS_FUNC << "nan in sum_vars_. last updated : " << _indicator_index_ << " "
                                 << indicator_vec_[_indicator_index_]->concise_indicator_description()
                                 << DBGLOG_ENDL_FLUSH;
        }
        is_ready_ = false;
        return;
      }
      prev_value_vec_[_indicator_index_] = _new_value_;
      new_data_arrived_ = true;
      // std::cerr << "Indicator value= " << _new_value_ << " Index= " << _indicator_index_ << "\n";
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        if ((last_indicators_debug_print_ == 0) ||
            (watch_.msecs_from_midnight() > last_indicators_debug_print_ + 600000)) {
          last_indicators_debug_print_ = watch_.msecs_from_midnight();
          ShowIndicatorValues();
        }
      }
    }
  }

  void AddIndicator(CommonIndicator* p_this_indicator_, const double& _this_weight_, bool _readiness_required_) {
    if (p_this_indicator_ != NULL) {
      indicator_vec_.push_back(p_this_indicator_);
      p_this_indicator_->add_unweighted_indicator_listener(indicator_vec_.size() - 1, this);
      const bool t_is_this_indicator_ready_ = p_this_indicator_->IsIndicatorReady();
      if (t_is_this_indicator_ready_ == false) {
        is_ready_vec_.push_back(false);
        readiness_required_vec_.push_back(_readiness_required_);
        is_ready_ = false;  // since is_ready_vec_ is now sure not to be false;
      } else {
        is_ready_vec_.push_back(true);
        readiness_required_vec_.push_back(false);
      }
      prev_value_vec_.push_back(0.0);
    } else {
      if (dbglogger_.CheckLoggingLevel(DBG_MODEL_ERROR)) {
        DBGLOG_TIME_CLASS_FUNC << "NULL t_indicator_" << DBGLOG_ENDL_FLUSH;
      }
      ExitVerbose(kExitErrorCodeGeneral, dbglogger_);
    }
  }

  void FinishCreation() {
    if (is_ready_vec_.empty()) {
      is_ready_ = true;
      DBGLOG_TIME_CLASS_FUNC << "No Indicators! So by default Ready!" << DBGLOG_ENDL_FLUSH;
    }
  }

  void StartCreation(double _beta_) { scale_ = _beta_; }

  uint64_t GetCpuCycleCount(void) {
    uint32_t lo, hi;
    __asm__ __volatile__(  // serialize
        "xorl %%eax,%%eax \n        cpuid" ::
            : "%rax", "%rbx", "%rcx", "%rdx");
    /* We cannot use "=A", since this would use %rax on x86_64 and return only the lower 32bits of the TSC */
    __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi) << 32 | lo;
  }

  void AddLeftChild(int boosting_level, int tree_count, std::vector<int>& left_child, int bid_or_ask) {
    if (bid_or_ask == 1)
      left_children_bid[std::make_pair(boosting_level, tree_count)] = left_child;
    else if (bid_or_ask == 2)
      left_children_ask[std::make_pair(boosting_level, tree_count)] = left_child;
  }

  void AddRightChild(int boosting_level, int tree_count, std::vector<int>& right_child, int bid_or_ask) {
    if (bid_or_ask == 1)
      right_children_bid[std::make_pair(boosting_level, tree_count)] = right_child;
    else if (bid_or_ask == 2)
      right_children_ask[std::make_pair(boosting_level, tree_count)] = right_child;
  }

  void AddFeature(int boosting_level, int tree_count, std::vector<int>& feature, int bid_or_ask) {
    if (bid_or_ask == 1)
      feature_bid[std::make_pair(boosting_level, tree_count)] = feature;
    else if (bid_or_ask == 2)
      feature_ask[std::make_pair(boosting_level, tree_count)] = feature;
  }

  void AddThreshold(int boosting_level, int tree_count, std::vector<double>& threshold, int bid_or_ask) {
    if (bid_or_ask == 1)
      threshold_bid[std::make_pair(boosting_level, tree_count)] = threshold;
    else if (bid_or_ask == 2)
      threshold_ask[std::make_pair(boosting_level, tree_count)] = threshold;
  }

  void AddValue(int boosting_level, int tree_count, std::vector<double>& value, int bid_or_ask) {
    if (bid_or_ask == 1) value_bid[std::make_pair(boosting_level, tree_count)] = value;
    if (bid_or_ask == 2) value_ask[std::make_pair(boosting_level, tree_count)] = value;
  }

  inline void SMVOnReady() {
    if (new_data_arrived_) {
      predicted_class_bid_ = decision(prev_value_vec_, 1);
      predicted_class_ask_ = decision(prev_value_vec_, 2);
      new_data_arrived_ = false;
      PropagateCancelSignal(predicted_class_bid_, 1);
      PropagateCancelSignal(predicted_class_ask_, 2);
    }
  }

  void ShowIndicatorValues() {
    if (dbglogger_.CheckLoggingLevel(DBG_MODEL_ERROR)) {
      DBGLOG_TIME_CLASS_FUNC << "\n================================================================="
                             << DBGLOG_ENDL_FLUSH;
      dbglogger_ << " Predicted bid class: " << predicted_class_bid_ << " Predicted ask class: " << predicted_class_ask_
                 << DBGLOG_ENDL_FLUSH;
      for (auto i = 0u; i < indicator_vec_.size(); i++) {
        dbglogger_ << " value: " << (prev_value_vec_[i] / dep_market_view_.min_price_increment()) << " of "
                   << indicator_vec_[i]->concise_indicator_description() << DBGLOG_ENDL_FLUSH;
      }
      dbglogger_ << "=================================================================" << DBGLOG_ENDL_FLUSH;
    }
  }

  void ShowIndicatorValuesToScreen() {
    printf("\033[36;1H");  // move to 35 th line
    std::cout << "\n=================================================================" << std::endl;
    for (auto i = 0u; i < indicator_vec_.size(); i++) {
      std::cout << " value: " << (prev_value_vec_[i] / dep_market_view_.min_price_increment()) << " of "
                << indicator_vec_[i]->concise_indicator_description() << "                  " << std::endl;
    }
    std::cout << "=================================================================" << std::endl;
  }

  void DumpIndicatorValues() {
    if (dbglogger_.CheckLoggingLevel(DBG_MODEL_ERROR)) {
      DBGLOG_TIME_CLASS_FUNC << "\n================================================================="
                             << DBGLOG_ENDL_FLUSH;
      for (auto i = 0u; i < indicator_vec_.size(); i++) {
        dbglogger_ << " value: " << (prev_value_vec_[i] / dep_market_view_.min_price_increment()) << " of "
                   << indicator_vec_[i]->concise_indicator_description() << DBGLOG_ENDL_FLUSH;
      }
      dbglogger_ << "=================================================================" << DBGLOG_ENDL_FLUSH;
    }
  }

  double decision(std::vector<double> new_data, int bid_or_ask) {
    std::vector<double> scores(num_trees_at_level_, 0.0);
    if (bid_or_ask == 1) {
      for (int i = 0; i < num_levels_; i++) {
        for (int k = 0; k < num_trees_at_level_; k++) {
          int node = 0;
          std::pair<int, int> tree_id(i, k);
          while (left_children_bid[tree_id][node] != -1) {
            if (new_data[feature_bid[tree_id][node]] <= threshold_bid[tree_id][node])
              node = left_children_bid[tree_id][node];
            else
              node = right_children_bid[tree_id][node];
          }
          scores[k] += scale_ * value_bid[tree_id][node];
        }
      }
    } else if (bid_or_ask == 2) {
      for (int i = 0; i < num_levels_; i++) {
        for (int k = 0; k < num_trees_at_level_; k++) {
          int node = 0;
          std::pair<int, int> tree_id(i, k);
          while (left_children_ask[tree_id][node] != -1) {
            if (new_data[feature_ask[tree_id][node]] <= threshold_ask[tree_id][node])
              node = left_children_ask[tree_id][node];
            else
              node = right_children_ask[tree_id][node];
          }
          scores[k] += scale_ * value_ask[tree_id][node];
        }
      }
    }

    if (num_classes_ == 2) {
      scores[0] = 1.0 / (1 + exp(-1.0 * scores[0]));
      scores.push_back(scores[scores.size() - 1]);
      scores[0] = 1 - scores[scores.size() - 1];
    } else {
      double sum_exp_score = 0.0;
      for (unsigned i = 0; i < scores.size(); i++) {
        sum_exp_score += exp(scores[i]);
      }
      for (unsigned i = 0; i < scores.size(); i++) {
        scores[i] = exp(scores[i]) / sum_exp_score;
      }
    }
    int result_label = 0;
    double max_prob = 0;
    for (unsigned i = 0; i < scores.size(); i++) {
      if (scores[i] > max_prob) {
        result_label = i;
        max_prob = scores[i];
      }
    }
    if (num_classes_ == 2)
      return scores[1];
    else
      return result_label;
  }

  inline void OnGlobalPositionChange(const unsigned int _security_id_, int _new_global_position_) {
    PropagateCancelSignal(predicted_class_bid_, 1);
    PropagateCancelSignal(predicted_class_ask_, 2);
  }

  void InterpretModelParameters(const std::vector<const char*> _tokens_) {
    num_levels_ = atoi(_tokens_[1]);
    num_classes_ = atoi(_tokens_[2]);
    if (num_classes_ == 2)
      num_trees_at_level_ = 1;
    else
      num_trees_at_level_ = num_classes_;
  }

  void set_basepx_pxtype() {
    for (auto i = 0u; i < indicator_vec_.size(); i++) {
      indicator_vec_[i]->set_basepx_pxtype(dep_market_view_, dep_baseprice_type_);
    }
  }

 protected:
  inline bool AreAllReady() {
    if (!dep_market_view_.is_ready()) {
      return false;
    }
    if (use_implied_price_) {
      if (!p_implied_price_indicator_->IsIndicatorReady()) {
        return false;
      }
    }
    if (use_mid_price_base_) {
      if (!p_diff_price_indicator_->IsIndicatorReady()) return false;
    }
    for (auto i = 0u; i < is_ready_vec_.size(); ++i) {
      if (is_ready_vec_[i] == false && readiness_required_vec_[i] == true) {
        return false;
      }
    }
    return true;
  }
};
}
