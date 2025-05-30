/**
  \file ModelMath/random_forest_model_aggregator.hpp

  \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
Address:
Suite No 353, Evoma, #14, Bhattarhalli,
Old Madras Road, Near Garden City College,
KR Puram, Bangalore 560049, India
+91 80 4190 3551
*/
#ifndef BASE_MODELMATH_TREE_BOOSTING_MODEL_AGGREGATOR_H
#define BASE_MODELMATH_TREE_BOOSTING_MODEL_AGGREGATOR_H

#include <assert.h>
#include "dvccode/Profiler/cpucycle_profiler.hpp"

#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvctrade/ModelMath/base_model_math.hpp"
#include "dvctrade/ModelMath/decision_tree.hpp"

namespace HFSAT {

class TreeBoostingModelAggregator : public BaseModelMath {
 protected:
  SecurityMarketView& dep_market_view_;
  PriceType_t dep_baseprice_type_;
  const bool is_returns_based_;

  double sum_beta_;
  double sum_vars_;
  double last_propagated_target_price_;

  unsigned int max_nodes_;
  unsigned int num_trees_;
  int last_indicators_debug_print_;

  bool any_tree_dirty_;

  std::vector<double> beta_;
  std::vector<double> prev_value_vec_;
  std::vector<unsigned int> dirty_trees_;
  std::vector<unsigned int> prev_end_index_vec_;
  std::vector<unsigned int> orig_idx_to_current_idx_map_;
  std::vector<std::vector<TreeLine*> > tree_vec_;
  std::vector<std::pair<double, unsigned> > tree_predictions_;
  std::vector<std::vector<TreeLine*> > indicator_based_nodes_vec_;

 public:
  TreeBoostingModelAggregator(DebugLogger& _dbglogger_, const Watch& _watch_, const std::string& _model_filename_,
                              SecurityMarketView& _dep_market_view_, PriceType_t _dep_baseprice_type_,
                              bool _is_returns_based_)
      : BaseModelMath(_dbglogger_, _watch_, _model_filename_),
        dep_market_view_(_dep_market_view_),
        dep_baseprice_type_(_dep_baseprice_type_),
        is_returns_based_(_is_returns_based_) {
    sum_beta_ = 0;
    sum_vars_ = 0;
    last_propagated_target_price_ = 0;

    max_nodes_ = 0;
    num_trees_ = 0;

    any_tree_dirty_ = false;
    SetPropagateThresholds(dep_market_view_.shortcode());
    GetHugeValueThreshold(dep_market_view_.shortcode());
  }

