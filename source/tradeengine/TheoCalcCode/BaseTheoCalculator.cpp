#include "tradeengine/TheoCalc/BaseTheoCalculator.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "tradeengine/Executioner/Dimer.hpp"
#include "tradeengine/Executioner/ElectronicEye.hpp"
#include "tradeengine/Executioner/Improver.hpp"
#include "tradeengine/Executioner/ModifyEye.hpp"
#include "tradeengine/Executioner/MultiQuoter.hpp"
#include "tradeengine/Executioner/Quoter.hpp"
#include "tradeengine/Executioner/RushQuoter.hpp"
#include "tradeengine/TheoCalc/SquareOffTheoCalculator.hpp"
#include "tradeengine/Utils/Parser.hpp"

BaseTheoCalculator::BaseTheoCalculator(std::map<std::string, std::string>* _key_val_map, HFSAT::Watch& _watch_,
                                       HFSAT::DebugLogger& _dbglogger_, int _trading_start_utc_mfm_,
                                       int _trading_end_utc_mfm_, int _aggressive_get_flat_mfm_,
                                       int _eff_squareoff_start_utc_mfm_ = 0, double _bid_multiplier_ = 1,
                                       double _ask_multiplier_ = 1)
    : key_val_map_(_key_val_map),
      watch_(_watch_),
      dbglogger_(_dbglogger_),
      config_reloaded_(false),
      size_offset_reloaded_(false),
      basic_om_(NULL),
      custom_price_aggregator_(NULL),
      trading_start_utc_mfm_(_trading_start_utc_mfm_),
      trading_end_utc_mfm_(_trading_end_utc_mfm_),
      aggressive_get_flat_mfm_(_aggressive_get_flat_mfm_),
      eff_squareoff_start_utc_mfm_(_eff_squareoff_start_utc_mfm_),
      bid_multiplier_(_bid_multiplier_),
      ask_multiplier_(_ask_multiplier_),
      bid_factor_(1),
      ask_factor_(1),
      runtime_id_(0),
      is_ready_(false),
      num_trades_(0),
      position_(0),
      current_position_(0),
      target_position_(0),
      total_pnl_(0),
      total_traded_qty_(0),
      total_traded_value_(0),
      strat_ltp_(0),
      reference_primary_bid_(0),
      reference_primary_ask_(0),
      use_position_shift_manager_(true),
      position_shift_amount_(0),
      bid_increase_shift_(0),
      bid_decrease_shift_(0),
      ask_increase_shift_(0),
      ask_decrease_shift_(0),
      bid_theo_shifts_(0),
      ask_theo_shifts_(0),
      primary0_size_filter_(0),
      primary0_bid_size_filter_(0),
      primary0_ask_size_filter_(0),
      primary0_size_max_depth_(0),
      initial_px_(0),
      min_level_clear_for_big_trade_(2),
      big_trade_int_ltp_(0),
      use_delta_pos_to_shift_(true),
      need_to_hedge_(false),
      eff_squareoff_on_(false),
      is_banned_prod_(false),
      use_banned_scaling_(false),
      trade_banned_prod_(true),
      banned_prod_offset_multiplier_(1),
      banned_prod_size_multiplier_(1),
      pnl_offset_multiplier_(1),
      pnl_size_multiplier_(1),
      vol_offset_multiplier_(1),
      secondary_smv_(NULL),
      primary0_smv_(NULL),
      sqoff_theo_(NULL),
      status_mask_(0xFF3F),  // Config status and control message status not set
      bid_percentage_offset_(0),
      ask_percentage_offset_(0),
      bid_offset_(0),
      ask_offset_(0),
      start_trading_(false),
      is_agressive_getflat_(false),
      hit_stoploss_(false),
      hit_hard_stoploss_(false),
      primary_book_valid_(false),
      secondary_book_valid_(false),
      spread_check_percent_(1),
      inverse_tick_size_primary_(1),
      avg_spread_percent_(1),
      avg_int_spread_(100000),
      inv_avg_int_spread_(0.00001),
      long_ema_int_spread_(100000),
      short_ema_int_spread_(100000),
      long_ema_factor_spread_(0),
      short_ema_factor_spread_(1),
      total_offset_in_avg_spread_(100),
      slope_obb_offset_mult_(0.00001),
      intercept_obb_offset_mult_(1),
      obb_offset_mult_(1),
      use_spread_facor_in_obb_(false),
      stop_loss_(0),
      delta_pos_limit_(0),
      is_modify_before_confirmation_(false),
      is_cancellable_before_confirmation_(false),
      is_secondary_sqoff_needed_(false),
      parent_mm_theo_(NULL),
      sim_base_pnl_(NULL),
      position_to_offset_(0),
      remaining_pos_to_close_(0),
      close_all_positions_(false),
      mkt_percent_limit_(0),
      use_pnl_scaling_(false),
      pnl_threshold_to_scale_up_(1000000),
      pnl_threshold_to_scale_down_(-100000),
      pnl_offset_scaling_step_factor_(1),
      pnl_size_scaling_step_factor_(1),
      pnl_offset_multiplier_limit_(1),
      pnl_inv_offset_multiplier_limit_(1),
      pnl_size_multiplier_limit_(1),
      pnl_inv_size_multiplier_limit_(1),
      pnl_sample_window_size_msecs_(300000),
      msecs_for_next_pnl_sample_start_(_trading_start_utc_mfm_ + 300000),
      pnl_in_current_sample_(0),
      pnl_last_sample_end_(0),
      max_pnl_from_last_down_(0),
      use_vol_scaling_(false),
      use_historic_vol_(false),
      vol_threshold_to_scale_up_(1000000),
      vol_threshold_to_scale_down_(-100000),
      vol_offset_scaling_step_factor_(1),
      vol_offset_multiplier_limit_(1),
      vol_inv_offset_multiplier_limit_(1),
      outside_position_shift_amount_(false),
      vol_sample_window_size_msecs_(300000),
      msecs_for_next_vol_sample_start_(_trading_start_utc_mfm_ + 300000),
      mean_vol_in_current_sample_(0),
      std_vol_in_current_sample_(0),
      std_vol_in_previous_sample_(0),
      vol_int_first_trade_(0),
      num_vol_entries_(0),
      num_long_term_vol_entries_(0),
      vol_ema_(0),
      vol_ema_up_threshold_(100),
      // vol_ema_down_threshold_(1),
      vol_ema_factor_(0),
      msecs_for_vol_start_(_trading_start_utc_mfm_),
      is_historic_vol_available_(false),
      // volume_in_current_sample_(0),
      // volume_in_open_sample_(0),
      listen_big_trades_(false),
      max_int_spread_for_mid_price_(100000),
      is_primary_last_close_valid_(false),
      primary_last_close_int_price_(0.0),
      is_secondary_last_close_valid_(false),
      secondary_last_close_int_price_(0.0),
      are_we_using_auto_getflat_near_circuit_(false),
      are_we_using_agg_auto_getflat_near_circuit_(false),
      auto_getflat_near_circuit_factor_(1.0),
      is_squaredoff_due_to_autogetflat_near_primary_circuit_(false),
      is_squaredoff_due_to_autogetflat_near_secondary_circuit_(false),
      is_theoturnoff_due_to_autogetflat_near_primary_circuit_(false),
      is_theoturnoff_due_to_autogetflat_near_secondary_circuit_(false),
      ipo_settlement_price_(0.0),
      ipo_circuit_(0.0),
      //by default this will be ineffective with 0 values
      primary_int_px_value_for_autogetflat_(0),
      secondary_int_px_value_for_autogetflat_(0),
      primary_int_px_value_for_autoresume_(0),
      secondary_int_px_value_for_autoresume_(0),
      DEP_ONE_LOT_SIZE_(1),
      INDEP_ONE_LOT_SIZE_(1),
      using_grt_control_file_(false),
      grt_control_key_map_(NULL),
      sec_name_indexer_(HFSAT::SecurityNameIndexer::GetUniqueInstance()) {

  grt_control_key_map_ = new std::map<std::string, std::string>();      

  // Initializing theo_values_ struct for each Theo
  theo_values_.is_valid_ = false;
  theo_values_.is_big_trade_ = 0;
  theo_values_.theo_bid_price_ = 0;
  theo_values_.theo_ask_price_ = 0;
  theo_values_.position_to_offset_ = 0;
  theo_values_.movement_indicator_ = pNEUTRAL;
  theo_values_.last_traded_int_price_ = 0;
  theo_values_.sweep_mode_active_ = 0;
  theo_values_.primary_best_bid_price_ = 0;
  theo_values_.primary_best_ask_price_ = 0;
  theo_values_.reference_primary_bid_ = 0;
  theo_values_.reference_primary_ask_ = 0;
  theo_values_.is_primary_update_ = false;

  InitializeSecurityID();
  std::vector<std::string> tokens1_;
  std::stringstream ss(secondary_smv_->shortcode());
  std::string item;
  while (std::getline(ss, item, '_')) {
    tokens1_.push_back(item);
  }
  ticker_name_ = tokens1_[1];
  tick_size_ = secondary_smv_->min_price_increment_;
  exec_log_buffer_.content_type_ = HFSAT::CDef::QueryExec;

  //get one lot size value
  if (NULL != secondary_smv_ && HFSAT::SecurityDefinitions::CheckIfContractSpecExists(secondary_smv_->shortcode(), watch_.YYYYMMDD())){
    DEP_ONE_LOT_SIZE_ = HFSAT::SecurityDefinitions::GetContractMinOrderSize(secondary_smv_->shortcode(), watch_.YYYYMMDD());
    DBGLOG_CLASS_FUNC_LINE_INFO << "ONE_LOT_SIZE : " << secondary_smv_->shortcode() << " DATE : " << watch_.YYYYMMDD() << " " << DEP_ONE_LOT_SIZE_ << DBGLOG_ENDL_FLUSH ;
    DBGLOG_CLASS_FUNC_LINE_INFO << "SECONDARY : " << secondary_smv_->shortcode() << " LAST_CLOSE : " << HFSAT::SecurityDefinitions::GetClosePriceFromShortCode(secondary_smv_->shortcode()) << DBGLOG_ENDL_FLUSH ;
    secondary_last_close_int_price_ = secondary_smv_->GetIntPx(HFSAT::SecurityDefinitions::GetClosePriceFromShortCode(secondary_smv_->shortcode()));
    if( secondary_last_close_int_price_ > 0 ){
      is_secondary_last_close_valid_ = true;
    }
  }

  if (NULL != primary0_smv_ && HFSAT::SecurityDefinitions::CheckIfContractSpecExists(primary0_smv_->shortcode(), watch_.YYYYMMDD())){
    INDEP_ONE_LOT_SIZE_ = HFSAT::SecurityDefinitions::GetContractMinOrderSize(primary0_smv_->shortcode(), watch_.YYYYMMDD());
    DBGLOG_CLASS_FUNC_LINE_INFO << "ONE_LOT_SIZE : " << primary0_smv_->shortcode() << " DATE : " << watch_.YYYYMMDD() << " " << INDEP_ONE_LOT_SIZE_ << DBGLOG_ENDL_FLUSH ;
    DBGLOG_CLASS_FUNC_LINE_INFO << "PRIMARY : " << primary0_smv_->shortcode() << " LAST_CLOSE : " << HFSAT::SecurityDefinitions::GetClosePriceFromShortCode(primary0_smv_->shortcode()) << DBGLOG_ENDL_FLUSH ;
    primary_last_close_int_price_ = primary0_smv_->GetIntPx(HFSAT::SecurityDefinitions::GetClosePriceFromShortCode(primary0_smv_->shortcode()));
    if( primary_last_close_int_price_ > 0 ){
      is_primary_last_close_valid_ = true;
    }
  }

}

