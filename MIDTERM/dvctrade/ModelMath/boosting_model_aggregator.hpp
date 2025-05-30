// =====================================================================================
//
//       Filename:  boosting_model_aggregator.hpp
//
//    Description:  This gives returns the target value after choosing conditionally from multiple model
//
//        Version:  1.0
//        Created:  Wednesday 22 January 2014 08:33:09  GMT
//       Revision:  none
//       Compiler:  g++
//
//         Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
//
//        Address:  Suite No 353, Evoma, #14, Bhattarhalli,
//                  Old Madras Road, Near Garden City College,
//                  KR Puram, Bangalore 560049, India
//          Phone:  +91 80 4190 3551
//
// =====================================================================================
#ifndef BASE_MODELMATH_BOOSTING_MODEL_AGGREGATOR_H
#define BASE_MODELMATH_BOOSTING_MODEL_AGGREGATOR_H

#include "dvctrade/ModelMath/base_model_math.hpp"

namespace HFSAT {
#define MAX_INDICATORS 25
#define MAX_MODELS 50

class BoostingModelAggregator : public BaseModelMath {
 protected:
  SecurityMarketView& dep_market_view_;
  std::vector<std::string>& core_shortcodes_;
  PriceType_t dep_baseprice_type_;

  double beta_[MAX_MODELS];                // std::vector < double > beta_;
  double prev_value_vec_[MAX_INDICATORS];  // std::vector < double > prev_value_vec_;

  unsigned orig_idx_to_current_idx_[MAX_MODELS];          // std::vector < unsigned > orig_idx_to_current_idx_;
  std::map<std::string, unsigned> indicator_desc_to_id_;  // used initially, ignore

  double indicator_weights_[MAX_MODELS][MAX_INDICATORS];  // std::vector < std::vector < double > > indicator_weights_;
  double max_indicator_weights_[MAX_INDICATORS];          // just for efficiently checking for HUGE vals
  double sum_vars_vec_1_[MAX_MODELS];  // std::vector < std::pair < double , unsigned > > sum_vars_vec_;
  unsigned sum_vars_vec_2_[MAX_MODELS];
  // we can skip this, assume an indicator is present in all models, in case
  // it is not present, assume its weight is zero
  // unsigned indicator_idx_to_models_[MAX_INDICATORS][MAX_MODELS];//std::vector < std::vector < unsigned > >
  // indicator_idx_to_models_;

  bool is_dirty_;
  unsigned num_models_;
  unsigned num_indicators_;
  unsigned tmp_indicators_count_;
  double sum_vars_;
  double sum_beta_;
  // double last_price_;

  bool is_returns_based_;
  bool initial_update_baseprice_;
  int last_indicators_debug_print_;
  unsigned current_model_;
  double model_intercept_;
  double last_updated_baseprice_;
  double last_propagated_target_price_;

 public:
  BoostingModelAggregator(DebugLogger& _dbglogger_, const Watch& _watch_, const std::string& _model_file_name_,
                          SecurityMarketView& _dep_market_view_, PriceType_t _dep_baseprice_type_,
                          bool _is_returns_based_, std::vector<std::string>& _core_shortcodes_)
      : BaseModelMath(_dbglogger_, _watch_, _model_file_name_),
        dep_market_view_(_dep_market_view_),
        core_shortcodes_(_core_shortcodes_),
        dep_baseprice_type_(_dep_baseprice_type_),
        is_dirty_(false),
        num_models_(0),
        num_indicators_(0),
        sum_vars_(0),
        sum_beta_(0),
        is_returns_based_(_is_returns_based_),
        initial_update_baseprice_(false),
        last_indicators_debug_print_(0),
        current_model_(0),
        model_intercept_(0),
        last_updated_baseprice_(0),
        last_propagated_target_price_(0) {
    tmp_indicators_count_ = 0;
    if (!dep_market_view_.subscribe_price_type(NULL, dep_baseprice_type_)) {
      PriceType_t t_error_price_type_ = dep_baseprice_type_;
      std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << " passed "
                << t_error_price_type_ << std::endl;
    }
    if (!dep_market_view_.subscribe_price_type(NULL, kPriceTypeMidprice)) {
      std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << " passed " << kPriceTypeMidprice
                << std::endl;
    }
    memset(indicator_weights_, 0, sizeof indicator_weights_);
    memset(max_indicator_weights_, 0, sizeof max_indicator_weights_);
    SetPropagateThresholds(dep_market_view_.shortcode());
    GetHugeValueThreshold(dep_market_view_.shortcode());
  }