  virtual ~TreeBoostingModelAggregator() {}

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
           (watch_.msecs_from_midnight() >
            last_indicators_debug_print_ + 2000))) {  // some indicator is not ready but at least one other is ready
        last_indicators_debug_print_ = watch_.msecs_from_midnight();

        std::ostringstream alert_indicators_not_ready_stream;

        for (auto i = 0u; i < indicator_vec_.size(); i++) {
          if (!is_ready_vec_[i]) {  // if this indicator isn't ready
            // print this and
            // ask if it was ready, but just could not notify us ... basically did not get any update yet.
            if (indicator_vec_[i]->IsIndicatorReady()) {  // if ti was secretly ready
              is_ready_vec_[i] = indicator_vec_[i]->IsIndicatorReady();
              is_ready_ = AreAllReady();
            } else {
              if (!dbglogger_.IsNoLogs()) {
                DBGLOG_TIME_CLASS_FUNC << "Indicator Not Ready [ " << i << " ] "
                                       << indicator_vec_[i]->concise_indicator_description() << DBGLOG_ENDL_FLUSH;
                DBGLOG_DUMP;

                // Add To Stream
                alert_indicators_not_ready_stream << "Indicator Not Ready [ " << i << " ] "
                                                  << indicator_vec_[i]->concise_indicator_description() << " <br/>";

                indicator_vec_[i]->WhyNotReady();
              }
            }
          }
        }

        // Call Base Class To Notify That Indicators Are Not Ready And It Will Decide
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

        // if is_returns_based_ then we should multiply all the weights by the baseprice to avoid an additional
        // multiplication of the sum of indicator values.
      }
    } else {
      if (std::isnan(_new_value_)) {
        if (!dbglogger_.IsNoLogs()) {
          std::cerr << watch_.tv() << ' ' << typeid(*this).name() << ':' << __func__ << ' '
                    << "nan in sum_vars_. last updated : " << _indicator_index_ << " "
                    << indicator_vec_[_indicator_index_]->concise_indicator_description() << std::endl;

          DBGLOG_TIME_CLASS_FUNC << "nan in sum_vars_. last updated : " << _indicator_index_ << " "
                                 << indicator_vec_[_indicator_index_]->concise_indicator_description()
                                 << DBGLOG_ENDL_FLUSH;
        }
        is_ready_ = false;
        return;
      }
      //	  HFSAT::CpucycleProfiler::GetUniqueInstance().Start(4);

      prev_value_vec_[_indicator_index_] = _new_value_;
      std::vector<TreeLine*> node_vec_ = indicator_based_nodes_vec_[_indicator_index_];
      if (node_vec_.size() == 1)  // this is a bloody useless indicator!!
      {
        return;
      }
      unsigned prev_end_index_ = prev_end_index_vec_[_indicator_index_];
      // small check to optimize bsearch
      if (prev_end_index_ == 0 && prev_end_index_ + 1 < node_vec_.size() &&
          _new_value_ <= node_vec_[prev_end_index_ + 1]->split_indicator_value_) {
        return;
      }
      if (prev_end_index_ > 0 && _new_value_ > node_vec_[prev_end_index_]->split_indicator_value_ &&
          (prev_end_index_ + 1 == node_vec_.size() ||
           _new_value_ <= node_vec_[prev_end_index_ + 1]->split_indicator_value_)) {
        return;
      }
      unsigned int start_index_ = 1;
      unsigned int end_index_ = node_vec_.size() - 1;
      unsigned int mid_index_;
      while (start_index_ < end_index_) {
        mid_index_ = (start_index_ + end_index_ + 1) >> 1;
        if (_new_value_ > node_vec_[mid_index_]->split_indicator_value_) {
          start_index_ = mid_index_;
        } else {
          end_index_ = mid_index_ - 1;
        }
      }
      if (_new_value_ <= node_vec_[end_index_]->split_indicator_value_) {
        end_index_--;
      }

      //	  HFSAT::CpucycleProfiler::GetUniqueInstance().End(4);

      if (prev_end_index_ == end_index_) {
        ;
      } else {
        if (end_index_ < prev_end_index_) {
          for (unsigned int i = end_index_ + 1; i <= prev_end_index_; i++) {
            node_vec_[i]->node_answer_ = true;
            dirty_trees_[node_vec_[i]->tree_index_] = 1;
            any_tree_dirty_ = true;
          }
        } else {
          for (unsigned int i = prev_end_index_ + 1; i <= end_index_; i++) {
            node_vec_[i]->node_answer_ = false;
            dirty_trees_[node_vec_[i]->tree_index_] = 1;
            any_tree_dirty_ = true;
          }
        }
        prev_end_index_vec_[_indicator_index_] = end_index_;
      }

      if (dbglogger_.CheckLoggingLevel(TRADEINIT_INFO)) {
        if ((last_indicators_debug_print_ == 0) ||
            (watch_.msecs_from_midnight() > last_indicators_debug_print_ + 2000)) {
          last_indicators_debug_print_ = watch_.msecs_from_midnight();

          ShowIndicatorValuesToScreen();
        }
      }

      if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        if ((last_indicators_debug_print_ == 0) ||
            (watch_.msecs_from_midnight() >
             last_indicators_debug_print_ + 600000)) {  // some indicator is not ready but at least one other is ready
          last_indicators_debug_print_ = watch_.msecs_from_midnight();

          ShowIndicatorValues();
        }
      }
    }
  }

  // ModelMath objects are notified after all the indicators are updated.
  // Indicators get updates from
  // (i) Portfolio which listens to SMV updates
  // (ii) SMV updates
  // (iii) GlobalPositionChange
  inline void SMVOnReady() {
    if (any_tree_dirty_) {
      for (auto i = 0u; i < tree_vec_.size(); i++) {
        if (!dirty_trees_[i]) {
          continue;
        }
        int current_index_ = 0;
        TreeLine* this_line_ = tree_vec_[i][current_index_];
        while (!this_line_->is_leaf_) {
          if (this_line_->node_answer_) {
            current_index_ = this_line_->left_daughter_;
          } else {
            current_index_ = this_line_->right_daughter_;
          }
          this_line_ = tree_vec_[i][current_index_];
        }
        tree_predictions_[orig_idx_to_current_idx_map_[i]].first = this_line_->prediction_;
        dirty_trees_[i] = 0;
      }
      any_tree_dirty_ = false;
      std::sort(tree_predictions_.begin(), tree_predictions_.end());
      double current_sum_ = 0;
      int current_idx_ = 0;
      while (current_sum_ <= sum_beta_ / 2) {
        current_sum_ += beta_[tree_predictions_[current_idx_].second];
        current_idx_++;
      }
      sum_vars_ = tree_predictions_[current_idx_ - 1].first;
      for (unsigned i = 0; i < tree_predictions_.size(); i++) {
        orig_idx_to_current_idx_map_[tree_predictions_[i].second] = i;
      }
    }
    CalcAndPropagate();
  }

  inline void OnGlobalPositionChange(const unsigned int _security_id_, int _new_global_position_) {
    CalcAndPropagate();
  }

  void AddTree(double _beta_) {
    sum_beta_ += _beta_;
    beta_.push_back(_beta_);
    unsigned tree_index_ = tree_vec_.size();
    tree_predictions_.push_back(std::make_pair(0, tree_index_));
    orig_idx_to_current_idx_map_.push_back(tree_index_);
    dirty_trees_.push_back(0);
  }

  void AddTreeLine(const int tree_index_, const int tree_line_index_, const int t_indicator_index_,
                   const int t_left_daughter_, const int t_right_daughter_, const double t_split_value_,
                   const double t_prediction_, const bool t_is_leaf_) {
    if (t_indicator_index_ >= 0 && (unsigned)t_indicator_index_ >= indicator_vec_.size()) {
      std::ostringstream err_stream_;
      err_stream_ << "TreeLine index: " << tree_index_ << ":" << tree_line_index_ << " uses Incorrect Indicator index "
                  << t_indicator_index_ << " while Total no. of Indicators are " << indicator_vec_.size();
      std::string err_string_ = err_stream_.str();
      ExitVerbose(kModelCreationIndicatorIncorrectArgs, err_string_.c_str());
    }
    TreeLine* t_line_ = new TreeLine();
    t_line_->left_daughter_ = t_left_daughter_;
    t_line_->right_daughter_ = t_right_daughter_;
    t_line_->split_indicator_index_ = t_indicator_index_;
    t_line_->split_indicator_value_ = t_split_value_;
    t_line_->prediction_ = t_prediction_;
    t_line_->is_leaf_ = t_is_leaf_;
    t_line_->node_answer_ = true;
    t_line_->tree_index_ = tree_index_;
    if (tree_line_index_ <= 0) {
      std::vector<TreeLine*> t_tree_;
      t_tree_.push_back(t_line_);
      tree_vec_.push_back(t_tree_);
    } else {
      tree_vec_[tree_index_].push_back(t_line_);
    }
    if (t_indicator_index_ >= 0) {
      indicator_based_nodes_vec_[t_indicator_index_].push_back(t_line_);
    }
  }

  void AddIndicator(CommonIndicator* p_this_indicator_, bool _readiness_required_) {
    if (p_this_indicator_ != NULL) {
      indicator_vec_.push_back(p_this_indicator_);
      p_this_indicator_->add_unweighted_indicator_listener(indicator_vec_.size() - 1, this);
      is_ready_vec_.push_back(p_this_indicator_->IsIndicatorReady());
      readiness_required_vec_.push_back(_readiness_required_);
      prev_value_vec_.push_back(0.0);
      std::vector<TreeLine*> indicator_based_nodes_;
      TreeLine* t_line_ = new TreeLine();
      indicator_based_nodes_.push_back(t_line_);
      indicator_based_nodes_vec_.push_back(indicator_based_nodes_);
      prev_end_index_vec_.push_back(0);
    } else {
      if (dbglogger_.CheckLoggingLevel(DBG_MODEL_ERROR)) {
        DBGLOG_TIME_CLASS_FUNC << "NULL t_indicator_" << DBGLOG_ENDL_FLUSH;
      }
      ExitVerbose(kExitErrorCodeGeneral, dbglogger_);
    }
  }

  void set_basepx_pxtype() {
    for (auto i = 0u; i < indicator_vec_.size(); i++) {
      indicator_vec_[i]->set_basepx_pxtype(dep_market_view_, dep_baseprice_type_);
    }
  }

  void FinishCreation() {
    if (is_ready_vec_.empty()) {
      is_ready_ = true;
      DBGLOG_TIME_CLASS_FUNC << "No Indicators! So by default Ready!" << DBGLOG_ENDL_FLUSH;
    }
    for (auto i = 0u; i < indicator_based_nodes_vec_.size(); i++) {
      std::sort(indicator_based_nodes_vec_[i].begin() + 1, indicator_based_nodes_vec_[i].end(), TreeNodeComparision);
    }
  }

  void InterpretModelParameters(const std::vector<const char*> _tokens_) {
    num_trees_ = atoi(_tokens_[1]);
    max_nodes_ = atoi(_tokens_[2]);
  }

  void ShowIndicatorValues() {
    if (dbglogger_.CheckLoggingLevel(DBG_MODEL_ERROR)) {
      DBGLOG_TIME_CLASS_FUNC << "\n================================================================="
                             << DBGLOG_ENDL_FLUSH;
      dbglogger_ << " sum_vars: " << sum_vars_ << DBGLOG_ENDL_FLUSH;
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

  inline void CalcAndPropagate() {
    if (is_ready_) {
      double new_target_bias_ = sum_vars_;
      if (use_mid_price_base_) {
        new_target_bias_ = new_target_bias_ - p_diff_price_indicator_->indicator_value(is_ready_);
      }
      double new_target_price_;
      if (use_implied_price_) {
        double implied_price_indicator_value_ = p_implied_price_indicator_->IsIndicatorReady()
                                                    ? p_implied_price_indicator_->GetBaseImpliedPrice()
                                                    : dep_market_view_.price_from_type(dep_baseprice_type_);
        new_target_price_ = new_target_bias_ + implied_price_indicator_value_;
      } else {
        new_target_price_ = dep_market_view_.price_from_type(dep_baseprice_type_) + new_target_bias_;
      }
      // const double kMinTicksMoved = 0.015;
      double kMinTicksMoved = min_ticks_move_;
      if (fabs(new_target_price_ - last_propagated_target_price_) >
          (kMinTicksMoved * dep_market_view_.min_price_increment())) {
        PropagateNewTargetPrice(new_target_price_, new_target_bias_);
        last_propagated_target_price_ = new_target_price_;
      }
    } else {
      if (last_is_ready_) {
        PropagateNotReady();
        last_is_ready_ = false;
      }
    }
  }
};
}
#endif  // BASE_MODELMATH_TREE_BOOSTING_MODEL_AGGREGATOR_H