BaseTheoCalculator::~BaseTheoCalculator() { delete key_val_map_; }

void BaseTheoCalculator::LoadParams() {
  theo_identifier_ = Parser::GetString(key_val_map_, "THEO_IDENTIFIER", "");
  is_modify_before_confirmation_ = Parser::GetBool(key_val_map_, "ONFLY_MODIFY", false);
  is_cancellable_before_confirmation_ = Parser::GetBool(key_val_map_, "ONFLY_CANCEL", false);
  is_secondary_sqoff_needed_ = Parser::GetBool(key_val_map_, "IS_SECONDARY_SQOFF_NEEDED", false);
  close_all_positions_ = Parser::GetBool(key_val_map_, "CLOSE_ALL_POSITIONS", false);
  listen_big_trades_ = Parser::GetBool(key_val_map_, "LISTEN_BIG_TRADES", false);
  min_level_clear_for_big_trade_ = Parser::GetInt(key_val_map_, "MIN_LEVEL_CLEAR_BIG_TRADE", 2);
  use_delta_pos_to_shift_ = Parser::GetBool(key_val_map_, "USE_DELTA_POS_TO_SHIFT", true);
  max_int_spread_for_mid_price_ = Parser::GetInt(key_val_map_, "MAX_INT_SPREAD_FOR_MID_PRICE", 100000);

  is_banned_prod_ = HFSAT::SecurityDefinitions::IsBannedProduct(ticker_name_);
  use_banned_scaling_ = Parser::GetBool(key_val_map_, "USE_BANNED_SCALING", false);
  trade_banned_prod_ = Parser::GetBool(key_val_map_, "TRADE_BANNED_PROD", true);
  banned_prod_offset_multiplier_ = Parser::GetDouble(key_val_map_, "BANNED_PROD_OFFSET_MULTIPLIER", 1);
  banned_prod_size_multiplier_ = Parser::GetDouble(key_val_map_, "BANNED_PROD_SIZE_MULTIPLIER", 1);
  if (is_banned_prod_ && !trade_banned_prod_) {
    TurnOffTheo(BANNED_STATUS_UNSET);
  } else {
    TurnOnTheo(BANNED_STATUS_SET);
    if (is_banned_prod_ && banned_prod_size_multiplier_ > 1) {
      dbglogger_ << watch_.tv() << " " << sec_name_indexer_.GetShortcodeFromId(secondary_id_)
                 << " Banned prod size multiplier greater than 1" << DBGLOG_ENDL_FLUSH;
      banned_prod_size_multiplier_ = 1;
    }

    if (is_banned_prod_ && banned_prod_offset_multiplier_ < 1) {
      dbglogger_ << watch_.tv() << " " << sec_name_indexer_.GetShortcodeFromId(secondary_id_)
                 << " Banned prod offset multiplier less than 1" << DBGLOG_ENDL_FLUSH;
      banned_prod_offset_multiplier_ = 1;
    }
  }

  are_we_using_auto_getflat_near_circuit_ = Parser::GetBool(key_val_map_, "USE_AUTO_GETFLAT_NEAR_CIRCUIT", false);
  are_we_using_agg_auto_getflat_near_circuit_ = Parser::GetBool(key_val_map_, "USE_AGGRESSIVE_AUTO_GETFLAT_NEAR_CIRCUIT", false);
  // using best bid/ ask price instead of mid price so factor need to be reduced
  auto_getflat_near_circuit_factor_ = Parser::GetDouble(key_val_map_, "AUTO_GETFLAT_NEAR_CIRCUIT_FACTOR", 0.75); 

  // IPO specific params
  ipo_settlement_price_ = Parser::GetDouble(key_val_map_, "IPO_PRICE", 0);
  ipo_circuit_ = Parser::GetDouble(key_val_map_, "IPO_CIRCUIT", 20.0)/100;

  //update the effective value offsets for autogetflat 
  if(true == is_primary_last_close_valid_){
    primary_int_px_value_for_autogetflat_ = 0.01 * primary_last_close_int_price_ * auto_getflat_near_circuit_factor_;
    primary_int_px_value_for_autoresume_ = 0.02 * primary_last_close_int_price_ * auto_getflat_near_circuit_factor_;
  }

  if(true == is_secondary_last_close_valid_){
    secondary_int_px_value_for_autogetflat_ = 0.01 * secondary_last_close_int_price_ * auto_getflat_near_circuit_factor_;
    secondary_int_px_value_for_autoresume_ = 0.02 * secondary_last_close_int_price_ * auto_getflat_near_circuit_factor_;
  }

  if(ipo_settlement_price_ > 0){
    primary_int_px_value_for_autogetflat_ = 0.01 * ipo_settlement_price_ * auto_getflat_near_circuit_factor_ / tick_size_;
    primary_int_px_value_for_autoresume_ = 0.02 * ipo_settlement_price_ * auto_getflat_near_circuit_factor_ / tick_size_;
    secondary_int_px_value_for_autogetflat_ = 0.01 * ipo_settlement_price_ * auto_getflat_near_circuit_factor_ / tick_size_;
    secondary_int_px_value_for_autoresume_ = 0.02 * ipo_settlement_price_ * auto_getflat_near_circuit_factor_ / tick_size_;
  }

  bid_percentage_offset_ = Parser::GetDouble(key_val_map_, "BID_PERCENTAGE_OFFSET", 1) / 100;
  ask_percentage_offset_ = Parser::GetDouble(key_val_map_, "ASK_PERCENTAGE_OFFSET", 1) / 100;
  use_position_shift_manager_ = Parser::GetBool(key_val_map_, "USE_POSITION_SHIFT_MANAGER", true);
  position_shift_amount_ = Parser::GetInt(key_val_map_, "POSITION_SHIFT_AMOUNT", 0, DEP_ONE_LOT_SIZE_, INDEP_ONE_LOT_SIZE_);

  bid_increase_shift_percent_ = Parser::GetDouble(key_val_map_, "BID_INCREASE_SHIFT_PERCENT", 0) / 100;
  bid_decrease_shift_percent_ = Parser::GetDouble(key_val_map_, "BID_DECREASE_SHIFT_PERCENT", 0) / 100;
  ask_increase_shift_percent_ = Parser::GetDouble(key_val_map_, "ASK_INCREASE_SHIFT_PERCENT", 0) / 100;
  ask_decrease_shift_percent_ = Parser::GetDouble(key_val_map_, "ASK_DECREASE_SHIFT_PERCENT", 0) / 100;

  bid_increase_max_shift_ = Parser::GetInt(key_val_map_, "BID_INCREASE_MAX_SHIFT", 0);
  bid_decrease_max_shift_ = Parser::GetInt(key_val_map_, "BID_DECREASE_MAX_SHIFT", 0);
  ask_increase_max_shift_ = Parser::GetInt(key_val_map_, "ASK_INCREASE_MAX_SHIFT", 0);
  ask_decrease_max_shift_ = Parser::GetInt(key_val_map_, "ASK_DECREASE_MAX_SHIFT", 0);
  spread_check_percent_ = Parser::GetDouble(key_val_map_, "SPREAD_CHECK_PERCENT", 2) / 100;
  avg_spread_percent_ = Parser::GetDouble(key_val_map_, "AVG_SPREAD_PERCENT", 0) / 100;
  use_spread_facor_in_obb_ = Parser::GetBool(key_val_map_, "USE_SPREAD_FACTOR_OBB", false);
  long_ema_factor_spread_ = Parser::GetDouble(key_val_map_, "LONG_EMA_FACTOR_SPREAD", 0.95);
  short_ema_factor_spread_ = Parser::GetDouble(key_val_map_, "SHORT_EMA_FACTOR_SPREAD", 0.05);

  mkt_percent_limit_ = Parser::GetDouble(key_val_map_, "MKT_PERCENT_LIMIT", 10);

  use_pnl_scaling_ = Parser::GetBool(key_val_map_, "USE_PNL_SCALING", false);
  pnl_sample_window_size_msecs_ = Parser::GetInt(key_val_map_, "PNL_SAMPLE_WINDOW_SIZE_MSECS", 300000);
  pnl_threshold_to_scale_up_ = Parser::GetInt(key_val_map_, "PNL_THRESH_TO_SCALE_UP", 100000);
  pnl_threshold_to_scale_down_ = Parser::GetInt(key_val_map_, "PNL_THRESH_TO_SCALE_DOWN", -100000);
  pnl_offset_scaling_step_factor_ = Parser::GetDouble(key_val_map_, "PNL_OFFSET_REDUCE_STEP_FACTOR", 1);
  pnl_size_scaling_step_factor_ = Parser::GetDouble(key_val_map_, "PNL_SIZE_SCALING_STEP_FACTOR", 1);
  pnl_offset_multiplier_limit_ = Parser::GetDouble(key_val_map_, "PNL_OFFSET_MULTIPLIER_LIMIT", 1);
  pnl_inv_offset_multiplier_limit_ = Parser::GetDouble(key_val_map_, "PNL_INV_OFFSET_MULTIPLIER_LIMIT", 1);
  pnl_size_multiplier_limit_ = Parser::GetDouble(key_val_map_, "PNL_SIZE_MULTIPLIER_LIMIT", 1);
  pnl_inv_size_multiplier_limit_ = 1;

  use_vol_scaling_ = Parser::GetBool(key_val_map_, "USE_VOL_SCALING", false);
  use_historic_vol_ = Parser::GetBool(key_val_map_, "USE_HISTORIC_VOL", false);
  vol_sample_window_size_msecs_ = Parser::GetInt(key_val_map_, "VOL_SAMPLE_WINDOW_SIZE_MSECS", 300000);
  vol_threshold_to_scale_up_ = Parser::GetDouble(key_val_map_, "VOL_THRESH_TO_SCALE_UP", 100000);
  vol_threshold_to_scale_down_ = Parser::GetDouble(key_val_map_, "VOL_THRESH_TO_SCALE_DOWN", -100000);
  // vol_threshold_to_scale_up_ = 4;
  // vol_threshold_to_scale_down_ = 0.75;
  vol_offset_scaling_step_factor_ = Parser::GetDouble(key_val_map_, "VOL_OFFSET_SCALING_STEP_FACTOR", 1);
  vol_ema_factor_ = Parser::GetDouble(key_val_map_, "VOL_EMA_FACTOR", 0.95);
  vol_ema_up_threshold_ = Parser::GetInt(key_val_map_, "VOL_EMA_UP_THRESHOLD", 9);
  // vol_ema_down_threshold_ = Parser::GetInt(key_val_map_, "EMA_DOWN_THRESHOLD", 3);
  vol_offset_multiplier_limit_ = Parser::GetDouble(key_val_map_, "VOL_UP_OFFSET_MULTIPLIER_LIMIT", 1);
  vol_inv_offset_multiplier_limit_ = Parser::GetDouble(key_val_map_, "VOL_DOWN_OFFSET_MULTIPLIER_LIMIT", 1);

  if (initial_px_) {
    initial_bid_increase_shift_ = bid_increase_shift_percent_ * initial_px_;
    initial_bid_decrease_shift_ = bid_decrease_shift_percent_ * initial_px_;
    initial_ask_increase_shift_ = ask_increase_shift_percent_ * initial_px_;
    initial_ask_decrease_shift_ = ask_decrease_shift_percent_ * initial_px_;
    initial_bid_offset_ = bid_percentage_offset_ * initial_px_;
    initial_ask_offset_ = ask_percentage_offset_ * initial_px_;
  }
  if (!is_ready_) {
    msecs_for_next_pnl_sample_start_ = trading_start_utc_mfm_ + pnl_sample_window_size_msecs_;
    std::string vol_start_time_ = Parser::GetString(key_val_map_, "VOL_START", "IST_920");

    int vol_start_hhmm_ = HFSAT::DateTime::GetUTCHHMMFromTZHHMM(watch_.YYYYMMDD(), atoi(vol_start_time_.c_str() + 4),
                                                                vol_start_time_.c_str());
    int init_trading_time_ = msecs_for_vol_start_;
    msecs_for_vol_start_ =
        HFSAT::DateTime::GetUTCMsecsFromMidnightFromTZHHMM(watch_.YYYYMMDD(), vol_start_hhmm_, "UTC_");
    if (msecs_for_vol_start_ < init_trading_time_) {
      msecs_for_vol_start_ = init_trading_time_;
    }
    msecs_for_next_vol_sample_start_ = msecs_for_vol_start_ + vol_sample_window_size_msecs_;
  }

  UpdateSizesAndOffsets();
}

