/**
        \file ModelMath/logistic_model_aggregator.hpp

        \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
Address:
Suite No 353, Evoma, #14, Bhattarhalli,
Old Madras Road, Near Garden City College,
KR Puram, Bangalore 560049, India
+91 80 4190 3551
*/
#ifndef BASE_MODELMATH_LOGISTIC_MODEL_AGGREGATOR_H
#define BASE_MODELMATH_LOGISTIC_MODEL_AGGREGATOR_H

#include <assert.h>
#include "dvccode/Profiler/cpucycle_profiler.hpp"

#include "dvctrade/ModelMath/base_model_math.hpp"

namespace HFSAT {
class LogisticModelAggregator : public BaseModelMath {
 protected:
  SecurityMarketView& dep_market_view_;
  std::vector<double> prev_value_vec_decrease_;
  std::vector<double> prev_value_vec_nochange_;
  std::vector<double> prev_value_vec_increase_;
  double sum_vars_decrease_;
  double sum_vars_nochange_;
  double sum_vars_increase_;
  double last_propagated_target_price_;

  PriceType_t dep_baseprice_type_;
  const bool is_returns_based_;
  bool initial_update_baseprice_;
  double last_updated_baseprice_;

  int last_indicators_debug_print_;
  bool flag;

 public:
  LogisticModelAggregator(DebugLogger& _dbglogger_, const Watch& _watch_, const std::string& _model_filename_,
                          SecurityMarketView& _dep_market_view_, PriceType_t _dep_baseprice_type_,
                          bool _is_returns_based_)
      : BaseModelMath(_dbglogger_, _watch_, _model_filename_),
        dep_market_view_(_dep_market_view_),
        prev_value_vec_decrease_(),
        prev_value_vec_nochange_(),
        prev_value_vec_increase_(),
        sum_vars_decrease_(0.0),
        sum_vars_nochange_(0.0),
        sum_vars_increase_(0.0),
        last_propagated_target_price_(0),
        dep_baseprice_type_(_dep_baseprice_type_),
        is_returns_based_(_is_returns_based_),
        initial_update_baseprice_(false),
        last_updated_baseprice_(1),
        last_indicators_debug_print_(0),
        flag(false) {
    prev_value_vec_decrease_.clear();
    prev_value_vec_nochange_.clear();
    prev_value_vec_increase_.clear();
    if (!dep_market_view_.subscribe_price_type(
            NULL, dep_baseprice_type_))  // To make sure dep_baseprice_type_ price is computed.
    {
      PriceType_t t_error_price_type_ = dep_baseprice_type_;
      std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << " passed "
                << t_error_price_type_ << std::endl;
    }
  }

