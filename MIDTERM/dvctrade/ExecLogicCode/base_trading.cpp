/*
 \file ExecLogicCode/base_trading.cpp

 \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
 Address:
 Suite No 353, Evoma, #14, Bhattarhalli,
 Old Madras Road, Near Garden City College,
 KR Puram, Bangalore 560049, India
 +91 80 4190 3551
 */

#include <sstream>

#include "dvccode/CDef/error_utils.hpp"
#include "dvccode/CDef/math_utils.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"

#include "dvccode/Profiler/cpucycle_profiler.hpp"

#include "baseinfra/VolatileTradingInfo/shortcode_ezone_vec.hpp"
#include "dvccode/ORSMessages/shortcode_ors_message_livesource_map.hpp"
#include "dvctrade/ExecLogic/base_trading.hpp"
#include "dvctrade/ExecLogic/strategy_options.hpp"
#include "dvctrade/Indicators/core_shortcodes.hpp"

// only using age based hysterisis

int HFSAT::BaseTrading::class_var_counter_ = 0;

namespace HFSAT {

void BaseTrading::CollectORSShortCodes(DebugLogger &_dbglogger_, const std::string &r_strategy_name_,
                                       const std::string &r_dep_shortcode_,
                                       std::vector<std::string> &source_shortcode_vec_,
                                       std::vector<std::string> &ors_source_needed_vec_) {
  if ((r_strategy_name_.compare(HFSAT::DirectionalAggressiveTrading::StrategyName()) == 0) ||
      (r_strategy_name_.compare(HFSAT::TradeBasedAggressiveTrading::StrategyName()) == 0) ||
      (r_strategy_name_.compare(HFSAT::DirectionalInterventionAggressiveTrading::StrategyName()) == 0) ||
      (r_strategy_name_.compare(HFSAT::DirectionalLogisticTrading::StrategyName()) == 0) ||
      (r_strategy_name_.compare(HFSAT::PriceBasedTrading::StrategyName()) == 0) ||
      (r_strategy_name_.compare(HFSAT::PriceBasedAggressiveTrading::StrategyName()) == 0) ||
      (r_strategy_name_.compare(HFSAT::PriceBasedInterventionAggressiveTrading::StrategyName()) == 0) ||
      (r_strategy_name_.compare(HFSAT::PriceBasedScalper::StrategyName()) == 0) ||
      (r_strategy_name_.compare(HFSAT::PriceBasedAggressiveScalper::StrategyName()) == 0) ||
      (r_strategy_name_.compare(HFSAT::PriceBasedVolTrading::StrategyName()) == 0) ||
      (r_strategy_name_.compare(HFSAT::RegimeBasedVolDatTrading::StrategyName()) == 0) ||
      (r_strategy_name_.compare(HFSAT::RetailTrading::StrategyName()) == 0) ||
      (r_strategy_name_.compare(HFSAT::RiskBasedStructuredTrading::StrategyName()) == 0) ||
      (r_strategy_name_.compare(HFSAT::ReturnsBasedAggressiveTrading::StrategyName()) == 0)) {
    HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, r_dep_shortcode_);
    HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, r_dep_shortcode_);
  } else if (r_strategy_name_.compare(HFSAT::PriceBasedSecurityAggressiveTrading::StrategyName()) == 0) {
    HFSAT::PriceBasedSecurityAggressiveTrading::CollectORSShortCodes(_dbglogger_, r_dep_shortcode_,
                                                                     source_shortcode_vec_, ors_source_needed_vec_);
  } else if (r_strategy_name_.compare(HFSAT::PricePairBasedAggressiveTrading::StrategyName()) == 0) {
    HFSAT::PricePairBasedAggressiveTrading::CollectORSShortCodes(_dbglogger_, r_dep_shortcode_, source_shortcode_vec_,
                                                                 ors_source_needed_vec_);
  } else if (r_strategy_name_.compare(HFSAT::PricePairBasedAggressiveTradingV2::StrategyName()) == 0) {
    HFSAT::PricePairBasedAggressiveTradingV2::CollectORSShortCodes(_dbglogger_, r_dep_shortcode_, source_shortcode_vec_,
                                                                   ors_source_needed_vec_);
  } else if (r_strategy_name_.compare(HFSAT::FEU3MM::StrategyName()) == 0) {
    HFSAT::FEU3MM::CollectORSShortCodes(_dbglogger_, r_dep_shortcode_, source_shortcode_vec_, ors_source_needed_vec_);
  } else if (r_strategy_name_.compare(HFSAT::OSEPMM::StrategyName()) == 0) {
    HFSAT::OSEPMM::CollectORSShortCodes(_dbglogger_, r_dep_shortcode_, source_shortcode_vec_, ors_source_needed_vec_);
  } else if (r_strategy_name_.compare(HFSAT::JP400PMM::StrategyName()) == 0) {
    HFSAT::JP400PMM::CollectORSShortCodes(_dbglogger_, r_dep_shortcode_, source_shortcode_vec_, ors_source_needed_vec_);
  } else if (r_strategy_name_.compare(HFSAT::ProjectedPricePairBasedAggressiveTrading::StrategyName()) == 0) {
    HFSAT::ProjectedPricePairBasedAggressiveTrading::CollectORSShortCodes(
        _dbglogger_, r_dep_shortcode_, source_shortcode_vec_, ors_source_needed_vec_);
  } else if (r_strategy_name_.compare(HFSAT::ImpliedPricePairAggressiveTrading::StrategyName()) == 0) {
    HFSAT::ImpliedPricePairAggressiveTrading::CollectORSShortCodes(_dbglogger_, r_dep_shortcode_, source_shortcode_vec_,
                                                                   ors_source_needed_vec_);
  } else if (r_strategy_name_.compare(HFSAT::ImpliedDirectionalAggressiveTrading::StrategyName()) == 0) {
    HFSAT::ImpliedDirectionalAggressiveTrading::CollectORSShortCodes(_dbglogger_, r_dep_shortcode_,
                                                                     source_shortcode_vec_, ors_source_needed_vec_);
  } else if (r_strategy_name_.compare(HFSAT::EquityTrading2::StrategyName()) == 0) {
    if (r_dep_shortcode_.substr(0, 4) == "NSE_") {
      HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, std::string("NSE_NIFTY_FUT0"));
      HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, std::string("NSE_BANKNIFTY_FUT0"));
    } else {
      HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, std::string("BR_IND_0"));
      HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, EquityTrading2::LoadProductList());
    }
    HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, r_dep_shortcode_);
    HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, r_dep_shortcode_);

  } else if (r_strategy_name_.compare(HFSAT::NKTrader::StrategyName()) == 0) {
    HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, std::string("NK_0"));
    HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, std::string("NKM_0"));
    HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, r_dep_shortcode_);
  } else {
    HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, r_dep_shortcode_);
    HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, r_dep_shortcode_);
  }
}

void BaseTrading::CollectShortCodes(DebugLogger &_dbglogger_, const std::string &paramfilename_,
                                    std::vector<std::string> &source_shortcode_vec_) {
  // Read the shortcodes requeired for regime detection  from the paramfile
  std::ifstream param_file_;
  param_file_.open(paramfilename_.c_str());
  if (param_file_.is_open()) {
    const int kParamfileListBufflerLen = 1024;
    char readlinebuffer_[kParamfileListBufflerLen];
    bzero(readlinebuffer_, kParamfileListBufflerLen);

    bool paramset_file_list_read_ = false;
    while (param_file_.good()) {
      bzero(readlinebuffer_, kParamfileListBufflerLen);
      param_file_.getline(readlinebuffer_, kParamfileListBufflerLen);
      PerishableStringTokenizer st_(readlinebuffer_, kParamfileListBufflerLen);
      const std::vector<const char *> &tokens_ = st_.GetTokens();
      if (tokens_.size() < 1) {
        continue;
      }

      if ((strcmp(tokens_[0], "PARAMFILELIST") == 0)) {
        paramset_file_list_read_ = true;
        continue;
      }

      if ((strcmp(tokens_[0], "INDICATOR") == 0) && paramset_file_list_read_ && tokens_.size() > 1) {
        // Read the indicator only if a paramlist is specified otherwise don't
        (CollectShortCodeFunc(tokens_[2]))(source_shortcode_vec_, source_shortcode_vec_, tokens_);
      }
      if (strcmp(tokens_[1], "REGIMEINDICATOR") == 0) {
        std::string ind_str = "";
        for (unsigned i = 2; i < tokens_.size(); i++) {
          if (i > 2) {
            ind_str += " ";
          }
          ind_str += tokens_[i];
        }
        const unsigned int kModelLineBufferLen = 1024;
        char buffer[kModelLineBufferLen];
        bzero(buffer, kModelLineBufferLen);
        std::string this_indicator_string = "INDICATOR 1.00 " + ind_str;
        strncpy(buffer, this_indicator_string.c_str(), this_indicator_string.length());
        PerishableStringTokenizer st_(buffer, kModelLineBufferLen);  // Perishable string readline_buffer_
        const std::vector<const char *> &ind_tokens = st_.GetTokens();
        (CollectShortCodeFunc(ind_tokens[2]))(source_shortcode_vec_, source_shortcode_vec_, ind_tokens);
      }
      if (strcmp(tokens_[1], "TRADEINDICATOR") == 0) {
        std::string ind_str = "";
        for (unsigned i = 2; i < tokens_.size(); i++) {
          if (i > 2) ind_str += " ";
          ind_str += tokens_[i];
        }
        const unsigned int kModelLineBufferLen = 1024;
        char buffer[kModelLineBufferLen];
        bzero(buffer, kModelLineBufferLen);
        std::string this_indicator_string = "INDICATOR 1.00 " + ind_str;
        strncpy(buffer, this_indicator_string.c_str(), this_indicator_string.length());
        PerishableStringTokenizer st_(buffer, kModelLineBufferLen);  // Perishable string readline_buffer_
        const std::vector<const char *> &ind_tokens = st_.GetTokens();
        (CollectShortCodeFunc(ind_tokens[2]))(source_shortcode_vec_, source_shortcode_vec_, ind_tokens);
      }
      if (strcmp(tokens_[1], "CANCELINDICATOR") == 0) {
        std::string ind_str = "";
        for (unsigned i = 2; i < tokens_.size(); i++) {
          if (i > 2) ind_str += " ";
          ind_str += tokens_[i];
        }
        const unsigned int kModelLineBufferLen = 1024;
        char buffer[kModelLineBufferLen];
        bzero(buffer, kModelLineBufferLen);
        std::string this_indicator_string = "INDICATOR 1.00 " + ind_str;
        strncpy(buffer, this_indicator_string.c_str(), this_indicator_string.length());
        PerishableStringTokenizer st_(buffer, kModelLineBufferLen);  // Perishable string readline_buffer_
        const std::vector<const char *> &ind_tokens = st_.GetTokens();
        (CollectShortCodeFunc(ind_tokens[2]))(source_shortcode_vec_, source_shortcode_vec_, ind_tokens);
      }

      if (strcmp(tokens_[1], "IMPLIEDMKTINDICATOR") == 0) {
        std::string ind_str = "";
        for (unsigned i = 2; i < tokens_.size(); i++) {
          if (i > 2) ind_str += " ";
          ind_str += tokens_[i];
        }
        const unsigned int kModelLineBufferLen = 1024;
        char buffer[kModelLineBufferLen];
        bzero(buffer, kModelLineBufferLen);
        std::string this_indicator_string = "INDICATOR 1.00 " + ind_str;
        strncpy(buffer, this_indicator_string.c_str(), this_indicator_string.length());
        PerishableStringTokenizer st_(buffer, kModelLineBufferLen);  // Perishable string readline_buffer_
        const std::vector<const char *> &ind_tokens = st_.GetTokens();
        (CollectShortCodeFunc(ind_tokens[2]))(source_shortcode_vec_, source_shortcode_vec_, ind_tokens);
      }

      if (strcmp(tokens_[1], "USE_BIGTRADES_SOURCE") == 0 && strcmp(tokens_[2], "") != 0) {
        HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, std::string(tokens_[2]));
      }
      if (strcmp(tokens_[1], "USE_BIGTRADES_PLACE_SOURCE") == 0 && strcmp(tokens_[2], "") != 0) {
        HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, std::string(tokens_[2]));
      }
      if (strcmp(tokens_[1], "USE_BIGTRADES_AGGRESS_SOURCE") == 0 && strcmp(tokens_[2], "") != 0) {
        HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, std::string(tokens_[2]));
      }
      if (strcmp(tokens_[1], "SOURCE_FOR_DMUR") == 0 && strcmp(tokens_[2], "") != 0) {
        HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, std::string(tokens_[2]));
      }
      if (strcmp(tokens_[1], "SOURCE_FOR_MKT_CONDITION") == 0 && strcmp(tokens_[2], "") != 0) {
        HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, std::string(tokens_[2]));
      }
      if (strcmp(tokens_[1], "AF_SOURCE_SHC") == 0 && strcmp(tokens_[2], "") != 0) {
        HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, std::string(tokens_[2]));
      }
      if (strcmp(tokens_[1], "SOURCE_STDEV_RISK_SCALING") == 0 && strcmp(tokens_[2], "") != 0) {
        HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, std::string(tokens_[2]));
      }
    }
  }
  // HFSAT::VectorUtils::UniqueVectorAdd ( source_shortcode_vec_, std::string ( "HSI_0" ) ) ;
  // HFSAT::VectorUtils::UniqueVectorAdd ( source_shortcode_vec_, std::string ( "HHI_0" ) ) ;
}

BaseTrading::BaseTrading(DebugLogger &t_dbglogger_, const Watch &_watch_, const SecurityMarketView &_dep_market_view_,
                         SmartOrderManager &_order_manager_, const std::string &_paramfilename_,
                         const bool _livetrading_, MulticastSenderSocket *_p_strategy_param_sender_socket_,
                         EconomicEventsManager &t_economic_events_manager_, const int t_trading_start_utc_mfm_,
                         const int t_trading_end_utc_mfm_, const int _runtime_id_,
                         const std::vector<std::string> _this_model_source_shortcode_vec_,
                         bool _is_structured_strategy_)
    : ExecInterface(t_dbglogger_, _watch_, _dep_market_view_, _order_manager_, _paramfilename_, _livetrading_),
      security_id_(_dep_market_view_.security_id()),
      p_prom_order_manager_(PromOrderManager::GetCreatedInstance(dep_market_view_.shortcode())),
      p_strategy_param_sender_socket_(_p_strategy_param_sender_socket_),
      trading_start_utc_mfm_(t_trading_start_utc_mfm_),
      trading_end_utc_mfm_(t_trading_end_utc_mfm_),
      runtime_id_(_runtime_id_),
      this_model_source_shortcode_vec_(_this_model_source_shortcode_vec_),
      current_risk_mapped_to_product_position_(0),
      my_position_(0),
      my_combined_position_(0),
      my_risk_(0),
      my_global_position_(0),
      my_combined_flat_pos_(0.0),
      map_pos_increment_(1),
      map_pos_increment_vec_(param_set_vec_.size(), 1),
      position_tradevarset_map_(2 * MAX_POS_MAP_SIZE + 1),
      position_tradevarset_map_vec_(param_set_vec_.size(), position_tradevarset_map_),
      P2TV_zero_idx_(MAX_POS_MAP_SIZE),
      closeout_zeropos_tradevarset_(),
      closeout_long_tradevarset_(),
      closeout_short_tradevarset_(),
      current_tradevarset_(),
      current_bid_tradevarset_(),
      current_ask_tradevarset_(),
      current_bid_keep_tradevarset_(),
      current_ask_keep_tradevarset_(),
      current_position_tradevarset_map_index_(MAX_POS_MAP_SIZE),
      should_be_getting_flat_(false),
      fok_flat_active_(false),
      getflat_due_to_external_getflat_(false),
      getflat_due_to_external_agg_getflat_(false),
      getflat_due_to_funds_rej_(false),
      getflat_due_to_close_(false),  // by default it is set to false, set to true in UpdateTarget or OnPositionChange
      // when watch->mfm is checked to trading_end_utc_mfm_
      getflat_due_to_max_position_(false),
      getflat_due_to_max_loss_(false),
      getflat_due_to_max_pnl_(false),
      getflat_due_to_global_max_loss_(false),
      getflat_due_to_short_term_global_max_loss_(false),
      getflat_due_to_max_opentrade_loss_(false),
      getflat_due_to_max_drawdown_(false),
      getflat_due_to_economic_times_(false),
      enable_market_data_interrupt_(true),
      getflat_due_to_market_data_interrupt_(false),
      getflat_due_to_market_status_(false),
      enable_non_standard_check_(true),
      getflat_due_to_non_standard_market_conditions_(false),
      getflat_due_to_non_tradable_events_(true),
      getflat_due_to_allowed_economic_event_(false),
      last_allowed_event_index_(0u),
      last_getflat_due_to_non_standard_market_conditions_triggered_at_msecs_(0),
      getflat_due_to_lpm_(false),
      getflat_due_to_regime_indicator_(false),
      non_standard_market_conditions_mode_prices_(),
      non_standard_market_condition_spread_(0),
      non_standard_market_condition_bid_size_(3, 0),
      non_standard_market_condition_ask_size_(3, 0),
      non_standard_market_condition_bid_orders_(3, 0),
      non_standard_market_condition_ask_orders_(3, 0),
      external_getflat_(true),  // by default true if livetrading since we need explicit yes to start trading
      rej_due_to_funds_(false),
      external_freeze_trading_(_livetrading_),  //
      freeze_due_to_exchange_stage_(false),
      freeze_due_to_funds_reject_(false),
      external_cancel_all_outstanding_orders_(_livetrading_),
      agg_getflat_(false),
      last_getflat_due_to_max_opentrade_loss_triggered_at_msecs_(0),
      last_getflat_due_to_short_term_global_max_loss_triggered_at_msecs_(0),
      break_msecs_on_max_opentrade_loss_(15 * 60 * 1000),  // waiting 15 minutes on opentrade/short_term loss hit
      // last_getflat_due_to_max_pertrade_loss_triggered_at_msecs_ ( 0 ) ,
      // break_msecs_on_max_pertrade_loss_ ( 5 * 60 * 1000 ) , // waiting 5 minutes on pertrade loss hit
      best_nonself_bid_price_(0),
      best_nonself_bid_int_price_(0),
      best_nonself_bid_size_(0),
      best_nonself_ask_price_(0),
      best_nonself_ask_int_price_(0),
      best_nonself_ask_size_(0),
      best_nonself_mid_price_(0),
      bestbid_queue_hysterisis_(0),
      bestask_queue_hysterisis_(0),
      dat_bid_reduce_l1bias_(0),
      dat_ask_reduce_l1bias_(0),
      target_price_(0),
      targetbias_numbers_(0),
      read_compute_model_stdev_(false),
      model_stdev_(0),
      flatfok_mode_(false),
      send_next_fok_time_(0),
      getflatfokmode_(false),
      is_ready_(false),
      market_status_(kMktTradingStatusOpen),
      top_bid_place_(false),
      top_bid_keep_(false),
      top_ask_place_(false),
      top_ask_keep_(false),
      top_bid_improve_(false),
      top_ask_lift_(false),
      top_ask_improve_(false),
      top_bid_hit_(false),
      placed_bids_this_round_(false),
      canceled_bids_this_round_(false),
      placed_asks_this_round_(false),
      canceled_asks_this_round_(false),
      bid_improve_keep_(false),
      ask_improve_keep_(false),
      last_non_best_level_om_msecs_(0),
      last_full_logging_msecs_(0),
      non_best_level_order_management_counter_(0u),
      control_reply_struct_(),
      economic_events_manager_(t_economic_events_manager_),
      ezone_vec_(),
      throttle_manager_(NULL),
      //
      last_buy_msecs_(0),
      last_buy_int_price_(0),
      last_sell_msecs_(0),
      last_sell_int_price_(0),
      last_agg_buy_msecs_(0),
      last_agg_sell_msecs_(0),
      //
      dump_inds(false),
      //
      stdev_(1.00),
      stdev_scaled_capped_in_ticks_(1.00),
      num_stdev_calls_(0),
      sum_stdev_calls_(0),
      //
      severity_to_getflat_on_base_(1.00),
      severity_to_getflat_on_(1.00),
      severity_change_end_msecs_(t_trading_end_utc_mfm_),
      applicable_severity_(0.00),
      zero_logging_mode_(true),
      override_zero_logging_mode_once_for_external_cmd_(false),
      last_flip_msecs_(0),
      our_global_pnl_(0),  // global PNL of this security
      our_short_term_global_pnl_(0),
      our_short_term_global_pnl_latest_(0),
      last_short_term_global_pnl_updated_msecs_(0),

      ticks_to_keep_bid_int_price_(0),
      ticks_to_keep_ask_int_price_(0),

      last_5_trade_prices_(),
      sec_counter_for_INDINFO(0),
      is_event_based_(false),
      allowed_events_present_(false),

      p_dep_indep_based_regime_(NULL),
      p_moving_bidask_spread_(NULL),

      min_msecs_to_switch_param_(900 * 1000),
      param_index_to_use_(param_set_vec_.size() - 1),
      last_param_index_update_mfm_(0),

      l1_bias_(0.0),
      l1_order_bias_(0.0),
      trade_bias_sell_(0.0),
      trade_bias_buy_(0.0),
      cancel_bias_sell_(0.0),
      cancel_bias_buy_(0.0),
      l1_ask_trade_bias_(0.0),
      l1_bid_trade_bias_(0.0),
      cancellation_model_bid_bias_(1.0),
      cancellation_model_ask_bias_(1.0),
      short_positioning_bias_(0.0),
      long_positioning_bias_(0.0),
      volume_adj_max_pos_(0.0),
      is_pair_strategy_(false),
      is_structured_strategy_(_is_structured_strategy_),
      is_structured_general_strategy_(false),
      num_opentrade_loss_hits_(0),
      max_opentrade_loss_(0),
      moving_avg_dep_bidask_spread_(1),
      last_spread_recorded_(0),
      start_not_given_(livetrading_),
      minimal_risk_position_(0),
      total_agg_flat_size_(0),
      getflat_mult_ord_(false),
      max_orders_(1),
      computing_trade_prices_(false),
      secid_global_pos_map_(NULL),
      pc1_proj_ratio_(NULL),
      is_combined_getflat_(false),
      tradingdate_(watch_.YYYYMMDD()),
      stdev_calculator_(NULL),
      should_increase_thresholds_in_volatile_times_(true),
      should_check_worst_pos_after_placecxl_(
          HFSAT::ExecLogicUtils::ToCheckWorstCaseOnPlaceCancel(dep_market_view_.shortcode())),
      pnl_sampling_timestamps_(),
      pnl_samples_(),
      sample_index_(0),
      is_affined_(false),
      last_affine_attempt_msecs_(0),
      first_affine_attempt_msecs_(0),
      position_to_add_at_start_(0),
      last_bigtrades_bid_cancel_msecs_(0),
      last_bigtrades_ask_cancel_msecs_(0),
      last_bigtrades_bid_place_msecs_(0),
      last_bigtrades_ask_place_msecs_(0),
      last_bigtrades_bid_aggress_msecs_(0),
      last_bigtrades_ask_aggress_msecs_(0),
      cancel_l1_bid_ask_flow_buy_(false),
      cancel_l1_bid_ask_flow_sell_(false),
      last_freeze_time_(0),
      freeze_due_to_rejects_(0),
      last_day_vol_ratio_(1),
      quoting_(false),
      last_msecs_quoted_(0),
      total_time_quoted_(0),
      is_alert_raised_(false),
      implied_mkt_bid_flag_(true),
      implied_mkt_ask_flag_(true),
      is_nse_earning_day_(false),
      improve_cancel_counter_(0),
      getflat_due_to_feature_model_(-1),
      improve_buy_time_stamp_vec_(),
      improve_sell_time_stamp_vec_(),
      are_log_vec_dumped_(false),
      pnl_at_close_(0),
      position_at_close_(0) {
  ///

  if (param_set_.min_order_size_ < 1) param_set_.min_order_size_ = dep_market_view_.min_order_size();

  for (auto &&param : param_set_vec_)
    if (param.min_order_size_ < 1) param.min_order_size_ = dep_market_view_.min_order_size();

  if (param_set_.read_place_cancel_cooloff_ && !livetrading_) {
    order_manager_.place_cancel_cooloff_ = param_set_.place_cancel_cooloff_;
  }

  watch_.subscribe_OnNewMidNight(this);  // probably pointless
  GetProductListToGetFlatMultOrder();

  if (param_set_.read_last_day_vol_ == true) {
    last_day_vol_ratio_ = std::min(
        std::max(1.0,
                 HFSAT::SampleDataUtil::GetAvgForPeriod(dep_market_view_.shortcode(), tradingdate_, 1, "STDEV") /
                     HFSAT::SampleDataUtil::GetAvgForPeriod(dep_market_view_.shortcode(), tradingdate_, 60, "STDEV")),
        param_set_.last_day_vol_factor_);
    stdev_scaled_capped_in_ticks_ = std::min(param_set_.stdev_overall_cap_, last_day_vol_ratio_);
  }
  product_ = new InstrumentInfo();
  SecurityMarketView *nonconst_smv =
      (ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(dep_market_view_.shortcode()));

  exec_logic_indicators_helper_ =
      new IndicatorHelper(dbglogger_, watch_, product_, nonconst_smv, &order_manager_, trading_start_utc_mfm_,
                          trading_end_utc_mfm_, runtime_id_, livetrading_);
  exec_logic_indicators_helper_->AddIndicatorHelperListener(this);

  tradevarset_builder_ = new TradeVarSetBuilder(dbglogger_, watch_, livetrading_);

  class_var_counter_++;
  pid_file_ = std::string(OUTPUT_LOGDIR) +
              HFSAT::ExchSourceStringForm(
                  HFSAT::SecurityDefinitions::GetContractExchSource(dep_market_view_.shortcode(), tradingdate_)) +
              "_" + std::to_string(runtime_id_) + "_PIDfile.txt";
  exit_bool_set_ = false;
  first_obj_ = (class_var_counter_ == 1);
  exit_cool_off_ = 0;

  if (p_prom_order_manager_) {
    p_prom_order_manager_->ManageOrdersAlso();
  }

  if (dbglogger_.CheckLoggingLevel(TRADEINIT_INFO)) {  // print book to screen
    printf("\033c");                                   // clear screen
    last_update_ttime_.tv_sec = last_update_ttime_.tv_usec = 0;
  }

  // Inform the order manager about bid and ask price-levels
  // to never cancel orders on.
  order_manager_.SetKeepBidIntPrice(&ticks_to_keep_bid_int_price_);
  order_manager_.SetKeepAskIntPrice(&ticks_to_keep_ask_int_price_);

  CheckParamSet();

  if (param_set_vec_.size() > 1)  // Multiple params exist
  {
    exec_logic_indicators_helper_->ComputeRegimes(regime_indicator_string_);
  }
  param_index_to_use_ = param_set_vec_.size() - 1;
  param_set_ = param_set_vec_[param_index_to_use_];

  if (param_set_.use_stable_bidask_levels_) {
    _dep_market_view_.subscribe_price_type(this, kPriceTypeValidLevelMidPrice);
    _dep_market_view_.ComputeValidLevelMidPrice();
    _dep_market_view_.use_stable_bidask_levels_ = true;  // best_nonself prices are based on  valid sizes sizes
  }

  if (param_set_.use_throttle_manager_) {
    throttle_manager_ =
        new HFSAT::ThrottleManager(param_set_.throttle_message_limit_);  // create for all queries, overhead: bool check
    throttle_manager_->start_throttle_manager(true);                     // use only when needed
  } else if (!param_set_.is_common_param_) {
    std::cerr << " ThrottleMessageLimit Not specified " << dep_market_view_.shortcode()
              << " . Trying to load it from offline-file." << std::endl;
    int throttle_message = ExecLogicUtils::GetThrottleForShortcode(dep_market_view_.shortcode(), trading_start_utc_mfm_,
                                                                   trading_end_utc_mfm_);
    if (throttle_message > 0) {
      throttle_manager_ = new HFSAT::ThrottleManager(throttle_message);  // create for all queries, overhead: bool check
      throttle_manager_->start_throttle_manager(true);
    }
  }
  _order_manager_.SetOrderManager(throttle_manager_);  // passed to BOM, so that both exec_logic and OM can access it.

  // Sets disclosed size factor., if the param SIZE_DISCLOSED_FACTOR exists.
  if (param_set_.read_size_disclosed_factor_ && dep_market_view_.shortcode() == "NSE_USDINR_FUT0") {
    std::cout << "SizeDiscFactor: " << param_set_.size_disclosed_factor_ << " "
              << param_set_.read_size_disclosed_factor_ << "\n";
    std::cout << "Setting disclosed volume factor \n";
    SetDisclosedFactor(param_set_.size_disclosed_factor_);
  }

  economic_events_manager_.AdjustSeverity(dep_market_view_.shortcode(), dep_market_view_.exch_source());
  economic_events_manager_.AllowEconomicEventsFromList(dep_market_view_.shortcode());
  allowed_events_present_ = economic_events_manager_.AllowedEventsPresent();

  if (param_set_.ezone_vec_[0] != EZ_MAX) {  // if given in param then just use that
    for (size_t i = 0; i < EZONE_MAXLEN; i++) {
      if (param_set_.ezone_vec_[i] < EZ_MAX) {
        ezone_vec_.push_back(param_set_.ezone_vec_[i]);
      }
    }
    DBGLOG_CLASS_FUNC << "From Param : Stopping for EZones:";
    for (auto i = 0u; i < ezone_vec_.size(); i++) {
      dbglogger_ << ' ' << GetStrFromEconomicZone(ezone_vec_[i]);
    }
    dbglogger_ << DBGLOG_ENDL_FLUSH;
  } else {
    // assume that all products in this sim are the same ... i mean same shortcode
    // for that shortcode .. depending on time of day calculate the numbers to stop for
    // ezones_all_events_ .. the economic szones such that all events with severity >= 1 we should stop for
    // ezones_strong_events_ ... the economic zones where we should stop in periods wherethe applicable severity of all
    // events of that period >= 2
    // ezones_super_events_ ... only severity >= 3 events
    // after getting these event vectors
    // ask economic times manager to precompute the times when we should stop and when we should not

    GetEZVecForShortcode(dep_market_view_.shortcode(), t_trading_start_utc_mfm_, ezone_vec_);
    DBGLOG_CLASS_FUNC << "After checking at mfm " << t_trading_start_utc_mfm_ << " Stopping for EZones:";
    for (auto i = 0u; i < ezone_vec_.size(); i++) {
      dbglogger_ << ' ' << GetStrFromEconomicZone(ezone_vec_[i]);
    }
    dbglogger_ << DBGLOG_ENDL_FLUSH;

    //( t_trading_end_utc_mfm_ - 1 ) to avoid events overlapping period for EU and US
    GetEZVecForShortcode(dep_market_view_.shortcode(), t_trading_end_utc_mfm_ - 1, ezone_vec_);
    DBGLOG_CLASS_FUNC << "After checking at mfm " << (t_trading_end_utc_mfm_ - 1) << " Stopping for EZones:";
    for (auto i = 0u; i < ezone_vec_.size(); i++) {
      dbglogger_ << ' ' << GetStrFromEconomicZone(ezone_vec_[i]);
    }
    dbglogger_ << DBGLOG_ENDL_FLUSH;
  }

  if (param_set_.read_severity_to_getflat_on_) {
    severity_to_getflat_on_base_ = param_set_.severity_to_getflat_on_;
    severity_to_getflat_on_ = severity_to_getflat_on_base_;
  }

  if (param_set_.read_break_msecs_on_max_opentrade_loss_) {
    break_msecs_on_max_opentrade_loss_ = std::max(1 * 60 * 1000, param_set_.break_msecs_on_max_opentrade_loss_);
  }
  for (unsigned i = 0; i < param_set_vec_.size(); i++) {
    if (param_set_vec_[i].online_stats_avg_dep_bidask_spread_ && p_moving_bidask_spread_ == NULL) {
      p_moving_bidask_spread_ = MovingAvgBidAskSpread::GetUniqueInstance(dbglogger_, watch_, &_dep_market_view_, 300.0);
    }
  }

  if (param_set_.online_stats_avg_dep_bidask_spread_ && p_moving_bidask_spread_ == NULL) {
    p_moving_bidask_spread_ = MovingAvgBidAskSpread::GetUniqueInstance(dbglogger_, watch_, &_dep_market_view_, 300.0);
  }

  for (auto i = 0u; i < param_set_vec_.size(); i++) {
    param_set_vec_[i].break_msecs_on_max_opentrade_loss_ =
        break_msecs_on_max_opentrade_loss_;  // in case somebody accesses via param_set
  }
  param_set_.break_msecs_on_max_opentrade_loss_ =
      param_set_vec_[param_index_to_use_].break_msecs_on_max_opentrade_loss_;

  if (!param_set_.is_common_param_ && !is_structured_strategy_ && !is_structured_general_strategy_) {
    // Build only if it's not a structured strat and param is not common

    if (param_set_.read_interday_scaling_factor_) {
      ChangeThresholdsByInterdayScalingFactor();
    }

    BuildTradeVarSets();
    BuildPosToThreshMap();
  }

  int t_max_loss_ = 0;
  for (auto i = 0u; i < param_set_vec_.size(); i++) {
    if (max_opentrade_loss_ < param_set_vec_[i].max_opentrade_loss_)
      max_opentrade_loss_ = param_set_vec_[i].max_opentrade_loss_;

    if (t_max_loss_ < param_set_vec_[i].max_loss_) t_max_loss_ = param_set_vec_[i].max_loss_;
  }
  for (auto i = 0u; i < param_set_vec_.size(); i++) {
    param_set_vec_[i].max_opentrade_loss_ = max_opentrade_loss_;  // in case somebody accesses opentrade via param_set
    param_set_vec_[i].max_loss_ = t_max_loss_;  // in case pick_strats put in different max_losses while installing
    param_set_vec_[i].getflat_aggress_ *= dep_market_view_.min_price_increment();
  }
  param_set_.max_opentrade_loss_ = param_set_vec_[param_index_to_use_].max_opentrade_loss_;
  param_set_.max_loss_ = param_set_vec_[param_index_to_use_].max_loss_;
  param_set_.getflat_aggress_ *= dep_market_view_.min_price_increment();

  for (auto i = 0u; i < param_set_vec_.size(); i++) {
    // multiply place inline parameter longevity_support_ with min_price_increment since
    // this is going to be used in a comparison where all other factors are in numbers ( exchange prices )
    // whereas this was specified in the paramfile in terms of increments or ticks
    if (param_set_vec_[i].longevity_support_ > 0.0) {
      param_set_vec_[i].longevity_support_ *= dep_market_view_.min_price_increment();
    }
    param_set_vec_[i].spread_increase_ *= dep_market_view_.min_price_increment();
  }

  for (auto i = 0u; i < param_set_vec_.size(); i++) {
    if (param_set_vec_[i].read_l1_trade_volume_bias_) {
      exec_logic_indicators_helper_->ComputeL1TradeVolumeBias(&param_set_vec_[i]);
    }
  }
  param_set_.l1_trade_volume_bias_ = param_set_vec_[param_index_to_use_].l1_trade_volume_bias_;

  param_set_.longevity_support_ = param_set_vec_[param_index_to_use_].longevity_support_;
  param_set_.spread_increase_ = param_set_vec_[param_index_to_use_].spread_increase_;

  exec_logic_indicators_helper_->ComputeLongevity(&param_set_);

  dep_market_view_.subscribe_OnReady((SecurityMarketViewOnReadyListener *)this);

  dep_market_view_.subscribe_price_type(this, kPriceTypeMktSizeWPrice);
  watch_.subscribe_BigTimePeriod(this);

  bool to_set_slow_stdev_calc_ = false;

  unsigned int stdev_duration_ = 100u;

  for (auto i = 0u; i < param_set_vec_.size(); i++) {
    param_set_vec_[i].stdev_fact_ticks_ = param_set_vec_[i].stdev_fact_ / dep_market_view_.min_price_increment();
    param_set_vec_[i].low_stdev_lvl_ *= dep_market_view_.min_price_increment();

    if (  // needed for increasing thresholds
        ((param_set_vec_[i].stdev_cap_ > 1) && (param_set_vec_[i].stdev_fact_ > 0)) ||
        // needed for placing orders
        (param_set_vec_[i].low_stdev_lvl_ > 0)) {
      // these are general situations, individual execlogics may impose more liberal conditions
      to_set_slow_stdev_calc_ = true;
    }

    stdev_duration_ = std::max(param_set_vec_[i].stdev_duration_, stdev_duration_);
    param_set_vec_[i].improve_ticks_ = param_set_vec_[i].improve_ / dep_market_view_.min_price_increment();
  }
  param_set_.stdev_fact_ticks_ = param_set_vec_[param_index_to_use_].stdev_fact_ticks_;
  param_set_.low_stdev_lvl_ = param_set_vec_[param_index_to_use_].low_stdev_lvl_;

  for (unsigned i = 0; i < param_set_vec_.size(); i++) {
    if (param_set_vec_[i].read_stdev_quote_factor_) {
      to_set_slow_stdev_calc_ = true;
      stdev_duration_ = std::max(param_set_vec_[i].stdev_duration_, stdev_duration_);
    }
  }
  if (to_set_slow_stdev_calc_) {
    stdev_calculator_ = SlowStdevCalculator::GetUniqueInstance(t_dbglogger_, _watch_, _dep_market_view_.shortcode(),
                                                               stdev_duration_ * 1000u);
    stdev_calculator_->AddSlowStdevCalculatorListener(this);
  }

  // commenting variable since this is not used.
  // SecurityMarketView &t_dep_market_view_nonconst_ =
  //     *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(dep_market_view_.shortcode()));

  if (param_set_.should_stdev_suppress_non_best_level_) {
    exec_logic_indicators_helper_->ComputeStdevTriggerForPCNBL(&param_set_);
  }

  if (param_set_.read_bigtrades_cancel_ && param_set_.bigtrades_source_id_.empty()) {
    exec_logic_indicators_helper_->ComputeBigTrades(&param_set_);
  }

  if (!param_set_.bigtrades_source_id_.empty()) {
    exec_logic_indicators_helper_->ComputeBigTradesIndep(&param_set_);
  }

  if (param_set_.read_bigtrades_place_) {
    if (param_set_.bigtrades_place_source_id_.empty()) {
      param_set_.bigtrades_place_source_id_ = _dep_market_view_.shortcode();
    }
    exec_logic_indicators_helper_->ComputeBigTradesPlaceIndep(&param_set_);
  }

  if (param_set_.read_bigtrades_aggress_) {
    if (param_set_.bigtrades_aggress_source_id_.empty()) {
      param_set_.bigtrades_aggress_source_id_ = _dep_market_view_.shortcode();
    }
    exec_logic_indicators_helper_->ComputeBigTradesAggressIndep(&param_set_);
  }
  if (param_set_.read_l1_bid_ask_flow_cancel_thresh_) {
    exec_logic_indicators_helper_->ComputeL1BidAskSizeFlow(&param_set_);
  }

  if (param_set_.read_regime_indicator_string_ && param_set_.read_regimes_to_trade_) {
    exec_logic_indicators_helper_->ComputeRegimesToTrade(&param_set_);
  }

  if (param_set_.read_trade_bias_ && param_set_.read_trade_indicator_string_) {
    exec_logic_indicators_helper_->ComputeTradeBias(&param_set_);
  }
  if (param_set_.read_cancel_bias_ && param_set_.read_cancel_indicator_string_) {
    exec_logic_indicators_helper_->ComputeCancelBias(&param_set_);
  }

  if (param_set_.read_implied_mkt_ && param_set_.read_implied_mkt_indicator_string_) {
    exec_logic_indicators_helper_->ComputeImpliedPrice(&param_set_);
  }
  if (param_set_.read_src_based_exec_changes_) {
    exec_logic_indicators_helper_->ComputeFlagsAsPerMktCondition(&param_set_);
  }

  // if any of the param has set to online model stdev as true
  // then it will compute the stdev
  // and it will use the first duration
  for (unsigned i = 0; i < param_set_vec_.size(); i++) {
    if (param_set_vec_[i].online_model_stdev_ && !read_compute_model_stdev_) {
      model_stdev_decay_page_factor_ = MathUtils::CalcDecayFactor(param_set_vec_[i].model_stdev_duration_);
      model_stdev_inv_decay_sum_ = (1 - model_stdev_decay_page_factor_);
      moving_avg_sumvars_ = param_set_.offline_model_stdev_;
      moving_avg_squared_sumvars_ = param_set_.offline_model_stdev_ * param_set_.offline_model_stdev_;
      read_compute_model_stdev_ = true;
    }
  }

  // these field of control_reply_struct_ will never be changed
  memcpy(control_reply_struct_.symbol_, dep_market_view_.secname(), kSecNameLen);
  control_reply_struct_.trader_id_ = runtime_id_;

  if (dbglogger_.CheckLoggingLevel(TRADING_ERROR))
  // not really error, escalating to error for printouts in normal case
  {
    ShowParams();
  }

  if ((!_livetrading_) && (param_set_.read_max_loss_)) {  // only in sim
    order_manager_.base_pnl().SetMaxLoss(-param_set_.max_loss_);
  }

  for (auto i = 0u; i < param_set_vec_.size(); i++) {
    if ((!_livetrading_) && (param_set_vec_[i].read_max_global_position_)) {
      param_set_vec_[i].read_max_global_position_ = false;
    }
  }
  param_set_.read_max_global_position_ = param_set_vec_[param_index_to_use_].read_max_global_position_;
  for (auto i = 0u; i < param_set_vec_.size(); i++) {
    param_set_vec_[i].high_spread_allowance_ *= dep_market_view_.min_price_increment();
  }
  param_set_.high_spread_allowance_ = param_set_vec_[param_index_to_use_].high_spread_allowance_;

  {
    std::vector<std::string> core_shortcodes_;
    GetCoreShortcodes(dep_market_view_.shortcode(), core_shortcodes_);
    for (auto i = 0u; i < this_model_source_shortcode_vec_.size(); i++) {
      this_model_source_shortcode_to_datainterrupt_map_[this_model_source_shortcode_vec_[i]] =
          (VectorUtils::LinearSearchValue(core_shortcodes_,
                                          this_model_source_shortcode_vec_[i])
               ? 1
               : 0);  // both in model and core
    }
  }

  for (auto i = 0u; i < param_set_vec_.size(); i++) {
    if (param_set_vec_[i].read_max_min_diff_) {
      exec_logic_indicators_helper_->ComputeL1SizeBias(&param_set_vec_[i]);
    }

    if (param_set_vec_[i].read_high_uts_factor_) {
      exec_logic_indicators_helper_->ComputeHighUTSFactor(&param_set_vec_[i]);
    }
  }
  param_set_.max_min_diff_ = param_set_vec_[param_index_to_use_].max_min_diff_;

  for (auto i = 0u; i < param_set_vec_.size(); i++) {
    if (param_set_vec_[i].read_max_min_diff_order_) {
      exec_logic_indicators_helper_->ComputeL1OrderBias(&param_set_vec_[i]);
    }
  }
  param_set_.max_min_diff_order_ = param_set_vec_[param_index_to_use_].max_min_diff_order_;

  for (auto i = 0u; i < param_set_vec_.size(); i++) {
    if (param_set_vec_[i].read_bucket_size_ && param_set_vec_[i].read_positioning_thresh_decrease_) {
      exec_logic_indicators_helper_->ComputePositioning(&param_set_vec_[i]);
      if ((int32_t)i == param_index_to_use_) {
        param_set_.positioning_indicator_ = param_set_vec_[i].positioning_indicator_;
      }
    }
  }
  param_set_.positioning_thresh_decrease_ = param_set_vec_[param_index_to_use_].positioning_thresh_decrease_;

  for (auto i = 0u; i < param_set_vec_.size(); i++) {
    if (param_set_vec_[i].read_scale_max_pos_) {
      exec_logic_indicators_helper_->ComputeScaledMaxPos(&param_set_vec_[i]);
    }
  }
  param_set_.volume_ratio_indicator_ = param_set_vec_[param_index_to_use_].volume_ratio_indicator_;
  param_set_.volume_lower_bound_ = param_set_vec_[param_index_to_use_].volume_lower_bound_;
  param_set_.volume_upper_bound_ = param_set_vec_[param_index_to_use_].volume_upper_bound_;
  param_set_.volume_norm_factor_ = param_set_vec_[param_index_to_use_].volume_norm_factor_;

  if (livetrading_) {
    SetGetFlatFokMode();
  }

  for (auto i = 0u; i < param_set_vec_.size(); i++) {
    if (param_set_vec_[i].should_get_combined_flat) {
      is_combined_getflat_ = true;
      break;
    }
  }

  // not necessarily needed but so that variables have consistent values
  for (auto i = 0u; i < param_set_vec_.size(); i++) {
    param_set_vec_[i].should_get_combined_flat = is_combined_getflat_;
  }

  // resetting in case we have missed updating param_set_ while updating param_set_vec_
  // TODO: convert param_set_ into a pointer
  param_set_ = param_set_vec_[param_index_to_use_];

  if (is_combined_getflat_) {
    InitialisePC1Risk();
  }

  ProcessGetFlat();

  int sampling_interval_msecs_ = HFSAT::ExecLogicUtils::GetSamplingIntervalForPnlSeries(dep_market_view_.shortcode());

  int t_sampling_start_utc_mfm_ = MathUtils::GetFlooredMultipleOf(t_trading_start_utc_mfm_, sampling_interval_msecs_);
  int t_sampling_end_utc_mfm_ = MathUtils::GetCeilMultipleOf(
      t_trading_end_utc_mfm_ + 60000, sampling_interval_msecs_);  // Adding 1 min to incorporate getflat also

  for (int sampling_mfm_ = t_sampling_start_utc_mfm_ + sampling_interval_msecs_;
       sampling_mfm_ <= t_sampling_end_utc_mfm_; sampling_mfm_ += sampling_interval_msecs_) {
    pnl_sampling_timestamps_.push_back(sampling_mfm_);
  }

  if (ExecLogicUtils::IsEarnings(dep_market_view_.shortcode(), tradingdate_)) {
    is_nse_earning_day_ = true;
    should_be_getting_flat_ = true;
  }

  if (param_set_.l1bias_model_stdev_ > 0) {
    exec_logic_indicators_helper_->InitiateStdevL1Bias(&param_set_);
  }
}