void BaseTheoCalculator::UpdateSizesAndOffsets() {
  double total_offset_multiplier_ = 1;
  double total_size_multiplier_ = 1;
  outside_position_shift_amount_ = false;

  // pnl offset size adjustments
  if (use_pnl_scaling_) {
    total_offset_multiplier_ *= pnl_offset_multiplier_;
    total_size_multiplier_ *= pnl_size_multiplier_;
  } else {
    pnl_offset_multiplier_ = 1;
    pnl_size_multiplier_ = 1;
    msecs_for_next_pnl_sample_start_ = 0;
  }

  total_size_multiplier_ =
      std::max(std::min(total_size_multiplier_, pnl_size_multiplier_limit_), pnl_inv_size_multiplier_limit_);
  total_offset_multiplier_ =
      std::max(std::min(total_offset_multiplier_, pnl_offset_multiplier_limit_), pnl_inv_offset_multiplier_limit_);

  // vol offset size adjustments
  if (use_vol_scaling_) {
    total_offset_multiplier_ *= vol_offset_multiplier_;
  } else {
    vol_offset_multiplier_ = 1;
  }
  total_offset_multiplier_ =
      std::max(std::min(total_offset_multiplier_, vol_offset_multiplier_limit_ * pnl_offset_multiplier_limit_),
               vol_inv_offset_multiplier_limit_ * pnl_inv_offset_multiplier_limit_);

  if (is_banned_prod_ && use_banned_scaling_) {
    total_offset_multiplier_ *= banned_prod_offset_multiplier_;
    total_size_multiplier_ *= banned_prod_size_multiplier_;
  }
  // check if order manager allows size change
  bool om_allowed_ = false;
  if (basic_om_) {
    om_allowed_ = basic_om_->SetSizeMultiplier(total_size_multiplier_);
    if (om_allowed_) {
      for (auto base_exec : base_exec_vec_) {
        base_exec->SetSizeMultiplier(total_size_multiplier_);
      }

      pnl_size_multiplier_ =
          std::max(std::min(pnl_size_multiplier_, pnl_size_multiplier_limit_), pnl_inv_size_multiplier_limit_);
    }
  }
  if (!om_allowed_) {
    pnl_size_multiplier_ /= pnl_size_scaling_step_factor_;
  }

  // scale corresponding offsets
  if (initial_px_) {
    bid_offset_ = initial_bid_offset_ * total_offset_multiplier_;
    ask_offset_ = initial_ask_offset_ * total_offset_multiplier_;

    bid_increase_shift_ = initial_bid_increase_shift_ * total_offset_multiplier_;
    bid_decrease_shift_ = initial_bid_decrease_shift_ * total_offset_multiplier_;
    ask_increase_shift_ = initial_ask_increase_shift_ * total_offset_multiplier_;
    ask_decrease_shift_ = initial_ask_decrease_shift_ * total_offset_multiplier_;
  }
  // revert one side if abs(position) is > position shift amount
  if (use_vol_scaling_ && position_ > position_shift_amount_) {
    ask_offset_ /= vol_offset_multiplier_;
    ask_increase_shift_ /= vol_offset_multiplier_;
    ask_decrease_shift_ /= vol_offset_multiplier_;
    outside_position_shift_amount_ = true;
  } else if (use_vol_scaling_ && position_ < -1 * position_shift_amount_) {
    bid_offset_ /= vol_offset_multiplier_;
    bid_increase_shift_ /= vol_offset_multiplier_;
    bid_decrease_shift_ /= vol_offset_multiplier_;
    outside_position_shift_amount_ = true;
  }
  SetPositionOffsets(position_);

  //   dbglogger_ << watch_.tv() << " " << theo_identifier_
  // << " Offsets:" << bid_offset_ << "|" << ask_offset_
  // << "|" << bid_increase_shift_ << "|" << bid_decrease_shift_
  // << "|" << ask_increase_shift_ << "|" << ask_decrease_shift_
  // << "|" << initial_bid_offset_ << "|" << total_offset_multiplier_
  // << "|" << total_size_multiplier_ << "|" << initial_px_
  // << "\n";
}

