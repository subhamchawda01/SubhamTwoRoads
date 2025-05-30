// =====================================================================================
//
//       Filename:  equity_trading_2.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  12/23/2014 02:54:39 PM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
//
//        Address:  Suite No 162, Evoma, #14, Bhattarhalli,
//                  Old Madras Road, Near Garden City College,
//                  KR Puram, Bangalore 560049, India
//          Phone:  +91 80 4190 3551
//
// =====================================================================================

#include "dvctrade/ExecLogic/equity_trading_2.hpp"
#include "dvctrade/Indicators/index_utils.hpp"
#include "dvctrade/ExecLogic/vol_utils.hpp"

#define SMALL_PX_DIFF 1e-05
#define SMALL_COOLOFF_MSECS_ 10

namespace HFSAT {

EquityTrading2::EquityTrading2(DebugLogger& _dbglogger_, const Watch& _watch_,
                               const SecurityMarketView& _dep_market_view_, SmartOrderManager& _order_manager_,
                               MultBasePNL* _mult_base_pnl_, const std::string& _common_paramfilename_,
                               const bool _livetrading_, MulticastSenderSocket* _p_strategy_param_sender_socket_,
                               EconomicEventsManager& t_economic_events_manager_, const int t_trading_start_utc_mfm_,
                               const int t_trading_end_utc_mfm_, const int t_runtime_id_,
                               const std::vector<std::string> _this_model_source_shortcode_vec_,
                               SecurityNameIndexer& _sec_name_indexer_)
    : BaseTrading(_dbglogger_, _watch_, _dep_market_view_, _order_manager_, _common_paramfilename_, _livetrading_,
                  _p_strategy_param_sender_socket_, t_economic_events_manager_, t_trading_start_utc_mfm_,
                  t_trading_end_utc_mfm_, t_runtime_id_, _this_model_source_shortcode_vec_, true),
      sec_name_indexer_(_sec_name_indexer_),
      dep_market_view_vec_(),
      order_manager_vec_(),
      model_math_vec_(),
      prod_position_tradevarset_map_vec_(),
      prod_paramset_vec_(),
      common_paramset_(ParamSet(_common_paramfilename_, _watch_.YYYYMMDD(), _dep_market_view_.shortcode())),
      closeout_zeropos_tradevarset_vec_(),
      closeout_long_tradevarset_vec_(),
      closeout_short_tradevarset_vec_(),
      l1_size_indicator_vec_(),
      l1_order_indicator_vec_(),
      product_vec_(),
      current_product_(nullptr),
      lrdb_vec_(),
      retlrdb_vec_(),
      mult_base_pnl_(_mult_base_pnl_),
      order_placing_logic_(kPriceBasedVolTrading),
      px_to_be_placed_at_(),
      int_px_to_be_placed_at_(),
      size_to_be_placed_(),
      order_level_indicator_vec_(),
      offline_beta_(),
      stdevratio_with_index_(),
      sec_id_to_index_(),
      products_being_traded_(),
      index_returns_(0),
      total_index_risk_(0),
      index_name_("BR_IND_0"),
      total_products_trading_(0),
      initialized_(false),
      cpu_allocated_(false),
      betas_(),
      max_global_beta_adjusted_notional_risk_(10000000000.0),
      self_pos_projection_factor_(1),
      max_notionl_risk_(10000000000.0),
      counter_(0),
      total_pnl_(0),
      open_unrealized_pnl_(0),
      realized_pnl_(0),
      max_loss_(0),
      last_max_opentrade_loss_hit_msecs_(0),
      getting_flat_(false),
      aggressively_getting_flat_(false),
      base_bid_price_(0.0),
      base_ask_price_(0.0),
      best_bid_place_cxl_px_(0.0),
      best_ask_place_cxl_px_(0.0),
      best_int_bid_place_cxl_px_(0),
      best_int_ask_place_cxl_px_(0),
      print_on_trade_(false),
      spread_diff_factor_(0.0),
      index_volume_ratio_(0.0) {
  //

  mult_base_pnl_->AddListener(this);

  if (_dep_market_view_.shortcode().substr(0, 4) == "NSE_") {
    index_name_ = "NSE_NIFTY_FUT0";
  }
  sec_id_to_index_.resize(sec_name_indexer_.NumSecurityId(), -1);
  total_products_trading_ = 0;
  param_set_vec_.clear();  // removing the common paramset value from here

  if (common_paramset_.read_max_global_beta_adjusted_notional_risk_) {
    max_global_beta_adjusted_notional_risk_ = common_paramset_.max_global_beta_adjusted_notional_risk_;
  } else {
    std::cerr << "Running with default global_beta_notional_risk_ \n";
  }

  if (common_paramset_.read_self_pos_projection_factor_) {
    self_pos_projection_factor_ = common_paramset_.self_pos_projection_factor_;
  } else {
    std::cerr << "Running with default self_pos_projection_factor_ \n";
  }

  if (common_paramset_.read_max_loss_) {
    max_loss_ = common_paramset_.max_loss_;
  }
  if (common_paramset_.read_max_opentrade_loss_) {
    max_opentrade_loss_ = common_paramset_.max_opentrade_loss_;
  }

  index_market_view_ = ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(index_name_);

  beta_adjusted_notional_risk_ = 0;

  double volume_ratio_duration_ = 3600;
  if (common_paramset_.read_stdev_duration_) {
    volume_ratio_duration_ = common_paramset_.stdev_duration_;
  }

  VolumeRatioCalculator* p_volume_ratio_indicator_ =
      VolumeRatioCalculator::GetUniqueInstance(dbglogger_, watch_, *index_market_view_, volume_ratio_duration_);
  if (p_volume_ratio_indicator_) {
    p_volume_ratio_indicator_->AddVolumeRatioListener(0, this);
  }
}

void EquityTrading2::SetOrderPlacingLogic(const StrategyType _strategy_type_) {
  order_placing_logic_ = _strategy_type_;
}

void EquityTrading2::AddProductToTrade(SecurityMarketView* _dep_market_view_, SmartOrderManager* _smart_order_manager_,
                                       BaseModelMath* _model_math_, std::string _paramfilename_,
                                       int _trading_start_mfm_, int _trading_end_mfm_) {
  total_products_trading_++;
  InstrumentInfo* t_ins_ = new InstrumentInfo();

  dep_market_view_vec_.push_back(_dep_market_view_);
  _dep_market_view_->subscribe_price_type(this, kPriceTypeMktSizeWPrice);
  _dep_market_view_->subscribe_MktStatus(this);

  ezone_vec_.clear();
  ezone_vec_.push_back(EZ_BRL);
  severity_to_getflat_on_ = 1;

  economic_events_manager_.AllowEconomicEventsFromList(_dep_market_view_->shortcode());
  allowed_events_present_ = economic_events_manager_.AllowedEventsPresent();
  economic_events_manager_.AdjustSeverity(_dep_market_view_->shortcode(), _dep_market_view_->exch_source());

  order_manager_vec_.push_back(_smart_order_manager_);
  _smart_order_manager_->SetOrderManager(throttle_manager_);
  _smart_order_manager_->AddPositionChangeListener(this);
  _smart_order_manager_->AddExecutionListener(this);
  _smart_order_manager_->AddCancelRejectListener(this);
  _smart_order_manager_->AddRejectDueToFundsListener(this);
  _smart_order_manager_->AddFokFillRejectListener(this);

  model_math_vec_.push_back(_model_math_);
  products_being_traded_.push_back(_dep_market_view_->shortcode());
  lrdb_vec_.push_back(
      &HFSAT::OfflineReturnsLRDB::GetUniqueInstance(dbglogger_, watch_, _dep_market_view_->shortcode()));
  retlrdb_vec_.push_back(
      &HFSAT::OfflineReturnsRetLRDB::GetUniqueInstance(dbglogger_, watch_, _dep_market_view_->shortcode()));
  ParamSet* paramset_ = new ParamSet(_paramfilename_, watch_.YYYYMMDD(), _dep_market_view_->shortcode());

  // Putting this to build tradevarset in basetrading
  {
    ParamSet t_paramset_(_paramfilename_, watch_.YYYYMMDD(), _dep_market_view_->shortcode());

    param_set_vec_.push_back(t_paramset_);
  }

  if (paramset_->read_place_cancel_cooloff_) {
    _smart_order_manager_->place_cancel_cooloff_ = paramset_->place_cancel_cooloff_;
  }

  paramset_->high_spread_allowance_ *= _dep_market_view_->min_price_increment();

  if (paramset_->use_throttle_manager_) {
    t_ins_->throttle_manager_ =
        new HFSAT::ThrottleManager(param_set_.throttle_message_limit_);  // create for all queries, overhead: bool check
    t_ins_->throttle_manager_->start_throttle_manager(true);             // use only when needed
  } else {
    std::cerr << " ThrottleMessageLimit Not specified " << dep_market_view_.shortcode()
              << " . Trying to load it from offline-file." << std::endl;
    int throttle_message = ExecLogicUtils::GetThrottleForShortcode(dep_market_view_.shortcode(), trading_start_utc_mfm_,
                                                                   trading_end_utc_mfm_);
    if (throttle_message > 0) {
      t_ins_->throttle_manager_ =
          new HFSAT::ThrottleManager(throttle_message);  // create for all queries, overhead: bool check
      t_ins_->throttle_manager_->start_throttle_manager(true);
    }
  }

  prod_paramset_vec_.push_back(paramset_);

  PositionTradeVarSetMap t_map_(2 * MAX_POS_MAP_SIZE + 1);

  position_tradevarset_map_vec_.push_back(t_map_);
  prod_position_tradevarset_map_vec_.push_back(t_map_);

  std::map<int, double> feature_avg_l1sz_;
  int prod_size_ =
      SampleDataUtil::GetAvgForPeriod(_dep_market_view_->shortcode(), watch_.YYYYMMDD(), 60, trading_start_utc_mfm_,
                                      trading_end_utc_mfm_, "L1SZ", feature_avg_l1sz_);

  DBGLOG_CLASS_FUNC_LINE << "shc:" << _dep_market_view_->shortcode() << " L1Size: " << prod_size_ << DBGLOG_ENDL_FLUSH;
  _dep_market_view_->set_level_size_thresh(prod_size_);

  if (paramset_->read_use_stable_book_) {
    t_ins_->place_at_stable_prices_ = paramset_->use_stable_book_;
    DBGLOG_TIME_CLASS_FUNC_LINE << " Using stable book for " << _dep_market_view_->shortcode()
                                << " size: " << _dep_market_view_->level_size_thresh_ << DBGLOG_ENDL_FLUSH;
  } else {
    t_ins_->place_at_stable_prices_ = false;
  }

  BuildTradeVarSets(total_products_trading_ - 1);
  current_global_tradevarset_vec_.push_back(closeout_zeropos_tradevarset_vec_[total_products_trading_ - 1]);
  _model_math_->AddListener(this, total_products_trading_ - 1);
  //  _model_math_ -> SetModelMathIndex ( total_products_trading_ -1 );
  sec_id_to_index_[_dep_market_view_->security_id()] = total_products_trading_ - 1;

  DBGLOG_TIME_CLASS_FUNC << " trading_start_time_ " << _trading_start_mfm_ << " trading_end_time_ " << _trading_end_mfm_
                         << "common: " << trading_start_utc_mfm_ << " " << trading_end_utc_mfm_ << DBGLOG_ENDL_FLUSH;
  if (_trading_start_mfm_ >= 0) {
    t_ins_->trading_start_utc_mfm_ = _trading_start_mfm_;
  } else {
    t_ins_->trading_start_utc_mfm_ = trading_start_utc_mfm_;
  }

  if (_trading_end_mfm_ >= 0) {
    t_ins_->trading_end_utc_mfm_ = _trading_end_mfm_;
  } else {
    t_ins_->trading_end_utc_mfm_ = trading_end_utc_mfm_;
  }

  SlowStdevCalculator* t_stdev_ =
      SlowStdevCalculator::GetUniqueInstance(dbglogger_, watch_, _dep_market_view_->shortcode(), 100u * 1000u);
  t_stdev_->AddSlowStdevCalculatorListener(this);

  p_moving_bidask_spread_ = MovingAvgBidAskSpread::GetUniqueInstance(dbglogger_, watch_, _dep_market_view_, 300.0);
  p_moving_bidask_spread_->add_unweighted_indicator_listener(total_products_trading_ - 1, this);

  std::map<int, double> feature_avg_spread_;
  t_ins_->hist_avg_spread_ =
      SampleDataUtil::GetAvgForPeriod(_dep_market_view_->shortcode(), watch_.YYYYMMDD(), 60, trading_start_utc_mfm_,
                                      trading_end_utc_mfm_, "BidAskSpread", feature_avg_spread_);
  t_ins_->size_to_join_ = paramset_->min_size_to_join_;
  t_ins_->notional_risk_ = 0;

  double volume_ratio_duration_ = 3600;
  if (common_paramset_.read_stdev_duration_) {
    volume_ratio_duration_ = common_paramset_.stdev_duration_;
  }

  VolumeRatioCalculator* p_volume_ratio_indicator_ =
      VolumeRatioCalculator::GetUniqueInstance(dbglogger_, watch_, *_dep_market_view_, volume_ratio_duration_);
  if (p_volume_ratio_indicator_) {
    p_volume_ratio_indicator_->AddVolumeRatioListener(total_products_trading_, this);
  }
  product_vec_.push_back(t_ins_);
}

void EquityTrading2::BuildTradeVarSets(int _product_index_) {
  BuildConstantTradeVarSets(_product_index_);
  // Putting hack here to avoid extra code
  if ((int)prod_position_tradevarset_map_vec_.size() < _product_index_ + 1) {
    prod_position_tradevarset_map_vec_.resize(_product_index_ + 1);
  }
  if ((int)position_tradevarset_map_vec_.size() < _product_index_) {
    position_tradevarset_map_vec_.resize(_product_index_ + 1);
  }
  if ((int)prod_map_pos_increment_.size() < _product_index_ + 1) {
    prod_map_pos_increment_.resize(_product_index_ + 1, 1);
  }
  if ((int)map_pos_increment_vec_.size() < _product_index_ + 1) {
    map_pos_increment_vec_.resize(_product_index_ + 1, 1);
  }

  tradevarset_builder_->BuildPositionTradeVarSetMap(
      &param_set_vec_[_product_index_], dep_market_view_vec_[_product_index_],
      position_tradevarset_map_vec_[_product_index_], map_pos_increment_vec_[_product_index_], P2TV_zero_idx_,
      livetrading_);

  prod_position_tradevarset_map_vec_[_product_index_] =
      position_tradevarset_map_vec_[_product_index_];  // start with default value
  prod_map_pos_increment_[_product_index_] = map_pos_increment_vec_[_product_index_];
}

void EquityTrading2::BuildConstantTradeVarSets(int _product_index_) {
  if (closeout_zeropos_tradevarset_vec_.size() <= (unsigned)_product_index_) {
    closeout_zeropos_tradevarset_vec_.resize(_product_index_ + 1);
  }
  if (closeout_short_tradevarset_vec_.size() <= (unsigned)_product_index_) {
    closeout_short_tradevarset_vec_.resize(_product_index_ + 1);
  }
  if (closeout_long_tradevarset_vec_.size() <= (unsigned)_product_index_) {
    closeout_long_tradevarset_vec_.resize(_product_index_ + 1);
  }
  closeout_zeropos_tradevarset_vec_[_product_index_] =
      TradeVars_t(HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE,
                  HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE,
                  HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE, 0, 0);

  closeout_long_tradevarset_vec_[_product_index_] =
      TradeVars_t(HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE, 0, 0, HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE,
                  HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE, 0,
                  MathUtils::GetFlooredMultipleOf(prod_paramset_vec_[_product_index_]->unit_trade_size_,
                                                  dep_market_view_vec_[_product_index_]->min_order_size()));

  closeout_short_tradevarset_vec_[_product_index_] =
      TradeVars_t(0, 0, HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE,
                  HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE,
                  MathUtils::GetFlooredMultipleOf(prod_paramset_vec_[_product_index_]->unit_trade_size_,
                                                  dep_market_view_vec_[_product_index_]->min_order_size()),
                  0);
}

void EquityTrading2::TradingLogic(int _product_index_) {
  current_product_ = product_vec_[_product_index_];
  my_risk_ = (int)current_product_->beta_adjusted_position_;

  TradeVars_t& current_global_tradevarset_ = current_global_tradevarset_vec_[_product_index_];
  ParamSet* t_paramset_ = prod_paramset_vec_[_product_index_];
  SecurityMarketView* p_dep_market_view_ = dep_market_view_vec_[_product_index_];

  best_nonself_ask_int_price_ = current_product_->best_nonself_ask_int_price_;
  best_nonself_ask_price_ = current_product_->best_nonself_ask_price_;
  best_nonself_ask_size_ = current_product_->best_nonself_ask_size_;
  best_nonself_bid_int_price_ = current_product_->best_nonself_bid_int_price_;
  best_nonself_bid_price_ = current_product_->best_nonself_bid_price_;
  best_nonself_bid_size_ = current_product_->best_nonself_bid_size_;

  base_bid_price_ = target_price_vec_[_product_index_] - current_product_->m_stdev_;
  base_ask_price_ = target_price_vec_[_product_index_] + current_product_->m_stdev_;

  {
    // Using current vs historical spread
    spread_diff_factor_ =
        std::max(0.0, p_dep_market_view_->min_price_increment() * common_paramset_.spread_factor_ *
                          ((current_product_->moving_avg_spread_ - current_product_->hist_avg_spread_) / (2.0)));

    base_bid_price_ = base_bid_price_ - spread_diff_factor_;
    base_ask_price_ = base_ask_price_ + spread_diff_factor_;
    if (t_paramset_->read_spread_quote_factor_) {
      base_bid_price_ = base_bid_price_ - t_paramset_->spread_quote_factor_ * current_product_->spread_;
      base_ask_price_ = base_ask_price_ + t_paramset_->spread_quote_factor_ * current_product_->spread_;
    }
  }
  /// first define the best price at which one would be willing to place new orders
  /// to buy and sell - don't have separate place and cxl thresholds
  best_bid_place_cxl_px_ = base_bid_price_ - current_global_tradevarset_.l1bid_place_;
  best_ask_place_cxl_px_ = base_ask_price_ + current_global_tradevarset_.l1ask_place_;

  /// move to intpx space for future computations
  best_int_bid_place_cxl_px_ = floor(best_bid_place_cxl_px_ / p_dep_market_view_->min_price_increment());
  best_int_ask_place_cxl_px_ = ceil(best_ask_place_cxl_px_ / p_dep_market_view_->min_price_increment());

  if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
    DBGLOG_TIME_CLASS_FUNC_LINE << " Targetprice: " << target_price_vec_[_product_index_] << " "
                                << dep_market_view_vec_[_product_index_]->mid_price() << " "
                                << dep_market_view_vec_[_product_index_]->shortcode()
                                << " stdev: " << current_product_->m_stdev_
                                << " histsprd: " << current_product_->hist_avg_spread_
                                << " cursprd: " << current_product_->moving_avg_spread_
                                << " Best_Bid_Place_Cxl_Px: " << best_int_bid_place_cxl_px_
                                << " Best_Ask_Place_Cxl_Px: " << best_int_ask_place_cxl_px_ << " thresh: bid "
                                << current_global_tradevarset_.l1bid_place_
                                << " ask: " << current_global_tradevarset_.l1ask_place_ << " pos: " << my_risk_
                                << " prodPos: " << current_product_->position_ << DBGLOG_ENDL_FLUSH;
  }

