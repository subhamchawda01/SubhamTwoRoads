/**
        \file ModelMath/nodependant_linear_model_aggregator.hpp

        \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
Address:
Suite No 353, Evoma, #14, Bhattarhalli,
Old Madras Road, Near Garden City College,
KR Puram, Bangalore 560049, India
+91 80 4190 3551
*/
#ifndef BASE_MODELMATH_NODEPENDANT_LINEAR_MODEL_AGGREGATOR_H
#define BASE_MODELMATH_NODEPENDANT_LINEAR_MODEL_AGGREGATOR_H

#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvctrade/ModelMath/base_model_math.hpp"

namespace HFSAT {
class NoDependantLinearModelAggregator : public BaseModelMath {
 protected:
  std::vector<double> prev_value_vec_;
  double sum_vars_;
  double last_propagated_target_price_;

  int last_indicators_debug_print_;

 public:
  NoDependantLinearModelAggregator(DebugLogger& _dbglogger_, const Watch& _watch_, const std::string& _model_filename_)
      : BaseModelMath(_dbglogger_, _watch_, _model_filename_),
        prev_value_vec_(),
        sum_vars_(0),
        last_propagated_target_price_(0),
        last_indicators_debug_print_(0) {
    prev_value_vec_.clear();
  }

  virtual ~NoDependantLinearModelAggregator() {}

  inline void OnControlUpdate(const ControlMessage& _control_message_, const char* symbol_, const int trader_id) {
    switch (_control_message_.message_code_) {
      case kControlMessageCodeShowIndicators: {
        ShowIndicatorValues();
      } break;
      default: { } break; }
  }

  inline void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& new_value_decrease_,
                                const double& new_value_nochange_, const double& new_value_increase_) {
    return;
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
            // ask if it was ready, nbut just could not notify us ... basically did not get any update yet.
            if (indicator_vec_[i]->IsIndicatorReady()) {  // if ti was secretly ready
              is_ready_vec_[i] = indicator_vec_[i]->IsIndicatorReady();
              is_ready_ = AreAllReady();
            } else {
              DBGLOG_TIME_CLASS_FUNC << "Indicator Not Ready " << indicator_vec_[i]->concise_indicator_description()
                                     << DBGLOG_ENDL_FLUSH;
              // Add To Stream
              alert_indicators_not_ready_stream << "Indicator Not Ready [ " << i << " ] "
                                                << indicator_vec_[i]->concise_indicator_description() << " <br/>";
            }
          }
        }

        // Call Base Class To Notify That Indicators Are Not Ready And It Will Decide
        if (!is_ready_) {
          AlertIndicatorsNotReady(alert_indicators_not_ready_stream.str());
        }
      }

      if (is_ready_) {
        DBGLOG_TIME_CLASS_FUNC << "All Indicators Ready " << DBGLOG_ENDL_FLUSH;
      }
    } else {
      sum_vars_ += (_new_value_ - prev_value_vec_[_indicator_index_]);

      if (std::isnan(sum_vars_)) {
        std::cerr << watch_.tv() << ' ' << typeid(*this).name() << ':' << __func__ << ' '
                  << "nan in sum_vars_. last updated : " << _indicator_index_ << " "
                  << indicator_vec_[_indicator_index_]->concise_indicator_description() << std::endl;

        DBGLOG_TIME_CLASS_FUNC << "nan in sum_vars_. last updated : " << _indicator_index_ << " "
                               << indicator_vec_[_indicator_index_]->concise_indicator_description()
                               << DBGLOG_ENDL_FLUSH;
        exit(0);
      }

      prev_value_vec_[_indicator_index_] = _new_value_;
    }
  }

  inline void SMVOnReady() { CalcAndPropagate(); }
  inline void OnGlobalPositionChange(const unsigned int _security_id_, int _new_global_position_) {
    CalcAndPropagate();
  }

  void AddIndicator(CommonIndicator* p_this_indicator_, const double& _this_weight_, bool _readiness_required_) {
    if (p_this_indicator_ != NULL) {
      p_this_indicator_->add_indicator_listener(indicator_vec_.size(), this, _this_weight_);
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
      prev_value_vec_.push_back(0.0);
    } else {
      if (dbglogger_.CheckLoggingLevel(DBG_MODEL_ERROR)) {
        DBGLOG_TIME_CLASS_FUNC << "NULL t_indicator_" << DBGLOG_ENDL_FLUSH;
      }
      ExitVerbose(kExitErrorCodeGeneral, dbglogger_);
    }
  }

  void set_basepx_pxtype() {  // nothing to do here since no dep
  }

  void FinishCreation() {}

  void InterpretModelParameters(const std::vector<const char*> _tokens_) {
    // MODELARGS ...
  }

  void ShowIndicatorValues() {
    if (dbglogger_.CheckLoggingLevel(DBG_MODEL_ERROR)) {
      DBGLOG_TIME_CLASS_FUNC << "\n================================================================="
                             << DBGLOG_ENDL_FLUSH;
      dbglogger_ << " sum_vars: " << sum_vars_ << DBGLOG_ENDL_FLUSH;
      for (auto i = 0u; i < indicator_vec_.size(); i++) {
        dbglogger_ << " value: " << prev_value_vec_[i] << " of " << indicator_vec_[i]->concise_indicator_description()
                   << DBGLOG_ENDL_FLUSH;
      }
      dbglogger_ << "=================================================================" << DBGLOG_ENDL_FLUSH;
    }
  }

  /// debug info
  void DumpIndicatorValues() {
    if (dbglogger_.CheckLoggingLevel(DBG_MODEL_ERROR)) {
      DBGLOG_TIME_CLASS_FUNC << "\n================================================================="
                             << DBGLOG_ENDL_FLUSH;
      for (auto i = 0u; i < indicator_vec_.size(); i++) {
        dbglogger_ << " value: " << prev_value_vec_[i] << " of " << indicator_vec_[i]->concise_indicator_description()
                   << DBGLOG_ENDL_FLUSH;
      }
      dbglogger_ << "=================================================================" << DBGLOG_ENDL_FLUSH;
    }
  }

 protected:
  inline bool AreAllReady() { return VectorUtils::CheckAllForValue(is_ready_vec_, true); }

  inline void CalcAndPropagate() {
    if (is_ready_) {
      // compute the new target and see if it needs to be sent ahead to listeners
      // note that returns based or not has already been taken care of in th multiplier
      // so now only addition to dep_baseprice_ is needed
      double new_target_price_ = sum_vars_;
      const double kMinTargetMoved = 0.000001;
      if (fabs(new_target_price_ - last_propagated_target_price_) > kMinTargetMoved) {
        PropagateNewTargetPrice(new_target_price_, sum_vars_);
        last_propagated_target_price_ = new_target_price_;
      }
    }
  }
};
}
#endif  // BASE_MODELMATH_NODEPENDANT_LINEAR_MODEL_AGGREGATOR_H