  virtual ~BoostingModelAggregator() {}

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
      if (is_ready_) {
        if (!dbglogger_.IsNoLogs()) {
          DBGLOG_TIME_CLASS_FUNC << "All Indicators Ready " << DBGLOG_ENDL_FLUSH;
          DBGLOG_DUMP;
        }
        last_is_ready_ = true;
        if ((is_returns_based_) && (initial_update_baseprice_ == false)) {
          MultiplyIndicatorNodeValuesBy(dep_market_view_.price_from_type(dep_baseprice_type_));
          last_updated_baseprice_ = dep_market_view_.price_from_type(dep_baseprice_type_);
          initial_update_baseprice_ = true;
        }
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
    if (!is_ready_ && !is_ready_vec_[_indicator_index_]) {
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
        if ((is_returns_based_) && (initial_update_baseprice_ == false)) {
          MultiplyIndicatorNodeValuesBy(dep_market_view_.price_from_type(dep_baseprice_type_));
          last_updated_baseprice_ = dep_market_view_.price_from_type(dep_baseprice_type_);
          initial_update_baseprice_ = true;
        }
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

      double max_change_in_ind_val_ =
          max_indicator_weights_[_indicator_index_] * (_new_value_ - prev_value_vec_[_indicator_index_]);
      if (fabs(max_change_in_ind_val_) > huge_value_threshold_ * dep_market_view_.min_price_increment()) {
        if (!dbglogger_.IsNoLogs()) {
          std::cerr << watch_.tv() << ' ' << typeid(*this).name() << ':' << __func__ << ' '
                    << "HUGE value in sum_vars_. last updated : " << _indicator_index_ << " "
                    << indicator_vec_[_indicator_index_]->concise_indicator_description()
                    << " ptgt: " << (dep_market_view_.price_from_type(dep_baseprice_type_) + sum_vars_) << " ntgt: "
                    << (dep_market_view_.price_from_type(dep_baseprice_type_) + sum_vars_ + (max_change_in_ind_val_))
                    << " numticks " << (fabs(max_change_in_ind_val_) / dep_market_view_.min_price_increment())
                    << std::endl;
          indicator_vec_[_indicator_index_]->PrintDebugInfo();
          DBGLOG_TIME_CLASS_FUNC << "HUGE in sum_vars_. last updated : " << _indicator_index_ << " "
                                 << indicator_vec_[_indicator_index_]->concise_indicator_description()
                                 << " ptgt: " << (dep_market_view_.price_from_type(dep_baseprice_type_) + sum_vars_)
                                 << " ntgt: " << (dep_market_view_.price_from_type(dep_baseprice_type_) + sum_vars_ +
                                                  (max_change_in_ind_val_))
                                 << " numticks "
                                 << (fabs(max_change_in_ind_val_) / dep_market_view_.min_price_increment())
                                 << DBGLOG_ENDL_FLUSH;
        }
      }
      if (prev_value_vec_[_indicator_index_] == _new_value_) {
        ;  // no need to update
      } else {
        // update all models in which this indicator is
        // for ( unsigned i = 0; i < indicator_idx_to_models_ [ _indicator_index_ ].size ( ); i++) {
        for (unsigned i = 0; i < num_models_; i++) {
          // unsigned model_idx_ = indicator_idx_to_models_ [ _indicator_index_ ][ i ];
          // sum_vars_vec_ [ orig_idx_to_current_idx_ [ model_idx_ ] ].first += indicator_weights_[ model_idx_ ][
          // _indicator_index_ ] * ( _new_value_ - prev_value_vec_ [ _indicator_index_ ] );
          sum_vars_vec_1_[orig_idx_to_current_idx_[i]] +=
              indicator_weights_[i][_indicator_index_] * (_new_value_ - prev_value_vec_[_indicator_index_]);
        }
        prev_value_vec_[_indicator_index_] = _new_value_;
        is_dirty_ = true;
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
            (watch_.msecs_from_midnight() > last_indicators_debug_print_ + SHOW_IND_MSECS_TIMEOUT)) {
          last_indicators_debug_print_ = watch_.msecs_from_midnight();
          ShowIndicatorValues();
        }
      }
    }
  }