void BaseTrading::ChangeThresholdsByInterdayScalingFactor() {
  param_set_.zeropos_keep_ *= param_set_.interday_scaling_factor_;
  param_set_.zeropos_place_ *= param_set_.interday_scaling_factor_;
  param_set_.increase_keep_ *= param_set_.interday_scaling_factor_;
  param_set_.increase_place_ *= param_set_.interday_scaling_factor_;
  for (unsigned int i = 0; i < param_set_vec_.size(); i++) {
    param_set_vec_[i].zeropos_keep_ *= param_set_vec_[i].interday_scaling_factor_;
    param_set_vec_[i].zeropos_place_ *= param_set_vec_[i].interday_scaling_factor_;
    param_set_vec_[i].increase_place_ *= param_set_vec_[i].interday_scaling_factor_;
    param_set_vec_[i].increase_keep_ *= param_set_vec_[i].interday_scaling_factor_;
  }
}

void BaseTrading::CheckParamSet() {
  for (auto i = 0u; i < param_set_vec_.size(); i++) {
    if (param_set_vec_[i].is_common_param_) {
      continue;
    }
    if (!param_set_vec_[i].read_worst_case_position_) {
      ExitVerbose(kStrategyDescParamFileIncomplete, "worst_case_position_");
    }
    if (!param_set_vec_[i].read_max_position_) {
      ExitVerbose(kStrategyDescParamFileIncomplete, "max_position_");
    }
    if (!param_set_vec_[i].read_unit_trade_size_) {
      ExitVerbose(kStrategyDescParamFileIncomplete, "unit_trade_size_");
    }
    if (!param_set_vec_[i].read_highpos_limits_ && !param_set_vec_[i].read_highpos_limits_unit_ratio_) {
      ExitVerbose(kStrategyDescParamFileIncomplete, "highpos_limits_ | highpos_limits_unit_ratio_");
    }
    if (!param_set_vec_[i].read_highpos_thresh_factor_) {
      ExitVerbose(kStrategyDescParamFileIncomplete, "highpos_thresh_factor_");
    }
    if (!param_set_vec_[i].read_highpos_size_factor_) {
      ExitVerbose(kStrategyDescParamFileIncomplete, "highpos_size_factor_");
    }
    if (!param_set_vec_[i].read_increase_place_) {
      ExitVerbose(kStrategyDescParamFileIncomplete, "increase_place_");
    }
    if (!param_set_vec_[i].read_increase_keep_) {
      ExitVerbose(kStrategyDescParamFileIncomplete, "increase_keep_");
    }
    if (!param_set_vec_[i].read_zeropos_limits_ && !param_set_vec_[i].read_zeropos_limits_unit_ratio_) {
      ExitVerbose(kStrategyDescParamFileIncomplete, "zeropos_limits_ | zeropos_limits_unit_ratio_");
    }
    if (!param_set_vec_[i].read_zeropos_place_) {
      ExitVerbose(kStrategyDescParamFileIncomplete, "zeropos_place_");
    }
    if (!param_set_vec_[i].read_zeropos_keep_) {
      ExitVerbose(kStrategyDescParamFileIncomplete, "zeropos_keep_");
    }
    if (!param_set_vec_[i].read_decrease_place_) {
      ExitVerbose(kStrategyDescParamFileIncomplete, "decrease_place_");
    }
    if (!param_set_vec_[i].read_decrease_keep_) {
      ExitVerbose(kStrategyDescParamFileIncomplete, "decrease_keep_");
    }
    if (!param_set_vec_[i].read_max_loss_) {
      ExitVerbose(kStrategyDescParamFileIncomplete, "max_loss_");
    }
    if (!param_set_vec_[i].read_max_opentrade_loss_) {
      ExitVerbose(kStrategyDescParamFileIncomplete, "max_opentrade_loss_");
    }
    if (param_set_vec_[i].unit_trade_size_ % dep_market_view_.min_order_size() != 0) {
      ExitVerbose(kStrategyDescParamFileIncomplete, "uts is not a multiple of min_order_size");
    }
  }
}

void BaseTrading::BuildTradeVarSets() {
  tradevarset_builder_->BuildConstantTradeVarSets(&param_set_, &dep_market_view_, closeout_zeropos_tradevarset_,
                                                  closeout_long_tradevarset_, closeout_short_tradevarset_);
  ;
  current_tradevarset_ = closeout_zeropos_tradevarset_;
  for (unsigned i = 0; i < param_set_vec_.size(); i++) {
    tradevarset_builder_->BuildPositionTradeVarSetMap(&param_set_vec_[i], &dep_market_view_,
                                                      position_tradevarset_map_vec_[i], map_pos_increment_vec_[i],
                                                      P2TV_zero_idx_, livetrading_);
    if ((int)i == param_index_to_use_) {
      // param_set_vec_ is changed thus param_set_ should be updated
      param_set_.max_position_ = param_set_vec_[param_index_to_use_].max_position_;
    }
  }
  position_tradevarset_map_ = position_tradevarset_map_vec_[param_index_to_use_];  // start with default value
  map_pos_increment_ = map_pos_increment_vec_[param_index_to_use_];
}

void BaseTrading::BuildPosToThreshMap() {
  int max_pos_possible_ = std::max(param_set_.max_position_, param_set_.worst_case_position_);
  pos_to_thresh_bid_place_.resize(2 * max_pos_possible_ + 1, 0);
  pos_to_thresh_ask_place_.resize(2 * max_pos_possible_ + 1, 0);
  pos_to_thresh_bid_keep_.resize(2 * max_pos_possible_ + 1, 0);
  pos_to_thresh_ask_keep_.resize(2 * max_pos_possible_ + 1, 0);

  for (int i = 0; i <= 2 * max_pos_possible_; i++) {
    int pos = i - max_pos_possible_;
    int t_lvl_ = abs(int(pos / param_set_.unit_trade_size_));

    if (pos < 0) {
      pos_to_thresh_bid_place_[i] = param_set_.thresh_place_ - t_lvl_ * param_set_.thresh_decrease_;
      pos_to_thresh_ask_place_[i] = param_set_.thresh_place_ + t_lvl_ * param_set_.thresh_increase_;
      pos_to_thresh_bid_keep_[i] =
          param_set_.thresh_place_ - param_set_.thresh_place_keep_diff_ - t_lvl_ * param_set_.thresh_decrease_;
      pos_to_thresh_ask_keep_[i] =
          param_set_.thresh_place_ - param_set_.thresh_place_keep_diff_ + t_lvl_ * param_set_.thresh_increase_;
    } else {
      pos_to_thresh_bid_place_[i] = param_set_.thresh_place_ + t_lvl_ * param_set_.thresh_increase_;
      pos_to_thresh_ask_place_[i] = param_set_.thresh_place_ - t_lvl_ * param_set_.thresh_decrease_;
      pos_to_thresh_bid_keep_[i] =
          param_set_.thresh_place_ - param_set_.thresh_place_keep_diff_ + t_lvl_ * param_set_.thresh_increase_;
      pos_to_thresh_ask_keep_[i] =
          param_set_.thresh_place_ - param_set_.thresh_place_keep_diff_ - t_lvl_ * param_set_.thresh_decrease_;
    }
  }
}

// by default not zeroing out other side
void BaseTrading::OnCancelReject(const TradeType_t _buysell_, const double _price_, const int r_int_price_,
                                 const int _security_id_) {
  if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
    dump_inds = true;
  }
  if (!is_structured_general_strategy_) {
    if (_buysell_ == kTradeTypeBuy) {
      if (is_structured_strategy_ && my_risk_ >= 0) {  // cooloff buys only if >=0 after this buy
        last_buy_msecs_ = watch_.msecs_from_midnight();
        last_buy_int_price_ = r_int_price_;
      }
      last_sell_msecs_ = 0;  // if we just bought then we should allow ourselves to sell again
    } else if (_buysell_ == kTradeTypeSell) {
      if (is_structured_strategy_ && my_risk_ <= 0) {  // cooloff sells only if position <= 0 after this sell
        last_sell_msecs_ = watch_.msecs_from_midnight();
        last_sell_int_price_ = r_int_price_;
      }
      last_buy_msecs_ = 0;
    } else {
      last_buy_msecs_ = 0;
      last_sell_msecs_ = 0;
    }
    last_exec_msecs_ = watch_.msecs_from_midnight();
  }
}

void BaseTrading::OnRejectDueToFunds(const TradeType_t _buysell_) {
  dbglogger_ << "OnRejectDueToFunds in basetrading \n";
  if ((_buysell_ == kTradeTypeBuy && my_position_ < 0) || (_buysell_ == kTradeTypeSell && my_position_ > 0)) {
    freeze_due_to_funds_reject_ = true;
    order_manager_.CancelAllOrders();
    dbglogger_ << "OnRejectDueToFunds - freeze \n";
  } else {
    rej_due_to_funds_ = true;
    ProcessGetFlat();
    dbglogger_ << "OnRejectDueToFunds - getflat \n";
  }
  dbglogger_.DumpCurrentBuffer();
}

void BaseTrading::OnWakeUpifRejectDueToFunds() {
  rej_due_to_funds_ = false;
  freeze_due_to_funds_reject_ = false;
  ProcessGetFlat();
}

void BaseTrading::ComputeCurrentSeverity() {
  applicable_severity_ = 0;
  for (auto i = 0u; i < ezone_vec_.size(); i++) {
    applicable_severity_ += economic_events_manager_.GetCurrentSeverity(ezone_vec_[i]);
  }

  if (economic_events_manager_.IsEventHappening() && !computing_trade_prices_) {
    SetComputeTradePrice(true);
  } else if ((!economic_events_manager_.IsEventHappening()) && (computing_trade_prices_)) {
    SetComputeTradePrice(false);
  }

  if (watch_.msecs_from_midnight() > severity_change_end_msecs_ &&
      severity_to_getflat_on_ != severity_to_getflat_on_base_) {
    severity_to_getflat_on_ = severity_to_getflat_on_base_;
    DBGLOG_TIME << "Seteco set to " << severity_to_getflat_on_ << " at time " << watch_.msecs_from_midnight()
                << DBGLOG_ENDL_FLUSH;
    severity_change_end_msecs_ = trading_end_utc_mfm_;
    ProcessGetFlat();
  }
}

void BaseTrading::HandleFreeze() {
  if (HFSAT::TradingStageManager::GetUniqueInstance(dbglogger_, watch_, dep_market_view_.shortcode(),
                                                    dep_market_view_.exch_source())->ShouldFreezeNow()) {
    if (!freeze_due_to_exchange_stage_) {  // if this has not already been done... send cancel... else nothing
      // set bool true
      freeze_due_to_exchange_stage_ = true;
      order_manager_.CancelAllOrders();
      p_base_model_math_->DumpIndicatorValues();
      DBGLOG_TIME << "Freeze due to exchange state in " << dep_market_view_.shortcode() << DBGLOG_ENDL_FLUSH;
      if (livetrading_) {
        DBGLOG_DUMP;
      }
    }
  } else {
    // set bool false
    if (freeze_due_to_exchange_stage_) {
      freeze_due_to_exchange_stage_ = false;
      DBGLOG_TIME << "UnFreeze " << DBGLOG_ENDL_FLUSH;
      p_base_model_math_->DumpIndicatorValues();
      ProcessGetFlat();  // this is what we want for VX
    }
  }
}

// TODO - logging and notification
void BaseTrading::OnGetFreezeDueToExchangeRejects(HFSAT::BaseUtils::FreezeEnforcedReason const &freeze_reason) {
  if (livetrading_) {
    char hostname[128];
    hostname[127] = '\0';
    gethostname(hostname, 127);

    std::ostringstream t_temp_oss;
    t_temp_oss << "Query - " << runtime_id_ << " Has Received GetFreeze Due To "
               << HFSAT::BaseUtils::FreezeEnforcedReasonString(freeze_reason) << " On : " << hostname;
    if (freeze_reason == BaseUtils::FreezeEnforcedReason::kFreezeOnNoResponseFromORS) {
      t_temp_oss << " Query Not Receiving any response from ORS, will not start without manual intervention..";
    }

    HFSAT::Email e;
    e.setSubject(t_temp_oss.str());
    e.addRecepient("nseall@tworoads.co.in");
    e.addSender("nseall@tworoads.co.in");
    e.sendMail();

    DBGLOG_TIME_CLASS_FUNC << "Received GetFreeze Due To Rejects.." << DBGLOG_ENDL_FLUSH;
    if (freeze_reason == BaseUtils::FreezeEnforcedReason::kFreezeOnNoResponseFromORS) {
      DBGLOG_TIME_CLASS_FUNC << order_manager_.ShowOrders() << DBGLOG_ENDL_FLUSH;
    }

    DBGLOG_DUMP;

    // We'll remain in freeze mode until,
    // 1) Either an unfreeze is given externally
    // 2) We reach timeout
    if (false == freeze_due_to_rejects_) {
      external_freeze_trading_ = true;
      freeze_due_to_rejects_ = true;
      last_freeze_time_ = watch_.msecs_from_midnight();
    }
  }
}

// TODO - logging and notification
void BaseTrading::OnGetFreezeDueToORSRejects() {
  if (livetrading_) {
    char hostname[128];
    hostname[127] = '\0';
    gethostname(hostname, 127);

    std::ostringstream t_temp_oss;
    t_temp_oss << "Query - " << runtime_id_ << " Has Received GetFreeze Due To ORS Rejects On : " << hostname;

    HFSAT::Email e;
    e.setSubject(t_temp_oss.str());
    e.addRecepient("nseall@tworoads.co.in");
    e.addSender("nseall@tworoads.co.in");
    e.sendMail();

    DBGLOG_TIME_CLASS_FUNC_LINE << "Received Getflat Due To ORS Rejects..." << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;

    // We'll remain in freeze mode until,
    // 1) Either an unfreeze is given externally
    // 2) We reach timeout
    if (false == freeze_due_to_rejects_) {
      external_freeze_trading_ = true;
      freeze_due_to_rejects_ = true;
      last_freeze_time_ = watch_.msecs_from_midnight();
    }
  }
}

void BaseTrading::OnResetByManualInterventionOverRejects() {
  char hostname[128];
  hostname[127] = '\0';
  gethostname(hostname, 127);

  std::ostringstream t_temp_oss;
  t_temp_oss << "Query - " << runtime_id_ << " Received Auto/External Unfreeze From Rejects On : " << hostname;

  HFSAT::Email e;
  e.setSubject(t_temp_oss.str());
  e.addRecepient("nseall@tworoads.co.in");
  e.addSender("nseall@tworoads.co.in");
  e.sendMail();

  freeze_due_to_rejects_ = false;
  external_freeze_trading_ = false;
}

void BaseTrading::OnExec(const int t_new_position_, const int _exec_quantity_, const TradeType_t _buysell_,
                         const double _price_, const int r_int_price_, const int _security_id_) {
  if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
    DBGLOG_TIME_CLASS_FUNC << " OnExec: "
                           << " npos : " << t_new_position_ << " qty : " << _exec_quantity_ << " px : " << _price_
                           << DBGLOG_ENDL_FLUSH;

    if (t_new_position_ != my_position_) {
      dump_inds = true;
    }
  }
  // for debug code INdicator info
  if (dbglogger_.CheckLoggingLevel(INDICATOR_INFO)) {
    char buf[1024] = {0};
    // print the trade file line check for flat or open
    if (t_new_position_ == 0) {
      sprintf(
          buf, "INDINFO %10d.%06d FLAT %s %c %4d %.7f %4d %8d %8d [ %5d %f X %f %5d ] [%.3f %.3f %.3f %.3f] ",
          watch_.tv().tv_sec, watch_.tv().tv_usec, order_manager_.base_pnl().numbered_secname(),
          GetTradeTypeChar(_buysell_), _exec_quantity_, _price_, 0, 0,
          (int)round(order_manager_.base_pnl().opentrade_unrealized_pnl() + order_manager_.base_pnl().realized_pnl()),
          dep_market_view_.bestbid_size(), dep_market_view_.bestbid_price(), dep_market_view_.bestask_price(),
          dep_market_view_.bestask_size(), current_tradevarset_.l1bid_place_, current_tradevarset_.l1bid_keep_,
          current_tradevarset_.l1ask_place_, current_tradevarset_.l1ask_keep_);
      dbglogger_ << buf;
    } else {
      sprintf(
          buf, "INDINFO %10d.%06d OPEN %s %c %4d %.7f %4d %8d %8d [ %5d %f X %f %5d ] [%.3f %.3f %.3f %.3f] ",
          watch_.tv().tv_sec, watch_.tv().tv_usec, order_manager_.base_pnl().numbered_secname(),
          GetTradeTypeChar(_buysell_), _exec_quantity_, _price_, t_new_position_,
          (int)round(order_manager_.base_pnl().opentrade_unrealized_pnl()),
          (int)round(order_manager_.base_pnl().opentrade_unrealized_pnl() + order_manager_.base_pnl().realized_pnl()),
          dep_market_view_.bestbid_size(), dep_market_view_.bestbid_price(), dep_market_view_.bestask_price(),
          dep_market_view_.bestask_size(), current_tradevarset_.l1bid_place_, current_tradevarset_.l1bid_keep_,
          current_tradevarset_.l1ask_place_, current_tradevarset_.l1ask_keep_);
      dbglogger_ << buf;
    }

    p_base_model_math_->DumpIndicatorContribution();

    dbglogger_.CheckToFlushBuffer();
  }

  if (t_new_position_ > my_position_) {
    if (t_new_position_ >= 0) {  // only cooloff buys if we are long
      last_buy_msecs_ = watch_.msecs_from_midnight();
      last_buy_int_price_ = r_int_price_;
    }

    last_sell_msecs_ = 0;
  } else if (t_new_position_ < my_position_) {
    if (t_new_position_ <= 0) {
      last_sell_msecs_ = watch_.msecs_from_midnight();
      last_sell_int_price_ = r_int_price_;
    }

    last_buy_msecs_ = 0;
  }
}

void BaseTrading::LoadOvernightPositions() {
  std::ifstream eod_position_file;
  std::ostringstream filepath;
  filepath << "/home/dvctrader/overnight_pnls_read.txt";
  eod_position_file.open(filepath.str().c_str(), std::ifstream::in);
  if (eod_position_file.is_open()) {
    const int kL1AvgBufferLen = 1024;
    char readline_buffer_[kL1AvgBufferLen];
    bzero(readline_buffer_, kL1AvgBufferLen);
    eod_position_file.getline(readline_buffer_, kL1AvgBufferLen);
    while (eod_position_file.good()) {
      char *str = readline_buffer_;
      char *pch;
      printf("Splitting string \"%s\" into tokens:\n", readline_buffer_);
      pch = strtok(str, ",");
      if (pch != NULL) {
        for (auto i = 0u; i < dep_market_view_.sec_name_indexer_.NumSecurityId(); i++) {
          if (std::string(dep_market_view_.sec_name_indexer_.GetSecurityNameFromId(i)).compare(std::string(pch)) == 0) {
            pch = strtok(NULL, " ,");
            my_combined_flat_pos_ = my_combined_flat_pos_ + atoi(pch) * pc1_proj_ratio_[i] / param_set_.number_strats;
            if (dbglogger_.CheckLoggingLevel(TRADING_ERROR) && getflat_due_to_close_) {
              DBGLOG_TIME << "pc1 risk :  " << my_combined_flat_pos_ << " sec_id : " << i
                          << " global_pos : " << atoi(pch) << " round_pos: " << std::round(my_combined_flat_pos_)
                          << DBGLOG_ENDL_FLUSH;
              if (livetrading_) {
                dbglogger_ << DBGLOG_ENDL_FLUSH;
              }
            }
          }
        }
      }
      bzero(readline_buffer_, kL1AvgBufferLen);
      eod_position_file.getline(readline_buffer_, kL1AvgBufferLen);
    }
  }
}

void BaseTrading::InitialisePC1Risk() {
  std::ifstream t_combined_flat_model_;
  std::ostringstream filepath;
  secid_global_pos_map_ = new int[dep_market_view_.sec_name_indexer_.NumSecurityId()];
  pc1_proj_ratio_ = new double[dep_market_view_.sec_name_indexer_.NumSecurityId()];
  for (auto i = 0u; i < dep_market_view_.sec_name_indexer_.NumSecurityId(); i++) {
    pc1_proj_ratio_[i] = 0;
    secid_global_pos_map_[i] = 0;
  }
  filepath << "/spare/local/tradeinfo/CombinedGetFlatInfo/Models/" << param_set_.combined_get_flat_model;
  if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
    DBGLOG_TIME << "/spare/local/tradeinfo/CombinedGetFlatInfo/Models/" << param_set_.combined_get_flat_model
                << DBGLOG_ENDL_FLUSH;
    if (livetrading_) {
      DBGLOG_DUMP;
    }
  }

  t_combined_flat_model_.open(filepath.str().c_str(), std::ifstream::in);
  if (t_combined_flat_model_.is_open()) {
    const int kL1AvgBufferLen = 1024;
    char readline_buffer_[kL1AvgBufferLen];
    bzero(readline_buffer_, kL1AvgBufferLen);
    t_combined_flat_model_.getline(readline_buffer_, kL1AvgBufferLen);
    std::vector<std::string> correlated_tokens_;

    if (t_combined_flat_model_.good()) {
      PerishableStringTokenizer st_(readline_buffer_, kL1AvgBufferLen);
      const std::vector<const char *> &tokens_ = st_.GetTokens();
      for (unsigned int i = 1; i < tokens_.size(); i++) {
        std::string this_token = tokens_[i];
        correlated_tokens_.push_back(this_token);
      }
      bzero(readline_buffer_, kL1AvgBufferLen);
      t_combined_flat_model_.getline(readline_buffer_, kL1AvgBufferLen);
    }
    while (t_combined_flat_model_.good()) {
      PerishableStringTokenizer st_(readline_buffer_, kL1AvgBufferLen);
      const std::vector<const char *> &tokens_ = st_.GetTokens();
      if (tokens_.size() > 0) {
        if (dep_market_view_.shortcode().compare(tokens_[0]) == 0) {
          for (unsigned int i = 1; i < tokens_.size(); i++) {
            if (i <= correlated_tokens_.size()) {
              int sec_id = dep_market_view_.sec_name_indexer_.GetIdFromString(correlated_tokens_[i - 1]);
              if (sec_id >= 0) {
                pc1_proj_ratio_[sec_id] = atof(tokens_[i]);
                secid_global_pos_map_[sec_id] = 0;
              } else {
                DBGLOG_TIME << "Security :  " << correlated_tokens_[i - 1] << " not present in sec_name_indexer"
                            << DBGLOG_ENDL_FLUSH;
                if (livetrading_) {
                  DBGLOG_DUMP;
                }
              }
              if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
                if (sec_id >= 0) {
                  DBGLOG_TIME << "pc1 proj ratio :  " << pc1_proj_ratio_[sec_id] << " sec_id : " << sec_id
                              << " num_strats: " << param_set_.number_strats << DBGLOG_ENDL_FLUSH;
                }
                if (livetrading_) {
                  DBGLOG_DUMP;
                }
              }
            }
          }
          break;
        }
      } else {
        ExitVerbose(kExitErrorCodeGeneral, "InComplete Combined Get Flat Model");
      }
      t_combined_flat_model_.getline(readline_buffer_, kL1AvgBufferLen);
    }
  } else {
    is_combined_getflat_ = false;
  }
  t_combined_flat_model_.close();
  // LoadOvernightPositions();
}

void BaseTrading::UpdatePC1Risk(int sec_id, int new_position_) {
  int pos_change_ = new_position_ - secid_global_pos_map_[sec_id];
  secid_global_pos_map_[sec_id] = new_position_;
  my_combined_flat_pos_ = my_combined_flat_pos_ + pos_change_ * pc1_proj_ratio_[sec_id] / param_set_.number_strats;
  if (dbglogger_.CheckLoggingLevel(TRADING_INFO) && getflat_due_to_close_) {
    DBGLOG_TIME << "pc1 risk :  " << my_combined_flat_pos_ << " sec_id : " << sec_id
                << " global_pos : " << new_position_ << " round_pos: " << std::round(my_combined_flat_pos_)
                << DBGLOG_ENDL_FLUSH;
    if (livetrading_) {
      dbglogger_ << DBGLOG_ENDL_FLUSH;
    }
  }
  if ((!external_freeze_trading_) && (!freeze_due_to_exchange_stage_) && (!freeze_due_to_funds_reject_)) {
    ProcessGetFlat();  // TODO ... see if this needs to be here ... I mean not much changes in get flat conditions
    // when
    // the signal is updating
    if (should_be_getting_flat_) {
      GetFlatFokTradingLogic();
      if (livetrading_ || (dbglogger_.CheckLoggingLevel(TRADING_INFO))) {
        LogFullStatus();
      }
    }
  }
}

void BaseTrading::OnGlobalPositionChange(const unsigned int _security_id_, int _new_global_position_) {
  my_global_position_ = _new_global_position_;
  if (!is_structured_general_strategy_ && is_ready_) TradeVarSetLogic(my_position_);

  if (is_combined_getflat_) {
    UpdatePC1Risk(_security_id_, _new_global_position_);
  }
}

void BaseTrading::OnGlobalPNLChange(double _new_global_pnl_) {
  our_global_pnl_ = _new_global_pnl_;
  if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
    DBGLOG_TIME_CLASS_FUNC << " GlobalPNL: " << our_global_pnl_ << DBGLOG_ENDL_FLUSH;
  }
}

void BaseTrading::OnPositionChange(int t_new_position_, int position_diff_, const unsigned int _security_id_) {
  if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
    // debug info ... Only in LiveTrading
    if (t_new_position_ != my_position_) {
      dump_inds = true;
    }
  }

  // SetPostionTradeVars ( ); // Set the apropriate position tradevars based on regime

  // recompute ticks_to_keep_bid_int_price_ &
  // ticks_to_keep_ask_int_price_ based on how the average-position trade price has changed
  if ((param_set_.num_increase_ticks_to_keep_ > 0 || param_set_.num_decrease_ticks_to_keep_ > 0) &&
      order_manager_.base_pnl().AverageOpenPrice() >
          0) {                  // won't enter for paramsets w/o these specifications ( majority )
    if (t_new_position_ > 0) {  // now long

      if (param_set_.num_increase_ticks_to_keep_ > 0) {
        // price level to keep an order on which will increase position on execution.
        ticks_to_keep_bid_int_price_ =
            ((order_manager_.base_pnl().AverageOpenPrice() + dep_market_view_.min_price_increment() * 0.5) /
             dep_market_view_.min_price_increment()) -
            param_set_.num_increase_ticks_to_keep_;

        if (ticks_to_keep_bid_int_price_ > best_nonself_bid_int_price_) {
          ticks_to_keep_bid_int_price_ = best_nonself_bid_int_price_;
        }
      }

      if (param_set_.num_decrease_ticks_to_keep_ > 0) {
        // price level to keep an order on which will decrease position on execution.
        ticks_to_keep_ask_int_price_ =
            ((order_manager_.base_pnl().AverageOpenPrice() + dep_market_view_.min_price_increment() * 0.5) /
             dep_market_view_.min_price_increment()) +
            param_set_.num_decrease_ticks_to_keep_;

        if (ticks_to_keep_ask_int_price_ < best_nonself_ask_int_price_) {
          ticks_to_keep_ask_int_price_ = best_nonself_ask_int_price_;
        }
      }
    } else if (t_new_position_ < 0) {  // now short

      if (param_set_.num_increase_ticks_to_keep_ > 0) {
        // price level to keep an order on which will increase position on execution.
        ticks_to_keep_ask_int_price_ =
            ((order_manager_.base_pnl().AverageOpenPrice() - dep_market_view_.min_price_increment() * 0.5) /
             dep_market_view_.min_price_increment()) +
            param_set_.num_increase_ticks_to_keep_;

        if (ticks_to_keep_ask_int_price_ < best_nonself_ask_int_price_) {
          ticks_to_keep_ask_int_price_ = best_nonself_ask_int_price_;
        }
      }

      if (param_set_.num_decrease_ticks_to_keep_ > 0) {
        // price level to keep an order on which will decrease position on execution.
        ticks_to_keep_bid_int_price_ =
            ((order_manager_.base_pnl().AverageOpenPrice() - dep_market_view_.min_price_increment() * 0.5) /
             dep_market_view_.min_price_increment()) -
            param_set_.num_decrease_ticks_to_keep_;

        if (ticks_to_keep_bid_int_price_ > best_nonself_bid_int_price_) {
          ticks_to_keep_bid_int_price_ = best_nonself_bid_int_price_;
        }
      }
    }

    if (t_new_position_) {  // Place keep orders.

      if (ticks_to_keep_bid_int_price_ &&
          (ticks_to_keep_bid_int_price_ <= best_nonself_bid_int_price_) &&  // don't wish to aggress on this param.
          order_manager_.GetTotalBidSizeOrderedAtIntPx(ticks_to_keep_bid_int_price_) ==
              0) {  // if no orders at this level
        order_manager_.SendTradeIntPx(ticks_to_keep_bid_int_price_,
                                      position_tradevarset_map_[P2TV_zero_idx_].l1bid_trade_size_, kTradeTypeBuy, 'S');
        if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {  // elevate to error-level till satisfied
          DBGLOG_TIME_CLASS_FUNC << " TicksToKeepBidIntPrice SupportingSendTrade B of "
                                 << position_tradevarset_map_[P2TV_zero_idx_].l1bid_trade_size_ << " @ "
                                 << (dep_market_view_.min_price_increment() * ticks_to_keep_bid_int_price_)
                                 << " IntPx: " << ticks_to_keep_bid_int_price_
                                 << " level: " << (best_nonself_bid_int_price_ - ticks_to_keep_bid_int_price_)
                                 << " mkt: " << best_nonself_bid_size_ << " @ " << best_nonself_bid_price_ << " X "
                                 << best_nonself_ask_price_ << " @ " << best_nonself_ask_size_ << " sumbidsizes "
                                 << order_manager_.SumBidSizes() << " + my_position_ " << my_position_
                                 << DBGLOG_ENDL_FLUSH;
        }
      }
      if (ticks_to_keep_ask_int_price_ &&
          (ticks_to_keep_ask_int_price_ >= best_nonself_ask_int_price_) &&  // don't wish to aggress on this param.
          order_manager_.GetTotalAskSizeOrderedAtIntPx(ticks_to_keep_ask_int_price_) ==
              0) {  // if no orders at this level
        order_manager_.SendTradeIntPx(ticks_to_keep_ask_int_price_,
                                      position_tradevarset_map_[P2TV_zero_idx_].l1ask_trade_size_, kTradeTypeSell, 'S');
        if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {  // elevate to error-level till satisfied
          DBGLOG_TIME_CLASS_FUNC << " TicksToKeepAskIntPrice SupportingSendTrade S of "
                                 << position_tradevarset_map_[P2TV_zero_idx_].l1ask_trade_size_ << " @ "
                                 << (dep_market_view_.min_price_increment() * ticks_to_keep_ask_int_price_)
                                 << " IntPx: " << ticks_to_keep_ask_int_price_
                                 << " level: " << (ticks_to_keep_ask_int_price_ - best_nonself_ask_int_price_)
                                 << " mkt: " << best_nonself_bid_size_ << " @ " << best_nonself_bid_price_ << " X "
                                 << best_nonself_ask_price_ << " @ " << best_nonself_ask_size_ << " sumasksizes "
                                 << order_manager_.SumAskSizes() << " + my_position_ " << my_position_
                                 << DBGLOG_ENDL_FLUSH;
        }
      }
    }

    if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {  // elevate to error-level till satisfied
      DBGLOG_TIME_CLASS_FUNC << " t_new_position_ = " << t_new_position_
                             << " AverageOpenPrice = " << order_manager_.base_pnl().AverageOpenPrice() << " mkt: [ "
                             << best_nonself_bid_int_price_ << " X " << best_nonself_ask_int_price_ << " ]"
                             << " ticks_to_keep_bid_int_price_ = " << ticks_to_keep_bid_int_price_
                             << " ticks_to_keep_ask_int_price_ = " << ticks_to_keep_ask_int_price_ << DBGLOG_ENDL_FLUSH;
    }
  }

  /// for PriceBasedScalper
  if (!is_pair_strategy_) {
    if (t_new_position_ * my_position_ <= 0) {
      last_flip_msecs_ = watch_.msecs_from_midnight();
    }
  }

  my_position_ = t_new_position_;
  UpdateBetaPosition(_security_id_, t_new_position_);

  // TODO check for issues here

  if (param_set_.read_scale_max_pos_) {
    volume_adj_max_pos_ = exec_logic_indicators_helper_->volume_adj_max_pos(&param_set_);
  }

  if (!is_structured_general_strategy_ && is_ready_)
    TradeVarSetLogic(my_position_);  ///< since position changed the applicable tradevarset might have changed

  // see if trades need to be placed because of getflat condition
  if ((!external_freeze_trading_) && (!freeze_due_to_exchange_stage_) && (!freeze_due_to_funds_reject_)) {
    ProcessGetFlat();

    if (should_be_getting_flat_) {
      GetFlatFokTradingLogic();
      if (livetrading_ || (dbglogger_.CheckLoggingLevel(TRADING_INFO))) {
        LogFullStatus();
      }
    }
  }
}

