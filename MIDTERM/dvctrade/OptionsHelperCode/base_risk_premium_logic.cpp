#include "dvctrade/OptionsHelper/base_risk_premium_logic.hpp"

namespace HFSAT {

BaseOptionRiskPremium::BaseOptionRiskPremium(DebugLogger& _dbglogger_, const Watch& _watch_,
                                             std::vector<SecurityMarketView*> _underlying_smv_vec_,
                                             const int r_tradingdate_, int _trading_start_mfm_, int _trading_end_mfm_)
    : dbglogger_(_dbglogger_),
      watch_(_watch_),
      trading_start_mfm_(_trading_start_mfm_),
      trading_end_mfm_(_trading_end_mfm_),
      underlying_smv_vec_(_underlying_smv_vec_),
      P2TV_zero_idx_(MAX_POS_MAP_SIZE) {
  unsigned int num_underlying_ = underlying_smv_vec_.size();
  underlying_2_options_smv_map_ = HFSAT::MultModelCreator::GetShcToConstSMVMap();
  closeout_zeropos_tradevarset_ =
      TradeVars_t(HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE,
                  HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE,
                  HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE, 0, 0);
  map_pos_increment_vec_.resize(num_underlying_, false);
  delta_tradevarset_vec_.resize(num_underlying_, DeltaTradeVarSetMap());
  risk_matrix_.resize(num_underlying_, std::vector<OptionRisk>());
  std::map<std::string, std::string> shc_param_file_map_ = HFSAT::MultModelCreator::GetParamFileMap();
  indicators_helper_ = new MultipleIndicatorHelper(dbglogger_, watch_, trading_start_mfm_, trading_end_mfm_);

  // a current tradevar-set of constituent
  // this current tradevarset is derived from underlying vector using total detla at point (
  // OnPositionChange/OnDeltaChange ), futher this is scaled up/down using current stdev of the underlying / current
  // stdev of the constituent / current spread of the constituent
  // we need to know which indicators to compute, underlying stdev / option stdev / option bidaskspread
  // underlying stdev will go with MULT
  // option stdev will also go with MULT
  // option bidaskspread will also with MULT

  for (unsigned int idx = 0; idx < num_underlying_; idx++) {
    OptionsParamSet* paramset_ = new OptionsParamSet(shc_param_file_map_[underlying_smv_vec_[idx]->shortcode()],
                                                     r_tradingdate_, underlying_smv_vec_[idx]->shortcode());

    if (paramset_->scale_by_fut_stdev_) {
      indicators_helper_->ComputeSlowStdev(paramset_->fut_stdev_duration_, underlying_smv_vec_[idx]);
    }

    paramset_vec_.push_back(paramset_);  // this is underlying referenced param profile
    current_delta_tradevarset_map_index_.push_back(P2TV_zero_idx_);
    BuildConstituentsRiskStructs(idx);

    current_tradevarset_matrix_.push_back(std::vector<TradeVars_t>());
    BuildConstituentsPremiumMaps(idx, delta_tradevarset_vec_[idx]);  // one per underlying
  }

  watch_.subscribe_FifteenMinutesPeriod(this);
}

void BaseOptionRiskPremium::BuildConstituentsPremiumMaps(int _product_index_,
                                                         DeltaTradeVarSetMap& prod_delta_tradevarset_map_) {
  // first we set the premium profile for the option
  // second we set the risk profile for option

  // premium profile
  // This map should be learned offline. Online Changes can also be made (but not very frequently) ?
  // online stdev or bidaskspread
  prod_delta_tradevarset_map_.resize(2 * MAX_POS_MAP_SIZE + 1);
  BuildDeltaTradeVarSetMap(paramset_vec_[_product_index_], underlying_smv_vec_[_product_index_],
                           prod_delta_tradevarset_map_, map_pos_increment_vec_[_product_index_], P2TV_zero_idx_);

  std::vector<SecurityMarketView*> constituents_smv_list_ =
      underlying_2_options_smv_map_[underlying_smv_vec_[_product_index_]->shortcode()];
  unsigned int num_constituents_ =
      underlying_2_options_smv_map_[underlying_smv_vec_[_product_index_]->shortcode()].size();

  for (unsigned int constituents_indx_ = 0; constituents_indx_ < num_constituents_; constituents_indx_++) {
    if (paramset_vec_[_product_index_]->scale_by_opt_stdev_) {
      indicators_helper_->ComputeSlowStdev(paramset_vec_[_product_index_]->opt_stdev_duration_,
                                           constituents_smv_list_[constituents_indx_]);
    }
    if (paramset_vec_[_product_index_]->scale_by_opt_bidaskspread_) {
      indicators_helper_->ComputeMovingBidAskSpread(paramset_vec_[_product_index_]->opt_bidaskspread_duration_,
                                                    constituents_smv_list_[constituents_indx_]);
    }

    current_tradevarset_matrix_[_product_index_].push_back(closeout_zeropos_tradevarset_);
  }
}

void BaseOptionRiskPremium::BuildConstituentsRiskStructs(int _product_index_) {
  // Risk profile
  std::vector<SecurityMarketView*> constituents_smv_list_ =
      underlying_2_options_smv_map_[underlying_smv_vec_[_product_index_]->shortcode()];
  unsigned int num_constituents_ = constituents_smv_list_.size();

  double sum_delta_ = 0;
  double sum_gamma_ = 0;
  double sum_vega_ = 0;

  double maximum_delta_ = 0;
  double maximum_gamma_ = 0;
  double maximum_vega_ = 0;

  for (unsigned int constituents_indx_ = 0; constituents_indx_ < num_constituents_; constituents_indx_++) {
    OptionRisk t_risk_;
    t_risk_.max_loss_ =
        paramset_vec_[_product_index_]->max_loss_;  // we give same max_loss ( adjust max_pos to normalize )

    HFSAT::OptionObject* t_opt_obj_ = HFSAT::OptionObject::GetUniqueInstance(
        dbglogger_, watch_, constituents_smv_list_[constituents_indx_]->shortcode());
    // distribue based on delta
    // HFSAT::NSESecurityDefinitions::GetUniqueInstance(date_).GetLastCloseForOptions(dep_short_code);
    double fut_px_ = HFSAT::NSESecurityDefinitions::GetUniqueInstance(watch_.YYYYMMDD())
                         .GetLastClose(constituents_smv_list_[constituents_indx_]->shortcode());
    double opt_px_ = HFSAT::NSESecurityDefinitions::GetUniqueInstance(watch_.YYYYMMDD())
                         .GetLastCloseForOptions(constituents_smv_list_[constituents_indx_]->shortcode());

    std::string shc_ = constituents_smv_list_[constituents_indx_]->shortcode();
    while(opt_px_ < 0)
    {  
      shc_ = HFSAT::NSESecurityDefinitions::GetPrevOptionInCurrentSchema(shc_);
      opt_px_ = HFSAT::NSESecurityDefinitions::GetUniqueInstance(watch_.YYYYMMDD()).GetLastCloseForOptions(shc_);
    }

    t_opt_obj_->ComputeGreeks(fut_px_, opt_px_);

    DBGLOG_TIME_CLASS_FUNC_LINE << constituents_smv_list_[constituents_indx_]->shortcode()
                                << " Delta: " << t_opt_obj_->greeks_.delta_ << " Gamma: " << t_opt_obj_->greeks_.gamma_
                                << " Vega: " << t_opt_obj_->greeks_.vega_ << DBGLOG_ENDL_FLUSH;

    if (paramset_vec_[_product_index_]->distribute_risk_stats_ == 1) {
      // Here we are distributing risk in delta weighted sense
      t_risk_.max_position_ = paramset_vec_[_product_index_]->max_position_ * std::fabs(t_opt_obj_->greeks_.delta_);
      t_risk_.worst_position_ =
          paramset_vec_[_product_index_]->worst_case_position_ * std::fabs(t_opt_obj_->greeks_.delta_);
      t_risk_.max_gamma_ = floor(paramset_vec_[_product_index_]->max_global_gamma_ * t_opt_obj_->greeks_.gamma_);
      t_risk_.max_vega_ = floor(paramset_vec_[_product_index_]->max_global_vega_ * t_opt_obj_->greeks_.vega_);
      sum_delta_ += std::fabs(t_opt_obj_->greeks_.delta_);
      sum_gamma_ += t_opt_obj_->greeks_.gamma_;
      sum_vega_ += t_opt_obj_->greeks_.vega_;
    } else if (paramset_vec_[_product_index_]->distribute_risk_stats_ == 2) {
      // Here we are distributing risk in delta weighted sense but we are keeping risk for maximum delta contract same
      // and reducing the risk for other by delta ratio
      t_risk_.max_position_ = paramset_vec_[_product_index_]->max_position_ * std::fabs(t_opt_obj_->greeks_.delta_);
      t_risk_.worst_position_ =
          paramset_vec_[_product_index_]->worst_case_position_ * std::fabs(t_opt_obj_->greeks_.delta_);
      t_risk_.max_gamma_ = floor(paramset_vec_[_product_index_]->max_global_gamma_ * t_opt_obj_->greeks_.gamma_);
      t_risk_.max_vega_ = floor(paramset_vec_[_product_index_]->max_global_vega_ * t_opt_obj_->greeks_.vega_);
      maximum_delta_ = std::max(maximum_delta_, std::fabs(t_opt_obj_->greeks_.delta_));
      maximum_gamma_ = std::max(maximum_delta_, std::fabs(t_opt_obj_->greeks_.gamma_));
      maximum_vega_ = std::max(maximum_delta_, std::fabs(t_opt_obj_->greeks_.vega_));

    } else {
      // risk value is lower but volume is also lower, hence overall keeping same max position is fine !?
      // we hope not all of them hit their limits at same time and controlling these will be easier overall
      // we also get flat on one side, which should maitain the risk limits anyway
      t_risk_.max_position_ = paramset_vec_[_product_index_]->max_position_;
      t_risk_.worst_position_ = paramset_vec_[_product_index_]->worst_case_position_;
      t_risk_.max_gamma_ = paramset_vec_[_product_index_]->max_global_gamma_;
      t_risk_.max_vega_ = paramset_vec_[_product_index_]->max_global_vega_;

      // if (dbglogger_.CheckLoggingLevel(DBG_PARAM_INFO)) {
      DBGLOG_TIME_CLASS_FUNC_LINE << constituents_smv_list_[constituents_indx_]->shortcode()
                                  << " MaxPos: " << t_risk_.max_position_ << " WorstPos: " << t_risk_.worst_position_
                                  << " MaxGamma: " << t_risk_.max_gamma_ << " MaxVega: " << t_risk_.max_vega_
                                  << DBGLOG_ENDL_FLUSH;
      //}
    }

    t_risk_.type_ =
        (OptionType_t)NSESecurityDefinitions::GetOptionType(constituents_smv_list_[constituents_indx_]->shortcode());
    t_risk_.cooloff_interval_ = paramset_vec_[_product_index_]->cooloff_interval_;
    t_risk_.agg_cooloff_interval_ = paramset_vec_[_product_index_]->agg_cooloff_interval_;
    t_risk_.allowed_to_aggress_ = paramset_vec_[_product_index_]->allowed_to_aggress_;
    t_risk_.max_position_to_aggress_ = paramset_vec_[_product_index_]->max_position_to_aggress_;
    t_risk_.max_int_spread_to_place_ = paramset_vec_[_product_index_]->max_int_spread_to_place_;
    t_risk_.max_int_spread_to_cross_ = paramset_vec_[_product_index_]->max_int_spread_to_cross_;
    t_risk_.min_int_price_to_place_ = paramset_vec_[_product_index_]->min_int_price_to_place_;

    risk_matrix_[_product_index_].push_back(t_risk_);  // deep copy? not sure !!
  }

  if (paramset_vec_[_product_index_]->distribute_risk_stats_ == 1) {
    for (unsigned int constituents_indx_ = 0; constituents_indx_ < num_constituents_; constituents_indx_++) {
      risk_matrix_[_product_index_][constituents_indx_].max_position_ = HFSAT::MathUtils::GetCeilMultipleOf(
          risk_matrix_[_product_index_][constituents_indx_].max_position_ / sum_delta_,
          underlying_smv_vec_[_product_index_]->min_order_size());
      risk_matrix_[_product_index_][constituents_indx_].worst_position_ = HFSAT::MathUtils::GetCeilMultipleOf(
          risk_matrix_[_product_index_][constituents_indx_].worst_position_ / sum_delta_,
          underlying_smv_vec_[_product_index_]->min_order_size());
      risk_matrix_[_product_index_][constituents_indx_].max_gamma_ /= sum_gamma_;
      risk_matrix_[_product_index_][constituents_indx_].max_vega_ /= sum_vega_;
      DBGLOG_TIME_CLASS_FUNC_LINE << constituents_smv_list_[constituents_indx_]->shortcode()
                                  << " Max Pos: " << risk_matrix_[_product_index_][constituents_indx_].max_position_
                                  << " Worst Pos: " << risk_matrix_[_product_index_][constituents_indx_].worst_position_
                                  << " MaxGamma : " << risk_matrix_[_product_index_][constituents_indx_].max_gamma_
                                  << " MaxVega: " << risk_matrix_[_product_index_][constituents_indx_].max_vega_
                                  << DBGLOG_ENDL_FLUSH;
    }
  }

  if (paramset_vec_[_product_index_]->distribute_risk_stats_ == 2) {
    for (unsigned int constituents_indx_ = 0; constituents_indx_ < num_constituents_; constituents_indx_++) {
      risk_matrix_[_product_index_][constituents_indx_].max_position_ = HFSAT::MathUtils::GetCeilMultipleOf(
          risk_matrix_[_product_index_][constituents_indx_].max_position_ / maximum_delta_,
          underlying_smv_vec_[_product_index_]->min_order_size());
      risk_matrix_[_product_index_][constituents_indx_].worst_position_ = HFSAT::MathUtils::GetCeilMultipleOf(
          risk_matrix_[_product_index_][constituents_indx_].worst_position_ / maximum_delta_,
          underlying_smv_vec_[_product_index_]->min_order_size());
      risk_matrix_[_product_index_][constituents_indx_].max_gamma_ /= maximum_gamma_;
      risk_matrix_[_product_index_][constituents_indx_].max_vega_ /= maximum_vega_;
      DBGLOG_TIME_CLASS_FUNC_LINE << constituents_smv_list_[constituents_indx_]->shortcode()
                                  << " Max Pos: " << risk_matrix_[_product_index_][constituents_indx_].max_position_
                                  << " Worst Pos: " << risk_matrix_[_product_index_][constituents_indx_].worst_position_
                                  << " MaxGamma : " << risk_matrix_[_product_index_][constituents_indx_].max_gamma_
                                  << " MaxVega: " << risk_matrix_[_product_index_][constituents_indx_].max_vega_
                                  << DBGLOG_ENDL_FLUSH;
    }
  }
}

// update delta index and correponding current_tradevarset_vector
void BaseOptionRiskPremium::OnRiskChange(int _product_index_, double current_delta_, double current_gamma_,
                                         double current_vega_) {
  unsigned int call_index_ =
      current_delta_ > 0
          ? P2TV_zero_idx_ +
                std::min(MAX_POS_MAP_SIZE, (int)(abs(current_delta_) / map_pos_increment_vec_[_product_index_]))
          : P2TV_zero_idx_ -
                std::min(MAX_POS_MAP_SIZE, (int)(abs(current_delta_) / map_pos_increment_vec_[_product_index_]));

  current_delta_tradevarset_map_index_[_product_index_] = call_index_;
  unsigned int put_index_ = 2 * P2TV_zero_idx_ - current_delta_tradevarset_map_index_[_product_index_];

  std::vector<SecurityMarketView*> const_smv_list_ =
      underlying_2_options_smv_map_[underlying_smv_vec_[_product_index_]->shortcode()];
  unsigned int num_const_ = const_smv_list_.size();
  for (unsigned int const_indx_ = 0; const_indx_ < num_const_; const_indx_++) {
    if (risk_matrix_[_product_index_][const_indx_].type_ == OptionType_t::CALL) {
      current_tradevarset_matrix_[_product_index_][const_indx_] = delta_tradevarset_vec_[_product_index_][call_index_];
      /*if(current_vega_ >= paramset_vec_[_product_index_]->max_global_vega_) {
        current_tradevarset_matrix_[_product_index_][const_indx_].l1bid_trade_size_ = 0;
      } else if (current_vega_ <= -paramset_vec_[_product_index_]->max_global_vega_) {
        current_tradevarset_matrix_[_product_index_][const_indx_].l1ask_trade_size_ = 0;
      }*/
    } else {
      current_tradevarset_matrix_[_product_index_][const_indx_] = delta_tradevarset_vec_[_product_index_][put_index_];
      /*if(current_vega_ >= paramset_vec_[_product_index_]->max_global_vega_) {
            current_tradevarset_matrix_[_product_index_][const_indx_].l1bid_trade_size_ = 0;
          } else if (current_vega_ <= -paramset_vec_[_product_index_]->max_global_vega_) {
        current_tradevarset_matrix_[_product_index_][const_indx_].l1ask_trade_size_ = 0;
          }*/
    }

    if (paramset_vec_[_product_index_]->scale_by_fut_stdev_) {
      current_tradevarset_matrix_[_product_index_][const_indx_].MultiplyBy(
          indicators_helper_->slow_stdev(underlying_smv_vec_[_product_index_]->security_id()));
    }
    if (paramset_vec_[_product_index_]->scale_by_opt_stdev_) {
      current_tradevarset_matrix_[_product_index_][const_indx_].MultiplyBy(
          indicators_helper_->slow_stdev(const_smv_list_[const_indx_]->security_id()));
    }
    if (paramset_vec_[_product_index_]->scale_by_opt_bidaskspread_) {
      current_tradevarset_matrix_[_product_index_][const_indx_].MultiplyBy(
          indicators_helper_->moving_bidask_spread(const_smv_list_[const_indx_]->security_id()));
    }

    if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
      DBGLOG_TIME_CLASS_FUNC_LINE << " BP " << current_tradevarset_matrix_[_product_index_][const_indx_].l1bid_place_
                                  << " AP " << current_tradevarset_matrix_[_product_index_][const_indx_].l1ask_place_
                                  << DBGLOG_ENDL_FLUSH;
    }
  }
}