  inline void SMVOnReady() { CalcAndPropagate(); }

  inline void OnGlobalPositionChange(const unsigned int _security_id_, int _new_global_position_) {
    CalcAndPropagate();
  }

  void AddIndicator(CommonIndicator* p_this_indicator_, const double& _this_weight_, bool _readiness_required_) {
    if (p_this_indicator_ != NULL) {
      std::string desc_ = p_this_indicator_->concise_indicator_description();
      unsigned indicator_id_;
      if (indicator_desc_to_id_.find(desc_) != indicator_desc_to_id_.end()) {
        indicator_id_ = indicator_desc_to_id_[desc_];
        // indicator_idx_to_models_ [ indicator_id_ ].push_back ( num_models_ );
        // indicator_idx_to_models_ [ indicator_id_ ][ num_models_ ] = num_models_;
      } else {
        p_this_indicator_->add_unweighted_indicator_listener(num_indicators_, this);
        // indicator_idx_to_models_.push_back ( std::vector<unsigned int>() );
        // indicator_idx_to_models_ [ num_indicators_ ].push_back ( num_models_ );
        // indicator_idx_to_models_ [ num_indicators_ ][ 0 ] = num_models_;
        indicator_id_ = num_indicators_;
        indicator_desc_to_id_[desc_] = num_indicators_;
        indicator_vec_.push_back(p_this_indicator_);
        is_ready_vec_.push_back(p_this_indicator_->IsIndicatorReady());
        readiness_required_vec_.push_back(_readiness_required_);
        if (!p_this_indicator_->IsIndicatorReady()) {
          is_ready_ = false;
        }
        prev_value_vec_[num_indicators_] = 0.0;  //.push_back ( 0.0 );
        num_indicators_++;
      }
      if (is_returns_based_ && initial_update_baseprice_) {
        // indicator_weights_ [ num_models_ ].push_back ( _this_weight_ * last_updated_baseprice_ );
        // indicator_weights_ [ num_models_ ][ tmp_indicators_count_ ] = _this_weight_ * last_updated_baseprice_;
        indicator_weights_[num_models_][indicator_id_] = _this_weight_ * last_updated_baseprice_;
      } else {
        // indicator_weights_ [ num_models_ ].push_back ( _this_weight_ );
        // indicator_weights_ [ num_models_ ][ tmp_indicators_count_ ] = _this_weight_;
        indicator_weights_[num_models_][indicator_id_] = _this_weight_;
      }
      if (fabs(indicator_weights_[num_models_][indicator_id_]) > fabs(max_indicator_weights_[indicator_id_])) {
        max_indicator_weights_[indicator_id_] = indicator_weights_[num_models_][indicator_id_];
      }
      tmp_indicators_count_++;
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

  void StartCreation(double _beta_) {
    beta_[num_models_] = _beta_;
    // beta_.push_back ( _beta_ );
    // indicator_weights_.push_back ( std::vector<double> ( ) );
    /*
       if ( dbglogger_ . CheckLoggingLevel ( DBG_MODEL_ERROR ) )
       {
       DBGLOG_TIME_CLASS_FUNC << " Model Number " << num_models_ << " MStartIndexSize: "
       << model_start_index_.size() << " IndicatorVecSize: " << indicator_vec_.size()
       << DBGLOG_ENDL_FLUSH;
       }
       */
  }

  void FinishCreation(bool all_models_finished_) {
    num_models_++;
    if (all_models_finished_) {
      // sum_vars_vec_.resize( num_models_);
      // orig_idx_to_current_idx_.resize( num_models_, 0 );
      for (unsigned j = 0; j < num_models_; j++)  // sum_vars_vec_.size ( ); j++ )
      {
        // sum_vars_vec_ [ j ].second = j;
        // orig_idx_to_current_idx_ [ j ] = j;
        sum_vars_vec_2_[j] = j;
        orig_idx_to_current_idx_[j] = j;
        sum_beta_ += beta_[j];
      }
      // std::cout << num_models_ << std::endl;
      // std::cout << num_indicators_ << std::endl;
    }
    DBGLOG_TIME_CLASS_FUNC << " Model No: " << num_models_ << DBGLOG_ENDL_FLUSH;
    /*
        for ( unsigned j = this_model_start_ ; j < this_model_end_; j++)
        {
    //add the original indices here
    DBGLOG_TIME_CLASS_FUNC
    << " Indicator [ " << j <<  " ] " << indicator_vec_[j]->concise_indicator_description()
    << DBGLOG_ENDL_FLUSH;
    }
    */
    /*
       if ( dbglogger_ . CheckLoggingLevel ( DBG_MODEL_ERROR ) )
       {
       DBGLOG_TIME_CLASS_FUNC << " Model Number " << num_models_ << " MStartIndexSize: "
       << model_start_index_.size() << " IndicatorVecSize: " << indicator_vec_.size()
       << DBGLOG_ENDL_FLUSH;
       }*/
    if (is_ready_vec_.empty()) {
      is_ready_ = true;
      DBGLOG_TIME_CLASS_FUNC << "No Indicators! So by default Ready!" << DBGLOG_ENDL_FLUSH;
    }
    DBGLOG_DUMP;
    tmp_indicators_count_ = 0;
  }

  void InterpretModelParameters(const std::vector<const char*> _tokens_) {  // MODELARGS ...
  }

  void ShowIndicatorValues() {
    if (dbglogger_.CheckLoggingLevel(DBG_MODEL_ERROR)) {
      DBGLOG_TIME_CLASS_FUNC << "sum vars: " << sum_vars_ << DBGLOG_ENDL_FLUSH;
      DBGLOG_TIME_CLASS_FUNC << "\n================================================================="
                             << DBGLOG_ENDL_FLUSH;
      /*
         for ( unsigned int i = model_start_index_[current_model_] ; i < model_end_index_[current_model_] ; i ++ )
         {
         dbglogger_
         << " value: " << ( prev_value_vec_ [ i ] / dep_market_view_.min_price_increment() )
         << " of " << indicator_vec_[ i ]->concise_indicator_description()
         << DBGLOG_ENDL_FLUSH ;
         }
         */
      dbglogger_ << "=================================================================" << DBGLOG_ENDL_FLUSH;
    }
  }

  void ShowIndicatorValuesToScreen() {
    printf("\033[36;1H");  // move to 35 th line
    std::cout << "\n=================================================================" << std::endl;
    /*
       for ( unsigned int i = model_start_index_[current_model_] ; i < model_end_index_[current_model_] ; i ++ )
       {
       std::cout
       << " value: " << ( prev_value_vec_ [ i ] / dep_market_view_.min_price_increment() )
       << " of " << indicator_vec_[ i ]->concise_indicator_description()
       << "                  "
       << std::endl;
       }*/
    std::cout << "=================================================================" << std::endl;
  }

  /// debug info
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
    if (!dep_market_view_.is_ready()) return false;
    for (auto i = 0u; i < is_ready_vec_.size(); ++i) {
      if (is_ready_vec_[i] == false && readiness_required_vec_[i] == true) {
        return false;
      }
    }
    return true;
  }

  /// called to tell the indicator that this listener would now want the
  /// premultiplied weight of values to be different
  void MultiplyIndicatorNodeValuesBy(const double& _mult_factor_) {
    for (auto i = 0u; i < indicator_vec_.size(); i++) {
      indicator_vec_[i]->MultiplyIndicatorListenerWeight(this, _mult_factor_);
    }
    sum_vars_ += (_mult_factor_ - 1) * model_intercept_;
    model_intercept_ *= _mult_factor_;
  }

  inline void CalcAndPropagate() {
    if (is_ready_) {
      // compute the new target and see if it needs to be sent ahead to listeners
      // note that returns based or not has already been taken care of in the multiplier
      // so now only addition to dep_baseprice_ is needed
      if (is_dirty_) {
        CalculateSumVars();
        is_dirty_ = false;
      }
      double new_target_price_ = dep_market_view_.price_from_type(dep_baseprice_type_) + sum_vars_;
      // const double kMinTicksMoved = 0.015;
      double kMinTicksMoved = min_ticks_move_;
      if (fabs(new_target_price_ - last_propagated_target_price_) >
          (kMinTicksMoved * dep_market_view_.min_price_increment())) {
        PropagateNewTargetPrice(new_target_price_, sum_vars_);
        last_propagated_target_price_ = new_target_price_;
      }
      // if this is based on predicting returns and not change
      // then periodically update the multipliers with a multiplying factor
      if (is_returns_based_ && (fabs(dep_market_view_.price_from_type(dep_baseprice_type_) - last_updated_baseprice_) >
                                5 * dep_market_view_.min_price_increment())) {
        double _this_mult_factor_ = dep_market_view_.price_from_type(dep_baseprice_type_) / last_updated_baseprice_;
        MultiplyIndicatorNodeValuesBy(_this_mult_factor_);
        last_updated_baseprice_ = dep_market_view_.price_from_type(dep_baseprice_type_);
      }
    } else {
      if (last_is_ready_) {
        PropagateNotReady();
        last_is_ready_ = false;
      }
    }
  }

  void CalculateSumVars() {
    // return the weighted median
    // std::sort( sum_vars_vec_.begin ( ), sum_vars_vec_.end ( ) );
    for (unsigned i = 1; i < num_models_; i++) {
      unsigned j = i;
      while (j > 0 && sum_vars_vec_1_[j] < sum_vars_vec_1_[j - 1]) {
        std::swap(sum_vars_vec_1_[j], sum_vars_vec_1_[j - 1]);
        std::swap(sum_vars_vec_2_[j], sum_vars_vec_2_[j - 1]);
        j--;
      }
    }
    double current_sum_ = 0;
    int current_idx_ = 0;
    while (current_sum_ <= sum_beta_ / 2) {
      current_sum_ += beta_[sum_vars_vec_2_[current_idx_]];  // beta_ [ sum_vars_vec_[ current_idx_ ].second ];
      current_idx_++;
    }
    current_model_ = current_idx_ - 1;
    sum_vars_ = sum_vars_vec_1_[current_idx_ - 1];  // sum_vars_vec_ [ current_idx_ - 1 ].first;
    for (unsigned i = 0; i < num_models_; i++)      // sum_vars_vec_.size ( ); i++ )
    {
      // orig_idx_to_current_idx_ [ sum_vars_vec_ [ i ].second ] = i;
      orig_idx_to_current_idx_[sum_vars_vec_2_[i]] = i;
    }
  }
};
}
#endif  // BASE_MODELMATH_BOOSTING_MODEL_AGGREGATOR_H
