/**
    \file ModelMath/linear_mult_model_aggregator.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#ifndef BASE_MODELMATH_LINEAR_MULT_MODEL_AGGREGATOR_H
#define BASE_MODELMATH_LINEAR_MULT_MODEL_AGGREGATOR_H

#include <assert.h>

#include "dvctrade/ModelMath/base_multiple_model_math.hpp"

namespace HFSAT {
class LinearMultModelAggregator : public BaseMultipleModelMath {
 protected:
 public:
  LinearMultModelAggregator(DebugLogger& _dbglogger_, const Watch& _watch_, SecurityMarketView* _smv_,
                            const std::string& _param_filename_, PriceType_t _dep_baseprice_type_)
      : BaseMultipleModelMath(_dbglogger_, _watch_, _smv_, _param_filename_, _dep_baseprice_type_) {}

  virtual ~LinearMultModelAggregator() {}

  inline void ForceIndicatorReadyGlobal(const unsigned int t_indicator_index_) {
    if (t_indicator_index_ < is_ready_vec_.size() && t_indicator_index_ < global_indicator_vec_.size() &&
        !is_ready_vec_[t_indicator_index_] && !global_indicator_vec_[t_indicator_index_]->IsIndicatorReady()) {
      if (!dbglogger_.IsNoLogs()) {
        DBGLOG_TIME_CLASS_FUNC << "Global Indicator Not Ready [ " << t_indicator_index_ << " ] "
                               << global_indicator_vec_[t_indicator_index_]->concise_indicator_description()
                               << DBGLOG_ENDL_FLUSH;
        DBGLOG_DUMP;

        global_indicator_vec_[t_indicator_index_]->WhyNotReady();
      }

      is_ready_vec_[t_indicator_index_] = true;

      if (!dbglogger_.IsNoLogs()) {
        DBGLOG_TIME_CLASS_FUNC << "Global Indicator Forced Ready [ " << t_indicator_index_ << " ] "
                               << global_indicator_vec_[t_indicator_index_]->concise_indicator_description()
                               << DBGLOG_ENDL_FLUSH;
        DBGLOG_DUMP;
      }

      is_ready_ = AreAllReady(-1);

      if (is_ready_) {  // Check for is_ready_ again , maybe this indicator was the last one.
        if (!dbglogger_.IsNoLogs()) {
          DBGLOG_TIME_CLASS_FUNC << "All Global Indicators Ready " << DBGLOG_ENDL_FLUSH;
          DBGLOG_DUMP;
        }
        last_is_ready_ = true;
      }
    }
  }

  inline void ForceIndicatorReady(const unsigned int t_indicator_index_, int _product_index_) {
    if (t_indicator_index_ < is_ready_vec_product_[_product_index_].size() &&
        t_indicator_index_ < individual_indicator_vec_[_product_index_].size() &&
        !is_ready_vec_product_[_product_index_][t_indicator_index_] &&
        !individual_indicator_vec_[_product_index_][t_indicator_index_]->IsIndicatorReady()) {
      if (!dbglogger_.IsNoLogs()) {
        DBGLOG_TIME_CLASS_FUNC
            << "Individual Indicator Not Ready [ " << t_indicator_index_ << " ] "
            << individual_indicator_vec_[_product_index_][t_indicator_index_]->concise_indicator_description()
            << DBGLOG_ENDL_FLUSH;
        DBGLOG_DUMP;

        individual_indicator_vec_[_product_index_][t_indicator_index_]->WhyNotReady();
      }

      is_ready_vec_product_[_product_index_][t_indicator_index_] = true;

      if (!dbglogger_.IsNoLogs()) {
        DBGLOG_TIME_CLASS_FUNC
            << "Individual Indicator Forced Ready [ " << t_indicator_index_ << " ] "
            << individual_indicator_vec_[_product_index_][t_indicator_index_]->concise_indicator_description()
            << DBGLOG_ENDL_FLUSH;
        DBGLOG_DUMP;
      }

      is_ready_product_[_product_index_] = AreAllReady(_product_index_);

      if (is_ready_product_[_product_index_]) {  // Check for is_ready_ again , maybe this indicator was the last one.
        if (!dbglogger_.IsNoLogs()) {
          DBGLOG_TIME_CLASS_FUNC << "All Indicators Ready For product" << const_smv_list_[_product_index_]->shortcode()
                                 << DBGLOG_ENDL_FLUSH;
          DBGLOG_DUMP;
        }
        last_is_ready_product_[_product_index_] = true;
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
        for (auto i = 0u; i < num_shortcodes_; i++) ShowIndicatorValues(i);
      } break;
      default: { } break; }
  }

  inline void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_) {
    unsigned int product_index_ = _indicator_index_ / num_individual_indicators_;
    int indicator_index_ = 0;
    if (product_index_ >= num_shortcodes_) {
      indicator_index_ = _indicator_index_ - num_individual_indicators_ * num_shortcodes_;
      OnGlobalIndicatorUpdate(indicator_index_, _new_value_);
      return;
    } else {
      indicator_index_ = _indicator_index_ % num_individual_indicators_;
    }

    if (!is_ready_product_[product_index_]) {
      is_ready_vec_product_[product_index_][indicator_index_] = true;
      is_ready_product_[product_index_] = AreAllReady(product_index_);

      if ((!is_ready_product_[product_index_]) &&
          (VectorUtils::CheckAllForValue(is_ready_vec_product_[product_index_], false) == false) &&
          ((last_individual_indicators_debug_print_[product_index_] == 0) ||
           (watch_.msecs_from_midnight() > last_individual_indicators_debug_print_[product_index_] +
                                               2000))) {  // some indicator is not ready but at least one other is ready

        last_individual_indicators_debug_print_[product_index_] = watch_.msecs_from_midnight();
        std::ostringstream alert_indicators_not_ready_stream;

        for (auto i = 0u; i < individual_indicator_vec_[product_index_].size(); i++) {
          if (!is_ready_vec_product_[product_index_][i]) {  // if this indicator isn't ready
            // print this and
            // ask if it was ready, but just could not notify us ... basically did not get any update yet.
            if (individual_indicator_vec_[product_index_][i]->IsIndicatorReady()) {  // if ti was secretly ready
              is_ready_vec_product_[product_index_][i] =
                  individual_indicator_vec_[product_index_][i]->IsIndicatorReady();
              is_ready_product_[product_index_] = AreAllReady(product_index_);
            } else {
              if (!dbglogger_.IsNoLogs()) {
                DBGLOG_TIME_CLASS_FUNC << "Individual Indicator Not Ready [ " << i << " ] "
                                       << individual_indicator_vec_[product_index_][i]->concise_indicator_description()
                                       << DBGLOG_ENDL_FLUSH;
                DBGLOG_DUMP;

                // Add To Stream
                alert_indicators_not_ready_stream
                    << "Individual Indicator Not Ready [ " << i << " ] "
                    << individual_indicator_vec_[product_index_][i]->concise_indicator_description() << " <br/>";

                individual_indicator_vec_[product_index_][i]->WhyNotReady();
              }
            }
          }
        }

        for (auto i = 0u; i < global_indicator_vec_.size(); i++) {
          if (!is_ready_vec_[i]) {  // if this indicator isn't ready
            // print this and
            // ask if it was ready, but just could not notify us ... basically did not get any update yet.
            if (global_indicator_vec_[i]->IsIndicatorReady()) {  // if ti was secretly ready
              is_ready_vec_[i] = global_indicator_vec_[i]->IsIndicatorReady();
              is_ready_ = AreAllReady(product_index_);
            } else {
              if (!dbglogger_.IsNoLogs()) {
                DBGLOG_TIME_CLASS_FUNC << "Global Indicator Not Ready [ " << i << " ] "
                                       << global_indicator_vec_[i]->concise_indicator_description()
                                       << DBGLOG_ENDL_FLUSH;
                DBGLOG_DUMP;

                // Add To Stream
                alert_indicators_not_ready_stream << "Global Indicator Not Ready [ " << i << " ] "
                                                  << global_indicator_vec_[i]->concise_indicator_description()
                                                  << " <br/>";

                global_indicator_vec_[i]->WhyNotReady();
              }
            }
          }
        }

        // Call Base Class To Notify That Indicators Are Not Ready And It Will Decide
        if (!is_ready_product_[product_index_]) {
          AlertIndicatorsNotReady(alert_indicators_not_ready_stream.str());
        }
      }

      if (is_ready_product_[product_index_]) {
        if (!dbglogger_.IsNoLogs()) {
          DBGLOG_TIME_CLASS_FUNC << "All Indicators Ready for " << shortcodes_list_[product_index_]
                                 << DBGLOG_ENDL_FLUSH;
          DBGLOG_DUMP;
        }
        last_is_ready_product_[product_index_] = true;
      }
    }

    if (std::isnan(_new_value_)) {
      if (!dbglogger_.IsNoLogs()) {
        std::cerr << watch_.tv() << ' ' << typeid(*this).name() << ':' << __func__ << ' '
                  << "nan in sum_vars_. last updated : " << indicator_index_ << " "
                  << individual_indicator_vec_[product_index_][indicator_index_]->concise_indicator_description()
                  << std::endl;

        DBGLOG_TIME_CLASS_FUNC
            << "nan in sum_vars_. last updated : " << indicator_index_ << " "
            << individual_indicator_vec_[product_index_][indicator_index_]->concise_indicator_description()
            << DBGLOG_ENDL_FLUSH;
      }
      is_ready_product_[product_index_] = false;
      return;
    }

    // TODO : Add huge value thresholds here

    value_vec_[product_index_] +=
        (_new_value_ - prev_individual_indicator_value_vec_[product_index_][indicator_index_]);
    prev_individual_indicator_value_vec_[product_index_][indicator_index_] = _new_value_;
    // index_to_propagate_ = product_index_;

    // ----- //
    // These are not related to trading.
    if (dbglogger_.CheckLoggingLevel(TRADEINIT_INFO)) {
      if ((last_individual_indicators_debug_print_[product_index_] == 0) ||
          (watch_.msecs_from_midnight() > last_individual_indicators_debug_print_[product_index_] + 2000)) {
        last_individual_indicators_debug_print_[product_index_] = watch_.msecs_from_midnight();
        ShowIndicatorValuesToScreen(product_index_);
      }
    }

    /*    if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
      if ((last_individual_indicators_debug_print_[product_index_] == 0) ||
          (watch_.msecs_from_midnight() >
           last_individual_indicators_debug_print_[product_index_] +
               SHOW_IND_MSECS_TIMEOUT)) {  // some indicator is not ready but at least one other is ready
        last_individual_indicators_debug_print_[product_index_] = watch_.msecs_from_midnight();
        ShowIndicatorValues(product_index_);
      }
      } */
  }

  inline void OnGlobalIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_) {
    if (!is_ready_) {
      is_ready_vec_[_indicator_index_] = true;
      is_ready_ = AreAllReady(-1);

      if ((!is_ready_) && (VectorUtils::CheckAllForValue(is_ready_vec_, false) == false) &&
          ((last_common_indicators_debug_print_ == 0) ||
           (watch_.msecs_from_midnight() > last_common_indicators_debug_print_ +
                                               2000))) {  // some indicator is not ready but at least one other is ready

        last_common_indicators_debug_print_ = watch_.msecs_from_midnight();
        std::ostringstream alert_indicators_not_ready_stream;

        for (auto i = 0u; i < global_indicator_vec_.size(); i++) {
          if (!is_ready_vec_[i]) {  // if this indicator isn't ready
            // print this and
            // ask if it was ready, but just could not notify us ... basically did not get any update yet.
            if (global_indicator_vec_[i]->IsIndicatorReady()) {  // if ti was secretly ready
              is_ready_vec_[i] = global_indicator_vec_[i]->IsIndicatorReady();
              is_ready_ = AreAllReady(-1);
            } else {
              if (!dbglogger_.IsNoLogs()) {
                DBGLOG_TIME_CLASS_FUNC << "Global Indicator Not Ready [ " << i << " ] "
                                       << global_indicator_vec_[i]->concise_indicator_description()
                                       << DBGLOG_ENDL_FLUSH;
                DBGLOG_DUMP;

                // Add To Stream
                alert_indicators_not_ready_stream << "Global Indicator Not Ready [ " << i << " ] "
                                                  << global_indicator_vec_[i]->concise_indicator_description()
                                                  << " <br/>";

                global_indicator_vec_[i]->WhyNotReady();
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
          DBGLOG_TIME_CLASS_FUNC << "All Global Indicators Ready for " << underlying_shc_ << DBGLOG_ENDL_FLUSH;
          DBGLOG_DUMP;
        }
        last_is_ready_ = true;
      }
    }

    if (std::isnan(_new_value_)) {
      if (!dbglogger_.IsNoLogs()) {
        std::cerr << watch_.tv() << ' ' << typeid(*this).name() << ':' << __func__ << ' '
                  << "nan in sum_vars_. last updated : " << _indicator_index_ << " "
                  << global_indicator_vec_[_indicator_index_]->concise_indicator_description() << std::endl;

        DBGLOG_TIME_CLASS_FUNC << "nan in sum_vars_. last updated : " << _indicator_index_ << " "
                               << global_indicator_vec_[_indicator_index_]->concise_indicator_description()
                               << DBGLOG_ENDL_FLUSH;
      }
      is_ready_ = false;
      return;
    }

    // There is no need to update sum_vars here. We are not using sum_vars till the SMVOnReady is called.
    // Hence we could easily compute sum_vars as sum of prev_value_vec in SMVOnReady. It might not be suboptimal at
    // all.
    // But in the interest of looking cool we are updating it here, before we update prev_value_vec_
    common_sum_vars_ += (_new_value_ - prev_global_indicator_value_vec_[_indicator_index_]);
    prev_global_indicator_value_vec_[_indicator_index_] = _new_value_;

    // ----- //
    // These are not related to trading.
    if (dbglogger_.CheckLoggingLevel(TRADEINIT_INFO)) {
      if ((last_common_indicators_debug_print_ == 0) ||
          (watch_.msecs_from_midnight() > last_common_indicators_debug_print_ + 2000)) {
        last_common_indicators_debug_print_ = watch_.msecs_from_midnight();
        ShowIndicatorValuesToScreen(-1);
      }
    }

    /*    if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
      if ((last_common_indicators_debug_print_ == 0) ||
          (watch_.msecs_from_midnight() >
           last_common_indicators_debug_print_ + SHOW_IND_MSECS_TIMEOUT)) {  // some indicator is not ready but at least
      one other is ready
        last_common_indicators_debug_print_ = watch_.msecs_from_midnight();
        ShowIndicatorValues(-1);
      }
      } */
  }

  // ModelMath objects are notified after all the indicators are updated.
  // Indicators get updates from
  // (i) Portfolio which listens to SMV updates
  // (ii) SMV updates
  // (iii) GlobalPositionChange
  inline void SMVOnReady() {
    if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
      DBGLOG_TIME_CLASS_FUNC_LINE << "OnTimePeriodUpdate" << DBGLOG_ENDL_FLUSH;
    }
    CalcAndPropagate();
  }

  inline void OnTimePeriodUpdate(const int num_pages_to_add_) {
    if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
      DBGLOG_TIME_CLASS_FUNC_LINE << "OnTimePeriodUpdate" << DBGLOG_ENDL_FLUSH;
    }
    CalcAndPropagate();
  }

  inline void OnGlobalPositionChange(const unsigned int _security_id_, int _new_global_position_) {
    // CalcAndPropagate();
  }

  void set_basepx_pxtype() {
    for (auto i = 0u; i < num_shortcodes_; i++) {
      for (unsigned int j = 0; j < individual_indicator_vec_[i].size(); j++) {
        individual_indicator_vec_[i][j]->set_start_mfm(t_trading_start_utc_mfm_);
        individual_indicator_vec_[i][j]->set_end_mfm(t_trading_end_utc_mfm_);
      }
    }
    for (auto i = 0u; i < global_indicator_vec_.size(); i++) {
      global_indicator_vec_[i]->set_basepx_pxtype((SecurityMarketView&)*underlying_smv_, dep_baseprice_type_);
    }
  }

  void ShowIndicatorValues(int _product_index_) {
    if (dbglogger_.CheckLoggingLevel(DBG_MODEL_ERROR)) {
      DBGLOG_TIME_CLASS_FUNC << "\n================================================================="
                             << DBGLOG_ENDL_FLUSH;
      dbglogger_ << " sum_vars: " << value_vec_[_product_index_] + common_sum_vars_ << DBGLOG_ENDL_FLUSH;
      if (_product_index_ > 0) {
        for (auto i = 0u; i < individual_indicator_vec_[_product_index_].size(); i++) {
          dbglogger_ << "Individual value: " << (prev_individual_indicator_value_vec_[_product_index_][i]) << " of "
                     << individual_indicator_vec_[_product_index_][i]->concise_indicator_description()
                     << DBGLOG_ENDL_FLUSH;
        }
      }
      for (auto i = 0u; i < global_indicator_vec_.size(); i++) {
        dbglogger_ << "Global value: " << (prev_global_indicator_value_vec_[i]) << " of "
                   << global_indicator_vec_[i]->concise_indicator_description() << DBGLOG_ENDL_FLUSH;
      }
      dbglogger_ << "=================================================================" << DBGLOG_ENDL_FLUSH;
    }
  }

  void ShowIndicatorValuesToScreen(int _product_index_) {
    printf("\033[36;1H");  // move to 35 th line

    std::cout << "\n=================================================================" << std::endl;
    if (_product_index_ > 0) {
      for (auto i = 0u; i < individual_indicator_vec_[_product_index_].size(); i++) {
        dbglogger_ << "Individual value: " << (prev_individual_indicator_value_vec_[_product_index_][i]) << " of "
                   << individual_indicator_vec_[_product_index_][i]->concise_indicator_description()
                   << DBGLOG_ENDL_FLUSH;
      }
    }
    for (auto i = 0u; i < global_indicator_vec_.size(); i++) {
      dbglogger_ << "Global value: " << (prev_global_indicator_value_vec_[i]) << " of "
                 << global_indicator_vec_[i]->concise_indicator_description() << DBGLOG_ENDL_FLUSH;
    }
    std::cout << "=================================================================" << std::endl;
  }
  /// debug info
  void DumpIndicatorValues() {
    // Only dunping global indicators here

    DBGLOG_TIME_CLASS_FUNC << "\n================================================================="
                           << DBGLOG_ENDL_FLUSH;
    for (auto i = 0u; i < global_indicator_vec_.size(); i++) {
      dbglogger_ << "Global value: " << (prev_global_indicator_value_vec_[i]) << " of "
                 << global_indicator_vec_[i]->concise_indicator_description() << DBGLOG_ENDL_FLUSH;
    }
    dbglogger_ << "=================================================================" << DBGLOG_ENDL_FLUSH;
    //	}
  }

  void FinishCreation() {
    if (num_individual_indicators_ == 0) {
      for (auto i = 0u; i < const_smv_list_.size(); i++) {
        is_ready_product_[i] = true;
      }
    }
    if (global_indicator_vec_.size() == 0) {
      is_ready_ = true;
    }
  }

 protected:
  inline bool AreAllReady(int _product_index_) {
    if (!underlying_smv_->is_ready()) {
      return false;
    }

    // Checking for global indicators
    for (auto i = 0u; i < is_ready_vec_.size(); ++i) {
      if (is_ready_vec_[i] == false) {
        return false;
      }
    }

    if (_product_index_ >= 0) {
      if (!const_smv_list_[_product_index_]->is_ready()) {
        return false;
      }

      // Checking for individual indicators
      for (auto i = 0u; i < is_ready_vec_product_[_product_index_].size(); ++i) {
        if (is_ready_vec_product_[_product_index_][i] == false) {
          return false;
        }
      }
    }

    return true;
  }

  inline void CalcAndPropagate() {
    // Check for presence of global sum_vars_
    double new_common_target_bias_;
    if (is_ready_) {
      new_common_target_bias_ = common_sum_vars_;
    } else {
      if (last_is_ready_) {
        PropagateNotReady(-1);
        last_is_ready_ = false;
      }
      return;
    }

    for (unsigned int _product_index_ = 0; _product_index_ < num_shortcodes_; _product_index_++) {
      if (is_ready_product_[_product_index_]) {
        double new_target_bias_ = value_vec_[_product_index_] + new_common_target_bias_;
        const double kMinMoved = 0.00;  // Can be added as a parameter (Default : 0.01%)

        if (fabs(new_target_bias_ - last_propagated_value_vec_[_product_index_]) > (kMinMoved)) {
          if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
            DBGLOG_TIME_CLASS_FUNC_LINE << " IV: " << new_target_bias_ << " Prod: " << _product_index_
                                        << DBGLOG_ENDL_FLUSH;
          }
          PropagateNewTargetPrice(new_target_bias_, new_target_bias_, _product_index_);
          last_propagated_value_vec_[_product_index_] = new_target_bias_;
        }
      } else {
        if (last_is_ready_product_[_product_index_]) {
          PropagateNotReady(_product_index_);
          last_is_ready_product_[_product_index_] = false;
        }
      }
    }
  }
};
}
#endif  // BASE_MODELMATH_LINEAR_MULT_MODEL_AGGREGATOR_H