void BaseTheoCalculator::printStats() {
  std::cout << ticker_name_ << " " << stop_loss_ << " " << hard_stop_loss_ << std::endl;
}

void BaseTheoCalculator::ReloadConfig() {
  std::string theo_cal = (*key_val_map_)["THEO_FOLDER"] + std::string("MainConfig.cfg");
  Parser::ParseConfig(theo_cal, *key_val_map_);
  double prev_stop_loss_ = stop_loss_;
  double prev_hard_stop_loss_ = hard_stop_loss_;
  bid_factor_ = 1;
  ask_factor_ = 1;
  SetEfficientSquareOff(false);
  LoadParams();
  if ((hit_hard_stoploss_) && ((hard_stop_loss_ != prev_hard_stop_loss_) && (total_pnl_ > hard_stop_loss_))) {
    // If loss check is set, but stop loss is now not reached
    if ((status_mask_ & HARDLOSS_STATUS_SET) == 0) {
      dbglogger_ << watch_.tv() << " RESETTING HARD STOP LOSS STATUS "
                 << sec_name_indexer_.GetShortcodeFromId(secondary_id_) << " PNL: " << total_pnl_
                 << " Hard SL: " << hard_stop_loss_ << " secmkt[" << secondary_smv_->market_update_info().bestbid_price_
                 << " x " << secondary_smv_->market_update_info().bestask_price_ << "] primkt["
                 << primary0_smv_->market_update_info().bestbid_price_ << " x "
                 << primary0_smv_->market_update_info().bestask_price_ << "] " << DBGLOG_ENDL_FLUSH;
      hit_hard_stoploss_ = false;
      TurnOnTheo(HARDLOSS_STATUS_SET);
      if (sqoff_theo_) {
        sqoff_theo_->TurnOnTheo(HARDLOSS_STATUS_SET);
      }
      if (parent_mm_theo_) {
        parent_mm_theo_->TurnOnTheo(HARDLOSS_STATUS_SET);
      }
    }
  }

  if ((hit_stoploss_) && ((stop_loss_ != prev_stop_loss_) && (total_pnl_ > stop_loss_))) {
    // If loss check is set, but stop loss is now not reached
    if ((status_mask_ & LOSS_STATUS_SET) == 0) {
      dbglogger_ << watch_.tv() << " RESETTING STOP LOSS STATUS " << sec_name_indexer_.GetShortcodeFromId(secondary_id_)
                 << " PNL: " << total_pnl_ << " SL: " << stop_loss_ << " secmkt["
                 << secondary_smv_->market_update_info().bestbid_price_ << " x "
                 << secondary_smv_->market_update_info().bestask_price_ << "] primkt["
                 << primary0_smv_->market_update_info().bestbid_price_ << " x "
                 << primary0_smv_->market_update_info().bestask_price_ << "] " << DBGLOG_ENDL_FLUSH;
      hit_stoploss_ = false;
      TurnOnTheo(LOSS_STATUS_SET);
      NoSquareOff();
      if (parent_mm_theo_) {
        parent_mm_theo_->TurnOnTheo(LOSS_STATUS_SET);
      }
    }
  }
  for (auto base_exec : base_exec_vec_) {
    base_exec->ReloadConfig();
  }

  if ((status_mask_ & CONFIG_STATUS_SET) == 0) {
    TurnOffTheo(CTRLMSG_STATUS_UNSET);
    if (need_to_hedge_) {
      for (uint8_t num = 0; num < hedge_vector_.size(); num++) {
        if (hedge_vector_[num]->hedge_status_) {
          hedge_vector_[num]->hedge_theo_->SquareOff();
        }
      }
      if (is_secondary_sqoff_needed_) {
        SquareOff();
      }
    } else {
      SquareOff();
    }
  } else {
    TurnOnTheo(CTRLMSG_STATUS_SET);
    if (need_to_hedge_) {
      for (uint8_t num = 0; num < hedge_vector_.size(); num++) {
        if (hedge_vector_[num]->hedge_status_) {
          hedge_vector_[num]->hedge_theo_->NoSquareOff();
        }
      }
    } else {
      NoSquareOff();
    }
  }

  config_reloaded_ = true;
}

