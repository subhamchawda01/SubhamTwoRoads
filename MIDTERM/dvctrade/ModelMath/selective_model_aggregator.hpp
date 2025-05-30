// =====================================================================================
//
//       Filename:  selective_model_aggregator.hpp
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
#ifndef BASE_MODELMATH_SELECTIVE_MODEL_AGGREGATOR_H
#define BASE_MODELMATH_SELECTIVE_MODEL_AGGREGATOR_H

#define HISTORY_TO_LOOK 60000  // one minute
#define DECAY_LENGTH 100       // time period update
#define PRICE_MOVE_FACTOR \
  0.05  // in the model creation, for 10 minutes it was 0.1, should be a parameter, take from model??

#define STATS_DIR "/spare/local/tradeinfo/volatilitylogs/"

#include "dvctrade/ModelMath/base_model_math.hpp"

namespace HFSAT {
class SelectiveModelAggregator : public BaseModelMath,
                                 public SecurityMarketViewChangeListener,
                                 public TimePeriodListener {
 protected:
  SecurityMarketView& dep_market_view_;
  std::vector<std::string>& core_shortcodes_;
  std::vector<double> prev_value_vec_;  // prev value of individual indicators
  std::vector<double> sum_vars_vec_;
  std::vector<double> last_propagated_target_price_vec_;
  PriceType_t dep_baseprice_type_;
  int last_indicators_debug_print_;
  bool initial_update_baseprice_;
  bool is_returns_based_;
  double last_updated_baseprice_;
  double last_propagated_target_price_;
  double model_intercept_;
  double sum_vars_;
  unsigned num_models_;
  std::vector<unsigned int> model_start_index_;
  std::vector<unsigned int> model_end_index_;
  int dependent_avg_moving_volume_;
  std::vector<int> sid_to_last_size_traded_;
  std::vector<int> sid_to_average_moving_volume_;
  std::vector<bool> sid_to_first_time_;
  std::vector<int> sid_to_average_statistical_volume_;  // integer ok?
  std::vector<bool> sid_to_value_required_;
  int last_updated_msecs_;
  double alpha_;
  bool dep_high_volume_period_;
  bool core_high_volume_period_;
  bool dep_trend_;
  double max_historical_price_move_in_day_;
  double max_dep_price_;
  double min_dep_price_;
  double max_dep_price_in_interval_;
  double min_dep_price_in_interval_;
  double last_price_;

 public:
  SelectiveModelAggregator(DebugLogger& _dbglogger_, const Watch& _watch_, const std::string& _model_file_name_,
                           SecurityMarketView& _dep_market_view_, PriceType_t _dep_baseprice_type_,
                           bool _is_returns_based_, std::vector<std::string>& _core_shortcodes_)
      : BaseModelMath(_dbglogger_, _watch_, _model_file_name_),
        dep_market_view_(_dep_market_view_),
        core_shortcodes_(_core_shortcodes_),
        prev_value_vec_(),
        sum_vars_vec_(),
        last_propagated_target_price_vec_(),
        dep_baseprice_type_(_dep_baseprice_type_),
        last_indicators_debug_print_(0),
        initial_update_baseprice_(false),
        is_returns_based_(_is_returns_based_),
        last_updated_baseprice_(0),
        last_propagated_target_price_(0),
        model_intercept_(0),
        sum_vars_(0),
        num_models_(0),
        dependent_avg_moving_volume_(0),
        sid_to_last_size_traded_(_dep_market_view_.sec_name_indexer_.NumSecurityId(), 0),
        sid_to_average_moving_volume_(_dep_market_view_.sec_name_indexer_.NumSecurityId(), 0),
        sid_to_first_time_(_dep_market_view_.sec_name_indexer_.NumSecurityId(), true),
        sid_to_average_statistical_volume_(_dep_market_view_.sec_name_indexer_.NumSecurityId(), 0),
        sid_to_value_required_(_dep_market_view_.sec_name_indexer_.NumSecurityId(), false),
        last_updated_msecs_(0),
        alpha_(0.0),
        dep_high_volume_period_(false),
        core_high_volume_period_(false),
        dep_trend_(false),
        max_dep_price_(0.0),
        min_dep_price_(0.0),
        max_dep_price_in_interval_(0.0),
        min_dep_price_in_interval_(0.0) {
    sum_vars_vec_.clear();
    prev_value_vec_.clear();

    alpha_ = double(1) / double(DECAY_LENGTH);
    // or
    alpha_ = double(DECAY_LENGTH) / double(HISTORY_TO_LOOK);

    if (!dep_market_view_.subscribe_price_type(
            NULL, dep_baseprice_type_))  // To make sure dep_baseprice_type_ price is computed.
    {
      PriceType_t t_error_price_type_ = dep_baseprice_type_;
      std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << " passed "
                << t_error_price_type_ << std::endl;
    }

    if (!dep_market_view_.subscribe_price_type(NULL, kPriceTypeMidprice)) {
      std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << " passed " << kPriceTypeMidprice
                << std::endl;
    }

    _dep_market_view_.subscribe_L1_Only(this);
    _watch_.subscribe_TimePeriod(this);

    for (unsigned index_ = 0; index_ < core_shortcodes_.size(); index_++) {
      SecurityMarketView* indep_smv_ =
          ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(core_shortcodes_[index_]);
      if (indep_smv_ != NULL) {
        indep_smv_->subscribe_L1_Only(this);
        sid_to_value_required_[indep_smv_->market_update_info_.security_id_] = true;
        DBGLOG_TIME_CLASS_FUNC << " Core Shortcode Name: " << core_shortcodes_[index_] << " index: " << index_
                               << DBGLOG_ENDL_FLUSH;
      }
    }

    LoadHistoricalValues();
    GetHugeValueThreshold(dep_market_view_.shortcode());
    SetPropagateThresholds(dep_market_view_.shortcode());
  }

  virtual ~SelectiveModelAggregator() {}

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

      if (fabs(_new_value_ - prev_value_vec_[_indicator_index_]) >
          huge_value_threshold_ * dep_market_view_.min_price_increment()) {
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

      for (unsigned j = 0; j < model_start_index_.size(); j++) {
        if ((_indicator_index_ >= model_start_index_[j]) && (_indicator_index_ < model_end_index_[j])) {
          //         DBGLOG_TIME_CLASS_FUNC << " Indicator Update: " << _indicator_index_ << " Model Update: " << j <<
          //         DBGLOG_ENDL_FLUSH;
          sum_vars_vec_[j] += _new_value_ - prev_value_vec_[_indicator_index_];
          break;
        }
      }
      // sum_vars_ += ( _new_value_ - prev_value_vec_ [ _indicator_index_ ] ) ;
      prev_value_vec_[_indicator_index_] = _new_value_;

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
            indicator_vec_.size(), this, (_this_weight_ * last_updated_baseprice_),
            false);  // premultiply the basepx to avoid multiplication during model computation
      } else {
        p_this_indicator_->add_indicator_listener(indicator_vec_.size(), this, _this_weight_, false);
      }