  /// to reduce message count -- place-keep diff logic
  {
    int t_best_int_bid_place_cxl_px_check_ = floor(
        (best_bid_place_cxl_px_ + current_global_tradevarset_.l1bid_place_ - current_global_tradevarset_.l1bid_keep_) /
        p_dep_market_view_->min_price_increment());
    int t_best_int_ask_place_cxl_px_check_ = ceil(
        (best_ask_place_cxl_px_ - current_global_tradevarset_.l1ask_place_ + current_global_tradevarset_.l1ask_keep_) /
        p_dep_market_view_->min_price_increment());

    if (t_best_int_bid_place_cxl_px_check_ > best_int_bid_place_cxl_px_ &&
        order_manager_vec_[_product_index_]->GetTotalBidSizeOrderedAtIntPx(t_best_int_bid_place_cxl_px_check_) > 0) {
      best_int_bid_place_cxl_px_ = t_best_int_bid_place_cxl_px_check_;
      best_bid_place_cxl_px_ =
          best_bid_place_cxl_px_ + current_global_tradevarset_.l1bid_place_ - current_global_tradevarset_.l1bid_keep_;
    }

    if (t_best_int_ask_place_cxl_px_check_ < best_int_ask_place_cxl_px_ &&
        order_manager_vec_[_product_index_]->GetTotalAskSizeOrderedAtIntPx(t_best_int_ask_place_cxl_px_check_) > 0) {
      best_int_ask_place_cxl_px_ = t_best_int_ask_place_cxl_px_check_;
      best_ask_place_cxl_px_ =
          best_ask_place_cxl_px_ - current_global_tradevarset_.l1ask_place_ + current_global_tradevarset_.l1ask_keep_;
    }
  }

  /// avoid aggressive and improve orders if param settings so dictate
  if (!t_paramset_->allowed_to_aggress_) {
    if (!t_paramset_->allowed_to_improve_) {
      best_int_bid_place_cxl_px_ = std::min(best_nonself_bid_int_price_, best_int_bid_place_cxl_px_);
      best_int_ask_place_cxl_px_ = std::max(best_nonself_ask_int_price_, best_int_ask_place_cxl_px_);
    } else {
      // For places where spread > 2
      int best_int_bid_improve_place_cxl_px_ = best_int_bid_place_cxl_px_;
      int best_int_ask_improve_place_cxl_px_ = best_int_ask_place_cxl_px_;
      // Use thresholds only when the  best_int_bid_place_cxl_px_ is atleast 2 ticks far from bet bid
      if (best_nonself_bid_int_price_ < best_int_bid_place_cxl_px_) {
        best_int_bid_improve_place_cxl_px_ = std::max(
            best_nonself_bid_int_price_,
            (int)(floor(best_bid_place_cxl_px_ / p_dep_market_view_->min_price_increment() - t_paramset_->improve_)));
      }

      if (best_nonself_ask_int_price_ > best_int_ask_place_cxl_px_) {
        best_int_ask_improve_place_cxl_px_ = std::min(
            best_nonself_ask_int_price_,
            (int)(ceil(best_ask_place_cxl_px_ / p_dep_market_view_->min_price_increment() + t_paramset_->improve_)));
      }

      best_int_bid_place_cxl_px_ = std::min(best_nonself_ask_int_price_ - 1, best_int_bid_improve_place_cxl_px_);
      best_int_ask_place_cxl_px_ = std::max(best_nonself_bid_int_price_ + 1, best_int_ask_improve_place_cxl_px_);
    }
  } else  /// sanity check for HUGE value instances et al
  {
    best_int_bid_place_cxl_px_ = std::min(best_nonself_ask_int_price_, best_int_bid_place_cxl_px_);
    best_int_ask_place_cxl_px_ = std::max(best_nonself_bid_int_price_, best_int_ask_place_cxl_px_);
  }

  /// implement cooloff logic
  if (t_paramset_->allowed_to_aggress_) {
    if (watch_.msecs_from_midnight() - current_product_->last_bid_agg_msecs_ < t_paramset_->agg_cooloff_interval_) {
      best_int_bid_place_cxl_px_ = std::min(best_nonself_ask_int_price_ - 1, best_int_bid_place_cxl_px_);
    }
    if (watch_.msecs_from_midnight() - current_product_->last_ask_agg_msecs_ < t_paramset_->agg_cooloff_interval_) {
      best_int_ask_place_cxl_px_ = std::max(best_nonself_bid_int_price_ + 1, best_int_ask_place_cxl_px_);
    }
  }

  if (t_paramset_->allowed_to_improve_) {
    if (watch_.msecs_from_midnight() - current_product_->last_bid_imp_msecs_ < t_paramset_->improve_cooloff_interval_ &&
        watch_.msecs_from_midnight() - current_product_->last_buy_msecs_ < t_paramset_->cooloff_interval_) {
      best_int_bid_place_cxl_px_ = std::min(best_nonself_bid_int_price_, best_int_bid_place_cxl_px_);
    }
    if (watch_.msecs_from_midnight() - current_product_->last_ask_imp_msecs_ < t_paramset_->improve_cooloff_interval_ &&
        watch_.msecs_from_midnight() - current_product_->last_sell_msecs_ < t_paramset_->cooloff_interval_) {
      best_int_ask_place_cxl_px_ = std::max(best_nonself_ask_int_price_, best_int_ask_place_cxl_px_);
    }
  }

  if (t_paramset_->cooloff_interval_ > 0) {
    if (watch_.msecs_from_midnight() - current_product_->last_buy_msecs_ < t_paramset_->cooloff_interval_ &&
        fabs(current_product_->last_buy_int_price_ - best_int_bid_place_cxl_px_) < 5) {
      best_int_bid_place_cxl_px_ = std::min(current_product_->last_buy_int_price_ - 1, best_int_bid_place_cxl_px_);
    }
    if (watch_.msecs_from_midnight() - current_product_->last_sell_msecs_ < t_paramset_->cooloff_interval_ &&
        fabs(current_product_->last_sell_int_price_ - best_int_ask_place_cxl_px_) < 5) {
      best_int_ask_place_cxl_px_ = std::max(current_product_->last_sell_int_price_ + 1, best_int_ask_place_cxl_px_);
    }
  }
  /// to safeguard against ORS - MDS gap delays - quite visible in Liffe & BMF
  /// would be better done via book changes
  //    if ( last_buy_int_price_ <= best_nonself_bid_int_price_ &&
  //        watch_.msecs_from_midnight( ) - last_buy_msecs_ <  SMALL_COOLOFF_MSECS_ )
  //      {
  //        best_int_bid_place_cxl_px_ = std::min( last_buy_int_price_ - 1, best_int_bid_place_cxl_px_ );
  //      }
  //
  //    if ( last_sell_int_price_ >= best_nonself_ask_int_price_ &&
  //        watch_.msecs_from_midnight( ) - last_sell_msecs_ < SMALL_COOLOFF_MSECS_ )
  //      {
  //        best_int_ask_place_cxl_px_ = std::max( last_sell_int_price_ + 1, best_int_ask_place_cxl_px_ );
  //      }

  /*                              CANCEL LOGIC                                */
  int retval_ = 0;
  int bid_retval_ = 0;
  int ask_retval_ = 0;
  int base_iter_bid_px_ = best_int_bid_place_cxl_px_;
  int base_iter_ask_px_ = best_int_ask_place_cxl_px_;
  int bid_cancel_px_ = best_int_bid_place_cxl_px_;
  int ask_cancel_px_ = best_int_ask_place_cxl_px_;
  /// On Bid Side - depending on whether we intend to place on top level or not
  if (current_global_tradevarset_.l1bid_trade_size_ > 0) {
    base_iter_bid_px_ = best_int_bid_place_cxl_px_;
  } else {
    base_iter_bid_px_ = best_nonself_bid_int_price_ - t_paramset_->px_band_;
    bid_cancel_px_ = base_iter_bid_px_;
  }
  bid_retval_ = order_manager_vec_[_product_index_]->SumBidSizeConfirmedAboveIntPrice(base_iter_bid_px_);

  /// Symmetric for Ask Side
  if (current_global_tradevarset_.l1ask_trade_size_ > 0) {
    base_iter_ask_px_ = best_int_ask_place_cxl_px_;
  } else {
    base_iter_ask_px_ = best_nonself_ask_int_price_ + t_paramset_->px_band_;
    ask_cancel_px_ = base_iter_ask_px_;
  }
  ask_retval_ = order_manager_vec_[_product_index_]->SumAskSizeConfirmedAboveIntPrice(base_iter_ask_px_);

  /*
   *                CANCEL ORDERS IN BAND
   *                // moving it inline with place logic
   */

  /*                                       PLACE LOGIC                                        */
  /// first bid side
  /*
   * Problem : When using notional value for calculating risk, if dep_1_price/dep_2_price_  is very big,
   * Any position in dep1 would either not let dep2 trade,
   */
  int tot_buy_placed_ = bid_retval_;
  int tot_sell_placed_ = ask_retval_;
  int band_level_ = 0;
  int old_tot_buy_placed_ = 0;
  int low_band_px_ = base_iter_bid_px_;
  int current_px_ = base_iter_bid_px_;
  int current_band_ordered_sz_ = 0;
  int current_band_target_sz_ = 0;

  int_px_to_be_placed_at_.clear();
  px_to_be_placed_at_.clear();
  size_to_be_placed_.clear();
  order_level_indicator_vec_.clear();

  while (my_risk_ + tot_buy_placed_ < (int)std::min(t_paramset_->max_position_, t_paramset_->worst_case_position_)) {
    /// first compute the total size shown in this px band
    /// lower limit of px_band
    old_tot_buy_placed_ = tot_buy_placed_;
    if (current_product_->place_at_stable_prices_ &&
        p_dep_market_view_->level_size_thresh_ >
            0)  // || p_dep_market_view_->market_update_info_.compute_stable_levels_ )
    {
      ExecLogicUtils::GetLowBandPx(p_dep_market_view_, low_band_px_, base_iter_bid_px_, kTradeTypeBuy);
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        DBGLOG_TIME_CLASS_FUNC_LINE << " curPrice: " << base_iter_bid_px_ << " low_band_px_ : " << low_band_px_
                                    << DBGLOG_ENDL_FLUSH;
      }
    } else {
      low_band_px_ = base_iter_bid_px_ - t_paramset_->px_band_;
    }

    if (low_band_px_ == kInvalidIntPrice) {
      break;
    }
    current_px_ = base_iter_bid_px_;
    current_band_ordered_sz_ = 0;
    if (current_global_tradevarset_.l1bid_trade_size_ > 0 && band_level_ == 0) {
      current_band_target_sz_ = current_global_tradevarset_.l1bid_trade_size_;
    } else {
      if (current_global_tradevarset_.l1bid_trade_size_ <= 0) {
        current_band_target_sz_ = 0;
      } else if (band_level_ > 0) {
        current_band_target_sz_ = MathUtils::GetFlooredMultipleOf(
            std::min(t_paramset_->max_position_ - my_risk_ - tot_buy_placed_, t_paramset_->unit_trade_size_),
            p_dep_market_view_->min_order_size());
      }
    }

    if (current_band_target_sz_ <= 0) {
      break;
    }
    // int new_low_band_px_ = low_band_px_ ;
    while (current_px_ > low_band_px_) {
      int size_at_this_level_ = order_manager_vec_[_product_index_]->GetTotalBidSizeOrderedAtIntPx(current_px_);
      if (size_at_this_level_ > 0) {
        //  new_low_band_px_ = current_px_ - t_paramset_->px_band_ ;
        current_band_ordered_sz_ += size_at_this_level_;
      }
      current_px_--;
    }
    if (current_product_->place_at_stable_prices_) {
      ExecLogicUtils::CancelOrdersInBand(order_manager_vec_[_product_index_], base_iter_bid_px_, low_band_px_,
                                         current_band_target_sz_, current_band_ordered_sz_, kTradeTypeBuy);
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        DBGLOG_TIME_CLASS_FUNC << " Cancelled " << retval_ << " Band Orders: " << base_iter_bid_px_ << " - "
                               << low_band_px_ + 1 << "\nPosition: " << my_risk_ << '\n';
        order_manager_vec_[_product_index_]->ShowOrderBook();
      }
    }
    // low_band_px_ = new_low_band_px_ ;
    /*
     * This should fix the problem of our orders at difference < px_band_size
     * Though this is most probably make a bound on prices which we quote to be aligned with the prices where we quoted
     * first time
     * which can continue till end
     * Need some handling of when to place fresh orders/not aligned with previous value in book ?
     */

    tot_buy_placed_ += current_band_ordered_sz_;
    if (current_band_ordered_sz_ < current_band_target_sz_ &&
        (p_dep_market_view_->bid_size_at_int_price(base_iter_bid_px_) >= current_product_->size_to_join_)) {
      int_px_to_be_placed_at_.push_back(base_iter_bid_px_);
      px_to_be_placed_at_.push_back(p_dep_market_view_->GetDoublePx(base_iter_bid_px_));
      size_to_be_placed_.push_back(current_band_target_sz_ - current_band_ordered_sz_);
      order_level_indicator_vec_.push_back(GetOrderLevelIndicator(kTradeTypeBuy, base_iter_bid_px_, _product_index_));

      if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        DBGLOG_TIME_CLASS_FUNC << " SendTrade "
                               << " Buy Order at px " << base_iter_bid_px_ << " size "
                               << current_band_target_sz_ - current_band_ordered_sz_ << " Mkt "
                               << best_nonself_bid_price_ << " --- " << best_nonself_ask_price_
                               << "\nTarget Px: " << target_price_
                               << "\nBest_Bid_Place_Cxl_Price: " << best_bid_place_cxl_px_ << "\nPosition: " << my_risk_
                               << '\n';
        order_manager_vec_[_product_index_]->ShowOrderBook();
      }
      tot_buy_placed_ += current_band_target_sz_ - current_band_ordered_sz_;
    }
    if (!(tot_buy_placed_ - old_tot_buy_placed_ > 0)) break;
    base_iter_bid_px_ = low_band_px_;
    band_level_++;
  }

  retval_ = order_manager_vec_[_product_index_]->CancelReplaceBidOrdersEqAboveAndEqBelowIntPrice(
      bid_cancel_px_ + 1, base_iter_bid_px_, px_to_be_placed_at_, int_px_to_be_placed_at_, size_to_be_placed_,
      order_level_indicator_vec_, kOrderDay);
  if (dbglogger_.CheckLoggingLevel(TRADING_INFO) && retval_ >= 0) {
    DBGLOG_TIME_CLASS_FUNC << " modified " << retval_ << " bid orders " << DBGLOG_ENDL_FLUSH;
  }

  /// Symetric treatment for ASK side
  tot_sell_placed_ = ask_retval_;
  band_level_ = 0;
  int old_tot_sell_placed_ = 0;

  int_px_to_be_placed_at_.clear();
  px_to_be_placed_at_.clear();
  size_to_be_placed_.clear();
  order_level_indicator_vec_.clear();

  while (my_risk_ - tot_sell_placed_ + (int)std::min(t_paramset_->max_position_, t_paramset_->worst_case_position_) >
         0) {
    /// first compute the total size shown in this px band
    /// lower limit of px_band
    old_tot_sell_placed_ = tot_sell_placed_;
    if (current_product_->place_at_stable_prices_ && p_dep_market_view_->level_size_thresh_ > 0) {
      ExecLogicUtils::GetLowBandPx(p_dep_market_view_, low_band_px_, base_iter_ask_px_, kTradeTypeSell);
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        DBGLOG_TIME_CLASS_FUNC_LINE << " curPrice: " << low_band_px_ << " low_band_px_ : " << base_iter_ask_px_
                                    << DBGLOG_ENDL_FLUSH;
      }
    } else {
      low_band_px_ = base_iter_ask_px_ + t_paramset_->px_band_;
    }
    if (low_band_px_ == kInvalidIntPrice) {
      break;
    }
    current_px_ = base_iter_ask_px_;
    current_band_ordered_sz_ = 0;
    if (current_global_tradevarset_.l1ask_trade_size_ > 0 && band_level_ == 0) {
      current_band_target_sz_ = current_global_tradevarset_.l1ask_trade_size_;
    } else {
      if (current_global_tradevarset_.l1ask_trade_size_ <= 0) {
        // not allowing to place trades
        current_band_target_sz_ = 0;
      } else if (band_level_ > 0) {
        current_band_target_sz_ = MathUtils::GetFlooredMultipleOf(
            std::min(t_paramset_->max_position_ + my_risk_ - tot_sell_placed_, t_paramset_->unit_trade_size_),
            p_dep_market_view_->min_order_size());
      }
    }

    if (current_band_target_sz_ <= 0) {
      break;
    }
    // int new_low_band_px_ = low_band_px_ ;
    while (current_px_ < low_band_px_) {
      int size_at_this_level_ = order_manager_vec_[_product_index_]->GetTotalAskSizeOrderedAtIntPx(current_px_);
      if (size_at_this_level_ > 0) {
        current_band_ordered_sz_ += order_manager_vec_[_product_index_]->GetTotalAskSizeOrderedAtIntPx(current_px_);
        // new_low_band_px_ = current_px_ + t_paramset_->px_band_ ;
      }
      current_px_++;
    }

    if (current_product_->place_at_stable_prices_) {
      ExecLogicUtils::CancelOrdersInBand(order_manager_vec_[_product_index_], base_iter_ask_px_, low_band_px_,
                                         current_band_target_sz_, current_band_ordered_sz_, kTradeTypeSell);
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        DBGLOG_TIME_CLASS_FUNC << " Cancelled " << retval_ << " Band Orders: " << base_iter_ask_px_ << " - "
                               << low_band_px_ + 1 << "\nPosition: " << my_risk_ << '\n';
        order_manager_vec_[_product_index_]->ShowOrderBook();
      }
    }
    // low_band_px_ = new_low_band_px_ ;

    tot_sell_placed_ += current_band_ordered_sz_;
    if (current_band_ordered_sz_ < current_band_target_sz_ &&
        (p_dep_market_view_->ask_size_at_int_price(base_iter_ask_px_) >= current_product_->size_to_join_)) {
      px_to_be_placed_at_.push_back(p_dep_market_view_->GetDoublePx(base_iter_ask_px_));
      int_px_to_be_placed_at_.push_back(base_iter_ask_px_);
      size_to_be_placed_.push_back(current_band_target_sz_ - current_band_ordered_sz_);
      order_level_indicator_vec_.push_back(GetOrderLevelIndicator(kTradeTypeSell, base_iter_ask_px_, _product_index_));

      if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        DBGLOG_TIME_CLASS_FUNC << " SendTrade "
                               << " Sell Order at px " << base_iter_ask_px_ << " size "
                               << current_band_target_sz_ - current_band_ordered_sz_ << " Mkt "
                               << best_nonself_bid_price_ << " --- " << best_nonself_ask_price_
                               << "\nTarget_Px: " << target_price_
                               << "\nBest_Ask_Place_Cxl_Px: " << best_ask_place_cxl_px_ << "\nPosition: " << my_risk_
                               << '\n';
        order_manager_vec_[_product_index_]->ShowOrderBook();
      }
      tot_sell_placed_ += current_band_target_sz_ - current_band_ordered_sz_;
    }
    if (!(tot_sell_placed_ - old_tot_sell_placed_ > 0)) break;
    base_iter_ask_px_ = low_band_px_;
    band_level_++;
  }

  retval_ = order_manager_vec_[_product_index_]->CancelReplaceAskOrdersEqAboveAndEqBelowIntPrice(
      ask_cancel_px_ - 1, base_iter_ask_px_, px_to_be_placed_at_, int_px_to_be_placed_at_, size_to_be_placed_,
      order_level_indicator_vec_, kOrderDay);

  // cancel asks above this
  // retval_ = order_manager_vec_[_product_index_]->CancelAsksEqBelowIntPrice ( base_iter_ask_px_ );

  if (dbglogger_.CheckLoggingLevel(TRADING_INFO) && retval_ > 0) {
    DBGLOG_TIME_CLASS_FUNC << " modifid " << retval_ << " ask orders \n";
    order_manager_vec_[_product_index_]->ShowOrderBook();
  }
}