bool BaseTrading::SetPositionOffset(int t_position_offset_) {
  DBGLOG_TIME_CLASS_FUNC_LINE << "BaseTrading::SetPositionOffset " << dep_market_view_.shortcode() << " "
                              << t_position_offset_ << DBGLOG_ENDL_FLUSH;

  t_position_offset_ = MathUtils::GetFlooredMultipleOf(t_position_offset_, dep_market_view_.min_order_size());

  char hostname_[128];
  hostname_[127] = '\0';
  gethostname(hostname_, 127);
  std::ostringstream t_oss_;

  if (t_position_offset_ == 0) {
    return true;
  } else if ((std::abs(my_position_ + t_position_offset_) > param_set_.max_global_risk_) && (livetrading_)) {
    t_oss_ << "Strategy: " << runtime_id_ << " addposition warning: [my_pos + offset > max_risk : " << my_position_
           << " + " << t_position_offset_ << " > " << param_set_.max_global_risk_ << " ] for product "
           << dep_market_view_.shortcode() << " = " << dep_market_view_.secname() << " on " << hostname_ << "\n";
    std::string getflat_email_string_ = t_oss_.str();
    SendMail(getflat_email_string_, getflat_email_string_);
  }

  double t_exec_price_ = t_position_offset_ > 0 ? best_nonself_bid_price_ : best_nonself_ask_price_;

  if ((!dep_market_view_.is_ready())) {
    t_oss_ << "Strategy: " << runtime_id_ << " addposition warning: Book not yet ready for product "
           << dep_market_view_.shortcode() << " = " << dep_market_view_.secname() << " on " << hostname_
           << ": Position will be added when Book will be ready.\n";
    std::string getflat_email_string_ = t_oss_.str();
    position_to_add_at_start_ += t_position_offset_;
    if (livetrading_) {
      SendMail(getflat_email_string_, getflat_email_string_);
    }
  } else {
    order_manager_.AddPosition(t_position_offset_, t_exec_price_);
  }

  return true;
}

void BaseTrading::OnOrderChange() {
  if (is_ready_ && !external_freeze_trading_ && !should_be_getting_flat_ && dep_market_view_.is_ready() &&
      (throttle_manager_->allowed_through_throttle(watch_.msecs_from_midnight()))) {
    TradingLogic();
  }
  exec_logic_indicators_helper_->RecomputeHysterisis();
}

void BaseTrading::OnFokFill(const TradeType_t _buysell_, const double _price_, const int r_int_price_,
                            const int size_exec_) {
  if (!flatfok_mode_) return;

  DBGLOG_TIME_CLASS_FUNC << " Fok fill received at IntPrice " << r_int_price_ << DBGLOG_ENDL_FLUSH;

  GetFlatFokTradingLogic();
}

void BaseTrading::OnFokReject(const TradeType_t _buysell_, const double _price_, const int r_int_price_,
                              const int size_exec_) {
  if (!flatfok_mode_) return;

  DBGLOG_TIME_CLASS_FUNC << " Fok Reject received at IntPrice " << r_int_price_ << DBGLOG_ENDL_FLUSH;

  GetFlatFokTradingLogic();
}

void BaseTrading::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {
  std::string shc_not_ready = dep_market_view_.sec_name_indexer_.GetShortcodeFromId(_security_id_);
  DBGLOG_TIME << " market_data_interrupt_ for " << shc_not_ready << DBGLOG_ENDL_FLUSH;

  std::vector<std::string> core_shortcodes_;
  GetCoreShortcodes(dep_market_view_.shortcode(), core_shortcodes_);
  GetSessionCoreShortcodes(dep_market_view_.shortcode(), core_shortcodes_, trading_start_utc_mfm_);
  if (this_model_source_shortcode_to_datainterrupt_map_.find(shc_not_ready) !=
      this_model_source_shortcode_to_datainterrupt_map_.end()) {
    if (VectorUtils::LinearSearchValue(core_shortcodes_, shc_not_ready) &&
        VectorUtils::LinearSearchValue(this_model_source_shortcode_vec_, shc_not_ready)) {
      this_model_source_shortcode_to_datainterrupt_map_[shc_not_ready] = 2;  // important and data interrupted
    } else {
      this_model_source_shortcode_to_datainterrupt_map_[shc_not_ready] = 0;  // not important
    }
  }

  bool anyone_interrupted_ = false;
  for (std::map<std::string, int>::iterator map_iter_ = this_model_source_shortcode_to_datainterrupt_map_.begin();
       map_iter_ != this_model_source_shortcode_to_datainterrupt_map_.end(); map_iter_++) {
    if (VectorUtils::LinearSearchValue(core_shortcodes_, map_iter_->first) &&
        VectorUtils::LinearSearchValue(this_model_source_shortcode_vec_, map_iter_->first)) {
      if (map_iter_->second == 2) {  // if it was marked as imprtant and not ready then recheck if it is important
        anyone_interrupted_ = true;
      }
    } else {
      map_iter_->second = 0;
    }
  }

  if (anyone_interrupted_ && (!getflat_due_to_market_data_interrupt_)) {
    // putting the check against enable_market_data_interrupt_ here
    // allows the internal variables to be modified as they should be
    // but we do not print / email getflat_due_to_market_data_interrupt_ ,
    // since we do not actually getflat.
    if (dbglogger_.CheckLoggingLevel(TRADING_ERROR) && enable_market_data_interrupt_) {
      DBGLOG_TIME << " getflat_due_to_market_data_interrupt_ of " << shc_not_ready << " for QueryID: " << runtime_id_
                  << DBGLOG_ENDL_FLUSH;
      if (livetrading_) {
        DBGLOG_DUMP;
      }

      bool sending_md_interrupt_email_ = true;
      if (sending_md_interrupt_email_ && livetrading_ &&  // live-trading and within trading window
          (watch_.msecs_from_midnight() > trading_start_utc_mfm_) &&
          (watch_.msecs_from_midnight() < trading_end_utc_mfm_)) {  // Send out an email / alert
        char hostname_[128];
        hostname_[127] = '\0';
        gethostname(hostname_, 127);

        std::string getflat_email_string_ = "";
        {
          std::ostringstream t_oss_;
          t_oss_ << "Strategy: " << runtime_id_ << " getflat_due_to_market_data_interrupt_ of " << shc_not_ready
                 << " for"
                 << " product " << dep_market_view_.shortcode() << " = " << dep_market_view_.secname() << " on "
                 << hostname_ << "\n";

          getflat_email_string_ = t_oss_.str();
        }

        //        SendMail(getflat_email_string_, getflat_email_string_);
      }
    }
    getflat_due_to_market_data_interrupt_ = true;
    ProcessGetFlat();
  }

  if (!anyone_interrupted_ && getflat_due_to_market_data_interrupt_) {
    if (dbglogger_.CheckLoggingLevel(TRADING_ERROR))  // Do not print if not resuming from a getflat
    {
      DBGLOG_TIME << " resume normal NO_market_data_interrupt_ " << DBGLOG_ENDL_FLUSH;
      if (livetrading_) {
        DBGLOG_DUMP;
      }
    }
    getflat_due_to_market_data_interrupt_ = false;
    ProcessGetFlat();
  }

  ProcessGetFlat();
}

void BaseTrading::OnMarketDataResumed(const unsigned int _security_id_) {
  std::string shc_ready =
      dep_market_view_.sec_name_indexer_.GetShortcodeFromId(_security_id_);  // data resumed for this shortcode

  std::vector<std::string> core_shortcodes_;
  GetCoreShortcodes(dep_market_view_.shortcode(), core_shortcodes_);
  if (this_model_source_shortcode_to_datainterrupt_map_.find(shc_ready) !=
      this_model_source_shortcode_to_datainterrupt_map_.end()) {
    if (VectorUtils::LinearSearchValue(core_shortcodes_, shc_ready) &&
        VectorUtils::LinearSearchValue(this_model_source_shortcode_vec_, shc_ready)) {
      this_model_source_shortcode_to_datainterrupt_map_[shc_ready] = 1;
    } else {
      this_model_source_shortcode_to_datainterrupt_map_[shc_ready] = 0;
    }
  }

  std::string shc_not_ready = "";

  bool anyone_interrupted_ = false;
  for (std::map<std::string, int>::iterator map_iter_ = this_model_source_shortcode_to_datainterrupt_map_.begin();
       map_iter_ != this_model_source_shortcode_to_datainterrupt_map_.end(); map_iter_++) {
    if (VectorUtils::LinearSearchValue(core_shortcodes_, map_iter_->first) &&
        VectorUtils::LinearSearchValue(this_model_source_shortcode_vec_, map_iter_->first)) {
      if (map_iter_->second == 2) {  // if it was marked as imprtant and not ready then recheck if it is important
        anyone_interrupted_ = true;
        shc_not_ready = map_iter_->first;
      }
    } else {
      map_iter_->second = 0;
    }
  }

  if (anyone_interrupted_ && (!getflat_due_to_market_data_interrupt_)) {
    // putting the check against enable_market_data_interrupt_ here
    // allows the internal variables to be modified as they should be
    // but we do not print / email getflat_due_to_market_data_interrupt_ ,
    // since we do not actually getflat.
    if (dbglogger_.CheckLoggingLevel(TRADING_ERROR) && enable_market_data_interrupt_) {
      DBGLOG_TIME << " getflat_due_to_market_data_interrupt_ for QueryID: " << runtime_id_ << DBGLOG_ENDL_FLUSH;
      if (livetrading_) {
        DBGLOG_DUMP;
      }

      bool sending_md_interrupt_email_ = true;
      if (sending_md_interrupt_email_ && livetrading_ &&  // live-trading and within trading window
          (watch_.msecs_from_midnight() > trading_start_utc_mfm_) &&
          (watch_.msecs_from_midnight() < trading_end_utc_mfm_)) {  // Send out an email / alert
        char hostname_[128];
        hostname_[127] = '\0';
        gethostname(hostname_, 127);

        std::string getflat_email_string_ = "";
        {
          std::ostringstream t_oss_;
          t_oss_ << "Strategy: " << runtime_id_ << " getflat_due_to_market_data_interrupt_ of " << shc_not_ready
                 << " for"
                 << " product " << dep_market_view_.shortcode() << " = " << dep_market_view_.secname() << " on "
                 << hostname_ << "\n";

          getflat_email_string_ = t_oss_.str();
        }

        //        SendMail(getflat_email_string_, getflat_email_string_);
      }
    }
    getflat_due_to_market_data_interrupt_ = true;
    ProcessGetFlat();
  }

  if (!anyone_interrupted_ && getflat_due_to_market_data_interrupt_) {
    if (dbglogger_.CheckLoggingLevel(TRADING_ERROR))  // Do not print if not resuming from a getflat
    {
      DBGLOG_TIME << " resume normal NO_market_data_interrupt_ of " << shc_ready << DBGLOG_ENDL_FLUSH;
      if (livetrading_) {
        DBGLOG_DUMP;
      }
    }
    getflat_due_to_market_data_interrupt_ = false;
    ProcessGetFlat();
  }
}

/// Make sure that all messages are handled in a way such that receiving them again
/// does not cause any problems.
void BaseTrading::OnControlUpdate(const ControlMessage &_control_message_, const char *symbol, const int trader_id) {
  switch (_control_message_.message_code_) {
    case kControlMessageCodeGetflat:
    case kControlMessageCodeGetFlatOnThisHost: {
      if (!external_getflat_) {
        if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
          DBGLOG_TIME << "getflat_due_to_external_getflat_ " << runtime_id_ << DBGLOG_ENDL_FLUSH;
          if (livetrading_) {
            DBGLOG_DUMP;
          }
        }
        external_getflat_ = true;
        ProcessGetFlat();  // called since should_be_getting_flat_ might change in value since external_getflat_ and
                           // hence getflat_due_to_external_getflat_ might have changed
      }
    } break;
    case kControlMessageCodeAggGetflat: {
      if (!agg_getflat_) {
        if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
          DBGLOG_TIME << "getflat_due_to_external_agg_getflat_ " << runtime_id_ << DBGLOG_ENDL_FLUSH;
          if (livetrading_) {
            DBGLOG_DUMP;
          }
          agg_getflat_ = true;
          total_agg_flat_size_ = 0;
        }
      }
    } break;
    case kControlMessageCodeStartTrading: {
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME_CLASS << "StartTrading Called " << runtime_id_ << DBGLOG_ENDL_FLUSH;
        if (livetrading_) {
          DBGLOG_DUMP;
        }
      }
      external_getflat_ = false;
      agg_getflat_ = false;
      getflat_due_to_external_agg_getflat_ = false;
      getflat_due_to_external_getflat_ = false;
      external_cancel_all_outstanding_orders_ = false;

      ProcessGetFlat();  // calling this since external_getflat_ and hence getflat_due_to_external_getflat_ might have
      // changed

      order_manager_.UnsetCancelAllOrders(); /* cancelallorders flag in ordermanager is used to indicate that
       unconfirmed orders are to be canceled as soon as they are confirmed.
       Hence we need to unset that */
      external_freeze_trading_ = false;      // need to unfreeze since otherwise will nto be able to send orders
      start_not_given_ = false;
      freeze_due_to_rejects_ = false;
      order_manager_.ResetRejectsBasedFreeze();
    } break;
    case kControlMessageCodeFreezeTrading: {
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME_CLASS << "FreezeTrading called " << runtime_id_ << DBGLOG_ENDL_FLUSH;
        if (livetrading_) {
          DBGLOG_DUMP;
        }
      }
      external_freeze_trading_ = true;
    } break;
    case kControlMessageCodeUnFreezeTrading: {
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME_CLASS << "UnFreezeTrading called " << runtime_id_ << DBGLOG_ENDL_FLUSH;
        if (livetrading_) {
          DBGLOG_DUMP;
        }
      }
      external_freeze_trading_ = false;
      freeze_due_to_rejects_ = false;
      order_manager_.ResetRejectsBasedFreeze();
    } break;
    case kControlMessageCodeCancelAllFreezeTrading: {
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME_CLASS << "CancelAllFreezeTrading called " << runtime_id_ << DBGLOG_ENDL_FLUSH;
        if (livetrading_) {
          DBGLOG_DUMP;
        }
      }
      // On receinving this, we will not be placing any new orders.
      // We will just cancel old orders
      external_cancel_all_outstanding_orders_ = true;
      order_manager_.CancelAllOrders(); /* cancelallorders flag in ordermanager is used to indicate that unconfirmed
       orders are to be canceled as soon as they are confirmed. Hence we need to set
       that */
      external_freeze_trading_ = true;
    } break;
    case kControlMessageCodeSetTradeSizes: {
      for (auto i = 0u; i < param_set_vec_.size(); i++) {
        if (_control_message_.intval_1_ > param_set_vec_[i].unit_trade_size_ / FAT_FINGER_FACTOR &&
            _control_message_.intval_1_ < param_set_vec_[i].unit_trade_size_ * FAT_FINGER_FACTOR) {
          param_set_vec_[i].unit_trade_size_ = std::max(1, _control_message_.intval_1_);
        }
        if (_control_message_.intval_2_ > param_set_vec_[i].max_position_ / FAT_FINGER_FACTOR &&
            _control_message_.intval_2_ < param_set_vec_[i].max_position_ * FAT_FINGER_FACTOR) {
          param_set_vec_[i].max_position_ = std::max(1, _control_message_.intval_2_);
          param_set_vec_[i].max_position_original_ = param_set_vec_[i].max_position_;
        }
        if (_control_message_.intval_3_ > param_set_vec_[i].worst_case_position_ / FAT_FINGER_FACTOR &&
            _control_message_.intval_3_ < param_set_vec_[i].worst_case_position_ * FAT_FINGER_FACTOR) {
          param_set_vec_[i].worst_case_position_ = std::max(1, _control_message_.intval_3_);
        }

        if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
          DBGLOG_TIME_CLASS << "SetTradeSizes " << runtime_id_ << " called with UnitSize "
                            << _control_message_.intval_1_ << " maxpos " << _control_message_.intval_2_
                            << " and worst case pos " << _control_message_.intval_3_ << DBGLOG_ENDL_FLUSH;
          dbglogger_ << "Final values set for param " << i << " are UnitSize" << param_set_vec_[i].unit_trade_size_
                     << " MaxPos " << param_set_vec_[i].max_position_ << " and WorstCasePos "
                     << param_set_vec_[i].worst_case_position_ << DBGLOG_ENDL_FLUSH;
          if (livetrading_) {
            DBGLOG_DUMP;
          }
        }
      }

      param_set_.unit_trade_size_ = param_set_vec_[param_index_to_use_].unit_trade_size_;
      param_set_.max_position_ = param_set_vec_[param_index_to_use_].max_position_;
      param_set_.max_position_original_ = param_set_.max_position_;
      param_set_.worst_case_position_ = param_set_vec_[param_index_to_use_].worst_case_position_;
      order_manager_.NotifyWorstPosToOrs(param_set_.worst_case_position_);
      BuildTradeVarSets();  /// need to update the tradevarsets map
      ProcessGetFlat();     /// check to see if due to change in the sizes getflat conditions have been triggered
      if (is_ready_) TradeVarSetLogic(my_position_);
    } break;
    case kControlMessageCodeSetUnitTradeSize: {
      for (auto i = 0u; i < param_set_vec_.size(); i++) {
        if (_control_message_.intval_1_ > param_set_vec_[i].unit_trade_size_ / FAT_FINGER_FACTOR &&
            _control_message_.intval_1_ < param_set_vec_[i].unit_trade_size_ * FAT_FINGER_FACTOR) {
          param_set_vec_[i].unit_trade_size_ = std::max(1, _control_message_.intval_1_);
        }
        if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
          DBGLOG_TIME_CLASS << "SetUnitTradeSize " << runtime_id_ << " called with " << _control_message_.intval_1_
                            << " and UnitTradeSize for param " << i << " set to " << param_set_vec_[i].unit_trade_size_
                            << "" << DBGLOG_ENDL_FLUSH;
          if (livetrading_) {
            DBGLOG_DUMP;
          }
        }
      }

      param_set_.unit_trade_size_ = param_set_vec_[param_index_to_use_].unit_trade_size_;
      BuildTradeVarSets();
      ProcessGetFlat();
      if (is_ready_) TradeVarSetLogic(my_position_);
    } break;
    case kControlMessageCodeSetMaxUnitRatio: {
      for (auto i = 0u; i < param_set_vec_.size(); i++) {
        if (int(_control_message_.doubleval_1_ * param_set_vec_[i].unit_trade_size_ + 0.5) >
                param_set_vec_[i].max_position_ / FAT_FINGER_FACTOR &&
            int(_control_message_.doubleval_1_ * param_set_vec_[i].unit_trade_size_ + 0.5) <
                param_set_vec_[i].max_position_ * FAT_FINGER_FACTOR) {
          param_set_vec_[i].max_position_ =
              std::min(param_set_vec_[i].max_position_,
                       (int)(std::max(1.0, _control_message_.doubleval_1_) * param_set_vec_[i].unit_trade_size_ + 0.5));
          param_set_vec_[i].max_position_original_ = param_set_vec_[i].max_position_;
        }
        if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
          DBGLOG_TIME_CLASS << "SetMaxUnitRatio " << runtime_id_ << " called with " << _control_message_.doubleval_1_
                            << " and MaxPosition for param " << i << " set to " << param_set_vec_[i].max_position_ << ""
                            << DBGLOG_ENDL_FLUSH;
          if (livetrading_) {
            DBGLOG_DUMP;
          }
        }
      }

      param_set_.max_position_ = param_set_vec_[param_index_to_use_].max_position_;
      param_set_.max_position_original_ = param_set_.max_position_;
      BuildTradeVarSets();
      ProcessGetFlat();
      if (is_ready_) TradeVarSetLogic(my_position_);
    } break;
    case kControlMessageCodeSetMaxGlobalRisk: {
      for (auto i = 0u; i < param_set_vec_.size(); i++) {
        if (_control_message_.intval_1_ > param_set_vec_[i].max_global_risk_ / FAT_FINGER_FACTOR &&
            _control_message_.intval_1_ < param_set_vec_[i].max_global_risk_ / FAT_FINGER_FACTOR) {
          param_set_vec_[i].max_global_risk_ = _control_message_.intval_1_;
        }
      }
      param_set_.max_global_risk_ = param_set_vec_[param_index_to_use_].max_global_risk_;
      BuildTradeVarSets();
      ProcessGetFlat();
      if (is_ready_) TradeVarSetLogic(my_position_);
      UpdateMaxGlobalRisk();
    } break;
    case kControlMessageCodeSetMaxPosition: {
      for (auto i = 0u; i < param_set_vec_.size(); i++) {
        if (_control_message_.intval_1_ > param_set_vec_[i].max_position_ / FAT_FINGER_FACTOR &&
            _control_message_.intval_1_ < param_set_vec_[i].max_position_ * FAT_FINGER_FACTOR) {
          param_set_vec_[i].max_position_ = std::max(1, _control_message_.intval_1_);
          param_set_vec_[i].max_position_original_ = param_set_vec_[i].max_position_;
        }
        if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
          DBGLOG_TIME_CLASS << "SetMaxPosition " << runtime_id_ << " called with " << _control_message_.intval_1_
                            << " and MaxPosition for param " << i << " set to " << param_set_vec_[i].max_position_ << ""
                            << DBGLOG_ENDL_FLUSH;
          if (livetrading_) {
            DBGLOG_DUMP;
          }
        }
      }

      param_set_.max_position_ = param_set_vec_[param_index_to_use_].max_position_;
      param_set_.max_position_original_ = param_set_.max_position_;
      BuildTradeVarSets();
      if (is_ready_) TradeVarSetLogic(my_position_);
    } break;
    case kControlMessageCodeSetWorstCaseUnitRatio: {
      for (auto i = 0u; i < param_set_vec_.size(); i++) {
        if (((_control_message_.intval_1_ * param_set_vec_[i].unit_trade_size_) >
                 param_set_vec_[i].worst_case_position_ / FAT_FINGER_FACTOR &&
             (_control_message_.intval_1_ * param_set_vec_[i].unit_trade_size_) <
                 param_set_vec_[i].worst_case_position_ * FAT_FINGER_FACTOR) ||
            (!param_set_vec_[i].worst_case_position_)) {
          param_set_vec_[i].worst_case_position_ =
              std::min(param_set_vec_[i].worst_case_position_,
                       std::max(0, _control_message_.intval_1_) * param_set_vec_[i].unit_trade_size_);
        }
        if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
          DBGLOG_TIME_CLASS << "SetWorstCaseUnitRatio " << runtime_id_ << " called with " << _control_message_.intval_1_
                            << " and WorstCasePosition for param " << i << " set to " << param_set_.worst_case_position_
                            << "" << DBGLOG_ENDL_FLUSH;
          if (livetrading_) {
            DBGLOG_DUMP;
          }
        }
      }

      param_set_.worst_case_position_ = param_set_vec_[param_index_to_use_].worst_case_position_;
      order_manager_.NotifyWorstPosToOrs(param_set_.worst_case_position_);
      BuildTradeVarSets();
      ProcessGetFlat();
      if (is_ready_) TradeVarSetLogic(my_position_);
    } break;
    case kControlMessageCodeSetWorstCasePosition: {
      for (auto i = 0u; i < param_set_vec_.size(); i++) {
        if ((!_control_message_.intval_1_) ||
            (_control_message_.intval_1_ > param_set_vec_[i].worst_case_position_ / FAT_FINGER_FACTOR &&
             _control_message_.intval_1_ < param_set_vec_[i].worst_case_position_ * FAT_FINGER_FACTOR) ||
            (!param_set_vec_[i].worst_case_position_)) {
          param_set_vec_[i].worst_case_position_ = std::max(0, _control_message_.intval_1_);
        }
        if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
          DBGLOG_TIME_CLASS << "SetWorstCasePosition " << runtime_id_ << " called with " << _control_message_.intval_1_
                            << " and WorstCasePosition for param " << i << " set to "
                            << param_set_vec_[i].worst_case_position_ << "" << DBGLOG_ENDL_FLUSH;
          if (livetrading_) {
            DBGLOG_DUMP;
          }
        }
      }

      param_set_.worst_case_position_ = param_set_vec_[param_index_to_use_].worst_case_position_;
      order_manager_.NotifyWorstPosToOrs(param_set_.worst_case_position_);
      BuildTradeVarSets();
      ProcessGetFlat();
      if (is_ready_) TradeVarSetLogic(my_position_);
    } break;
    case kControlMessageCodeDisableImprove: {
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME_CLASS << "DisableImprove " << runtime_id_ << " called." << DBGLOG_ENDL_FLUSH;
        if (livetrading_) {
          DBGLOG_DUMP;
        }
      }
      for (auto i = 0u; i < param_set_vec_.size(); i++) {
        param_set_vec_[i].allowed_to_improve_ = false;
      }
      param_set_.allowed_to_improve_ = param_set_vec_[param_index_to_use_].allowed_to_improve_;
    } break;
    case kControlMessageCodeEnableImprove: {
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME_CLASS << "EnableImprove " << runtime_id_ << " called." << DBGLOG_ENDL_FLUSH;
        if (livetrading_) {
          DBGLOG_DUMP;
        }
      }
      for (auto i = 0u; i < param_set_vec_.size(); i++) {
        param_set_vec_[i].allowed_to_improve_ = true;
      }
      param_set_.allowed_to_improve_ = param_set_vec_[param_index_to_use_].allowed_to_improve_;
    } break;
    case kControlMessageCodeDisableAggressive: {
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME_CLASS << "DisableAggressive " << runtime_id_ << " called." << DBGLOG_ENDL_FLUSH;
        if (livetrading_) {
          DBGLOG_DUMP;
        }
      }
      for (auto i = 0u; i < param_set_vec_.size(); i++) {
        param_set_vec_[i].allowed_to_aggress_ = false;
      }
      param_set_.allowed_to_aggress_ = param_set_vec_[param_index_to_use_].allowed_to_aggress_;
    } break;
    case kControlMessageCodeEnableAggressive: {
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME_CLASS << "EnableAggressive " << runtime_id_ << " called." << DBGLOG_ENDL_FLUSH;
        if (livetrading_) {
          DBGLOG_DUMP;
        }
      }
      for (auto i = 0u; i < param_set_vec_.size(); i++) {
        param_set_vec_[i].allowed_to_aggress_ = true;
      }
      param_set_.allowed_to_aggress_ = param_set_vec_[param_index_to_use_].allowed_to_aggress_;
    } break;
    case kControlMessageCodeAddPosition: {
      DBGLOG_TIME_CLASS << "kControlMessageCodeAddPosition " << dep_market_view_.shortcode() << " "
                        << _control_message_.intval_1_ << DBGLOG_ENDL_FLUSH;
      SetPositionOffset(_control_message_.intval_1_);
      if (livetrading_) {
        DBGLOG_DUMP;
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
    case kControlMessageCodeShowIndicators: {
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME_CLASS << "ShowIndicators " << runtime_id_ << " called." << DBGLOG_ENDL_FLUSH;
        if (livetrading_) {
          DBGLOG_DUMP;
        }
      }
      dump_inds = true;
      override_zero_logging_mode_once_for_external_cmd_ = true;

    } break;
    case kControlMessageCodeCleanSumSizeMaps: {
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME_CLASS << "CleanSumSizeMaps " << runtime_id_ << " called." << DBGLOG_ENDL_FLUSH;
        if (livetrading_) {
          DBGLOG_DUMP;
        }
      }
      order_manager_.CleanSumSizeMapsBasedOnOrderMaps();
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
      int complete_days_append_ = watch_.msecs_from_midnight() / 86400000;
      t_severity_change_end_msecs_ += complete_days_append_ * 86400000;

      severity_change_end_msecs_ = std::min(trading_end_utc_mfm_, t_severity_change_end_msecs_);
      DBGLOG_TIME << "Seteco set to " << severity_to_getflat_on_ << " at time " << watch_.msecs_from_midnight()
                  << " with end time as " << severity_change_end_msecs_ << DBGLOG_ENDL_FLUSH;
      ProcessGetFlat();
    } break;
    case kControlMessageCodeForceIndicatorReady: {
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME_CLASS << "ForceIndicatorReady " << runtime_id_
                          << " called for indicator_index_ = " << _control_message_.intval_1_ << DBGLOG_ENDL_FLUSH;

        if (livetrading_) {
          DBGLOG_DUMP;
        }
      }

      p_base_model_math_->ForceIndicatorReady(_control_message_.intval_1_);
    } break;
    case kControlMessageCodeForceAllIndicatorReady: {
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME_CLASS << "ForceAllIndicatorReady " << runtime_id_ << DBGLOG_ENDL_FLUSH;

        if (livetrading_) {
          DBGLOG_DUMP;
        }
      }

      p_base_model_math_->ForceAllIndicatorReady();
    } break;
    case kControlMessageDisableSelfOrderCheck: {
      dep_market_view_.SetSelfOrdersFromBook(false);
    } break;
    case kControlMessageEnableSelfOrderCheck: {
      dep_market_view_.SetSelfOrdersFromBook(true);
    } break;
    case kControlMessageDumpNonSelfSMV: {
      if (dep_market_view_.remove_self_orders_from_book()) {
        dep_market_view_.DumpNonSelfSMV();
      }
    } break;

    case kControlMessageCodeEnableAggCooloff: {
      order_manager_.SetAggTradingCooloff(true);
    } break;
    case kControlMessageCodeDisableAggCooloff: {
      order_manager_.SetAggTradingCooloff(false);
    } break;

    case kControlMessageCodeEnableNonStandardCheck: {
      enable_non_standard_check_ = true;
      DBGLOG_TIME_CLASS_FUNC << " enable_non_standard_check_=true" << DBGLOG_ENDL_FLUSH;
      if (livetrading_) {
        DBGLOG_DUMP;
      }

      ProcessGetFlat();  // See if this changes anything.
    } break;

    case kControlMessageCodeDisableNonStandardCheck: {
      enable_non_standard_check_ = false;
      DBGLOG_TIME_CLASS_FUNC << " enable_non_standard_check_=false" << DBGLOG_ENDL_FLUSH;
      if (livetrading_) {
        DBGLOG_DUMP;
      }

      ProcessGetFlat();  // See if this changes anything.
    } break;

    case kControlMessageDisableMarketManager: {
      if (enable_market_data_interrupt_) {  // Print stuff if going from enable -> disable.
        enable_market_data_interrupt_ = false;
        DBGLOG_TIME_CLASS_FUNC << " enable_market_data_interrupt_=false" << DBGLOG_ENDL_FLUSH;
        if (livetrading_) {
          DBGLOG_DUMP;
        }

        ProcessGetFlat();  // See if this changes anything.
      }
    } break;

    case kControlMessageEnableMarketManager: {
      if (!enable_market_data_interrupt_) {  // Print stuff if going from disable -> enable.
        enable_market_data_interrupt_ = true;
        DBGLOG_TIME_CLASS_FUNC << " enable_market_data_interrupt_=true" << DBGLOG_ENDL_FLUSH;
        if (livetrading_) {
          DBGLOG_DUMP;
        }

        ProcessGetFlat();  // See if this changes anything.
      }
    } break;

    case kControlMessageCodeEnableLogging: {
      DBGLOG_TIME_CLASS << "EnableLogging " << runtime_id_ << " called." << DBGLOG_ENDL_FLUSH;

      if (livetrading_) {
        DBGLOG_DUMP;
      }
    } break;

    case kControlMessageCodeDisableLogging: {
      DBGLOG_TIME_CLASS << "DisableLogging " << runtime_id_ << " called." << DBGLOG_ENDL_FLUSH;

      if (livetrading_) {
        DBGLOG_DUMP;
      }
    } break;

    case kControlMessageCodeSetMaxLoss: {
      for (auto i = 0u; i < param_set_vec_.size(); i++) {
        if (_control_message_.intval_1_ > param_set_vec_[i].max_loss_ / FAT_FINGER_FACTOR &&
            _control_message_.intval_1_ < param_set_vec_[i].max_loss_ * FAT_FINGER_FACTOR) {
          param_set_vec_[i].max_loss_ = _control_message_.intval_1_;
        }

        if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
          DBGLOG_TIME_CLASS << "SetMaxLoss " << runtime_id_
                            << " called for abs_max_loss_ = " << _control_message_.intval_1_
                            << " and MaxLoss for param " << i << " set to " << param_set_vec_[i].max_loss_
                            << DBGLOG_ENDL_FLUSH;

          if (livetrading_) {
            DBGLOG_DUMP;
          }
        }
      }
      param_set_.max_loss_ = param_set_vec_[param_index_to_use_].max_loss_;
      ProcessGetFlat();
    } break;
    case kControlMessageCodeSetMaxDrawDown: {
      for (auto i = 0u; i < param_set_vec_.size(); i++) {
        if (_control_message_.intval_1_ > param_set_vec_[i].max_drawdown_ / FAT_FINGER_FACTOR &&
            _control_message_.intval_1_ < param_set_vec_[i].max_drawdown_ * FAT_FINGER_FACTOR) {
          param_set_vec_[i].max_drawdown_ = _control_message_.intval_1_;
          param_set_vec_[i].read_max_drawdown_ = true;
        }

        if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
          DBGLOG_TIME_CLASS << "SetMaxDrawDown " << runtime_id_
                            << " called for max_drawdown_ = " << _control_message_.intval_1_
                            << " and MaxDrawDown for param " << i << " set to " << param_set_vec_[i].max_drawdown_
                            << DBGLOG_ENDL_FLUSH;

          if (livetrading_) {
            DBGLOG_DUMP;
          }
        }
      }
      param_set_.max_drawdown_ = param_set_vec_[param_index_to_use_].max_drawdown_;
      ProcessGetFlat();
    } break;

    case kControlMessageCodeSetGlobalMaxLoss: {
      for (auto i = 0u; i < param_set_vec_.size(); i++) {
        if (_control_message_.intval_1_ > param_set_vec_[i].global_max_loss_ / FAT_FINGER_FACTOR &&
            _control_message_.intval_1_ < param_set_vec_[i].global_max_loss_ * FAT_FINGER_FACTOR) {
          param_set_vec_[i].global_max_loss_ = _control_message_.intval_1_;
        }

        if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
          DBGLOG_TIME_CLASS << "SetGlobalMaxLoss " << runtime_id_
                            << " called for abs_global_max_loss_ = " << _control_message_.intval_1_
                            << " and GlobalMaxLoss for param " << i << " set to " << param_set_.global_max_loss_
                            << DBGLOG_ENDL_FLUSH;

          if (livetrading_) {
            DBGLOG_DUMP;
          }
        }
      }
      param_set_.global_max_loss_ = param_set_vec_[param_index_to_use_].global_max_loss_;
      ProcessGetFlat();
    } break;

    case kControlMessageCodeSetShortTermGlobalMaxLoss: {
      for (auto i = 0u; i < param_set_vec_.size(); i++) {
        if (_control_message_.intval_1_ > param_set_vec_[i].short_term_global_max_loss_ / FAT_FINGER_FACTOR &&
            _control_message_.intval_1_ < param_set_vec_[i].short_term_global_max_loss_ * FAT_FINGER_FACTOR) {
          param_set_vec_[i].short_term_global_max_loss_ = _control_message_.intval_1_;
        }

        if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
          DBGLOG_TIME_CLASS << "SetShortTermGlobalMaxLoss " << runtime_id_
                            << " called for abs_short_term_global_max_loss_ = " << _control_message_.intval_1_
                            << " and ShortTermGlobalMaxLoss for param " << i << " set to "
                            << param_set_.short_term_global_max_loss_ << DBGLOG_ENDL_FLUSH;

          if (livetrading_) {
            DBGLOG_DUMP;
          }
        }
      }
      param_set_.short_term_global_max_loss_ = param_set_vec_[param_index_to_use_].short_term_global_max_loss_;
      ProcessGetFlat();
    } break;

    case kControlMessageCodeShowOrders: {
      DBGLOG_TIME_CLASS << "ShowOrders " << runtime_id_ << " called." << DBGLOG_ENDL_FLUSH;

      if (livetrading_) {
        DBGLOG_DUMP;
      }

      last_full_logging_msecs_ = 0;
      override_zero_logging_mode_once_for_external_cmd_ = true;
      LogFullStatus();
    } break;

    case kControlMessageCodeSetMaxGlobalPosition: {
      for (auto i = 0u; i < param_set_vec_.size(); i++) {
        if (param_set_vec_[i].read_max_global_position_) {
          if (_control_message_.intval_1_ >= 0 &&
              _control_message_.intval_1_ < param_set_vec_[i].max_global_position_ * FAT_FINGER_FACTOR) {
            param_set_vec_[i].max_global_position_ = std::max(
                0, _control_message_
                       .intval_1_);  // Allow 0 to disable trading when our net position across all queries = 0 ???
          }
        } else {  // Param file did not specify a max-global-position , set it to provided value.
          if (_control_message_.intval_1_ >= 0) {
            param_set_vec_[i].max_global_position_ = std::max(0, _control_message_.intval_1_);
            param_set_vec_[i].read_max_global_position_ =
                true;  // Enable this for the max-global-position to take effect.
          }
        }

        if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
          DBGLOG_TIME_CLASS << "SetMaxGlobalPosition " << runtime_id_ << " called with " << _control_message_.intval_1_
                            << " and MaxGlobalPosition for param " << i << " set to "
                            << param_set_vec_[i].max_global_position_ << "" << DBGLOG_ENDL_FLUSH;
          if (livetrading_) {
            DBGLOG_DUMP;
          }
        }
      }
      param_set_.max_global_position_ = param_set_vec_[param_index_to_use_].max_global_position_;
      param_set_.read_max_global_position_ = param_set_vec_[param_index_to_use_].read_max_global_position_;
      if (is_ready_) TradeVarSetLogic(my_position_);
      ProcessGetFlat();
    } break;

    case kControlMessageCodeEnableZeroLoggingMode: {
      DBGLOG_TIME_CLASS << "EnableZeroLoggingMode " << runtime_id_ << " called." << DBGLOG_ENDL_FLUSH;
      zero_logging_mode_ = true;
      if (livetrading_) {
        DBGLOG_DUMP;
      }
    } break;

    case kControlMessageCodeDisableZeroLoggingMode: {
      DBGLOG_TIME_CLASS << "DisableZeroLoggingMode " << runtime_id_ << " called." << DBGLOG_ENDL_FLUSH;
      zero_logging_mode_ = false;
      if (livetrading_) {
        DBGLOG_DUMP;
      }
    } break;

    case kControlMessageCodeSetMaxSecurityPosition: {
      for (auto i = 0u; i < param_set_vec_.size(); i++) {
        if (_control_message_.intval_1_ >= 0 &&
            _control_message_.intval_1_ < param_set_vec_[i].max_security_position_ * FAT_FINGER_FACTOR) {
          param_set_vec_[i].max_security_position_ = std::max(
              0, _control_message_
                     .intval_1_);  // Allow 0 to disable trading when our net position across all queries = 0 ???
        }
        if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
          DBGLOG_TIME_CLASS << "SetMaxSecurityPosition " << runtime_id_ << " called with "
                            << _control_message_.intval_1_ << " and MaxSecurityPosition for param " << i << " set to "
                            << param_set_.max_security_position_ << "" << DBGLOG_ENDL_FLUSH;
          if (livetrading_) {
            DBGLOG_DUMP;
          }
        }
      }
      param_set_.max_security_position_ = param_set_vec_[param_index_to_use_].max_security_position_;
      if (is_ready_) TradeVarSetLogic(my_position_);
      ProcessGetFlat();
    } break;

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
    } break;

    case kControlMessageCodeSetEndTime: {
      int old_trading_end_utc_mfm_ = -1;

      if (_control_message_.intval_1_ >= 0 && _control_message_.intval_1_ <= 2359) {  // utc hhmm time sanity check.
        old_trading_end_utc_mfm_ = trading_end_utc_mfm_;
        trading_end_utc_mfm_ = GetMsecsFromMidnightFromHHMM(_control_message_.intval_1_) +
                               watch_.day_offset();  // no solution if someone calls it during the previous day UTC
                                                     // itself.Trading will stop immediately in this pathological case
      }

      DBGLOG_TIME_CLASS << "SetEndTime " << runtime_id_ << " called with " << _control_message_.intval_1_
                        << " and trading_end_utc_mfm_ set to " << trading_end_utc_mfm_ << " from "
                        << old_trading_end_utc_mfm_ << DBGLOG_ENDL_FLUSH;

      if (livetrading_) {
        DBGLOG_DUMP;
      }

      ProcessGetFlat();
    } break;

    case kControlMessageCodeSetOpenTradeLoss: {
      if (_control_message_.intval_1_ > max_opentrade_loss_ / FAT_FINGER_FACTOR &&
          _control_message_.intval_1_ < max_opentrade_loss_ * FAT_FINGER_FACTOR) {
        max_opentrade_loss_ = _control_message_.intval_1_;
      }

      for (auto i = 0u; i < param_set_vec_.size(); i++) {
        param_set_vec_[i].max_opentrade_loss_ = max_opentrade_loss_;

        if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
          DBGLOG_TIME_CLASS << "SetOpenTradeLoss " << runtime_id_
                            << " called for abs_opentrade_loss_ = " << _control_message_.intval_1_
                            << " and OpenTradeLoss for param " << i << " set to " << max_opentrade_loss_
                            << DBGLOG_ENDL_FLUSH;

          if (livetrading_) {
            DBGLOG_DUMP;
          }
        }
      }

      param_set_.max_opentrade_loss_ = param_set_vec_[param_index_to_use_].max_opentrade_loss_;
      ProcessGetFlat();
    } break;

    case kControlMessageCodeSetMaxIntSpreadToPlace: {
      for (auto i = 0u; i < param_set_vec_.size(); i++) {
        if (abs(_control_message_.intval_1_ - param_set_vec_[i].max_int_spread_to_place_) <=
            1) {  // Increments of 1 at most
          param_set_vec_[i].max_int_spread_to_place_ = _control_message_.intval_1_;
        }

        if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
          DBGLOG_TIME_CLASS << "SetMaxIntSpreadToPlace " << runtime_id_
                            << " called for max_int_spread_to_place_ = " << _control_message_.intval_1_
                            << " and max_int_spread_to_place_ for param " << i << " set to "
                            << param_set_vec_[i].max_int_spread_to_place_ << DBGLOG_ENDL_FLUSH;

          if (livetrading_) {
            DBGLOG_DUMP;
          }
        }
      }
      param_set_.max_int_spread_to_place_ = param_set_vec_[param_index_to_use_].max_int_spread_to_place_;
      ProcessGetFlat();
    } break;

    case kControlMessageCodeSetMaxIntLevelDiffToPlace: {
      for (auto i = 0u; i < param_set_vec_.size(); i++) {
        if (abs(_control_message_.intval_1_ - param_set_vec_[i].max_int_level_diff_to_place_) <=
            1) {  // Increments of 1 at most
          param_set_vec_[i].max_int_level_diff_to_place_ = _control_message_.intval_1_;
        }

        if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
          DBGLOG_TIME_CLASS << "SetMaxIntLevelDiffToPlace " << runtime_id_
                            << " called for max_int_level_diff_to_place_ = " << _control_message_.intval_1_
                            << " and max_int_level_diff_to_place_ for param " << i << " set to "
                            << param_set_.max_int_level_diff_to_place_ << DBGLOG_ENDL_FLUSH;

          if (livetrading_) {
            DBGLOG_DUMP;
          }
        }
      }
      param_set_.max_int_level_diff_to_place_ = param_set_vec_[param_index_to_use_].max_int_level_diff_to_place_;
      ProcessGetFlat();
    } break;

    case kControlMessageCodeSetMaxPnl: {
      for (auto i = 0u; i < param_set_vec_.size(); i++) {
        if (_control_message_.intval_1_ > 0) {
          param_set_vec_[i].max_pnl_ = _control_message_.intval_1_;
        }

        if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
          DBGLOG_TIME_CLASS << "SetMaxPnl " << runtime_id_
                            << " called for abs_max_pnl_ = " << _control_message_.intval_1_ << " and MaxPnl for param "
                            << i << " set to " << param_set_.max_pnl_ << DBGLOG_ENDL_FLUSH;

          if (livetrading_) {
            DBGLOG_DUMP;
          }
        }
      }
      param_set_.max_pnl_ = param_set_vec_[param_index_to_use_].max_pnl_;
      ProcessGetFlat();
    } break;

    case kControlMessageCodeSetExplicitMaxLongPosition: {
      for (auto i = 0u; i < param_set_vec_.size(); i++) {
        if (_control_message_.intval_1_ > 0) {
          param_set_vec_[i].explicit_max_long_position_ =
              std::min(5 * param_set_vec_[i].max_position_, _control_message_.intval_1_);
          param_set_vec_[i].read_explicit_max_long_position_ = true;
        }
        if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
          DBGLOG_TIME_CLASS << "SetExplicitMaxLongPosition " << runtime_id_
                            << " called for explicit_max_long_position_ = " << _control_message_.intval_1_
                            << " and ExplicitMaxLongPosition for param " << i << " set to "
                            << param_set_vec_[i].explicit_max_long_position_ << DBGLOG_ENDL_FLUSH;
        }
      }

      param_set_.explicit_max_long_position_ = param_set_vec_[param_index_to_use_].explicit_max_long_position_;
      param_set_.read_explicit_max_long_position_ =
          param_set_vec_[param_index_to_use_].read_explicit_max_long_position_;
      BuildTradeVarSets();
      ProcessGetFlat();
      if (is_ready_) TradeVarSetLogic(my_position_);

    } break;

    case kControlMessageCodeSetExplicitWorstLongPosition: {
      for (auto i = 0u; i < param_set_vec_.size(); i++) {
        if (_control_message_.intval_1_ > 0) {
          param_set_vec_[i].explicit_worst_case_long_position_ =
              std::min(5 * param_set_vec_[i].worst_case_position_, _control_message_.intval_1_);
          param_set_vec_[i].read_explicit_worst_case_long_position_ = true;
        }
        if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
          DBGLOG_TIME_CLASS << "SetExplicitWorstLongPosition " << runtime_id_
                            << " called for explicit_worst_case_long_position_ = " << _control_message_.intval_1_
                            << " and ExplicitWorstLongPosition for param " << i << " set to "
                            << param_set_vec_[i].explicit_worst_case_long_position_ << DBGLOG_ENDL_FLUSH;
        }
      }

      param_set_.explicit_worst_case_long_position_ =
          param_set_vec_[param_index_to_use_].explicit_worst_case_long_position_;
      param_set_.read_explicit_worst_case_long_position_ =
          param_set_vec_[param_index_to_use_].read_explicit_worst_case_long_position_;
      BuildTradeVarSets();
      ProcessGetFlat();
      if (is_ready_) TradeVarSetLogic(my_position_);
    } break;

    case kControlMessageCodeSetBreakMsecsOpenTradeLoss: {
      if (_control_message_.intval_1_ > 0) {
        break_msecs_on_max_opentrade_loss_ = std::max(1 * 60 * 1000, _control_message_.intval_1_);
      }

      for (auto i = 0u; i < param_set_vec_.size(); i++) {
        if (_control_message_.intval_1_ > 0) {
          param_set_vec_[i].break_msecs_on_max_opentrade_loss_ = break_msecs_on_max_opentrade_loss_;
        }
        if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
          DBGLOG_TIME_CLASS << "SetBreakMsecsOpenTradeLoss " << runtime_id_
                            << " called for break_msecs_on_max_opentrade_loss_ = " << _control_message_.intval_1_
                            << " and BreakMsecsOpenTradeLoss for param " << i << " set to "
                            << param_set_vec_[i].break_msecs_on_max_opentrade_loss_ << DBGLOG_ENDL_FLUSH;
        }
      }
      param_set_.break_msecs_on_max_opentrade_loss_ =
          param_set_vec_[param_index_to_use_].break_msecs_on_max_opentrade_loss_;
      ProcessGetFlat();
    } break;

    case kControlMessageReloadEconomicEvents: {
      DBGLOG_TIME_CLASS << "BaseTrading::Got refreshecoevents user_msg " << DBGLOG_ENDL_FLUSH;
      economic_events_manager_.ReloadDB();
      economic_events_manager_.AllowEconomicEventsFromList(dep_market_view_.shortcode());
      allowed_events_present_ = economic_events_manager_.AllowedEventsPresent();
      getflat_due_to_allowed_economic_event_ = false;
      last_allowed_event_index_ = 0;
      ProcessAllowedEco();
      economic_events_manager_.AdjustSeverity(dep_market_view_.shortcode(), dep_market_view_.exch_source());
    } break;

    case kControlMessageShowEconomicEvents: {
      DBGLOG_TIME_CLASS << "BaseTrading::Got showecoevents user_msg " << DBGLOG_ENDL_FLUSH;
      economic_events_manager_.ShowDB();
    } break;

    case kControlMessageDisableFreezeOnRejects: {
      DBGLOG_TIME_CLASS << "BaseTrading::Disable Freeze On Rejects" << DBGLOG_ENDL_FLUSH;
      order_manager_.UpdateAutoFreezeSystem(false);
    } break;

    case kControlMessageEnableFreezeOnRejects: {
      DBGLOG_TIME_CLASS << "BaseTrading::Enable Freeze On Rejects" << DBGLOG_ENDL_FLUSH;
      order_manager_.UpdateAutoFreezeSystem(true);
    } break;
    default:
      break;
  }
}