      bool indicator_already_added_ = false;
      for (unsigned i = 0; i < indicator_vec_.size(); i++) {
        if (indicator_vec_[i] == p_this_indicator_) {
          indicator_already_added_ = true;
        }
      }
      indicator_vec_.push_back(p_this_indicator_);

      const bool t_is_this_indicator_ready_ = p_this_indicator_->IsIndicatorReady();
      if (t_is_this_indicator_ready_ == false && !indicator_already_added_) {
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

  void StartCreation() {
    model_start_index_.push_back(indicator_vec_.size());
    if (dbglogger_.CheckLoggingLevel(DBG_MODEL_ERROR)) {
      DBGLOG_TIME_CLASS_FUNC << " Model Number " << num_models_ << " MStartIndexSize: " << model_start_index_.size()
                             << " IndicatorVecSize: " << indicator_vec_.size() << DBGLOG_ENDL_FLUSH;
    }
  }

  void FinishCreation(bool all_models_finished_) {
    model_end_index_.push_back(indicator_vec_.size());
    num_models_++;
    if (all_models_finished_) {
      sum_vars_vec_.resize(num_models_, 0);
      prev_value_vec_.resize(indicator_vec_.size());
    }
    unsigned this_model_start_ = model_start_index_[model_start_index_.size() - 1];
    unsigned this_model_end_ = model_end_index_[model_end_index_.size() - 1];
    DBGLOG_TIME_CLASS_FUNC << " Model No: " << num_models_ << DBGLOG_ENDL_FLUSH;

    for (unsigned j = this_model_start_; j < this_model_end_; j++) {
      DBGLOG_TIME_CLASS_FUNC << " Indicator [ " << j << " ] " << indicator_vec_[j]->concise_indicator_description()
                             << DBGLOG_ENDL_FLUSH;
    }

    if (dbglogger_.CheckLoggingLevel(DBG_MODEL_ERROR)) {
      DBGLOG_TIME_CLASS_FUNC << " Model Number " << num_models_ << " MStartIndexSize: " << model_start_index_.size()
                             << " IndicatorVecSize: " << indicator_vec_.size() << DBGLOG_ENDL_FLUSH;
    }
    if (is_ready_vec_.empty()) {
      is_ready_ = true;
      DBGLOG_TIME_CLASS_FUNC << "No Indicators! So by default Ready!" << DBGLOG_ENDL_FLUSH;
    }
    DBGLOG_DUMP;
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
      CalculateSumVars();
      double new_target_price_ = dep_market_view_.price_from_type(dep_baseprice_type_) + sum_vars_;

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

  void CalculateSumVars() {
    // select the sumvars based on the regime
    int count_ = 0;
    sum_vars_ = 0;
    if (dep_high_volume_period_ || core_high_volume_period_ || dep_trend_) {
      // DBGLOG_TIME_CLASS_FUNC << " in if cond: " << dep_high_volume_period_ << " " << core_high_volume_period_ << " "
      // << dep_trend_ << DBGLOG_ENDL_FLUSH;
      // if there are more than one periods active, take the strongest signal
      if (dep_high_volume_period_) {
        sum_vars_ += sum_vars_vec_[0];
        count_++;
        // DBGLOG_TIME_CLASS_FUNC << "High Volume dep : "  << sum_vars_ << DBGLOG_ENDL_FLUSH;
      }

      if (core_high_volume_period_) {
        sum_vars_ += sum_vars_vec_[1];
        count_++;
        // DBGLOG_TIME_CLASS_FUNC << "High Volume core : "  << sum_vars_ << DBGLOG_ENDL_FLUSH;
      }

      if (dep_trend_) {
        sum_vars_ += sum_vars_vec_[2];
        count_++;
        // DBGLOG_TIME_CLASS_FUNC << "dep price move: "  << sum_vars_ << DBGLOG_ENDL_FLUSH;
      }
      sum_vars_ /= double(count_);
    } else {
      sum_vars_ = sum_vars_vec_[3];  // not a special time

      // DBGLOG_TIME_CLASS_FUNC << "Nothing : "  << sum_vars_ << DBGLOG_ENDL_FLUSH;
    }

    // DBGLOG_TIME_CLASS_FUNC << "Sum vars_ "<< sum_vars_ << " "  << sum_vars_vec_[0] << " "<< sum_vars_vec_[1] << " "
    // << sum_vars_vec_[2] << " " << sum_vars_vec_[3]  << DBGLOG_ENDL_FLUSH;
  }

  void OnMarketUpdate(unsigned int security_id_, const HFSAT::MarketUpdateInfo& _market_update_info_) {
    if (security_id_ == dep_market_view_.market_update_info_.security_id_) {
      last_price_ = dep_market_view_.market_update_info_.mid_price_;
      // used only for price updates
    }
  }

  void OnTimePeriodUpdate(int num_pages_to_add_) {
    if (min_dep_price_ > last_price_) {
      min_dep_price_ = last_price_;
    }
    if (max_dep_price_ < last_price_) {
      max_dep_price_ = last_price_;
    }
    if (watch_.msecs_from_midnight() > last_updated_msecs_ + DECAY_LENGTH)  // decay lenght will b correct?
    {
      int this_num_pages_to_add_ = (watch_.msecs_from_midnight() - last_updated_msecs_) /
                                   DECAY_LENGTH;  // having page widht equal to decay length

      dep_high_volume_period_ = false;
      core_high_volume_period_ = false;

      for (unsigned security_id_ = 0; security_id_ < sid_to_average_moving_volume_.size();
           security_id_++)  // can optimize here because we use only dep and core shc?
      {
        if (!sid_to_value_required_[security_id_]) continue;
        //   DBGLOG_TIME_CLASS_FUNC  << " size traded between updates: " << sid_to_last_size_traded_ [ security_id_ ]
        //    << DBGLOG_ENDL_FLUSH;
        if (sid_to_first_time_[security_id_]) {
          sid_to_average_moving_volume_[security_id_] = sid_to_last_size_traded_[security_id_];
          sid_to_first_time_[security_id_] = false;
          sid_to_last_size_traded_[security_id_] = 0;
        } else {
          sid_to_average_moving_volume_[security_id_] =
              (1.0 - alpha_ * this_num_pages_to_add_) * (sid_to_average_moving_volume_[security_id_]) +
              sid_to_last_size_traded_[security_id_];
          sid_to_last_size_traded_[security_id_] = 0;
        }

        //          DBGLOG_TIME_CLASS_FUNC << " Moving Vol " << sid_to_average_moving_volume_ [ security_id_]
        //            << " Historical Vol: " << sid_to_average_statistical_volume_[ security_id_ ]
        //            << " alpha: " << alpha_
        //            << " Num Pages to add: " << this_num_pages_to_add_
        //            << DBGLOG_ENDL_FLUSH;

        if ((sid_to_average_moving_volume_[security_id_] > sid_to_average_statistical_volume_[security_id_])) {
          if (security_id_ == dep_market_view_.market_update_info_.security_id_) {
            dep_high_volume_period_ = true;
          } else {
            core_high_volume_period_ = true;  // can also calculate the factor
          }
        }
      }

      min_dep_price_in_interval_ = min_dep_price_;
      max_dep_price_in_interval_ = max_dep_price_;
      min_dep_price_ = last_price_;
      max_dep_price_ = last_price_;
      dep_trend_ = false;
      // DBGLOG_TIME_CLASS_FUNC << " Max price: " << max_dep_price_in_interval_ << " Min Price: " <<
      // min_dep_price_in_interval_ << DBGLOG_ENDL_FLUSH;
      if ((max_dep_price_in_interval_ - min_dep_price_in_interval_) >
          PRICE_MOVE_FACTOR * max_historical_price_move_in_day_) {
        dep_trend_ = true;
      }

      last_updated_msecs_ = watch_.msecs_from_midnight();
    }
  }

  void OnTradePrint(unsigned int security_id_, const HFSAT::TradePrintInfo& _tradeprint_info_,
                    const HFSAT::MarketUpdateInfo& _market_update_info_) {
    // DBGLOG_TIME_CLASS_FUNC << " Traded Volume: " << _tradeprint_info_.size_traded_ << " Sec: " <<
    // _market_update_info_.secname_ << DBGLOG_ENDL_FLUSH;
    sid_to_last_size_traded_[security_id_] += _tradeprint_info_.size_traded_;
  }

  void LoadHistoricalValues() {
    int interval_in_training_ = 600;
    std::stringstream st_;
    st_ << interval_in_training_;
    // int this_duration_;
    // load the historical data for the product and its core shortcodes
    // for testing purpose, using that day data only, in live we nned to use the avg over some days
    std::string this_stat_filename_ = std::string(STATS_DIR) + "/avg_vol_price_" + st_.str();
    int file_size_ = 0;
    if (FileUtils::ExistsWithSize(this_stat_filename_, file_size_)) {
      std::ifstream stats_file_(this_stat_filename_);
      if (stats_file_.is_open()) {
        char this_line_[1024];
        while (stats_file_.getline(this_line_, sizeof(this_line_))) {
          PerishableStringTokenizer st_(this_line_, sizeof(this_line_));
          const std::vector<const char*> tokens_ = st_.GetTokens();
          if (tokens_.size() < 4) {
            continue;
          }
          int sid_ = dep_market_view_.sec_name_indexer_.GetIdFromString(tokens_[0]);
          if (sid_ < 0) continue;

          if (sid_to_value_required_[sid_]) {
            sid_to_average_statistical_volume_[sid_] = atoi(tokens_[1]) / 10;
            DBGLOG_TIME_CLASS_FUNC << " This Shortcode: " << tokens_[0] << " AvgVolume: " << atoi(tokens_[1])
                                   << DBGLOG_ENDL_FLUSH;
          }
          if (sid_ == int(dep_market_view_.market_update_info_.security_id_)) {
            max_historical_price_move_in_day_ = fabs(atof(tokens_[2]) - atof(tokens_[3]));
            DBGLOG_TIME_CLASS_FUNC << " This Shortcode: " << tokens_[0]
                                   << " Max price move: " << max_historical_price_move_in_day_ << DBGLOG_ENDL_FLUSH;
          }
        }
      }
    } else {
      DBGLOG_TIME_CLASS_FUNC << "The file: " << this_stat_filename_ << " doesn't exist.\n" << DBGLOG_ENDL_FLUSH;
    }
  }
};
}
#endif  // BASE_MODELMATH_SELECTIVE_MODEL_AGGREGATOR_H