char EquityTrading2::GetOrderLevelIndicator(TradeType_t order_side, int int_order_px, int _product_index_) {
  if (order_side == kTradeTypeBuy) {
    if (int_order_px >= current_product_->best_nonself_ask_int_price_) {
      /*
       * Should keep two differnt variables for aggress
       * 1- We sent aggres order
       * 2- When we received aggressive fill
       */
      current_product_->last_bid_agg_msecs_ = watch_.msecs_from_midnight();
      current_product_->last_agg_buy_msecs_ = watch_.msecs_from_midnight();
      return 'A';
    } else if (int_order_px > current_product_->best_nonself_bid_int_price_) {
      current_product_->last_bid_imp_msecs_ = watch_.msecs_from_midnight();
      return 'I';
    } else if (int_order_px == current_product_->best_nonself_bid_int_price_) {
      return 'B';
    } else
      return 'S';
  } else {
    if (int_order_px <= current_product_->best_nonself_bid_int_price_) {
      current_product_->last_ask_agg_msecs_ = watch_.msecs_from_midnight();
      current_product_->last_agg_sell_msecs_ = watch_.msecs_from_midnight();
      return 'A';
    } else if (int_order_px < current_product_->best_nonself_ask_int_price_) {
      current_product_->last_ask_imp_msecs_ = watch_.msecs_from_midnight();
      return 'I';
    } else if (int_order_px == current_product_->best_nonself_ask_int_price_) {
      return 'B';
    } else
      return 'S';
  }
}

void EquityTrading2::OnExec(const int t_new_position_, const int _exec_quantity_, const TradeType_t _buysell_,
                            const double _price_, const int r_int_price_, const int _security_id_) {
  int index_ = sec_id_to_index_[_security_id_];
  current_product_ = product_vec_[index_];

  if (t_new_position_ > current_product_->position_) {
    current_product_->last_buy_msecs_ = watch_.msecs_from_midnight();
    current_product_->last_buy_int_price_ = r_int_price_;
  } else if (t_new_position_ < current_product_->position_) {
    current_product_->last_sell_msecs_ = watch_.msecs_from_midnight();
    current_product_->last_sell_int_price_ = r_int_price_;
  }
}

void EquityTrading2::OnCancelReject(const TradeType_t _buysell_, const double _price_, const int r_int_price_,
                                    const int _security_id_) {
  int index_ = sec_id_to_index_[_security_id_];
  current_product_ = product_vec_[index_];
  if (_buysell_ == kTradeTypeBuy) {
    current_product_->last_buy_msecs_ = watch_.msecs_from_midnight();
    current_product_->last_buy_int_price_ = r_int_price_;
  } else {
    current_product_->last_sell_msecs_ = watch_.msecs_from_midnight();
    current_product_->last_sell_int_price_ = r_int_price_;
  }
}

bool EquityTrading2::UpdateTarget(double _target_price_, double _targetbias_numbers_, int _modelmath_index_) {
  if (!initialized_) {
    Initialize();
  }
  current_product_ = product_vec_[_modelmath_index_];
  if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
    DBGLOG_CLASS_FUNC_LINE << " tgtpr: " << _target_price_ << " tgtbias: " << _targetbias_numbers_
                           << " mmi : " << _modelmath_index_ << " ir: " << is_ready_ << " "
                           << current_product_->trading_start_utc_mfm_ << DBGLOG_ENDL_FLUSH;
  }

  if (!is_ready_vec_[_modelmath_index_]) {
    bool t_is_ready_ = true;
    t_is_ready_ = dep_market_view_vec_[_modelmath_index_]->is_ready() && model_math_vec_[_modelmath_index_]->is_ready();

    if ((watch_.msecs_from_midnight() > current_product_->trading_start_utc_mfm_ &&
         watch_.YYYYMMDD() == tradingdate_) &&
        t_is_ready_ && ((_target_price_ >= dep_market_view_vec_[_modelmath_index_]->bestbid_price()) &&
                        (_target_price_ <= dep_market_view_vec_[_modelmath_index_]->bestask_price()))) {
      // Now it's the right time to get CPU
      if (livetrading_ && !cpu_allocated_) {
        AllocateCPU();
        cpu_allocated_ = true;
      }
      is_ready_vec_[_modelmath_index_] = true;

      DBGLOG_TIME_CLASS_FUNC << "SACI: " << order_manager_vec_[_modelmath_index_]->server_assigned_client_id_
                             << " got ready! shc: " << dep_market_view_vec_[_modelmath_index_]->shortcode()
                             << DBGLOG_ENDL_FLUSH;
      model_math_vec_[_modelmath_index_]->ShowIndicatorValues();
      TradeVarSetLogic(_modelmath_index_);
    }
  } else {
    target_price_vec_[_modelmath_index_] = _target_price_;
    targetbias_numbers_vec_[_modelmath_index_] = _targetbias_numbers_;

    if ((!external_freeze_trading_) && (!freeze_due_to_exchange_stage_) && (!freeze_due_to_funds_reject_)) {
      ProcessGetFlat();

      if ((!should_be_getting_flat_) && !should_be_getting_flat_vec_[_modelmath_index_] &&
          current_product_->is_trading_today_ &&
          (current_product_->throttle_manager_->allowed_through_throttle(watch_.msecs_from_midnight()))) {
        // TradingLogic ( _modelmath_index_ ) ;
        if (order_placing_logic_ == kPriceBasedVolTrading) {
          TradingLogic(_modelmath_index_);
        } else if (order_placing_logic_ == kDirectionalAggressiveTrading) {
          DatTradingLogic(_modelmath_index_);
        } else if (order_placing_logic_ == kPriceBasedAggressiveTrading) {
          PbatTradingLogic(_modelmath_index_);
        } else {
          TradingLogic(_modelmath_index_);
        }
        CallPlaceCancelNonBestLevels(_modelmath_index_);
      } else if (should_be_getting_flat_) {
        // Getflat on all products
        for (int i = 0; i < total_products_trading_; i++) {
          GetFlatTradingLogic(i);
        }
        if (livetrading_ || (dbglogger_.CheckLoggingLevel(TRADING_INFO))) {
          LogFullStatus();
        }
      } else if (should_be_getting_flat_vec_[_modelmath_index_] || !current_product_->is_trading_today_) {
        // GetFlatOnIndividualProducts
        GetFlatTradingLogic(_modelmath_index_);
        if (livetrading_ || (dbglogger_.CheckLoggingLevel(TRADING_INFO))) {
          LogFullStatus();
        }
      }
    }
  }

  if (dump_inds) {
    dump_inds = false;

    if (zero_logging_mode_ && !override_zero_logging_mode_once_for_external_cmd_) {
      return false;
    }

    // only override on external command once
    if (override_zero_logging_mode_once_for_external_cmd_) {
      override_zero_logging_mode_once_for_external_cmd_ = false;
    }

    if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {  // Only dump indicator values if this is set
      DBGLOG_TIME << " mkt: " << best_nonself_bid_size_ << " @ " << best_nonself_bid_price_ << " X "
                  << best_nonself_ask_price_ << " @ " << best_nonself_ask_size_ << " tgt: " << target_price_
                  << " tgtnum: " << targetbias_numbers_ << DBGLOG_ENDL_FLUSH;
      return true;
    }
  }

  if (dbglogger_.CheckLoggingLevel(TRADEINIT_INFO)) {  // print book to screen
    ShowBook();
  }

  return false;
}

void EquityTrading2::OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {
  if (!initialized_) {
    Initialize();
  }

  if (dbglogger_.CheckLoggingLevel(OLSMM_INFO)) {
    int index_ = sec_id_to_index_[_security_id_];
    DBGLOG_TIME_CLASS_FUNC << "Main_mkt\n";
    for (int level_ = 0; level_ < 5; level_++) {
      dbglogger_ << "[ " << dep_market_view_vec_[index_]->bid_order(level_) << " "
                 << dep_market_view_vec_[index_]->bid_size(level_) << " "
                 << dep_market_view_vec_[index_]->bid_price(level_) << " "
                 << dep_market_view_vec_[index_]->bid_int_price(level_) << " * "
                 << dep_market_view_vec_[index_]->ask_int_price(level_) << " "
                 << dep_market_view_vec_[index_]->ask_price(level_) << " "
                 << dep_market_view_vec_[index_]->ask_size(level_) << " "
                 << dep_market_view_vec_[index_]->ask_order(level_) << " ] " << DBGLOG_ENDL_FLUSH;
    }
  }
  NonSelfMarketUpdate(sec_id_to_index_[_security_id_]);
}

void EquityTrading2::OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                                  const MarketUpdateInfo& _market_update_info_) {
  current_product_ = product_vec_[sec_id_to_index_[_security_id_]];
  NonSelfMarketUpdate(sec_id_to_index_[_security_id_]);
  // check opentrade loss
  if (open_unrealized_pnl_ < -max_opentrade_loss_) {
    if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
      DBGLOG_TIME << "getflat_due_to_max_opentrade_loss_ ! Current opentradepnl : "
                  << order_manager_.base_pnl().opentrade_unrealized_pnl() << " < " << -max_opentrade_loss_
                  << " Strategy: " << runtime_id_ << DBGLOG_ENDL_FLUSH;
      if (livetrading_) {
        DBGLOG_DUMP;
      }
      if (livetrading_ &&  // live-trading and within trading window
          (watch_.msecs_from_midnight() > current_product_->trading_start_utc_mfm_) &&
          (watch_.msecs_from_midnight() < current_product_->trading_end_utc_mfm_)) {
        char hostname_[128];
        hostname_[127] = '\0';
        gethostname(hostname_, 127);
        std::string getflat_email_string_ = "";
        {
          std::ostringstream t_oss_;
          t_oss_ << "Strategy: " << runtime_id_ << " getflat_due_to_max_opentrade_loss_: " << open_unrealized_pnl_
                 << " on " << hostname_ << "\n";
          getflat_email_string_ = t_oss_.str();
        }

        SendMail(getflat_email_string_, getflat_email_string_);
      }
    }
    getflat_due_to_max_opentrade_loss_ = true;
    last_getflat_due_to_max_opentrade_loss_triggered_at_msecs_ = watch_.msecs_from_midnight();
    num_opentrade_loss_hits_++;
  }
}

void EquityTrading2::NonSelfMarketUpdate(int _product_index_) {
  if (_product_index_ >= total_products_trading_) {
    return;
  }
  current_product_ = product_vec_[_product_index_];

  if (prod_paramset_vec_[_product_index_]->use_stable_bidask_levels_) {
    current_product_->best_nonself_bid_price_ = dep_market_view_vec_[_product_index_]->GetDoublePx(
        dep_market_view_vec_[_product_index_]->bid_side_valid_level_int_price_);
    current_product_->best_nonself_bid_int_price_ =
        dep_market_view_vec_[_product_index_]->bid_side_valid_level_int_price_;
    current_product_->best_nonself_bid_size_ = dep_market_view_vec_[_product_index_]->bid_side_valid_level_size_;

    current_product_->best_nonself_ask_price_ = dep_market_view_vec_[_product_index_]->GetDoublePx(
        dep_market_view_vec_[_product_index_]->ask_side_valid_level_int_price_);
    current_product_->best_nonself_ask_int_price_ =
        dep_market_view_vec_[_product_index_]->ask_side_valid_level_int_price_;
    current_product_->best_nonself_ask_size_ = dep_market_view_vec_[_product_index_]->bid_side_valid_level_size_;
  } else {
    current_product_->best_nonself_bid_price_ = dep_market_view_vec_[_product_index_]->bestbid_price();
    current_product_->best_nonself_bid_int_price_ = dep_market_view_vec_[_product_index_]->bestbid_int_price();
    current_product_->best_nonself_bid_size_ = dep_market_view_vec_[_product_index_]->bestbid_size();

    current_product_->best_nonself_ask_price_ = dep_market_view_vec_[_product_index_]->bestask_price();
    current_product_->best_nonself_ask_int_price_ = dep_market_view_vec_[_product_index_]->bestask_int_price();
    current_product_->best_nonself_ask_size_ = dep_market_view_vec_[_product_index_]->bestask_size();
  }

  current_product_->spread_ = current_product_->best_nonself_ask_price_ - current_product_->best_nonself_bid_price_;
  // This is meant to decrease MUR based on volumes
  UpdateVolumeAdjustedMaxPos(_product_index_);
  auto& t_current_global_tradevarset_ = current_global_tradevarset_vec_[_product_index_];
  auto t_my_risk_ = (int)current_product_->beta_adjusted_position_;

  if (prod_paramset_vec_[_product_index_]->read_scale_max_pos_) {
    if ((t_current_global_tradevarset_.l1bid_trade_size_ + t_my_risk_ > volume_adj_max_pos_vec_[_product_index_]) &&
        (t_my_risk_ >= 0)) {  // Query is too long
      t_current_global_tradevarset_.l1bid_trade_size_ =
          std::max(0, volume_adj_max_pos_vec_[_product_index_] - t_my_risk_);
    } else {
      if ((t_current_global_tradevarset_.l1ask_trade_size_ - t_my_risk_ > volume_adj_max_pos_vec_[_product_index_]) &&
          (t_my_risk_ <= 0)) {  // Query is too short
        t_current_global_tradevarset_.l1ask_trade_size_ =
            std::max(0, volume_adj_max_pos_vec_[_product_index_] + t_my_risk_);
      }
    }
  }
}

void EquityTrading2::OnMarketStatusChange(const unsigned int _security_id_, const MktStatus_t _new_market_status_) {
  if (!initialized_) {
    Initialize();
  }
  int _product_index_ = sec_id_to_index_[_security_id_];
  current_product_ = product_vec_[_product_index_];

  DBGLOG_TIME_CLASS_FUNC_LINE << "ForProduct: " << products_being_traded_[_product_index_]
                              << " Trading Status Changed from: "
                              << MktTradingStatusStr(current_product_->trading_status_)
                              << " To: " << MktTradingStatusStr(_new_market_status_) << DBGLOG_ENDL_FLUSH;

  switch (_new_market_status_) {
    case kMktTradingStatusOpen: {
      current_product_->getflat_due_to_market_status_ = false;
      current_product_->allowed_to_cancel_orders_ = true;
      if (livetrading_ && (watch_.msecs_from_midnight() > current_product_->trading_start_utc_mfm_ &&
                           watch_.YYYYMMDD() == tradingdate_) &&
          (current_product_->trading_status_ == kMktTradingStatusClosed ||
           current_product_->trading_status_ == kMktTradingStatusForbidden ||
           current_product_->trading_status_ == kMktTradingStatusClosed ||
           current_product_->trading_status_ == kMktTradingStatusPreOpen ||
           current_product_->trading_status_ == kMktTradingStatusReserved)) {
        char hostname_[128];
        hostname_[127] = '\0';
        gethostname(hostname_, 127);
        std::string getflat_email_string_ = "";
        {
          std::ostringstream t_oss_;
          t_oss_ << "Strategy: " << runtime_id_ << " For Product: " << products_being_traded_[_product_index_]
                 << " Trading status changed from : " << MktTradingStatusStr(current_product_->trading_status_)
                 << " to: " << MktTradingStatusStr(_new_market_status_) << " at : " << watch_.NYTimeString() << ", "
                 << watch_.UTCTimeString() << " UTC"
                 << " CurrentlyTrading : " << (is_ready_ ? "YES" : "NO") << " on " << hostname_ << "\n";
          getflat_email_string_ = t_oss_.str();
        }

        std::ostringstream st_;
        st_ << "Strategy: " << runtime_id_ << " For Product - " << products_being_traded_[_product_index_]
            << " Market Status: " << MktTradingStatusStr(_new_market_status_) << " at : " << watch_.NYTimeString()
            << ", " << watch_.UTCTimeString() << " UTC" << std::endl;

        std::string getflat_email_subject_ = st_.str();
        SendMail(getflat_email_string_, getflat_email_subject_);
      }
      current_product_->allowed_to_cancel_orders_ = true;
    } break;
    case kMktTradingStatusClosed: {
      current_product_->getflat_due_to_market_status_ = true;
      current_product_->allowed_to_cancel_orders_ = false;
    } break;
    case kMktTradingStatusPreOpen:
    case kMktTradingStatusReserved: {
      if (livetrading_ && watch_.msecs_from_midnight() > current_product_->trading_start_utc_mfm_ &&
          watch_.YYYYMMDD() == tradingdate_) {
        char hostname_[128];
        hostname_[127] = '\0';
        gethostname(hostname_, 127);
        std::string getflat_email_string_ = "";
        {
          std::ostringstream t_oss_;
          t_oss_ << "Strategy: " << runtime_id_ << " For Product: " << products_being_traded_[_product_index_]
                 << " Trading status changed from : " << MktTradingStatusStr(current_product_->trading_status_)
                 << " to: " << MktTradingStatusStr(_new_market_status_) << " at : " << watch_.NYTimeString() << ", "
                 << watch_.UTCTimeString() << " UTC"
                 << " CurrentlyTrading : " << (is_ready_ ? "YES" : "NO") << " on " << hostname_ << "\n";
          getflat_email_string_ = t_oss_.str();
        }

        std::ostringstream st_;
        st_ << "Strategy: " << runtime_id_ << " For Product - " << products_being_traded_[_product_index_]
            << " Market Status: " << MktTradingStatusStr(_new_market_status_) << " at : " << watch_.NYTimeString()
            << ", " << watch_.UTCTimeString() << " UTC" << std::endl;
        std::string getflat_email_subject_ = st_.str();
        SendMail(getflat_email_string_, getflat_email_subject_);
      }
      current_product_->allowed_to_cancel_orders_ = true;
      current_product_->getflat_due_to_market_status_ = true;
    } break;
    case kMktTradingStatuFinalClosingCall:
    case kMktTradingStatusUnknown:
    case kMktTradingStatusForbidden:
    default: { break; }
  }
  current_product_->trading_status_ = _new_market_status_;
  ProcessGetFlat();
}
void EquityTrading2::OnStdevUpdate(const unsigned int security_id_, const double& _new_stdev_val_) {
  product_vec_[sec_id_to_index_[security_id_]]->m_stdev_ = (common_paramset_.stdev_fact_ * _new_stdev_val_);

  /*
   * m_stdev_vec_[ sec_id_to_index_ [security_id_ ] ] = std::min ( common_paramset_.stdev_fact_ * _new_stdev_val_ ,
   * prod_paramset_vec_[sec_id_to_index_[security_id_]]->stdev_cap_ / dep_market_view_vec_[ sec_id_to_index_[
   * security_id_ ]]->min_price_increment() );
   */

  num_stdev_calls_++;
  sum_stdev_calls_ += _new_stdev_val_;
}