TradeVars_t BaseOptionRiskPremium::GetPremium(int _product_index_, int _option_index_, int _position_) {
  TradeVars_t _tradevarset_;

  _tradevarset_ = current_tradevarset_matrix_[_product_index_][_option_index_];

  // TODO :: Here check for individual vega && global gamma

  if (_position_ >= risk_matrix_[_product_index_][_option_index_].max_position_) _tradevarset_.l1bid_trade_size_ = 0;
  if (_position_ <= -risk_matrix_[_product_index_][_option_index_].max_position_) _tradevarset_.l1ask_trade_size_ = 0;

  return _tradevarset_;
}

// update current_tradevarset_set ( just in case enough time has passed for exec, should make much diff
void BaseOptionRiskPremium::OnTimePeriodUpdate(const int num_pages_to_add_) {
  for (unsigned int _product_index_ = 0; _product_index_ < underlying_smv_vec_.size(); _product_index_++) {
    unsigned int call_index_ = current_delta_tradevarset_map_index_[_product_index_];
    unsigned int put_index_ = 2 * P2TV_zero_idx_ - call_index_;

    std::vector<SecurityMarketView*> const_smv_list_ =
        underlying_2_options_smv_map_[underlying_smv_vec_[_product_index_]->shortcode()];
    unsigned int num_const_ = const_smv_list_.size();
    for (unsigned int const_indx_ = 0; const_indx_ < num_const_; const_indx_++) {
      if (risk_matrix_[_product_index_][const_indx_].type_ == OptionType_t::CALL) {
        current_tradevarset_matrix_[_product_index_][const_indx_] =
            delta_tradevarset_vec_[_product_index_][call_index_];
      } else {
        current_tradevarset_matrix_[_product_index_][const_indx_] = delta_tradevarset_vec_[_product_index_][put_index_];
      }

      if (paramset_vec_[_product_index_]->scale_by_fut_stdev_) {
        current_tradevarset_matrix_[_product_index_][const_indx_].MultiplyBy(
            indicators_helper_->slow_stdev(underlying_smv_vec_[_product_index_]->security_id()));
      }
      if (paramset_vec_[_product_index_]->scale_by_opt_stdev_) {
        current_tradevarset_matrix_[_product_index_][const_indx_].MultiplyBy(
            indicators_helper_->slow_stdev(const_smv_list_[const_indx_]->security_id()));
      }
      if (paramset_vec_[_product_index_]->scale_by_opt_bidaskspread_) {
        current_tradevarset_matrix_[_product_index_][const_indx_].MultiplyBy(
            indicators_helper_->moving_bidask_spread(const_smv_list_[const_indx_]->security_id()));
      }

      if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        DBGLOG_TIME_CLASS_FUNC_LINE << " BP " << current_tradevarset_matrix_[_product_index_][const_indx_].l1bid_place_
                                    << " AP " << current_tradevarset_matrix_[_product_index_][const_indx_].l1ask_place_
                                    << DBGLOG_ENDL_FLUSH;
      }
    }
  }
}

