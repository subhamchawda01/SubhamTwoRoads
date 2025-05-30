/**
    \file ExecLogic/options_exec_interface.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#include "dvctrade/ExecLogic/options_exec_interface.hpp"

namespace HFSAT {

void OptionsExecInterface::LoadParamSetVec(std::string paramfilename_) {
  std::ifstream paramlistfile_;
  paramlistfile_.open(paramfilename_.c_str());
  bool paramset_file_list_read_ = false;
  if (paramlistfile_.is_open()) {
    const int kParamfileListBufflerLen = 1024;
    char readlinebuffer_[kParamfileListBufflerLen];
    bzero(readlinebuffer_, kParamfileListBufflerLen);
    while (paramlistfile_.good()) {
      bzero(readlinebuffer_, kParamfileListBufflerLen);
      paramlistfile_.getline(readlinebuffer_, kParamfileListBufflerLen);
      std::string this_line_ = std::string(readlinebuffer_);
      PerishableStringTokenizer st_(readlinebuffer_, kParamfileListBufflerLen);
      const std::vector<const char*>& tokens_ = st_.GetTokens();
      if (tokens_.size() < 1) {
        continue;
      }
      if ((strcmp(tokens_[0], "PARAMVALUE") == 0) && !paramset_file_list_read_) {
        paramlistfile_.close();
        common_paramset_ = OptionsParamSet(paramfilename_, watch_.YYYYMMDD(), underlying_market_view_.shortcode());
        break;
      }
    }
    if (paramlistfile_.is_open()) {
      paramlistfile_.close();
    }
  } else {
    std::cerr << "OptionsExecInterface::LoadOptionsParamSetVec: can't open paramlistfile_: " << paramfilename_
              << " for reading\n";
    exit(1);
  }
}

void OptionsExecInterface::AddProductToTrade(SecurityMarketView* _dep_market_view_,
                                             SmartOrderManager* _smart_order_manager_, BaseModelMath* _model_math_,
                                             std::string _paramfilename_, int _trading_start_mfm_,
                                             int _trading_end_mfm_) {
  total_products_trading_++;

  options_market_view_vec_.push_back(_dep_market_view_);
  _dep_market_view_->subscribe_price_type(this, kPriceTypeMidprice);
  _dep_market_view_->subscribe_MktStatus(this);

  order_manager_vec_.push_back(_smart_order_manager_);
  _smart_order_manager_->SetOrderManager(throttle_manager_);
  _smart_order_manager_->AddPositionChangeListener(this);
  _smart_order_manager_->AddExecutionListener(this);
  _smart_order_manager_->AddCancelRejectListener(this);

  products_being_traded_.push_back(_dep_market_view_->shortcode());

  OptionsParamSet paramset_(_paramfilename_, watch_.YYYYMMDD(), _dep_market_view_->shortcode());

  model_math_vec_.push_back(_model_math_);
  _model_math_->AddListener(this, total_products_trading_ - 1);

  param_set_vec_.push_back(paramset_);

  sec_id_to_index_[_dep_market_view_->security_id()] = total_products_trading_ - 1;

  OptionsExecVars* option_to_hedge_ = new OptionsExecVars();
  option_to_hedge_->smv_ = _dep_market_view_;


  options_data_vec_.push_back(option_to_hedge_);
}

void OptionsExecInterface::BuildTradeVarSets() {
  BuildConstantTradeVarSets();

  BuildPositionTradeVarSetMap(&common_paramset_, &underlying_market_view_, prod_position_tradevarset_map_,
                              map_pos_increment_, P2TV_zero_idx_, livetrading_);
}

// KP no useful anymore
void OptionsExecInterface::BuildConstantTradeVarSets() {
  closeout_zeropos_tradevarset_ =
      TradeVars_t(HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE,
                  HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE,
                  HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE, 0, 0);

  closeout_long_tradevarset_ = TradeVars_t(
      HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE, 0, 0, HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE,
      HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE, 0,
      MathUtils::GetFlooredMultipleOf(common_paramset_.unit_trade_size_, underlying_market_view_.min_order_size()));

  closeout_short_tradevarset_ = TradeVars_t(
      0, 0, HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE,
      HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE,
      MathUtils::GetFlooredMultipleOf(common_paramset_.unit_trade_size_, underlying_market_view_.min_order_size()), 0);
}

void OptionsExecInterface::BuildPositionTradeVarSetMap(OptionsParamSet* param, const SecurityMarketView* smv,
                                                       PositionTradeVarSetMap& position_tradevarset_map,
                                                       int& map_pos_increment, const int& P2TV_zero_idx,
                                                       bool livetrading) {
  const int original_max_position_ = param->max_position_;

  if (param->max_position_ < param->unit_trade_size_) {  // True only for MUR < 1.0
    param->max_position_ = param->unit_trade_size_;      // Build the tradevarset assuming MUR = 1
  }
  // boundary of what is considered zero_position
  double zeropos_limit_position_ = 0;
  zeropos_limit_position_ = (param->zeropos_limits_unit_ratio_ * (double)param->unit_trade_size_);

  // boundary of what is considered high_position
  double highpos_limit_position_ = 0;
  highpos_limit_position_ = (param->highpos_limits_unit_ratio_ * (double)param->unit_trade_size_);

  // keeping increase size unchanged in high_position mode
  // just increasing the decrease trade size to get away from high position faster
  int highpos_adjusted_increase_trade_size_ =
      MathUtils::GetFlooredMultipleOf(param->unit_trade_size_, smv->min_order_size());
  // int highpos_adjusted_increase_trade_size_ = ( int ) round ( ( double ) param_set_.unit_trade_size_ / ( 1 +
  // param_set_.highpos_size_factor_ ) ) ; // keeping increase size unchanged in high_position mode
  int highpos_adjusted_decrease_trade_size_ = MathUtils::GetFlooredMultipleOf(
      (int)round((double)param->unit_trade_size_ * (1 + param->highpos_size_factor_)), smv->min_order_size());

  // upto param_set_.max_position_ = MAX_POS_MAP_SIZE , keep a map_pos_increment_ = 1,
  // then onwards increase to keep the maximum map size to ( 2 * MAX_POS_MAP_SIZE + 1 )
  // for _this_position_ position_tradevarset_map [ P2TV_zero_idx + round ( _this_position_ / map_pos_increment_ ) ]
  // is the applicable threshold set
  map_pos_increment = std::max(1, (int)ceil((double)param->max_position_ / (double)MAX_POS_MAP_SIZE));
  // position_tradevarset_map.resize ( 2 * MAX_POS_MAP_SIZE + 1 ) ; //not needed, as appropriate allocation is done
  // in constructor

  // for zeropos
  position_tradevarset_map[P2TV_zero_idx].Assign(
      param->zeropos_place_, param->zeropos_keep_, param->zeropos_place_, param->zeropos_keep_,
      param->zeropos_place_ + param->improve_, param->zeropos_keep_ + param->improve_,
      param->zeropos_place_ + param->aggressive_, param->zeropos_place_ + param->improve_,
      param->zeropos_keep_ + param->improve_, param->zeropos_place_ + param->aggressive_,
      MathUtils::GetFlooredMultipleOf(std::max(0, std::min(param->max_position_, param->unit_trade_size_)),
                                      smv->min_order_size()),
      MathUtils::GetFlooredMultipleOf(std::max(0, std::min(param->max_position_, param->unit_trade_size_)),
                                      smv->min_order_size()));

  if (livetrading || dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
    DBGLOG_FUNC << "Position " << 0 << " mapidx " << P2TV_zero_idx << ' '
                << ToString(position_tradevarset_map[P2TV_zero_idx]).c_str() << DBGLOG_ENDL_FLUSH;
  }

  const double very_high_barrier_ = 100;
  const double override_signal_ = 1.0;

  // for positive position values
  for (unsigned int i = (P2TV_zero_idx + 1); i < position_tradevarset_map.size(); i++) {
    int for_position_ = (i - P2TV_zero_idx) * map_pos_increment;
    bool set_explicitly_ = false;

    if (!set_explicitly_) {
      if (for_position_ <= zeropos_limit_position_) {  // [ 0, zeropos ]

        double _zeropos_fraction_ = ((double)for_position_ / (double)zeropos_limit_position_);

        position_tradevarset_map[i].Assign(
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
        position_tradevarset_map[i].Assign(
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

        position_tradevarset_map[i].Assign(
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

        position_tradevarset_map[i].Assign(
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

        position_tradevarset_map[i].Assign(
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

      if (original_max_position_ != param->max_position_) {  // True only for fractional MUR < 1.0
        if (for_position_ >= original_max_position_) {       // For long position beyond the original MUR * UTS , set
                                                             // bid-place and keep thresh.
          position_tradevarset_map[i].AddBidsBy(100.0);
        }
      }

      // if ( for_position_ <= param_set_.max_position_ )
      if (livetrading_ || dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        DBGLOG_FUNC << "Position " << for_position_ << " mapidx " << i << ' '
                    << ToString(position_tradevarset_map[i]).c_str() << DBGLOG_ENDL_FLUSH;
      }
    }
  }

  // for negative position values
  for (int i = (P2TV_zero_idx - 1); i >= 0; i--) {
    int for_position_ = (i - P2TV_zero_idx) * map_pos_increment;

    bool set_explicitly_ = false;

    if (!set_explicitly_) {
      if (for_position_ >= -zeropos_limit_position_) {  // [ -zeropos, 0 ]

        double _zeropos_fraction_ = ((double)-for_position_ / (double)zeropos_limit_position_);
        position_tradevarset_map[i].Assign(
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

        position_tradevarset_map[i].Assign(
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

        position_tradevarset_map[i].Assign(
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

        position_tradevarset_map[i].Assign(
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

        position_tradevarset_map[i].Assign(
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
    }  // end !set_explicitly_

    if (original_max_position_ != param->max_position_) {  // True only for fractional MUR < 1.0.
      if (for_position_ <= -original_max_position_) {      // For short position beyond the original MUR * UTS , set
                                                           // ask-place and keep thresh.
        position_tradevarset_map[i].AddAsksBy(100.0);
      }
    }

    // if ( for_position_ >= -param_set_.max_position_ )
    if (livetrading_ || dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
      DBGLOG_FUNC << "Position " << for_position_ << " mapidx " << i << ' '
                  << ToString(position_tradevarset_map[i]).c_str() << DBGLOG_ENDL_FLUSH;
    }
  }

  // multiply all values by smv->min_price_increment ( )
  // since it will be compared to targetprice and bestmarket price differences
  for (auto i = 0u; i < position_tradevarset_map.size(); i++) {
    position_tradevarset_map[i].MultiplyBy(smv->min_price_increment());
  }
}
}