void EquityTrading2::OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_) {
  if (_indicator_index_ < (unsigned)total_products_trading_) {
    product_vec_[_indicator_index_]->moving_avg_spread_ = _new_value_;
  } else if (_indicator_index_ < (unsigned)2 * total_products_trading_) {
    product_vec_[_indicator_index_ - total_products_trading_]->returns_ = _new_value_;
  } else if (_indicator_index_ == (unsigned)2 * total_products_trading_) {
    index_returns_ = _new_value_;
  } else if (_indicator_index_ < (unsigned)2 * total_products_trading_ + 1 + sector_returns_.size()) {
    sector_returns_[_indicator_index_ - 1 - 2 * total_products_trading_] = _new_value_;
  }
}

void EquityTrading2::OnVolumeRatioUpdate(const unsigned int _security_index_, const double& r_new_volume_ratio_) {
  if (_security_index_ == 0) {
    // assuming index is only source other than self
    index_volume_ratio_ = r_new_volume_ratio_;
    index_volume_ratio_ = 0;
    return;
  }

  int product_index_ = _security_index_ - 1;
  current_product_ = product_vec_[product_index_];

  if (!initialized_) {
    Initialize();
  }
  if (r_new_volume_ratio_ > current_product_->volume_ratio_stop_trading_upper_threshold_) {
    should_be_getting_flat_vec_[product_index_] = true;

    if (current_product_->is_trading_today_ && dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
      DBGLOG_TIME_CLASS_FUNC_LINE << " Stopped trading product : " << products_being_traded_[product_index_]
                                  << " due to high volumes. VR: " << r_new_volume_ratio_
                                  << " thres: " << current_product_->volume_ratio_stop_trading_lower_threshold_ << " "
                                  << current_product_->volume_ratio_stop_trading_upper_threshold_ << " "
                                  << index_volume_ratio_ << DBGLOG_ENDL_FLUSH;
    }
    current_product_->is_trading_today_ = false;
  } else if (r_new_volume_ratio_ < current_product_->volume_ratio_stop_trading_lower_threshold_) {
    should_be_getting_flat_vec_[product_index_] = true;

    if (current_product_->is_trading_today_ && dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
      DBGLOG_TIME_CLASS_FUNC_LINE << " Stopped trading product : " << products_being_traded_[product_index_]
                                  << " due to low volumes. VR: " << r_new_volume_ratio_
                                  << " thres: " << current_product_->volume_ratio_stop_trading_lower_threshold_ << " "
                                  << current_product_->volume_ratio_stop_trading_upper_threshold_ << " "
                                  << index_volume_ratio_ << DBGLOG_ENDL_FLUSH;
    }
    current_product_->is_trading_today_ = false;
  } else {
    if (!current_product_->is_trading_today_) {
      should_be_getting_flat_vec_[product_index_] = false;
      current_product_->is_trading_today_ = true;
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME_CLASS_FUNC_LINE << " Resumed trading product : " << products_being_traded_[product_index_]
                                    << " volume ratio is within range. VR: " << r_new_volume_ratio_
                                    << " thres: " << current_product_->volume_ratio_stop_trading_lower_threshold_ << " "
                                    << current_product_->volume_ratio_stop_trading_upper_threshold_ << " "
                                    << index_volume_ratio_ << DBGLOG_ENDL_FLUSH;
      }
    }
  }
}

void EquityTrading2::GetFlatTradingLogic(int _product_index_) {
  current_product_ = product_vec_[_product_index_];

  if (current_product_->getflat_mult_order_)
    MultOrderGetFlatTradingLogic(_product_index_);
  else if (current_product_->getflat_aggressive_)
    AggressiveGetFlatTradingLogic(_product_index_);
  else if (livetrading_ && (current_product_->trading_status_ == kMktTradingStatusPreOpen ||
                            current_product_->trading_status_ == kMktTradingStatusReserved)) {
    if (current_product_->theoretical_price_ > 0) {
      UnusualTradingStatusGetFlatTradingLogic(_product_index_);
    } else {
      NormalGetFlatTradingLogic(_product_index_);
    }
  } else
    NormalGetFlatTradingLogic(_product_index_);
}

void EquityTrading2::NormalGetFlatTradingLogic(int _product_index_) {
  current_product_ = product_vec_[_product_index_];

  if (!current_product_->allowed_to_cancel_orders_) {
    return;
  }

  int t_position_ = current_product_->position_;
  if (t_position_ == 0) {  // nothing to be done, cancel all remaining orders
    order_manager_vec_[_product_index_]->CancelAllOrders();
  } else if (t_position_ > 0) {  // long hence cancel all bid orders
    order_manager_vec_[_product_index_]->CancelAllBidOrders();
    // cancel all non active (best_level) sell orders
    order_manager_vec_[_product_index_]->CancelAsksBelowIntPrice(current_product_->best_nonself_ask_int_price_);
    // ideal size to place ask for is what the position is, except limited to unit_trade_size_ ( for sanity and market
    // effect reasons )
    int t_size_ordered_ = order_manager_vec_[_product_index_]->SumAskSizeConfirmedEqAboveIntPrice(
                              current_product_->best_nonself_ask_int_price_) +
                          order_manager_vec_[_product_index_]->SumAskSizeUnconfirmedEqAboveIntPrice(
                              current_product_->best_nonself_ask_int_price_);
    int trade_size_required_ =
        MathUtils::GetFlooredMultipleOf(std::min(t_position_, prod_paramset_vec_[_product_index_]->unit_trade_size_),
                                        dep_market_view_vec_[_product_index_]->min_order_size());
    if (t_size_ordered_ < trade_size_required_) {  // if the required size is not present then add more
      order_manager_vec_[_product_index_]->SendTrade(current_product_->best_nonself_ask_price_,
                                                     current_product_->best_nonself_ask_int_price_,
                                                     trade_size_required_ - t_size_ordered_, kTradeTypeSell, 'B');

      if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        DBGLOG_TIME_CLASS_FUNC << "GetFlatSendTrade S of " << trade_size_required_ - t_size_ordered_ << " @ "
                               << best_nonself_ask_price_ << " IntPx: " << current_product_->best_nonself_ask_int_price_
                               << " mkt: " << current_product_->best_nonself_bid_size_ << " @ "
                               << current_product_->best_nonself_bid_price_ << " X "
                               << current_product_->best_nonself_ask_price_ << " @ "
                               << current_product_->best_nonself_ask_size_ << DBGLOG_ENDL_FLUSH;
      }
    }
  } else {
    // short hence cancel all sell orders
    order_manager_vec_[_product_index_]->CancelAllAskOrders();
    // cancel all non bestlevel bid orders
    order_manager_vec_[_product_index_]->CancelBidsBelowIntPrice(current_product_->best_nonself_bid_int_price_);
    // ideal size to place ask for is what the position is, except limited to unit_trade_size_ ( for sanity and market
    // effect reasons )
    int t_size_ordered_ = order_manager_vec_[_product_index_]->SumBidSizeConfirmedEqAboveIntPrice(
                              current_product_->best_nonself_bid_int_price_) +
                          order_manager_vec_[_product_index_]->SumBidSizeUnconfirmedEqAboveIntPrice(
                              current_product_->best_nonself_bid_int_price_);
    int trade_size_required_ =
        MathUtils::GetFlooredMultipleOf(std::min(-t_position_, prod_paramset_vec_[_product_index_]->unit_trade_size_),
                                        dep_market_view_vec_[_product_index_]->min_order_size());
    if (t_size_ordered_ < trade_size_required_) {  // if the required size is not present then add more
      order_manager_vec_[_product_index_]->SendTrade(current_product_->best_nonself_bid_price_,
                                                     current_product_->best_nonself_bid_int_price_,
                                                     trade_size_required_ - t_size_ordered_, kTradeTypeBuy, 'B');

      if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        DBGLOG_TIME_CLASS_FUNC << "GetFlatSendTrade B of " << trade_size_required_ - t_size_ordered_ << " @ "
                               << current_product_->best_nonself_bid_price_
                               << " IntPx: " << current_product_->best_nonself_bid_int_price_
                               << " mkt: " << best_nonself_bid_size_ << " @ "
                               << current_product_->best_nonself_bid_price_ << " X "
                               << current_product_->best_nonself_ask_int_price_ << " @ "
                               << current_product_->best_nonself_ask_size_ << DBGLOG_ENDL_FLUSH;
      }
    }
  }
}

void EquityTrading2::AggressiveGetFlatTradingLogic(int _product_index_) {
  current_product_ = product_vec_[_product_index_];

  if (!current_product_->allowed_to_cancel_orders_) {
    return;
  }

  int t_position_ = current_product_->position_;
  if (t_position_ == 0) {  // nothing to be done, cancel all remaining orders
    order_manager_vec_[_product_index_]->CancelAllOrders();
  } else if (t_position_ > 0) {  // long hence cancel all bid orders
    order_manager_vec_[_product_index_]->CancelAllBidOrders();
    // cancel all non active (best_level) sell orders
    order_manager_vec_[_product_index_]->CancelAsksBelowIntPrice(current_product_->best_nonself_bid_int_price_);
    // ideal size to place ask for is what the position is, except limited to unit_trade_size_ ( for sanity and market
    // effect reasons )
    int t_size_ordered_ = order_manager_vec_[_product_index_]->SumAskSizeConfirmedEqAboveIntPrice(
                              current_product_->best_nonself_bid_int_price_) +
                          order_manager_vec_[_product_index_]->SumAskSizeUnconfirmedEqAboveIntPrice(
                              current_product_->best_nonself_bid_int_price_);
    int trade_size_required_ =
        MathUtils::GetFlooredMultipleOf(std::min(t_position_, prod_paramset_vec_[_product_index_]->unit_trade_size_),
                                        dep_market_view_vec_[_product_index_]->min_order_size());
    if (t_size_ordered_ < trade_size_required_) {  // if the required size is not present then add more
      order_manager_vec_[_product_index_]->SendTrade(current_product_->best_nonself_bid_price_,
                                                     current_product_->best_nonself_bid_int_price_,
                                                     trade_size_required_ - t_size_ordered_, kTradeTypeSell, 'A');

      if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        DBGLOG_TIME_CLASS_FUNC << "GetFlatSendTrade S of " << trade_size_required_ - t_size_ordered_ << " @ "
                               << best_nonself_bid_price_ << " IntPx: " << current_product_->best_nonself_bid_int_price_
                               << " mkt: " << current_product_->best_nonself_bid_size_ << " @ "
                               << current_product_->best_nonself_bid_price_ << " X "
                               << current_product_->best_nonself_ask_price_ << " @ "
                               << current_product_->best_nonself_ask_size_ << DBGLOG_ENDL_FLUSH;
      }
    }
  } else {
    order_manager_vec_[_product_index_]->CancelAllAskOrders();  // short hence cancel all sell orders
    order_manager_vec_[_product_index_]->CancelBidsBelowIntPrice(
        current_product_->best_nonself_ask_int_price_);  // cancel all non bestlevel bid orders
    // ideal size to place ask for is what the position is, except limited to unit_trade_size_ ( for sanity and market
    // effect reasons )
    int t_size_ordered_ = order_manager_vec_[_product_index_]->SumBidSizeConfirmedEqAboveIntPrice(
                              current_product_->best_nonself_ask_int_price_) +
                          order_manager_vec_[_product_index_]->SumBidSizeUnconfirmedEqAboveIntPrice(
                              current_product_->best_nonself_ask_int_price_);
    int trade_size_required_ =
        MathUtils::GetFlooredMultipleOf(std::min(-t_position_, prod_paramset_vec_[_product_index_]->unit_trade_size_),
                                        dep_market_view_vec_[_product_index_]->min_order_size());
    if (t_size_ordered_ < trade_size_required_) {  // if the required size is not present then add more
      order_manager_vec_[_product_index_]->SendTrade(current_product_->best_nonself_ask_price_,
                                                     current_product_->best_nonself_ask_int_price_,
                                                     trade_size_required_ - t_size_ordered_, kTradeTypeBuy, 'A');

      if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        DBGLOG_TIME_CLASS_FUNC << "GetFlatSendTrade B of " << trade_size_required_ - t_size_ordered_ << " @ "
                               << current_product_->best_nonself_bid_price_
                               << " IntPx: " << current_product_->best_nonself_bid_int_price_
                               << " mkt: " << best_nonself_bid_size_ << " @ "
                               << current_product_->best_nonself_bid_price_ << " X "
                               << current_product_->best_nonself_ask_price_ << " @ "
                               << current_product_->best_nonself_ask_size_ << DBGLOG_ENDL_FLUSH;
      }
    }
  }
}

void EquityTrading2::MultOrderGetFlatTradingLogic(int _product_index_) {
  // Currently getting flat individually
  current_product_ = product_vec_[_product_index_];

  if (!current_product_->allowed_to_cancel_orders_) {
    return;
  }

  int t_position_ = current_product_->position_;
  if (t_position_ == 0) {  // nothing to be done, cancel all remaining orders
    order_manager_vec_[_product_index_]->CancelAllOrders();
  } else if (t_position_ > 0) {  // long hence cancel all bid orders
    order_manager_vec_[_product_index_]->CancelAllBidOrders();
    // cancel all non active (best_level) sell orders
    order_manager_vec_[_product_index_]->CancelAsksBelowIntPrice(current_product_->best_nonself_ask_int_price_);
    {
      // ideal size to place ask for is what the position is, except limited to unit_trade_size_ ( for sanity and market
      // effect reasons )
      int t_size_ordered_ = order_manager_vec_[_product_index_]->SumAskSizeConfirmedEqAboveIntPrice(
                                current_product_->best_nonself_ask_int_price_) +
                            order_manager_vec_[_product_index_]->SumAskSizeUnconfirmedEqAboveIntPrice(
                                current_product_->best_nonself_ask_int_price_);
      // getflat by placing multiple orders
      int total_order_placed_ = 0;
      if (t_size_ordered_ < t_position_) {
        while (total_order_placed_ < t_position_ - t_size_ordered_ &&
               (total_order_placed_ + t_size_ordered_ <
                max_orders_ * prod_paramset_vec_[_product_index_]->unit_trade_size_)) {
          int this_trade_size_ =
              MathUtils::GetFlooredMultipleOf(std::min(t_position_ - t_size_ordered_ - total_order_placed_,
                                                       prod_paramset_vec_[_product_index_]->unit_trade_size_),
                                              dep_market_view_vec_[_product_index_]->min_order_size());

          order_manager_vec_[_product_index_]->SendTrade(current_product_->best_nonself_ask_price_,
                                                         current_product_->best_nonself_ask_int_price_,
                                                         this_trade_size_, kTradeTypeSell, 'B');

          if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
            DBGLOG_TIME_CLASS_FUNC << "GetFlatSendMultTrade S of " << this_trade_size_ << " @ "
                                   << current_product_->best_nonself_ask_int_price_
                                   << " IntPx: " << current_product_->best_nonself_ask_int_price_
                                   << " mkt: " << best_nonself_bid_size_ << " @ "
                                   << current_product_->best_nonself_bid_price_ << " X "
                                   << current_product_->best_nonself_ask_int_price_ << " @ "
                                   << current_product_->best_nonself_ask_size_ << DBGLOG_ENDL_FLUSH;
          }
          total_order_placed_ += this_trade_size_;
        }
      }
    }
  } else {                                                      // my_position_ < 0
    order_manager_vec_[_product_index_]->CancelAllAskOrders();  // short hence cancel all sell orders
    order_manager_vec_[_product_index_]->CancelBidsBelowIntPrice(
        current_product_->best_nonself_bid_int_price_);  // cancel all non bestlevel bid orders
    {
      // ideal size to place ask for is what the position is, except limited to unit_trade_size_ ( for sanity and market
      // effect reasons )
      int t_size_ordered_ = order_manager_vec_[_product_index_]->SumBidSizeConfirmedEqAboveIntPrice(
                                current_product_->best_nonself_bid_int_price_) +
                            order_manager_vec_[_product_index_]->SumBidSizeUnconfirmedEqAboveIntPrice(
                                current_product_->best_nonself_bid_int_price_);
      int total_order_placed_ = 0;
      if (t_size_ordered_ < -t_position_) {
        while ((total_order_placed_ < -t_position_ - t_size_ordered_) &&
               (total_order_placed_ + t_size_ordered_ <
                max_orders_ * prod_paramset_vec_[_product_index_]->unit_trade_size_)) {
          int this_trade_size_ =
              MathUtils::GetFlooredMultipleOf(std::min(-t_position_ - t_size_ordered_ - total_order_placed_,
                                                       prod_paramset_vec_[_product_index_]->unit_trade_size_),
                                              dep_market_view_vec_[_product_index_]->min_order_size());

          order_manager_vec_[_product_index_]->SendTrade(current_product_->best_nonself_bid_price_,
                                                         current_product_->best_nonself_bid_int_price_,
                                                         this_trade_size_, kTradeTypeBuy, 'B');

          if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
            DBGLOG_TIME_CLASS_FUNC << "GetFlatSendMultTrade B of " << this_trade_size_ << " @ "
                                   << current_product_->best_nonself_bid_price_
                                   << " IntPx: " << current_product_->best_nonself_bid_int_price_
                                   << " mkt: " << current_product_->best_nonself_bid_size_ << " @ "
                                   << current_product_->best_nonself_bid_price_ << " X "
                                   << current_product_->best_nonself_ask_int_price_ << " @ "
                                   << current_product_->best_nonself_ask_size_ << DBGLOG_ENDL_FLUSH;
          }

          total_order_placed_ += this_trade_size_;
        }
      }
    }
  }
}