void BaseTheoCalculator::InitializeSecurityID() {
  uint32_t count = 0;
  std::string primary_symbol = std::string("PRIMARY") + std::to_string(count);
  while (key_val_map_->find(primary_symbol) != key_val_map_->end()) {
    int pr_id = sec_name_indexer_.GetIdFromString((*key_val_map_)[primary_symbol]);
    if (pr_id != -1) {
      if (count == 0) {
        primary0_id_ = pr_id;
        primary_id_.push_back(pr_id);
        primary0_smv_ =
            HFSAT::ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView((*key_val_map_)[primary_symbol]);
        primary_smv_vec_.push_back(primary0_smv_);
        inverse_tick_size_primary_ = 1 / primary0_smv_->min_price_increment_;
      } else {
        primary_id_.push_back(pr_id);
        primary_smv_vec_.push_back(
            HFSAT::ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView((*key_val_map_)[primary_symbol]));
      }
    } else {
      std::cerr << __func__ << " Primary symbol " << (*key_val_map_)[primary_symbol] << " not found in sec indexer "
                << std::endl;
    }
    count++;
    primary_symbol = std::string("PRIMARY") + std::to_string(count);
  }
  secondary_id_ = sec_name_indexer_.GetIdFromString((*key_val_map_)["SECONDARY"]);
  if ((int)secondary_id_ != -1) {
    secondary_smv_ = HFSAT::ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView((*key_val_map_)["SECONDARY"]);
  } else {
    std::cerr << __func__ << " Secondary symbol " << (*key_val_map_)["SECONDARY"] << " not found in sec indexer "
              << std::endl;
  }
}

