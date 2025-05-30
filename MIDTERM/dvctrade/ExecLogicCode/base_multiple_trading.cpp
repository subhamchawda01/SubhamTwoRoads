/**
   \file ExecLogic/base_multiple_trading.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#include <sstream>
#include "dvctrade/ExecLogic/base_multiple_trading.hpp"
#include "dvccode/CDef/math_utils.hpp"
#include "dvccode/CDef/error_utils.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/Profiler/cpucycle_profiler.hpp"
#include <string>

#define SMALL_PX_DIFF 1e-05
#define SMALL_COOLOFF_MSECS_ 10

int HFSAT::BaseMultipleTrading::class_var_counter_ = 0;

namespace HFSAT {

void BaseMultipleTrading::CollectORSShortCodes(DebugLogger& _dbglogger_, const std::string& r_strategy_name_,
                                               const std::vector<std::string>& r_options_shortcodes_,
                                               std::vector<std::string>& source_shortcode_vec_,
                                               std::vector<std::string>& ors_source_needed_vec_) {}

void BaseMultipleTrading::CollectShortCodes(DebugLogger& _dbglogger_, const std::string& r_dep_shortcode_,
                                            std::vector<std::string>& source_shortcode_vec_) {}

BaseMultipleTrading::BaseMultipleTrading(DebugLogger& _dbglogger_, const Watch& _watch_,
                                         std::vector<SecurityMarketView*> _underlying_market_view_vec_,
                                         std::map<std::string, std::vector<SmartOrderManager*> > _shc_const_som_map_,
                                         OptionsRiskManager* _options_risk_manager_,
                                         BaseOptionRiskPremium* _options_risk_premium_, int _trading_start_mfm_,
                                         int _trading_end_mfm_, const bool _livetrading_, const int t_runtime_id_,
                                         SecurityNameIndexer& _sec_name_indexer_)
    : dbglogger_(_dbglogger_),
      watch_(_watch_),
      tradingdate_(watch_.YYYYMMDD()),
      sec_name_indexer_(_sec_name_indexer_),
      trading_start_utc_mfm_(_trading_start_mfm_),
      trading_end_utc_mfm_(_trading_end_mfm_),
      runtime_id_(t_runtime_id_),
      options_risk_manager_(_options_risk_manager_),
      options_risk_premium_(_options_risk_premium_),
      target_vec_(),
      targetbias_numbers_vec_(),
      current_tradevarset_(),
      cpu_allocated_(false),
      livetrading_(_livetrading_),
      start_not_given_(_livetrading_),
      is_affined_(false),
      last_affine_attempt_msecs_(0),
      print_on_trade_(false),
      last_full_logging_msecs_(0),
      zero_logging_mode_(true),
      override_zero_logging_mode_once_for_external_cmd_(false),
      is_alert_raised_ (false),
      improve_keep_cancels_(0){
  class_var_counter_++;
  pid_file_ = std::string(OUTPUT_LOGDIR) +
              HFSAT::ExchSourceStringForm(HFSAT::SecurityDefinitions::GetContractExchSource(
                  _underlying_market_view_vec_[0]->shortcode(), tradingdate_)) +
              "_" + std::to_string(runtime_id_) + "_PIDfile.txt";
  exit_bool_set_ = false;
  first_obj_ = (class_var_counter_ == 1);

  options_data_matrix_ = options_risk_manager_->options_data_matrix_;
  underlying_data_vec_ = options_risk_manager_->underlying_data_vec_;
  underlying_to_risk_matrix_ = options_risk_premium_->risk_matrix_;

  std::map<std::string, std::vector<SecurityMarketView*> > shc_const_smv_map_ =
      HFSAT::MultModelCreator::GetShcToConstSMVMap();
  security_id_prod_idx_map_ = HFSAT::MultModelCreator::GetSecIdMap();

  num_underlying_ = underlying_data_vec_.size();

  target_vec_.resize(num_underlying_, std::vector<double>());
  targetbias_numbers_vec_.resize(num_underlying_, std::vector<double>());

  for (unsigned int idx = 0; idx < _underlying_market_view_vec_.size(); idx++) {
    for (unsigned int const_indx_ = 0; const_indx_ < options_data_matrix_[idx].size(); const_indx_++) {
      target_vec_[idx].push_back(0.0);
      targetbias_numbers_vec_[idx].push_back(0.0);
      options_risk_manager_->ProcessGetFlat(idx, const_indx_);
    }
  }
  // use a default 120 ( sum )
  // constituents * 10 =~ 50 per underlying
  // default is 130
  options_risk_manager_->InitThrottleManager(throttle_manager_pvec_);
}

void BaseMultipleTrading::TradingLogic(int _product_index_, int _option_index_) {
  current_tradevarset_ = options_risk_premium_->GetPremium(
      _product_index_, _option_index_, options_data_matrix_[_product_index_][_option_index_]->position_);

  OptionVars* current_exec_vars_ = options_data_matrix_[_product_index_][_option_index_];
  OptionRisk current_risk_vars_ = underlying_to_risk_matrix_[_product_index_][_option_index_];
  double target_ = target_vec_[_product_index_][_option_index_];
  SecurityMarketView* p_dep_market_view_ = current_exec_vars_->smv_;
  SmartOrderManager* p_smart_order_manager_ = current_exec_vars_->som_;

  double best_nonself_ask_int_price_ = current_exec_vars_->best_nonself_ask_int_price_;
  double best_nonself_ask_price_ = current_exec_vars_->best_nonself_ask_price_;
  double best_nonself_ask_size_ = current_exec_vars_->best_nonself_ask_size_;
  double best_nonself_bid_int_price_ = current_exec_vars_->best_nonself_bid_int_price_;
  double best_nonself_bid_price_ = current_exec_vars_->best_nonself_bid_price_;
  double best_nonself_bid_size_ = current_exec_vars_->best_nonself_bid_size_;

  if ((dbglogger_.CheckLoggingLevel(TRADING_INFO)) || (dbglogger_.CheckLoggingLevel(DBG_PARAM_INFO))) {
    DBGLOG_TIME_CLASS_FUNC_LINE << " TargetNumber: " << target_ << " Mkt: " << best_nonself_bid_size_ << "@"
                                << best_nonself_bid_price_ << " -- " << best_nonself_ask_price_ << "@"
                                << best_nonself_ask_size_ << " SHC: " << p_dep_market_view_->shortcode()
                                << " BP: " << current_tradevarset_.l1bid_place_
                                << " AP: " << current_tradevarset_.l1ask_place_
                                << " CPOS: " << current_exec_vars_->position_ << DBGLOG_ENDL_FLUSH;
  }

  // lift, place, keep
  bool top_bid_place_ = false;
  bool top_bid_keep_ = false;
  bool top_ask_lift_ = false;
  // improve_keep
  // this is tricky
  //         1) the behaviour of market can change, if we keep improve orders ( size is one thing, but price (?) )
  //         2) assuming the behaviour doesnt change, we run only strat, in real we will book including our orders, thus
  //         are we improving ? will  always be false

  // (cancel orders when number of orders <= (no_of_confirmed_orders) && live)  // holding this for now, this one
  // shouldnt change sim results if and when done
  // (cancel orders when number of orders <= 0 ) // for sim case

  // getflat logics ( once triggered will be stayed in the state
  //(p_dep_market_view_->spread_increments() <= current_risk_vars_.max_int_spread_to_place_)

  // not very interested in this for now, dealing with
  //(best_nonself_bid_size_ > current_risk_vars_.min_size_to_join_)

  //(best_nonself_bid_price_ > current_risk_vars_.min_int_price_to_place_)

  // can buy ?
  if (current_tradevarset_.l1bid_trade_size_ > 0) {
    // place passive ?
    if (((((current_exec_vars_->last_buy_msecs_ <= 0) ||
           (watch_.msecs_from_midnight() - current_exec_vars_->last_buy_msecs_ >=
            current_risk_vars_.cooloff_interval_) ||
           (best_nonself_bid_int_price_ != current_exec_vars_->last_buy_int_price_)) &&
          (p_dep_market_view_->spread_increments() <= current_risk_vars_.max_int_spread_to_place_)) ||
         current_exec_vars_->position_ < 0) &&
        target_ - best_nonself_bid_price_ >= current_tradevarset_.l1bid_place_) {
      top_bid_place_ = true;
      top_bid_keep_ = true;
      // further can agg ?
      if ((current_risk_vars_.allowed_to_aggress_) &&
          ((watch_.msecs_from_midnight() - current_exec_vars_->last_agg_buy_msecs_ >=
            current_risk_vars_.agg_cooloff_interval_) ||
           (current_exec_vars_->position_ < 0)) &&
          (p_dep_market_view_->spread_increments() <= current_risk_vars_.max_int_spread_to_cross_) &&
          (current_exec_vars_->position_ <= current_risk_vars_.max_position_to_aggress_) &&
          (target_ - best_nonself_ask_price_ >= current_tradevarset_.l1bid_aggressive_)) {
        top_ask_lift_ = true;
      }
      // atleast keep ?
    } else if (target_ - best_nonself_bid_price_ >= current_tradevarset_.l1bid_keep_) {
      top_bid_keep_ = true;
    }
  }

  bool top_ask_place_ = false;
  bool top_ask_keep_ = false;
  bool top_bid_hit_ = false;

  if (current_tradevarset_.l1ask_trade_size_ > 0) {
    // place passive ?
    if (((((current_exec_vars_->last_sell_msecs_ <= 0) ||
           (watch_.msecs_from_midnight() - current_exec_vars_->last_sell_msecs_ >=
            current_risk_vars_.cooloff_interval_) ||
           (best_nonself_ask_int_price_ != current_exec_vars_->last_buy_int_price_)) &&
          (p_dep_market_view_->spread_increments() <= current_risk_vars_.max_int_spread_to_place_)) ||
         current_exec_vars_->position_ > 0) &&
        best_nonself_ask_price_ - target_ >= current_tradevarset_.l1ask_place_) {
      top_ask_place_ = true;
      top_ask_keep_ = true;
      // further can lift ?
      if ((current_risk_vars_.allowed_to_aggress_) &&
          ((watch_.msecs_from_midnight() - current_exec_vars_->last_agg_sell_msecs_ >=
            current_risk_vars_.agg_cooloff_interval_) ||
           (current_exec_vars_->position_ > 0)) &&
          (p_dep_market_view_->spread_increments() <= current_risk_vars_.max_int_spread_to_cross_) &&
          (current_exec_vars_->position_ >= current_risk_vars_.max_position_to_aggress_) &&
          (best_nonself_bid_price_ - target_ >= current_tradevarset_.l1ask_aggressive_)) {
        top_bid_hit_ = true;
      }
      // atleast keep
    } else if (best_nonself_ask_price_ - target_ >= current_tradevarset_.l1ask_keep_) {
      top_ask_keep_ = true;
    }
  }

  // BID
  // IMPROVE_KEEP defaulted to false
  if((best_nonself_bid_size_ <= p_smart_order_manager_->SumBidSizeConfirmedEqAboveIntPrice(best_nonself_bid_int_price_)) &&
     (watch_.msecs_from_midnight() - p_smart_order_manager_->GetTopConfirmedBidOrder()->placed_msecs() > 1) && (livetrading_)) {
    p_smart_order_manager_->CancelBidsEqAboveIntPrice(best_nonself_bid_int_price_);
    improve_keep_cancels_ ++;
  } else {
    int improve_bid_canceled_size_ = p_smart_order_manager_->CancelBidsAboveIntPrice(best_nonself_bid_int_price_);
    if (improve_bid_canceled_size_ > 0 && dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << " Cancelled Improve Bid orders Above: " << best_nonself_bid_price_
			     << " Position: " << current_exec_vars_->position_
			     << " Cancelled: " << improve_bid_canceled_size_ << " Mkt: " << best_nonself_bid_size_
			     << " @ " << best_nonself_bid_price_ << "  ---  " << best_nonself_ask_price_ << " @ "
			     << best_nonself_ask_size_ << DBGLOG_ENDL_FLUSH;
    }
  }

  if (top_ask_lift_) {
    if (p_smart_order_manager_->GetTotalBidSizeAboveIntPx(best_nonself_bid_int_price_) == 0) {
      // can be passive order + this_agg > max_position {!!}
      p_smart_order_manager_->SendTrade(best_nonself_ask_price_, best_nonself_ask_int_price_,
                                        current_tradevarset_.l1bid_trade_size_, kTradeTypeBuy, 'A');
      current_exec_vars_->last_agg_buy_msecs_ = watch_.msecs_from_midnight();
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        DBGLOG_TIME_CLASS_FUNC << "SendTrade AGG B of " << current_tradevarset_.l1bid_trade_size_ << " @ "
                               << best_nonself_ask_price_ << " tgt: " << target_
                               << " thresh_t: " << current_tradevarset_.l1bid_place_
                               << " SHC: " << p_dep_market_view_->shortcode() << " Mkt: " << best_nonself_bid_size_
                               << " @ " << best_nonself_bid_price_ << "  ---  " << best_nonself_ask_price_ << " @ "
                               << best_nonself_ask_size_ << DBGLOG_ENDL_FLUSH;
      }
    }
  } else if (top_bid_place_) {
    // DBGLOG_TIME_CLASS_FUNC_LINE << " " <<
    // p_smart_order_manager_->GetTotalBidSizeEqAboveIntPx(best_nonself_bid_int_price_) << DBGLOG_ENDL_FLUSH;
    if ((p_smart_order_manager_->GetTotalBidSizeEqAboveIntPx(best_nonself_bid_int_price_) == 0)) {
      p_smart_order_manager_->SendTrade(best_nonself_bid_price_, best_nonself_bid_int_price_,
                                        current_tradevarset_.l1bid_trade_size_, kTradeTypeBuy, 'B');
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        DBGLOG_TIME_CLASS_FUNC << "SendTrade B of " << current_tradevarset_.l1bid_trade_size_ << " @ "
                               << best_nonself_bid_price_ << " tgt: " << target_
                               << " thresh_t: " << current_tradevarset_.l1bid_place_
                               << " SHC: " << p_dep_market_view_->shortcode() << " Mkt: " << best_nonself_bid_size_
                               << " @ " << best_nonself_bid_price_ << "  ---  " << best_nonself_ask_price_ << " @ "
                               << best_nonself_ask_size_ << DBGLOG_ENDL_FLUSH;
      }
    }
  } else if (!top_bid_keep_) {  // cancel all orders at best_nonself_bid_price_
    int canceled_size_ = p_smart_order_manager_->CancelBidsEqAboveIntPrice(best_nonself_bid_int_price_);
    if (canceled_size_ > 0) {
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        DBGLOG_TIME << "Canceled B of " << canceled_size_ << " EqAbove " << best_nonself_bid_price_
                    << " tgt_bias: " << target_ / p_dep_market_view_->min_price_increment()
                    << " thresh_t: " << current_tradevarset_.l1bid_keep_
                    << " SHC: " << p_dep_market_view_->shortcode() << " Mkt: " << best_nonself_bid_size_
                    << " @ " << best_nonself_bid_price_ << " ---- " << best_nonself_ask_price_ << " @ "
                    << " tMktSz: " << best_nonself_bid_size_ << DBGLOG_ENDL_FLUSH;
      }
    }
  }

  // ASK
  if((best_nonself_ask_size_ <= p_smart_order_manager_->SumAskSizeConfirmedEqAboveIntPrice(best_nonself_ask_int_price_)) &&
     (watch_.msecs_from_midnight() - p_smart_order_manager_->GetTopConfirmedAskOrder()->placed_msecs() > 1) && (livetrading_)){
    p_smart_order_manager_->CancelAsksEqAboveIntPrice(best_nonself_ask_int_price_);
    improve_keep_cancels_ ++;
  } else {
    int improve_ask_canceled_size_ = p_smart_order_manager_->CancelAsksAboveIntPrice(best_nonself_ask_int_price_);
    if (improve_ask_canceled_size_ > 0 && dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << " Cancelled Improve Ask orders Above: " << best_nonself_ask_price_
			     << " Position: " << current_exec_vars_->position_
			     << " Cancelled: " << improve_ask_canceled_size_ << " Mkt: " << best_nonself_bid_size_
			     << " @ " << best_nonself_bid_price_ << "  ---  " << best_nonself_ask_price_ << " @ "
			     << best_nonself_ask_size_ << DBGLOG_ENDL_FLUSH;
    }
  }

  if (top_bid_hit_) {
    if (p_smart_order_manager_->GetTotalBidSizeEqAboveIntPx(best_nonself_ask_int_price_) == 0) {
      p_smart_order_manager_->SendTrade(best_nonself_bid_price_, best_nonself_bid_int_price_,
                                        current_tradevarset_.l1ask_trade_size_, kTradeTypeSell, 'A');
      current_exec_vars_->last_agg_sell_msecs_ = watch_.msecs_from_midnight();
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        DBGLOG_TIME_CLASS_FUNC << "SendTrade AGG S of " << current_tradevarset_.l1ask_trade_size_ << " @ "
                               << best_nonself_bid_price_ << " tgt: " << target_
                               << " thresh_t: " << current_tradevarset_.l1ask_place_
                               << " SHC: " << p_dep_market_view_->shortcode() << " Mkt: " << best_nonself_bid_size_
                               << " @ " << best_nonself_bid_price_ << " ---- " << best_nonself_ask_price_ << " @ "
                               << best_nonself_ask_size_ << DBGLOG_ENDL_FLUSH;
      }
    }
  } else if (top_ask_place_) {
    if ((p_smart_order_manager_->GetTotalAskSizeEqAboveIntPx(best_nonself_ask_int_price_) == 0)) {
      p_smart_order_manager_->SendTrade(best_nonself_ask_price_, best_nonself_ask_int_price_,
                                        current_tradevarset_.l1ask_trade_size_, kTradeTypeSell, 'B');
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        DBGLOG_TIME_CLASS_FUNC << "SendTrade S of " << current_tradevarset_.l1ask_trade_size_ << " @ "
                               << best_nonself_ask_price_ << " tgt: " << target_
                               << " thresh_t: " << current_tradevarset_.l1ask_place_
                               << " SHC: " << p_dep_market_view_->shortcode() << " Mkt: " << best_nonself_bid_size_
                               << " @ " << best_nonself_bid_price_ << " ---- " << best_nonself_ask_price_ << " @ "
                               << best_nonself_ask_size_ << DBGLOG_ENDL_FLUSH;
      }
    }
  } else if (!top_ask_keep_) {
    int canceled_size_ = p_smart_order_manager_->CancelAsksEqAboveIntPrice(best_nonself_ask_int_price_);
    if (canceled_size_ > 0) {
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        DBGLOG_TIME << "Canceled S of " << canceled_size_ << " EqAbove " << best_nonself_ask_price_
                    << " tgt: " << -target_ / p_dep_market_view_->min_price_increment()
                    << " thresh_t: " << current_tradevarset_.l1ask_keep_
                    << " SHC: " << p_dep_market_view_->shortcode() << " Mkt: " << best_nonself_bid_size_
                    << " @ " << best_nonself_bid_price_ << " ---- " << best_nonself_ask_price_ << " @ "
                    << " tMktSz: " << best_nonself_ask_size_ << DBGLOG_ENDL_FLUSH;
      }
    }
  }
}
// this should be called by specific option_contract
bool BaseMultipleTrading::UpdateTarget(double _target_price_, double _targetbias_numbers_, int _modelmath_index_,
                                       int _option_index_) {
  OptionVars* current_exec_vars_ = options_data_matrix_[_modelmath_index_][_option_index_];
  OptionVars* underlying_exec_vars_ = underlying_data_vec_[_modelmath_index_];

  SecurityMarketView* current_smv_ = current_exec_vars_->smv_;
  SmartOrderManager* current_som_ = current_exec_vars_->som_;

  if ((!current_exec_vars_->is_ready_) || (!underlying_exec_vars_->is_ready_)) {
    bool t_is_ready_ = true;
    t_is_ready_ = current_smv_->is_ready() && underlying_exec_vars_->smv_->is_ready();

    if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << current_smv_->secname() << " trying to get ready" << DBGLOG_ENDL_FLUSH;
    }

    if ((watch_.msecs_from_midnight() > current_exec_vars_->trading_start_utc_mfm_ &&
         watch_.YYYYMMDD() == tradingdate_) &&
        t_is_ready_) {
      // Now it's the right time to get CPU
      if (livetrading_ && !cpu_allocated_) {
        AllocateCPU();
        cpu_allocated_ = true;
      }

      underlying_exec_vars_->is_ready_ = true;
      current_exec_vars_->Initialize();

      DBGLOG_TIME_CLASS_FUNC << "SACI: " << current_som_->server_assigned_client_id_
                             << " got ready! shc: " << current_smv_->shortcode() << DBGLOG_ENDL_FLUSH;
    }
  } else {
    targetbias_numbers_vec_[_modelmath_index_][_option_index_] = _targetbias_numbers_;
    target_vec_[_modelmath_index_][_option_index_] = _target_price_;
    options_risk_manager_->ProcessGetFlat(_modelmath_index_, _option_index_);

    // strategy global loss
    if (options_risk_manager_->is_max_loss_reached_global_) {
      for (int i = 0; i < num_underlying_; i++) {
        options_risk_manager_->GetFlatTradingLogic(i, -1);
        options_risk_manager_->CallPlaceCancelNonBestLevels(i);
        for (unsigned int j = 0; j < options_data_matrix_[i].size(); j++) {
          options_risk_manager_->GetFlatTradingLogic(i, j);
          CallPlaceCancelNonBestLevels(i, j);
        }
      }
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        DBGLOG_TIME_CLASS_FUNC << current_smv_->shortcode()
                               << " Strategy GlobalMaxLoss Triggered: " << DBGLOG_ENDL_FLUSH;
      }
      // checking underlying loss and option loss along with other non-trading conditions
      // non-trading conditions include, external_flat, close, otl, max_loss, mkt_data_interrupt
      // should include {price < 2 ticks, economic event }
    } else if ((!current_exec_vars_->should_be_getting_flat_) && (!underlying_exec_vars_->should_be_getting_flat_)) {
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        DBGLOG_TIME_CLASS_FUNC << current_smv_->shortcode()
                               << " tgtbias: " << targetbias_numbers_vec_[_modelmath_index_][_option_index_]
                               << DBGLOG_ENDL_FLUSH;
      }
      if (throttle_manager_pvec_[_modelmath_index_]->allowed_through_throttle(watch_.msecs_from_midnight())) {
        TradingLogic(_modelmath_index_, _option_index_);
        CallPlaceCancelNonBestLevels(_modelmath_index_, _option_index_);
      } else {
        //DBGLOG_TIME_CLASS_FUNC << "Warning Throttle Hit" << DBGLOG_ENDL_FLUSH;
      }
    } else if (underlying_exec_vars_->should_be_getting_flat_) {
      // Getflat on the underlying
      options_risk_manager_->GetFlatTradingLogic(_modelmath_index_, -1);
      options_risk_manager_->CallPlaceCancelNonBestLevels(_modelmath_index_);
      for (auto i = 0u; i < options_data_matrix_[_modelmath_index_].size(); i++) {
        options_risk_manager_->GetFlatTradingLogic(_modelmath_index_, i);
        CallPlaceCancelNonBestLevels(_modelmath_index_, i);
      }
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        DBGLOG_TIME_CLASS_FUNC << current_smv_->shortcode()
                               << " Hit GlobalMaxLoss of Underlying: " << DBGLOG_ENDL_FLUSH;
      }

    } else if (options_data_matrix_[_modelmath_index_][_option_index_]->should_be_getting_flat_) {
      // GetFlat On Individual Product
      options_risk_manager_->GetFlatTradingLogic(_modelmath_index_, _option_index_);
      CallPlaceCancelNonBestLevels(_modelmath_index_, _option_index_);
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        DBGLOG_TIME_CLASS_FUNC << current_smv_->shortcode() << " Hit GlobalMaxLoss of Shortcode: " << DBGLOG_ENDL_FLUSH;
      }
    }
  }

  return false;
}

inline void BaseMultipleTrading::CallPlaceCancelNonBestLevels(int _product_index_, int _option_index_) {
  if (underlying_to_risk_matrix_[_product_index_][_option_index_].worst_position_ == 0) {
    int bid_cancelled_ = options_data_matrix_[_product_index_][_option_index_]->som_->CancelBidsBelowIntPrice(
        options_data_matrix_[_product_index_][_option_index_]->best_nonself_bid_int_price_);
    if (dbglogger_.CheckLoggingLevel(TRADING_INFO) && bid_cancelled_ > 0) {
      DBGLOG_TIME_CLASS_FUNC << " Cancelled " << bid_cancelled_ << " BidOrders" << DBGLOG_ENDL_FLUSH;
    }

    int ask_cancelled_ = options_data_matrix_[_product_index_][_option_index_]->som_->CancelAsksBelowIntPrice(
        options_data_matrix_[_product_index_][_option_index_]->best_nonself_ask_int_price_);
    if (dbglogger_.CheckLoggingLevel(TRADING_INFO) && ask_cancelled_ > 0) {
      DBGLOG_TIME_CLASS_FUNC << " Cancelled " << ask_cancelled_ << " AskOrders" << DBGLOG_ENDL_FLUSH;
    }
  } else {
    PlaceCancelNonBestLevels(_product_index_, _option_index_);
  }
}

inline void BaseMultipleTrading::PlaceCancelNonBestLevels(int _product_index_, int _option_index_) {
  // TODO :: Num non best level monitor
}

void BaseMultipleTrading::TargetNotReady(int _modelmath_index_, int _product_index_) {
  if (_product_index_ >= 0) {
    options_data_matrix_[_modelmath_index_][_product_index_]->is_ready_ = false;
    if (!dbglogger_.IsNoLogs() || livetrading_)
    {
      DBGLOG_TIME_CLASS_FUNC << "SACI: "
                             << options_data_matrix_[_modelmath_index_][_product_index_]->som_->server_assigned_client_id_
                             << " got NOTready! shc: "
                             << options_data_matrix_[_modelmath_index_][_product_index_]->smv_->shortcode()
                             << " queryid: " << runtime_id_ << DBGLOG_ENDL_FLUSH;
    }
    options_data_matrix_[_modelmath_index_][_product_index_]->external_getflat_ =
        true;  /// hack to get the query to stop trading

    if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
      DBGLOG_TIME_CLASS_FUNC << "Setting external_getflat_=true and is_ready_=false" << DBGLOG_ENDL_FLUSH;
      if (livetrading_) {
        DBGLOG_DUMP;
      }
    }
    options_risk_manager_->GetFlatTradingLogic(_modelmath_index_, _product_index_);
  } else {
    for (unsigned int const_index_ = 0; const_index_ < options_data_matrix_[_modelmath_index_].size(); const_index_++) {
      options_data_matrix_[_modelmath_index_][const_index_]->is_ready_ = false;
      if (!dbglogger_.IsNoLogs() || livetrading_)
      {
        DBGLOG_TIME_CLASS_FUNC << "SACI: "
                               << options_data_matrix_[_modelmath_index_][const_index_]->som_->server_assigned_client_id_
                               << " got NOTready! shc: "
                               << options_data_matrix_[_modelmath_index_][const_index_]->smv_->shortcode()
                               << " queryid: " << runtime_id_ << DBGLOG_ENDL_FLUSH;
      }
      options_data_matrix_[_modelmath_index_][const_index_]->external_getflat_ =
          true;  /// hack to get the query to stop trading
      options_risk_manager_->GetFlatTradingLogic(_modelmath_index_, const_index_);
    }
    underlying_data_vec_[_modelmath_index_]->is_ready_ = false;

    if(!dbglogger_.IsNoLogs() || livetrading_)
    {
      DBGLOG_TIME_CLASS_FUNC << "SACI: " << underlying_data_vec_[_modelmath_index_]->som_->server_assigned_client_id_
                             << " got NOTready! shc: " << underlying_data_vec_[_modelmath_index_]->smv_->shortcode()
                             << " queryid: " << runtime_id_ << DBGLOG_ENDL_FLUSH;
    }
    underlying_data_vec_[_modelmath_index_]->external_getflat_ = true;  /// hack to get the query to stop trading

    if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
      DBGLOG_TIME_CLASS_FUNC << "Setting external_getflat_=true and is_ready_=false" << DBGLOG_ENDL_FLUSH;
      if (livetrading_) {
        DBGLOG_DUMP;
      }
    }
    options_risk_manager_->GetFlatTradingLogic(_modelmath_index_, -1);
  }
}

int BaseMultipleTrading::GetImproveCancels() {
  return improve_keep_cancels_;
}
void BaseMultipleTrading::PrintFullStatus(int _product_index_, int _option_index_) {
  OptionVars* current_exec_vars_ = options_data_matrix_[_product_index_][_option_index_];
  DBGLOG_TIME << "shc: " << options_data_matrix_[_product_index_][_option_index_]->smv_->shortcode()
              << " pos: " << current_exec_vars_->position_ << DBGLOG_ENDL_FLUSH;
}
}
// =====================================================================================
//