void BaseTrading::UpdateBetaPosition(const unsigned int sec_id_, int new_position) {}

void BaseTrading::OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo &_market_update_info_) {
  if (dbglogger_.CheckLoggingLevel(TRADEINIT_INFO) && !livetrading_)  // don't sleep in live-trading.
  {
    if (last_update_ttime_.tv_sec == 0 && last_update_ttime_.tv_usec == 0) {
      last_update_ttime_ = watch_.tv();
    } else {
      HFSAT::ttime_t t_diff_ = watch_.tv() - last_update_ttime_;
      last_update_ttime_ = watch_.tv();
      if ((watch_.msecs_from_midnight() > (trading_start_utc_mfm_ - 5000)) &&
          (watch_.msecs_from_midnight() < trading_end_utc_mfm_)) {
        HFSAT::usleep(t_diff_.tv_sec * 1000000 + t_diff_.tv_usec);
        HFSAT::usleep(t_diff_.tv_sec * 1000000 + t_diff_.tv_usec);
      }
    }
  }

  NonSelfMarketUpdate();
  /* no need to call TradingLogic since we expect a UpdateTarget call after this */

  if (dbglogger_.CheckLoggingLevel(TRADEINIT_INFO)) {  // print book to screen
    ShowBook();
  }
}

void BaseTrading::OnTradePrint(const unsigned int _security_id_, const TradePrintInfo &_trade_print_info_,
                               const MarketUpdateInfo &_market_update_info_) {
  NonSelfMarketUpdate();
  /* no need to call TradingLogic since we expect a UpdateTarget call after this */

  if (getflat_due_to_non_standard_market_conditions_ &&
      last_getflat_due_to_non_standard_market_conditions_triggered_at_msecs_ <
          watch_.msecs_from_midnight()) {  // We are currently in non-standard market conditions ,
                                           // save these trade prices for checks before exiting this mode.
    non_standard_market_conditions_mode_prices_[_trade_print_info_.int_trade_price_] = 1;
  }

  if (!is_pair_strategy_ && !is_structured_strategy_ && !getflat_due_to_max_opentrade_loss_ &&
      !is_structured_general_strategy_) {
    // check opentrade loss
    if (order_manager_.base_pnl().opentrade_unrealized_pnl() < -max_opentrade_loss_) {
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME << "getflat_due_to_max_opentrade_loss_ ! Current opentradepnl : "
                    << order_manager_.base_pnl().opentrade_unrealized_pnl() << " < " << -max_opentrade_loss_
                    << " Strategy: " << runtime_id_ << DBGLOG_ENDL_FLUSH;
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
            t_oss_ << "Strategy: " << runtime_id_
                   << " getflat_due_to_max_opentrade_loss_: " << order_manager_.base_pnl().opentrade_unrealized_pnl()
                   << " product " << dep_market_view_.shortcode() << " = " << dep_market_view_.secname() << " on "
                   << hostname_ << "\n";

            getflat_email_string_ = t_oss_.str();
          }

          //          SendMail(getflat_email_string_, getflat_email_string_);
        }
      }
      getflat_due_to_max_opentrade_loss_ = true;
      last_getflat_due_to_max_opentrade_loss_triggered_at_msecs_ = watch_.msecs_from_midnight();
      num_opentrade_loss_hits_++;
    }
  }

  if (dbglogger_.CheckLoggingLevel(TRADEINIT_INFO)) {  // print book to screen
    std::string t_s_ = _trade_print_info_.ToString() + " " + _market_update_info_.ToString();

    last_5_trade_prices_.push_back(t_s_);
    if (last_5_trade_prices_.size() > 5) {
      last_5_trade_prices_.erase(last_5_trade_prices_.begin());
    }

    ShowBook();
  }
}
void BaseTrading::OnMarketStatusChange(const unsigned int _security_id_, const MktStatus_t _new_market_status_) {
  switch (_new_market_status_) {
    case kMktTradingStatusOpen: {
      getflat_due_to_market_status_ = false;
    } break;
    case kMktTradingStatusClosed: {
      getflat_due_to_close_ = true;
      ProcessGetFlat();
    } break;
    case kMktTradingStatusPreOpen:
    case kMktTradingStatusReserved: {
      getflat_due_to_market_status_ = true;
    } break;
    case kMktTradingStatuFinalClosingCall: {
    } break;
    case kMktTradingStatusUnknown:
    case kMktTradingStatusForbidden: {
      getflat_due_to_market_status_ = true;
    } break;
    default: { break; }
  }
  market_status_ = _new_market_status_;
}

bool BaseTrading::UsingCancellationModel() {
  if (param_set_.read_use_cancellation_model_) {
    return true;
  } else {
    return false;
  }
}

void BaseTrading::SetupVariablesBeforeTradingLogic(double target_price, double targetbias_numbers) {
  if (param_set_.read_longevity_support_) {
    bestask_queue_hysterisis_ = exec_logic_indicators_helper_->bestask_queue_hysterisis();
    bestbid_queue_hysterisis_ = exec_logic_indicators_helper_->bestbid_queue_hysterisis();
  }

  if (param_set_.read_bigtrades_cancel_) {
    last_bigtrades_ask_cancel_msecs_ = exec_logic_indicators_helper_->last_bigtrades_ask_cancel_msecs();
    last_bigtrades_bid_cancel_msecs_ = exec_logic_indicators_helper_->last_bigtrades_bid_cancel_msecs();
  }

  if (param_set_.read_bigtrades_place_) {
    last_bigtrades_ask_place_msecs_ = exec_logic_indicators_helper_->last_bigtrades_ask_place_msecs();
    last_bigtrades_bid_place_msecs_ = exec_logic_indicators_helper_->last_bigtrades_bid_place_msecs();
  }

  if (param_set_.read_bigtrades_aggress_) {
    last_bigtrades_ask_aggress_msecs_ = exec_logic_indicators_helper_->last_bigtrades_ask_aggress_msecs();
    last_bigtrades_bid_aggress_msecs_ = exec_logic_indicators_helper_->last_bigtrades_bid_aggress_msecs();
  }
  if (param_set_.read_l1_bid_ask_flow_cancel_thresh_) {
    cancel_l1_bid_ask_flow_buy_ = exec_logic_indicators_helper_->l1_bid_ask_size_flow_cancel_buy();
    cancel_l1_bid_ask_flow_sell_ = exec_logic_indicators_helper_->l1_bid_ask_size_flow_cancel_sell();
  }

  if (param_set_.l1bias_model_stdev_ > 0) {
    double basemid_diff_ = target_price - targetbias_numbers - dep_market_view_.mid_price();
    double scaled_basemid_diff_ = exec_logic_indicators_helper_->ComputeScaledBaseMidDiff(basemid_diff_);
    target_price += scaled_basemid_diff_ - basemid_diff_;
    if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
      DBGLOG_TIME << "Scaled_basemid_diff_ " << scaled_basemid_diff_ << DBGLOG_ENDL_FLUSH;
    };
  }
  target_price_ = target_price + (param_set_.sumvars_scaling_factor_ - 1) * targetbias_numbers;
  targetbias_numbers_ = targetbias_numbers * param_set_.sumvars_scaling_factor_;
}

/// called by ModelMath . Gets ready the first time the received target_price_ is in the bid-ask range
/// after that store value locally and start TradingLogic ( )
bool BaseTrading::UpdateTarget(double _target_price_, double _targetbias_numbers_, int _modelmath_index_) {
  if (!is_ready_) {  // checking whether we are post start time
    if (livetrading_ && (watch_.msecs_from_midnight() >
                         (trading_start_utc_mfm_ - (RELEASE_CORE_BEFORE_GIVEN_INTERVAL_IN_MSEC - 30 * 1000)))) {
      // allocating CPU 1 mintue before start_time as it takes few seconds for allocation which delays initial order
      // placing
      AllocateCPU();

      // We are not starting the query unless it gets affined
      if (!is_affined_) return false;
    }

    // if dependant is ready
    // and if so, then if the signal is between bid and offer \\ not sure if this check is necessary watch_.YYYYMMDD (
    // )
    // == tradingdate_

    target_price_ = _target_price_;
    targetbias_numbers_ = _targetbias_numbers_;

    double bidaskspread_ = dep_market_view_.bestask_price() - dep_market_view_.bestbid_price();
    if (watch_.msecs_from_midnight() > trading_start_utc_mfm_ && dep_market_view_.is_ready() &&
        (target_price_ >= (dep_market_view_.bestbid_price() - 6 * bidaskspread_) &&
         target_price_ <= (dep_market_view_.bestask_price() + 6 * bidaskspread_))) {
      is_ready_ = true;
      DBGLOG_TIME_CLASS_FUNC << "SACI: " << order_manager_.server_assigned_client_id_
                             << " got ready! shc: " << dep_market_view_.shortcode() << " queryid: " << runtime_id_
                             << DBGLOG_ENDL_FLUSH;
      p_base_model_math_->ShowIndicatorValues();
      // added here since this is the first time TradeVarSetLogic ( ) is being called to set
      // current_tradevarset_
      SetupVariablesBeforeTradingLogic(_target_price_, _targetbias_numbers_);
      TradeVarSetLogic(my_position_);
    }
  } else {  // is_ready amd tradevars are already set

    SetupVariablesBeforeTradingLogic(_target_price_, _targetbias_numbers_);

    if ((!external_freeze_trading_) && (!freeze_due_to_exchange_stage_) && (!freeze_due_to_funds_reject_)) {
      ProcessGetFlat();  // TODO ... see if this needs to be here ... I mean not much changes in get flat conditions
      // when the position is updating

      if ((!should_be_getting_flat_) && (throttle_manager_->allowed_through_throttle(watch_.msecs_from_midnight()))) {
        CancelAndClose();
        TradingLogic();
        CallPlaceCancelNonBestLevels();  // we probably don't want to do this for RetailTrading ... so set WPOS = 0 ?
      } else if (should_be_getting_flat_) {
        GetFlatFokTradingLogic();
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

/// called by ModelMath . Gets ready the first time the received target_price_ is in the bid-ask range
/// after that store value locally and start TradingLogic ( )
bool BaseTrading::UpdateTarget(double _prob_decrease_, double _prob_nochange_, double _prob_increase_,
                               int _modelmath_index_) {
  if (!is_ready_) {  // checking whether we are post start time and 30 sec after release core was initiated
    if (livetrading_ && (watch_.msecs_from_midnight() >
                         (trading_start_utc_mfm_ - (RELEASE_CORE_BEFORE_GIVEN_INTERVAL_IN_MSEC - 30 * 1000)))) {
      // allocating CPU 1 mintue before start_time as it takes few seconds for allocation which delays initial order
      // placing
      AllocateCPU();
    }

    // if dependant is ready
    // and if so, then if the signal is between bid and offer \\ not sure if this check is necessary watch_.YYYYMMDD (
    // )
    // == tradingdate_
    if ((watch_.msecs_from_midnight() > trading_start_utc_mfm_) && (dep_market_view_.is_ready())) {
      is_ready_ = true;
      DBGLOG_TIME_CLASS_FUNC << "SACI: " << order_manager_.server_assigned_client_id_
                             << " got ready! shc: " << dep_market_view_.shortcode() << " queryid: " << runtime_id_
                             << DBGLOG_ENDL_FLUSH;
      p_base_model_math_->ShowIndicatorValues();
      // added here since this is the first time TradeVarSetLogic ( ) is being called to set
      // current_tradevarset_
      TradeVarSetLogic(my_position_);
    }
  } else {  // is_ready amd tradevars are already set

    prob_decrease_ = _prob_decrease_;
    prob_nochange_ = _prob_nochange_;
    prob_increase_ = _prob_increase_;

    if (!external_freeze_trading_) {
      ProcessGetFlat();  // TODO ... see if this needs to be here ... I mean not much changes in get flat conditions
      // when the position is updating

      if ((!should_be_getting_flat_) && (throttle_manager_->allowed_through_throttle(watch_.msecs_from_midnight()))) {
        CancelAndClose();
        TradingLogic();
        CallPlaceCancelNonBestLevels();
      } else if (should_be_getting_flat_) {
        GetFlatFokTradingLogic();
        if (livetrading_ || (dbglogger_.CheckLoggingLevel(TRADING_INFO))) {
          LogFullStatus();
        }
      }
    }
  }

  if (dump_inds) {
    dump_inds = false;

    if (zero_logging_mode_ && !override_zero_logging_mode_once_for_external_cmd_) return true;

    // only override on external command once
    if (override_zero_logging_mode_once_for_external_cmd_) override_zero_logging_mode_once_for_external_cmd_ = false;

    if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {  // Only dump indicator values if this is set
      DBGLOG_TIME << " mkt: " << best_nonself_bid_size_ << " @ " << best_nonself_bid_price_ << " X "
                  << best_nonself_ask_price_ << " @ " << best_nonself_ask_size_
                  << " prob_decrease_: " << _prob_decrease_ << " prob_nochange_: " << _prob_nochange_
                  << " prob_increase_: " << _prob_increase_ << DBGLOG_ENDL_FLUSH;
      return true;
    }
  }

  return false;
}

void BaseTrading::UpdateCancelSignal(double predicted_class_prob_, int bid_or_ask) {
  if (is_ready_) {
    if ((!external_freeze_trading_) && (!freeze_due_to_exchange_stage_) && (!freeze_due_to_funds_reject_)) {
      ProcessGetFlat();
      if ((!should_be_getting_flat_) && (throttle_manager_->allowed_through_throttle(watch_.msecs_from_midnight()))) {
        ComputeCancellationModelBias(predicted_class_prob_, bid_or_ask);
        if (param_set_.read_use_cancellation_model_) {
          current_bid_tradevarset_.MultiplyBidsBy(cancellation_model_bid_bias_);
          current_bid_keep_tradevarset_.MultiplyBidsBy(cancellation_model_bid_bias_);
          current_ask_tradevarset_.MultiplyAsksBy(cancellation_model_ask_bias_);
          current_ask_keep_tradevarset_.MultiplyAsksBy(cancellation_model_ask_bias_);
        }
      } else if (should_be_getting_flat_) {
        GetFlatFokTradingLogic();
        if (livetrading_ || (dbglogger_.CheckLoggingLevel(TRADING_INFO))) {
          LogFullStatus();
        }
      }
    }
  }
}

inline void BaseTrading::TargetNotReady() {
  is_ready_ = false;
  if (!dbglogger_.IsNoLogs() || livetrading_) {
    DBGLOG_TIME_CLASS_FUNC << "SACI: " << order_manager_.server_assigned_client_id_
                           << " got NOTready! shc: " << dep_market_view_.shortcode() << " queryid: " << runtime_id_
                           << DBGLOG_ENDL_FLUSH;
  }
  external_getflat_ = true;  /// hack to get the query to stop trading

  if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
    DBGLOG_TIME_CLASS_FUNC << "Setting external_getflat_=true and is_ready_=false" << DBGLOG_ENDL_FLUSH;
    if (livetrading_) {
      DBGLOG_DUMP;
    }
  }
  GetFlatFokTradingLogic();
}

inline void BaseTrading::ProcessGetFlat() {
  bool t_should_be_getting_flat_ = ShouldBeGettingFlat();

  // getflat if self volume is very high

  if (!t_should_be_getting_flat_) {
    if (param_set_.read_min_size_to_quote_ && param_set_.max_bid_ask_order_diff_) {
      if (my_position_ != 0) {
        t_should_be_getting_flat_ = true;
      }
    }
  }

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
}

bool BaseTrading::ShouldBeGettingFlat() {
  /*
   should_be_getting_flat_ = getflat_due_to_external_getflat_ ||
   getflat_due_to_close_ ||
   getflat_due_to_max_position_ ||
   getflat_due_to_max_loss_ ||
   getflat_due_to_max_opentrade_loss_ ||
   // getflat_due_to_max_pertrade_loss_ ||
   getflat_due_to_economic_times_ ;
   */

  if (is_nse_earning_day_) {
    return true;
  }
  if (getflat_due_to_feature_model_ == 1) return true;

  if (freeze_due_to_exchange_stage_) {
    if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
      DBGLOG_TIME << "Setting should be getting flat to false for exchange freeze " << runtime_id_ << DBGLOG_ENDL_FLUSH;
      if (livetrading_) {
        DBGLOG_DUMP;
      }
    }
    return false;
  }

  if (getflat_due_to_close_ ||
      (watch_.msecs_from_midnight() >
       trading_end_utc_mfm_)) {  // if getflat_due_to_close_ is once activated then it can't be reset  ...
    // perhaps unless a usermessage is sent ?
    if (!getflat_due_to_close_) {
      pnl_at_close_ = order_manager_.base_pnl().total_pnl();
      position_at_close_ = my_position_;

      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME << " getflat_due_to_close_ current: " << watch_.msecs_from_midnight() << " " << trading_end_utc_mfm_
                    << " Shortcode: " << dep_market_view_.shortcode() << " Strategy: " << runtime_id_
                    << DBGLOG_ENDL_FLUSH;
        if (livetrading_) {
          DBGLOG_DUMP;
        }
      }
      getflat_due_to_close_ = true;
      if (!are_log_vec_dumped_ && dbglogger_.CheckLoggingLevel(OM_INFO)) {
        std::ostringstream temp_imp_buy_vec_str_;
        if (improve_buy_time_stamp_vec_.size() > 0) {
          for (unsigned int i = 0; i < improve_buy_time_stamp_vec_.size(); i++) {
            temp_imp_buy_vec_str_ << improve_buy_time_stamp_vec_[i] << "\n";
          }
        }

        DBGLOG_TIME << " Improve Buy Vec :"
                    << "\n" << temp_imp_buy_vec_str_.str() << DBGLOG_ENDL_FLUSH;

        std::ostringstream temp_imp_sell_vec_str_;
        if (improve_sell_time_stamp_vec_.size() > 0) {
          for (unsigned int i = 0; i < improve_sell_time_stamp_vec_.size(); i++) {
            temp_imp_sell_vec_str_ << improve_sell_time_stamp_vec_[i] << "\n";
          }
        }

        DBGLOG_TIME << " Improve Sell Vec :"
                    << "\n" << temp_imp_sell_vec_str_.str() << DBGLOG_ENDL_FLUSH;

        if (livetrading_) {
          DBGLOG_DUMP;
        }
        are_log_vec_dumped_ = true;
      }
    }
    return true;
  } else if (watch_.msecs_from_midnight() >
             (trading_end_utc_mfm_ - RELEASE_CORE_BEFORE_GIVEN_INTERVAL_IN_MSEC))  // Relase core before the close, why
                                                                                   // an offset - because next guy will
                                                                                   // ask for core exactly at the
                                                                                   // boundaryif it's ready with data
  {
    // release cores, anyways we are closing in
    if (livetrading_ && param_set_.release_core_premature_) {
      CPUManager::AffinToInitCores(getpid());
    }
  } else {
    getflat_due_to_close_ = false;
  }

  if (external_getflat_) {
    if (!getflat_due_to_external_getflat_) {  // first time that getflat_due_to_external_getflat_ is false and
      // external_getflat_ is true ..
      // which means the user message was received right now

      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME << " getflat_due_to_external_getflat_ " << runtime_id_ << DBGLOG_ENDL_FLUSH;
        if (livetrading_) {
          DBGLOG_DUMP;
        }
      }
      getflat_due_to_external_getflat_ = true;
    }
    return true;
  } else {
    getflat_due_to_external_getflat_ = false;
  }

  if (agg_getflat_) {
    if (!getflat_due_to_external_agg_getflat_) {
      // first time we are receiving the external getflat
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME << " getflat_due_to_external_agg_getflat_" << runtime_id_ << DBGLOG_ENDL_FLUSH;
        if (livetrading_) {
          DBGLOG_DUMP;
        }
      }
      getflat_due_to_external_agg_getflat_ = true;
    }
    return true;
  } else {
    getflat_due_to_external_agg_getflat_ = false;
  }

  if (rej_due_to_funds_) {
    if (!getflat_due_to_funds_rej_) {  // first time that getflat_due_to_funds_rej_ is false and rej_due_to_funds_ is
      // true ..
      // which means the ORS message for reject due to funds was received right now

      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME << " getflat_due_to_funds_rej " << runtime_id_ << DBGLOG_ENDL_FLUSH;
        if (livetrading_) {
          DBGLOG_DUMP;
        }
      }
      getflat_due_to_funds_rej_ = true;
    }
    return true;
  } else {
    getflat_due_to_funds_rej_ = false;
  }
  // put this check if necessary watch_.YYYYMMDD ( ) == tradingdate_

  if (getflat_due_to_market_status_) {
    DBGLOG_TIME << "getflat_due_to_market_status_ " << MktTradingStatusStr(dep_market_view_.current_market_status_)
                << DBGLOG_ENDL_FLUSH;
    return true;
  }

  if ((!is_pair_strategy_ && !is_structured_strategy_ &&
       abs(my_position_) >= (param_set_.max_position_ + param_set_.unit_trade_size_)) ||
      (is_pair_strategy_ && abs(my_combined_position_) >= (param_set_.max_position_ + param_set_.unit_trade_size_)) ||
      (is_structured_strategy_ &&
       abs(minimal_risk_position_) >= (param_set_.max_position_ + param_set_.unit_trade_size_)))

  {
    if (!getflat_due_to_max_position_) {
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME << " getflat_due_to_max_position_ " << runtime_id_ << DBGLOG_ENDL_FLUSH;
        if (livetrading_) {
          DBGLOG_DUMP;
        }
      }
      getflat_due_to_max_position_ = true;
    }
    return true;
  } else {
    getflat_due_to_max_position_ = false;
  }

  // only getflat in live .. in sim ... ride and use SimBasePNL to record the low
  if (!is_structured_general_strategy_ && !is_structured_strategy_) {
    if (livetrading_ && (order_manager_.base_pnl().total_pnl() < -param_set_.max_loss_)) {
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
              t_oss_ << "Strategy: " << runtime_id_
                     << " getflat_due_to_max_loss_: " << order_manager_.base_pnl().total_pnl() << " product "
                     << dep_market_view_.shortcode() << " = " << dep_market_view_.secname() << " on " << hostname_
                     << "\n";

              getflat_email_string_ = t_oss_.str();
            }
            //          SendMail(getflat_email_string_, getflat_email_string_);
          }
        }
        getflat_due_to_max_loss_ = true;
      }
      return true;
    } else {
      getflat_due_to_max_loss_ = false;
    }
  } else if (livetrading_ && getflat_due_to_max_loss_) {
    return true;
  }
  // only getflat in live .. in sim ... ride and use SimBasePNL to record the low
  if (param_set_.read_max_drawdown_ && livetrading_ && !is_structured_strategy_ && !is_structured_general_strategy_ &&
      (order_manager_.base_pnl().drawdown() > param_set_.max_drawdown_)) {
    if (!getflat_due_to_max_drawdown_) {
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME << " getflat_due_to_max_drawdown_ " << runtime_id_ << DBGLOG_ENDL_FLUSH;
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
            t_oss_ << "Strategy: " << runtime_id_
                   << " getflat_due_to_max_drawdown_: " << order_manager_.base_pnl().drawdown() << " product "
                   << dep_market_view_.shortcode() << " = " << dep_market_view_.secname() << " on " << hostname_
                   << "\n";

            getflat_email_string_ = t_oss_.str();
          }

          //          SendMail(getflat_email_string_, getflat_email_string_);
        }
      }
      getflat_due_to_max_drawdown_ = true;
    }
    return true;
  } else {
    getflat_due_to_max_drawdown_ = false;
  }

  // only getflat in live .. in sim ... ride and use SimBasePNL to record the high
  if (livetrading_ && (order_manager_.base_pnl().total_pnl() > param_set_.max_pnl_)) {
    if (!getflat_due_to_max_pnl_) {
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME << " getflat_due_to_max_pnl_ " << runtime_id_ << DBGLOG_ENDL_FLUSH;
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
            t_oss_ << "Strategy: " << runtime_id_
                   << " getflat_due_to_max_pnl_: " << order_manager_.base_pnl().total_pnl() << " product "
                   << dep_market_view_.shortcode() << " = " << dep_market_view_.secname() << " on " << hostname_
                   << "\n";

            getflat_email_string_ = t_oss_.str();
          }

          //          SendMail(getflat_email_string_, getflat_email_string_);
        }
      }
      getflat_due_to_max_pnl_ = true;
    }
    return true;
  } else {
    getflat_due_to_max_pnl_ = false;
  }

  if (livetrading_ && param_set_.read_global_max_loss_ && our_global_pnl_ < -param_set_.global_max_loss_) {
    if (!getflat_due_to_global_max_loss_) {
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME << " getflat_due_to_global_max_loss_ " << our_global_pnl_ << " is less than "
                    << -param_set_.global_max_loss_ << " Strategy: " << runtime_id_ << DBGLOG_ENDL_FLUSH;
        if (livetrading_) {
          DBGLOG_DUMP;
        }
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
          t_oss_ << "Strategy: " << runtime_id_ << " getflat_due_to_global_max_loss_: " << our_global_pnl_
                 << " product " << dep_market_view_.shortcode() << " = " << dep_market_view_.secname() << " on "
                 << hostname_ << "\n";

          getflat_email_string_ = t_oss_.str();
        }

        //        SendMail(getflat_email_string_, getflat_email_string_);
      }

      getflat_due_to_global_max_loss_ = true;
    }
    return true;
  } else {
    getflat_due_to_global_max_loss_ = false;
  }

  if (livetrading_ && ((our_global_pnl_ - our_short_term_global_pnl_) < -param_set_.short_term_global_max_loss_)) {
    if (!getflat_due_to_short_term_global_max_loss_) {
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME << " getflat_due_to_short_term_global_max_loss_ " << (our_global_pnl_ - our_short_term_global_pnl_)
                    << " is less than " << -param_set_.short_term_global_max_loss_ << " Strategy: " << runtime_id_
                    << DBGLOG_ENDL_FLUSH;
        if (livetrading_) {
          DBGLOG_DUMP;
        }
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
          t_oss_ << "Strategy: " << runtime_id_
                 << " getflat_due_to_short_term_global_max_loss_: " << (our_global_pnl_ - our_short_term_global_pnl_)
                 << " product " << dep_market_view_.shortcode() << " = " << dep_market_view_.secname() << " on "
                 << hostname_ << "\n";

          getflat_email_string_ = t_oss_.str();
        }
        //        SendMail(getflat_email_string_, getflat_email_string_);
      }

      last_getflat_due_to_short_term_global_max_loss_triggered_at_msecs_ = watch_.msecs_from_midnight();
      getflat_due_to_short_term_global_max_loss_ = true;
    }
    return true;
  } else {
    getflat_due_to_short_term_global_max_loss_ = false;
  }

  if (getflat_due_to_max_opentrade_loss_) {
    return true;
  }

  if (getflat_due_to_non_tradable_events_ && is_event_based_) {
    return true;
  }
  // if ( getflat_due_to_max_pertrade_loss_ )
  //   {
  // 	return true;
  //   }

  if (getflat_due_to_allowed_economic_event_) {
    return true;
  }

  if (enable_non_standard_check_ && getflat_due_to_non_standard_market_conditions_) {
    return true;
  }

  // TODO .. add support for different Economic Zones, based on basename
  // and possibly override by paramfile

  // Another TODO ... change the threshold of severity based on the strategy parameters
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

  /*  getflat due to large_price_mmovement, Currently used in Alphaflash execs */
  if (getflat_due_to_lpm_) {
    return true;
  }

  return false;
}