void EquityTrading2::UnusualTradingStatusGetFlatTradingLogic(int _product_index_) {
  // Can handle cases here seperately
  // 1 : When we are allowed to place/cancel orders at this time.
  current_product_ = product_vec_[_product_index_];
  double theoretical_price_ = current_product_->theoretical_price_;
  int theoretical_int_price_ = dep_market_view_vec_[_product_index_]->GetIntPx(theoretical_price_);

  int t_position_ = current_product_->position_;
  int current_bid_position_ =
      order_manager_vec_[_product_index_]->SumBidSizeConfirmedEqAboveIntPrice(theoretical_int_price_) +
      order_manager_vec_[_product_index_]->SumBidSizeUnconfirmedEqAboveIntPrice(theoretical_int_price_);
  order_manager_vec_[_product_index_]->CancelBidsBelowIntPrice(theoretical_int_price_);

  int current_ask_position_ =
      order_manager_vec_[_product_index_]->SumAskSizeConfirmedEqAboveIntPrice(theoretical_int_price_) +
      order_manager_vec_[_product_index_]->SumAskSizeUnconfirmedEqAboveIntPrice(theoretical_int_price_);
  order_manager_vec_[_product_index_]->CancelAsksBelowIntPrice(theoretical_int_price_);
  int current_net_position_ = current_ask_position_ + current_bid_position_ + t_position_;

  if (current_net_position_ > 0) {
    int trade_size_required_ = MathUtils::GetFlooredMultipleOf(
        std::min(current_net_position_, prod_paramset_vec_[_product_index_]->unit_trade_size_),
        dep_market_view_vec_[_product_index_]->min_order_size());
    order_manager_vec_[_product_index_]->SendTrade(theoretical_price_, theoretical_int_price_, trade_size_required_,
                                                   kTradeTypeSell, 'B');
    if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << "GetFlatSendTrade S of " << trade_size_required_ << " @ " << theoretical_price_
                             << " IntPx: " << current_product_->best_nonself_bid_int_price_
                             << " mkt: " << best_nonself_bid_size_ << " @ " << current_product_->best_nonself_bid_price_
                             << " X " << current_product_->best_nonself_ask_price_ << " @ "
                             << current_product_->best_nonself_ask_size_ << DBGLOG_ENDL_FLUSH;
    }
  } else if (current_net_position_ < 0) {
    int trade_size_required_ = MathUtils::GetFlooredMultipleOf(
        std::min(-current_net_position_, prod_paramset_vec_[_product_index_]->unit_trade_size_),
        dep_market_view_vec_[_product_index_]->min_order_size());

    order_manager_vec_[_product_index_]->SendTrade(theoretical_price_, theoretical_int_price_, trade_size_required_,
                                                   kTradeTypeBuy, 'B');
    if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << "GetFlatSendTrade B of " << trade_size_required_ << " @ " << theoretical_price_
                             << " IntPx: " << current_product_->best_nonself_bid_int_price_
                             << " mkt: " << best_nonself_bid_size_ << " @ " << current_product_->best_nonself_bid_price_
                             << " X " << current_product_->best_nonself_ask_price_ << " @ "
                             << current_product_->best_nonself_ask_size_ << DBGLOG_ENDL_FLUSH;
    }
  }
}
void EquityTrading2::OnPositionChange(int t_new_position_, int position_diff_, const unsigned int _security_id_) {
#if CCPROFILING_TRADEINIT
    HFSAT::CpucycleProfiler::GetUniqueInstance().End(25);
    HFSAT::CpucycleProfiler::GetUniqueInstance().Start(26);
    HFSAT::CpucycleProfiler::GetUniqueInstance().Start(27);
#endif
  UpdateBetaPosition(_security_id_, t_new_position_);
  UpdateVolumeAdjustedMaxPos(sec_id_to_index_[_security_id_]);

  if (!livetrading_) {  // we were getting control here before the signal was updated
    // hence only doing getflat here in sim

    // see if trades need to be placed
    if ((!external_freeze_trading_) && (!freeze_due_to_exchange_stage_)) {
      ProcessGetFlat();  // TODO ... see if this needs to be here ... I mean not much changes in get flat conditions
                         // when the signal is updating
    }
  } else {
    if ((!external_freeze_trading_) && (!freeze_due_to_exchange_stage_) && (!freeze_due_to_funds_reject_)) {
      ProcessGetFlat();
      if (!should_be_getting_flat_) {
        last_non_best_level_om_msecs_ = watch_.msecs_from_midnight();
        PlaceCancelNonBestLevels(sec_id_to_index_[_security_id_]);
      } else {
        //  Getting flat in all products
        for (int i = 0; i < total_products_trading_; i++) {
          if (is_ready_vec_[i]) {
            GetFlatTradingLogic(i);
          }
        }
        if (livetrading_ || (dbglogger_.CheckLoggingLevel(TRADING_INFO))) {
          LogFullStatus();
        }
      }
    }
  }
}

void EquityTrading2::UpdateBetaPosition(const unsigned int sec_id_, int _new_position_) {
  //    static int t_counter_ = 1;
  if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
    DBGLOG_TIME_CLASS_FUNC_LINE << "sid: " << sec_id_ << " pid: " << sec_id_to_index_[sec_id_]
                                << " pname: " << products_being_traded_[sec_id_to_index_[sec_id_]]
                                << " oldpos : " << product_vec_[sec_id_to_index_[sec_id_]]->position_
                                << " newpos: " << _new_position_ << DBGLOG_ENDL_FLUSH;
  }

  UpdateProductPosition(sec_id_, _new_position_);

  int t_index_ = sec_id_to_index_[sec_id_];
  ParamSet* t_paramset_ = nullptr;
  double t_old_beta_risk_ = beta_adjusted_notional_risk_;

  if (dep_market_view_vec_[t_index_]->mkt_size_weighted_price() <= 0) {
    beta_adjusted_notional_risk_ -= betas_[t_index_] * current_product_->notional_risk_;
    current_product_->notional_risk_ = 0;
  } else {
    beta_adjusted_notional_risk_ =
        (beta_adjusted_notional_risk_ - current_product_->notional_risk_ * betas_[t_index_]) +
        (_new_position_ * dep_market_view_vec_[t_index_]->mkt_size_weighted_price() * betas_[t_index_]);
    current_product_->notional_risk_ = _new_position_ * dep_market_view_vec_[t_index_]->mkt_size_weighted_price();
  }

  max_notionl_risk_ = common_paramset_.max_position_ * index_market_view_->price_from_type(kPriceTypeMidprice);

  if (dep_market_view_vec_[sec_id_to_index_[sec_id_]]->mkt_size_weighted_price() <= 0) {
    ExitVerbose(kExitErrorCodeGeneral, " Got position update in security with 0 price");
  }

  for (int i = 0; i < total_products_trading_; i++) {
    t_paramset_ = prod_paramset_vec_[i];
    /// round to lotsize
    product_vec_[i]->beta_adjusted_position_ =
        MathUtils::GetFlooredMultipleOf(self_pos_projection_factor_ * product_vec_[i]->position_ +
                                            (1 - self_pos_projection_factor_) * beta_adjusted_notional_risk_ /
                                                max_global_beta_adjusted_notional_risk_ * t_paramset_->max_position_,
                                        dep_market_view_vec_[i]->min_order_size());
    TradeVarSetLogic(i);

    if ((!external_freeze_trading_) && (!freeze_due_to_exchange_stage_) && (!freeze_due_to_funds_reject_) &&
        is_ready_vec_[i]) {
      if ((!should_be_getting_flat_) && !should_be_getting_flat_vec_[i] &&
          (product_vec_[i]->throttle_manager_->allowed_through_throttle(watch_.msecs_from_midnight())) &&
          (target_price_vec_[i] != 0)) {
        if (order_placing_logic_ == kPriceBasedVolTrading) {
          TradingLogic(i);
        } else if (order_placing_logic_ == kDirectionalAggressiveTrading) {
          DatTradingLogic(i);
        } else if (order_placing_logic_ == kPriceBasedAggressiveTrading) {
          PbatTradingLogic(i);
        } else {
          TradingLogic(i);
        }

      } else if (should_be_getting_flat_ || should_be_getting_flat_vec_[i]) {
        // GetFlatOnIndividualProducts
        GetFlatTradingLogic(i);
        if (livetrading_ || (dbglogger_.CheckLoggingLevel(TRADING_INFO))) {
          LogFullStatus();
        }
      }
    }
  }

  //    t_counter_++;
  // TradingLogic ( sec_id_to_index_[sec_id_] );
  UpdateOpenUnrealizedPNL(t_old_beta_risk_, beta_adjusted_notional_risk_);
  mult_base_pnl_->UpdateTotalRisk(beta_adjusted_notional_risk_);
}

void EquityTrading2::UpdateProductPosition(int sec_id_, int _new_position_) {
  product_vec_[sec_id_to_index_[sec_id_]]->position_ = _new_position_;
  if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
    DBGLOG_TIME_CLASS_FUNC_LINE << " stock " << products_being_traded_[sec_id_to_index_[sec_id_]]
                                << " position: " << product_vec_[sec_id_to_index_[sec_id_]]->position_
                                << " betapos: " << product_vec_[sec_id_to_index_[sec_id_]]->beta_adjusted_position_
                                << DBGLOG_ENDL_FLUSH;
  }
}

inline void EquityTrading2::UpdateVolumeAdjustedMaxPos(int _product_index_) {
  if (prod_paramset_vec_[_product_index_]->read_scale_max_pos_) {
    double t_recent_vol_ = prod_paramset_vec_[_product_index_]->volume_ratio_indicator_->recent_volume();
    int t_vol_scaled_mur_ = 0;
    if (t_recent_vol_ > prod_paramset_vec_[_product_index_]->volume_upper_bound_) {
      t_vol_scaled_mur_ = (prod_paramset_vec_[_product_index_]->volume_upper_bound_ -
                           prod_paramset_vec_[_product_index_]->volume_lower_bound_) *
                          prod_paramset_vec_[_product_index_]->volume_norm_factor_;
    } else if (t_recent_vol_ < prod_paramset_vec_[_product_index_]->volume_lower_bound_) {
      t_vol_scaled_mur_ = 0;
    } else {
      t_vol_scaled_mur_ = (t_recent_vol_ - prod_paramset_vec_[_product_index_]->volume_lower_bound_) *
                          prod_paramset_vec_[_product_index_]->volume_norm_factor_;
    }

    volume_adj_max_pos_vec_[_product_index_] = (prod_paramset_vec_[_product_index_]->base_mur_ + t_vol_scaled_mur_) *
                                               prod_paramset_vec_[_product_index_]->unit_trade_size_;
  }
}

void EquityTrading2::TradeVarSetLogic(int _product_index_) {
  // BaseTrading::TradeVarSetLogic() ;
  /* tradevars for global beta adjusted position
   * we can have different thresholdprofile for this
   * for now using same profile
   */
  current_product_ = product_vec_[_product_index_];
  current_position_tradevarset_map_index_ = P2TV_zero_idx_;
  int t_my_risk_ = int(current_product_->beta_adjusted_position_);
  int t_map_pos_increment_ = map_pos_increment_vec_[_product_index_];
  PositionTradeVarSetMap& t_position_tradevarset_map_ = prod_position_tradevarset_map_vec_[_product_index_];
  TradeVars_t t_current_global_tradevarset_;
  if (t_map_pos_increment_ > 1)  // only when param_set_.max_position_ > MAX_POS_MAP_SIZE
  {
    if (t_my_risk_ > 0) {
      current_position_tradevarset_map_index_ += std::min(MAX_POS_MAP_SIZE, (abs(t_my_risk_) / t_map_pos_increment_));
      t_current_global_tradevarset_ = t_position_tradevarset_map_[current_position_tradevarset_map_index_];
      if ((t_current_global_tradevarset_.l1bid_trade_size_ + t_my_risk_) >
          prod_paramset_vec_[_product_index_]->max_position_) {
        t_current_global_tradevarset_.l1bid_trade_size_ = MathUtils::GetFlooredMultipleOf(
            std::max(0, prod_paramset_vec_[_product_index_]->max_position_ - t_my_risk_),
            dep_market_view_vec_[_product_index_]->min_order_size());
      }
    } else {
      current_position_tradevarset_map_index_ -= std::min(MAX_POS_MAP_SIZE, (abs(t_my_risk_) / t_map_pos_increment_));
      t_current_global_tradevarset_ = t_position_tradevarset_map_[current_position_tradevarset_map_index_];
      if ((t_current_global_tradevarset_.l1ask_trade_size_ - t_my_risk_) >
          prod_paramset_vec_[_product_index_]->max_position_) {
        t_current_global_tradevarset_.l1ask_trade_size_ = MathUtils::GetFlooredMultipleOf(
            std::max(0, prod_paramset_vec_[_product_index_]->max_position_ + t_my_risk_),
            dep_market_view_vec_[_product_index_]->min_order_size());
      }
    }
  } else {
    if (t_my_risk_ > 0) {
      current_position_tradevarset_map_index_ += std::min(MAX_POS_MAP_SIZE, abs(t_my_risk_));
    } else {
      current_position_tradevarset_map_index_ -= std::min(MAX_POS_MAP_SIZE, abs(t_my_risk_));
    }
    t_current_global_tradevarset_ = t_position_tradevarset_map_[current_position_tradevarset_map_index_];
  }

  // Place/Cancel basd on global position on same stock skipped
  current_risk_mapped_to_product_position_ = current_product_->position_;

  if (current_risk_mapped_to_product_position_ >= prod_paramset_vec_[_product_index_]->max_position_ ||
      beta_adjusted_notional_risk_ >= max_notionl_risk_) {  //  too long in same underlying
    t_current_global_tradevarset_.l1bid_trade_size_ = 0;
  } else {
    if (current_risk_mapped_to_product_position_ <= -prod_paramset_vec_[_product_index_]->max_position_ ||
        beta_adjusted_notional_risk_ <= -max_notionl_risk_) {  //  too short in same underlying
      t_current_global_tradevarset_.l1ask_trade_size_ = 0;
    }
  }

  // This is meant to decrease MUR based on volumes
  if (prod_paramset_vec_[_product_index_]->read_scale_max_pos_) {
    if ((t_current_global_tradevarset_.l1bid_trade_size_ + t_my_risk_ > volume_adj_max_pos_vec_[_product_index_]) &&
        (t_my_risk_ >= 0)) {  // Query is too long
      t_current_global_tradevarset_.l1bid_trade_size_ =
          std::max(0, volume_adj_max_pos_vec_[_product_index_] - t_my_risk_);
    } else {
      if ((t_current_global_tradevarset_.l1ask_trade_size_ - t_my_risk_ > volume_adj_max_pos_vec_[_product_index_]) &&
          (t_my_risk_ <= 0)) {  // Query is too short
        t_current_global_tradevarset_.l1ask_trade_size_ =
            std::max(0, volume_adj_max_pos_vec_[_product_index_] + t_my_risk_);
      }
    }
  }
  current_global_tradevarset_vec_[_product_index_] = t_current_global_tradevarset_;
}

void EquityTrading2::ProcessGetFlat() {
  if (!initialized_) {
    Initialize();
  }
  bool t_should_be_getting_flat_ = ShouldBeGettingFlat();
  if (should_be_getting_flat_ &&
      !t_should_be_getting_flat_) {  // currently it is set  ... so after this call we will start normal trading
    if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
      DBGLOG_TIME << "resume_normal_trading @"
                  << " NYC: " << watch_.NYTimeString() << " UTC: " << watch_.UTCTimeString()
                  << " IND: " << watch_.IndTimeString() << DBGLOG_ENDL_FLUSH;
      if (livetrading_) {
        DBGLOG_DUMP;
      }
    }
  }
  should_be_getting_flat_ = t_should_be_getting_flat_;
  for (unsigned index_ = 0; index_ < should_be_getting_flat_vec_.size(); index_++) {
    if (!should_be_getting_flat_vec_[index_] && old_should_be_getting_flat_vec_[index_]) {
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME << "resume_normal_trading for product : " << products_being_traded_[index_] << " @ "
                    << " NYC: " << watch_.NYTimeString() << " UTC: " << watch_.UTCTimeString()
                    << " IND: " << watch_.IndTimeString() << DBGLOG_ENDL_FLUSH;
        if (livetrading_) {
          DBGLOG_DUMP;
        }
      }
    }
    old_should_be_getting_flat_vec_[index_] = should_be_getting_flat_vec_[index_];
  }
}

