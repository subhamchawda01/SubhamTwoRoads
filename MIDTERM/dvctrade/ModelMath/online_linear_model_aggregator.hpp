/*
 * online_linear_model_aggregator.hpp
 *
 *  Created on: 16-Sep-2015
 *      Author: raghuram
 */

#ifndef MODELMATHCODE_ONLINE_LINEAR_MODEL_AGGREGATOR_HPP_
#define MODELMATHCODE_ONLINE_LINEAR_MODEL_AGGREGATOR_HPP_

#include <assert.h>
#include <string>

#include "dvctrade/ModelMath/base_model_math.hpp"
#include "dvctrade/ModelMath/online_lr_weights.hpp"

/*
this aggregator is very similar to linear aggregator.
this class has kalman_filter/Online_LR_Weights instance as member variable which dumps price and indicators into a file
and updates the weights based on the recent data periodically
*/

namespace HFSAT {
class OnlineLinearModelAggregator : public BaseModelMath {
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

  std::vector<double> current_indicator_vals;

 public:
  int num_indicators_ = 0;
  double observation_var_ = 0;
  std::vector<double> indicator_var_vec_;
  std::vector<double> current_weights_;
  std::vector<double> current_weights_var_;
  Kalman::Online_LR_Weights kalman_filter_;

  int pred_dur_ = 0;
  int weights_update_dur_ = 0;
  int num_samples_per_update_ = 0;
  double thres_factor_ = 0;
  double last_sum_vars_ = 0;
  OnlineLinearModelAggregator(DebugLogger& _dbglogger_, const Watch& _watch_, const std::string& _model_filename_,
                              SecurityMarketView& _dep_market_view_, PriceType_t _dep_baseprice_type_,
                              bool _is_returns_based_, std::string model_type_)
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
        kalman_filter_() {
    prev_value_vec_.clear();
    if (!dep_market_view_.subscribe_price_type(
            NULL, dep_baseprice_type_))  // To make sure dep_baseprice_type_ price is computed.
    {
      PriceType_t t_error_price_type_ = dep_baseprice_type_;
      std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << " passed "
                << t_error_price_type_ << std::endl;
    }
    SetPropagateThresholds(dep_market_view_.shortcode());

    GetHugeValueThreshold(dep_market_view_.shortcode());
    if (model_type_.compare("LINEAR") == 0) {
      parse_linear_model_file();
    }
    if (model_type_.compare("SIGLR") == 0) {
      parse_siglr_model_file();
    }
  }

  //  void add_kalman_filter(Kalman::Online_LR_Weights _kf_) { kalman_filter_ = _kf_; }
  void initialize_kalman_filter() {
    kalman_filter_ = Kalman::Online_LR_Weights(num_indicators_, current_weights_, current_weights_var_,
                                               indicator_var_vec_, observation_var_, pred_dur_, weights_update_dur_,
                                               num_samples_per_update_, watch_, thres_factor_, model_filename_);
    watch_.subscribe_BigTimePeriod(&kalman_filter_);
  }

  void add_weights(double _init_weight_, double _init_weight_var_, double _indicator_var_, int _index_,
                   double _thres_factor_ = 0.0, int _num_samples_per_update_ = 1) {
    if (_index_ == 0) {
      observation_var_ = _indicator_var_;
      pred_dur_ = _init_weight_;
      weights_update_dur_ = _init_weight_var_;
      num_samples_per_update_ = _num_samples_per_update_;
      thres_factor_ = _thres_factor_;

    } else {
      indicator_var_vec_.push_back(_indicator_var_);
      current_weights_.push_back(_init_weight_);
      current_weights_var_.push_back(_init_weight_var_);
      num_indicators_++;
    }
  }

  virtual ~OnlineLinearModelAggregator() {}

  void AddIntercept(const double& _this_weight_constant_) {
    sum_vars_ = _this_weight_constant_;
    model_intercept_ = _this_weight_constant_;
  }

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

      kalman_filter_.update_indicator(_new_value_, _indicator_index_);