void BaseTheoCalculator::InitializeDataSubscriptions() {
  for (auto pr_smv : primary_smv_vec_) {
    pr_smv->subscribe_L2(this);
    pr_smv->subscribe_circuit(this);
  }
  secondary_smv_->subscribe_L2(this);
  secondary_smv_->subscribe_circuit(this);
}

SecPriceType_t BaseTheoCalculator::GetPriceTypeFromStr(std::string pstr) {
  if (pstr == "ORDER_BOOK_BEST_PRICE") {
    return pORDER_BOOK_BEST_PRICE;
  } else if (pstr == "VWAP_PRICE") {
    return pVWAP_PRICE;
  } else if (pstr == "VWAP_PRICE_PASSIVE") {
    return pVWAP_PRICE_PASSIVE;
  } else if (pstr == "CUSTOM_PRICE_TYPE") {
    return pCUSTOM_PRICE_TYPE;
  } else if (pstr == "MID_PRICE") {
    return pMID_PRICE;
  }
  return pORDER_BOOK_BEST_PRICE;
}

bool BaseTheoCalculator::CalculatePrices(double& bid_price_, double& ask_price_, HFSAT::SecurityMarketView* smv_,
                                         int vwap_levels, SecPriceType_t price_type, int bid_size_filter,
                                         int ask_size_filter, int max_depth) {
  switch (price_type) {
    case pORDER_BOOK_BEST_PRICE: {
      if (max_depth <= 0) {
        bid_price_ = smv_->market_update_info().bestbid_price_;
        ask_price_ = smv_->market_update_info().bestask_price_;
      } else {
        int sum_bid_size_ = 0;
        int level_count_ = 0;
        HFSAT::MarketUpdateInfoLevelStruct* bid_info_ = NULL;
        while (level_count_ < max_depth) {
          bid_info_ = smv_->bid_info(level_count_);
          if (bid_info_ == NULL) {
            bid_price_ = smv_->market_update_info().bestbid_price_;
            break;
          }
          sum_bid_size_ += bid_info_->limit_size_;
          if (sum_bid_size_ >= bid_size_filter) {
            bid_price_ = bid_info_->limit_price_;
            break;
          }
          ++level_count_;
        }
        if (level_count_ == max_depth) {
          bid_price_ = bid_info_->limit_price_;
        }
        level_count_ = 0;
        int sum_ask_size_ = 0;
        HFSAT::MarketUpdateInfoLevelStruct* ask_info_ = NULL;
        while (level_count_ < max_depth) {
          ask_info_ = smv_->ask_info(level_count_);
          if (ask_info_ == NULL) {
            ask_price_ = smv_->market_update_info().bestask_price_;
            break;
          }
          sum_ask_size_ += ask_info_->limit_size_;
          if (sum_ask_size_ >= ask_size_filter) {
            ask_price_ = ask_info_->limit_price_;
            break;
          }
          ++level_count_;
        }
        if (level_count_ == max_depth) {
          ask_price_ = ask_info_->limit_price_;
        }
        /*dbglogger_ << watch_.tv() << smv_->ShowMarket() << DBGLOG_ENDL_FLUSH;
        dbglogger_ << "filtered price " << bid_price_ << " x " << ask_price_
                << " size filter " << bid_size_filter << " "
                << ask_size_filter << DBGLOG_ENDL_FLUSH;*/
      }
      if (use_spread_facor_in_obb_) {
        int int_spread_ = (ask_price_ - bid_price_) * inverse_tick_size_primary_;
        long_ema_int_spread_ =
            long_ema_factor_spread_ * long_ema_int_spread_ + (1 - long_ema_factor_spread_) * int_spread_;
        short_ema_int_spread_ =
            short_ema_factor_spread_ * short_ema_int_spread_ + (1 - short_ema_factor_spread_) * int_spread_;
        //				double current_spread_in_avg_spread_ = int_spread_*inv_avg_int_spread_;
        double short_ema_spread_in_long_ema_spread_ = short_ema_int_spread_ / long_ema_int_spread_;
        if (short_ema_spread_in_long_ema_spread_ < 1) {
          obb_offset_mult_ = 1;
        } else if (short_ema_spread_in_long_ema_spread_ > (total_offset_in_avg_spread_ + 1)) {
          obb_offset_mult_ = 0;
        } else {
          obb_offset_mult_ =
              intercept_obb_offset_mult_ - (slope_obb_offset_mult_ * short_ema_spread_in_long_ema_spread_);
        }
        // double offset_mult_increase_ = std::min(std::max(1.0,(long_ema_int_spread_*inv_avg_int_spread_)),2.0);
        // obb_offset_mult_ *= offset_mult_increase_;
      }
      return true;
    } break;
    case pVWAP_PRICE: {
      /*dbglogger_ << watch_.tv() << " best bid "
              << smv_->market_update_info().bestbid_price_
              << " ask " << smv_->market_update_info().bestask_price_
              << DBGLOG_ENDL_FLUSH;*/

      double total_bid_size = 0;
      double total_bid_value = 0;
      double total_ask_size = 0;
      double total_ask_value = 0;

      for (int level_count_ = 0; level_count_ < vwap_levels; level_count_++) {
        HFSAT::MarketUpdateInfoLevelStruct* bid_info_ = smv_->bid_info(level_count_);
        HFSAT::MarketUpdateInfoLevelStruct* ask_info_ = smv_->ask_info(level_count_);

        if ((ask_info_ == NULL) || (bid_info_ == NULL)) {
          // Hit bottom of book
          bid_price_ = smv_->market_update_info().bestbid_price_;
          ask_price_ = smv_->market_update_info().bestask_price_;
          return false;
        }

        double _bid_price_ = bid_info_->limit_price_;
        double _ask_price_ = ask_info_->limit_price_;

        int bid_size_ = bid_info_->limit_size_;
        int ask_size_ = ask_info_->limit_size_;

        total_bid_size += bid_size_;
        total_bid_value += (bid_size_ * _bid_price_);
        total_ask_size += ask_size_;
        total_ask_value += (ask_size_ * _ask_price_);
      }

      double avg_bid_px = total_bid_value / total_bid_size;
      double avg_ask_px = total_ask_value / total_ask_size;
      bid_price_ = (avg_bid_px * total_ask_size + avg_ask_px * total_bid_size) / (total_bid_size + total_ask_size);
      ask_price_ = bid_price_;

      /*int factor1 = total_bid_value*total_ask_size*total_ask_size;
      int factor2 = total_ask_value*total_bid_size*total_bid_size;
      int denom = (total_bid_size + total_ask_size)*total_bid_size*total_ask_size;
      bid_price_ = double(factor1 + factor2)/double(denom);
      ask_price_ = bid_price_;*/
      return true;
    } break;
    case pVWAP_PRICE_PASSIVE: {
    } break;
    case pCUSTOM_PRICE_TYPE: {
      custom_price_aggregator_->GetPrice(bid_price_, ask_price_);
    } break;
    case pMID_PRICE: {
      bid_price_ = smv_->market_update_info().bestbid_price_;
      ask_price_ = smv_->market_update_info().bestask_price_;

      if ((bid_price_ == kInvalidPrice) || (ask_price_ == kInvalidPrice)) {
        return false;
      }

      int int_spread_ = smv_->market_update_info().bestask_int_price_ - smv_->market_update_info().bestbid_int_price_;
      double mid_price_ = (bid_price_ + ask_price_) * 0.5;
      if (int_spread_ <= max_int_spread_for_mid_price_) {
        bid_price_ = mid_price_;
        ask_price_ = mid_price_;
      } else {
        bid_price_ = (mid_price_ + bid_price_) * 0.5;
        ask_price_ = (mid_price_ + ask_price_) * 0.5;
      }
      return true;
    } break;
    default:
      break;
  };
  return false;
}