void BaseTrading::SetComputeTradePrice(bool set_) {
  HFSAT::ShortcodeSecurityMarketViewMap &shortcode_smv_map_ =
      HFSAT::ShortcodeSecurityMarketViewMap::GetUniqueInstance();

  for (unsigned i = 0; i < this_model_source_shortcode_vec_.size(); i++) {
    SecurityMarketView *this_smv_ = shortcode_smv_map_.GetSecurityMarketView(this_model_source_shortcode_vec_[i]);
    this_smv_->compute_trade_prices_ = set_;
  }
  computing_trade_prices_ = set_;
}

void BaseTrading::ProcessTimePeriodUpdate(const int num_pages_to_add_) {
  // We have shifted their handling to individual functions
  ComputeCurrentSeverity();
  HandleFreeze();

// update hysterisis if based on age alone
#ifndef USING_SELF_CALCULATED_QUEUE_SIZES
  exec_logic_indicators_helper_->RecomputeHysterisis();
#endif

  if (!is_structured_strategy_ && !is_pair_strategy_ && !is_structured_general_strategy_) {
    if (!getflat_due_to_max_opentrade_loss_) {
      // check open-trade loss
      if (order_manager_.base_pnl().opentrade_unrealized_pnl() < -max_opentrade_loss_) {
        if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
          DBGLOG_TIME << "getflat_due_to_max_opentrade_loss_ ! Current opentradepnl : "
                      << order_manager_.base_pnl().opentrade_unrealized_pnl() << " < " << -max_opentrade_loss_
                      << " Strategy: " << runtime_id_ << DBGLOG_ENDL_FLUSH;
          if (livetrading_) {
            DBGLOG_DUMP;
          }
        }
        getflat_due_to_max_opentrade_loss_ = true;
        last_getflat_due_to_max_opentrade_loss_triggered_at_msecs_ = watch_.msecs_from_midnight();
        num_opentrade_loss_hits_++;
      }
    } else {
      if ((last_getflat_due_to_max_opentrade_loss_triggered_at_msecs_ > 0) &&
          // should atleast getflat before waking up, will be useful when breakmsecs is
          // very small and open_pnl keep oscilliating aroung max_opentrade_loss_
          (watch_.msecs_from_midnight() - last_getflat_due_to_max_opentrade_loss_triggered_at_msecs_ >
           break_msecs_on_max_opentrade_loss_) &&
          (GetPositionToClose() == 0)) {
        getflat_due_to_max_opentrade_loss_ = false;
      }
    }

    if (!getflat_due_to_short_term_global_max_loss_) {
      // check opentrade loss
      if (our_global_pnl_ - our_short_term_global_pnl_ < -param_set_.short_term_global_max_loss_) {
        if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
          DBGLOG_TIME << "getflat_due_to_short_term_global_max_loss_ ! Current ShortTermGlobalLoss : "
                      << our_global_pnl_ - our_short_term_global_pnl_ << " < "
                      << -param_set_.short_term_global_max_loss_ << " Strategy: " << runtime_id_ << DBGLOG_ENDL_FLUSH;
          if (livetrading_) {
            DBGLOG_DUMP;
          }
        }
        getflat_due_to_short_term_global_max_loss_ = true;
        last_getflat_due_to_short_term_global_max_loss_triggered_at_msecs_ = watch_.msecs_from_midnight();
      }
    } else {
      if ((last_getflat_due_to_short_term_global_max_loss_triggered_at_msecs_ > 0) &&
          (watch_.msecs_from_midnight() - last_getflat_due_to_short_term_global_max_loss_triggered_at_msecs_ >
           param_set_.short_term_global_max_loss_getflat_msecs_)) {
        getflat_due_to_short_term_global_max_loss_ = false;
      }
    }

    if (watch_.msecs_from_midnight() - last_short_term_global_pnl_updated_msecs_ >
        param_set_.msecs_for_short_term_global_max_loss_ / 2) {
      our_short_term_global_pnl_ = our_short_term_global_pnl_latest_;
      our_short_term_global_pnl_latest_ = our_global_pnl_;
    }

    ProcessAllowedEco();
  }

  int t_position = my_position_;
  if (is_pair_strategy_) {
    t_position = my_combined_position_;
  } else if (is_structured_strategy_) {
    t_position = my_risk_;
  }

  exec_logic_indicators_helper_->ProcessOnTimePeriodUpdate(num_pages_to_add_, &param_set_, t_position);

  if (param_set_.read_max_min_diff_) {
    l1_bias_ = exec_logic_indicators_helper_->l1_bias();
  }

  if (param_set_.read_max_min_diff_order_) {
    l1_order_bias_ = exec_logic_indicators_helper_->l1_order_bias();
  }

  if (param_set_.read_high_uts_factor_) {
    if (exec_logic_indicators_helper_->updated_factor_this_round()) {
      ProcessGetFlat();
      if (is_ready_) TradeVarSetLogic(my_position_);
    }
  }

  if (param_set_.read_bucket_size_ && param_set_.read_positioning_thresh_decrease_) {
    long_positioning_bias_ = exec_logic_indicators_helper_->long_positioning_bias();
    short_positioning_bias_ = exec_logic_indicators_helper_->short_positioning_bias();
  }

  if (param_set_.read_scale_max_pos_) {
    volume_adj_max_pos_ = exec_logic_indicators_helper_->volume_adj_max_pos(&param_set_);
  }

  // this is bigtimeperiod ( number of secs till last call ) update call
  if (param_set_.online_stats_avg_dep_bidask_spread_) {
    bool t_is_ready_ = true;
    moving_avg_dep_bidask_spread_ = p_moving_bidask_spread_->indicator_value(t_is_ready_);
  }

  // Computing online stdev of model sumvars
  // for Threshold scaling
  if (read_compute_model_stdev_) {
    moving_avg_sumvars_ =
        (targetbias_numbers_ * model_stdev_inv_decay_sum_) + (moving_avg_sumvars_ * model_stdev_decay_page_factor_);
    moving_avg_squared_sumvars_ = (targetbias_numbers_ * targetbias_numbers_ * model_stdev_inv_decay_sum_) +
                                  (moving_avg_squared_sumvars_ * model_stdev_decay_page_factor_);
    model_stdev_ = sqrt(moving_avg_squared_sumvars_ - (moving_avg_sumvars_ * moving_avg_sumvars_));

    online_model_scale_fact_ =
        std::max(param_set_.min_model_scale_fact_,
                 std::min(param_set_.max_model_scale_fact_, (model_stdev_ / param_set_.offline_model_stdev_)));
  }
}

void BaseTrading::OnTimePeriodUpdate(const int num_pages_to_add_) {
  if (watch_.msecs_from_midnight() > trading_start_utc_mfm_ && getflat_due_to_feature_model_ == -1) {
    if (param_set_.read_feature_modelfile_ && param_set_.read_feature_threshold_) {
      FeaturePredictor ft_pred(param_set_.feature_modelfile_, watch_.YYYYMMDD(), livetrading_);
      ft_pred.InitializeModel();
      double estimated_ft = ft_pred.GetModelPrediction();

      if (param_set_.feature_more_ > 0 && estimated_ft < param_set_.feature_threshold_) {
        DBGLOG_TIME_CLASS << "Not Starting because of feature model. Predicted value: " << estimated_ft
                          << "  < Threshold: " << param_set_.feature_threshold_ << DBGLOG_ENDL_FLUSH;
        getflat_due_to_feature_model_ = 1;
      } else if (param_set_.feature_more_ < 0 && estimated_ft > param_set_.feature_threshold_) {
        DBGLOG_TIME_CLASS << "Not Starting because of feature model. Predicted value: " << estimated_ft
                          << "  > Threshold: " << param_set_.feature_threshold_ << DBGLOG_ENDL_FLUSH;
        getflat_due_to_feature_model_ = 1;
      } else
        getflat_due_to_feature_model_ = 0;
    } else
      getflat_due_to_feature_model_ = 0;
  }
  if (!livetrading_) {
    if (sample_index_ < pnl_sampling_timestamps_.size() &&
        watch_.msecs_from_midnight() >= pnl_sampling_timestamps_[sample_index_]) {
      pnl_samples_.push_back((int)(order_manager_.base_pnl().total_pnl()));
      sample_index_++;
    }
  }
  ProcessTimePeriodUpdate(num_pages_to_add_);

  if (param_set_.check_mur_reset_time_ && watch_.msecs_from_midnight() > param_set_.mur_reset_time_) {
    param_set_.check_mur_reset_time_ = false;
    for (auto i = 0u; i < param_set_vec_.size(); i++) {
      param_set_vec_[i].max_position_ =
          std::min(param_set_vec_[i].max_position_,
                   (int)(std::max(1, param_set_.mur_reset_value_) * param_set_vec_[i].unit_trade_size_ + 0.5));
      param_set_vec_[i].max_position_original_ = param_set_vec_[i].max_position_;
    }

    param_set_.max_position_ = param_set_vec_[param_index_to_use_].max_position_;
    param_set_.max_position_original_ = param_set_.max_position_;
    BuildTradeVarSets();
    ProcessGetFlat();
    if (is_ready_) TradeVarSetLogic(my_position_);
  }

  if (param_set_.read_min_size_to_quote_ && param_set_.read_max_bid_ask_order_diff_) {
    ProcessGetFlat();
    if (watch_.msecs_from_midnight() > trading_start_utc_mfm_ && dep_market_view_.is_ready() &&
        !should_be_getting_flat_) {
      CallPlaceCancelNonBestLevels();
    }
  }

  if (param_set_.sgx_market_making_) {
    if (watch_.msecs_from_midnight() > trading_start_utc_mfm_ && dep_market_view_.is_ready()) {
      double quoting_fulfilled_ = 0;
      int spread_ = best_nonself_ask_int_price_ - best_nonself_bid_int_price_;

      int min_size = 0, spread_required = 0;
      if (dep_market_view_.shortcode() == "SGX_NK_0") {
        min_size = 5;
        spread_required = 2;
      } else if (dep_market_view_.shortcode() == "SGX_CN_0") {
        min_size = 10;
        spread_required = 8;
      } else if (dep_market_view_.shortcode() == "SGX_CN_1") {
        min_size = 10;
        spread_required = 16;
      }

      for (int bid_distance = 0; bid_distance <= spread_required - spread_; bid_distance++) {
        int bid_size = order_manager_.SumBidSizeConfirmedEqAboveIntPrice(best_nonself_bid_int_price_ - bid_distance);
        int ask_size = order_manager_.SumAskSizeConfirmedEqAboveIntPrice(best_nonself_ask_int_price_ + spread_required -
                                                                         bid_distance - spread_);
        if (bid_size >= min_size && ask_size >= min_size) {
          quoting_fulfilled_ = 1;
          break;
        }
      }

      if ((quoting_fulfilled_ == 1) && quoting_) {
        total_time_quoted_ += (watch_.msecs_from_midnight() - last_msecs_quoted_);
        // std::cerr << watch_.msecs_from_midnight() << " " << last_msecs_quoted_ << " " << (
        // watch_.msecs_from_midnight() - last_msecs_quoted_ ) << "\n";
        last_msecs_quoted_ = watch_.msecs_from_midnight();
      } else if (quoting_fulfilled_ == 1) {
        quoting_ = true;
        last_msecs_quoted_ = watch_.msecs_from_midnight();
      } else if ((quoting_fulfilled_ == 0) && quoting_) {
        quoting_ = false;
      } else if (quoting_fulfilled_ == 0) {
        quoting_ = false;
      }
    }
  }
  // for debug code INdicator info
  if (dbglogger_.CheckLoggingLevel(INDICATOR_INFO)) {
    char buf[1024] = {0};
    // print the trade file line check for flat or open
    if (sec_counter_for_INDINFO % 60 == 0) {
      sprintf(buf, "INDINFOTIME %10d.%06d %4f \n", watch_.tv().tv_sec, watch_.tv().tv_usec,
              dep_market_view_.mkt_size_weighted_price());

      dbglogger_ << buf;
    }

    dbglogger_.CheckToFlushBuffer();
    sec_counter_for_INDINFO++;
  }
}

/**
 * 0 position corresponds to index MAX_POS_MAP_SIZE,
 * map_pos_increment_ = (int)std::max ( 1, ( param_set_.max_position_ / MAX_POS_MAP_SIZE ) ) )
 * idx=2*MAX_POS_MAP_SIZE corresponds to position = ( MAX_POS_MAP_SIZE * map_pos_increment_ )
 * idx=0 corresponds to position = ( MAX_POS_MAP_SIZE * map_pos_increment_ )
 * position P corresponds to idx= MAX_POS_MAP_SIZE + ( P / map_pos_increment_ )
 */

int BaseTrading::GetPositonToTradeVarsetMapIndex(int t_position_) {
  int this_position_tradevarset_map_index_ = P2TV_zero_idx_;
  // only when param_set_.max_position_ > MAX_POS_MAP_SIZE
  if (map_pos_increment_ > 1) {
    if (t_position_ > 0) {
      this_position_tradevarset_map_index_ += std::min(MAX_POS_MAP_SIZE, (abs(t_position_) / map_pos_increment_));
    } else {
      this_position_tradevarset_map_index_ -= std::min(MAX_POS_MAP_SIZE, (abs(t_position_) / map_pos_increment_));
    }
  } else {
    if (t_position_ > 0) {
      this_position_tradevarset_map_index_ += std::min(MAX_POS_MAP_SIZE, abs(t_position_));
    } else {
      this_position_tradevarset_map_index_ -= std::min(MAX_POS_MAP_SIZE, abs(t_position_));
    }
  }
  return this_position_tradevarset_map_index_;
}

void BaseTrading::TradeVarSetLogic(int t_position) {
  current_position_tradevarset_map_index_ = P2TV_zero_idx_;
  // only when param_set_.max_position_ > MAX_POS_MAP_SIZE
  if (map_pos_increment_ > 1) {
    if (t_position > 0) {
      current_position_tradevarset_map_index_ += std::min(MAX_POS_MAP_SIZE, (abs(t_position) / map_pos_increment_));
      current_tradevarset_ = position_tradevarset_map_[current_position_tradevarset_map_index_];
      if ((current_tradevarset_.l1bid_trade_size_ + t_position) > param_set_.max_position_) {
        current_tradevarset_.l1bid_trade_size_ = MathUtils::GetFlooredMultipleOf(
            std::max(0, param_set_.max_position_ - t_position), dep_market_view_.min_order_size());
      }
    } else {
      current_position_tradevarset_map_index_ -= std::min(MAX_POS_MAP_SIZE, (abs(t_position) / map_pos_increment_));
      current_tradevarset_ = position_tradevarset_map_[current_position_tradevarset_map_index_];
      if ((current_tradevarset_.l1ask_trade_size_ - t_position) > param_set_.max_position_) {
        current_tradevarset_.l1ask_trade_size_ = MathUtils::GetFlooredMultipleOf(
            std::max(0, param_set_.max_position_ + t_position), dep_market_view_.min_order_size());
      }
    }
  } else {
    if (t_position > 0) {
      current_position_tradevarset_map_index_ += std::min(MAX_POS_MAP_SIZE, abs(t_position));
    } else {
      current_position_tradevarset_map_index_ -= std::min(MAX_POS_MAP_SIZE, abs(t_position));
    }
    current_tradevarset_ = position_tradevarset_map_[current_position_tradevarset_map_index_];
  }

  if (param_set_.read_high_uts_factor_) {
    double buy_uts_factor_ = exec_logic_indicators_helper_->buy_uts_factor();
    double sell_uts_factor_ = exec_logic_indicators_helper_->sell_uts_factor();

    if (buy_uts_factor_ < FAT_FINGER_FACTOR) {
      current_tradevarset_.l1bid_trade_size_ *= buy_uts_factor_;
      current_tradevarset_.l1bid_trade_size_ =
          std::min(current_tradevarset_.l1bid_trade_size_, param_set_.max_position_ - t_position);
    }

    if (sell_uts_factor_ < FAT_FINGER_FACTOR) {
      current_tradevarset_.l1ask_trade_size_ *= sell_uts_factor_;
      current_tradevarset_.l1ask_trade_size_ =
          std::min(current_tradevarset_.l1ask_trade_size_, param_set_.max_position_ + t_position);
    }
  }

  if (param_set_.read_max_global_position_) {
    if ((my_global_position_ >= param_set_.max_global_position_) && (t_position >= 0)) {
      // All queries are together too long
      current_tradevarset_.l1bid_trade_size_ = 0;
      int canceled_bid_size_ = order_manager_.CancelBidsAtIntPrice(best_nonself_bid_int_price_);

      if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        DBGLOG_TIME_CLASS_FUNC << " GlobalMaxPosition = " << param_set_.max_global_position_
                               << " my_global_position_ =  " << my_global_position_
                               << " canceled_bid_size_ = " << canceled_bid_size_ << " @ " << best_nonself_bid_int_price_
                               << DBGLOG_ENDL_FLUSH;
      }
    } else {
      if ((my_global_position_ <= -param_set_.max_global_position_) && (t_position <= 0)) {
        // All queries are together too short
        current_tradevarset_.l1ask_trade_size_ = 0;
        int canceled_ask_size_ = order_manager_.CancelAsksAtIntPrice(best_nonself_ask_int_price_);

        if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
          DBGLOG_TIME_CLASS_FUNC << " GlobalMaxPosition = " << param_set_.max_global_position_
                                 << " my_global_position_ =  " << my_global_position_
                                 << " canceled_ask_size_ = " << canceled_ask_size_ << " @ "
                                 << best_nonself_ask_int_price_ << DBGLOG_ENDL_FLUSH;
        }
      }
    }
  }

  if (param_set_.read_max_security_position_) {
    if (current_risk_mapped_to_product_position_ >=
        param_set_.max_security_position_) {  //  too long in same underlying
      current_tradevarset_.l1bid_trade_size_ = 0;
    } else {
      if (current_risk_mapped_to_product_position_ <=
          -param_set_.max_security_position_) {  //  too short in same underlying
        current_tradevarset_.l1ask_trade_size_ = 0;
      }
    }
  }

  // This is meant to decrease MUR based on volumes
  if (param_set_.read_scale_max_pos_) {
    if ((current_tradevarset_.l1bid_trade_size_ + t_position > volume_adj_max_pos_) && (t_position >= 0)) {
      // Query is too long
      current_tradevarset_.l1bid_trade_size_ = std::max(0, volume_adj_max_pos_ - t_position);
    } else {
      if ((current_tradevarset_.l1ask_trade_size_ - t_position > volume_adj_max_pos_) && (t_position <= 0)) {
        // Query is too short
        current_tradevarset_.l1ask_trade_size_ = std::max(0, volume_adj_max_pos_ + t_position);
      }
    }
  }
  ModifyThresholdsAsPerVolatility();
  ModifyThresholdsAsPerModelStdev();
  ModifyThresholdsAsPerPreGetFlat();
  ModifyThresholdsAsPerMktConditions();

  ProcessRegimeChange();

  if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
    DBGLOG_TIME_CLASS_FUNC << " newpos: " << t_position << " gpos: " << my_global_position_ << " stddpfac "
                           << stdev_scaled_capped_in_ticks_ << " mapidx " << current_position_tradevarset_map_index_
                           << ' ' << ToString(current_tradevarset_).c_str() << DBGLOG_ENDL_FLUSH;
  }

  if (current_tradevarset_.l1bid_trade_size_ < param_set_.min_allowed_unit_trade_size_) {
    current_tradevarset_.l1bid_trade_size_ = 0;
  }

  if (current_tradevarset_.l1ask_trade_size_ < param_set_.min_allowed_unit_trade_size_) {
    current_tradevarset_.l1ask_trade_size_ = 0;
  }

  // no need to have multiple order check, because we are anyway modifying it before usage
  current_bid_tradevarset_ = current_tradevarset_;
  current_bid_keep_tradevarset_ = current_tradevarset_;
  current_ask_tradevarset_ = current_tradevarset_;
  current_ask_keep_tradevarset_ = current_tradevarset_;

  if (is_ready_ && !external_freeze_trading_ && !should_be_getting_flat_ && dep_market_view_.is_ready() &&
      (throttle_manager_->allowed_through_throttle(watch_.msecs_from_midnight()))) {
    TradingLogic();
  }

  // UpdateThresholds () ; no need to call this
}

void BaseTrading::UpdateThresholds() {
  if (stdev_scaled_capped_in_ticks_ > 1 &&
      should_increase_thresholds_in_volatile_times_) {  // more than normally volatile

    if (param_set_.increase_thresholds_symm_) {
      current_bid_tradevarset_.MultiplyBidsBy(stdev_scaled_capped_in_ticks_);
      current_bid_keep_tradevarset_.MultiplyBidsBy(stdev_scaled_capped_in_ticks_);
      current_ask_tradevarset_.MultiplyAsksBy(stdev_scaled_capped_in_ticks_);
      current_ask_keep_tradevarset_.MultiplyAsksBy(stdev_scaled_capped_in_ticks_);
    } else {
      // only scaling increasing thresholds
      if (my_position_ >= 0) {
        current_bid_tradevarset_.MultiplyBidsBy(stdev_scaled_capped_in_ticks_);
        current_bid_keep_tradevarset_.MultiplyBidsBy(stdev_scaled_capped_in_ticks_);
      }
      if (my_position_ <= 0) {
        current_ask_tradevarset_.MultiplyAsksBy(stdev_scaled_capped_in_ticks_);
        current_ask_keep_tradevarset_.MultiplyAsksBy(stdev_scaled_capped_in_ticks_);
      }
    }
  }

  if (param_set_.use_pre_getflat_) {
    int msecs_since_pre_getflat_ =
        watch_.msecs_from_midnight() - (trading_end_utc_mfm_ - param_set_.pre_getflat_msecs_);
    if (msecs_since_pre_getflat_ >= 0) {
      // double factor = exp(0.000001*param_set_.pre_getflat_multiplier_ * msecs_since_pre_getflat_) ;
      // AB: I think we can approximate exp with following linear fn : e^x ~ 1+x for x~0, to be confirmed aashay
      double factor =
          1 + param_set_.pre_getflat_multiplier_ * (double(msecs_since_pre_getflat_) / param_set_.pre_getflat_msecs_);

      if (my_position_ >= 0) {
        current_bid_tradevarset_.MultiplyBidsBy(factor);
        current_bid_keep_tradevarset_.MultiplyBidsBy(factor);
        current_ask_tradevarset_.MultiplyBidsBy(factor);
        current_ask_keep_tradevarset_.MultiplyBidsBy(factor);
      }
      if (my_position_ <= 0) {
        current_bid_tradevarset_.MultiplyAsksBy(factor);
        current_bid_keep_tradevarset_.MultiplyAsksBy(factor);
        current_ask_tradevarset_.MultiplyAsksBy(factor);
        current_ask_keep_tradevarset_.MultiplyAsksBy(factor);
      }
    }
  }
}

/** \brief only update internal variables that depict the topmost bid price and topmost ask price where our own orders
 *are not a huge part
 *
 * first rescale best_nonself_levels
 * best_nonself_levels are the topmost market prices at which this queries' orders are not a huge part of the market.
 * In future this computation will be made global,
 * As in we should receive all order routing messages for all our queries and construct a self market book.
 */
void BaseTrading::NonSelfMarketUpdate() {
  if (param_set_.read_max_self_ratio_at_level_) {
    order_manager_.GetBestNonSelfBid(param_set_.max_self_ratio_at_level_, best_nonself_bid_price_,
                                     best_nonself_bid_int_price_, best_nonself_bid_size_);
    order_manager_.GetBestNonSelfAsk(param_set_.max_self_ratio_at_level_, best_nonself_ask_price_,
                                     best_nonself_ask_int_price_, best_nonself_ask_size_);
  } else {
    if (param_set_.use_stable_bidask_levels_) {
      best_nonself_bid_price_ = dep_market_view_.GetDoublePx(dep_market_view_.bid_side_valid_level_int_price_);
      best_nonself_bid_int_price_ = dep_market_view_.bid_side_valid_level_int_price_;
      best_nonself_bid_size_ = dep_market_view_.bid_side_valid_level_size_;

      best_nonself_ask_price_ = dep_market_view_.GetDoublePx(dep_market_view_.ask_side_valid_level_int_price_);
      best_nonself_ask_int_price_ = dep_market_view_.ask_side_valid_level_int_price_;
      best_nonself_ask_size_ = dep_market_view_.bid_side_valid_level_size_;
    } else {
      best_nonself_bid_price_ = dep_market_view_.bestbid_price();
      if (param_set_.allowed_to_improve_ && best_nonself_bid_int_price_ > dep_market_view_.bestbid_int_price() &&
          bid_improve_keep_) {
        order_manager_.ResetBidsAsImproveUponCancellation(dep_market_view_.bestbid_int_price(),
                                                          dep_market_view_.bestask_int_price());
      }
      best_nonself_bid_int_price_ = dep_market_view_.bestbid_int_price();
      best_nonself_bid_size_ = dep_market_view_.bestbid_size();

      best_nonself_ask_price_ = dep_market_view_.bestask_price();
      if (param_set_.allowed_to_improve_ && best_nonself_ask_int_price_ < dep_market_view_.bestask_int_price() &&
          ask_improve_keep_) {
        order_manager_.ResetAsksAsImproveUponCancellation(dep_market_view_.bestbid_int_price(),
                                                          dep_market_view_.bestask_int_price());
      }
      best_nonself_ask_int_price_ = dep_market_view_.bestask_int_price();
      best_nonself_ask_size_ = dep_market_view_.bestask_size();
    }
  }

  {
    product_->best_nonself_bid_price_ = best_nonself_bid_price_;
    product_->best_nonself_bid_int_price_ = best_nonself_bid_int_price_;
    product_->best_nonself_bid_size_ = best_nonself_bid_size_;
    product_->best_nonself_ask_price_ = best_nonself_ask_price_;
    product_->best_nonself_ask_int_price_ = best_nonself_ask_int_price_;
    product_->best_nonself_ask_size_ = best_nonself_ask_size_;
  }

  best_nonself_mid_price_ = (best_nonself_bid_price_ + best_nonself_ask_price_) / 2.0;
  best_nonself_mkt_price_ =
      (best_nonself_bid_price_ * best_nonself_ask_size_ + best_nonself_ask_price_ * best_nonself_bid_size_) /
      (best_nonself_bid_size_ + best_nonself_ask_size_);

  current_tradevarset_ = position_tradevarset_map_[current_position_tradevarset_map_index_];
  if (map_pos_increment_ > 1)  // only when param_set_.max_position_ > MAX_POS_MAP_SIZE
  {
    auto t_position = my_position_;
    if (is_structured_strategy_) {
      t_position = my_risk_;
    }

    if (t_position > 0) {
      if ((current_tradevarset_.l1bid_trade_size_ + t_position) > param_set_.max_position_) {
        current_tradevarset_.l1bid_trade_size_ = MathUtils::GetFlooredMultipleOf(
            std::max(0, param_set_.max_position_ - t_position), dep_market_view_.min_order_size());
      }
    } else {
      if ((current_tradevarset_.l1ask_trade_size_ - t_position) > param_set_.max_position_) {
        current_tradevarset_.l1ask_trade_size_ = MathUtils::GetFlooredMultipleOf(
            std::max(0, param_set_.max_position_ + t_position), dep_market_view_.min_order_size());
      }
    }
  }

  if (param_set_.read_max_global_position_ && !is_pair_strategy_ && !is_structured_strategy_ &&
      !is_structured_general_strategy_) {
    if ((my_global_position_ >= param_set_.max_global_position_) && (my_position_ >= 0)) {
      // All queries are together too long
      current_tradevarset_.l1bid_trade_size_ = 0;
    } else {
      if ((my_global_position_ <= -param_set_.max_global_position_) && (my_position_ <= 0)) {
        // All queries are together too short
        current_tradevarset_.l1ask_trade_size_ = 0;
      }
    }
  }

  if (param_set_.read_max_security_position_) {
    if (current_risk_mapped_to_product_position_ >= param_set_.max_security_position_) {
      //  too long in same underlying
      current_tradevarset_.l1bid_trade_size_ = 0;
    } else {
      if (current_risk_mapped_to_product_position_ <= -param_set_.max_security_position_) {
        //  too short in same underlying
        current_tradevarset_.l1ask_trade_size_ = 0;
      }
    }
  }

  // This is meant to decrease MUR based on volumes
  if (param_set_.read_scale_max_pos_) {
    auto t_position = my_position_;
    if (is_structured_strategy_) {
      t_position = my_risk_;
    }
    if ((current_tradevarset_.l1bid_trade_size_ + t_position > volume_adj_max_pos_) &&
        (t_position >= 0)) {  // Query is too long
      current_tradevarset_.l1bid_trade_size_ = std::max(0, volume_adj_max_pos_ - t_position);
    } else {
      if ((current_tradevarset_.l1ask_trade_size_ - t_position > volume_adj_max_pos_) &&
          (t_position <= 0)) {  // Query is too short
        current_tradevarset_.l1ask_trade_size_ = std::max(0, t_position + volume_adj_max_pos_);
      }
    }
  }

  ModifyThresholdsAsPerVolatility();
  ModifyThresholdsAsPerModelStdev();
  ModifyThresholdsAsPerPreGetFlat();
  ModifyThresholdsAsPerMktConditions();

  if (current_tradevarset_.l1bid_trade_size_ < param_set_.min_allowed_unit_trade_size_) {
    current_tradevarset_.l1bid_trade_size_ = 0;
  }

  if (current_tradevarset_.l1ask_trade_size_ < param_set_.min_allowed_unit_trade_size_) {
    current_tradevarset_.l1ask_trade_size_ = 0;
  }

  if (param_set_.read_trade_bias_) {
    if (my_position_ > 0 && exec_logic_indicators_helper_->trade_bias() < 0) {
      trade_bias_sell_ = -1.0 * exec_logic_indicators_helper_->trade_bias();
      trade_bias_buy_ = 0;
    } else if (my_position_ < 0 && exec_logic_indicators_helper_->trade_bias() > 0) {
      trade_bias_buy_ = exec_logic_indicators_helper_->trade_bias();
      trade_bias_sell_ = 0;
    } else {
      trade_bias_buy_ = 0;
      trade_bias_sell_ = 0;
    }
  } else {
    trade_bias_buy_ = 0;
    trade_bias_sell_ = 0;
  }

  if (param_set_.read_cancel_bias_) {
    if (exec_logic_indicators_helper_->cancel_bias() < 0) {
      cancel_bias_buy_ = -1.0 * exec_logic_indicators_helper_->cancel_bias();
      cancel_bias_sell_ = 0;
    } else {
      cancel_bias_sell_ = exec_logic_indicators_helper_->cancel_bias();
      cancel_bias_buy_ = 0;
    }
  } else {
    cancel_bias_buy_ = 0;
    cancel_bias_sell_ = 0;
  }

  if (param_set_.read_implied_mkt_) {
    if (exec_logic_indicators_helper_->implied_price()) {
      if (exec_logic_indicators_helper_->implied_price() - best_nonself_bid_price_ <=
              param_set_.implied_mkt_thres_factor_ * dep_market_view_.min_price_increment() &&
          best_nonself_bid_size_ < param_set_.max_size_to_cancel_) {
        implied_mkt_bid_flag_ = false;
      } else {
        implied_mkt_bid_flag_ = true;
      }
      if (best_nonself_ask_price_ - exec_logic_indicators_helper_->implied_price() <=
              param_set_.implied_mkt_thres_factor_ * dep_market_view_.min_price_increment() &&
          best_nonself_ask_size_ < param_set_.max_size_to_cancel_) {
        implied_mkt_ask_flag_ = false;
      } else {
        implied_mkt_ask_flag_ = true;
      }
    }
  }
  ProcessRegimeChange();

  current_bid_tradevarset_ = current_tradevarset_;
  current_bid_keep_tradevarset_ = current_tradevarset_;
  current_ask_tradevarset_ = current_tradevarset_;
  current_ask_keep_tradevarset_ = current_tradevarset_;

  // recompute ticks_to_keep_bid_int_price_ &
  // ticks_to_keep_ask_int_price_ based on how the market has changed
  if ((param_set_.num_increase_ticks_to_keep_ > 0 || param_set_.num_decrease_ticks_to_keep_ > 0) &&
      order_manager_.base_pnl().AverageOpenPrice() >
          0) {               // won't enter for paramsets w/o these specifications ( majority )
    if (my_position_ > 0) {  // now long

      if (param_set_.num_increase_ticks_to_keep_ > 0) {
        // price level to keep an order on which will increase position on execution.
        ticks_to_keep_bid_int_price_ =
            ((order_manager_.base_pnl().AverageOpenPrice() + dep_market_view_.min_price_increment() * 0.5) /
             dep_market_view_.min_price_increment()) -
            param_set_.num_increase_ticks_to_keep_;

        if (ticks_to_keep_bid_int_price_ > best_nonself_bid_int_price_) {
          ticks_to_keep_bid_int_price_ = best_nonself_bid_int_price_;
        }
      }

      if (param_set_.num_decrease_ticks_to_keep_ > 0) {
        // price level to keep an order on which will decrease position on execution.
        ticks_to_keep_ask_int_price_ =
            ((order_manager_.base_pnl().AverageOpenPrice() + dep_market_view_.min_price_increment() * 0.5) /
             dep_market_view_.min_price_increment()) +
            param_set_.num_decrease_ticks_to_keep_;

        if (ticks_to_keep_ask_int_price_ < best_nonself_ask_int_price_) {
          ticks_to_keep_ask_int_price_ = best_nonself_ask_int_price_;
        }
      }
    } else if (my_position_ < 0) {  // now short

      if (param_set_.num_increase_ticks_to_keep_ > 0) {
        // price level to keep an order on which will increase position on execution.
        ticks_to_keep_ask_int_price_ =
            ((order_manager_.base_pnl().AverageOpenPrice() - dep_market_view_.min_price_increment() * 0.5) /
             dep_market_view_.min_price_increment()) +
            param_set_.num_increase_ticks_to_keep_;

        if (ticks_to_keep_ask_int_price_ < best_nonself_ask_int_price_) {
          ticks_to_keep_ask_int_price_ = best_nonself_ask_int_price_;
        }
      }

      if (param_set_.num_decrease_ticks_to_keep_ > 0) {
        // price level to keep an order on which will decrease position on execution.
        ticks_to_keep_bid_int_price_ =
            ((order_manager_.base_pnl().AverageOpenPrice() - dep_market_view_.min_price_increment() * 0.5) /
             dep_market_view_.min_price_increment()) -
            param_set_.num_decrease_ticks_to_keep_;

        if (ticks_to_keep_bid_int_price_ > best_nonself_bid_int_price_) {
          ticks_to_keep_bid_int_price_ = best_nonself_bid_int_price_;
        }
      }
    }

    if (my_position_) {  // Place keep orders.

      if (ticks_to_keep_bid_int_price_ &&
          (ticks_to_keep_bid_int_price_ <= best_nonself_bid_int_price_) &&  // don't wish to aggress on this param.
          order_manager_.GetTotalBidSizeOrderedAtIntPx(ticks_to_keep_bid_int_price_) ==
              0) {  // if no orders at this level
        order_manager_.SendTradeIntPx(ticks_to_keep_bid_int_price_,
                                      position_tradevarset_map_[P2TV_zero_idx_].l1bid_trade_size_, kTradeTypeBuy, 'S');
        if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {  // elevate to error-level till satisfied
          DBGLOG_TIME_CLASS_FUNC << " TicksToKeepBidIntPrice SupportingSendTrade B of "
                                 << position_tradevarset_map_[P2TV_zero_idx_].l1bid_trade_size_ << " @ "
                                 << (dep_market_view_.min_price_increment() * ticks_to_keep_bid_int_price_)
                                 << " IntPx: " << ticks_to_keep_bid_int_price_
                                 << " level: " << (best_nonself_bid_int_price_ - ticks_to_keep_bid_int_price_)
                                 << " mkt: " << best_nonself_bid_size_ << " @ " << best_nonself_bid_price_ << " X "
                                 << best_nonself_ask_price_ << " @ " << best_nonself_ask_size_ << " sumbidsizes "
                                 << order_manager_.SumBidSizes() << " + my_position_ " << my_position_
                                 << DBGLOG_ENDL_FLUSH;
        }
      }
      if (ticks_to_keep_ask_int_price_ &&
          (ticks_to_keep_ask_int_price_ >= best_nonself_ask_int_price_) &&  // don't wish to aggress on this param.
          order_manager_.GetTotalAskSizeOrderedAtIntPx(ticks_to_keep_ask_int_price_) ==
              0) {  // if no orders at this level
        order_manager_.SendTradeIntPx(ticks_to_keep_ask_int_price_,
                                      position_tradevarset_map_[P2TV_zero_idx_].l1ask_trade_size_, kTradeTypeSell, 'S');
        if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {  // elevate to error-level till satisfied
          DBGLOG_TIME_CLASS_FUNC << " TicksToKeepAskIntPrice SupportingSendTrade S of "
                                 << position_tradevarset_map_[P2TV_zero_idx_].l1ask_trade_size_ << " @ "
                                 << (dep_market_view_.min_price_increment() * ticks_to_keep_ask_int_price_)
                                 << " IntPx: " << ticks_to_keep_ask_int_price_
                                 << " level: " << (ticks_to_keep_ask_int_price_ - best_nonself_ask_int_price_)
                                 << " mkt: " << best_nonself_bid_size_ << " @ " << best_nonself_bid_price_ << " X "
                                 << best_nonself_ask_price_ << " @ " << best_nonself_ask_size_ << " sumasksizes "
                                 << order_manager_.SumAskSizes() << " + my_position_ " << my_position_
                                 << DBGLOG_ENDL_FLUSH;
        }
      }
    }

    if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {  // elevate to error-level till satisfied
      DBGLOG_TIME_CLASS_FUNC << " position_ = " << my_position_
                             << " AverageOpenPrice = " << order_manager_.base_pnl().AverageOpenPrice() << " mkt: [ "
                             << best_nonself_bid_int_price_ << " X " << best_nonself_ask_int_price_ << " ]"
                             << " ticks_to_keep_bid_int_price_ = " << ticks_to_keep_bid_int_price_
                             << " ticks_to_keep_ask_int_price_ = " << ticks_to_keep_ask_int_price_ << DBGLOG_ENDL_FLUSH;
    }
  }
  if (is_structured_general_strategy_ && is_ready_) {
    TradeVarSetLogic(my_position_);
  }
// only need to recompute on market update if based on queue sizes
#ifdef USING_SELF_CALCULATED_QUEUE_SIZES
  exec_logic_indicators_helper_->RecomputeHysterisis();
#endif
}