bool EquityTrading2::ShouldBeGettingFlat() {
  // Global controls
  if (freeze_due_to_exchange_stage_) {
    if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
      DBGLOG_TIME << "Setting should be getting flat to false for exchange freeze " << runtime_id_ << DBGLOG_ENDL_FLUSH;
      if (livetrading_) {
        DBGLOG_DUMP;
      }
    }
    return false;
  }

  if (external_getflat_) {
    if (!getflat_due_to_external_getflat_) {  // first time that getflat_due_to_external_getflat_ is false and
                                              // external_getflat_ is true ..
      // which means the user message was received right now
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME << " getflat_due_to_external_getflat_ " << runtime_id_ << DBGLOG_ENDL_FLUSH;
      }
      getflat_due_to_external_getflat_ = true;
    }
    return true;
  } else {
    getflat_due_to_external_getflat_ = false;
  }

  if (rej_due_to_funds_) {
    if (!getflat_due_to_funds_rej_) {
      // first time that getflat_due_to_funds_rej_ is false and rej_due_to_funds_ is true ..
      // which means the ORS message for reject due to funds was received right now

      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME << " getflat_due_to_funds_rej " << runtime_id_ << DBGLOG_ENDL_FLUSH;
      }
      getflat_due_to_funds_rej_ = true;
    }
    return true;
  } else {
    getflat_due_to_funds_rej_ = false;
  }

  if (getflat_due_to_close_ ||
      (watch_.msecs_from_midnight() > trading_end_utc_mfm_ &&
       watch_.YYYYMMDD() == tradingdate_)) {  // if getflat_due_to_close_ is once activated then it can't be reset  ...
    // perhaps unless a usermessage is sent ?
    if (!getflat_due_to_close_) {
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME << " getflat_due_to_close_ current: " << watch_.msecs_from_midnight() << " " << trading_end_utc_mfm_
                    << " Strategy: " << runtime_id_ << DBGLOG_ENDL_FLUSH;
        if (livetrading_) {
          DBGLOG_DUMP;
        }
      }
      getflat_due_to_close_ = true;
    }
    return true;
  } else if (watch_.msecs_from_midnight() >
             (trading_end_utc_mfm_ - RELEASE_CORE_BEFORE_GIVEN_INTERVAL_IN_MSEC))  // Relase core before the close, why
                                                                                   // an offset - because next guy will
                                                                                   // ask for core exactly at the
                                                                                   // boundaryif it's ready with data
  {
    // release cores, anyways we are closing in
    if (livetrading_) {
      CPUManager::AffinToInitCores(getpid());
    }
  } else {
    getflat_due_to_close_ = false;
  }

  if (open_unrealized_pnl_ < -max_opentrade_loss_) {
    if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
      DBGLOG_TIME << "getflat_due_to_max_opentrade_loss_ ! Current opentradepnl : " << open_unrealized_pnl_ << " < "
                  << -max_opentrade_loss_ << " Strategy: " << runtime_id_ << DBGLOG_ENDL_FLUSH;
      if (livetrading_) {
        DBGLOG_DUMP;
      }
      if (livetrading_ &&  // live-trading and within trading window
          (watch_.msecs_from_midnight() > trading_start_utc_mfm_) &&
          (watch_.msecs_from_midnight() < trading_end_utc_mfm_)) {
        char hostname_[128];
        hostname_[127] = '\0';
        gethostname(hostname_, 127);
        std::string getflat_email_string_ = "";
        {
          std::ostringstream t_oss_;
          t_oss_ << "Strategy: " << runtime_id_ << " getflat_due_to_max_opentrade_loss_: " << open_unrealized_pnl_
                 << " on " << hostname_ << "\n";
          getflat_email_string_ = t_oss_.str();
        }

        SendMail(getflat_email_string_, getflat_email_string_);
      }
    }
    getflat_due_to_max_opentrade_loss_ = true;
    last_getflat_due_to_max_opentrade_loss_triggered_at_msecs_ = watch_.msecs_from_midnight();
    num_opentrade_loss_hits_++;
  } else {
    if ((last_getflat_due_to_max_opentrade_loss_triggered_at_msecs_ > 0) &&
        (watch_.msecs_from_midnight() - last_getflat_due_to_max_opentrade_loss_triggered_at_msecs_ >
         break_msecs_on_max_opentrade_loss_)) {
      getflat_due_to_max_opentrade_loss_ = false;
    }
  }

  if (mult_base_pnl_->total_pnl() < -max_loss_) {
    if (!getflat_due_to_max_loss_) {
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME << " getflat_due_to_max_loss_ " << runtime_id_ << DBGLOG_ENDL_FLUSH;
        if (livetrading_) {
          DBGLOG_DUMP;
        }

        if (livetrading_ &&  // live-trading and within trading window
            (watch_.msecs_from_midnight() > trading_start_utc_mfm_) &&
            (watch_.msecs_from_midnight() < trading_end_utc_mfm_)) {  // Send out an email / alert
          char hostname_[128];
          hostname_[127] = '\0';
          gethostname(hostname_, 127);

          std::string getflat_email_string_ = "";
          {
            std::ostringstream t_oss_;
            t_oss_ << "Strategy: " << runtime_id_ << " getflat_due_to_max_loss_: " << mult_base_pnl_->total_pnl()
                   << " on " << hostname_ << "\n";

            getflat_email_string_ = t_oss_.str();
          }
          SendMail(getflat_email_string_, getflat_email_string_);
        }
      }
      getflat_due_to_max_loss_ = true;
    }
    return true;
  } else {
    getflat_due_to_max_loss_ = false;
  }

  if (our_global_pnl_ < -common_paramset_.global_max_loss_) {
    if (!getflat_due_to_global_max_loss_) {
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME << " getflat_due_to_global_max_loss_ " << our_global_pnl_ << " is less than "
                    << -param_set_.global_max_loss_ << " Strategy: " << runtime_id_ << DBGLOG_ENDL_FLUSH;
        if (livetrading_) {
          DBGLOG_DUMP;
        }
      }

      if (  // live-trading and within trading window
          (watch_.msecs_from_midnight() > trading_start_utc_mfm_) &&
          (watch_.msecs_from_midnight() < trading_end_utc_mfm_)) {  // Send out an email / alert
        char hostname_[128];
        hostname_[127] = '\0';
        gethostname(hostname_, 127);

        std::string getflat_email_string_ = "";
        {
          std::ostringstream t_oss_;
          t_oss_ << "Strategy: " << runtime_id_ << " getflat_due_to_global_max_loss_: " << our_global_pnl_ << " on "
                 << hostname_ << "\n";

          getflat_email_string_ = t_oss_.str();
        }
        SendMail(getflat_email_string_, getflat_email_string_);
      }

      getflat_due_to_global_max_loss_ = true;
    }
    return true;
  } else {
    getflat_due_to_global_max_loss_ = false;
  }
  if (getflat_due_to_non_tradable_events_ && is_event_based_) {
    return true;
  }

  if (getflat_due_to_allowed_economic_event_) {
    return true;
  }

  if (enable_non_standard_check_ && getflat_due_to_non_standard_market_conditions_) {
    return true;
  }

  if (applicable_severity_ >= severity_to_getflat_on_) {
    if (!getflat_due_to_economic_times_) {
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME << "getflat_due_to_economic_times_ @"
                    << " NYC: " << watch_.NYTimeString() << " UTC: " << watch_.UTCTimeString()
                    << " IND: " << watch_.IndTimeString() << " Strategy: " << runtime_id_ << DBGLOG_ENDL_FLUSH;
        if (livetrading_) {
          DBGLOG_DUMP;
        }
      }
      getflat_due_to_economic_times_ = true;
    }
    return true;
  } else {
    getflat_due_to_economic_times_ = false;
  }

  if (getflat_due_to_market_data_interrupt_ && enable_market_data_interrupt_) {
    return true;
  }

  // local controls
  for (int index_ = 0; index_ < total_products_trading_; index_++) {
    bool getflat_set_this_iteration_ = false;
    if (livetrading_ && order_manager_vec_[index_] &&
        (order_manager_vec_[index_]->base_pnl().total_pnl() < -prod_paramset_vec_[index_]->max_loss_)) {
      if (!product_vec_[index_]->getflat_due_to_max_loss_) {
        product_vec_[index_]->getflat_due_to_max_loss_ = true;
        DBGLOG_TIME_CLASS << "getflat_due_to_max_loss_ " << dep_market_view_vec_[index_]->shortcode() << " "
                          << order_manager_vec_[index_]->base_pnl().total_pnl() << DBGLOG_ENDL_FLUSH;
      }
      should_be_getting_flat_vec_[index_] = true;
      getflat_set_this_iteration_ = true;
    } else {
      product_vec_[index_]->getflat_due_to_max_loss_ = false;
    }

    if (product_vec_[index_]->getflat_due_to_market_status_) {
      if (!should_be_getting_flat_vec_[index_]) {
        DBGLOG_TIME_CLASS_FUNC << "getflat_due_to_market_status_ " << dep_market_view_vec_[index_]->shortcode() << " "
                               << MktTradingStatusStr(product_vec_[index_]->trading_status_) << DBGLOG_ENDL_FLUSH;
      }

      should_be_getting_flat_vec_[index_] = true;
      getflat_set_this_iteration_ = true;
    }

    if (product_vec_[index_]->getflat_due_to_external_getflat_) {
      if (!should_be_getting_flat_vec_[index_]) {
        DBGLOG_TIME_CLASS_FUNC << "getflat_due_to_exernal_getflat_ product: " << products_being_traded_[index_] << " "
                               << runtime_id_ << DBGLOG_ENDL_FLUSH;
      }
      should_be_getting_flat_vec_[index_] = true;
      getflat_set_this_iteration_ = true;
    }

    if (product_vec_[index_]->getflat_due_to_close_ ||
        watch_.msecs_from_midnight() > product_vec_[index_]->trading_end_utc_mfm_) {
      if (!product_vec_[index_]->getflat_due_to_close_) {
        DBGLOG_TIME_CLASS_FUNC << "getflat_due_to_close_ product: " << products_being_traded_[index_]
                               << DBGLOG_ENDL_FLUSH;
        product_vec_[index_]->getflat_due_to_close_ = true;
      }
      should_be_getting_flat_vec_[index_] = true;
      getflat_set_this_iteration_ = true;
    }

    if (!getflat_set_this_iteration_) {
      should_be_getting_flat_vec_[index_] = false;
    }
  }

  return false;
}