void BaseTheoCalculator::CreateAllExecutioners(HFSAT::BasicOrderManager* _basic_om, bool _livetrading_) {
  basic_om_ = _basic_om;

  if (basic_om_) {
    basic_om_->AddExchangeRejectsListeners(this);
  } else {
    DBGLOG_TIME_CLASS_FUNC << "ERROR! Cant create executioner, OrderMgr not set" << DBGLOG_ENDL_FLUSH;
    return;
  }
  CreateExecutioner("MODIFYEYE", _livetrading_);
  CreateExecutioner("ELECTRONICEYE", _livetrading_);
  CreateExecutioner("MULTIQUOTER", _livetrading_);
  CreateExecutioner("QUOTER", _livetrading_);
  CreateExecutioner("DIMER", _livetrading_);
  CreateExecutioner("IMPROVER", _livetrading_);
  CreateExecutioner("RUSHQUOTER", _livetrading_);
  UpdateSizesAndOffsets();
}

void BaseTheoCalculator::CreateExecutioner(std::string exec_name, bool _livetrading_) {
  uint64_t count = 1;
  std::string exec_num = std::string(exec_name) + std::to_string(count);
  while (key_val_map_->find(exec_num) != key_val_map_->end()) {
    std::string exec_param_file = (*key_val_map_)["THEO_FOLDER"] + (*key_val_map_)[exec_num];
    BaseExecutioner* base_exec = NULL;
    if (exec_name == "ELECTRONICEYE") {
      base_exec = new ElectronicEye(exec_param_file, watch_, dbglogger_, secondary_smv_, basic_om_, _livetrading_,
                                    is_modify_before_confirmation_, is_cancellable_before_confirmation_, theo_values_);
    } else if (exec_name == "QUOTER") {
      base_exec = new Quoter(exec_param_file, watch_, dbglogger_, secondary_smv_, basic_om_, _livetrading_,
                             is_modify_before_confirmation_, is_cancellable_before_confirmation_, theo_values_);
    } else if (exec_name == "DIMER") {
      base_exec = new Dimer(exec_param_file, watch_, dbglogger_, secondary_smv_, basic_om_, _livetrading_,
                            is_modify_before_confirmation_, is_cancellable_before_confirmation_, theo_values_);
    } else if (exec_name == "IMPROVER") {
      base_exec = new Improver(exec_param_file, watch_, dbglogger_, secondary_smv_, basic_om_, _livetrading_,
                               is_modify_before_confirmation_, is_cancellable_before_confirmation_, theo_values_);
    } else if (exec_name == "MULTIQUOTER") {
      base_exec = new MultiQuoter(exec_param_file, watch_, dbglogger_, secondary_smv_, basic_om_, _livetrading_,
                                  is_modify_before_confirmation_, is_cancellable_before_confirmation_, theo_values_);
    } else if (exec_name == "MODIFYEYE") {
      base_exec = new ModifyEye(exec_param_file, watch_, dbglogger_, secondary_smv_, basic_om_, _livetrading_,
                                is_modify_before_confirmation_, is_cancellable_before_confirmation_, theo_values_);
    } else if (exec_name == "RUSHQUOTER") {
      base_exec = new RushQuoter(exec_param_file, watch_, dbglogger_, secondary_smv_, basic_om_, _livetrading_,
                                is_modify_before_confirmation_, is_cancellable_before_confirmation_, theo_values_);
    }
    if (base_exec) {
      base_exec_vec_.push_back(base_exec);
    }
    count++;
    exec_num = std::string(exec_name) + std::to_string(count);
  }
}

