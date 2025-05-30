/**
        \file ModelMath/linear_model_aggregator.hpp

        \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
Address:
Suite No 353, Evoma, #14, Bhattarhalli,
Old Madras Road, Near Garden City College,
KR Puram, Bangalore 560049, India
+91 80 4190 3551
*/
#ifndef BASE_MODELMATH_NON_LINEAR_MODEL_AGGREGATOR_H
#define BASE_MODELMATH_NON_LINEAR_MODEL_AGGREGATOR_H

#include <assert.h>
// #include "dvccode/Profiler/cpucycle_profiler.hpp"

#include "dvctrade/ModelMath/base_model_math.hpp"

namespace HFSAT {
class NonLinearModelAggregator : public BaseModelMath {
 protected:
  SecurityMarketView& dep_market_view_;
  std::vector<double> prev_value_vec_;
  double sum_vars_;
  double last_propagated_target_price_;
  double model_intercept_;

  PriceType_t dep_baseprice_type_;
  const bool is_returns_based_;
  bool initial_update_baseprice_;
  double last_updated_baseprice_;

  int last_indicators_debug_print_;
  int t_debug_print_counter_;

  std::vector<NonLinearWrapper*> non_linear_component_vec_;
  std::vector<std::vector<int>*> indicator_index_to_affected_non_linear_vector_component_vec_;
  unsigned int last_logged_msec_;

 public:
  NonLinearModelAggregator(DebugLogger& _dbglogger_, const Watch& _watch_, const std::string& _model_filename_,
                           SecurityMarketView& _dep_market_view_, PriceType_t _dep_baseprice_type_,
                           bool _is_returns_based_)
      : BaseModelMath(_dbglogger_, _watch_, _model_filename_),
        dep_market_view_(_dep_market_view_),
        prev_value_vec_(),
        sum_vars_(0),
        last_propagated_target_price_(0),
        model_intercept_(0),
        dep_baseprice_type_(_dep_baseprice_type_),
        is_returns_based_(_is_returns_based_),
        initial_update_baseprice_(false),
        last_updated_baseprice_(1),
        last_indicators_debug_print_(0),
        t_debug_print_counter_(0),
        indicator_index_to_affected_non_linear_vector_component_vec_(),
        last_logged_msec_(0) {
    prev_value_vec_.clear();
    if (!dep_market_view_.subscribe_price_type(
            NULL, dep_baseprice_type_))  // To make sure dep_baseprice_type_ price is computed.
    {
      PriceType_t t_error_price_type_ = dep_baseprice_type_;
      std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << " passed "
                << t_error_price_type_ << std::endl;
    }
    SetPropagateThresholds(dep_market_view_.shortcode());
  }

  virtual ~NonLinearModelAggregator() {}
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