inline void EquityTrading2::OnControlUpdate(const ControlMessage& _control_message_, const char* symbol,
                                            const int trader_id) {
  if (!initialized_) {
    Initialize();
  }
  switch (_control_message_.message_code_) {
    case kControlMessageCodeGetflat: {
      if (strlen(_control_message_.strval_1_) == 0)  // Its global command
      {
        if (!getting_flat_) {
          if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
            DBGLOG_TIME << "getflat_due_to_external_getflat_ " << trader_id << DBGLOG_ENDL_FLUSH;
            if (livetrading_) {
              DBGLOG_DUMP;
            }
          }
          // Need external getflat for individual proucts?
          external_getflat_ = true;
          getting_flat_ = true;
          for (unsigned i = 0; i < product_vec_.size(); i++) {
            product_vec_[i]->getflat_due_to_external_getflat_ = true;
          }
          ProcessGetFlat();
        }
      } else {
        // Command for specific product
        for (unsigned i = 0; i < products_being_traded_.size(); i++) {
          if (strcmp(_control_message_.strval_1_, products_being_traded_[i].c_str()) == 0) {
            if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
              DBGLOG_TIME_CLASS << "getflat_due_to_external_getflat_ " << trader_id
                                << " for product: " << products_being_traded_[i] << DBGLOG_ENDL_FLUSH;
              if (livetrading_) {
                DBGLOG_DUMP;
              }
            }
            product_vec_[i]->getflat_due_to_external_getflat_ = true;
            ProcessGetFlat();
            break;
          }
        }
      }
    } break;
    case kControlMessageCodeAggGetflat: {
      if (strlen(_control_message_.strval_1_) == 0)  // Its global command
      {
        if (!aggressively_getting_flat_) {
          if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
            DBGLOG_TIME << "getflat_due_to_external_aggressive_getflat_ " << trader_id << DBGLOG_ENDL_FLUSH;
            if (livetrading_) {
              DBGLOG_DUMP;
            }
          }
          // Need external getflat for individual proucts?
          external_getflat_ = true;
          getting_flat_ = true;
          for (unsigned i = 0; i < product_vec_.size(); i++) {
            product_vec_[i]->getflat_due_to_external_getflat_ = true;
            product_vec_[i]->getflat_aggressive_ = true;
          }
          aggressively_getting_flat_ = true;
          ProcessGetFlat();
        }
      } else {
        // Command for specific product
        for (unsigned i = 0; i < products_being_traded_.size(); i++) {
          if (strcmp(_control_message_.strval_1_, products_being_traded_[i].c_str()) == 0) {
            if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
              DBGLOG_TIME_CLASS << "getflat_due_to_external_aggressive_getflat_ " << trader_id
                                << " for product: " << products_being_traded_[i] << DBGLOG_ENDL_FLUSH;
              if (livetrading_) {
                DBGLOG_DUMP;
              }
            }
            product_vec_[i]->getflat_due_to_external_getflat_ = true;
            product_vec_[i]->getflat_aggressive_ = true;
            ProcessGetFlat();
            break;
          }
        }
      }
    } break;
    case kControlMessageCodeStartTrading: {
      if (strlen(_control_message_.strval_1_) == 0) {
        if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
          DBGLOG_TIME_CLASS << "StartTrading Called " << runtime_id_ << DBGLOG_ENDL_FLUSH;
          if (livetrading_) {
            DBGLOG_DUMP;
          }
        }
        external_getflat_ = false;
        getflat_due_to_external_getflat_ = false;
        external_cancel_all_outstanding_orders_ = false;
        getting_flat_ = false;
        for (unsigned i = 0; i < product_vec_.size(); i++) {
          product_vec_[i]->getflat_due_to_external_getflat_ = false;
        }

        aggressively_getting_flat_ = false;
        ProcessGetFlat();
        external_freeze_trading_ = false;
        start_not_given_ = false;
      } else {
        for (unsigned i = 0; i < products_being_traded_.size(); i++) {
          if (strcmp(_control_message_.strval_1_, products_being_traded_[i].c_str()) == 0) {
            if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
              DBGLOG_TIME_CLASS << "StartTrading Called " << trader_id << " for product: " << products_being_traded_[i]
                                << DBGLOG_ENDL_FLUSH;
              if (livetrading_) {
                DBGLOG_DUMP;
              }
              product_vec_[i]->getflat_due_to_external_getflat_ = false;
              product_vec_[i]->getflat_aggressive_ = false;
            }
          }
        }
      }
    } break;

    case kControlMessageCodeDumpPositions: {
      DBGLOG_TIME_CLASS << "DumpPositions" << DBGLOG_ENDL_FLUSH;
      if (livetrading_) {
        DBGLOG_DUMP;
      }
    } break;
    case kControlMessageCodeSetMaxGlobalRisk: {
      if (strlen(_control_message_.strval_1_) == 0) {
        DBGLOG_TIME_CLASS << "Set Max GlobalRisk called with no shortcode "
                          << " val : " << _control_message_.intval_1_ << DBGLOG_ENDL_FLUSH;
        if (livetrading_) {
          DBGLOG_DUMP;
        }
        if (_control_message_.intval_1_ > common_paramset_.max_global_risk_ / FAT_FINGER_FACTOR &&
            _control_message_.intval_1_ < common_paramset_.max_global_risk_ / FAT_FINGER_FACTOR) {
          common_paramset_.max_global_risk_ = _control_message_.intval_1_;
        }
      } else {
        for (auto i = 0u; i < products_being_traded_.size(); i++) {
          if (strcmp(_control_message_.strval_1_, products_being_traded_[i].c_str()) == 0) {
            if (_control_message_.intval_1_ > prod_paramset_vec_[i]->max_global_risk_ / FAT_FINGER_FACTOR &&
                _control_message_.intval_1_ < prod_paramset_vec_[i]->max_global_risk_ / FAT_FINGER_FACTOR) {
              prod_paramset_vec_[i]->max_global_risk_ = _control_message_.intval_1_;
            }
          }
        }
      }
      ProcessGetFlat();
    } break;
    case kControlMessageCodeSetMaxPosition: {
      if (strlen(_control_message_.strval_1_) == 0) {
        DBGLOG_TIME_CLASS << "Set Max position called with no shortcode "
                          << " val : " << _control_message_.intval_1_ << DBGLOG_ENDL_FLUSH;
        if (livetrading_) {
          DBGLOG_DUMP;
        }
      } else {
        for (auto i = 0u; i < products_being_traded_.size(); i++) {
          if (strcmp(_control_message_.strval_1_, products_being_traded_[i].c_str()) == 0) {
            if (_control_message_.intval_1_ > prod_paramset_vec_[i]->max_position_ / FAT_FINGER_FACTOR &&
                _control_message_.intval_1_ < prod_paramset_vec_[i]->max_position_ * FAT_FINGER_FACTOR) {
              prod_paramset_vec_[i]->max_position_ = std::max(1, _control_message_.intval_1_);
            }
            BuildTradeVarSets(i);
            TradeVarSetLogic(i);
            if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
              DBGLOG_TIME_CLASS << "SetMaxPosition " << runtime_id_ << " called with " << _control_message_.intval_1_
                                << " and MaxPosition for param " << i << " set to "
                                << prod_paramset_vec_[i]->max_position_ << "" << DBGLOG_ENDL_FLUSH;
              if (livetrading_) {
                DBGLOG_DUMP;
              }
            }
            break;
          }
        }
      }
    } break;
    case kControlMessageCodeSetUnitTradeSize: {
      if (strlen(_control_message_.strval_1_) == 0) {
        DBGLOG_TIME_CLASS << "Set UnitTradeSize called with no shortcode "
                          << " val : " << _control_message_.intval_1_ << DBGLOG_ENDL_FLUSH;
        if (livetrading_) {
          DBGLOG_DUMP;
        }
      } else {
        for (auto i = 0u; i < products_being_traded_.size(); i++) {
          if (strcmp(_control_message_.strval_1_, products_being_traded_[i].c_str()) == 0) {
            if (_control_message_.intval_1_ > prod_paramset_vec_[i]->unit_trade_size_ / FAT_FINGER_FACTOR &&
                _control_message_.intval_1_ < prod_paramset_vec_[i]->unit_trade_size_ * FAT_FINGER_FACTOR) {
              prod_paramset_vec_[i]->unit_trade_size_ = std::max(1, _control_message_.intval_1_);
            }
            BuildTradeVarSets(i);
            TradeVarSetLogic(i);
            if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
              DBGLOG_TIME_CLASS << "SetUnitTradeSize " << runtime_id_ << " called with " << _control_message_.intval_1_
                                << " and UnitTradeSize for product " << products_being_traded_[i] << " set to "
                                << prod_paramset_vec_[i]->unit_trade_size_ << "" << DBGLOG_ENDL_FLUSH;
              if (livetrading_) {
                DBGLOG_DUMP;
              }
            }
            break;
          }
        }
      }
    } break;
    case kControlMessageCodeSetMaxUnitRatio: {
      if (strlen(_control_message_.strval_1_) == 0) {
        DBGLOG_TIME_CLASS << "Set MaxUnitRatio called with no shortcode "
                          << " val : " << _control_message_.intval_1_ << DBGLOG_ENDL_FLUSH;
        if (livetrading_) {
          DBGLOG_DUMP;
        }
      } else {
        for (auto i = 0u; i < products_being_traded_.size(); i++) {
          if (strcmp(_control_message_.strval_1_, products_being_traded_[i].c_str()) == 0) {
            if (_control_message_.intval_1_ > prod_paramset_vec_[i]->max_unit_ratio_ / FAT_FINGER_FACTOR &&
                _control_message_.intval_1_ < prod_paramset_vec_[i]->max_unit_ratio_ * FAT_FINGER_FACTOR) {
              prod_paramset_vec_[i]->max_unit_ratio_ = std::max(1, _control_message_.intval_1_);
            }
            BuildTradeVarSets(i);
            TradeVarSetLogic(i);
            if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
              DBGLOG_TIME_CLASS << "SetMaxUnitRatio " << runtime_id_ << " called with " << _control_message_.intval_1_
                                << " and MaxUnitRatio for product " << products_being_traded_[i] << " set to "
                                << prod_paramset_vec_[i]->max_position_ << "" << DBGLOG_ENDL_FLUSH;
              if (livetrading_) {
                DBGLOG_DUMP;
              }
            }
            break;
          }
        }
      }
    } break;
    case kControlMessageCodeSetWorstCasePosition: {
      if (strlen(_control_message_.strval_1_) == 0) {
        DBGLOG_TIME_CLASS << "Set WorstCasePosition called with no shortcode "
                          << " val : " << _control_message_.intval_1_ << DBGLOG_ENDL_FLUSH;
        if (livetrading_) {
          DBGLOG_DUMP;
        }
      } else {
        for (auto i = 0u; i < products_being_traded_.size(); i++) {
          if (strcmp(_control_message_.strval_1_, products_being_traded_[i].c_str()) == 0) {
            if (_control_message_.intval_1_ > prod_paramset_vec_[i]->worst_case_position_ / FAT_FINGER_FACTOR &&
                _control_message_.intval_1_ < prod_paramset_vec_[i]->worst_case_position_ * FAT_FINGER_FACTOR) {
              prod_paramset_vec_[i]->worst_case_position_ = std::max(1, _control_message_.intval_1_);
            }
            BuildTradeVarSets(i);
            TradeVarSetLogic(i);
            if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
              DBGLOG_TIME_CLASS << "WorstCasePosition " << runtime_id_ << " called with " << _control_message_.intval_1_
                                << " and WorstCasePosition for product " << products_being_traded_[i] << " set to "
                                << prod_paramset_vec_[i]->worst_case_position_ << "" << DBGLOG_ENDL_FLUSH;
              if (livetrading_) {
                DBGLOG_DUMP;
              }
            }
            break;
          }
        }
      }
    } break;
    case kControlMessageCodeSetWorstCaseUnitRatio: {
      if (strlen(_control_message_.strval_1_) == 0) {
        DBGLOG_TIME_CLASS << "Set WorstCaseUnitRatio called with no shortcode "
                          << " val : " << _control_message_.intval_1_ << DBGLOG_ENDL_FLUSH;
        if (livetrading_) {
          DBGLOG_DUMP;
        }
      } else {
        for (auto i = 0u; i < products_being_traded_.size(); i++) {
          if (strcmp(_control_message_.strval_1_, products_being_traded_[i].c_str()) == 0) {
            if (_control_message_.intval_1_ > prod_paramset_vec_[i]->worst_case_unit_ratio_ / FAT_FINGER_FACTOR &&
                _control_message_.intval_1_ < prod_paramset_vec_[i]->worst_case_unit_ratio_ * FAT_FINGER_FACTOR) {
              prod_paramset_vec_[i]->worst_case_unit_ratio_ = std::max(1, _control_message_.intval_1_);
              prod_paramset_vec_[i]->worst_case_position_ =
                  prod_paramset_vec_[i]->unit_trade_size_ * prod_paramset_vec_[i]->worst_case_unit_ratio_;
            }
            BuildTradeVarSets(i);
            TradeVarSetLogic(i);
            if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
              DBGLOG_TIME_CLASS << "WorstCaseUnitRatio " << runtime_id_ << " called with "
                                << _control_message_.intval_1_ << " and WorstCaseUnitRatio for product "
                                << products_being_traded_[i] << " set to "
                                << prod_paramset_vec_[i]->worst_case_unit_ratio_ << "" << DBGLOG_ENDL_FLUSH;
              if (livetrading_) {
                DBGLOG_DUMP;
              }
            }
            break;
          }
        }
      }
    } break;
    case kControlMessageCodeShowParams: {
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME_CLASS << "ShowParams " << runtime_id_ << " called." << DBGLOG_ENDL_FLUSH;
        if (livetrading_) {
          DBGLOG_DUMP;
        }
      }
      ShowParams();
    } break;
    case kControlMessageCodeForceIndicatorReady: {
      if (strlen(_control_message_.strval_1_) == 0) {
        DBGLOG_TIME_CLASS << "ForceIndicatorReady " << runtime_id_
                          << " called without product name: " << _control_message_.intval_1_ << DBGLOG_ENDL_FLUSH;
      } else {
        for (unsigned i = 0; i < products_being_traded_.size(); i++) {
          if (strcmp(_control_message_.strval_1_, products_being_traded_[i].c_str()) == 0) {
            if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
              DBGLOG_TIME_CLASS << "ForceIndicatorReady " << runtime_id_
                                << " called for indicator_index_ = " << _control_message_.intval_1_
                                << " Product: " << products_being_traded_[i] << DBGLOG_ENDL_FLUSH;
              if (livetrading_) {
                DBGLOG_DUMP;
              }
            }
            if (model_math_vec_[i]) {
              model_math_vec_[i]->ForceIndicatorReady(_control_message_.intval_1_);
            }
          }
        }
      }
    } break;
    case kControlMessageCodeForceAllIndicatorReady: {
    } break;
    case kControlMessageCodeFreezeTrading:
    case kControlMessageCodeUnFreezeTrading:
    case kControlMessageCodeCancelAllFreezeTrading:
    case kControlMessageCodeSetTradeSizes:
    case kControlMessageCodeAddPosition:
    case kControlMessageCodeDisableImprove:
    case kControlMessageCodeEnableImprove:
    case kControlMessageCodeDisableAggressive:
    case kControlMessageCodeEnableAggressive:
    case kControlMessageCodeCleanSumSizeMaps:
    case kControlMessageDisableSelfOrderCheck:
    case kControlMessageEnableSelfOrderCheck:
    case kControlMessageDumpNonSelfSMV:
    case kControlMessageCodeEnableAggCooloff:
    case kControlMessageCodeDisableAggCooloff:
    case kControlMessageCodeEnableNonStandardCheck:
    case kControlMessageCodeDisableNonStandardCheck:
    case kControlMessageCodeSetMaxIntSpreadToPlace:
    case kControlMessageCodeSetMaxIntLevelDiffToPlace:
    case kControlMessageCodeSetExplicitMaxLongPosition: {
      if (livetrading_) {
        DBGLOG_DUMP;
      }
    } break;
    case kControlMessageCodeSetEcoSeverity: {
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME_CLASS << "IgnoreEconomicNumbers " << runtime_id_ << " called." << DBGLOG_ENDL_FLUSH;
        if (livetrading_) {
          DBGLOG_DUMP;
        }
      }
      severity_to_getflat_on_ = std::max(1.00, _control_message_.doubleval_1_);
      int t_severity_change_end_msecs_ = GetMsecsFromMidnightFromHHMMSS(_control_message_.intval_1_);
      severity_change_end_msecs_ = std::min(trading_end_utc_mfm_, t_severity_change_end_msecs_);
      DBGLOG_TIME << "Seteco set to " << severity_to_getflat_on_ << " at time " << watch_.msecs_from_midnight()
                  << " with end time as " << severity_change_end_msecs_ << DBGLOG_ENDL_FLUSH;
      ProcessGetFlat();
    } break;
    case kControlMessageCodeSetExplicitWorstLongPosition: {
      for (auto i = 0u; i < prod_paramset_vec_.size(); i++) {
        if (strcmp(_control_message_.strval_1_, products_being_traded_[i].c_str()) == 0) {
          DBGLOG_TIME_CLASS << "User Message for shortcode " << products_being_traded_[i] << DBGLOG_ENDL_FLUSH;
          if (livetrading_) {
            DBGLOG_DUMP;
          }

          if (_control_message_.intval_1_ > 0) {
            prod_paramset_vec_[i]->explicit_worst_case_long_position_ =
                std::min(5 * prod_paramset_vec_[i]->worst_case_position_, _control_message_.intval_1_);
            prod_paramset_vec_[i]->read_explicit_worst_case_long_position_ = true;
          }
          if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
            DBGLOG_TIME_CLASS << "SetExplicitWorstLongPosition " << runtime_id_
                              << " called for explicit_worst_case_long_position_ = " << _control_message_.intval_1_
                              << " and ExplicitWorstLongPosition for param " << i << " set to "
                              << param_set_vec_[i].explicit_worst_case_long_position_ << DBGLOG_ENDL_FLUSH;
          }
        }

        ProcessGetFlat();
        BuildTradeVarSets(i);
        TradeVarSetLogic(i);
      }
    } break;
    case kControlMessageCodeShowIndicators:
    case kControlMessageDisableMarketManager:
    case kControlMessageEnableMarketManager:
    case kControlMessageCodeEnableLogging:
    case kControlMessageCodeDisableLogging:
      break;
    case kControlMessageCodeShowOrders: {
      DBGLOG_TIME_CLASS << "ShowOrders " << runtime_id_ << " called." << DBGLOG_ENDL_FLUSH;
      if (livetrading_) {
        DBGLOG_DUMP;
      }
      last_full_logging_msecs_ = 0;
      override_zero_logging_mode_once_for_external_cmd_ = true;
      LogFullStatus();
    } break;
    case kControlMessageCodeEnableZeroLoggingMode:
    case kControlMessageCodeDisableZeroLoggingMode: {
      if (livetrading_) {
        DBGLOG_DUMP;
      }
      break;
    }
    case kControlMessageCodeSetStartTime: {
      int old_trading_start_utc_mfm_ = -1;

      if (_control_message_.intval_1_ >= 0 && _control_message_.intval_1_ <= 2359) {  // utc hhmm time sanity check.
        old_trading_start_utc_mfm_ = trading_start_utc_mfm_;
        trading_start_utc_mfm_ = GetMsecsFromMidnightFromHHMM(_control_message_.intval_1_) + watch_.day_offset();
      }

      DBGLOG_TIME_CLASS << "SetStartTime " << runtime_id_ << " called with " << _control_message_.intval_1_
                        << " and trading_start_utc_mfm_ set to " << trading_start_utc_mfm_ << " from "
                        << old_trading_start_utc_mfm_ << DBGLOG_ENDL_FLUSH;

      if (livetrading_) {
        DBGLOG_DUMP;
      }

      ProcessGetFlat();
      break;
    }
    case kControlMessageCodeSetEndTime: {
      int old_trading_end_utc_mfm_ = -1;

      if (_control_message_.intval_1_ >= 0 && _control_message_.intval_1_ <= 2359) {  // utc hhmm time sanity check.
        old_trading_end_utc_mfm_ = trading_end_utc_mfm_;
        trading_end_utc_mfm_ = GetMsecsFromMidnightFromHHMM(_control_message_.intval_1_);
      }

      DBGLOG_TIME_CLASS << "SetEndTime " << runtime_id_ << " called with " << _control_message_.intval_1_
                        << " and trading_end_utc_mfm_ set to " << trading_end_utc_mfm_ << " from "
                        << old_trading_end_utc_mfm_ << DBGLOG_ENDL_FLUSH;

      if (livetrading_) {
        DBGLOG_DUMP;
      }
      ProcessGetFlat();
    } break;
    case kControlMessageCodeSetMaxLoss: {
      if (strlen(_control_message_.strval_1_) == 0) {
        if (_control_message_.intval_1_ > max_loss_ / FAT_FINGER_FACTOR &&
            _control_message_.intval_1_ < max_loss_ * FAT_FINGER_FACTOR) {
          max_loss_ = _control_message_.intval_1_;
          common_paramset_.max_loss_ = _control_message_.intval_1_;
        }

        if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
          DBGLOG_TIME_CLASS << "SetMaxLoss " << trader_id
                            << " called for abs_max_loss_ = " << _control_message_.intval_1_ << " and MaxLoss set to "
                            << max_loss_ << DBGLOG_ENDL_FLUSH;

          if (livetrading_) {
            DBGLOG_DUMP;
          }
        }
      } else {
        for (auto i = 0u; i < products_being_traded_.size(); i++) {
          if (strcmp(_control_message_.strval_1_, products_being_traded_[i].c_str()) == 0) {
            if (_control_message_.intval_1_ > prod_paramset_vec_[i]->max_loss_ / FAT_FINGER_FACTOR &&
                _control_message_.intval_1_ < prod_paramset_vec_[i]->max_loss_ * FAT_FINGER_FACTOR) {
              prod_paramset_vec_[i]->max_loss_ = _control_message_.intval_1_;
              if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
                DBGLOG_TIME_CLASS << "SetMaxLoss " << trader_id << " Product: " << products_being_traded_[i]
                                  << " called for abs_max_loss = " << _control_message_.intval_1_
                                  << " and MaxLoss set to " << prod_paramset_vec_[i]->max_loss_ << DBGLOG_ENDL_FLUSH;
                if (livetrading_) {
                  DBGLOG_DUMP;
                }
              }
            }
            break;
          }
        }
      }
    } break;

    case kControlMessageCodeSetOpenTradeLoss: {
      if (_control_message_.intval_1_ > max_opentrade_loss_ / FAT_FINGER_FACTOR &&
          _control_message_.intval_1_ < max_opentrade_loss_ * FAT_FINGER_FACTOR) {
        max_opentrade_loss_ = _control_message_.intval_1_;
      }

      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME_CLASS << "SetOpenTradeLoss " << trader_id
                          << "UpdatePNL called for abs_opentrade_loss_ = " << _control_message_.intval_1_
                          << " and OpenTradeLoss set to " << max_opentrade_loss_ << DBGLOG_ENDL_FLUSH;

        if (livetrading_) {
          DBGLOG_DUMP;
        }
      }
    } break;

    case kControlMessageCodeSetBreakMsecsOpenTradeLoss: {
      if (_control_message_.intval_1_ > 0) {
        break_msecs_on_max_opentrade_loss_ = _control_message_.intval_1_;
        break_msecs_on_max_opentrade_loss_ = std::max(1 * 60 * 1000, break_msecs_on_max_opentrade_loss_);
      }
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME_CLASS << "SetBreakMsecsOpenTradeLoss " << trader_id
                          << " called for break_msecs_on_max_opentrade_loss_ = " << _control_message_.intval_1_
                          << " and BreakMsecsOpenTradeLoss set to " << break_msecs_on_max_opentrade_loss_
                          << DBGLOG_ENDL_FLUSH;
        if (livetrading_) {
          DBGLOG_DUMP;
        }
      }
    } break;
    default:
      break;
  }
}

inline double EquityTrading2::total_pnl() { return total_pnl_; }

inline void EquityTrading2::UpdatePNL(int _total_pnl_) {
  total_pnl_ = _total_pnl_;
  open_unrealized_pnl_ = total_pnl_ - realized_pnl_;
}

inline void EquityTrading2::UpdateOpenUnrealizedPNL(double last_pc1_risk_, double current_pc1_risk_) {
  if (current_pc1_risk_ * last_pc1_risk_ < 0) {
    realized_pnl_ += open_unrealized_pnl_;
    open_unrealized_pnl_ = 0.0;
  }
}

bool EquityTrading2::IsHittingMaxLoss() {
  if (total_pnl_ < -max_loss_) {
    return true;
  } else {
    return false;
  }
}

bool EquityTrading2::IsHittingOpentradeLoss() {
  if (open_unrealized_pnl_ < -max_opentrade_loss_) {
    last_max_opentrade_loss_hit_msecs_ = watch_.msecs_from_midnight();
    return true;
  } else if (watch_.msecs_from_midnight() - last_max_opentrade_loss_hit_msecs_ < break_msecs_on_max_opentrade_loss_) {
    return true;
  } else {
    return false;
  }
}

void EquityTrading2::GetProductListToGetFlatMultOrder() {
  std::ifstream product_list_file_;
  product_list_file_.open(MULT_ORDER_GET_FLAT_PROD_FILE, std::ifstream::in);

  if (product_list_file_.is_open()) {
    int kProductLineLength = 1024;
    char line[kProductLineLength];
    bzero(line, kProductLineLength);
    while (product_list_file_.good()) {
      bzero(line, kProductLineLength);
      product_list_file_.getline(line, kProductLineLength);
      PerishableStringTokenizer st_(line, kProductLineLength);
      const std::vector<const char*>& tokens_ = st_.GetTokens();
      if (tokens_.size() < 2) {
        continue;
      }
      std::string this_prod_ = std::string(tokens_[0]);
      for (unsigned i = 0; i < products_being_traded_.size(); i++) {
        if (products_being_traded_[i].compare(this_prod_) == 0) {
          product_vec_[i]->getflat_mult_order_ = true;
          max_orders_ = atoi(tokens_[1]);
        }
      }
    }
    product_list_file_.close();
  } else {
    DBGLOG_CLASS_FUNC << "can't find file: " << MULT_ORDER_GET_FLAT_PROD_FILE << " for reading" << DBGLOG_ENDL_FLUSH;
  }
}