// position wrt to underlying
// delta wrt to consitituents
void BaseOptionRiskPremium::BuildDeltaTradeVarSetMap(OptionsParamSet* param, const SecurityMarketView* smv,
                                                     DeltaTradeVarSetMap& delta_tradevarset_map, int& map_pos_increment,
                                                     const int& P2TV_zero_idx) {
  double zeropos_limit_position_ = 0;
  zeropos_limit_position_ = (param->zeropos_limits_unit_ratio_ * (double)param->unit_trade_size_);

  // boundary of what is considered high_position
  double highpos_limit_position_ = 0;
  highpos_limit_position_ = (param->highpos_limits_unit_ratio_ * (double)param->unit_trade_size_);

  // to getout of risk quicker
  int highpos_adjusted_increase_trade_size_ =
      MathUtils::GetFlooredMultipleOf(param->unit_trade_size_, smv->min_order_size());
  int highpos_adjusted_decrease_trade_size_ = MathUtils::GetFlooredMultipleOf(
      (int)round((double)param->unit_trade_size_ * (1 + param->highpos_size_factor_)), smv->min_order_size());

  // to hold [-max_position, max_position], a vector should be atleast, 2*max_postion + 1 size
  // for _this_position_ position_tradevarset_map [ P2TV_zero_idx + round ( _this_position_ / map_pos_increment_ ) ]
  // max_position_ = unit_trade_size_ * min_order_size_ * max_unit_ratio_

  // ideal increment would be min_order_size_, meaning unit_trade_size_ * max_unit_ratio_ < MAX_POS_MAP_SIZE
  // BUT WE ARE DEALING WITH DELTA'S,  MIN INCREAMENT OF DELTAS COULD BE 0.1, IDEAL WOULD BE
  // MIN_ORDER_SIZE*MIN(ALL_CONSTITUENTS_DELTA)
  // 0.1*75 = 7.5 of NIFTY
  map_pos_increment = std::max(1, (int)ceil((double)param->max_position_ / (double)MAX_POS_MAP_SIZE));

  // best(bid_place, bid_keep, ask_place, ask_keep)
  // improve and agg (bid_place, bid_keep, bid_place, ask_place, ask_keep, bid_place)
  // bidsize and asksize
  delta_tradevarset_map[P2TV_zero_idx].Assign(
      param->zeropos_place_, param->zeropos_keep_, param->zeropos_place_, param->zeropos_keep_,
      param->zeropos_place_ + param->improve_, param->zeropos_keep_ + param->improve_,
      param->zeropos_place_ + param->aggressive_, param->zeropos_place_ + param->improve_,
      param->zeropos_keep_ + param->improve_, param->zeropos_place_ + param->aggressive_,
      MathUtils::GetFlooredMultipleOf(std::max(0, std::min(param->max_position_, param->unit_trade_size_)),
                                      smv->min_order_size()),
      MathUtils::GetFlooredMultipleOf(std::max(0, std::min(param->max_position_, param->unit_trade_size_)),
                                      smv->min_order_size()));

  const double very_high_barrier_ = 100;
  const double override_signal_ = 1.0;

  // for positive delta values
  for (unsigned int i = (P2TV_zero_idx + 1); i < delta_tradevarset_map.size(); i++) {
    int for_position_ = (i - P2TV_zero_idx) * map_pos_increment;
    if (for_position_ <= zeropos_limit_position_) {  // [ 0, zeropos ]
      double _zeropos_fraction_ = ((double)for_position_ / (double)zeropos_limit_position_);

      delta_tradevarset_map[i].Assign(
          (param->zeropos_place_ + ((param->increase_place_ - param->zeropos_place_) * _zeropos_fraction_)),
          (param->zeropos_keep_ + ((param->increase_keep_ - param->zeropos_keep_) * _zeropos_fraction_)),
          (param->zeropos_place_ + ((param->decrease_place_ - param->zeropos_place_) * _zeropos_fraction_)),
          (param->zeropos_keep_ + ((param->decrease_keep_ - param->zeropos_keep_) * _zeropos_fraction_)),
          (param->zeropos_place_ + ((param->increase_place_ - param->zeropos_place_) * _zeropos_fraction_)) +
              param->improve_,
          (param->zeropos_keep_ + ((param->increase_keep_ - param->zeropos_keep_) * _zeropos_fraction_)) +
              param->improve_,
          (param->zeropos_place_ + ((param->increase_place_ - param->zeropos_place_) * _zeropos_fraction_)) +
              param->aggressive_,
          (param->zeropos_place_ + ((param->decrease_place_ - param->zeropos_place_) * _zeropos_fraction_)) +
              param->improve_,
          (param->zeropos_keep_ + ((param->decrease_keep_ - param->zeropos_keep_) * _zeropos_fraction_)) +
              param->improve_,
          (param->zeropos_place_ + ((param->decrease_place_ - param->zeropos_place_) * _zeropos_fraction_)) +
              param->aggressive_,
          MathUtils::GetFlooredMultipleOf(
              std::max(0, std::min((param->max_position_ - for_position_), param->unit_trade_size_)),
              smv->min_order_size()),
          MathUtils::GetFlooredMultipleOf(
              std::max(0, std::min((param->max_position_ + for_position_), param->unit_trade_size_)),
              smv->min_order_size()));

    } else if (for_position_ <= highpos_limit_position_) {  // ( zeropos, highpos ]
      delta_tradevarset_map[i].Assign(
          param->increase_place_, param->increase_keep_, param->decrease_place_, param->decrease_keep_,
          param->increase_place_ + param->improve_, param->increase_keep_ + param->improve_,
          param->increase_place_ + param->aggressive_, param->decrease_place_ + param->improve_,
          param->decrease_keep_ + param->improve_, param->decrease_place_ + param->aggressive_,
          MathUtils::GetFlooredMultipleOf(
              std::max(0, std::min((param->max_position_ - for_position_), param->unit_trade_size_)),
              smv->min_order_size()),
          MathUtils::GetFlooredMultipleOf(
              std::max(0, std::min((param->max_position_ + for_position_), param->unit_trade_size_)),
              smv->min_order_size()));

    } else if (for_position_ < param->max_position_) {  // ( highpos, maxpos ]
      double _super_highpos_fraction_ = ((double)for_position_ / (double)highpos_limit_position_);

      // high_position
      // increasing place-threshold of increasing position on high_position mode
      double highpos_adjusted_increase_place_ =
          param->increase_place_ + (_super_highpos_fraction_ * param->highpos_thresh_factor_);
      // increasing keep-threshold of increasing position on high_position mode
      double highpos_adjusted_increase_keep_ =
          param->increase_keep_ + (_super_highpos_fraction_ * param->highpos_thresh_factor_);
      // decreasing place-threshold of reducing position on high_position mode
      double highpos_adjusted_decrease_place_ =
          param->decrease_place_ - (_super_highpos_fraction_ * param->highpos_thresh_decrease_);
      // decreasing keep-threshold of reducing position on high_position mode
      double highpos_adjusted_decrease_keep_ =
          param->decrease_keep_ - (_super_highpos_fraction_ * param->highpos_thresh_decrease_);

      delta_tradevarset_map[i].Assign(
          highpos_adjusted_increase_place_, highpos_adjusted_increase_keep_, highpos_adjusted_decrease_place_,
          highpos_adjusted_decrease_keep_, highpos_adjusted_increase_place_ + param->improve_ + very_high_barrier_,
          highpos_adjusted_increase_keep_ + param->improve_ + very_high_barrier_,
          highpos_adjusted_increase_place_ + param->aggressive_ + very_high_barrier_,
          highpos_adjusted_decrease_place_ + param->improve_, highpos_adjusted_decrease_keep_ + param->improve_,
          highpos_adjusted_decrease_place_ + param->aggressive_,
          MathUtils::GetFlooredMultipleOf(
              std::max(0, std::min((param->max_position_ - for_position_), highpos_adjusted_increase_trade_size_)),
              smv->min_order_size()),
          MathUtils::GetFlooredMultipleOf(
              std::max(0, std::min((param->max_position_ + for_position_), highpos_adjusted_decrease_trade_size_)),
              smv->min_order_size()));

    } else if (for_position_ == param->max_position_) {  // [ maxpos, maxpos ]
      double _super_highpos_fraction_ = ((double)for_position_ / (double)highpos_limit_position_);

      // high_position
      // increasing place-threshold of increasing position on high_position mode
      double highpos_adjusted_increase_place_ =
          param->increase_place_ + (_super_highpos_fraction_ * param->highpos_thresh_factor_);
      // increasing keep-threshold of increasing position on high_position mode
      double highpos_adjusted_increase_keep_ =
          param->increase_keep_ + (_super_highpos_fraction_ * param->highpos_thresh_factor_);
      // decreasing place-threshold of reducing position on high_position mode
      double highpos_adjusted_decrease_place_ =
          param->decrease_place_ - (_super_highpos_fraction_ * param->highpos_thresh_decrease_);
      // decreasing keep-threshold of reducing position on high_position mode
      double highpos_adjusted_decrease_keep_ =
          param->decrease_keep_ - (_super_highpos_fraction_ * param->highpos_thresh_decrease_);

      delta_tradevarset_map[i].Assign(
          highpos_adjusted_increase_place_ + very_high_barrier_, highpos_adjusted_increase_keep_ + very_high_barrier_,
          highpos_adjusted_decrease_place_, highpos_adjusted_decrease_keep_,
          highpos_adjusted_increase_place_ + param->improve_ + very_high_barrier_,
          highpos_adjusted_increase_keep_ + param->improve_ + very_high_barrier_,
          highpos_adjusted_increase_place_ + param->aggressive_ + very_high_barrier_,
          highpos_adjusted_decrease_place_ + param->improve_, highpos_adjusted_decrease_keep_ + param->improve_,
          highpos_adjusted_decrease_place_ + param->aggressive_, 0,
          MathUtils::GetFlooredMultipleOf(
              std::max(0, std::min((param->max_position_ + for_position_), highpos_adjusted_decrease_trade_size_)),
              smv->min_order_size()));

    } else {  // ( maxpos, inf )
      double _super_highpos_fraction_ = ((double)for_position_ / (double)highpos_limit_position_);

      // high_position
      // increasing place-threshold of increasing position on high_position mode
      double highpos_adjusted_increase_place_ =
          param->increase_place_ + (_super_highpos_fraction_ * param->highpos_thresh_factor_);
      // increasing keep-threshold of increasing position on high_position mode
      double highpos_adjusted_increase_keep_ =
          param->increase_keep_ + (_super_highpos_fraction_ * param->highpos_thresh_factor_);
      // decreasing place-threshold of reducing position on high_position mode
      double highpos_adjusted_decrease_place_ =
          param->decrease_place_ - (_super_highpos_fraction_ * param->highpos_thresh_decrease_);
      // decreasing keep-threshold of reducing position on high_position mode
      double highpos_adjusted_decrease_keep_ =
          param->decrease_keep_ - (_super_highpos_fraction_ * param->highpos_thresh_decrease_);

      delta_tradevarset_map[i].Assign(
          highpos_adjusted_increase_place_ + very_high_barrier_, highpos_adjusted_increase_keep_ + very_high_barrier_,
          highpos_adjusted_decrease_place_ - override_signal_, highpos_adjusted_decrease_keep_ - override_signal_,
          highpos_adjusted_increase_place_ + param->improve_ + very_high_barrier_,
          highpos_adjusted_increase_keep_ + param->improve_ + very_high_barrier_,
          highpos_adjusted_increase_place_ + param->aggressive_ + very_high_barrier_,
          highpos_adjusted_decrease_place_ + param->improve_ +
              very_high_barrier_,  // disable agg decrease at these position levels since decrease place could be very
          // low, we are clearly here by accident
          highpos_adjusted_decrease_keep_ + param->improve_ + very_high_barrier_,  // Same logic as above??
          highpos_adjusted_decrease_place_ + param->aggressive_ + very_high_barrier_,
          0, MathUtils::GetFlooredMultipleOf(
                 std::max(0, std::min((param->max_position_ + for_position_), highpos_adjusted_decrease_trade_size_)),
                 smv->min_order_size()));
    }
  }

  // for negative position values
  for (int i = (P2TV_zero_idx - 1); i >= 0; i--) {
    int for_position_ = (i - P2TV_zero_idx) * map_pos_increment;

    if (for_position_ >= -zeropos_limit_position_) {  // [ -zeropos, 0 ]

      double _zeropos_fraction_ = ((double)-for_position_ / (double)zeropos_limit_position_);
      delta_tradevarset_map[i].Assign(
          (param->zeropos_place_ + ((param->decrease_place_ - param->zeropos_place_) * _zeropos_fraction_)),
          (param->zeropos_keep_ + ((param->decrease_keep_ - param->zeropos_keep_) * _zeropos_fraction_)),
          (param->zeropos_place_ + ((param->increase_place_ - param->zeropos_place_) * _zeropos_fraction_)),
          (param->zeropos_keep_ + ((param->increase_keep_ - param->zeropos_keep_) * _zeropos_fraction_)),
          (param->zeropos_place_ + ((param->decrease_place_ - param->zeropos_place_) * _zeropos_fraction_)) +
              param->improve_,
          (param->zeropos_keep_ + ((param->decrease_keep_ - param->zeropos_keep_) * _zeropos_fraction_)) +
              param->improve_,
          (param->zeropos_place_ + ((param->decrease_place_ - param->zeropos_place_) * _zeropos_fraction_)) +
              param->aggressive_,
          (param->zeropos_place_ + ((param->increase_place_ - param->zeropos_place_) * _zeropos_fraction_)) +
              param->improve_,
          (param->zeropos_keep_ + ((param->increase_keep_ - param->zeropos_keep_) * _zeropos_fraction_)) +
              param->improve_,
          (param->zeropos_place_ + ((param->increase_place_ - param->zeropos_place_) * _zeropos_fraction_)) +
              param->aggressive_,
          MathUtils::GetFlooredMultipleOf(
              std::max(0, std::min((param->max_position_ - for_position_), param->unit_trade_size_)),
              smv->min_order_size()),
          MathUtils::GetFlooredMultipleOf(
              std::max(0, std::min((param->max_position_ + for_position_), param->unit_trade_size_)),
              smv->min_order_size()));

    } else if (for_position_ >= -highpos_limit_position_) {  // [ -highpos, -zeropos )

      delta_tradevarset_map[i].Assign(
          param->decrease_place_, param->decrease_keep_, param->increase_place_, param->increase_keep_,
          param->decrease_place_ + param->improve_, param->decrease_keep_ + param->improve_,
          param->decrease_place_ + param->aggressive_, param->increase_place_ + param->improve_,
          param->increase_keep_ + param->improve_, param->increase_place_ + param->aggressive_,
          MathUtils::GetFlooredMultipleOf(
              std::max(0, std::min((param->max_position_ - for_position_), param->unit_trade_size_)),
              smv->min_order_size()),
          MathUtils::GetFlooredMultipleOf(
              std::max(0, std::min((param->max_position_ + for_position_), param->unit_trade_size_)),
              smv->min_order_size()));

    } else if (for_position_ > -param->max_position_) {  // ( -maxpos, -highpos )

      double _super_highpos_fraction_ = ((double)-for_position_ / (double)highpos_limit_position_);

      // high_position
      // increasing place-threshold of increasing position on high_position mode
      double highpos_adjusted_increase_place_ =
          param->increase_place_ + (_super_highpos_fraction_ * param->highpos_thresh_factor_);
      // increasing keep-threshold of increasing position on high_position mode
      double highpos_adjusted_increase_keep_ =
          param->increase_keep_ + (_super_highpos_fraction_ * param->highpos_thresh_factor_);
      // decreasing place-threshold of reducing position on high_position mode
      double highpos_adjusted_decrease_place_ =
          param->decrease_place_ - (_super_highpos_fraction_ * param->highpos_thresh_decrease_);
      // decreasing keep-threshold of reducing position on high_position mode
      double highpos_adjusted_decrease_keep_ =
          param->decrease_keep_ - (_super_highpos_fraction_ * param->highpos_thresh_decrease_);

      delta_tradevarset_map[i].Assign(
          highpos_adjusted_decrease_place_, highpos_adjusted_decrease_keep_, highpos_adjusted_increase_place_,
          highpos_adjusted_increase_keep_, highpos_adjusted_decrease_place_ + param->improve_,
          highpos_adjusted_increase_keep_ + param->improve_, highpos_adjusted_decrease_place_ + param->aggressive_,
          highpos_adjusted_increase_place_ + param->improve_ + very_high_barrier_,
          highpos_adjusted_increase_keep_ + param->improve_ + very_high_barrier_,
          highpos_adjusted_increase_place_ + param->aggressive_ + very_high_barrier_,
          MathUtils::GetFlooredMultipleOf(
              std::max(0, std::min((param->max_position_ - for_position_), highpos_adjusted_decrease_trade_size_)),
              smv->min_order_size()),
          MathUtils::GetFlooredMultipleOf(
              std::max(0, std::min((param->max_position_ + for_position_), highpos_adjusted_increase_trade_size_)),
              smv->min_order_size()));

    } else if (for_position_ == -param->max_position_) {  // [ -maxpos, -maxpos ]

      double _super_highpos_fraction_ = ((double)-for_position_ / (double)highpos_limit_position_);

      // high_position
      // increasing place-threshold of increasing position on high_position mode
      double highpos_adjusted_increase_place_ =
          param->increase_place_ + (_super_highpos_fraction_ * param->highpos_thresh_factor_);
      // increasing keep-threshold of increasing position on high_position mode
      double highpos_adjusted_increase_keep_ =
          param->increase_keep_ + (_super_highpos_fraction_ * param->highpos_thresh_factor_);
      // decreasing place-threshold of reducing position on high_position mode
      double highpos_adjusted_decrease_place_ =
          param->decrease_place_ - (_super_highpos_fraction_ * param->highpos_thresh_decrease_);
      // decreasing keep-threshold of reducing position on high_position mode
      double highpos_adjusted_decrease_keep_ =
          param->decrease_keep_ - (_super_highpos_fraction_ * param->highpos_thresh_decrease_);

      delta_tradevarset_map[i].Assign(
          highpos_adjusted_decrease_place_, highpos_adjusted_decrease_keep_,
          highpos_adjusted_increase_place_ + very_high_barrier_, highpos_adjusted_increase_keep_ + very_high_barrier_,
          highpos_adjusted_decrease_place_ + param->improve_, highpos_adjusted_decrease_keep_ + param->improve_,
          highpos_adjusted_decrease_place_ + param->aggressive_,
          highpos_adjusted_increase_place_ + param->improve_ + very_high_barrier_,
          highpos_adjusted_increase_place_ + param->improve_ + very_high_barrier_,
          highpos_adjusted_increase_place_ + param->aggressive_ + very_high_barrier_,
          MathUtils::GetFlooredMultipleOf(
              std::max(0, std::min((param->max_position_ - for_position_), highpos_adjusted_decrease_trade_size_)),
              smv->min_order_size()),
          0);

    } else {  // ( -inf, -maxpos )
      double _super_highpos_fraction_ = ((double)-for_position_ / (double)highpos_limit_position_);

      // high_position
      // increasing place-threshold of increasing position on high_position mode
      double highpos_adjusted_increase_place_ =
          param->increase_place_ + (_super_highpos_fraction_ * param->highpos_thresh_factor_);
      // increasing keep-threshold of increasing position on high_position mode
      double highpos_adjusted_increase_keep_ =
          param->increase_keep_ + (_super_highpos_fraction_ * param->highpos_thresh_factor_);
      // decreasing place-threshold of reducing position on high_position mode
      double highpos_adjusted_decrease_place_ =
          param->decrease_place_ - (_super_highpos_fraction_ * param->highpos_thresh_decrease_);
      // decreasing keep-threshold of reducing position on high_position mode
      double highpos_adjusted_decrease_keep_ =
          param->decrease_keep_ - (_super_highpos_fraction_ * param->highpos_thresh_decrease_);

      delta_tradevarset_map[i].Assign(
          highpos_adjusted_decrease_place_ - override_signal_, highpos_adjusted_decrease_keep_ - override_signal_,
          highpos_adjusted_increase_place_ + very_high_barrier_, highpos_adjusted_increase_keep_ + very_high_barrier_,
          highpos_adjusted_decrease_place_ + param->improve_ + very_high_barrier_,
          highpos_adjusted_decrease_keep_ + param->improve_ + very_high_barrier_,
          highpos_adjusted_decrease_place_ + param->aggressive_ + very_high_barrier_,
          highpos_adjusted_increase_place_ + param->improve_ + very_high_barrier_,
          highpos_adjusted_increase_keep_ + param->improve_ + very_high_barrier_,
          highpos_adjusted_increase_place_ + param->aggressive_ + very_high_barrier_,
          MathUtils::GetFlooredMultipleOf(
              std::max(0, std::min((param->max_position_ - for_position_), highpos_adjusted_decrease_trade_size_)),
              smv->min_order_size()),
          0);
    }
  }

  // multiply all values by smv->min_price_increment ( ) when spread is used or when none is used
  // since it will be compared to targetprice and bestmarket price differences
  if ((param->scale_by_opt_bidaskspread_) || !(param->scale_by_opt_stdev_ || param->scale_by_fut_stdev_)) {
    for (auto i = 0u; i < delta_tradevarset_map.size(); i++) {
      delta_tradevarset_map[i].MultiplyBy(smv->min_price_increment());
    }
  }
}
}
