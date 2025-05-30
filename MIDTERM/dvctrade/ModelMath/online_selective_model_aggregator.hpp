// =====================================================================================
//
//       Filename:  selective_model_aggregator_new.hpp
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
#pragma once

#define HISTORY_TO_LOOK 60000  // one minute
#define DECAY_LENGTH 100       // time period update
#define PRICE_MOVE_FACTOR \
  0.05  // in the model creation, for 10 minutes it was 0.1, should be a parameter, take from model??

#define STATS_DIR "/spare/local/tradeinfo/volatilitylogs/"

#include "dvctrade/ModelMath/base_model_math.hpp"
#include "dvctrade/Indicators/offline_breakout_adjusted_pairs_trend.hpp"

namespace HFSAT {
class OnlineSelectiveModelAggregatorNew : public BaseModelMath,
                                          public SecurityMarketViewChangeListener,
                                          public TimePeriodListener {
 protected:
  SecurityMarketView& dep_market_view_;
  std::vector<double> prev_value_vec_;  // prev value of individual indicators
  std::vector<double> sum_vars_vec_;
  std::vector<double> last_sum_vars_vec_;
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
  int regime_index_;
  std::vector<std::vector<double>> ind_idx_to_weight_map_;
  std::vector<std::vector<unsigned int>> ind_idx_to_presence_map_;
  std::vector<std::vector<unsigned int>> ind_idx_to_rank_map_;
  std::vector<std::vector<unsigned int>> model_to_ind_map_;

  bool flag = true;

 public:
  int num_indicators_ = 0;
  double observation_var_ = 0;
  std::vector<double> indicator_var_vec_;
  std::vector<double> current_weights_;
  std::vector<double> current_weights_var_;
  std::vector<Kalman::Online_LR_Weights*> kalman_filter_vec_;

  int pred_dur_ = 0;
  int weights_update_dur_ = 0;
  int num_samples_per_update_ = 0;
  double thres_factor_ = 0;
  int id_offset = 1;
  unsigned int added_ind_ = 0;
  bool first_indicator_call_ = false;
  std::string model_type_ = "";
  OnlineSelectiveModelAggregatorNew(DebugLogger& _dbglogger_, const Watch& _watch_,
                                    const std::string& _model_file_name_, SecurityMarketView& _dep_market_view_,
                                    PriceType_t _dep_baseprice_type_, bool _is_returns_based_, std::string _model_type_)
      : BaseModelMath(_dbglogger_, _watch_, _model_file_name_),
        dep_market_view_(_dep_market_view_),
        prev_value_vec_(),
        sum_vars_vec_(),
        last_sum_vars_vec_(),
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
        regime_index_(-1),
        model_type_(_model_type_) {
    sum_vars_vec_.clear();
    last_sum_vars_vec_.clear();
    prev_value_vec_.clear();

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

    // p_dep_indep_based_regime_ = OfflineBreakoutAdjustedPairsTrend::GetUniqueInstance ( _dbglogger_, _watch_,
    // dep_market_view_, indep_market_view_, 900, 900, 0.1, StringToPriceType_t ("MktSizeWPrice") );

    // LoadHistoricalValues ();
    GetHugeValueThreshold(dep_market_view_.shortcode());
    SetPropagateThresholds(dep_market_view_.shortcode());
  }

  virtual ~OnlineSelectiveModelAggregatorNew() {}

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

  void initialize_kalman_filter() {
    Kalman::Online_LR_Weights* temp_kalman_filter = new Kalman::Online_LR_Weights(
        num_indicators_, current_weights_, current_weights_var_, indicator_var_vec_, observation_var_, pred_dur_,
        weights_update_dur_, num_samples_per_update_, watch_, thres_factor_, model_filename_);

    kalman_filter_vec_.push_back(temp_kalman_filter);

    num_indicators_ = 0;
    current_weights_.erase(current_weights_.begin(), current_weights_.end());
    current_weights_var_.erase(current_weights_var_.begin(), current_weights_var_.end());
    indicator_var_vec_.erase(indicator_var_vec_.begin(), indicator_var_vec_.end());
    observation_var_ = 0;

    pred_dur_ = 0;
    weights_update_dur_ = 0;
    num_samples_per_update_ = 0;
    thres_factor_ = 0;

    watch_.subscribe_BigTimePeriod(&(*temp_kalman_filter));
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
    if (!first_indicator_call_) {
      if (model_type_ == "LINEAR") {
        parse_linear_model_file();
      } else {
        if (model_type_ == "SIGLR") {
          parse_siglr_model_file();
        } else {
          std::cout << "no model type found" << std::endl;
        }
      }
      create_model_toind_map();
      first_indicator_call_ = true;
    }

    if (_indicator_index_ == 0) {
      regime_index_ = _new_value_;
    } else {
      if (!is_ready_) {
        is_ready_vec_[_indicator_index_] = true;
        is_ready_ = AreAllReady();

        if ((!is_ready_) && (VectorUtils::CheckAllForValue(is_ready_vec_, false) == false) &&
            ((last_indicators_debug_print_ == 0) ||
             (watch_.msecs_from_midnight() >
              last_indicators_debug_print_ + 2000))) {  // some indicator is not ready but at least one other is ready
          last_indicators_debug_print_ = watch_.msecs_from_midnight();

          for (unsigned int i = 1; i < indicator_vec_.size(); i++) {
            if (!is_ready_vec_[i]) {  // if this indicator isn't ready
              // print this and
              // ask if it was ready, but just could not notify us ... basically did not get any update yet.
              if (indicator_vec_[i]->IsIndicatorReady()) {  // if ti was secretly ready
                is_ready_vec_[i] = indicator_vec_[i]->IsIndicatorReady();
                is_ready_ = AreAllReady();
              } else {
                if (!dbglogger_.IsNoLogs()) {
                  DBGLOG_TIME_CLASS_FUNC << "Indicator Not Ready [ " << i << " ] "
                                         << indicator_vec_[i]->concise_indicator_description() << " "
                                         << last_indicators_debug_print_ << DBGLOG_ENDL_FLUSH;
                  DBGLOG_DUMP;

                  indicator_vec_[i]->WhyNotReady();
                }
              }
            }
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

        int _this_model_ = 0;
        int _this_rank_ = 0;
        for (unsigned j = 0; j < ind_idx_to_presence_map_[_indicator_index_].size(); j++) {
          _this_model_ = ind_idx_to_presence_map_[_indicator_index_][j];
          _this_rank_ = ind_idx_to_rank_map_[_indicator_index_][j];

          (kalman_filter_vec_[_this_model_])->update_indicator(_new_value_, _this_rank_);

          (kalman_filter_vec_[_this_model_])->update_pred_val();
          last_sum_vars_vec_[ind_idx_to_presence_map_[_indicator_index_][j]] =
              sum_vars_vec_[ind_idx_to_presence_map_[_indicator_index_][j]];
          sum_vars_vec_[ind_idx_to_presence_map_[_indicator_index_][j]] =
              (kalman_filter_vec_[_this_model_])->get_pred_val();

          double this_ind_diff_ = sum_vars_vec_[ind_idx_to_presence_map_[_indicator_index_][j]] -
                                  last_sum_vars_vec_[ind_idx_to_presence_map_[_indicator_index_][j]];

          // we can omit this check if needed
          if (fabs(this_ind_diff_) > 10 * dep_market_view_.min_price_increment()) {
            if (!dbglogger_.IsNoLogs()) {
              std::cerr << watch_.tv() << ' ' << typeid(*this).name() << ':' << __func__ << ' '
                        << "HUGE value in sum_vars_. last updated : " << _indicator_index_ << " "
                        << indicator_vec_[_indicator_index_]->concise_indicator_description()
                        << " ptgt: " << (dep_market_view_.price_from_type(dep_baseprice_type_) + sum_vars_)
                        << " ntgt: " << (dep_market_view_.price_from_type(dep_baseprice_type_) + sum_vars_ +
                                         (_new_value_ - prev_value_vec_[_indicator_index_]))
                        << " numticks " << (fabs(_new_value_ - prev_value_vec_[_indicator_index_]) /
                                            dep_market_view_.min_price_increment())
                        << std::endl;

              indicator_vec_[_indicator_index_]->PrintDebugInfo();

              DBGLOG_TIME_CLASS_FUNC << "HUGE in sum_vars_. last updated : " << _indicator_index_ << " "
                                     << indicator_vec_[_indicator_index_]->concise_indicator_description()
                                     << " ptgt: " << (dep_market_view_.price_from_type(dep_baseprice_type_) + sum_vars_)
                                     << " ntgt: " << (dep_market_view_.price_from_type(dep_baseprice_type_) +
                                                      sum_vars_ + (_new_value_ - prev_value_vec_[_indicator_index_]))
                                     << DBGLOG_ENDL_FLUSH;
            }
          }
          //  sum_vars_vec_[ind_idx_to_presence_map_[_indicator_index_][j]] += this_ind_diff_;
        }

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
  void AddIndicator(CommonIndicator* _this_indicator_, bool _readiness_required_) {
    AddIndicator(_this_indicator_, 1.00, 1, false);
  }
  void AddIndicator(CommonIndicator* _this_indicator_, const double& _this_weight_, bool _readiness_required_) {
    AddIndicator(_this_indicator_, _this_weight_, 1, false);
  }

  void AddIndicator(CommonIndicator* p_this_indicator_, const double& _this_weight_, const unsigned int& _this_id_,
                    bool _readiness_required_) {
    if (p_this_indicator_ != NULL) {
      int ind_idx_ = -1;
      for (unsigned i = 0; i < indicator_vec_.size(); i++) {
        if (indicator_vec_[i] == p_this_indicator_) {
          ind_idx_ = i;
          break;
        }
      }

      double this_weight_ = _this_weight_;
      if (is_returns_based_ && initial_update_baseprice_) {
        this_weight_ *= last_updated_baseprice_;
      }

      if (ind_idx_ < 0) {
        is_ready_vec_.push_back(false);
        readiness_required_vec_.push_back(_readiness_required_);
        is_ready_ = false;
        ind_idx_ = indicator_vec_.size();

        ind_idx_to_weight_map_.push_back(std::vector<double>(1, this_weight_));
        ind_idx_to_rank_map_.push_back(std::vector<unsigned int>(1, _this_id_));
        ind_idx_to_presence_map_.push_back(std::vector<unsigned int>(1, num_models_));

        p_this_indicator_->add_unweighted_indicator_listener(indicator_vec_.size(), this);
        indicator_vec_.push_back(p_this_indicator_);
        prev_value_vec_.push_back(0.0);
      } else if (!VectorUtils::LinearSearchValue(ind_idx_to_presence_map_[ind_idx_], num_models_)) {
        ind_idx_to_weight_map_[ind_idx_].push_back(this_weight_);
        ind_idx_to_rank_map_[ind_idx_].push_back(_this_id_);
        ind_idx_to_presence_map_[ind_idx_].push_back(num_models_);

      } else {
        std::cerr << "Same indicator " << p_this_indicator_->concise_indicator_description() << " added twice in model "
                  << num_models_ << std::endl;
        exit(0);
      }

    } else {
      if (dbglogger_.CheckLoggingLevel(DBG_MODEL_ERROR)) {
        DBGLOG_TIME_CLASS_FUNC << "NULL t_indicator_" << DBGLOG_ENDL_FLUSH;
      }
      ExitVerbose(kExitErrorCodeGeneral, dbglogger_);
    }
  }

  void AddIndicator(CommonIndicator* p_this_indicator_, const double& _this_alpha_, const double& _this_beta_,
                    const unsigned int& _this_id_, bool _readiness_required_) {
    if (p_this_indicator_ != NULL) {
      int ind_idx_ = -1;
      for (unsigned i = 0; i < indicator_vec_.size(); i++) {
        if (indicator_vec_[i] == p_this_indicator_) {
          ind_idx_ = i;
          break;
        }
      }

      double this_beta_ = _this_beta_;
      if (is_returns_based_ && initial_update_baseprice_) {
        this_beta_ *= last_updated_baseprice_;
      }

      if (ind_idx_ < 0) {
        is_ready_vec_.push_back(false);
        readiness_required_vec_.push_back(_readiness_required_);
        is_ready_ = false;
        ind_idx_ = indicator_vec_.size();

        ind_idx_to_weight_map_.push_back(std::vector<double>(1, this_beta_));
        ind_idx_to_rank_map_.push_back(std::vector<unsigned int>(1, _this_id_));
        ind_idx_to_presence_map_.push_back(std::vector<unsigned int>(1, num_models_));

        p_this_indicator_->add_indicator_listener(indicator_vec_.size(), this, _this_alpha_, _this_beta_);
        indicator_vec_.push_back(p_this_indicator_);
        prev_value_vec_.push_back(0.0);
      } else if (!VectorUtils::LinearSearchValue(ind_idx_to_presence_map_[ind_idx_], num_models_)) {
        ind_idx_to_weight_map_[ind_idx_].push_back(this_beta_);
        ind_idx_to_rank_map_[ind_idx_].push_back(_this_id_);
        ind_idx_to_presence_map_[ind_idx_].push_back(num_models_);

      } else {
        std::cerr << "Same indicator " << p_this_indicator_->concise_indicator_description() << " added twice in model "
                  << num_models_ << std::endl;
        exit(0);
      }

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

  void FinishCreation(bool all_models_finished_) {
    num_models_++;
    if (all_models_finished_) {
      sum_vars_vec_.resize(num_models_, 0);
      last_sum_vars_vec_.resize(num_models_, 0);
      prev_value_vec_.resize(indicator_vec_.size());
    }
  }

  void InterpretModelParameters(const std::vector<const char*> _tokens_) {  // MODELARGS ...
  }

  void ShowIndicatorValues() {
    if (dbglogger_.CheckLoggingLevel(DBG_MODEL_ERROR)) {
      DBGLOG_TIME_CLASS_FUNC << "\n================================================================="
                             << DBGLOG_ENDL_FLUSH;
      dbglogger_ << " sum_vars: " << sum_vars_ << DBGLOG_ENDL_FLUSH;
      dbglogger_ << " Current Regime: " << regime_index_ << " of " << indicator_vec_[0]->concise_indicator_description()
                 << DBGLOG_ENDL_FLUSH;

      std::vector<double> temp_weights_vec = {};
      unsigned int temp_rank = 0;
      for (unsigned int j = 0; j < num_models_; j++) {
        dbglogger_ << "MODEL: " << j << DBGLOG_ENDL_FLUSH;
        temp_weights_vec = kalman_filter_vec_[j]->current_weights_;
        for (unsigned int i : model_to_ind_map_[j])  // Not Printing Regime Indicator
        {
          temp_rank = ind_idx_to_rank_map_[i][j];

          if (temp_rank > 0) {
            dbglogger_ << " value: "
                       << temp_weights_vec[temp_rank - 1] *
                              (prev_value_vec_[i] / dep_market_view_.min_price_increment())
                       << "weights: " << temp_weights_vec[temp_rank - 1] << " of "
                       << indicator_vec_[i]->concise_indicator_description() << DBGLOG_ENDL_FLUSH;
          }
        }
        dbglogger_ << "=================================================================" << DBGLOG_ENDL_FLUSH;
      }
    }
  }

  void ShowIndicatorValuesToScreen() {
    printf("\033[36;1H");  // move to 35 th line
    std::cout << "\n=================================================================" << std::endl;
    std::cout << " Current Regime: " << regime_index_ << " of " << indicator_vec_[0]->concise_indicator_description()
              << std::endl;

    std::vector<double> temp_weights_vec = {};
    unsigned int temp_rank;

    for (unsigned int j = 0; j < num_models_; j++) {
      std::cout << "MODEL: " << j << "\n";
      temp_weights_vec = kalman_filter_vec_[j]->current_weights_;
      for (unsigned int i : model_to_ind_map_[j]) {
        temp_rank = ind_idx_to_rank_map_[i][j];
        if (temp_rank > 0) {
          std::cout << " value: "
                    << temp_weights_vec[temp_rank - 1] * (prev_value_vec_[i] / dep_market_view_.min_price_increment())
                    << "weights: " << temp_weights_vec[temp_rank - 1] << " of "
                    << indicator_vec_[i]->concise_indicator_description() << "                  " << std::endl;
        }
      }

      std::cout << "=================================================================" << std::endl;
    }
  }

  /// debug info
  void DumpIndicatorValues() {
    DBGLOG_TIME_CLASS_FUNC << "\n================================================================="
                           << DBGLOG_ENDL_FLUSH;
    std::cout << " Current Regime: " << regime_index_ << " of " << indicator_vec_[0]->concise_indicator_description()
              << std::endl;
    std::vector<double> temp_weights_vec = {};
    unsigned int temp_rank;
    for (unsigned int j = 0; j < num_models_; j++) {
      dbglogger_ << "MODEL: " << j << DBGLOG_ENDL_FLUSH;
      temp_weights_vec = kalman_filter_vec_[j]->current_weights_;
      for (unsigned int i : model_to_ind_map_[j]) {
        temp_rank = ind_idx_to_rank_map_[i][j];
        if (temp_rank > 0) {
          dbglogger_ << " value: "
                     << temp_weights_vec[temp_rank - 1] * (prev_value_vec_[i] / dep_market_view_.min_price_increment())
                     << "weights: " << temp_weights_vec[temp_rank - 1] << " of "
                     << indicator_vec_[i]->concise_indicator_description() << DBGLOG_ENDL_FLUSH;
        }
      }
      dbglogger_ << "=================================================================" << DBGLOG_ENDL_FLUSH;
    }
  }

 protected:
  inline bool AreAllReady() {
    // if dependent not ready, all not ready
    if (!dep_market_view_.is_ready()) return false;

    if (use_implied_price_) {
      if (!p_implied_price_indicator_->IsIndicatorReady()) {
        return false;
      }
    }

    if (use_mid_price_base_) {
      if (!p_diff_price_indicator_->IsIndicatorReady()) return false;
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
      CalculateSumVars();
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

      // const double kMinTicksMoved = 0.015 ;
      double kMinTicksMoved = min_ticks_move_;

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

  void CalculateSumVars() {
    // select the sumvars based on the regime
    sum_vars_ = 0;
    if (regime_index_ <= 0) {
      for (auto i = 0u; i < sum_vars_vec_.size(); i++) {
        sum_vars_ += sum_vars_vec_[i];
      }
      sum_vars_ /= sum_vars_vec_.size();
    } else {
      sum_vars_ = sum_vars_vec_[regime_index_ - 1];
    }
    // DBGLOG_TIME_CLASS_FUNC << "Sum vars_ "<< sum_vars_ << " "  << sum_vars_vec_[0] << " "<< sum_vars_vec_[1] << " "
    // << sum_vars_vec_[2] << " " << sum_vars_vec_[3]  << DBGLOG_ENDL_FLUSH;
  }

  void OnMarketUpdate(unsigned int security_id_, const HFSAT::MarketUpdateInfo& _market_update_info_) { return; }

  void OnTimePeriodUpdate(int num_pages_to_add_) {
    /*if (p_dep_indep_based_regime_)
            {
            bool flag = p_dep_indep_based_regime_->IsIndicatorReady ( );
    //int prev_regime_index_ = regime_index_;
    regime_index_ = (int) p_dep_indep_based_regime_->indicator_value( flag );
    //if ( prev_regime_index_ != regime_index_ ) { DBGLOG_TIME << "Flag : " << flag << " Model switched to "<<
    regime_index_ - 1 << " " << p_dep_indep_based_regime_->concise_indicator_description ( ) << "\n"; }
    }*/
  }

  void OnTradePrint(unsigned int security_id_, const HFSAT::TradePrintInfo& _tradeprint_info_,
                    const HFSAT::MarketUpdateInfo& _market_update_info_) {
    // DBGLOG_TIME_CLASS_FUNC << " Traded Volume: " << _tradeprint_info_.size_traded_ << " Sec: " <<
    // _market_update_info_.secname_ << DBGLOG_ENDL_FLUSH;
    return;
  }

  void parse_linear_model_file() {
    bool start_reading = false;
    char delimiter = ' ';

    int indicator_index_ = 0;

    std::ifstream model_file;
    model_file.open(model_filename_.c_str(), std::ios::in);

    std::string l1;
    while (std::getline(model_file, l1)) {
      if (l1.at(0) == '#') {
        continue;
      }

      if (!start_reading) {
        if (l1.compare("INDICATORSTART") == 0) {
          start_reading = true;
          continue;
        } else {
          continue;
        }
      } else {
        if (l1.compare("INDICATORINTERMEDIATE") == 0) {
          initialize_kalman_filter();
          // start_reading = false;
          indicator_index_ = 0;
          continue;
        } else {
          if (l1.compare("INDICATOREND") == 0) {
            initialize_kalman_filter();
            start_reading = false;
            indicator_index_ = 0;
            break;
          }
        }
        std::stringstream ss(l1);
        std::string tok;

        double _this_weight_ = 0.0;
        double _this_weight_var_ = 0.0;
        double _this_indicator_var_ = 0.0;
        double _this_thres_factor_ = 0.0;
        double _this_num_samples_per_update_ = 0.0;

        getline(ss, tok, delimiter);
        getline(ss, tok, delimiter);

        std::istringstream iss(tok);
        std::string token;

        // rank of indicator is not needed here. so not storing it

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
      if (l1.at(0) == '#') {
        continue;
      }

      if (!start_reading) {
        if (l1.compare("INDICATORSTART") == 0) {
          start_reading = true;
          continue;
        } else {
          continue;
        }
      } else {
        if (l1.compare("INDICATORINTERMEDIATE") == 0) {
          initialize_kalman_filter();
          // start_reading = false;
          indicator_index_ = 0;
          continue;
        } else {
          if (l1.compare("INDICATOREND") == 0) {
            initialize_kalman_filter();
            start_reading = false;
            indicator_index_ = 0;
            break;
          }
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

        // rank of indicator is not needed here. so not storing it
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

  void create_model_toind_map() {
    std::vector<unsigned int> temp_vec = {};
    for (unsigned int j = 0; j < num_models_; j++) {
      temp_vec.erase(temp_vec.begin(), temp_vec.end());
      for (unsigned int i = 1; i < indicator_vec_.size(); i++) {
        if (ind_idx_to_presence_map_[i].end() !=
            std::find(ind_idx_to_presence_map_[i].begin(), ind_idx_to_presence_map_[i].end(), j)) {
          temp_vec.push_back(i);
        }
      }
      model_to_ind_map_.push_back(temp_vec);
    }
  }
};
}