void BaseTrading::SetGetFlatFokMode() {
  if (param_set_.read_get_flat_by_fok_mode_) {
    getflatfokmode_ = param_set_.get_flat_by_fok_mode_;
    return;
  }
  std::ifstream fok_mode_shc_;
  fok_mode_shc_.open(FOK_MODE_SHC_FILE, std::ifstream::in);
  if (fok_mode_shc_.is_open()) {
    std::string t_shc_ = dep_market_view_.shortcode();
    const int kshc_length_ = 128;
    char readline_buffer_[kshc_length_];

    while (fok_mode_shc_.good()) {
      bzero(readline_buffer_, kshc_length_);
      fok_mode_shc_.getline(readline_buffer_, kshc_length_);
      PerishableStringTokenizer st_(readline_buffer_, kshc_length_);
      const std::vector<const char *> &tokens_ = st_.GetTokens();

      if (tokens_.size() > 0u) {
        if (t_shc_.compare(tokens_[0]) == 0) {
          getflatfokmode_ = true;
          break;
        }
      }
    }
  }
}

void BaseTrading::GetFlatSendFok() {
  int t_position_ = my_position_;
  fok_flat_active_ = false;
  if (is_combined_getflat_ && getflat_due_to_close_ && !getflat_due_to_external_getflat_) {
    t_position_ = std::round(my_combined_flat_pos_);
  }

  int fok_book_depth_ = param_set_.flatfok_book_depth_;
  // DBGLOG_TIME_CLASS_FUNC << "position: " << t_position_ << DBGLOG_ENDL_FLUSH;
  if (t_position_ > 0) {
    if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << "position > 0, position: " << t_position_ << DBGLOG_ENDL_FLUSH;
    }
    int curr_bid_intpx_ = best_nonself_bid_int_price_;
    int fok_bsize_already_placed_ = 0;
    for (int i = 0; i < fok_book_depth_; i++) {
      int curr_bidsize_placed_ =
          p_prom_order_manager_->GetBidSizePlacedAboveEqIntPx(curr_bid_intpx_, curr_bid_intpx_ + 1);
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        DBGLOG_TIME_CLASS_FUNC << "BidSize at Px:" << curr_bid_intpx_ << " is : " << curr_bidsize_placed_
                               << DBGLOG_ENDL_FLUSH;
      }
      int fok_size_to_place_this_lvl_ = std::min(curr_bidsize_placed_, (t_position_ - fok_bsize_already_placed_));
      while (fok_size_to_place_this_lvl_ > 0) {
        int t_size_ = MathUtils::GetFlooredMultipleOf(
            std::min(fok_size_to_place_this_lvl_, param_set_.unit_trade_size_), dep_market_view_.min_order_size());
        if (t_size_ <= 0) {
          break;
        }

        order_manager_.SendTrade(curr_bid_intpx_ * dep_market_view_.min_price_increment(), curr_bid_intpx_, t_size_,
                                 kTradeTypeSell, 'A', true);  // USe getDblPx from smv
        fok_bsize_already_placed_ += t_size_;
        fok_size_to_place_this_lvl_ -= t_size_;

        if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
          DBGLOG_TIME_CLASS_FUNC << "FOKSendTrade S of " << t_size_ << " @ "
                                 << (curr_bid_intpx_ * dep_market_view_.min_price_increment())
                                 << " IntPx: " << curr_bid_intpx_ << DBGLOG_ENDL_FLUSH;
        }
      }
      curr_bid_intpx_--;
      if (fok_bsize_already_placed_ >= t_position_) {
        break;
      }
    }
    if (fok_bsize_already_placed_ > 0) {
      fok_flat_active_ = true;
    }
  } else if (t_position_ < 0) {
    if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << "position < 0, position: " << t_position_ << DBGLOG_ENDL_FLUSH;
    }
    t_position_ = abs(t_position_);
    int curr_ask_intpx_ = best_nonself_ask_int_price_;
    int fok_asize_already_placed_ = 0;
    for (int i = 0; i < fok_book_depth_; i++) {
      int curr_asksize_placed_ =
          p_prom_order_manager_->GetAskSizePlacedAboveEqIntPx(curr_ask_intpx_, curr_ask_intpx_ - 1);
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        DBGLOG_TIME_CLASS_FUNC << "AskSize at Px:" << curr_ask_intpx_ << " is : " << curr_asksize_placed_
                               << DBGLOG_ENDL_FLUSH;
      }
      int fok_size_to_place_this_lvl_ = std::min(curr_asksize_placed_, (t_position_ - fok_asize_already_placed_));
      while (fok_size_to_place_this_lvl_ > 0) {
        int t_size_ = MathUtils::GetFlooredMultipleOf(
            std::min(fok_size_to_place_this_lvl_, param_set_.unit_trade_size_), dep_market_view_.min_order_size());
        if (t_size_ <= 0) {
          break;
        }

        order_manager_.SendTrade(curr_ask_intpx_ * dep_market_view_.min_price_increment(), curr_ask_intpx_, t_size_,
                                 kTradeTypeBuy, 'A', true);
        fok_asize_already_placed_ += t_size_;
        fok_size_to_place_this_lvl_ -= t_size_;

        if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
          DBGLOG_TIME_CLASS_FUNC << "FOKSendTrade B of " << t_size_ << " @ "
                                 << (curr_ask_intpx_ * dep_market_view_.min_price_increment())
                                 << " IntPx: " << curr_ask_intpx_ << DBGLOG_ENDL_FLUSH;
        }
      }
      curr_ask_intpx_++;
      if (fok_asize_already_placed_ >= t_position_) {
        break;
      }
    }
    if (fok_asize_already_placed_ > 0) {
      fok_flat_active_ = true;
    }
  }
}

void BaseTrading::GetFlatFokTradingLogic() {
  if (getflat_due_to_close_ && GetPositionToClose() == 0 && order_manager_.SumAskSizes() == 0 &&
      order_manager_.SumBidSizes() == 0 && livetrading_) {
    if (exit_cool_off_ == 0) {
      exit_cool_off_ = watch_.msecs_from_midnight();
    } else if (!exit_bool_set_ && watch_.msecs_from_midnight() - exit_cool_off_ > 1000u) {
      class_var_counter_--;
      exit_bool_set_ = true;
    }
    if (class_var_counter_ == 0 && first_obj_) {
      DBGLOG_TIME_CLASS_FUNC << "getflat called killing tradeinit" << DBGLOG_ENDL_FLUSH;
      if (remove(pid_file_.c_str()) != 0) {
        DBGLOG_CLASS_FUNC << "Error deleting file :: " << pid_file_ << DBGLOG_ENDL_FLUSH;
      } else {
        DBGLOG_CLASS_FUNC << "Successfully deleted file :: " << pid_file_ << DBGLOG_ENDL_FLUSH;
      }
      HFSAT::BulkFileWriter _dummy_writer_;
      ReportResults(_dummy_writer_, true);
      dbglogger_.Close();
      exit(0);
    }
  }
  if (!getflatfokmode_)
  //( is_combined_getflat_ && getflat_due_to_close_ && !getflat_due_to_external_getflat_ ) )
  {
    // DBGLOG_TIME_CLASS_FUNC << "Calling the Old GetFlatLogic" << DBGLOG_ENDL_FLUSH;
    if (getflat_due_to_external_agg_getflat_ && total_agg_flat_size_ < param_set_.max_position_) {
      GetFlatAggTradingLogic();
    } else {
      GetFlatTradingLogic();
    }
    return;
  }

  int t_position_ = GetPositionToClose();

  if (!t_position_ && (order_manager_.GetFokBidOrderSize() == 0 && order_manager_.GetFokAskOrderSize() == 0)) {
    flatfok_mode_ = false;
  } else {
    if (!flatfok_mode_)  // GetFlatFokTradingLogic is called for the first time
    {
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        DBGLOG_TIME_CLASS_FUNC << "Calling the GetFlatFokLogic" << DBGLOG_ENDL_FLUSH;
      }
      flatfok_mode_ = true;
      srand(time(NULL));
    }

    if ((order_manager_.GetFokBidOrderSize() == 0 && order_manager_.GetFokAskOrderSize() == 0) &&
        watch_.msecs_from_midnight() >
            send_next_fok_time_)  // current time has exceeded the time designated for next sending of fok orders
    {
      GetFlatSendFok();

      int sleep_ms = rand() % 50 + 1;
      send_next_fok_time_ = watch_.msecs_from_midnight() + sleep_ms;
    }
  }
  if (getflat_due_to_external_agg_getflat_ && !fok_flat_active_ && total_agg_flat_size_ < param_set_.max_position_) {
    GetFlatAggTradingLogic();
  } else {
    GetFlatTradingLogic();
  }
}

void BaseTrading::GetFlatTradingLogic() {
  // only passive execution in getting out
  if (param_set_.read_agg_closeout_utc_hhmm_ == true &&
      watch_.msecs_from_midnight() > (int)(GetMsecsFromMidnightFromHHMMSS(100 * param_set_.agg_closeout_utc_hhmm_))) {
    GetFlatAggTradingLogic();
    return;
  }
  int t_position_ = GetPositionToClose();

  if (t_position_ == 0) {  // nothing to be done, cancel all remaining orders
    order_manager_.CancelAllOrders();
  } else if (t_position_ > 0) {
    // long hence cancel all bid orders
    order_manager_.CancelAllBidOrders();

    bool done_for_this_round_ = false;
    if (param_set_.allow_to_aggress_on_getflat_ &&
        // TODO: check if we want to keep this to 1 or use, param_set_.max_int_spread_to_cross_
        dep_market_view_.spread_increments() <= param_set_.max_spread_getflat_aggress_ &&
        best_nonself_bid_size_ < param_set_.max_size_to_aggress_ &&
        dep_market_view_.bestask_price() - dep_market_view_.mkt_size_weighted_price() > param_set_.getflat_aggress_) {
      // aggress after proper checks
      int agg_trade_size_required_ = MathUtils::GetFlooredMultipleOf(std::min(t_position_, param_set_.unit_trade_size_),
                                                                     dep_market_view_.min_order_size());
      int agg_size_ordered_ = order_manager_.SumAskSizeConfirmedEqAboveIntPrice(best_nonself_bid_int_price_) +
                              order_manager_.SumAskSizeUnconfirmedEqAboveIntPrice(best_nonself_bid_int_price_);

      int size_ = agg_trade_size_required_ - agg_size_ordered_;
      if (size_ > 0) {
        SendTradeAndLog(best_nonself_bid_price_, best_nonself_bid_int_price_, size_, kTradeTypeSell, 'A',
                        "GetFlatAggSendTrade");
        done_for_this_round_ = true;
      }
    }

    if (!done_for_this_round_) {
      // haven't agressed or places any orders in this round
      int eqabove_best_size_ordered_ = order_manager_.SumAskSizeConfirmedEqAboveIntPrice(best_nonself_ask_int_price_) +
                                       order_manager_.SumAskSizeUnconfirmedEqAboveIntPrice(best_nonself_ask_int_price_);

      if (getflat_mult_ord_) {
        // getflat by placing multiple orders
        int total_order_placed_ = 0;
        if (eqabove_best_size_ordered_ < t_position_) {
          while (total_order_placed_ < t_position_ - eqabove_best_size_ordered_ &&
                 total_order_placed_ + eqabove_best_size_ordered_ < max_orders_ * param_set_.unit_trade_size_) {
            int this_trade_size_ = MathUtils::GetFlooredMultipleOf(
                std::min(t_position_ - eqabove_best_size_ordered_ - total_order_placed_, param_set_.unit_trade_size_),
                dep_market_view_.min_order_size());
            order_manager_.SendTrade(best_nonself_ask_price_, best_nonself_ask_int_price_, this_trade_size_,
                                     kTradeTypeSell, 'B');
            if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
              DBGLOG_TIME_CLASS_FUNC << "GetFlatSendMultTrade S of " << this_trade_size_ << " @ "
                                     << best_nonself_ask_price_ << " IntPx: " << best_nonself_ask_int_price_
                                     << " mkt: " << best_nonself_bid_size_ << " @ " << best_nonself_bid_price_ << " X "
                                     << best_nonself_ask_price_ << " @ " << best_nonself_ask_size_ << DBGLOG_ENDL_FLUSH;
            }

            total_order_placed_ += this_trade_size_;
          }
        }
      } else {
        int trade_size_required_ = MathUtils::GetFlooredMultipleOf(std::min(t_position_, param_set_.unit_trade_size_),
                                                                   dep_market_view_.min_order_size());

        if (eqabove_best_size_ordered_ < trade_size_required_) {  // if the required size is not present then add more
          order_manager_.SendTrade(best_nonself_ask_price_, best_nonself_ask_int_price_,
                                   trade_size_required_ - eqabove_best_size_ordered_, kTradeTypeSell, 'B');

          if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
            DBGLOG_TIME_CLASS_FUNC << "GetFlatSendTrade S of " << trade_size_required_ - eqabove_best_size_ordered_
                                   << " @ " << best_nonself_ask_price_ << " IntPx: " << best_nonself_ask_int_price_
                                   << " mkt: " << best_nonself_bid_size_ << " @ " << best_nonself_bid_price_ << " X "
                                   << best_nonself_ask_price_ << " @ " << best_nonself_ask_size_ << DBGLOG_ENDL_FLUSH;
          }
        }
        // not doing anything if the placed size is more than required // well u can send a replace
      }
    }

    // instead of cancelling all non-best orders, cancel just enough to avoid overfill
    int t_total_asks_placed_ = order_manager_.SumAskSizes();
    if (!getflat_due_to_max_position_ && t_total_asks_placed_ > t_position_) {
      order_manager_.KeepAskSizeInPriceRange(t_position_);
    }
    if (getflat_due_to_max_position_ && (t_position_ - t_total_asks_placed_ < param_set_.max_position_)) {
      order_manager_.KeepAskSizeInPriceRange(t_position_ - param_set_.max_position_);
    }
  } else {  // my_position_ < 0
    // short hence cancel all sell orders
    order_manager_.CancelAllAskOrders();

    bool done_for_this_round_ = false;
    if (param_set_.allow_to_aggress_on_getflat_ &&
        dep_market_view_.spread_increments() <=
            param_set_.max_spread_getflat_aggress_ &&  // TODO: check if we want to keep this to 1 or use
        // param_set_.max_int_spread_to_cross_
        best_nonself_ask_size_ < param_set_.max_size_to_aggress_ &&
        dep_market_view_.mkt_size_weighted_price() - dep_market_view_.bestbid_price() > param_set_.getflat_aggress_) {
      // aggress after proper checks
      int agg_trade_size_required_ = MathUtils::GetFlooredMultipleOf(
          std::min(-t_position_, param_set_.unit_trade_size_), dep_market_view_.min_order_size());
      int agg_size_ordered_ = order_manager_.SumBidSizeConfirmedEqAboveIntPrice(best_nonself_ask_int_price_) +
                              order_manager_.SumBidSizeUnconfirmedEqAboveIntPrice(best_nonself_ask_int_price_);

      int size_ = agg_trade_size_required_ - agg_size_ordered_;
      if (size_ > 0) {
        SendTradeAndLog(best_nonself_ask_price_, best_nonself_ask_int_price_, size_, kTradeTypeBuy, 'A',
                        "GetFlatAggSendTrade");
        done_for_this_round_ = true;
      }
    }

    if (!done_for_this_round_) {
      // haven't agressed or places any orders in this round
      int eqabove_best_size_ordered_ = order_manager_.SumBidSizeConfirmedEqAboveIntPrice(best_nonself_bid_int_price_) +
                                       order_manager_.SumBidSizeUnconfirmedEqAboveIntPrice(best_nonself_bid_int_price_);

      if (getflat_mult_ord_) {
        int total_order_placed_ = 0;
        if (eqabove_best_size_ordered_ < -t_position_) {
          while (total_order_placed_ < -t_position_ - eqabove_best_size_ordered_ &&
                 total_order_placed_ + eqabove_best_size_ordered_ < max_orders_ * param_set_.unit_trade_size_) {
            int this_trade_size_ = MathUtils::GetFlooredMultipleOf(
                std::min(-t_position_ - eqabove_best_size_ordered_ - total_order_placed_, param_set_.unit_trade_size_),
                dep_market_view_.min_order_size());
            order_manager_.SendTrade(best_nonself_bid_price_, best_nonself_bid_int_price_, this_trade_size_,
                                     kTradeTypeBuy, 'B');
            if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
              DBGLOG_TIME_CLASS_FUNC << "GetFlatSendMultTrade B of " << this_trade_size_ << " @ "
                                     << best_nonself_bid_price_ << " IntPx: " << best_nonself_bid_int_price_
                                     << " mkt: " << best_nonself_bid_size_ << " @ " << best_nonself_bid_price_ << " X "
                                     << best_nonself_ask_price_ << " @ " << best_nonself_ask_size_ << DBGLOG_ENDL_FLUSH;
            }

            total_order_placed_ += this_trade_size_;
          }
        }
      } else {
        int trade_size_required_ = MathUtils::GetFlooredMultipleOf(std::min(-t_position_, param_set_.unit_trade_size_),
                                                                   dep_market_view_.min_order_size());

        if (eqabove_best_size_ordered_ < trade_size_required_) {  // if the required size is not present then add more
          order_manager_.SendTrade(best_nonself_bid_price_, best_nonself_bid_int_price_,
                                   trade_size_required_ - eqabove_best_size_ordered_, kTradeTypeBuy, 'B');

          if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
            DBGLOG_TIME_CLASS_FUNC << "GetFlatSendTrade B of " << trade_size_required_ - eqabove_best_size_ordered_
                                   << " @ " << best_nonself_bid_price_ << " IntPx: " << best_nonself_bid_int_price_
                                   << " mkt: " << best_nonself_bid_size_ << " @ " << best_nonself_bid_price_ << " X "
                                   << best_nonself_ask_price_ << " @ " << best_nonself_ask_size_ << DBGLOG_ENDL_FLUSH;
          }
        }
        // not doing anything if the placed size is more than required
      }
      // instead of cancelling all non-best orders cancel, just enough to avoid overfill
      int t_total_bids_placed_ = order_manager_.SumBidSizes();
      if (!getflat_due_to_max_position_ && t_total_bids_placed_ > -t_position_) {
        order_manager_.KeepBidSizeInPriceRange(-t_position_);
      }
      if (getflat_due_to_max_position_ && (-t_position_ - t_total_bids_placed_ < param_set_.max_position_)) {
        order_manager_.KeepBidSizeInPriceRange(-t_position_ - param_set_.max_position_);
      }
    }
  }
}

void BaseTrading::GetFlatAggTradingLogic() {
  int t_position_ = GetPositionToClose();
  if (t_position_ == 0) {
    order_manager_.CancelAllOrders();
  } else if (t_position_ > 0) {
    order_manager_.CancelAllBidOrders();
    order_manager_.CancelAsksBelowIntPrice(best_nonself_bid_int_price_);
    int t_size_ordered_ = order_manager_.SumAskSizeConfirmedEqAboveIntPrice(best_nonself_bid_int_price_) +
                          order_manager_.SumAskSizeUnconfirmedEqAboveIntPrice(best_nonself_bid_int_price_);
    int trade_size_required_ = MathUtils::GetFlooredMultipleOf(std::min(t_position_, param_set_.unit_trade_size_),
                                                               dep_market_view_.min_order_size());
    if (t_size_ordered_ < trade_size_required_) {
      order_manager_.SendTrade(best_nonself_bid_price_, best_nonself_bid_int_price_,
                               trade_size_required_ - t_size_ordered_, kTradeTypeSell, 'A');
      total_agg_flat_size_ += (trade_size_required_ - t_size_ordered_);
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        DBGLOG_TIME_CLASS_FUNC << "GetFlatSendTrade S of " << trade_size_required_ - t_size_ordered_ << " @ "
                               << best_nonself_bid_price_ << " IntPx: " << best_nonself_bid_int_price_
                               << " mkt: " << best_nonself_bid_size_ << " @ " << best_nonself_bid_price_ << " X "
                               << best_nonself_ask_price_ << " @ " << best_nonself_ask_size_ << DBGLOG_ENDL_FLUSH;
      }
    }
  } else if (t_position_ < 0) {
    order_manager_.CancelAllAskOrders();                                  // short hence cancel all sell orders
    order_manager_.CancelBidsBelowIntPrice(best_nonself_ask_int_price_);  // cancel all non bestlevel bid orders
    // ideal size to place ask for is what the position is, except limited to unit_trade_size_ ( for sanity and market
    // effect reasons )
    int t_size_ordered_ = order_manager_.SumBidSizeConfirmedEqAboveIntPrice(best_nonself_ask_int_price_) +
                          order_manager_.SumBidSizeUnconfirmedEqAboveIntPrice(best_nonself_ask_int_price_);
    int trade_size_required_ = MathUtils::GetFlooredMultipleOf(std::min(-t_position_, param_set_.unit_trade_size_),
                                                               dep_market_view_.min_order_size());
    if (t_size_ordered_ < trade_size_required_) {  // if the required size is not present then add more
      order_manager_.SendTrade(best_nonself_ask_price_, best_nonself_ask_int_price_,
                               trade_size_required_ - t_size_ordered_, kTradeTypeBuy, 'A');

      total_agg_flat_size_ += (trade_size_required_ - t_size_ordered_);

      if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        DBGLOG_TIME_CLASS_FUNC << "GetFlatSendTrade B of " << trade_size_required_ - t_size_ordered_ << " @ "
                               << best_nonself_bid_price_ << " IntPx: " << best_nonself_bid_int_price_
                               << " mkt: " << best_nonself_bid_size_ << " @ " << best_nonself_bid_price_ << " X "
                               << best_nonself_ask_price_ << " @ " << best_nonself_ask_size_ << DBGLOG_ENDL_FLUSH;
      }
    }
  }
}
void BaseTrading::GetProductListToGetFlatMultOrder() {
  std::ifstream product_list_file_;
  product_list_file_.open(std::string(MULT_ORDER_GET_FLAT_PROD_FILE).c_str(), std::ifstream::in);

  if (product_list_file_.is_open()) {
    int kProductLineLength = 1024;
    char line[kProductLineLength];
    bzero(line, kProductLineLength);
    while (product_list_file_.good()) {
      bzero(line, kProductLineLength);
      product_list_file_.getline(line, kProductLineLength);
      PerishableStringTokenizer st_(line, kProductLineLength);
      const std::vector<const char *> &tokens_ = st_.GetTokens();
      if (tokens_.size() < 2) {
        continue;
      }
      std::string this_prod_ = std::string(tokens_[0]);
      if (this_prod_.compare(dep_market_view_.shortcode()) == 0) {
        getflat_mult_ord_ = true;
        max_orders_ = atoi(tokens_[1]);
      }
    }
    product_list_file_.close();
  } else {
    DBGLOG_CLASS_FUNC << "can't find file: " << std::string(MULT_ORDER_GET_FLAT_PROD_FILE) << " for reading"
                      << DBGLOG_ENDL_FLUSH;
  }
}

void BaseTrading::OnNewMidNight() {
  DBGLOG_TIME_CLASS << "BaseTrading::Got refreshecoevents user_msg " << DBGLOG_ENDL_FLUSH;
  economic_events_manager_.ReloadDB();
  economic_events_manager_.AllowEconomicEventsFromList(dep_market_view_.shortcode());
  allowed_events_present_ = economic_events_manager_.AllowedEventsPresent();
  getflat_due_to_allowed_economic_event_ = false;
  last_allowed_event_index_ = 0;
  ProcessAllowedEco();
  economic_events_manager_.AdjustSeverity(dep_market_view_.shortcode(), dep_market_view_.exch_source());
}

inline void BaseTrading::CallPlaceCancelNonBestLevels() {
  // 0 implies no supp orders ever
  if (param_set_.worst_case_position_ == 0) {
    order_manager_.CancelBidsEqBelowIntPrice(best_nonself_bid_int_price_ - param_set_.px_band_);
    order_manager_.CancelAsksEqBelowIntPrice(best_nonself_ask_int_price_ + param_set_.px_band_);
  } else {
#define NON_BEST_LEVEL_ORDER_MANAGEMENT_INTERVAL_MSECS 1000  ///< every 1000 msecs look at supporting orders
    if ((last_non_best_level_om_msecs_ == 0) || (watch_.msecs_from_midnight() - last_non_best_level_om_msecs_ >
                                                 NON_BEST_LEVEL_ORDER_MANAGEMENT_INTERVAL_MSECS)) {
      last_non_best_level_om_msecs_ = watch_.msecs_from_midnight();
      // non best level order management
      PlaceCancelNonBestLevels();

      if (livetrading_ || (dbglogger_.CheckLoggingLevel(TRADING_INFO))) {
        LogFullStatus();
      }
    } else if (should_check_worst_pos_after_placecxl_) {
      if (placed_bids_this_round_) {
        CancelNonBestBids();
      } else if (canceled_bids_this_round_) {
        PlaceNonBestBids();
      } else if (placed_asks_this_round_) {
        CancelNonBestAsks();
      } else if (canceled_asks_this_round_) {
        PlaceNonBestAsks();
      }

      if (placed_bids_this_round_ || canceled_bids_this_round_ || placed_asks_this_round_ ||
          canceled_asks_this_round_) {
        if (livetrading_ || (dbglogger_.CheckLoggingLevel(TRADING_INFO))) LogFullStatus();
      }
    }
#undef NON_BEST_LEVEL_ORDER_MANAGEMENT_INTERVAL_MSECS
  }

  if (param_set_.read_explicit_worst_case_long_position_ && (param_set_.explicit_worst_case_long_position_ == 0)) {
    order_manager_.CancelBidsEqBelowIntPrice(best_nonself_bid_int_price_ - param_set_.px_band_);
  }
  if (param_set_.read_explicit_worst_case_short_position_ && (param_set_.explicit_worst_case_short_position_ == 0)) {
    order_manager_.CancelAsksEqBelowIntPrice(best_nonself_ask_int_price_ + param_set_.px_band_);
  }
}

bool BaseTrading::CanPlaceNextOrder(int _int_price_, TradeType_t _trade_type_) {
  if (_trade_type_ == kTradeTypeBuy) {
    BaseOrder *this_order_ = order_manager_.GetBottomBidOrderAtIntPx(_int_price_);
    if (this_order_ == NULL) {
      return true;
    } else if (this_order_->queue_size_behind_ > param_set_.size_diff_between_orders_) {
      return true;
    }
  } else if (_trade_type_ == kTradeTypeSell) {
    BaseOrder *this_order_ = order_manager_.GetBottomAskOrderAtIntPx(_int_price_);
    if (this_order_ == NULL) {
      return true;
    } else if (this_order_->queue_size_behind_ > param_set_.size_diff_between_orders_) {
      return true;
    }
  }
  return false;
}

void BaseTrading::ComputeTradeBias() {
  // buy side
  if (watch_.msecs_from_midnight() - last_buy_msecs_ < TRADE_EFFECT_TIME &&
      last_buy_int_price_ == best_nonself_bid_int_price_) {
    // last buy is recent and it was at same price as current price
    BaseOrder *bid_order_at_this_price_ = order_manager_.GetBottomBidOrderAtIntPx(best_nonself_bid_int_price_);
    if (bid_order_at_this_price_ != NULL) {
      if (dep_market_view_.bestbid_size() > 1.5 * param_set_.size_diff_between_orders_) {
        l1_bid_trade_bias_ =
            (double(bid_order_at_this_price_->queue_size_behind_) /
             double(bid_order_at_this_price_->queue_size_ahead_ + bid_order_at_this_price_->queue_size_behind_)) *
            dep_market_view_.min_price_increment();
      } else {
        l1_bid_trade_bias_ = 0.0;
      }
    } else {
      l1_bid_trade_bias_ = 0.0;
    }
  } else {
    l1_bid_trade_bias_ = 0.0;
  }

  // sell side
  if (watch_.msecs_from_midnight() - last_sell_msecs_ < TRADE_EFFECT_TIME &&
      last_sell_int_price_ == best_nonself_ask_int_price_) {
    BaseOrder *ask_order_at_this_price_ = order_manager_.GetBottomAskOrderAtIntPx(best_nonself_ask_int_price_);
    if (ask_order_at_this_price_ != NULL) {
      if (dep_market_view_.bestask_size() > 1.5 * param_set_.size_diff_between_orders_) {
        l1_ask_trade_bias_ =
            (double(ask_order_at_this_price_->queue_size_behind_) /
             double(ask_order_at_this_price_->queue_size_behind_ + ask_order_at_this_price_->queue_size_ahead_)) *
            dep_market_view_.min_price_increment();
      } else {
        l1_ask_trade_bias_ = 0.0;
      }
    } else {
      l1_ask_trade_bias_ = 0.0;
    }
  } else {
    l1_ask_trade_bias_ = 0.0;
  }
}

void BaseTrading::ComputeCancellationModelBias(double predicted_class_prob_, int bid_or_ask) {
  double factor;
  if (!param_set_.read_canc_prob_) {
    factor = 0;
  } else {
    factor = param_set_.canc_prob_;
  }
  if (bid_or_ask == 1) {
    cancellation_model_bid_bias_ = predicted_class_prob_ > param_set_.cancel_prob_thresh_
                                       ? (1 + factor * (predicted_class_prob_ - param_set_.cancel_prob_thresh_))
                                       : 1;
  } else if (bid_or_ask == 2) {
    cancellation_model_ask_bias_ = predicted_class_prob_ > param_set_.cancel_prob_thresh_
                                       ? (1 + factor * (predicted_class_prob_ - param_set_.cancel_prob_thresh_))
                                       : 1;
  }
}
void BaseTrading::SendMail(const std::string &mail_content, const std::string &mail_subject) {
  HFSAT::Email email_;
  email_.setSubject(mail_subject);
  email_.addRecepient("nseall@tworoads.co.in");
  email_.addSender("nseall@tworoads.co.in");
  email_.content_stream << mail_content << "<br/>";
  email_.sendMail();
}

void BaseTrading::LogFullStatus() {
  if (zero_logging_mode_ && !override_zero_logging_mode_once_for_external_cmd_) {
    return;
  }

#define FULL_LOGGING_INTERVAL_MSECS 5000

  if (override_zero_logging_mode_once_for_external_cmd_) {
    override_zero_logging_mode_once_for_external_cmd_ = false;
  }

  if ((last_full_logging_msecs_ == 0) ||
      (watch_.msecs_from_midnight() - last_full_logging_msecs_ > FULL_LOGGING_INTERVAL_MSECS)) {
    last_full_logging_msecs_ = watch_.msecs_from_midnight();
    if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
      PrintFullStatus();

      DBGLOG_TIME_CLASS_FUNC << "Orders:" << DBGLOG_ENDL_FLUSH;
      order_manager_.LogFullStatus();
      dump_inds = true;
    }
  }

#undef FULL_LOGGING_INTERVAL_MSECS
}