        // if is_returns_based_ then we should multiply all the weights by the baseprice to avoid an additional
        // multiplication of the sum of indicator values.
        if ((is_returns_based_) && (initial_update_baseprice_ == false)) {
          MultiplyIndicatorNodeValuesBy(dep_market_view_.price_from_type(dep_baseprice_type_));
          last_updated_baseprice_ = dep_market_view_.price_from_type(dep_baseprice_type_);
          initial_update_baseprice_ = true;
        }
      }
    }
  }

  // not using now, assuming a model file will have the correct indexes

  //    //find the indicator index corrosponding the indicator descriptor from the model file for the non-linear wrapper
  //    construction
  //    inline int GetIndicatorIndexFromConciseDescription ( const std::string & this_indicator_concise_description_ ) {
  //
  //      for( unsigned int indicator_index_counter_ = 0 ; indicator_index_counter_ < indicator_vec_.size () ;
  //      indicator_index_counter_ ++ ) {
  //
  //        if ( indicator_vec_ [ indicator_index_counter_ ] -> concise_indicator_description () ==
  //        this_indicator_concise_description_ ) {
  //
  //          return indicator_index_counter_ ;
  //
  //        }
  //
  //      }
  //
  //      return -1 ;
  //
  //    }

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

                // Add to Stream
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

      /// check incorrect for nonlinear
      // if ( fabs ( _new_value_ - prev_value_vec_ [ _indicator_index_ ] ) > 10 * dep_market_view_.min_price_increment (
      // ) )
      //   {
      //     if ( ! dbglogger_.IsNoLogs ( ) )
      // 	{
      // 	  std::cerr << watch_.tv() << ' ' << typeid( *this ).name() << ':' << __func__ << ' '
      // 		    << "HUGE value in sum_vars_. last updated : "
      // 		    << _indicator_index_ << " "
      // 		    << indicator_vec_[_indicator_index_]->concise_indicator_description()
      // 		    << " ptgt: " << ( dep_market_view_.price_from_type ( dep_baseprice_type_ ) + sum_vars_ )
      // 		    << " ntgt: " << ( dep_market_view_.price_from_type ( dep_baseprice_type_ ) + sum_vars_ + (
      // _new_value_ - prev_value_vec_ [ _indicator_index_ ] ) )
      // 		    << " numticks " << ( fabs ( _new_value_ - prev_value_vec_ [ _indicator_index_ ] ) /
      // dep_market_view_.min_price_increment ( ) )
      // 		    << std::endl;

      // 	  indicator_vec_[_indicator_index_]->PrintDebugInfo();

      // 	  DBGLOG_TIME_CLASS_FUNC
      // 	    << "HUGE in sum_vars_. last updated : "
      // 	    << _indicator_index_ << " "
      // 	    << indicator_vec_[_indicator_index_]->concise_indicator_description()
      // 	    << " ptgt: " << ( dep_market_view_.price_from_type ( dep_baseprice_type_ ) + sum_vars_ )
      // 	    << " ntgt: " << ( dep_market_view_.price_from_type ( dep_baseprice_type_ ) + sum_vars_ + (
      // _new_value_ - prev_value_vec_ [ _indicator_index_ ] ) )
      // 	    << DBGLOG_ENDL_FLUSH ;
      // 	}
      //   }

      double this_new_value_ = 0.0;

      // only affected indicator values to be computed

      for (unsigned int affected_nonlinear_component_counter_ = 0;
           affected_nonlinear_component_counter_ <
           indicator_index_to_affected_non_linear_vector_component_vec_[_indicator_index_]->size();
           affected_nonlinear_component_counter_++) {
        int component_index_ = indicator_index_to_affected_non_linear_vector_component_vec_
            [_indicator_index_][0][affected_nonlinear_component_counter_];

        //	    if(this_new_value_ != 0.0 ) { std::cerr<< "Yes I am counted twice...: " << component_index_ <<
        // std::endl; }
        this_new_value_ = non_linear_component_vec_[component_index_]->GetComputedValueFromNewIndicatorValue(
            _new_value_, _indicator_index_);
        if (fabs(this_new_value_) >= 8 * dep_market_view_.min_price_increment())
          continue;  // skip very huge value updates

        sum_vars_ += (this_new_value_ - prev_value_vec_[component_index_]);

        prev_value_vec_[component_index_] = this_new_value_;
      }

      if (dbglogger_.CheckLoggingLevel(TRADEINIT_INFO)) {
        if ((last_indicators_debug_print_ == 0) ||
            (watch_.msecs_from_midnight() > last_indicators_debug_print_ + 2000)) {
          last_indicators_debug_print_ = watch_.msecs_from_midnight();

          ShowIndicatorValuesToScreen();
        }
      }

      if ((watch_.msecs_from_midnight() - last_logged_msec_ > 1000) &&
          (fabs(sum_vars_) > 15 * dep_market_view_.min_price_increment())) {
        std::cerr << "HIGH_VALUE: " << watch_.tv()
                  << "\tSumVARTicks: " << sum_vars_ / dep_market_view_.min_price_increment()
                  << " Indicator: " << _new_value_ << " " << _indicator_index_ << std::endl;
        last_logged_msec_ = watch_.msecs_from_midnight();
      }
    }
  }

  void dumpIndicatorValidationData() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << " Debug \n";
    for (auto i = 0u; i < indicator_vec_.size(); i++) {
      t_temp_oss_ << (indicator_vec_[i]->concise_indicator_description()) << " WEIGHT "
                  << (indicator_vec_[i]->GetIndicatorListenerWeight(this)) << "\n";
      t_temp_oss_ << " Affected nonlinear indicators \n";
      for (unsigned int j = 0; j < indicator_index_to_affected_non_linear_vector_component_vec_[i]->size(); j++) {
        int component_index_ = indicator_index_to_affected_non_linear_vector_component_vec_[i][0][j];
        t_temp_oss_ << non_linear_component_vec_[component_index_]->getStringDesc();
      }
      t_temp_oss_ << "\n\n";
    }
    for (auto i = 0u; i < non_linear_component_vec_.size(); i++) {
      t_temp_oss_ << "Nonlinear component " << non_linear_component_vec_[i]->getStringDesc() << "\n";
      t_temp_oss_ << non_linear_component_vec_[i]->dumpValues();
    }
    t_temp_oss_ << "------------------------------------------------------\n\n\n";
    std::cerr << t_temp_oss_.str();
  }

  // ModelMath objects are notified after all the indicators are updated.
  // Indicators get updates from
  // (i) Portfolio which listens to SMV updates
  // (ii) SMV updates
  // (iii) GlobalPositionChange
  inline void SMVOnReady() {
    // based on mktpx - midpx
    // we could give a weight
    // like when ( 2 * abs ( mktpx - midpx ) ) / ( spread ) > 0.45
    // that means
    CalcAndPropagate();
  }

  inline void OnGlobalPositionChange(const unsigned int _security_id_, int _new_global_position_) {
    CalcAndPropagate();
  }

  void AddIntercept(const double& _this_weight_constant_) {
    sum_vars_ = _this_weight_constant_;
    model_intercept_ = _this_weight_constant_;
  }

  void AddIndicator(CommonIndicator* p_this_indicator_, const double& _this_weight_, bool _readiness_required_) {
    if (p_this_indicator_ != NULL) {
      if (is_returns_based_ && initial_update_baseprice_) {
        p_this_indicator_->add_indicator_listener(
            indicator_vec_.size(), this,
            (_this_weight_ *
             last_updated_baseprice_));  // premultiply the basepx to avoid multiplication during model computation
      } else {
        p_this_indicator_->add_indicator_listener(indicator_vec_.size(), this, _this_weight_);
      }
      indicator_vec_.push_back(p_this_indicator_);

      const bool t_is_this_indicator_ready_ = p_this_indicator_->IsIndicatorReady();
      if (t_is_this_indicator_ready_ == false) {
        is_ready_vec_.push_back(false);
        readiness_required_vec_.push_back(_readiness_required_);
        is_ready_ = false;  // since is_ready_vec_ is now sure not to be false;
      } else {
        is_ready_vec_.push_back(true);
        readiness_required_vec_.push_back(false);
      }

      std::vector<int>* this_new_non_linear_component_index_vec_ = new std::vector<int>();
      indicator_index_to_affected_non_linear_vector_component_vec_.push_back(this_new_non_linear_component_index_vec_);

    } else {
      if (dbglogger_.CheckLoggingLevel(DBG_MODEL_ERROR)) {
        DBGLOG_TIME_CLASS_FUNC << "NULL t_indicator_" << DBGLOG_ENDL_FLUSH;
      }
      ExitVerbose(kExitErrorCodeGeneral, dbglogger_);
    }
  }

  void AddNonLinearComponent(NonLinearWrapper* p_this_non_linear_component_) {
    if (p_this_non_linear_component_ != NULL) {
      non_linear_component_vec_.push_back(p_this_non_linear_component_);
      prev_value_vec_.push_back(0.0);

      for (int this_non_linear_base_wrapper_counter_ = 0;
           this_non_linear_base_wrapper_counter_ < (p_this_non_linear_component_->TotalBaseElements());
           this_non_linear_base_wrapper_counter_++) {
        int this_indicator_index_ =
            p_this_non_linear_component_->GetIndicatorIndexByElementIndex(this_non_linear_base_wrapper_counter_);

        indicator_index_to_affected_non_linear_vector_component_vec_[this_indicator_index_]->push_back(
            non_linear_component_vec_.size() - 1);
      }

    } else {
      if (dbglogger_.CheckLoggingLevel(DBG_MODEL_ERROR)) {
        DBGLOG_TIME_CLASS_FUNC << "NULL t_nonlinear_model_component_" << DBGLOG_ENDL_FLUSH;
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
  }

  void InterpretModelParameters(const std::vector<const char*> _tokens_) {  // MODELARGS ...
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
    // if dependent not ready, all not ready
    if (!dep_market_view_.is_ready()) return false;

    // instead of just checking is ready of all indicators,
    // only checking for indicators with readiness_required_vec_ [i] == true
    for (auto i = 0u; i < is_ready_vec_.size(); ++i) {
      if (is_ready_vec_[i] == false && readiness_required_vec_[i] == true) {
        return false;
      }
    }
    return true;
    // earlier was
    // return VectorUtils::CheckAllForValue ( is_ready_vec_, true ) ;
  }

  /// called to tell the indicator that this listener would now want the
  /// premultiplied weight of values to be different
  void MultiplyIndicatorNodeValuesBy(const double& _mult_factor_) {
    for (auto i = 0u; i < non_linear_component_vec_.size(); i++) {
      non_linear_component_vec_[i]->MultiplyWeight(_mult_factor_);
    }
    sum_vars_ += (_mult_factor_ - 1) * model_intercept_;
    model_intercept_ *= _mult_factor_;
  }

  inline void CalcAndPropagate() {
    if (is_ready_) {
      // compute the new target and see if it needs to be sent ahead to listeners
      // note that returns based or not has already been taken care of in th multiplier
      // so now only addition to dep_baseprice_ is needed
      double new_target_price_ = dep_market_view_.price_from_type(dep_baseprice_type_) + sum_vars_;

      // debug
      // if( fabs( sum_vars_ ) > 0.001* new_target_price_ && t_debug_print_counter_ < 100 )
      if ((last_logged_msec_ - watch_.msecs_from_midnight() > 1000) &&
          (fabs(sum_vars_) > 15 * dep_market_view_.min_price_increment()) && t_debug_print_counter_ < 1000) {
        t_debug_print_counter_++;
        if (!dbglogger_.IsNoLogs()) {
          std::cerr << "HUGE values \n";
        }

        for (auto i = 0u; i < non_linear_component_vec_.size(); i++) {
          if (fabs(non_linear_component_vec_[i]->lastValue()) > 10 * dep_market_view_.min_price_increment())
            // std::cerr << non_linear_component_vec_[ i ] -> getStringDesc( ) << " value " <<
            // non_linear_component_vec_[ i ] -> lastValue( ) << "\n";
            if (!dbglogger_.IsNoLogs()) {
              std::cerr << non_linear_component_vec_[i]->dumpValues();
            }
        }
        if (!dbglogger_.IsNoLogs()) {
          std::cerr << "SumVars: " << sum_vars_ << " MinPriceInc: " << dep_market_view_.min_price_increment() << "\n\n";
        }

        last_logged_msec_ = watch_.msecs_from_midnight();
      }
      // const double kMinTicksMoved = 0.015 ;
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
};
}
#endif  // BASE_MODELMATH_LINEAR_MODEL_AGGREGATOR_H