void EquityTrading2::Initialize() {
  is_ready_vec_.resize(total_products_trading_, false);
  should_be_getting_flat_vec_.resize(total_products_trading_, false);
  old_should_be_getting_flat_vec_.resize(total_products_trading_, false);
  ls_position_.resize(total_products_trading_, 0);
  target_position_.resize(total_products_trading_, 0);

  target_price_vec_.resize(total_products_trading_, 0.0);
  targetbias_numbers_vec_.resize(total_products_trading_, 0.0);
  volume_adj_max_pos_vec_.resize(total_products_trading_, 0.0);
  prod_index_to_sector_index_.resize(total_products_trading_, 0);
  if (common_paramset_.read_volume_ratio_stop_trading_lower_threshold_) {
    for (unsigned i = 0; i < product_vec_.size(); i++) {
      product_vec_[i]->volume_ratio_stop_trading_lower_threshold_ =
          common_paramset_.volume_ratio_stop_trading_lower_threshold_;
    }
  }

  for (unsigned i = 0; i < prod_paramset_vec_.size(); i++) {
    if (prod_paramset_vec_[i]->read_volume_ratio_stop_trading_lower_threshold_) {
      // Overwriting if the values are given seperately for different products
      product_vec_[i]->volume_ratio_stop_trading_lower_threshold_ =
          prod_paramset_vec_[i]->volume_ratio_stop_trading_lower_threshold_;
    }
  }

  if (common_paramset_.read_volume_ratio_stop_trading_upper_threshold_) {
    for (unsigned i = 0; i < product_vec_.size(); i++) {
      product_vec_[i]->volume_ratio_stop_trading_upper_threshold_ =
          common_paramset_.volume_ratio_stop_trading_upper_threshold_;
    }
  }

  for (unsigned i = 0; i < prod_paramset_vec_.size(); i++) {
    if (prod_paramset_vec_[i]->read_volume_ratio_stop_trading_upper_threshold_) {
      // Overwriting if the values are given seperately for different products
      product_vec_[i]->volume_ratio_stop_trading_upper_threshold_ =
          prod_paramset_vec_[i]->volume_ratio_stop_trading_upper_threshold_;
    }
  }

  InitializeBetaValues();

  if (order_placing_logic_ != kPriceBasedVolTrading && order_placing_logic_ != kDirectionalAggressiveTrading &&
      order_placing_logic_ != kPriceBasedAggressiveTrading) {
    SetComputeReturns();
  }

  SetComputeTresholds();

  initialized_ = true;
}
// Beta values of the stocks being traded
void EquityTrading2::InitializeBetaValues() {
  // Change LRDB
  offline_beta_.resize(total_products_trading_ + 1);
  for (auto i = 0u; i < products_being_traded_.size(); i++) {
    offline_beta_[i].resize(total_products_trading_ + 1, 0.0);
    for (unsigned int j = 0; j < products_being_traded_.size(); j++) {
      if (i != j) {
        offline_beta_[i][j] =
            (lrdb_vec_[i]->GetLRCoeff(products_being_traded_[i], products_being_traded_[j])).lr_coeff_;
      } else {
        offline_beta_[i][j] = 1;
      }
      //            DBGLOG_TIME_CLASS_FUNC_LINE << " LR value: " << products_being_traded_[i] << " and " <<
      //            products_being_traded_[j] << " : " << offline_beta_[i][j] << DBGLOG_ENDL_FLUSH ;
    }
  }

  offline_beta_[total_products_trading_].resize(total_products_trading_ + 1, 0.0);
  for (unsigned i = 0; i < products_being_traded_.size(); i++) {
    offline_beta_[products_being_traded_.size()][i] =
        lrdb_vec_[i]->GetLRCoeff(index_name_, products_being_traded_[i]).lr_coeff_;
    //        DBGLOG_TIME_CLASS_FUNC_LINE << " LR value: BR_IND_0 "  << " and " << products_being_traded_[i] << " : " <<
    //        offline_beta_[products_being_traded_.size()][i]  << DBGLOG_ENDL_FLUSH ;
  }
  for (unsigned i = 0; i < products_being_traded_.size(); i++) {
    offline_beta_[i][products_being_traded_.size()] =
        lrdb_vec_[i]->GetLRCoeff(products_being_traded_[i], index_name_).lr_coeff_;
    //        DBGLOG_TIME_CLASS_FUNC_LINE << " LR value: " << products_being_traded_[i] << " and BR_IND_0 : " <<
    //        offline_beta_[i][products_being_traded_.size()]  << DBGLOG_ENDL_FLUSH ;
  }

  offline_beta_[products_being_traded_.size()][products_being_traded_.size()] = 1;

  for (auto i = 0u; i < products_being_traded_.size(); i++) {
    stdevratio_with_index_.push_back(sqrt(offline_beta_[i][products_being_traded_.size()] /
                                          offline_beta_[products_being_traded_.size()][i]));  // with IND
    DBGLOG_TIME_CLASS_FUNC_LINE << " LR value: Ratio: " << stdevratio_with_index_[i] << " " << products_being_traded_[i]
                                << DBGLOG_ENDL_FLUSH;
  }

  // Returns LRDB
  offline_ret_beta_.resize(total_products_trading_ + 1);
  betas_.resize(total_products_trading_);
  betas_with_sector_.resize(total_products_trading_);
  offline_ret_beta_[total_products_trading_].resize(total_products_trading_ + 1, 0.0);
  for (unsigned i = 0; i < products_being_traded_.size(); i++) {
    offline_ret_beta_[i].resize(total_products_trading_ + 1, 0.0);
    offline_ret_beta_[products_being_traded_.size()][i] =
        retlrdb_vec_[i]->GetLRCoeff(index_name_, products_being_traded_[i]).lr_coeff_;
    DBGLOG_TIME_CLASS_FUNC_LINE << " Returns LR value: BR_IND_0 "
                                << " and " << products_being_traded_[i] << " : "
                                << offline_ret_beta_[products_being_traded_.size()][i] << DBGLOG_ENDL_FLUSH;
  }
  for (unsigned i = 0; i < products_being_traded_.size(); i++) {
    offline_ret_beta_[i][products_being_traded_.size()] =
        retlrdb_vec_[i]->GetLRCoeff(products_being_traded_[i], index_name_).lr_coeff_;
    betas_[i] = offline_ret_beta_[i][products_being_traded_.size()];
    DBGLOG_TIME_CLASS_FUNC_LINE << " Returns LR value: " << products_being_traded_[i]
                                << " and BR_IND_0 : " << offline_ret_beta_[i][products_being_traded_.size()]
                                << DBGLOG_ENDL_FLUSH;
  }

  for (unsigned i = 0; i < products_being_traded_.size(); i++) {
    std::string sectorport_ = GetSectorForBMFStock(products_being_traded_[i]);
    double beta_ = retlrdb_vec_[i]->GetLRCoeff(products_being_traded_[i], sectorport_).lr_coeff_;
    if (beta_ == 0.0) {
      std::cerr << " no beta found for " << products_being_traded_[i] << " " << sectorport_ << std::endl;
      // ExitVerbose( kExitErrorCodeGeneral, (" no beta found for " + products_being_traded_[i] + " " +
      // sectorport_).c_str() );
    }
    betas_with_sector_[i] = beta_;
  }

  for (auto i = 0u; i < products_being_traded_.size(); i++) {
    for (unsigned int j = 0; j < products_being_traded_.size(); j++) {
      if (i != j) {
        offline_ret_beta_[i][j] =
            (retlrdb_vec_[i]->GetLRCoeff(products_being_traded_[i], products_being_traded_[j])).lr_coeff_;
        if (offline_ret_beta_[i][j] == 0.0) {
          DBGLOG_TIME_CLASS_FUNC_LINE << " No RetLR for " << products_being_traded_[i] << " and "
                                      << products_being_traded_[j] << " Pair. Calculating it from IND"
                                      << DBGLOG_ENDL_FLUSH;
          // In case we dont have the ret-beta calculated for this pair
          // Calculate it via IND
          offline_ret_beta_[i][j] = retlrdb_vec_[i]->GetLRCoeff(products_being_traded_[i], index_name_).lr_coeff_ *
                                    retlrdb_vec_[i]->GetLRCoeff(index_name_, products_being_traded_[j]).lr_coeff_;
        }
      } else {
        offline_ret_beta_[i][j] = 1;
      }
      DBGLOG_TIME_CLASS_FUNC_LINE << " Returns LR value: " << products_being_traded_[i] << " and "
                                  << products_being_traded_[j] << " : " << offline_ret_beta_[i][j] << DBGLOG_ENDL_FLUSH;
    }
  }

  offline_ret_beta_[products_being_traded_.size()][products_being_traded_.size()] = 1;
}

void EquityTrading2::PrintFullStatus(int _product_index_) {
  DBGLOG_TIME << "shc: " << dep_market_view_vec_[_product_index_]->shortcode()
              << "tgt: " << target_price_vec_[_product_index_] << " ExpBidPft@ "
              << current_product_->best_nonself_bid_price_ << " X " << current_product_->best_nonself_bid_size_ << ' '
              << (target_price_vec_[_product_index_] - current_product_->best_nonself_bid_price_) << " ExpAskPft@ "
              << current_product_->best_nonself_ask_price_ << " X " << current_product_->best_nonself_ask_size_ << ' '
              << (current_product_->best_nonself_ask_price_ - target_price_vec_[_product_index_])
              << " signalbias: " << ((target_price_vec_[_product_index_] -
                                      dep_market_view_vec_[_product_index_]->mkt_size_weighted_price()) /
                                     dep_market_view_vec_[_product_index_]->min_price_increment())
              << " notrisk: " << current_product_->notional_risk_ << " pos: " << current_product_->position_
              << " betapos: " << current_product_->beta_adjusted_position_
              << " trdstatus: " << MktTradingStatusStr(current_product_->trading_status_) << DBGLOG_ENDL_FLUSH;
}

void EquityTrading2::ShowParams() {
  for (auto i = 0u; i < prod_paramset_vec_.size(); i++) {
    prod_paramset_vec_[i]->WriteSendStruct(control_reply_struct_.param_set_send_struct_);
    ParamSetSendStruct& param_set_send_struct_ = control_reply_struct_.param_set_send_struct_;

    param_set_send_struct_.query_control_bits_ = GetControlChars();

    // log the state
    LogControlChars(param_set_send_struct_.query_control_bits_);
    DBGLOG_TIME << "==========================================\n"
                << "Product " << products_being_traded_[i] << ":" << DBGLOG_ENDL_FLUSH;

    LogParamSetSendStruct(param_set_send_struct_);

    // log position and pnl
    DBGLOG_TIME << "product_position_ " << product_vec_[i]->position_ << DBGLOG_ENDL_FLUSH;
    DBGLOG_TIME << "product_pnl_ " << order_manager_vec_[i]->base_pnl().total_pnl() << DBGLOG_ENDL_FLUSH;
    DBGLOG_TIME << "total_pnl_ " << total_pnl_ << DBGLOG_ENDL_FLUSH;
    DBGLOG_TIME << "==========================================" << DBGLOG_ENDL_FLUSH;

    if (livetrading_) {
      DBGLOG_DUMP;
    }

    control_reply_struct_.time_set_by_query_ = watch_.tv();
    control_reply_struct_.my_position_ = my_position_;
    control_reply_struct_.total_pnl_ = order_manager_.base_pnl().total_pnl();
  }
}

void EquityTrading2::LogFullStatus() {
  if (zero_logging_mode_ && !override_zero_logging_mode_once_for_external_cmd_) {
    return;
  }
#define FULL_LOGGING_INTERVAL_MSECS 5000
  if (override_zero_logging_mode_once_for_external_cmd_) {
    override_zero_logging_mode_once_for_external_cmd_ = false;
  }

  if ((last_full_logging_msecs_ == 0) ||
      (watch_.msecs_from_midnight() - last_full_logging_msecs_ > FULL_LOGGING_INTERVAL_MSECS)) {
    DBGLOG_TIME_CLASS_FUNC << "Orders:" << DBGLOG_ENDL_FLUSH;
    for (unsigned i = 0; i < order_manager_vec_.size(); i++) {
      DBGLOG_TIME_CLASS_FUNC_LINE << "Product: " << products_being_traded_[i] << " " << DBGLOG_ENDL_FLUSH;
      if (order_manager_vec_[i] != nullptr) {
        PrintFullStatus(i);
        order_manager_vec_[i]->LogFullStatus();
      }
    }
  }
}

void EquityTrading2::ReportResults(HFSAT::BulkFileWriter& trades_writer_, bool _conservative_close_) {
  int t_total_volume_ = 0;
  int t_supporting_orders_filled_ = 0;
  int t_bestlevel_orders_filled_ = 0;
  int t_aggressive_orders_filled_ = 0;
  int t_improve_orders_filled_ = 0;
  int t_total_pnl_ = 0;
  // t_total_pnl_ = total_pnl_;
  if (!initialized_) {
    Initialize();
  }  // doing it here just to avoid segfaults in logging
  for (unsigned int index_ = 0; index_ < products_being_traded_.size(); index_++) {
    DBGLOG_TIME_CLASS_FUNC_LINE << " Product: " << products_being_traded_[index_] << " "
                                << product_vec_[index_]->position_ << " " << total_index_risk_
                                << " nr: " << total_index_risk_ << " "
                                << dep_market_view_vec_[index_]->mkt_size_weighted_price() << " "
                                << order_manager_vec_[index_]->base_pnl().total_pnl() << " : "
                                << dep_market_view_vec_[index_]->shortcode() << DBGLOG_ENDL_FLUSH;
  }
  if (livetrading_) {
    DBGLOG_TIME << "PNLSPLIT " << runtime_id_ << " ";
  } else {
    trades_writer_ << "PNLSPLIT " << runtime_id_ << " ";
  }
  for (auto i = 0u; i < order_manager_vec_.size(); i++) {
    if (order_manager_vec_[i] != nullptr) {
      t_total_pnl_ = order_manager_vec_[i]->base_pnl().mult_total_pnl();
      SmartOrderManager& t_order_manager_ = *order_manager_vec_[i];
      t_total_volume_ += t_order_manager_.trade_volume();
      t_supporting_orders_filled_ += t_order_manager_.SupportingOrderFilledPercent() * t_order_manager_.trade_volume();
      t_bestlevel_orders_filled_ += t_order_manager_.BestLevelOrderFilledPercent() * t_order_manager_.trade_volume();
      t_improve_orders_filled_ += t_order_manager_.ImproveOrderFilledPercent() * t_order_manager_.trade_volume();
      t_aggressive_orders_filled_ += t_order_manager_.AggressiveOrderFilledPercent() * t_order_manager_.trade_volume();
      std::cerr << dep_market_view_vec_[i]->shortcode() << " " << t_order_manager_.base_pnl().total_pnl() << " "
                << t_order_manager_.trade_volume() << std::endl;
      if (livetrading_) {
        dbglogger_ << dep_market_view_vec_[i]->shortcode() << " " << t_order_manager_.base_pnl().total_pnl() << " "
                   << t_order_manager_.trade_volume() << " " << (int)t_order_manager_.SupportingOrderFilledPercent()
                   << " " << (int)t_order_manager_.BestLevelOrderFilledPercent() << " "
                   << t_order_manager_.ImproveOrderFilledPercent() << " "
                   << t_order_manager_.AggressiveOrderFilledPercent() << " ";
      } else {
        trades_writer_ << dep_market_view_vec_[i]->shortcode() << " " << t_order_manager_.base_pnl().total_pnl() << " "
                       << t_order_manager_.trade_volume() << " " << (int)t_order_manager_.SupportingOrderFilledPercent()
                       << " " << (int)t_order_manager_.BestLevelOrderFilledPercent() << " "
                       << t_order_manager_.ImproveOrderFilledPercent() << " "
                       << t_order_manager_.AggressiveOrderFilledPercent() << " ";
      }
    }
  }

  if (livetrading_) {
    DBGLOG_DUMP;
  } else {
    trades_writer_ << "\n";
    trades_writer_.DumpCurrentBuffer();
  }

  int num_messages_ = 0;
  for (unsigned i = 0; i < order_manager_vec_.size(); i++) {
    if (order_manager_vec_[i] != nullptr) {
      SmartOrderManager& t_order_manager_ = *order_manager_vec_[i];
      num_messages_ +=
          (t_order_manager_.SendOrderCount() + t_order_manager_.CxlOrderCount() + t_order_manager_.ModifyOrderCount());
      if (livetrading_) {
        dbglogger_ << "EOD_MSG_COUNT: " << runtime_id_ << " " << dep_market_view_vec_[i]->shortcode() << " "
                   << (t_order_manager_.SendOrderCount() + t_order_manager_.CxlOrderCount() +
                       t_order_manager_.ModifyOrderCount()) << "\n";
      } else {
        trades_writer_ << "EOD_MSG_COUNT: " << runtime_id_ << " " << dep_market_view_vec_[i]->shortcode() << " "
                       << (t_order_manager_.SendOrderCount() + t_order_manager_.CxlOrderCount() +
                           t_order_manager_.ModifyOrderCount()) << "\n";
      }
    }
  }

  if (livetrading_) {
    dbglogger_ << "EOD_MSG_COUNT: " << runtime_id_ << " TOTAL " << num_messages_;
    DBGLOG_DUMP;
  } else {
    trades_writer_ << "EOD_MSG_COUNT: " << runtime_id_ << " TOTAL " << num_messages_;
    trades_writer_ << "\n";
    trades_writer_.DumpCurrentBuffer();
  }

  if (t_total_volume_ > 0) {
    t_supporting_orders_filled_ /= t_total_volume_;
    t_bestlevel_orders_filled_ /= t_total_volume_;
    t_improve_orders_filled_ /= t_total_volume_;
    t_aggressive_orders_filled_ /= t_total_volume_;
  }

  printf("SIMRESULT %d %d %d %d %d %d\n", t_total_pnl_, (t_total_volume_), t_supporting_orders_filled_,
         t_bestlevel_orders_filled_, t_aggressive_orders_filled_, t_improve_orders_filled_);
}

void EquityTrading2::SendMail(std::string _mail_content_, std::string _mail_subject_) {
  HFSAT::Email email_;
  email_.setSubject(_mail_subject_);
  email_.addRecepient("nseall@tworoads.co.in");
  email_.addSender("nseall@tworoads.co.in");
  email_.content_stream << _mail_content_ << "<br/>";
  email_.sendMail();
}

void EquityTrading2::get_positions(std::vector<std::string>& _instrument_vec_, std::vector<int>& _position_vec_) {
  for (unsigned i = 0; i < products_being_traded_.size(); i++) {
    if (product_vec_[i]->position_ != 0) {
      _instrument_vec_.push_back(products_being_traded_[i]);
      _position_vec_.push_back(product_vec_[i]->position_);
    }
  }
}

std::vector<std::string> EquityTrading2::LoadProductList() {
  std::vector<std::string> slist_;
  std::ifstream shortcode_list_file;
  int date_ = HFSAT::ExchangeSymbolManager::GetUniqueInstance().YYYYMMDD();  // hack
  std::string filename_ = GetBovespaIndexConstituentFileList(date_);
  shortcode_list_file.open(filename_.c_str(), std::ifstream::in);

  if (shortcode_list_file.is_open()) {
    while (shortcode_list_file.good()) {
      const int kL1AvgBufferLen = 1024;
      char readline_buffer_[kL1AvgBufferLen];
      bzero(readline_buffer_, kL1AvgBufferLen);
      shortcode_list_file.getline(readline_buffer_, kL1AvgBufferLen);
      if (readline_buffer_[0] == '#') {
        continue;
      }
      // # shortcode_ theoretical_volume_
      PerishableStringTokenizer st_(readline_buffer_, kL1AvgBufferLen);
      const std::vector<const char*>& tokens_ = st_.GetTokens();
      if (tokens_.size() < 2) {
        continue;
      }
      if (tokens_.size() > 4 && std::string(tokens_[1]).compare("Reductor") == 0) {
        continue;
      }
      std::string t_shc_ = tokens_[0];
      slist_.push_back(t_shc_);
    }
  }
  slist_.push_back("BR_IND_0");
  return slist_;
}
}