      kalman_filter_.update_pred_val();
      last_sum_vars_ = sum_vars_;
      sum_vars_ = kalman_filter_.get_pred_val();

      if (fabs(sum_vars_ - last_sum_vars_) > huge_value_threshold_ * dep_market_view_.min_price_increment()) {
        if (!dbglogger_.IsNoLogs()) {
          std::cerr << watch_.tv() << ' ' << typeid(*this).name() << ':' << __func__ << ' '
                    << "HUGE value in sum_vars_. last updated : " << _indicator_index_ << " "
                    << indicator_vec_[_indicator_index_]->concise_indicator_description()
                    << " ptgt: " << (dep_market_view_.price_from_type(dep_baseprice_type_) + sum_vars_)
                    << " ntgt: " << (dep_market_view_.price_from_type(dep_baseprice_type_) + sum_vars_ +
                                     (_new_value_ - prev_value_vec_[_indicator_index_]))
                    << " numticks "
                    << (fabs(_new_value_ - prev_value_vec_[_indicator_index_]) / dep_market_view_.min_price_increment())
                    << std::endl;

          indicator_vec_[_indicator_index_]->PrintDebugInfo();

          DBGLOG_TIME_CLASS_FUNC << "HUGE in sum_vars_. last updated : " << _indicator_index_ << " "
                                 << indicator_vec_[_indicator_index_]->concise_indicator_description()
                                 << " ptgt: " << (dep_market_view_.price_from_type(dep_baseprice_type_) + sum_vars_)
                                 << " ntgt: " << (dep_market_view_.price_from_type(dep_baseprice_type_) + sum_vars_ +
                                                  (_new_value_ - prev_value_vec_[_indicator_index_]))
                                 << DBGLOG_ENDL_FLUSH;
        }
      }

      // There is no need to update sum_vars here. We are not using sum_vars till the SMVOnReady is called.
      // Hence we could easily compute sum_vars as sum of prev_value_vec in SMVOnReady. It might not be suboptimal at
      // all.
      // But in the interest of looking cool we are updating it here, before we update prev_value_vec_

      prev_value_vec_[_indicator_index_] = _new_value_;