  virtual ~LogisticModelAggregator() {}

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
        /*
                 if ( ( is_returns_based_ ) && ( initial_update_baseprice_ == false ) )
                 {
                 MultiplyIndicatorNodeValuesBy ( dep_market_view_.price_from_type ( dep_baseprice_type_ ) ) ;
                 last_updated_baseprice_ = dep_market_view_.price_from_type ( dep_baseprice_type_ ) ;
                 initial_update_baseprice_ = true;
                 }
                 */
      }
    }
  }

  inline void OnControlUpdate(const ControlMessage& _control_message_, const char* symbol_, const int trader_id) {
    switch (_control_message_.message_code_) {
      case kControlMessageCodeShowIndicators: {
        ShowIndicatorValues();
      } break;
      default: { } break; }
  }

  inline void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_) { return; }

  inline void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_decrease_,
                                const double& _new_value_nochange_, const double& _new_value_increase_) {
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
        /*if ( ( is_returns_based_ ) && ( initial_update_baseprice_ == false ) )
                {
                MultiplyIndicatorNodeValuesBy ( dep_market_view_.price_from_type ( dep_baseprice_type_ ) ) ;
                last_updated_baseprice_ = dep_market_view_.price_from_type ( dep_baseprice_type_ ) ;
                initial_update_baseprice_ = true;
                }
                */
      }
    } else {
      /*
               if ( isnan ( _new_value_ ) )
               {
               if ( ! dbglogger_.IsNoLogs ( ) )
               {
               std::cerr << watch_.tv() << ' ' << typeid( *this ).name() << ':' << __func__ << ' '
               << "nan in sum_vars_. last updated : "
               << _indicator_index_ << " "
               << indicator_vec_[_indicator_index_]->concise_indicator_description()
               << std::endl;

               DBGLOG_TIME_CLASS_FUNC << "nan in sum_vars_. last updated : "
               << _indicator_index_ << " "
               << indicator_vec_[_indicator_index_]->concise_indicator_description()
               << DBGLOG_ENDL_FLUSH ;
               }
               is_ready_ = false;
               return;
               }
               */
      // std::cout<<sum_vars_decrease_<<"\t"<<sum_vars_nochange_<<"\t"<<sum_vars_increase_<<"\n";
      sum_vars_decrease_ += _new_value_decrease_ - prev_value_vec_decrease_[_indicator_index_];
      prev_value_vec_decrease_[_indicator_index_] = _new_value_decrease_;
      sum_vars_nochange_ += _new_value_nochange_ - prev_value_vec_nochange_[_indicator_index_];
      prev_value_vec_nochange_[_indicator_index_] = _new_value_nochange_;
      sum_vars_increase_ += _new_value_increase_ - prev_value_vec_increase_[_indicator_index_];
      prev_value_vec_increase_[_indicator_index_] = _new_value_increase_;
    }
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

  void AddIntercept(const double& _this_weight_decrease_, const double& _this_weight_nochange_,
                    const double& _this_weight_increase_) {
    sum_vars_decrease_ = _this_weight_decrease_;
    sum_vars_nochange_ = _this_weight_nochange_;
    sum_vars_increase_ = _this_weight_increase_;
  }

  void AddIndicator(CommonIndicator* p_this_indicator_, const double& _this_weight_decrease_,
                    const double& _this_weight_nochange_, const double& _this_weight_increase_,
                    bool _readiness_required_) {
    if (p_this_indicator_ != NULL) {
      /*
               if ( is_returns_based_ && initial_update_baseprice_ )
               {
               p_this_indicator_->add_indicator_listener ( indicator_vec_.size ( ), this, ( _this_weights_ *
         last_updated_baseprice_ ) ) ; // premultiply the basepx to avoid multiplication during model computation
               }

               else
               */
      {
        p_this_indicator_->add_indicator_listener(indicator_vec_.size(), this, _this_weight_decrease_,
                                                  _this_weight_nochange_, _this_weight_increase_);
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
      prev_value_vec_decrease_.push_back(0.0);
      prev_value_vec_nochange_.push_back(0.0);
      prev_value_vec_increase_.push_back(0.0);
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
  }

  void InterpretModelParameters(const std::vector<const char*> _tokens_) {  // MODELARGS ...
  }

  void ShowIndicatorValues() {
    if (dbglogger_.CheckLoggingLevel(DBG_MODEL_ERROR)) {
      DBGLOG_TIME_CLASS_FUNC << "\n================================================================="
                             << DBGLOG_ENDL_FLUSH;
      for (auto i = 0u; i < indicator_vec_.size(); i++) {
        dbglogger_ << " values: " << prev_value_vec_decrease_[i] << " : " << prev_value_vec_nochange_[i] << " : "
                   << prev_value_vec_increase_[i] << " of " << indicator_vec_[i]->concise_indicator_description()
                   << DBGLOG_ENDL_FLUSH;
      }
      dbglogger_ << "=================================================================" << DBGLOG_ENDL_FLUSH;
    }
  }

  /// debug info
  void DumpIndicatorValues() {
    if (dbglogger_.CheckLoggingLevel(DBG_MODEL_ERROR)) {
      /// TODO
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
  /*
           void MultiplyIndicatorNodeValuesBy ( const double & _mult_factor_ )
           {
           for ( unsigned int i = 0 ; i < indicator_vec_.size ( ) ; i ++ )
           {
           indicator_vec_ [i]->MultiplyIndicatorListenerWeight ( this, _mult_factor_ ) ;
           }
           }
           */

  inline void CalcAndPropagate() {
    if (is_ready_) {
      // compute the new target and see if it needs to be sent ahead to listeners
      // note that returns based or not has already been taken care of in th multiplier
      // so now only addition to dep_baseprice_ is needed

      /*
               int t_max_indx_ = 0;
               int t_max_ = -100;
               for(unsigned int i=0; i<sum_vars_.size();i++)
               {
               if(sum_vars_[i]>t_max_)
               {
               t_max_= sum_vars_[i];
               t_max_indx_ = i;
               }
               }
               double new_target_price_ = dep_market_view_.price_from_type ( dep_baseprice_type_ );
               double target_bias_ = 0.0;

               if(t_max_indx_ == 2){
               if(dep_market_view_.market_update_info_.bestask_price_ > 0)
               new_target_price_ = dep_market_view_.market_update_info_.bestask_price_ ;//+ 1.0 *
         dep_market_view_.min_price_increment();
               else
               new_target_price_ = dep_market_view_.price_from_type ( dep_baseprice_type_ );
               }
               else if(t_max_indx_ == 0){
               if(dep_market_view_.market_update_info_.bestbid_price_ > 0)
               new_target_price_ =  dep_market_view_.market_update_info_.bestbid_price_ ;//- 1.0 *
         dep_market_view_.min_price_increment();
               else
               new_target_price_ = dep_market_view_.price_from_type ( dep_baseprice_type_ );
               }

               target_bias_ = new_target_price_ - dep_market_view_.price_from_type ( dep_baseprice_type_ );

               const double kMinTicksMoved = 0.015 ;
               if ( fabs ( new_target_price_ - last_propagated_target_price_ ) > ( kMinTicksMoved *
         dep_market_view_.min_price_increment ( ) ) )
               {
               PropagateNewTargetPrice ( new_target_price_, target_bias_ ) ;
               last_propagated_target_price_ = new_target_price_ ;
               }
               */
      double max = 0.0;
      if (sum_vars_decrease_ > sum_vars_nochange_) {
        if (sum_vars_decrease_ > sum_vars_increase_) {
          max = sum_vars_decrease_;
        }
        if (sum_vars_increase_ > sum_vars_decrease_) {
          max = sum_vars_increase_;
        }
      } else {
        if (sum_vars_nochange_ > sum_vars_increase_) {
          max = sum_vars_nochange_;
        }
        if (sum_vars_increase_ > sum_vars_nochange_) {
          max = sum_vars_increase_;
        }
      }

      double exp_decrease_ = exp(sum_vars_decrease_ - max);
      double exp_nochange_ = exp(sum_vars_nochange_ - max);
      double exp_increase_ = exp(sum_vars_increase_ - max);

      double prob_decrease_ = exp_decrease_ / (exp_decrease_ + exp_nochange_ + exp_increase_);
      double prob_nochange_ = exp_nochange_ / (exp_decrease_ + exp_nochange_ + exp_increase_);
      double prob_increase_ = exp_increase_ / (exp_decrease_ + exp_nochange_ + exp_increase_);
      if (std::isnan(exp_decrease_) || std::isnan(exp_nochange_) || std::isnan(exp_increase_)) {
        std::cout << "exps compromized\n";
      }
      if (std::isnan(prob_decrease_) || std::isnan(prob_nochange_) || std::isnan(prob_increase_)) {
        std::cout << "probs compromized\t" << exp_decrease_ << "\t" << exp_nochange_ << "\t" << exp_increase_ << "\n";
      }
      //	  std::cout<<exp_increase_<<"\t"<<exp_nochange_<<"\t"<<exp_increase_<<"\n";
      //	  std::cout<<prob_decrease_<<"\t"<<prob_nochange_<<"\t"<<prob_increase_<<"\n";

      PropagateNewTargetPrice(prob_decrease_, prob_nochange_, prob_increase_);

      // if this is based on predicting returns and not change
      // then periodically update the multipliers with a multiplying factor
      /*
               if ( is_returns_based_ &&
               ( fabs ( dep_market_view_.price_from_type ( dep_baseprice_type_ ) - last_updated_baseprice_ ) > 5 *
         dep_market_view_.min_price_increment ( ) ) )
               {
               double _this_mult_factor_ = dep_market_view_.price_from_type ( dep_baseprice_type_ ) /
         last_updated_baseprice_ ;
               MultiplyIndicatorNodeValuesBy ( _this_mult_factor_ ) ;
               last_updated_baseprice_ = dep_market_view_.price_from_type ( dep_baseprice_type_ ) ;
               }
               */
    } else {
      if (last_is_ready_) {
        PropagateNotReady();
        last_is_ready_ = false;
      }
    }
  }
};
}
#endif  // BASE_MODELMATH_LOGISTIC_MODEL_AGGREGATOR_H