void BaseTrading::PlaceCancelNonBestLevels() {
  if (getflat_due_to_regime_indicator_) {
    return;
  }
  // compute them less frequently
  if (exec_logic_indicators_helper_->place_nonbest()) {
    int t_position_ = my_position_;
    if (is_pair_strategy_) {
      t_position_ = my_combined_position_;
    } else if (is_structured_strategy_) {
      t_position_ = my_risk_;
    }

    const int t_worst_case_long_position_ =
        (param_set_.read_explicit_max_long_position_ ? param_set_.explicit_worst_case_long_position_
                                                     : param_set_.worst_case_position_);
    const int t_worst_case_short_position_ =
        (param_set_.read_explicit_max_short_position_ ? param_set_.explicit_worst_case_short_position_
                                                      : param_set_.worst_case_position_);

    if (param_set_.read_max_bid_ask_order_diff_ && param_set_.read_min_size_to_quote_) {
      int max_bid_ask_diff_ = param_set_.max_bid_ask_order_diff_;

      if (param_set_.read_maturity_ && best_nonself_bid_int_price_ > (20.00 / dep_market_view_.min_price_increment())) {
        if (param_set_.maturity_ < 4) {
          // starting from 0
          max_bid_ask_diff_ = (int)(0.02 * best_nonself_bid_int_price_);
        } else if (param_set_.maturity_ >= 4) {
          max_bid_ask_diff_ = (int)(0.03 * best_nonself_ask_int_price_);
        }
      }

      int max_bid_distance_ =
          (int)(0.5 *
                std::max(0, (max_bid_ask_diff_ - (int)(best_nonself_ask_int_price_ - best_nonself_bid_int_price_))));
      int max_ask_distance_ =
          std::max(0, (max_bid_ask_diff_ - (int)(best_nonself_ask_int_price_ - best_nonself_bid_int_price_)) -
                          max_bid_distance_);

      int min_bid_distance_ =
          std::max(0.0, (max_bid_distance_ - (ceil(param_set_.min_size_to_quote_ / param_set_.unit_trade_size_)) + 1));
      int min_ask_distance_ =
          std::max(0.0, (max_ask_distance_ - (ceil(param_set_.min_size_to_quote_ / param_set_.unit_trade_size_)) + 1));

      if (param_set_.read_stdev_quote_factor_ && param_set_.read_min_quote_distance_from_best_) {
        DBGLOG_CLASS_FUNC << " StdevFactor: "
                          << (int)(stdev_ * param_set_.stdev_quote_factor_ / dep_market_view_.min_price_increment())
                          << "stdev: " << stdev_ << DBGLOG_ENDL_FLUSH;

        // Applying min-quote distance only when using stdev
        min_bid_distance_ = std::max((int)param_set_.min_quote_distance_from_best_,
                                     std::min(max_bid_distance_, (int)(stdev_ * param_set_.stdev_quote_factor_ /
                                                                       dep_market_view_.min_price_increment())));
        min_ask_distance_ = std::max((int)param_set_.min_quote_distance_from_best_,
                                     std::min(max_ask_distance_, (int)(stdev_ * param_set_.stdev_quote_factor_ /
                                                                       dep_market_view_.min_price_increment())));
      }

      {
        order_manager_.CancelBidsEqBelowIntPrice(best_nonself_bid_int_price_ - max_bid_distance_ - 1);
        order_manager_.CancelBidsEqAboveIntPrice(best_nonself_bid_int_price_ - min_bid_distance_ + 1);
        order_manager_.CancelAsksEqBelowIntPrice(best_nonself_ask_int_price_ + max_ask_distance_ + 1);
        order_manager_.CancelAsksEqAboveIntPrice(best_nonself_ask_int_price_ + min_ask_distance_ - 1);
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
          DBGLOG_TIME_CLASS_FUNC << "Cancelling bid orders Eq and Below Int Price "
                                 << (best_nonself_bid_int_price_ - max_bid_distance_ - 1) << DBGLOG_ENDL_FLUSH;
          DBGLOG_TIME_CLASS_FUNC << "Cancelling bid orders Eq and Above Int Price "
                                 << (best_nonself_bid_int_price_ - min_bid_distance_ + 1) << DBGLOG_ENDL_FLUSH;
          DBGLOG_TIME_CLASS_FUNC << "Cancelling ask orders Eq and Above Int Price "
                                 << (best_nonself_ask_int_price_ + max_ask_distance_ + 1) << DBGLOG_ENDL_FLUSH;
          DBGLOG_TIME_CLASS_FUNC << "Cancelling ask orders Eq and Below Int Price "
                                 << (best_nonself_ask_int_price_ + min_ask_distance_ - 1) << DBGLOG_ENDL_FLUSH;
        }
      }

      for (int _t_bid_int_price_ = (int)best_nonself_bid_int_price_ - max_bid_distance_;
           _t_bid_int_price_ <= (int)best_nonself_bid_int_price_ - min_bid_distance_; _t_bid_int_price_++) {
        if ((int)(t_position_ + order_manager_.SumBidSizes() + (2 * param_set_.unit_trade_size_) <
                  t_worst_case_long_position_)) {
          if (order_manager_.GetTotalBidSizeOrderedAtIntPx(_t_bid_int_price_) == 0) {
            unsigned int size_to_be_placed_ =
                std::min(position_tradevarset_map_[P2TV_zero_idx_].l1bid_trade_size_,
                         std::max(0, (int)param_set_.min_size_to_quote_ - order_manager_.SumBidSizes()));
            if (size_to_be_placed_ == 0) {
              break;
            }
            order_manager_.SendTradeIntPx(_t_bid_int_price_, size_to_be_placed_, kTradeTypeBuy, 'S');
            if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
              DBGLOG_TIME_CLASS_FUNC << "SupportingSendTrade B of " << size_to_be_placed_ << " @ "
                                     << (dep_market_view_.min_price_increment() * _t_bid_int_price_)
                                     << " IntPx: " << _t_bid_int_price_
                                     << " level: " << best_nonself_bid_int_price_ - _t_bid_int_price_
                                     << " mkt: " << best_nonself_bid_size_ << " @ " << best_nonself_bid_price_ << " X "
                                     << best_nonself_ask_price_ << " @ " << best_nonself_ask_size_ << " sumbidsizes "
                                     << order_manager_.SumBidSizes() << " + my_position_ " << t_position_
                                     << DBGLOG_ENDL_FLUSH;
            }
          }
        }
      }

      for (int _t_ask_int_price_ = (int)best_nonself_ask_int_price_ + max_ask_distance_;
           _t_ask_int_price_ >= (int)best_nonself_ask_int_price_ + min_ask_distance_; _t_ask_int_price_--) {
        if ((int)(t_position_ + order_manager_.SumAskSizes() + (2 * param_set_.unit_trade_size_) <
                  t_worst_case_short_position_)) {
          if (order_manager_.GetTotalAskSizeOrderedAtIntPx(_t_ask_int_price_) == 0) {
            unsigned int size_to_be_placed_ =
                std::min(position_tradevarset_map_[P2TV_zero_idx_].l1ask_trade_size_,
                         std::max(0, (int)param_set_.min_size_to_quote_ - order_manager_.SumAskSizes()));
            if (size_to_be_placed_ == 0) {
              break;
            }
            order_manager_.SendTradeIntPx(_t_ask_int_price_, size_to_be_placed_, kTradeTypeSell, 'S');
            if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
              DBGLOG_TIME_CLASS_FUNC << "SupportingSendTrade S of " << size_to_be_placed_ << " @ "
                                     << (dep_market_view_.min_price_increment() * _t_ask_int_price_)
                                     << " IntPx: " << _t_ask_int_price_
                                     << " level: " << _t_ask_int_price_ - best_nonself_ask_int_price_
                                     << " mkt: " << best_nonself_bid_size_ << " @ " << best_nonself_bid_price_ << " X "
                                     << best_nonself_ask_price_ << " @ " << best_nonself_ask_size_ << " sumasksizes "
                                     << order_manager_.SumAskSizes() << " + my_position_ " << t_position_
                                     << DBGLOG_ENDL_FLUSH;
            }
          }
        }
      }
      return;
    }

    // placing supporting orders to num_non_best_bid_levels_monitored_
    if (my_position_ <= (param_set_.max_position_ / 2) ||
        param_set_.ignore_max_pos_check_for_non_best_) {  // position is not very long
      unsigned int _start_index_ = 1;

      _start_index_ = std::max(_start_index_, param_set_.min_distance_for_non_best_);

      if (param_set_.min_size_ahead_for_non_best_ > 0) {
        unsigned int _size_ahead_ = 0;
        auto i = 0u;
        for (; _size_ahead_ <= param_set_.min_size_ahead_for_non_best_ && i <= param_set_.max_distance_for_non_best_;
             i++) {
          _size_ahead_ += dep_market_view_.bid_size(i);
        }
        _start_index_ = std::max(_start_index_, i);
      }

      for (unsigned int _level_index_ = _start_index_;
           _level_index_ < (_start_index_ + param_set_.num_non_best_bid_levels_monitored_ - 1);
           _level_index_++) {  // check levels from 2 to param_set_.num_non_best_bid_levels_monitored_

        if ((int)(t_position_ + order_manager_.SumBidSizes() + (2 * param_set_.unit_trade_size_)) <
            t_worst_case_long_position_) {  // placing orders still keeps the total active orders within
          // worst_case_position_ then place orders at this level

          int _this_bid_int_price_ = (best_nonself_bid_int_price_ - _level_index_);
          if (order_manager_.GetTotalBidSizeOrderedAtIntPx(_this_bid_int_price_) == 0) {  // if no orders at this level
            order_manager_.SendTradeIntPx(
                _this_bid_int_price_, position_tradevarset_map_[P2TV_zero_idx_].l1bid_trade_size_, kTradeTypeBuy, 'S');
            if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
              DBGLOG_TIME_CLASS_FUNC << "SupportingSendTrade B of "
                                     << position_tradevarset_map_[P2TV_zero_idx_].l1bid_trade_size_ << " @ "
                                     << (dep_market_view_.min_price_increment() * _this_bid_int_price_)
                                     << " IntPx: " << _this_bid_int_price_ << " level: " << _level_index_
                                     << " mkt: " << best_nonself_bid_size_ << " @ " << best_nonself_bid_price_ << " X "
                                     << best_nonself_ask_price_ << " @ " << best_nonself_ask_size_ << " sumbidsizes "
                                     << order_manager_.SumBidSizes() << " + my_position_ " << t_position_
                                     << DBGLOG_ENDL_FLUSH;
            }
          }
        }
      }
    }

    if (my_position_ >= (-param_set_.max_position_ / 2) ||
        param_set_.ignore_max_pos_check_for_non_best_) {  // position is not too short

      unsigned int _start_index_ = 1;
      _start_index_ = std::max(_start_index_, param_set_.min_distance_for_non_best_);
      if (param_set_.min_size_ahead_for_non_best_ > 0) {
        unsigned int _size_ahead_ = 0;
        auto i = 0u;
        for (; _size_ahead_ <= param_set_.min_size_ahead_for_non_best_ && i <= param_set_.max_distance_for_non_best_;
             i++) {
          _size_ahead_ += dep_market_view_.bid_size(i);
        }
        _start_index_ = std::max(_start_index_, i);
      }

      for (unsigned int _level_index_ = _start_index_;
           _level_index_ < (_start_index_ + param_set_.num_non_best_ask_levels_monitored_ - 1);
           _level_index_++) {  // check levels from 2 to param_set_.num_non_best_ask_levels_monitored_

        if ((int)(-t_position_ + order_manager_.SumAskSizes() + (2 * param_set_.unit_trade_size_)) <
            t_worst_case_short_position_) {  // placing orders still keeps the total active orders within
          // worst_case_position_ then place orders at this level

          int _this_ask_int_price_ = best_nonself_ask_int_price_ + _level_index_;
          if (order_manager_.GetTotalAskSizeOrderedAtIntPx(_this_ask_int_price_) == 0) {  // if no orders at this level
            order_manager_.SendTradeIntPx(
                _this_ask_int_price_, position_tradevarset_map_[P2TV_zero_idx_].l1ask_trade_size_, kTradeTypeSell, 'S');
            if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
              DBGLOG_TIME_CLASS_FUNC << "SupportingSendTrade S of "
                                     << position_tradevarset_map_[P2TV_zero_idx_].l1ask_trade_size_ << " @ "
                                     << (dep_market_view_.min_price_increment() * _this_ask_int_price_)
                                     << " IntPx: " << _this_ask_int_price_ << " level: " << _level_index_
                                     << " mkt: " << best_nonself_bid_size_ << " @ " << best_nonself_bid_price_ << " X "
                                     << best_nonself_ask_price_ << " @ " << best_nonself_ask_size_ << " sumasksizes "
                                     << order_manager_.SumAskSizes() << " + -my_position_ " << -t_position_
                                     << DBGLOG_ENDL_FLUSH;
            }
          }
        }
      }
    }

    // check if it is better to cancel orders now rather than on-demand

    // cancel orders that get us to worst_case_position_ or more
    {
      // setting up aliases
      std::vector<int> &sum_bid_confirmed_ = order_manager_.SumBidConfirmed();
      int confirmed_top_bid_index_ = order_manager_.GetConfirmedTopBidIndex();
      int confirmed_bottom_bid_index_ = order_manager_.GetConfirmedBottomBidIndex();
      int confirmed_bid_index_ = confirmed_top_bid_index_;

      std::vector<int> &sum_bid_unconfirmed_ = order_manager_.SumBidUnconfirmed();
      int unconfirmed_top_bid_index_ = order_manager_.GetUnconfirmedTopBidIndex();
      int unconfirmed_bottom_bid_index_ = order_manager_.GetUnconfirmedBottomBidIndex();
      int unconfirmed_bid_index_ = unconfirmed_top_bid_index_;

      // std::vector < std::vector < BaseOrder * > > & bid_order_vec_ = order_manager_.BidOrderVec ( );
      int order_vec_top_bid_index_ = order_manager_.GetOrderVecTopBidIndex();
      int order_vec_bottom_bid_index_ = order_manager_.GetOrderVecBottomBidIndex();

      unsigned int _size_seen_so_far_ = 0;
      int _max_size_resting_bids_ = t_worst_case_long_position_ - std::max(0, t_position_);
      if (order_vec_top_bid_index_ != -1) {
        for (int order_vec_index_ = order_vec_top_bid_index_; order_vec_index_ >= order_vec_bottom_bid_index_;
             order_vec_index_--) {
          // the following code makes this not look at top level orders
          // hence top level orders are unaffected by worst_case_position_
          if (order_manager_.GetBidIntPrice(order_vec_index_) >= best_nonself_bid_int_price_) continue;

          // _size_seen_so_far_ adds up all confirmed and unconfirmed sizes of bids at levels at or above ( closer to
          // midprice ) than this level
          // where this level refers to the int_price_ at order_vec_index_

          while (unconfirmed_bottom_bid_index_ != -1 && unconfirmed_bid_index_ >= unconfirmed_bottom_bid_index_ &&
                 order_manager_.GetBidIntPrice(unconfirmed_bid_index_) >=
                     order_manager_.GetBidIntPrice(order_vec_index_))  // TODO: replace with index_ comparison
          {
            _size_seen_so_far_ += sum_bid_unconfirmed_[unconfirmed_bid_index_];
            unconfirmed_bid_index_--;
          }

          while (confirmed_bottom_bid_index_ != -1 && confirmed_bid_index_ >= confirmed_bottom_bid_index_ &&
                 order_manager_.GetBidIntPrice(confirmed_bid_index_) >=
                     order_manager_.GetBidIntPrice(order_vec_index_))  // TODO: replace with index_ comparison
          {
            _size_seen_so_far_ += sum_bid_confirmed_[confirmed_bid_index_];
            confirmed_bid_index_--;
          }

          // if simply all orders in calculating the _size_seen_so_far_ get executed in a sweep then our position will
          // reach
          // _size_seen_so_far_ + my_position_
          // and if this number equals or exceeds ( param_set_.worst_case_position_ )
          // then cancel all orders we can at this level or below
          // to avoid confusion perhaps we should avoid placing multiple orders atthe same level
          // since otherwise this could cause placing and canceling
          if ((int)_size_seen_so_far_ > _max_size_resting_bids_) {
            order_manager_.CancelBidsEqBelowIntPrice(order_manager_.GetBidIntPrice(order_vec_index_));

            if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
              DBGLOG_TIME_CLASS_FUNC << "CancelBidsEqBelowIntPrice " << order_manager_.GetBidIntPrice(order_vec_index_)
                                     << " since _size_seen_so_far_ = " << _size_seen_so_far_
                                     << " _max_size_resting_bids_ = " << _max_size_resting_bids_ << DBGLOG_ENDL_FLUSH;
            }
            break;
          }
        }
      }
    }

    {
      // setting up aliases
      std::vector<int> &sum_ask_confirmed_ = order_manager_.SumAskConfirmed();
      int confirmed_top_ask_index_ = order_manager_.GetConfirmedTopAskIndex();
      int confirmed_bottom_ask_index_ = order_manager_.GetConfirmedBottomAskIndex();
      int confirmed_ask_index_ = confirmed_top_ask_index_;

      std::vector<int> &sum_ask_unconfirmed_ = order_manager_.SumAskUnconfirmed();
      int unconfirmed_top_ask_index_ = order_manager_.GetUnconfirmedTopAskIndex();
      int unconfirmed_bottom_ask_index_ = order_manager_.GetUnconfirmedBottomAskIndex();
      int unconfirmed_ask_index_ = unconfirmed_top_ask_index_;

      // std::vector < std::vector < BaseOrder * > > & ask_order_vec_ = order_manager_.AskOrderVec ( );
      int order_vec_top_ask_index_ = order_manager_.GetOrderVecTopAskIndex();
      int order_vec_bottom_ask_index_ = order_manager_.GetOrderVecBottomAskIndex();

      int _size_seen_so_far_ = 0;
      int _max_size_resting_asks_ = t_worst_case_short_position_ + std::min(0, t_position_);
      if (order_vec_top_ask_index_ != -1) {
        for (int order_vec_index_ = order_vec_top_ask_index_; order_vec_index_ >= order_vec_bottom_ask_index_;
             order_vec_index_--) {
          // the following code makes this not look at top level orders
          // hence top level orders are unaffected by worst_case_position_
          if (order_manager_.GetAskIntPrice(order_vec_index_) <= best_nonself_ask_int_price_) continue;

          // _size_seen_so_far_ adds up all confirmed and unconfirmed sizes of asks at levels at or above ( closer to
          // midprice ) than this level
          // where this level refers to the key of _intpx_2_ask_order_vec_iter_

          while (unconfirmed_top_ask_index_ != -1 && unconfirmed_ask_index_ >= unconfirmed_bottom_ask_index_ &&
                 order_manager_.GetAskIntPrice(unconfirmed_ask_index_) <=
                     order_manager_.GetAskIntPrice(order_vec_index_)) {
            _size_seen_so_far_ += sum_ask_unconfirmed_[unconfirmed_ask_index_];
            unconfirmed_ask_index_--;
          }

          while (confirmed_top_ask_index_ != -1 && confirmed_ask_index_ >= confirmed_bottom_ask_index_ &&
                 order_manager_.GetAskIntPrice(confirmed_ask_index_) <=
                     order_manager_.GetAskIntPrice(order_vec_index_)) {
            _size_seen_so_far_ += sum_ask_confirmed_[confirmed_ask_index_];
            confirmed_ask_index_--;
          }

          // if simply all orders in calculating the _size_seen_so_far_ get executed in a sweep then our position will
          // reach
          // _size_seen_so_far_ + my_position_
          // and if this number equals or exceeds ( param_set_.worst_case_position_ )
          // then cancel all orders we can at this level or below
          // to avoid confusion perhaps we should avoid placing multiple orders atthe same level
          // since otherwise this could cause placing and canceling
          if ((int)_size_seen_so_far_ > _max_size_resting_asks_) {
            order_manager_.CancelAsksEqBelowIntPrice(order_manager_.GetAskIntPrice(order_vec_index_));

            if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
              DBGLOG_TIME_CLASS_FUNC << "CancelAsksEqBelowIntPrice " << order_manager_.GetAskIntPrice(order_vec_index_)
                                     << " since _size_seen_so_far_ = " << _size_seen_so_far_
                                     << " _max_size_resting_asks_ = " << _max_size_resting_asks_ << DBGLOG_ENDL_FLUSH;
            }
            break;
          }
        }
      }
    }
  } else {
    if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << "Cancelling all non best level orders as stdev is high" << DBGLOG_ENDL_FLUSH;
    }
    order_manager_.CancelBidsEqBelowIntPrice(best_nonself_bid_int_price_ - 1);
    order_manager_.CancelAsksEqBelowIntPrice(best_nonself_ask_int_price_ + 1);
  }
}

void BaseTrading::PlaceNonBestBids() {
  int t_position_ = my_position_;
  if (is_pair_strategy_) {
    t_position_ = my_combined_position_;
  } else if (is_structured_strategy_) {
    t_position_ = my_risk_;
  }

  const int t_worst_case_long_position_ =
      (param_set_.read_explicit_max_long_position_ ? param_set_.explicit_worst_case_long_position_
                                                   : param_set_.worst_case_position_);

  // placing supporting orders to num_non_best_bid_levels_monitored_
  if (my_position_ <= (param_set_.max_position_ / 2) ||
      param_set_.ignore_max_pos_check_for_non_best_) {  // position is not very long

    unsigned int _start_index_ = 1;
    _start_index_ = std::max(_start_index_, param_set_.min_distance_for_non_best_);
    if (param_set_.min_size_ahead_for_non_best_ > 0) {
      unsigned int _size_ahead_ = 0;
      auto i = 0u;
      for (; _size_ahead_ <= param_set_.min_size_ahead_for_non_best_ && i <= param_set_.max_distance_for_non_best_;
           i++) {
        _size_ahead_ += dep_market_view_.bid_size(i);
      }
      _start_index_ = std::max(_start_index_, i);
    }

    for (unsigned int _level_index_ = _start_index_;
         _level_index_ < (_start_index_ + param_set_.num_non_best_bid_levels_monitored_ - 1);
         _level_index_++) {  // check levels from 2 to param_set_.num_non_best_bid_levels_monitored_

      if ((int)(t_position_ + order_manager_.SumBidSizes() + (2 * param_set_.unit_trade_size_)) <
          t_worst_case_long_position_) {  // placing orders still keeps the total active orders within
        // worst_case_position_ then place orders at this level

        int _this_bid_int_price_ = (best_nonself_bid_int_price_ - _level_index_);
        if (order_manager_.GetTotalBidSizeOrderedAtIntPx(_this_bid_int_price_) == 0) {  // if no orders at this level
          order_manager_.SendTradeIntPx(
              _this_bid_int_price_, position_tradevarset_map_[P2TV_zero_idx_].l1bid_trade_size_, kTradeTypeBuy, 'S');
          if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
            DBGLOG_TIME_CLASS_FUNC << "SupportingSendTrade B of "
                                   << position_tradevarset_map_[P2TV_zero_idx_].l1bid_trade_size_ << " @ "
                                   << (dep_market_view_.min_price_increment() * _this_bid_int_price_)
                                   << " IntPx: " << _this_bid_int_price_ << " level: " << _level_index_
                                   << " mkt: " << best_nonself_bid_size_ << " @ " << best_nonself_bid_price_ << " X "
                                   << best_nonself_ask_price_ << " @ " << best_nonself_ask_size_ << " sumbidsizes "
                                   << order_manager_.SumBidSizes() << " + my_position_ " << t_position_
                                   << DBGLOG_ENDL_FLUSH;
          }
        }
      }
    }
  }
}

void BaseTrading::CancelNonBestBids() {
  int t_position_ = my_position_;
  if (is_pair_strategy_) {
    t_position_ = my_combined_position_;
  } else if (is_structured_strategy_) {
    t_position_ = my_risk_;
  }

  const int t_worst_case_long_position_ =
      (param_set_.read_explicit_max_long_position_ ? param_set_.explicit_worst_case_long_position_
                                                   : param_set_.worst_case_position_);
  {
    std::vector<int> &sum_bid_confirmed_ = order_manager_.SumBidConfirmed();
    int confirmed_top_bid_index_ = order_manager_.GetConfirmedTopBidIndex();
    int confirmed_bottom_bid_index_ = order_manager_.GetConfirmedBottomBidIndex();
    int confirmed_bid_index_ = confirmed_top_bid_index_;

    std::vector<int> &sum_bid_unconfirmed_ = order_manager_.SumBidUnconfirmed();
    int unconfirmed_top_bid_index_ = order_manager_.GetUnconfirmedTopBidIndex();
    int unconfirmed_bottom_bid_index_ = order_manager_.GetUnconfirmedBottomBidIndex();
    int unconfirmed_bid_index_ = unconfirmed_top_bid_index_;

    // std::vector < std::vector < BaseOrder * > > & bid_order_vec_ = order_manager_.BidOrderVec ( );
    int order_vec_top_bid_index_ = order_manager_.GetOrderVecTopBidIndex();
    int order_vec_bottom_bid_index_ = order_manager_.GetOrderVecBottomBidIndex();

    unsigned int _size_seen_so_far_ = 0;
    int _max_size_resting_bids_ = t_worst_case_long_position_ - std::max(0, t_position_);
    if (order_vec_top_bid_index_ != -1) {
      for (int order_vec_index_ = order_vec_top_bid_index_; order_vec_index_ >= order_vec_bottom_bid_index_;
           order_vec_index_--) {
        // the following code makes this not look at top level orders
        // hence top level orders are unaffected by worst_case_position_
        if (order_manager_.GetBidIntPrice(order_vec_index_) >= best_nonself_bid_int_price_) continue;

        // _size_seen_so_far_ adds up all confirmed and unconfirmed sizes of bids at levels at or above ( closer to
        // midprice ) than this level
        // where this level refers to the int_price_ at order_vec_index_

        while (unconfirmed_bottom_bid_index_ != -1 && unconfirmed_bid_index_ >= unconfirmed_bottom_bid_index_ &&
               order_manager_.GetBidIntPrice(unconfirmed_bid_index_) >=
                   order_manager_.GetBidIntPrice(order_vec_index_))  // TODO: replace with index_ comparison
        {
          _size_seen_so_far_ += sum_bid_unconfirmed_[unconfirmed_bid_index_];
          unconfirmed_bid_index_--;
        }

        while (confirmed_bottom_bid_index_ != -1 && confirmed_bid_index_ >= confirmed_bottom_bid_index_ &&
               order_manager_.GetBidIntPrice(confirmed_bid_index_) >=
                   order_manager_.GetBidIntPrice(order_vec_index_))  // TODO: replace with index_ comparison
        {
          _size_seen_so_far_ += sum_bid_confirmed_[confirmed_bid_index_];
          confirmed_bid_index_--;
        }

        // if simply all orders in calculating the _size_seen_so_far_ get executed in a sweep then our position will
        // reach
        // _size_seen_so_far_ + my_position_
        // and if this number equals or exceeds ( param_set_.worst_case_position_ )
        // then cancel all orders we can at this level or below
        // to avoid confusion perhaps we should avoid placing multiple orders atthe same level
        // since otherwise this could cause placing and canceling
        if ((int)_size_seen_so_far_ > _max_size_resting_bids_) {
          order_manager_.CancelBidsEqBelowIntPrice(order_manager_.GetBidIntPrice(order_vec_index_));

          if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
            DBGLOG_TIME_CLASS_FUNC << "CancelBidsEqBelowIntPrice " << order_manager_.GetBidIntPrice(order_vec_index_)
                                   << " since _size_seen_so_far_ = " << _size_seen_so_far_
                                   << " _max_size_resting_bids_ = " << _max_size_resting_bids_ << DBGLOG_ENDL_FLUSH;
          }
          break;
        }
      }
    }
  }
}

void BaseTrading::PlaceNonBestAsks() {
  int t_position_ = my_position_;
  if (is_pair_strategy_) {
    t_position_ = my_combined_position_;
  } else if (is_structured_strategy_) {
    t_position_ = my_risk_;
  }

  const int t_worst_case_short_position_ =
      (param_set_.read_explicit_max_short_position_ ? param_set_.explicit_worst_case_short_position_
                                                    : param_set_.worst_case_position_);

  // placing supporting orders to num_non_best_bid_levels_monitored_
  if (my_position_ >= (-param_set_.max_position_ / 2) ||
      param_set_.ignore_max_pos_check_for_non_best_) {  // position is not too short

    unsigned int _start_index_ = 1;
    _start_index_ = std::max(_start_index_, param_set_.min_distance_for_non_best_);
    if (param_set_.min_size_ahead_for_non_best_ > 0) {
      unsigned int _size_ahead_ = 0;
      auto i = 0u;
      for (; _size_ahead_ <= param_set_.min_size_ahead_for_non_best_ && i <= param_set_.max_distance_for_non_best_;
           i++) {
        _size_ahead_ += dep_market_view_.bid_size(i);
      }
      _start_index_ = std::max(_start_index_, i);
    }

    for (unsigned int _level_index_ = _start_index_;
         _level_index_ < (_start_index_ + param_set_.num_non_best_ask_levels_monitored_ - 1);
         _level_index_++) {  // check levels from 2 to param_set_.num_non_best_ask_levels_monitored_

      if ((int)(-t_position_ + order_manager_.SumAskSizes() + (2 * param_set_.unit_trade_size_)) <
          t_worst_case_short_position_) {  // placing orders still keeps the total active orders within
        // worst_case_position_ then place orders at this level

        int _this_ask_int_price_ = best_nonself_ask_int_price_ + _level_index_;
        if (order_manager_.GetTotalAskSizeOrderedAtIntPx(_this_ask_int_price_) == 0) {  // if no orders at this level
          order_manager_.SendTradeIntPx(
              _this_ask_int_price_, position_tradevarset_map_[P2TV_zero_idx_].l1ask_trade_size_, kTradeTypeSell, 'S');
          if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
            DBGLOG_TIME_CLASS_FUNC << "SupportingSendTrade S of "
                                   << position_tradevarset_map_[P2TV_zero_idx_].l1ask_trade_size_ << " @ "
                                   << (dep_market_view_.min_price_increment() * _this_ask_int_price_)
                                   << " IntPx: " << _this_ask_int_price_ << " level: " << _level_index_
                                   << " mkt: " << best_nonself_bid_size_ << " @ " << best_nonself_bid_price_ << " X "
                                   << best_nonself_ask_price_ << " @ " << best_nonself_ask_size_ << " sumasksizes "
                                   << order_manager_.SumAskSizes() << " + -my_position_ " << -t_position_
                                   << DBGLOG_ENDL_FLUSH;
          }
        }
      }
    }
  }
}

void BaseTrading::CancelNonBestAsks() {
  int t_position_ = my_position_;
  if (is_pair_strategy_) {
    t_position_ = my_combined_position_;
  } else if (is_structured_strategy_) {
    t_position_ = my_risk_;
  }

  const int t_worst_case_short_position_ =
      (param_set_.read_explicit_max_short_position_ ? param_set_.explicit_worst_case_short_position_
                                                    : param_set_.worst_case_position_);
  {
    // setting up aliases
    std::vector<int> &sum_ask_confirmed_ = order_manager_.SumAskConfirmed();
    int confirmed_top_ask_index_ = order_manager_.GetConfirmedTopAskIndex();
    int confirmed_bottom_ask_index_ = order_manager_.GetConfirmedBottomAskIndex();
    int confirmed_ask_index_ = confirmed_top_ask_index_;

    std::vector<int> &sum_ask_unconfirmed_ = order_manager_.SumAskUnconfirmed();
    int unconfirmed_top_ask_index_ = order_manager_.GetUnconfirmedTopAskIndex();
    int unconfirmed_bottom_ask_index_ = order_manager_.GetUnconfirmedBottomAskIndex();
    int unconfirmed_ask_index_ = unconfirmed_top_ask_index_;

    // std::vector < std::vector < BaseOrder * > > & ask_order_vec_ = order_manager_.AskOrderVec ( );
    int order_vec_top_ask_index_ = order_manager_.GetOrderVecTopAskIndex();
    int order_vec_bottom_ask_index_ = order_manager_.GetOrderVecBottomAskIndex();

    int _size_seen_so_far_ = 0;
    int _max_size_resting_asks_ = t_worst_case_short_position_ + std::min(0, t_position_);
    if (order_vec_top_ask_index_ != -1) {
      for (int order_vec_index_ = order_vec_top_ask_index_; order_vec_index_ >= order_vec_bottom_ask_index_;
           order_vec_index_--) {
        // the following code makes this not look at top level orders
        // hence top level orders are unaffected by worst_case_position_
        if (order_manager_.GetAskIntPrice(order_vec_index_) <= best_nonself_ask_int_price_) continue;

        // _size_seen_so_far_ adds up all confirmed and unconfirmed sizes of asks at levels at or above ( closer to
        // midprice ) than this level
        // where this level refers to the key of _intpx_2_ask_order_vec_iter_

        while (unconfirmed_top_ask_index_ != -1 && unconfirmed_ask_index_ >= unconfirmed_bottom_ask_index_ &&
               order_manager_.GetAskIntPrice(unconfirmed_ask_index_) <=
                   order_manager_.GetAskIntPrice(order_vec_index_)) {
          _size_seen_so_far_ += sum_ask_unconfirmed_[unconfirmed_ask_index_];
          unconfirmed_ask_index_--;
        }

        while (confirmed_top_ask_index_ != -1 && confirmed_ask_index_ >= confirmed_bottom_ask_index_ &&
               order_manager_.GetAskIntPrice(confirmed_ask_index_) <= order_manager_.GetAskIntPrice(order_vec_index_)) {
          _size_seen_so_far_ += sum_ask_confirmed_[confirmed_ask_index_];
          confirmed_ask_index_--;
        }

        // if simply all orders in calculating the _size_seen_so_far_ get executed in a sweep then our position will
        // reach
        // _size_seen_so_far_ + my_position_
        // and if this number equals or exceeds ( param_set_.worst_case_position_ )
        // then cancel all orders we can at this level or below
        // to avoid confusion perhaps we should avoid placing multiple orders atthe same level
        // since otherwise this could cause placing and canceling
        if ((int)_size_seen_so_far_ > _max_size_resting_asks_) {
          order_manager_.CancelAsksEqBelowIntPrice(order_manager_.GetAskIntPrice(order_vec_index_));

          if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
            DBGLOG_TIME_CLASS_FUNC << "CancelAsksEqBelowIntPrice " << order_manager_.GetAskIntPrice(order_vec_index_)
                                   << " since _size_seen_so_far_ = " << _size_seen_so_far_
                                   << " _max_size_resting_asks_ = " << _max_size_resting_asks_ << DBGLOG_ENDL_FLUSH;
          }
          break;
        }
      }
    }
  }
}

// this function doesnt let tradinglogic place passive orders by adjusting thresholds and tradesizes
// otherwise this function only places aggressive orders, cancels orders and turnsoff nonbest_monitoring
// dont place passive orders from here

void BaseTrading::CancelAndClose() {
  if (!param_set_.read_src_based_exec_changes_) {
    return;
  }
  int t_position_ = my_position_;

  if (exec_logic_indicators_helper_->avoid_long_market()) {
    order_manager_.CancelAllBidOrders();
    // for now just do 1 UTS per update
    // it is fine if we get an additional fill on short side, thus passive orders are left as they are
    // set buy side thresholds to GETFLATTHRESHOLDS
    if (exec_logic_indicators_helper_->avoid_long_market_aggressively() && my_position_ > 0 &&
        dep_market_view_.spread_increments() <= param_set_.max_spread_getflat_aggress_ &&
        best_nonself_bid_size_ < param_set_.max_size_to_aggress_ &&
        dep_market_view_.bestask_price() - dep_market_view_.mkt_size_weighted_price() > param_set_.getflat_aggress_) {
      int agg_trade_size_required_ = MathUtils::GetFlooredMultipleOf(std::min(t_position_, param_set_.unit_trade_size_),
                                                                     dep_market_view_.min_order_size());
      int agg_size_ordered_ = order_manager_.SumAskSizeConfirmedEqAboveIntPrice(best_nonself_bid_int_price_) +
                              order_manager_.SumAskSizeUnconfirmedEqAboveIntPrice(best_nonself_bid_int_price_);

      int size_ = agg_trade_size_required_ - agg_size_ordered_;
      if (size_ > 0) {
        SendTradeAndLog(best_nonself_bid_price_, best_nonself_bid_int_price_, size_, kTradeTypeSell, 'A',
                        "AvoidLongMarket");
      }
    }
    if (my_position_ >= 0) {
      current_tradevarset_.AddBidsBy(HIGH_THRESHOLD_VALUE);
    }
  }
  if (exec_logic_indicators_helper_->avoid_short_market()) {
    order_manager_.CancelAllAskOrders();
    if (exec_logic_indicators_helper_->avoid_short_market_aggressively() && my_position_ < 0 &&
        dep_market_view_.spread_increments() <= param_set_.max_spread_getflat_aggress_ &&
        best_nonself_ask_size_ < param_set_.max_size_to_aggress_ &&
        dep_market_view_.mkt_size_weighted_price() - dep_market_view_.bestbid_price() > param_set_.getflat_aggress_) {
      int agg_trade_size_required_ = MathUtils::GetFlooredMultipleOf(
          std::min(-t_position_, param_set_.unit_trade_size_), dep_market_view_.min_order_size());
      int agg_size_ordered_ = order_manager_.SumBidSizeConfirmedEqAboveIntPrice(best_nonself_ask_int_price_) +
                              order_manager_.SumBidSizeUnconfirmedEqAboveIntPrice(best_nonself_ask_int_price_);

      int size_ = agg_trade_size_required_ - agg_size_ordered_;
      if (size_ > 0) {
        SendTradeAndLog(best_nonself_ask_price_, best_nonself_ask_int_price_, size_, kTradeTypeBuy, 'A',
                        "AvoidShortMarket");
      }
    }
    if (my_position_ <= 0) {
      current_tradevarset_.AddAsksBy(HIGH_THRESHOLD_VALUE);
    }
  }
}

void BaseTrading::ProcessRegimeChange() {
  getflat_due_to_regime_indicator_ = exec_logic_indicators_helper_->getflat_due_to_regime_indicator();
  if (getflat_due_to_regime_indicator_) {
    if (my_position_ == 0) {
      current_tradevarset_.l1bid_trade_size_ = 0;
      current_tradevarset_.l1ask_trade_size_ = 0;
      order_manager_.CancelAllOrders();
    } else if (my_position_ < 0) {
      current_tradevarset_.l1bid_trade_size_ = MathUtils::GetFlooredMultipleOf(
          std::min(-my_position_, current_tradevarset_.l1bid_trade_size_), dep_market_view_.min_order_size());
      current_tradevarset_.l1ask_trade_size_ = 0;
      order_manager_.CancelAllAskOrders();
      CancelNonBestBids();
    } else if (my_position_ > 0) {
      current_tradevarset_.l1bid_trade_size_ = 0;
      current_tradevarset_.l1ask_trade_size_ = MathUtils::GetFlooredMultipleOf(
          std::min(my_position_, current_tradevarset_.l1ask_trade_size_), dep_market_view_.min_order_size());
      order_manager_.CancelAllBidOrders();
      CancelNonBestAsks();
    }
  }
}

void BaseTrading::ProcessAllowedEco() {
  if (allowed_events_present_ && !is_event_based_) {
    const std::vector<EventLine> &allowed_events_of_the_day_ = economic_events_manager_.allowed_events_of_the_day();
    if (getflat_due_to_allowed_economic_event_) {  // if currently in getflat see if we are getting out of it
      if ((last_allowed_event_index_ <= allowed_events_of_the_day_.size()) &&
          (watch_.msecs_from_midnight() >= allowed_events_of_the_day_[last_allowed_event_index_].end_mfm_)) {
        getflat_due_to_allowed_economic_event_ = false;
        if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
          DBGLOG_TIME << "getflat_due_to_allowed_economic_event_ set to false" << DBGLOG_ENDL_FLUSH;
          if (livetrading_) {
            DBGLOG_DUMP;
          }
        }
        getflat_due_to_allowed_economic_event_ = false;
      }
    } else {
      // TODO optimize by searching in only nearby events by time ... not all events
      for (unsigned int i = last_allowed_event_index_; i < allowed_events_of_the_day_.size(); i++) {
        if (watch_.msecs_from_midnight() < allowed_events_of_the_day_[i].start_mfm_) {
          break;
        } else {  // >= start
          if (watch_.msecs_from_midnight() < allowed_events_of_the_day_[i].end_mfm_) {
            if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
              DBGLOG_TIME << "getflat_due_to_allowed_economic_event_ set to true " << DBGLOG_ENDL_FLUSH;
              if (livetrading_) {
                DBGLOG_DUMP;
              }
            }
            getflat_due_to_allowed_economic_event_ = true;
            last_allowed_event_index_ = i;
            break;
          }
        }
      }
    }
  }
}

void BaseTrading::ProcessIndicatorHelperUpdate() {
  /// Used in cases like ors_indicator update

  if (exec_logic_indicators_helper_->called_on_regime_update()) {
    if (param_index_to_use_ != exec_logic_indicators_helper_->regime_param_index()) {
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        DBGLOG_TIME << "Param switched to " << exec_logic_indicators_helper_->regime_param_index() << "\n";
      }
      param_index_to_use_ = exec_logic_indicators_helper_->regime_param_index();
      param_set_ = param_set_vec_[param_index_to_use_];
      position_tradevarset_map_ = position_tradevarset_map_vec_[param_index_to_use_];
      map_pos_increment_ = map_pos_increment_vec_[param_index_to_use_];
      max_opentrade_loss_ = param_set_vec_[param_index_to_use_].max_opentrade_loss_;
      ///< since  position_tradevarset_map_ has changed, we need to reset the current_tradevarset_
      if (is_ready_) TradeVarSetLogic(my_position_);
    }
  }
}