      // ----- //
      // These are not related to trading.
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
             last_indicators_debug_print_ +
                 SHOW_IND_MSECS_TIMEOUT)) {  // some indicator is not ready but at least one other is ready
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
    // based on mktpx - midpx
    // we could give a weight
    // like when ( 2 * abs ( mktpx - midpx ) ) / ( spread ) > 0.45
    // that means
    CalcAndPropagate();
  }

  inline void OnGlobalPositionChange(const unsigned int _security_id_, int _new_global_position_) {
    CalcAndPropagate();
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
      prev_value_vec_.push_back(0.0);
    } else {
      if (dbglogger_.CheckLoggingLevel(DBG_MODEL_ERROR)) {
        DBGLOG_TIME_CLASS_FUNC << "NULL t_indicator_" << DBGLOG_ENDL_FLUSH;
      }
      ExitVerbose(kExitErrorCodeGeneral, dbglogger_);
    }
  }

  void AddIndicator(CommonIndicator* p_this_indicator_, const double& _this_alpha_, const double& _this_beta_,
                    bool _readiness_required_) {
    if (p_this_indicator_ != NULL) {
      { p_this_indicator_->add_indicator_listener(indicator_vec_.size(), this, _this_alpha_, _this_beta_); }
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
      for (unsigned int i = 1; i < indicator_vec_.size(); i++) {
        dbglogger_ << " weighted value in ticks: " << (prev_value_vec_[i] * kalman_filter_.current_weights_[i - 1] /
                                                       dep_market_view_.min_price_increment())
                   << " weight  " << kalman_filter_.current_weights_[i - 1] << " of "
                   << indicator_vec_[i]->concise_indicator_description() << DBGLOG_ENDL_FLUSH;
      }
      dbglogger_ << "=================================================================" << DBGLOG_ENDL_FLUSH;
    }
  }

  void ShowIndicatorValuesToScreen() {
    printf("\033[36;1H");  // move to 35 th line

    std::cout << "\n=================================================================" << std::endl;
    for (unsigned int i = 1; i < indicator_vec_.size(); i++) {
      std::cout << " weighted value in ticks: " << (prev_value_vec_[i] * kalman_filter_.current_weights_[i - 1] /
                                                    dep_market_view_.min_price_increment())
                << " weight  " << kalman_filter_.current_weights_[i - 1] << " of "
                << indicator_vec_[i]->concise_indicator_description() << "                  " << std::endl;
    }
    std::cout << "=================================================================" << std::endl;
  }

  /// debug info
  void DumpIndicatorValues() {
    //      if ( dbglogger_.CheckLoggingLevel ( DBG_MODEL_ERROR ) )
    //	{
    DBGLOG_TIME_CLASS_FUNC << "\n================================================================="
                           << DBGLOG_ENDL_FLUSH;
    for (unsigned int i = 1; i < indicator_vec_.size(); i++) {
      dbglogger_ << " weighted value in ticks: " << (prev_value_vec_[i] * kalman_filter_.current_weights_[i - 1] /
                                                     dep_market_view_.min_price_increment())
                 << " weight  " << kalman_filter_.current_weights_[i - 1] << " of "
                 << indicator_vec_[i]->concise_indicator_description() << DBGLOG_ENDL_FLUSH;
    }
    dbglogger_ << "=================================================================" << DBGLOG_ENDL_FLUSH;
    //	}
  }

 protected:
  inline bool AreAllReady() {
    // if dependent not ready, all not ready
    if (!dep_market_view_.is_ready()) {
      return false;
    }

    if (use_implied_price_) {
      if (!p_implied_price_indicator_->IsIndicatorReady()) {
        return false;
      }
    }

    if (use_mid_price_base_) {
      if (!p_diff_price_indicator_->IsIndicatorReady() ||
          (p_regime_indicator_for_dat_ && !p_regime_indicator_for_dat_->IsIndicatorReady())) {
        return false;
      }
    }
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
    for (auto i = 0u; i < indicator_vec_.size(); i++) {
      indicator_vec_[i]->MultiplyIndicatorListenerWeight(this, _mult_factor_);
    }
    sum_vars_ += (_mult_factor_ - 1) * model_intercept_;
    model_intercept_ *= _mult_factor_;
  }

  // not sure why this was written the proper function call is "DumpIndicatorValues"
  // which is a virtual call from BaseModelMath
  // void dumpIndicatorValues( )
  // {
  //   std::ostringstream t_temp_oss_;
  //   t_temp_oss_ << " Debug Info\n";
  //   for ( auto i = 0u; i < indicator_vec_.size( ); i++ )
  // 	{
  // 	  t_temp_oss_ << ( indicator_vec_[ i ] -> concise_indicator_description( ) ) << " WEIGHT " << ( indicator_vec_[
  // i ] -> GetIndicatorListenerWeight( this ) ) << "\n";
  // 	}
  //   t_temp_oss_ << "---------------------------------\n\n";
  //   std::cerr << t_temp_oss_.str( );
  // }

  inline void CalcAndPropagate() {
    if (is_ready_) {
      // compute the new target and see if it needs to be sent ahead to listeners
      // note that returns based or not has already been taken care of in th multiplier
      // so now only addition to dep_baseprice_ is needed
      double new_target_bias_ = sum_vars_;
      if (use_mid_price_base_ &&
          (!p_regime_indicator_for_dat_ || p_regime_indicator_for_dat_->indicator_value(is_ready_) == 1)) {
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

      if (use_own_base_px_ && own_base_px_ != kPriceTypeMax) {
        new_target_price_ = dep_market_view_.price_from_type(own_base_px_) + new_target_bias_;
      }

      if (target_price_reporter_ != NULL) {
        new_target_price_ = target_price_reporter_->price_from_type(dep_baseprice_type_) + new_target_bias_;
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
          DBGLOG_TIME_CLASS_FUNC << "tgt_price: " << new_target_price_ << DBGLOG_ENDL_FLUSH;
          target_price_reporter_->ShowBook();
        }
      }

      // const double kMinTicksMoved = 0.015 ;
      const double kMinTicksMoved = min_ticks_move_;

      if (fabs(new_target_price_ - last_propagated_target_price_) >
          (kMinTicksMoved * dep_market_view_.min_price_increment())) {
        PropagateNewTargetPrice(new_target_price_, new_target_bias_);
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

  void parse_linear_model_file() {
    bool start_reading = false;
    char delimiter = ' ';

    int indicator_index_ = 0;

    std::ifstream model_file;
    model_file.open(model_filename_.c_str(), std::ios::in);

    std::string l1;
    while (std::getline(model_file, l1)) {
      if (!start_reading) {
        if (l1.compare("INDICATORSTART") == 0) {
          start_reading = true;
          continue;
        } else {
          continue;
        }
      } else {
        if (l1.compare("INDICATOREND") == 0) {
          initialize_kalman_filter();
          break;
        }
        if (l1.at(0) == '#') {
          continue;
        }

        std::stringstream ss(l1);
        std::string tok;
        getline(ss, tok, delimiter);
        getline(ss, tok, delimiter);

        double _this_weight_ = 0.0;
        double _this_weight_var_ = 0.0;
        double _this_indicator_var_ = 0.0;
        double _this_thres_factor_ = 0.0;
        double _this_num_samples_per_update_ = 0.0;
        std::istringstream iss(tok);
        std::string token;
        getline(iss, token, ':');
        _this_weight_ = atof(token.c_str());
        getline(iss, token, ':');
        _this_weight_var_ = atof(token.c_str());
        getline(iss, token, ':');
        _this_indicator_var_ = atof(token.c_str());
        if (indicator_index_ == 0) {
          getline(iss, token, ':');
          _this_thres_factor_ = atof(token.c_str());
          getline(iss, token, ':');
          _this_num_samples_per_update_ = atof(token.c_str());
        }

        add_weights(_this_weight_, _this_weight_var_, _this_indicator_var_, indicator_index_, _this_thres_factor_,
                    _this_num_samples_per_update_);

        indicator_index_++;
      }
    }
  }

  void parse_siglr_model_file() {
    bool start_reading = false;
    char delimiter = ' ';

    int indicator_index_ = 0;

    std::ifstream model_file;
    model_file.open(model_filename_.c_str(), std::ios::in);

    std::string l1;
    while (std::getline(model_file, l1)) {
      if (!start_reading) {
        if (l1.compare("INDICATORSTART") == 0) {
          start_reading = true;
          continue;
        } else {
          continue;
        }
      } else {
        if (l1.compare("INDICATOREND") == 0) {
          initialize_kalman_filter();
          break;
        }
        if (l1.at(0) == '#') {
          continue;
        }

        std::stringstream ss(l1);
        std::string tok;
        getline(ss, tok, delimiter);
        getline(ss, tok, delimiter);

        double _this_weight_ = 0.0;
        double _this_weight_var_ = 0.0;
        double _this_indicator_var_ = 0.0;
        double _this_thres_factor_ = 0.0;
        double _this_num_samples_per_update_ = 0.0;
        std::istringstream iss(tok);
        std::string token;

        if (indicator_index_ != 0) {
          getline(iss, token, ':');
        }

        getline(iss, token, ':');
        _this_weight_ = atof(token.c_str());
        getline(iss, token, ':');
        _this_weight_var_ = atof(token.c_str());
        getline(iss, token, ':');
        _this_indicator_var_ = atof(token.c_str());
        if (indicator_index_ == 0) {
          getline(iss, token, ':');
          _this_thres_factor_ = atof(token.c_str());
          getline(iss, token, ':');
          _this_num_samples_per_update_ = atof(token.c_str());
        }

        add_weights(_this_weight_, _this_weight_var_, _this_indicator_var_, indicator_index_, _this_thres_factor_,
                    _this_num_samples_per_update_);

        indicator_index_++;
      }
    }
  }
};
}

#endif /* MODELMATHCODE_ONLINE_LINEAR_MODEL_AGGREGATOR_HPP_ */