void BaseTheoCalculator::PNLStats(HFSAT::BulkFileWriter* trades_writer_, bool dump_to_cout) {
  // Currently exposure is not being used elsewhere so updated only end of run

  if (basic_om_ == NULL) {
    return;
  }
  std::string user_dump_mssg_order = " [ ";
  exposure_ =
      current_position_ *
      (secondary_smv_->market_update_info().bestbid_price_ + secondary_smv_->market_update_info().bestask_price_) * 0.5;
  int volume_ = basic_om_->trade_volume();
  user_dump_mssg_order += GetTotalOrderCountString();
  user_dump_mssg_order += "] ";

  dbglogger_ << watch_.tv() << " SIMRESULT " << secondary_smv_->shortcode() << " "
             << " PNL: " << sim_base_pnl_->total_pnl() << " POS: " << current_position_ << " VOLUME: " << volume_
             << " EXP: " << exposure_
             << " %VOLUME: " << (total_traded_qty_ ? (double)volume_ * 100 / (double)total_traded_qty_ : 0)
             << " MINPNL: " << sim_base_pnl_->min_pnl_till_now() << " TTV: " << total_traded_value_ << " NUM_ORDERS "
             << GetTotalOrderCount() << user_dump_mssg_order << " mkt[" << secondary_smv_->market_update_info().bestbid_price_ << " x "
             << secondary_smv_->market_update_info().bestask_price_ << "] " << DBGLOG_ENDL_FLUSH;

  if (dump_to_cout) {
    std::cout << "SIMRESULT " << secondary_smv_->shortcode() << " " << sim_base_pnl_->total_pnl() << " " << volume_
              << " 0 0 " << (total_traded_qty_ ? (double)volume_ * 100 / (double)total_traded_qty_ : 0) << " 0 "
              << exposure_ << " " << total_traded_value_ << " " << sim_base_pnl_->min_pnl_till_now() << " "
              << GetTotalOrderCount() << DBGLOG_ENDL_FLUSH;
  }

  if (trades_writer_ != nullptr) {
    *trades_writer_ << "SIMRESULT " << secondary_smv_->shortcode() << " " << sim_base_pnl_->total_pnl() << " "
                    << volume_ << " 0 0 " << (total_traded_qty_ ? (double)volume_ * 100 / (double)total_traded_qty_ : 0)
                    << " 0 " << exposure_ << " " << total_traded_value_ << " " << sim_base_pnl_->min_pnl_till_now()
                    << " " << GetTotalOrderCount() << "\n";
  }
}

void BaseTheoCalculator::ConfigureHedgeDetails(std::map<std::string, BaseTheoCalculator*>& theo_map_) {
  HedgeDetails* hedge_detail = new HedgeDetails();
  std::string hedge_symbol_ = Parser::GetString(key_val_map_, "HEDGE1_SYMBOL", "INVALID");
  std::string hedge_theo_identifier_ = Parser::GetString(key_val_map_, "HEDGE1_THEO_IDENTIFIER", "INVALID");
  hedge_detail->hedge_status_ = Parser::GetBool(key_val_map_, "HEDGE1_STATUS", false);
  hedge_detail->hedge_factor_ = Parser::GetDouble(key_val_map_, "HEDGE1_FACTOR", 1);
  if (hedge_detail->hedge_status_) {
    // Find hedge_theo_identifier_ in theo_map_
    if (theo_map_.find(hedge_theo_identifier_) == theo_map_.end()) {
      DBGLOG_TIME_CLASS_FUNC << "HEDGE THEO NOT FOUND (exiting) " << DBGLOG_ENDL_FLUSH;
    }
    hedge_detail->hedge_theo_ = theo_map_[hedge_theo_identifier_];
    hedge_detail->hedge_theo_->SetParentTheo(this);
    // Push these hedge theo in hedge_notifier
  }
  hedge_vector_.push_back(hedge_detail);
  need_to_hedge_ = need_to_hedge_ or hedge_detail->hedge_status_;
}

void BaseTheoCalculator::NotifyHedgeTheoListeners() {
  if (need_to_hedge_) {
    position_to_offset_ = position_;
    for (uint8_t num = 0; num < hedge_vector_.size(); num++) {
      if (hedge_vector_[num]->hedge_status_) {
        double hedge_amount = current_position_ * hedge_vector_[num]->hedge_factor_;
        int hedge_target = (current_position_ > 0) ? (int(hedge_amount + 0.5)) : (int(hedge_amount - 0.5));
        remaining_pos_to_close_ =
            (close_all_positions_)
                ? (current_position_)
                : (current_position_ - hedge_target / hedge_vector_[num]->hedge_factor_ +
                   (hedge_target -
                    HFSAT::MathUtils::GetFlooredMultipleOf(
                        hedge_target, hedge_vector_[num]->hedge_theo_->secondary_smv_->min_order_size_)) /
                       hedge_vector_[num]->hedge_factor_);
        hedge_vector_[num]->hedge_theo_->SetTargetPosition(hedge_target);
        hedge_vector_[num]->hedge_theo_->UpdateTotalRisk();
        position_to_offset_ = hedge_vector_[num]->hedge_factor_ * hedge_vector_[num]->hedge_theo_->GetTotalPosition();
      }
    }
    SetPositionOffsets(position_to_offset_);
  }
}