int BaseTrading::GetControlChars() const {
  int query_control_bits_ = 0;
  if (should_be_getting_flat_) {
    query_control_bits_ |= QCB_SBGF;
  }
  if (getflat_due_to_close_) {
    query_control_bits_ |= QCB_GFDTC;
  }
  if (getflat_due_to_max_position_) {
    query_control_bits_ |= QCB_GFDTMP;
  }
  if (getflat_due_to_max_loss_) {
    query_control_bits_ |= QCB_GFDTML;
  }
  if (getflat_due_to_max_opentrade_loss_) {
    query_control_bits_ |= QCB_GFDTMOTL;
  }
  // if ( getflat_due_to_max_pertrade_loss_ ) { query_control_bits_ |= QCB_GFDTMPTL; }
  if (getflat_due_to_economic_times_) {
    query_control_bits_ |= QCB_GFDTET;
  }
  if (external_getflat_) {
    query_control_bits_ |= QCB_EGF;
  }
  if (external_freeze_trading_) {
    query_control_bits_ |= QCB_EFT;
  }
  if (external_cancel_all_outstanding_orders_) {
    query_control_bits_ |= QCB_ECAOO;
  }
  return query_control_bits_;
}

void BaseTrading::LogControlChars(const int query_control_bits_) {
  if (query_control_bits_ & QCB_SBGF) {
    DBGLOG_TIME << "QCB_SBGF " << '\n';
  }
  if (query_control_bits_ & QCB_GFDTC) {
    DBGLOG_TIME << "QCB_GFDTC " << '\n';
  }
  if (query_control_bits_ & QCB_GFDTMP) {
    DBGLOG_TIME << "QCB_GFDTMP " << '\n';
  }
  if (query_control_bits_ & QCB_GFDTML) {
    DBGLOG_TIME << "QCB_GFDTML " << '\n';
  }
  if (query_control_bits_ & QCB_GFDTMOTL) {
    DBGLOG_TIME << "QCB_GFDTMOTL " << '\n';
  }
  if (query_control_bits_ & QCB_GFDTMPTL) {
    DBGLOG_TIME << "QCB_GFDTMPTL " << '\n';
  }
  if (query_control_bits_ & QCB_GFDTET) {
    DBGLOG_TIME << "QCB_GFDTET " << '\n';
  }
  if (query_control_bits_ & QCB_EGF) {
    DBGLOG_TIME << "QCB_EGF " << '\n';
  }
  if (query_control_bits_ & QCB_EFT) {
    DBGLOG_TIME << "QCB_EFT " << '\n';
  }
  if (query_control_bits_ & QCB_ECAOO) {
    DBGLOG_TIME << "QCB_ECAOO " << '\n';
  }
  dbglogger_.CheckToFlushBuffer();
}

void BaseTrading::ShowParams() {
  // if ( ! livetrading_ ) // only do the rest of the logic if this is livetrading
  //   return;
  for (auto i = 0u; i < param_set_vec_.size(); i++) {
    param_set_vec_[i].WriteSendStruct(control_reply_struct_.param_set_send_struct_);
    ParamSetSendStruct &param_set_send_struct_ = control_reply_struct_.param_set_send_struct_;

    param_set_send_struct_.query_control_bits_ = GetControlChars();

    // log the state
    LogControlChars(param_set_send_struct_.query_control_bits_);
    DBGLOG_TIME << "==========================================\n"
                << "Param " << i << ":" << DBGLOG_ENDL_FLUSH;
    // log the variables
    LogParamSetSendStruct(param_set_send_struct_);

    DBGLOG_TIME << "non_standard_market_condition_check_short_msecs_ "
                << param_set_.non_standard_market_condition_check_short_msecs_ << DBGLOG_ENDL_FLUSH;
    DBGLOG_TIME << "non_standard_market_condition_check_long_msecs_ "
                << param_set_.non_standard_market_condition_check_long_msecs_ << DBGLOG_ENDL_FLUSH;
    DBGLOG_TIME << "non_standard_market_condition_min_best_level_size_ "
                << param_set_.non_standard_market_condition_min_best_level_size_ << DBGLOG_ENDL_FLUSH;
    DBGLOG_TIME << "non_standard_market_condition_min_best_level_order_count_ "
                << param_set_.non_standard_market_condition_min_best_level_order_count_ << DBGLOG_ENDL_FLUSH;
    DBGLOG_TIME << "non_standard_market_condition_max_avg_order_size_ "
                << param_set_.non_standard_market_condition_max_avg_order_size_ << DBGLOG_ENDL_FLUSH;
    DBGLOG_TIME << "non_standard_market_condition_max_spread_ " << param_set_.non_standard_market_condition_max_spread_
                << DBGLOG_ENDL_FLUSH;

    // log position and pnl
    DBGLOG_TIME << "my_position_ " << my_position_ << DBGLOG_ENDL_FLUSH;
    DBGLOG_TIME << "total_pnl_ " << order_manager_.base_pnl().total_pnl() << DBGLOG_ENDL_FLUSH;
    DBGLOG_TIME << "global_pnl_ " << our_global_pnl_ << DBGLOG_ENDL_FLUSH;
    DBGLOG_TIME << "==========================================" << DBGLOG_ENDL_FLUSH;
    if (livetrading_) {
      DBGLOG_DUMP;
    }

    // record time position and pnl
    control_reply_struct_.time_set_by_query_ = watch_.tv();
    control_reply_struct_.my_position_ = my_position_;
    control_reply_struct_.total_pnl_ = order_manager_.base_pnl().total_pnl();
  }
}

void BaseTrading::LogParamSetSendStruct(ParamSetSendStruct &param_set_send_struct_) {
  DBGLOG_TIME << "worst_case_position_ " << param_set_send_struct_.worst_case_position_ << DBGLOG_ENDL_FLUSH;
  DBGLOG_TIME << "max_position_ " << param_set_send_struct_.max_position_ << DBGLOG_ENDL_FLUSH;
  DBGLOG_TIME << "max_global_position_ " << param_set_send_struct_.max_global_position_ << DBGLOG_ENDL_FLUSH;
  DBGLOG_TIME << "max_security_position_ " << param_set_send_struct_.max_security_position_ << DBGLOG_ENDL_FLUSH;
  DBGLOG_TIME << "unit_trade_size_" << param_set_send_struct_.unit_trade_size_ << DBGLOG_ENDL_FLUSH;

  DBGLOG_TIME << "highpos_limits_" << param_set_send_struct_.highpos_limits_ << DBGLOG_ENDL_FLUSH;
  DBGLOG_TIME << "highpos_thresh_factor_ " << param_set_send_struct_.highpos_thresh_factor_ << DBGLOG_ENDL_FLUSH;
  DBGLOG_TIME << "highpos_thresh_decrease_ " << param_set_send_struct_.highpos_thresh_decrease_ << DBGLOG_ENDL_FLUSH;
  DBGLOG_TIME << "highpos_size_factor_ " << param_set_send_struct_.highpos_size_factor_ << DBGLOG_ENDL_FLUSH;

  DBGLOG_TIME << "increase_place_ " << param_set_send_struct_.increase_place_ << DBGLOG_ENDL_FLUSH;
  DBGLOG_TIME << "increase_keep_ " << param_set_send_struct_.increase_keep_ << DBGLOG_ENDL_FLUSH;
  DBGLOG_TIME << "zeropos_limits_ " << param_set_send_struct_.zeropos_limits_ << DBGLOG_ENDL_FLUSH;
  DBGLOG_TIME << "zeropos_place_ " << param_set_send_struct_.zeropos_place_ << DBGLOG_ENDL_FLUSH;
  DBGLOG_TIME << "zeropos_keep_ " << param_set_send_struct_.zeropos_keep_ << DBGLOG_ENDL_FLUSH;
  DBGLOG_TIME << "decrease_place_ " << param_set_send_struct_.decrease_place_ << DBGLOG_ENDL_FLUSH;
  DBGLOG_TIME << "decrease_keep_ " << param_set_send_struct_.decrease_keep_ << DBGLOG_ENDL_FLUSH;

  DBGLOG_TIME << "place_keep_diff_ " << param_set_send_struct_.place_keep_diff_ << DBGLOG_ENDL_FLUSH;
  DBGLOG_TIME << "increase_zeropos_diff_ " << param_set_send_struct_.increase_zeropos_diff_ << DBGLOG_ENDL_FLUSH;
  DBGLOG_TIME << "zeropos_decrease_diff_ " << param_set_send_struct_.zeropos_decrease_diff_ << DBGLOG_ENDL_FLUSH;

  DBGLOG_TIME << "safe_distance_ " << param_set_send_struct_.safe_distance_ << DBGLOG_ENDL_FLUSH;

  DBGLOG_TIME << "allowed_to_improve_ " << param_set_send_struct_.allowed_to_improve_ << DBGLOG_ENDL_FLUSH;
  DBGLOG_TIME << "allowed_to_aggress_ " << param_set_send_struct_.allowed_to_aggress_ << DBGLOG_ENDL_FLUSH;
  DBGLOG_TIME << "improve_ " << param_set_send_struct_.improve_ << DBGLOG_ENDL_FLUSH;
  DBGLOG_TIME << "aggressive_ " << param_set_send_struct_.aggressive_ << DBGLOG_ENDL_FLUSH;

  DBGLOG_TIME << "max_self_ratio_at_level_ " << param_set_send_struct_.max_self_ratio_at_level_ << DBGLOG_ENDL_FLUSH;
  DBGLOG_TIME << "longevity_support_ " << param_set_send_struct_.longevity_support_ << DBGLOG_ENDL_FLUSH;

  DBGLOG_TIME << "max_position_to_lift_ " << param_set_send_struct_.max_position_to_lift_ << DBGLOG_ENDL_FLUSH;
  DBGLOG_TIME << "max_position_to_bidimprove_ " << param_set_send_struct_.max_position_to_bidimprove_
              << DBGLOG_ENDL_FLUSH;
  DBGLOG_TIME << "max_position_to_cancel_on_lift_ " << param_set_send_struct_.max_position_to_cancel_on_lift_
              << DBGLOG_ENDL_FLUSH;
  DBGLOG_TIME << "max_size_to_aggress_ " << param_set_send_struct_.max_size_to_aggress_ << DBGLOG_ENDL_FLUSH;
  DBGLOG_TIME << "min_position_to_hit_ " << param_set_send_struct_.min_position_to_hit_ << DBGLOG_ENDL_FLUSH;
  DBGLOG_TIME << "min_position_to_askimprove_ " << param_set_send_struct_.min_position_to_askimprove_
              << DBGLOG_ENDL_FLUSH;
  DBGLOG_TIME << "min_position_to_cancel_on_hit_ " << param_set_send_struct_.min_position_to_cancel_on_hit_
              << DBGLOG_ENDL_FLUSH;

  DBGLOG_TIME << "max_int_spread_to_place_ " << param_set_send_struct_.max_int_spread_to_place_ << DBGLOG_ENDL_FLUSH;
  DBGLOG_TIME << "max_int_spread_to_cross_ " << param_set_send_struct_.max_int_spread_to_cross_ << DBGLOG_ENDL_FLUSH;
  DBGLOG_TIME << "min_int_spread_to_improve_ " << param_set_send_struct_.min_int_spread_to_improve_
              << DBGLOG_ENDL_FLUSH;
  DBGLOG_TIME << "num_non_best_bid_levels_monitored_ " << param_set_send_struct_.num_non_best_bid_levels_monitored_
              << DBGLOG_ENDL_FLUSH;
  DBGLOG_TIME << "num_non_best_ask_levels_monitored_ " << param_set_send_struct_.num_non_best_ask_levels_monitored_
              << DBGLOG_ENDL_FLUSH;

  DBGLOG_TIME << "num_increase_ticks_to_keep_ " << param_set_.num_increase_ticks_to_keep_ << DBGLOG_ENDL_FLUSH;
  DBGLOG_TIME << "num_decrease_ticks_to_keep_ " << param_set_.num_decrease_ticks_to_keep_ << DBGLOG_ENDL_FLUSH;

  DBGLOG_TIME << "moderate_time_limit_ " << param_set_.moderate_time_limit_ << DBGLOG_ENDL_FLUSH;
  DBGLOG_TIME << "high_time_limit_ " << param_set_.high_time_limit_ << DBGLOG_ENDL_FLUSH;
  DBGLOG_TIME << "pclose_factor_ " << param_set_.pclose_factor_ << DBGLOG_ENDL_FLUSH;

  DBGLOG_TIME << "max_loss_ " << -param_set_send_struct_.max_loss_ << DBGLOG_ENDL_FLUSH;
  DBGLOG_TIME << "max_pnl_ " << param_set_send_struct_.max_pnl_ << DBGLOG_ENDL_FLUSH;
  DBGLOG_TIME << "global_max_loss_ " << -param_set_send_struct_.global_max_loss_ << DBGLOG_ENDL_FLUSH;
  DBGLOG_TIME << "max_opentrade_loss_ " << -param_set_send_struct_.max_opentrade_loss_ << DBGLOG_ENDL_FLUSH;

  DBGLOG_TIME << "cooloff_interval_ " << param_set_send_struct_.cooloff_interval_ << DBGLOG_ENDL_FLUSH;
  DBGLOG_TIME << "agg_cooloff_interval_ " << param_set_send_struct_.agg_cooloff_interval_ << DBGLOG_ENDL_FLUSH;

  DBGLOG_TIME << "stdev_fact_ " << param_set_send_struct_.stdev_fact_ << DBGLOG_ENDL_FLUSH;
  DBGLOG_TIME << "stdev_cap_ " << param_set_send_struct_.stdev_cap_ << DBGLOG_ENDL_FLUSH;

  DBGLOG_TIME << "px_band_ " << param_set_send_struct_.px_band_ << DBGLOG_ENDL_FLUSH;
  DBGLOG_TIME << "low_stdev_lvl_ " << param_set_send_struct_.low_stdev_lvl_ << DBGLOG_ENDL_FLUSH;
  DBGLOG_TIME << "min_size_to_join_ " << param_set_send_struct_.min_size_to_join_ << DBGLOG_ENDL_FLUSH;
  DBGLOG_TIME << "use_sqrt_stdev_ " << (param_set_.use_sqrt_stdev_ ? "Y" : "N") << DBGLOG_ENDL_FLUSH;

  DBGLOG_TIME << "severity_to_getflat_on_ " << severity_to_getflat_on_ << DBGLOG_ENDL_FLUSH;
  DBGLOG_TIME << "throttle_message_limit_ " << param_set_send_struct_.throttle_message_limit_ << DBGLOG_ENDL_FLUSH;
}
void BaseTrading::ReportResults(HFSAT::BulkFileWriter &trades_writer_, bool conservative_close_) {
  if (livetrading_) {
    printf("SIMRESULT %d %d %d %d %d %d %d\n",
           (int)order_manager_.base_pnl().ReportConservativeTotalPNL(conservative_close_),
           order_manager_.trade_volume(), order_manager_.SupportingOrderFilledPercent(),
           order_manager_.BestLevelOrderFilledPercent(), order_manager_.AggressiveOrderFilledPercent(),
           order_manager_.ImproveOrderFilledPercent(), improve_cancel_counter_);
    DBGLOG_TIME << "SIMRESULT " << (int)order_manager_.base_pnl().ReportConservativeTotalPNL(conservative_close_) << " "
                << order_manager_.trade_volume() << " " << order_manager_.SupportingOrderFilledPercent() << " "
                << order_manager_.BestLevelOrderFilledPercent() << " " << order_manager_.AggressiveOrderFilledPercent()
                << " " << order_manager_.ImproveOrderFilledPercent() << " " << improve_cancel_counter_ << " "
                << DBGLOG_ENDL_FLUSH;
  } else {
    {
      int t_pnl_ = (int)(order_manager_.base_pnl().ReportConservativeTotalPNL(conservative_close_));
      printf("SIMRESULT %d %d %d %d %d %d\n", t_pnl_, order_manager_.trade_volume(),
             order_manager_.SupportingOrderFilledPercent(), order_manager_.BestLevelOrderFilledPercent(),
             order_manager_.AggressiveOrderFilledPercent(), order_manager_.ImproveOrderFilledPercent());
      if (param_set_.sgx_market_making_) {
        unsigned int total_time_ = trading_end_utc_mfm_ - trading_start_utc_mfm_;
        double fraction_time_quoted_ = (100.0 * total_time_quoted_) / total_time_;
        std::cerr << trading_start_utc_mfm_ << " " << trading_end_utc_mfm_ << " " << total_time_ << "\n";
        std::cerr << total_time_quoted_ << " " << fraction_time_quoted_ << "\n";
        printf("Time Quoted %d\n", (int)fraction_time_quoted_);
      }
      if ((!strncmp(getenv("USER"), "sghosh", strlen("rkumar")))) {
        if (num_stdev_calls_ > 0) {
          printf("AVG_STDEV %f \n", sum_stdev_calls_ / num_stdev_calls_);
        }
      }

      trades_writer_ << "SIMRESULT " << runtime_id_ << " " << t_pnl_ << " " << order_manager_.trade_volume() << " "
                     << order_manager_.SupportingOrderFilledPercent() << " "
                     << order_manager_.BestLevelOrderFilledPercent() << " "
                     << order_manager_.AggressiveOrderFilledPercent() << " "
                     << order_manager_.ImproveOrderFilledPercent() << " " << improve_cancel_counter_ << " "
                     << "\n";

      trades_writer_ << "EOD_MIN_PNL: " << runtime_id_ << " " << order_manager_.base_pnl().min_pnl_till_now() << " "
                     << num_opentrade_loss_hits_ << "\n";
      trades_writer_ << "EOD_MSG_COUNT: " << runtime_id_ << " "
                     << (order_manager_.SendOrderCount() + order_manager_.CxlOrderCount() +
                         order_manager_.ModifyOrderCount()) << "\n";
      trades_writer_ << "NUM_OPENTRADE_HITS: " << runtime_id_ << " " << num_opentrade_loss_hits_ << "\n";
      trades_writer_ << "UNIT_TRADE_SIZE: " << runtime_id_ << " " << param_set_.unit_trade_size_ << "\n";
      trades_writer_ << "PNLSAMPLES " << runtime_id_ << " ";
      if (pnl_samples_.size() > 0) {
        for (auto i = 0u; i < pnl_samples_.size(); i++) {
          trades_writer_ << pnl_sampling_timestamps_[i] << " " << pnl_samples_[i] << " ";
        }
        trades_writer_ << "\n";
      } else {
        trades_writer_ << trading_end_utc_mfm_ << " " << t_pnl_ << "\n";
      }

      trades_writer_.CheckToFlushBuffer();

      dbglogger_ << "SIMRESULT " << runtime_id_ << " " << t_pnl_ << " " << order_manager_.trade_volume() << " "
                 << order_manager_.SupportingOrderFilledPercent() << " " << order_manager_.BestLevelOrderFilledPercent()
                 << " " << order_manager_.AggressiveOrderFilledPercent() << " "
                 << order_manager_.ImproveOrderFilledPercent() << " " << improve_cancel_counter_ << " "
                 << "\n";

      dbglogger_ << "EOD_MIN_PNL: " << runtime_id_ << " " << order_manager_.base_pnl().min_pnl_till_now() << " "
                 << num_opentrade_loss_hits_ << "\n";
      dbglogger_ << "EOD_MSG_COUNT: " << runtime_id_ << " "
                 << (order_manager_.SendOrderCount() + order_manager_.CxlOrderCount() +
                     order_manager_.ModifyOrderCount()) << "\n";
      dbglogger_ << "NUM_OPENTRADE_HITS: " << runtime_id_ << " " << num_opentrade_loss_hits_ << "\n";
      dbglogger_ << "UNIT_TRADE_SIZE: " << runtime_id_ << " " << param_set_.unit_trade_size_ << "\n";
      dbglogger_ << "PNLSAMPLES " << runtime_id_ << " ";

      if (pnl_samples_.size() > 0) {
        for (auto i = 0u; i < pnl_samples_.size(); i++) {
          dbglogger_ << pnl_sampling_timestamps_[i] << " " << pnl_samples_[i] << " ";
        }
        dbglogger_ << "\n";
      } else {
        dbglogger_ << trading_end_utc_mfm_ << " " << t_pnl_ << "\n";
      }

      dbglogger_.CheckToFlushBuffer();

      if ((!strncmp(getenv("USER"), "sghosh", strlen("sghosh"))) ||
          (!strncmp(getenv("USER"), "mayank", strlen("mayank"))) ||
          (!strncmp(getenv("USER"), "kishenp", strlen("kishenp")))) {
        printf("FILLRATIO %d\n", order_manager_.AllOrderFilledPercent());
        printf("SEND-MSG-COUNT %7d | CXL-MSG-COUNT %7d | TOTAL-MSG-COUNT %7d\n", order_manager_.SendOrderCount(),
               order_manager_.CxlOrderCount(), (order_manager_.SendOrderCount() + order_manager_.CxlOrderCount()));
      }
    }
  }
}

void BaseTrading::ShowBook() {
  printf("\033[1;1H");  // move to 1st line
  printf("Time: %s | %s\n", watch_.const_time_string().c_str(), watch_.tv().ToString().c_str());
  // printf ( "Time: %s\n", watch_.tv ( ) . ToString ( ) . c_str ( ) );
  printf("Secname: %s   \n", dep_market_view_.secname());
  printf("\033[4;1H");  // move to 4th line
  unsigned int max_levels_ = 5;

  unsigned int m_m_levels =
      std::min(dep_market_view_.MinValidNumBidLevels(max_levels_), dep_market_view_.MinValidNumAskLevels(max_levels_));

  for (unsigned int t_level_ = 0; t_level_ < m_m_levels; t_level_++) {
    if (t_level_ == 0) {
      if (dep_market_view_.this_smv_exch_source_ == HFSAT::kExchSourceMICEX ||
          dep_market_view_.this_smv_exch_source_ == HFSAT::kExchSourceMICEX_EQ ||
          dep_market_view_.this_smv_exch_source_ == HFSAT::kExchSourceMICEX_CR) {
        printf(
            "%s%4d %10d %6d %20.9f %14d%s X %s%14d %20.9f %6d %10d %4d                 \n%s",
            (order_manager_.GetTotalBidSizeOrderedAtIntPx(dep_market_view_.bid_int_price(t_level_)) == 0) ? BLUE : RED,

            dep_market_view_.bid_int_price_level(t_level_), dep_market_view_.bid_size(t_level_),
            dep_market_view_.bid_order(t_level_), dep_market_view_.bid_price(t_level_),
            dep_market_view_.bid_int_price(t_level_),

            GREEN,
            (order_manager_.GetTotalAskSizeOrderedAtIntPx(dep_market_view_.ask_int_price(t_level_)) == 0) ? BLUE : RED,

            dep_market_view_.ask_int_price(t_level_), dep_market_view_.ask_price(t_level_),
            dep_market_view_.ask_order(t_level_), dep_market_view_.ask_size(t_level_),
            dep_market_view_.ask_int_price_level(t_level_),

            GREEN);
      } else {
        printf(
            "%s%2d %5d %3d %10.7f %7d%s X %s%7d %10.7f %3d %5d %2d                 \n%s",
            (order_manager_.GetTotalBidSizeOrderedAtIntPx(dep_market_view_.bid_int_price(t_level_)) == 0) ? BLUE : RED,

            dep_market_view_.bid_int_price_level(t_level_), dep_market_view_.bid_size(t_level_),
            dep_market_view_.bid_order(t_level_), dep_market_view_.bid_price(t_level_),
            dep_market_view_.bid_int_price(t_level_),

            GREEN,
            (order_manager_.GetTotalAskSizeOrderedAtIntPx(dep_market_view_.ask_int_price(t_level_)) == 0) ? BLUE : RED,

            dep_market_view_.ask_int_price(t_level_), dep_market_view_.ask_price(t_level_),
            dep_market_view_.ask_order(t_level_), dep_market_view_.ask_size(t_level_),
            dep_market_view_.ask_int_price_level(t_level_),

            GREEN);
      }
    } else {
      if (dep_market_view_.this_smv_exch_source_ == HFSAT::kExchSourceMICEX ||
          dep_market_view_.this_smv_exch_source_ == HFSAT::kExchSourceMICEX_EQ ||
          dep_market_view_.this_smv_exch_source_ == HFSAT::kExchSourceMICEX_CR) {
        printf(
            "%s%4d %10d %6d %20.9f %14d%s X %s%14d %20.9f %6d %10d %4d                 \n%s",
            (order_manager_.GetTotalBidSizeOrderedAtIntPx(dep_market_view_.bid_int_price(t_level_)) == 0) ? GREEN : RED,

            dep_market_view_.bid_int_price_level(t_level_), dep_market_view_.bid_size(t_level_),
            dep_market_view_.bid_order(t_level_), dep_market_view_.bid_price(t_level_),
            dep_market_view_.bid_int_price(t_level_),

            GREEN,
            (order_manager_.GetTotalAskSizeOrderedAtIntPx(dep_market_view_.ask_int_price(t_level_)) == 0) ? GREEN : RED,

            dep_market_view_.ask_int_price(t_level_), dep_market_view_.ask_price(t_level_),
            dep_market_view_.ask_order(t_level_), dep_market_view_.ask_size(t_level_),
            dep_market_view_.ask_int_price_level(t_level_),

            GREEN);
      } else {
        printf(
            "%s%2d %5d %3d %10.7f %7d%s X %s%7d %10.7f %3d %5d %2d                 \n%s",
            (order_manager_.GetTotalBidSizeOrderedAtIntPx(dep_market_view_.bid_int_price(t_level_)) == 0) ? GREEN : RED,

            dep_market_view_.bid_int_price_level(t_level_), dep_market_view_.bid_size(t_level_),
            dep_market_view_.bid_order(t_level_), dep_market_view_.bid_price(t_level_),
            dep_market_view_.bid_int_price(t_level_),

            GREEN,
            (order_manager_.GetTotalAskSizeOrderedAtIntPx(dep_market_view_.ask_int_price(t_level_)) == 0) ? GREEN : RED,

            dep_market_view_.ask_int_price(t_level_), dep_market_view_.ask_price(t_level_),
            dep_market_view_.ask_order(t_level_), dep_market_view_.ask_size(t_level_),
            dep_market_view_.ask_int_price_level(t_level_),

            GREEN);
      }
    }
  }

  if ((m_m_levels < max_levels_) && (m_m_levels < dep_market_view_.MinValidNumBidLevels(max_levels_))) {
    for (unsigned int t_level_ = m_m_levels; t_level_ < dep_market_view_.MinValidNumBidLevels(max_levels_);
         t_level_++) {
      if (t_level_ == 0) {
        if (dep_market_view_.this_smv_exch_source_ == HFSAT::kExchSourceMICEX ||
            dep_market_view_.this_smv_exch_source_ == HFSAT::kExchSourceMICEX_EQ ||
            dep_market_view_.this_smv_exch_source_ == HFSAT::kExchSourceMICEX_CR) {
          printf("%s%4d %10d %6d %20.9f %14d%s X %s%14s %20s %6s %10s %4s                 \n%s",
                 (order_manager_.GetTotalBidSizeOrderedAtIntPx(dep_market_view_.bid_int_price(t_level_)) == 0) ? BLUE
                                                                                                               : RED,

                 dep_market_view_.bid_int_price_level(t_level_), dep_market_view_.bid_size(t_level_),
                 dep_market_view_.bid_order(t_level_), dep_market_view_.bid_price(t_level_),
                 dep_market_view_.bid_int_price(t_level_),

                 GREEN,
                 (order_manager_.GetTotalAskSizeOrderedAtIntPx(dep_market_view_.ask_int_price(t_level_)) == 0) ? BLUE
                                                                                                               : RED,

                 "-", "-", "-", "-", "-",

                 GREEN);
        } else {
          printf("%s%2d %5d %3d %10.7f %7d%s X %s%7s %10s %3s %5s %2s                 \n%s",
                 (order_manager_.GetTotalBidSizeOrderedAtIntPx(dep_market_view_.bid_int_price(t_level_)) == 0) ? BLUE
                                                                                                               : RED,

                 dep_market_view_.bid_int_price_level(t_level_), dep_market_view_.bid_size(t_level_),
                 dep_market_view_.bid_order(t_level_), dep_market_view_.bid_price(t_level_),
                 dep_market_view_.bid_int_price(t_level_),

                 GREEN,
                 (order_manager_.GetTotalAskSizeOrderedAtIntPx(dep_market_view_.ask_int_price(t_level_)) == 0) ? BLUE
                                                                                                               : RED,

                 "-", "-", "-", "-", "-",

                 GREEN);
        }
      } else {
        if (dep_market_view_.this_smv_exch_source_ == HFSAT::kExchSourceMICEX ||
            dep_market_view_.this_smv_exch_source_ == HFSAT::kExchSourceMICEX_EQ ||
            dep_market_view_.this_smv_exch_source_ == HFSAT::kExchSourceMICEX_CR) {
          printf("%s%4d %10d %6d %20.9f %14d%s X %s%14s %20s %6s %10s %4s                 \n%s",
                 (order_manager_.GetTotalBidSizeOrderedAtIntPx(dep_market_view_.bid_int_price(t_level_)) == 0) ? GREEN
                                                                                                               : RED,

                 dep_market_view_.bid_int_price_level(t_level_), dep_market_view_.bid_size(t_level_),
                 dep_market_view_.bid_order(t_level_), dep_market_view_.bid_price(t_level_),
                 dep_market_view_.bid_int_price(t_level_),

                 GREEN,
                 (order_manager_.GetTotalAskSizeOrderedAtIntPx(dep_market_view_.ask_int_price(t_level_)) == 0) ? GREEN
                                                                                                               : RED,

                 "-", "-", "-", "-", "-",

                 GREEN);
        } else {
          printf("%s%2d %5d %3d %10.7f %7d%s X %s%7s %10s %3s %5s %2s                 \n%s",
                 (order_manager_.GetTotalBidSizeOrderedAtIntPx(dep_market_view_.bid_int_price(t_level_)) == 0) ? GREEN
                                                                                                               : RED,

                 dep_market_view_.bid_int_price_level(t_level_), dep_market_view_.bid_size(t_level_),
                 dep_market_view_.bid_order(t_level_), dep_market_view_.bid_price(t_level_),
                 dep_market_view_.bid_int_price(t_level_),

                 GREEN,
                 (order_manager_.GetTotalAskSizeOrderedAtIntPx(dep_market_view_.ask_int_price(t_level_)) == 0) ? GREEN
                                                                                                               : RED,

                 "-", "-", "-", "-", "-",

                 GREEN);
        }
      }
    }
  }

  if ((m_m_levels < max_levels_) && (m_m_levels < (unsigned int)dep_market_view_.NumAskLevels())) {
    for (unsigned int t_level_ = m_m_levels;
         t_level_ < std::min(max_levels_, (unsigned int)dep_market_view_.NumAskLevels()); t_level_++) {
      if (t_level_ == 0) {
        if (dep_market_view_.this_smv_exch_source_ == HFSAT::kExchSourceMICEX ||
            dep_market_view_.this_smv_exch_source_ == HFSAT::kExchSourceMICEX_EQ ||
            dep_market_view_.this_smv_exch_source_ == HFSAT::kExchSourceMICEX_CR) {
          printf("%s%4s %10s %6s %20s %14s%s X %s%14d %20.9f %6d %10d %4d                 \n%s",
                 (order_manager_.GetTotalBidSizeOrderedAtIntPx(dep_market_view_.bid_int_price(t_level_)) == 0) ? BLUE
                                                                                                               : RED,

                 "-", "-", "-", "-", "-",

                 GREEN,
                 (order_manager_.GetTotalAskSizeOrderedAtIntPx(dep_market_view_.ask_int_price(t_level_)) == 0) ? BLUE
                                                                                                               : RED,

                 dep_market_view_.ask_int_price(t_level_), dep_market_view_.ask_price(t_level_),
                 dep_market_view_.ask_order(t_level_), dep_market_view_.ask_size(t_level_),
                 dep_market_view_.ask_int_price_level(t_level_), GREEN);
        } else {
          printf("%s%2s %5s %3s %10s %7s%s X %s%7d %10.7f %3d %5d %2d                 \n%s",
                 (order_manager_.GetTotalBidSizeOrderedAtIntPx(dep_market_view_.bid_int_price(t_level_)) == 0) ? BLUE
                                                                                                               : RED,

                 "-", "-", "-", "-", "-",

                 GREEN,
                 (order_manager_.GetTotalAskSizeOrderedAtIntPx(dep_market_view_.ask_int_price(t_level_)) == 0) ? BLUE
                                                                                                               : RED,

                 dep_market_view_.ask_int_price(t_level_), dep_market_view_.ask_price(t_level_),
                 dep_market_view_.ask_order(t_level_), dep_market_view_.ask_size(t_level_),
                 dep_market_view_.ask_int_price_level(t_level_), GREEN);
        }
      } else {
        if (dep_market_view_.this_smv_exch_source_ == HFSAT::kExchSourceMICEX ||
            dep_market_view_.this_smv_exch_source_ == HFSAT::kExchSourceMICEX_EQ ||
            dep_market_view_.this_smv_exch_source_ == HFSAT::kExchSourceMICEX_CR) {
          printf("%s%4s %10s %6s %20s %14s%s X %s%14d %20.9f %6d %10d %4d                 \n%s",
                 (order_manager_.GetTotalBidSizeOrderedAtIntPx(dep_market_view_.bid_int_price(t_level_)) == 0) ? GREEN
                                                                                                               : RED,

                 "-", "-", "-", "-", "-",

                 GREEN,
                 (order_manager_.GetTotalAskSizeOrderedAtIntPx(dep_market_view_.ask_int_price(t_level_)) == 0) ? GREEN
                                                                                                               : RED,

                 dep_market_view_.ask_int_price(t_level_), dep_market_view_.ask_price(t_level_),
                 dep_market_view_.ask_order(t_level_), dep_market_view_.ask_size(t_level_),
                 dep_market_view_.ask_int_price_level(t_level_),

                 GREEN);
        } else {
          printf("%s%2s %5s %3s %10s %7s%s X %s%7d %10.7f %3d %5d %2d                 \n%s",
                 (order_manager_.GetTotalBidSizeOrderedAtIntPx(dep_market_view_.bid_int_price(t_level_)) == 0) ? GREEN
                                                                                                               : RED,

                 "-", "-", "-", "-", "-",

                 GREEN,
                 (order_manager_.GetTotalAskSizeOrderedAtIntPx(dep_market_view_.ask_int_price(t_level_)) == 0) ? GREEN
                                                                                                               : RED,

                 dep_market_view_.ask_int_price(t_level_), dep_market_view_.ask_price(t_level_),
                 dep_market_view_.ask_order(t_level_), dep_market_view_.ask_size(t_level_),
                 dep_market_view_.ask_int_price_level(t_level_),

                 GREEN);
        }
      }
    }
  }

  std::cout << std::endl;
  for (auto i = 0u; i < last_5_trade_prices_.size(); ++i) {
    std::cout << last_5_trade_prices_[i] << "              " << std::endl;
  }

  printf("\033[14;1H");  // move to 14 th line

  std::cout << "\n=================================================================" << std::endl;

  std::cout << order_manager_.OMToString()
            << "=================================================================" << std::endl;

  printf("\033[20;1H");  // move to 20 th line

  std::cout << "\n=================================================================" << std::endl;
  printf(" avg-open-price: %8.2f pos: %2d             \n", order_manager_.base_pnl().AverageOpenPrice(), my_position_);
  printf(" opnl:         %10.2f pnl: %10.2f \n", order_manager_.base_pnl().opentrade_unrealized_pnl(),
         order_manager_.base_pnl().total_pnl());
  std::cout << " trade_i_quote :       " << (dep_market_view_.trade_update_implied_quote() ? "T" : "F")
            << "                     " << std::endl;
  printf(" target_price_ : %8.2f best_nonself_ : %2d %8.2f X %8.2f %2d                \n", target_price_,
         best_nonself_bid_size_, best_nonself_bid_price_, best_nonself_ask_price_, best_nonself_ask_size_);
  printf(" tp-bbp : %5.2lf l1bp : %5.2lf l1bk : %5.2lf              \n", (target_price_ - best_nonself_bid_price_),
         current_tradevarset_.l1bid_place_, current_tradevarset_.l1bid_keep_);
  printf(" tp-bap : %5.2lf l1ap : %5.2lf l1ak : %5.2lf              \n", -(target_price_ - best_nonself_ask_price_),
         current_tradevarset_.l1ask_place_, current_tradevarset_.l1ask_keep_);
  std::cout << " targetbias_numbers_ : " << targetbias_numbers_
            << " signalbias :          " << (targetbias_numbers_ / dep_market_view_.min_price_increment())
            << "                     " << std::endl;
  std::cout << "=================================================================" << std::endl;

  std::cout << order_manager_.base_pnl().ToString() << std::endl;
}

void BaseTrading::SendTradeAndLog(double _px_, int _int_px_, int _size_, TradeType_t _buysell_, char _level_indicator_,
                                  const std::string &_logging_str_) {
  order_manager_.SendTrade(_px_, _int_px_, _size_, _buysell_, _level_indicator_);
  if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
    DBGLOG_TIME_CLASS_FUNC << _logging_str_ << " " << GetTradeTypeChar(_buysell_) << " " << dep_market_view_.shortcode()
                           << " of " << _size_ << " @ " << _px_ << " IntPx: " << _int_px_
                           << " mkt: " << best_nonself_bid_size_ << " @ " << best_nonself_bid_price_ << " X "
                           << best_nonself_ask_price_ << " @ " << best_nonself_ask_size_
                           << " SumBid: " << order_manager_.SumBidSizes() << " SumAsk: " << order_manager_.SumAskSizes()
                           << DBGLOG_ENDL_FLUSH;
  }
}

void BaseTrading::SMVOnReady() {
  if (dep_market_view_.is_ready_complex(2)) {
    if (position_to_add_at_start_ != 0) {
      double t_exec_price_ =
          position_to_add_at_start_ > 0 ? dep_market_view_.bestbid_price() : dep_market_view_.bestask_price();
      char hostname_[128];
      hostname_[127] = '\0';
      gethostname(hostname_, 127);
      std::ostringstream t_oss_;
      t_oss_ << "Strategy: " << runtime_id_ << " addposition message: Book now ready for product "
             << dep_market_view_.shortcode() << " = " << dep_market_view_.secname() << " on " << hostname_
             << " : Adding " << position_to_add_at_start_ << " Positions \n";
      std::string getflat_email_string_ = t_oss_.str();

      SendMail(getflat_email_string_, getflat_email_string_);
      order_manager_.AddPosition(position_to_add_at_start_, t_exec_price_);
    }
    position_to_add_at_start_ = 0;
    // Unsubscribing from on ready listener
    // as now positions are added so no need of this
    VectorUtils::UniqueVectorRemove(dep_market_view_.onready_listeners_, (SecurityMarketViewOnReadyListener *)this);
  }
}
}
