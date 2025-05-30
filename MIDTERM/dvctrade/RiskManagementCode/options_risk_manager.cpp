/*
  \file RiskManagementCode/options_risk_manager.cpp

  \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
  Address:
  Suite No 353, Evoma, #14, Bhattarhalli,
  Old Madras Road, Near Garden City College,
  KR Puram, Bangalore 560049, India
  +91 80 4190 3551
 */

#include "dvctrade/RiskManagement/options_risk_manager.hpp"
#include "dvccode/CDef/math_utils.hpp"

namespace HFSAT {

OptionsRiskManager::OptionsRiskManager(DebugLogger& _dbglogger_, const Watch& _watch_,
                                       BaseOptionRiskPremium* _options_risk_premium_,
                                       std::vector<SecurityMarketView*> _underlying_market_view_vec_,
                                       std::vector<SmartOrderManager*> _order_manager_vec_,std::vector<BaseModelMath*> _fut_modelmath_vec_,
                                       std::map<std::string, std::vector<SmartOrderManager*> > _shc_const_som_map_,
                                       std::vector<MultBasePNL*> _mult_base_pnl_vec, MultBasePNL* _total_base_pnl_,
                                       int _trading_start_mfm_, int _trading_end_mfm_, bool _livetrading_,
                                       int _runtime_id_, SecurityNameIndexer& _sec_name_indexer_)
    : watch_(_watch_),
      dbglogger_(_dbglogger_),
      sec_name_indexer_(_sec_name_indexer_),
      options_risk_premium_(_options_risk_premium_),
      underlying_market_view_vec_(_underlying_market_view_vec_),
      order_manager_vec_(_order_manager_vec_),
      fut_modelmath_vec_(_fut_modelmath_vec_),
      mult_base_pnl_vec(_mult_base_pnl_vec),
      total_base_pnl_(_total_base_pnl_),
      pnl_sampling_timestamps_(),
      pnl_samples_(),
      pnl_samples_vec_(),
      sample_index_(0),
      trading_start_utc_mfm_(_trading_start_mfm_),
      trading_end_utc_mfm_(_trading_end_mfm_),
      is_max_loss_reached_global_(false),
      global_max_loss_(0.0),
      total_pnl_(0.0),
      total_pos_(0.0),
      min_pnl_(0.0),
      interest_rate_(NSESecurityDefinitions::GetInterestRate(watch_.YYYYMMDD())),
      getting_flat_(false),
      aggressively_getting_flat_(false),
      livetrading_(_livetrading_),
      runtime_id_(_runtime_id_) {
  num_underlying_ = underlying_market_view_vec_.size();

  paramset_vec_ = options_risk_premium_->paramset_vec_;
  should_be_getting_hedge_.resize(num_underlying_, false);
  should_be_getting_aggressive_hedge_.resize(num_underlying_, false);
  pnl_per_underlying_.resize(num_underlying_, 0.0);
  prev_delta_unhedged_.resize(num_underlying_, 0.0);
  min_pnl_per_underlying_.resize(num_underlying_, 0.0);
  total_delta_per_underlying_.resize(num_underlying_, 0);
  total_gamma_per_underlying_.resize(num_underlying_, 0);
  total_vega_per_underlying_.resize(num_underlying_, 0);
  total_theta_per_underlying_.resize(num_underlying_, 0);
  delta_hedged_.resize(num_underlying_, 0);
  delta_unhedged_.resize(num_underlying_, 0);

  std::map<std::string, std::vector<SecurityMarketView*> > _shc_const_smv_map_ =
      HFSAT::MultModelCreator::GetShcToConstSMVMap();
  security_id_prod_idx_map_ = HFSAT::MultModelCreator::GetSecIdMap();

  closeout_zeropos_tradevarset_ =
      TradeVars_t(HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE,
                  HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE,
                  HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE, 0, 0);

  // we are subscribing to fut_smv/opt_smv/fut_som/opt_som/otp !!

  for (unsigned int idx = 0; idx < underlying_market_view_vec_.size(); idx++) {
    upper_threshold_vec_.push_back(paramset_vec_[idx]->delta_hedge_upper_threshold_);
    lower_threshold_vec_.push_back(paramset_vec_[idx]->delta_hedge_lower_threshold_);

    hist_stdev_.push_back(SampleDataUtil::GetAvgForPeriod(underlying_market_view_vec_[idx]->shortcode(), watch_.YYYYMMDD(), 20,
    		trading_start_utc_mfm_, trading_end_utc_mfm_, "STDEV", false));

    long_tradevarset_vec_.push_back(TradeVars_t(HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE,
    		hist_stdev_[idx]*paramset_vec_[idx]->fut_place_, hist_stdev_[idx]*paramset_vec_[idx]->fut_keep_,
            HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE,
            HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE, 0,underlying_market_view_vec_[idx]->min_order_size_));

    short_tradevarset_vec_.push_back(TradeVars_t(hist_stdev_[idx]*paramset_vec_[idx]->fut_place_, hist_stdev_[idx]*paramset_vec_[idx]->fut_keep_,
    		HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE,
            HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE,
            HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE, underlying_market_view_vec_[idx]->min_order_size_,0));

    pnl_samples_vec_.push_back(std::vector<int>());
    order_manager_vec_[idx]->AddPositionChangeListener(this);
    order_manager_vec_[idx]->AddExecutionListener(this);
    underlying_market_view_vec_[idx]->subscribe_price_type(this, kPriceTypeMidprice);
    underlying_data_vec_.push_back(new OptionVars(dbglogger_, watch_, _underlying_market_view_vec_[idx],
                                                  order_manager_vec_[idx], trading_start_utc_mfm_,
                                                  trading_end_utc_mfm_));

    global_max_loss_ += paramset_vec_[idx]->global_max_loss_;
    options_data_matrix_.push_back(std::vector<OptionVars*>());

    if(paramset_vec_[idx]->delta_hedge_logic_ == 3)
    {
      if(fut_modelmath_vec_[idx] == NULL)
        paramset_vec_[idx]->delta_hedge_logic_ = 1;
      else
        fut_modelmath_vec_[idx]->AddListener(this,idx);
    }

    std::vector<SecurityMarketView*> const_smv_list_ =
        _shc_const_smv_map_[underlying_market_view_vec_[idx]->shortcode()];
    std::vector<SmartOrderManager*> const_som_list_ =
        _shc_const_som_map_[underlying_market_view_vec_[idx]->shortcode()];

    for (unsigned int const_indx_ = 0; const_indx_ < const_smv_list_.size(); const_indx_++) {
      options_data_matrix_[idx].push_back(new OptionVars(dbglogger_, watch_, const_smv_list_[const_indx_],
                                                         underlying_market_view_vec_[idx], const_som_list_[const_indx_],
                                                         trading_start_utc_mfm_, trading_end_utc_mfm_));
      options_data_matrix_[idx][const_indx_]->AddRiskChangeListener(this);

      options_data_matrix_[idx][const_indx_]->smv_->subscribe_price_type(this, kPriceTypeMidprice);
      options_data_matrix_[idx][const_indx_]->som_->AddPositionChangeListener(this);
      options_data_matrix_[idx][const_indx_]->som_->AddExecutionListener(this);
    }
  }

  double global_loss_factor_ = std::max(0.6, 1.1 - underlying_market_view_vec_.size() * 0.1);
  global_max_loss_ *= global_loss_factor_;
  watch_.subscribe_BigTimePeriod(
      this);  // Required so that delta hedge logic is called repeatedly and also checking for staggered geltflat here

  int sampling_interval_msecs_ = HFSAT::ExecLogicUtils::GetSamplingIntervalForPnlSeries("NSE_NIFTY_FUT0");

  int t_sampling_start_utc_mfm_ = MathUtils::GetFlooredMultipleOf(_trading_start_mfm_, sampling_interval_msecs_);
  int t_sampling_end_utc_mfm_ = MathUtils::GetCeilMultipleOf(
      _trading_end_mfm_ + 300000, sampling_interval_msecs_);  // Adding 5 minutes to consider getflat also

  for (int sampling_mfm_ = t_sampling_start_utc_mfm_ + sampling_interval_msecs_;
       sampling_mfm_ <= t_sampling_end_utc_mfm_; sampling_mfm_ += sampling_interval_msecs_) {
    pnl_sampling_timestamps_.push_back(sampling_mfm_);
  }
}

void OptionsRiskManager::InitThrottleManager(std::vector<ThrottleManager*>& t_throttle_manager_vec_) {
  for (auto i = 0u; i < underlying_data_vec_.size(); i++) {
    int size_t = paramset_vec_[i]->throttle_message_limit_;
    ThrottleManager* t_throttle_manager_ = new ThrottleManager(size_t);
    underlying_data_vec_[i]->som_->SetOrderManager(t_throttle_manager_);

    for (unsigned int j = 0; j < options_data_matrix_[i].size(); j++) {
      options_data_matrix_[i][j]->som_->SetOrderManager(t_throttle_manager_);
    }
    t_throttle_manager_->start_throttle_manager(true);
    t_throttle_manager_vec_.push_back(t_throttle_manager_);
  }
}

void OptionsRiskManager::OnOptionRiskChange(unsigned int _security_id_, double change_delta_, double change_gamma_,
                                            double change_vega_, double change_theta_) {
  int _product_index_ = security_id_prod_idx_map_[_security_id_].first;
  int _option_index_ = security_id_prod_idx_map_[_security_id_].second;
  total_delta_per_underlying_[_product_index_] +=
      (options_data_matrix_[_product_index_][_option_index_]->position_) * change_delta_;
  delta_unhedged_[_product_index_] = total_delta_per_underlying_[_product_index_] - delta_hedged_[_product_index_];
  total_gamma_per_underlying_[_product_index_] +=
      (options_data_matrix_[_product_index_][_option_index_]->position_) * change_gamma_;
  total_vega_per_underlying_[_product_index_] +=
      (options_data_matrix_[_product_index_][_option_index_]->position_) * change_vega_;
  total_theta_per_underlying_[_product_index_] +=
      (options_data_matrix_[_product_index_][_option_index_]->position_) * change_theta_;

  options_risk_premium_->OnRiskChange(_product_index_, delta_unhedged_[_product_index_],
                                      total_gamma_per_underlying_[_product_index_],
                                      total_vega_per_underlying_[_product_index_]);

  GetDeltaHedgeLogic(_product_index_);
  CallPlaceCancelNonBestLevels(_product_index_);
}

void OptionsRiskManager::OnMarketUpdate(const unsigned int _security_id_,
                                        const MarketUpdateInfo& _market_update_info_) {
  int option_index_ = security_id_prod_idx_map_[_security_id_].second;
  int future_index_ = security_id_prod_idx_map_[_security_id_].first;

  OptionVars* current_exec_vars_;

  if (option_index_ < 0)
    current_exec_vars_ = underlying_data_vec_[future_index_];
  else
    current_exec_vars_ = options_data_matrix_[future_index_][option_index_];

  SecurityMarketView* current_smv_ = current_exec_vars_->smv_;

  current_exec_vars_->best_nonself_bid_price_ = current_smv_->bestbid_price();
  current_exec_vars_->best_nonself_bid_int_price_ = current_smv_->bestbid_int_price();
  current_exec_vars_->best_nonself_bid_size_ = current_smv_->bestbid_size();

  current_exec_vars_->best_nonself_ask_price_ = current_smv_->bestask_price();
  current_exec_vars_->best_nonself_ask_int_price_ = current_smv_->bestask_int_price();
  current_exec_vars_->best_nonself_ask_size_ = current_smv_->bestask_size();
}

// who calls this function ?
void OptionsRiskManager::OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                                      const MarketUpdateInfo& _market_update_info_) {
  int _product_index_ = security_id_prod_idx_map_[_security_id_].first;
  int _option_index_ = security_id_prod_idx_map_[_security_id_].second;

  if (_option_index_ < 0) return;  // Here productwise opentrade loss can be checked

  OptionVars* current_exec_vars_ = options_data_matrix_[_product_index_][_option_index_];

  // Checking for open trade loss of that product of the same underlying (max_opentrade_loss is per underlying)

  if (current_exec_vars_->som_->base_pnl().opentrade_unrealized_pnl() <
        -paramset_vec_[_product_index_]->max_opentrade_loss_*1.5) {
    if (!current_exec_vars_->getflat_due_to_max_opentrade_loss_) {
      current_exec_vars_->getflat_due_to_max_opentrade_loss_ = true;
      current_exec_vars_->last_getflat_due_to_max_opentrade_loss_triggered_at_msecs_ = watch_.msecs_from_midnight();
      current_exec_vars_->num_opentrade_loss_hits_++;
      DBGLOG_TIME_CLASS << "getflat_due_to_max_opentrade_loss_ " << current_exec_vars_->smv_->shortcode() << " "
                        << current_exec_vars_->som_->base_pnl().opentrade_unrealized_pnl() << DBGLOG_ENDL_FLUSH;
    }
    options_data_matrix_[_product_index_][_option_index_]->should_be_getting_flat_ = true;
    options_data_matrix_[_product_index_][_option_index_]->getflat_aggressive_ = true;
  }
  else if (current_exec_vars_->som_->base_pnl().opentrade_unrealized_pnl() <
      -paramset_vec_[_product_index_]->max_opentrade_loss_) {
    if (!current_exec_vars_->getflat_due_to_max_opentrade_loss_) {
      current_exec_vars_->getflat_due_to_max_opentrade_loss_ = true;
      current_exec_vars_->last_getflat_due_to_max_opentrade_loss_triggered_at_msecs_ = watch_.msecs_from_midnight();
      current_exec_vars_->num_opentrade_loss_hits_++;
      DBGLOG_TIME_CLASS << "getflat_due_to_max_opentrade_loss_ " << current_exec_vars_->smv_->shortcode() << " "
                        << current_exec_vars_->som_->base_pnl().opentrade_unrealized_pnl() << DBGLOG_ENDL_FLUSH;
    }
    options_data_matrix_[_product_index_][_option_index_]->should_be_getting_flat_ = true;
  } else {
    if ((current_exec_vars_->getflat_due_to_max_opentrade_loss_) &&
        (watch_.msecs_from_midnight() -
             options_data_matrix_[_product_index_][_option_index_]
                 ->last_getflat_due_to_max_opentrade_loss_triggered_at_msecs_ >
         paramset_vec_[_product_index_]->break_msecs_on_max_opentrade_loss_)) {
      options_data_matrix_[_product_index_][_option_index_]->getflat_due_to_max_opentrade_loss_ = false;
      options_data_matrix_[_product_index_][_option_index_]->getflat_aggressive_ = false;
      DBGLOG_TIME_CLASS << "resume trading after getflat_due_to_max_opentrade_loss_ "
                        << current_exec_vars_->smv_->shortcode() << " "
                        << current_exec_vars_->som_->base_pnl().opentrade_unrealized_pnl() << DBGLOG_ENDL_FLUSH;
    }
  }

  if (!(livetrading_) && (current_exec_vars_->is_ready_) && !(current_exec_vars_->getflat_due_to_close_)) {
    current_exec_vars_->total_volume_traded_ += _trade_print_info_.size_traded_;
  }
}

// Only Market Data Interrupt of Trading Products are considered
void OptionsRiskManager::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                 const int msecs_since_last_receive_) {
  int _product_index_ = security_id_prod_idx_map_[_security_id_].first;
  int _option_index_ = security_id_prod_idx_map_[_security_id_].second;

  OptionVars* current_exec_vars_;

  if (_option_index_ < 0)
    current_exec_vars_ = underlying_data_vec_[_product_index_];
  else
    current_exec_vars_ = options_data_matrix_[_product_index_][_option_index_];

  std::string shc_not_ready = current_exec_vars_->smv_->shortcode();

  DBGLOG_TIME << " market_data_interrupt_ for " << shc_not_ready << DBGLOG_ENDL_FLUSH;

  if ((!current_exec_vars_->getflat_due_to_market_data_interrupt_)) {
    // putting the check against enable_market_data_interrupt_ here
    // allows the internal variables to be modified as they should be
    // but we do not print / email getflat_due_to_market_data_interrupt_ ,
    // since we do not actually getflat.
    if (dbglogger_.CheckLoggingLevel(TRADING_ERROR) && current_exec_vars_->enable_market_data_interrupt_) {
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
                 << " on " << hostname_ << "\n";

          getflat_email_string_ = t_oss_.str();
        }

        SendMail(getflat_email_string_, getflat_email_string_);
      }
    }
    current_exec_vars_->getflat_due_to_market_data_interrupt_ = true;
    ProcessGetFlat(_product_index_, _option_index_);
  }

  ProcessGetFlat(_product_index_, _option_index_);
}

void OptionsRiskManager::OnMarketDataResumed(const unsigned int _security_id_) {
  int _product_index_ = security_id_prod_idx_map_[_security_id_].first;
  int _option_index_ = security_id_prod_idx_map_[_security_id_].second;

  OptionVars* current_exec_vars_;

  if (_option_index_ < 0)
    current_exec_vars_ = underlying_data_vec_[_product_index_];
  else
    current_exec_vars_ = options_data_matrix_[_product_index_][_option_index_];

  std::string shc_not_ready = current_exec_vars_->smv_->shortcode();

  if (current_exec_vars_->getflat_due_to_market_data_interrupt_) {
    if (dbglogger_.CheckLoggingLevel(TRADING_ERROR))  // Do not print if not resuming from a getflat
    {
      DBGLOG_TIME << " resume normal NO_market_data_interrupt_ of " << shc_not_ready << DBGLOG_ENDL_FLUSH;
      if (livetrading_) {
        DBGLOG_DUMP;
      }
    }
    current_exec_vars_->getflat_due_to_market_data_interrupt_ = false;
    ProcessGetFlat(_product_index_, _option_index_);
  }
}

// Checks for only a particular contract and underlying
void OptionsRiskManager::ProcessGetFlat(int _product_index_, int _option_index_) {
  OptionVars* current_exec_vars_;

  current_exec_vars_ = options_data_matrix_[_product_index_][_option_index_];
  bool t_should_be_getting_flat_ = ShouldBeGettingFlat(_product_index_, _option_index_, current_exec_vars_);
  if (current_exec_vars_->should_be_getting_flat_ &&
      !t_should_be_getting_flat_) {  // currently it is set  ... so after this call we will start normal trading
    if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
      DBGLOG_TIME << "resume_normal_trading @"
                  << " NYC: " << watch_.NYTimeString() << " UTC: " << watch_.UTCTimeString()
                  << " IND: " << watch_.IndTimeString() << " SHC: " << current_exec_vars_->smv_->shortcode()
                  << DBGLOG_ENDL_FLUSH;
    }
  }
  current_exec_vars_->should_be_getting_flat_ = t_should_be_getting_flat_;

  current_exec_vars_ = underlying_data_vec_[_product_index_];
  t_should_be_getting_flat_ = ShouldBeGettingFlat(_product_index_, -1, current_exec_vars_);
  if (current_exec_vars_->should_be_getting_flat_ &&
      !t_should_be_getting_flat_) {  // currently it is set  ... so after this call we will start normal trading
    if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
      DBGLOG_TIME << "resume_normal_trading @"
                  << " NYC: " << watch_.NYTimeString() << " UTC: " << watch_.UTCTimeString()
                  << " IND: " << watch_.IndTimeString() << " SHC: " << current_exec_vars_->smv_->shortcode()
                  << DBGLOG_ENDL_FLUSH;
    }
  }
  current_exec_vars_->should_be_getting_flat_ = t_should_be_getting_flat_;
}

// Checks for all the contracts in the underlying
// who call this ?
void OptionsRiskManager::ProcessGetFlat(int _product_index_) {
  OptionVars* current_exec_vars_;

  for (unsigned int const_idx_ = 0; const_idx_ < options_data_matrix_[_product_index_].size(); const_idx_++) {
    current_exec_vars_ = options_data_matrix_[_product_index_][const_idx_];
    bool t_should_be_getting_flat_ = ShouldBeGettingFlat(_product_index_, const_idx_, current_exec_vars_);
    if (current_exec_vars_->should_be_getting_flat_ &&
        !t_should_be_getting_flat_) {  // currently it is set  ... so after this call we will start normal trading
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME << "resume_normal_trading @"
                    << " NYC: " << watch_.NYTimeString() << " UTC: " << watch_.UTCTimeString()
                    << " IND: " << watch_.IndTimeString() << " SHC: " << current_exec_vars_->smv_->shortcode()
                    << DBGLOG_ENDL_FLUSH;
        if (livetrading_) {
          DBGLOG_DUMP;
        }
      }
    }
    current_exec_vars_->should_be_getting_flat_ = t_should_be_getting_flat_;
  }

  current_exec_vars_ = underlying_data_vec_[_product_index_];
  bool t_should_be_getting_flat_ = ShouldBeGettingFlat(_product_index_, -1, current_exec_vars_);
  if (current_exec_vars_->should_be_getting_flat_ &&
      !t_should_be_getting_flat_) {  // currently it is set  ... so after this call we will start normal trading
    if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
      DBGLOG_TIME << "resume_normal_trading @"
                  << " NYC: " << watch_.NYTimeString() << " UTC: " << watch_.UTCTimeString()
                  << " IND: " << watch_.IndTimeString() << " SHC: " << current_exec_vars_->smv_->shortcode()
                  << DBGLOG_ENDL_FLUSH;
      if (livetrading_) {
        DBGLOG_DUMP;
      }
    }
  }
  current_exec_vars_->should_be_getting_flat_ = t_should_be_getting_flat_;
}

bool OptionsRiskManager::ShouldBeGettingFlat(int _product_index_, int _option_index_, OptionVars* current_exec_vars_) {
  if (current_exec_vars_->external_getflat_) {
    if (!current_exec_vars_
             ->getflat_due_to_external_getflat_) {  // first time that getflat_due_to_external_getflat_ is false and
                                                    // external_getflat_ is true ..
      // which means the user message was received right now
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME_CLASS_FUNC_LINE << current_exec_vars_->smv_->shortcode() << " getflat_due_to_external_getflat_ "
                                    << runtime_id_ << DBGLOG_ENDL_FLUSH;
      }
      current_exec_vars_->getflat_due_to_external_getflat_ = true;
    }
    return true;
  } else {
    current_exec_vars_->getflat_due_to_external_getflat_ = false;
  }

  if (current_exec_vars_->getflat_due_to_close_ ||
      (watch_.msecs_from_midnight() >
       trading_end_utc_mfm_)) {  // if getflat_due_to_close_ is once activated then it can't be reset  ...
    if (!current_exec_vars_->getflat_due_to_close_) {
      // std::cout << "GetFlat due to close : " << current_exec_vars_->smv_->shortcode() << std::endl;
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME_CLASS_FUNC_LINE << current_exec_vars_->smv_->shortcode()
                                    << " getflat_due_to_close_ current: " << watch_.msecs_from_midnight() << " "
                                    << trading_end_utc_mfm_ << " Strategy: " << runtime_id_ << DBGLOG_ENDL_FLUSH;
      }
      current_exec_vars_->getflat_due_to_close_ = true;
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
    current_exec_vars_->getflat_due_to_close_ = false;
  }

  if (current_exec_vars_->getflat_due_to_max_opentrade_loss_) {
    /*  if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME_CLASS_FUNC_LINE << current_exec_vars_->smv_->shortcode()
                                    << " getflat due to open trade loss: "
                                    << current_exec_vars_->som_->base_pnl().opentrade_unrealized_pnl()
                                    << " Strategy: " << runtime_id_ << DBGLOG_ENDL_FLUSH;
      }*/
    return true;
  }
  if (current_exec_vars_->getflat_due_to_max_loss_) {
    return true;
  }
  if (current_exec_vars_->getflat_due_to_market_data_interrupt_ && current_exec_vars_->enable_market_data_interrupt_) {
    if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
      DBGLOG_TIME_CLASS_FUNC_LINE << " getflat_due_to_interrupt: " << watch_.msecs_from_midnight()
                                  << " Strategy: " << runtime_id_ << DBGLOG_ENDL_FLUSH;
    }
    return true;
  }

  if (current_exec_vars_->getflat_due_to_min_int_price_ ||
      ((current_exec_vars_->smv_->is_ready_) &&
       (current_exec_vars_->smv_->bestbid_int_price() <= paramset_vec_[_product_index_]->min_int_price_to_place_))) {
    if (!current_exec_vars_->getflat_due_to_min_int_price_) {
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME_CLASS_FUNC_LINE << current_exec_vars_->smv_->shortcode()
                                    << " getflat_due_to_min_int_price: " << watch_.msecs_from_midnight()
                                    << " Strategy: " << runtime_id_ << DBGLOG_ENDL_FLUSH;
      }
      current_exec_vars_->getflat_due_to_min_int_price_ = true;
    }
    return true;
  }

  return false;
}

// total_future_exposure_ = delta_from_options_ + futures_position_from_delta_hedging_
// total_unhedged_ = delta_from_options_ + total_hedged_

// if ( abs(total_unhedged_) > upper_threshold_ ) increase_totalhedged
// if ( abs(total_unhedged_) < lower_threshold_ ) decrease_totalhedged

void OptionsRiskManager::GetDeltaHedgeLogic(int _product_index_) {
  // If it's already getting flat then other securities are also trying to get flat ... no need to delta hedge in that
  // case

  if (paramset_vec_[_product_index_]->delta_hedge_logic_ == 1) GetDeltaHedgeLogic1(_product_index_);

  if (paramset_vec_[_product_index_]->delta_hedge_logic_ == 2) GetDeltaHedgeLogic2(_product_index_);

  if (paramset_vec_[_product_index_]->delta_hedge_logic_ == 3) GetDeltaHedgeLogic3(_product_index_);

}

void OptionsRiskManager::GetDeltaHedgeLogic1(int _product_index_) {
  // If it's already getting flat then other securities are also trying to get flat ... no need to delta hedge in that
  // case
  if (underlying_data_vec_[_product_index_]->should_be_getting_flat_) return;

  if (std::abs(delta_unhedged_[_product_index_]) >
      (upper_threshold_vec_[_product_index_] + underlying_market_view_vec_[_product_index_]->min_order_size())) {
    int position_ = (delta_unhedged_[_product_index_] > 0)
                        ? (int)(delta_unhedged_[_product_index_] - upper_threshold_vec_[_product_index_])
                        : (int)(delta_unhedged_[_product_index_] + upper_threshold_vec_[_product_index_]);
    GetAggressiveDeltaHedge(_product_index_, position_);
  } else if (std::abs(delta_unhedged_[_product_index_]) >
             (lower_threshold_vec_[_product_index_] + underlying_market_view_vec_[_product_index_]->min_order_size())) {
    int position_ = (delta_unhedged_[_product_index_] > 0)
                        ? (int)(delta_unhedged_[_product_index_] - lower_threshold_vec_[_product_index_])
                        : (int)(delta_unhedged_[_product_index_] + lower_threshold_vec_[_product_index_]);
    GetDeltaHedge(_product_index_, position_);
  }
}

void OptionsRiskManager::GetDeltaHedgeLogic2(int _product_index_) {
  if (std::abs(total_delta_per_underlying_[_product_index_]) >
      (upper_threshold_vec_[_product_index_] + underlying_market_view_vec_[_product_index_]->min_order_size())) {
    int position_ = (delta_unhedged_[_product_index_] > 0)
                        ? (int)(delta_unhedged_[_product_index_] - upper_threshold_vec_[_product_index_])
                        : (int)(delta_unhedged_[_product_index_] + upper_threshold_vec_[_product_index_]);
    GetAggressiveDeltaHedge(_product_index_, position_);
  } else if (std::abs(total_delta_per_underlying_[_product_index_]) >
             (lower_threshold_vec_[_product_index_] + underlying_market_view_vec_[_product_index_]->min_order_size())) {
    int position_ = (delta_unhedged_[_product_index_] > 0)
                        ? (int)(delta_unhedged_[_product_index_] - lower_threshold_vec_[_product_index_])
                        : (int)(delta_unhedged_[_product_index_] + lower_threshold_vec_[_product_index_]);
    GetDeltaHedge(_product_index_, position_);
  } else
    GetFlatTradingLogic(_product_index_, -2);  // Magnitude of negative number signify delta hedge logic number
}

void OptionsRiskManager::GetDeltaHedgeLogic3(int _product_index_) {

  if (total_delta_per_underlying_[_product_index_] > lower_threshold_vec_[_product_index_])
  {
    current_tradevarset_vec_[_product_index_] =  long_tradevarset_vec_[_product_index_];
  }
  else if (total_delta_per_underlying_[_product_index_] < -lower_threshold_vec_[_product_index_])
  {
    current_tradevarset_vec_[_product_index_] =  short_tradevarset_vec_[_product_index_];
  }
  else {
    current_tradevarset_vec_[_product_index_] =  closeout_zeropos_tradevarset_;
  }
}

// How to define thresholds here. We do not want an extra param profile . On the contrary if we have 0 thresholds,
// we would have lots of place and cancel

bool OptionsRiskManager::UpdateTarget(double _new_target_, double _new_sum_vars_, int _product_index_) {

  TradeVars_t current_tradevarset_ = current_tradevarset_vec_[_product_index_];
  OptionVars* current_exec_vars_ = underlying_data_vec_[_product_index_];
  OptionsParamSet* param_ = paramset_vec_[_product_index_];
  SmartOrderManager* p_smart_order_manager_ = current_exec_vars_->som_;
  SecurityMarketView* p_dep_market_view_ = current_exec_vars_->smv_;


  double best_nonself_ask_int_price_ = current_exec_vars_->best_nonself_ask_int_price_;
  double best_nonself_ask_price_ = current_exec_vars_->best_nonself_ask_price_;
  double best_nonself_ask_size_ = current_exec_vars_->best_nonself_ask_size_;
  double best_nonself_bid_int_price_ = current_exec_vars_->best_nonself_bid_int_price_;
  double best_nonself_bid_price_ = current_exec_vars_->best_nonself_bid_price_;
  double best_nonself_bid_size_ = current_exec_vars_->best_nonself_bid_size_;


  bool top_bid_place_ = false;
  bool top_bid_keep_ = false;

  if (current_tradevarset_.l1bid_trade_size_ > 0) {
    if ( ((current_exec_vars_->last_buy_msecs_ <= 0) ||
          (watch_.msecs_from_midnight() - current_exec_vars_->last_buy_msecs_ >= param_->cooloff_interval_)) &&
         (_new_target_ - best_nonself_bid_price_ >= current_tradevarset_.l1bid_place_) ) {
      top_bid_place_ = true;
      top_bid_keep_ = true;
    }
    else if (_new_target_ - best_nonself_bid_price_ >= current_tradevarset_.l1bid_keep_) {
      top_bid_keep_ = true;
    }
  }

  bool top_ask_place_ = false;
  bool top_ask_keep_ = false;

  if (current_tradevarset_.l1ask_trade_size_ > 0) {
    if ( ((current_exec_vars_->last_sell_msecs_ <= 0) ||
          (watch_.msecs_from_midnight() - current_exec_vars_->last_sell_msecs_ >= param_->cooloff_interval_)) &&
         (best_nonself_ask_price_ - _new_target_ >= current_tradevarset_.l1ask_place_) ) {
      top_ask_place_ = true;
      top_ask_keep_ = true;
    }
    else if (best_nonself_ask_price_ - _new_target_ >= current_tradevarset_.l1ask_keep_) {
      top_ask_keep_ = true;
    }
  }

  if (top_bid_place_) {
    if ((p_smart_order_manager_->GetTotalBidSizeEqAboveIntPx(best_nonself_bid_int_price_) == 0)) {
      p_smart_order_manager_->SendTrade(best_nonself_bid_price_, best_nonself_bid_int_price_,
                                        current_tradevarset_.l1bid_trade_size_, kTradeTypeBuy, 'B');
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
         DBGLOG_TIME_CLASS_FUNC << "SendTrade B of " << current_tradevarset_.l1bid_trade_size_ << " @ "
                                << best_nonself_bid_price_ << " tgt: " << _new_target_
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
                    << " tgt_bias: " << _new_target_ / p_dep_market_view_->min_price_increment()
                    << " thresh_t: " << current_tradevarset_.l1bid_keep_
                    << " SHC: " << p_dep_market_view_->shortcode() << " Mkt: " << best_nonself_bid_size_
                    << " @ " << best_nonself_bid_price_ << " ---- " << best_nonself_ask_price_ << " @ "
                    << " tMktSz: " << best_nonself_bid_size_ << DBGLOG_ENDL_FLUSH;
      }
    }
  }

  if(top_ask_place_) {
    if ((p_smart_order_manager_->GetTotalAskSizeEqAboveIntPx(best_nonself_ask_int_price_) == 0)) {
      p_smart_order_manager_->SendTrade(best_nonself_ask_price_, best_nonself_ask_int_price_,
                                        current_tradevarset_.l1ask_trade_size_, kTradeTypeSell, 'B');
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        DBGLOG_TIME_CLASS_FUNC << "SendTrade S of " << current_tradevarset_.l1ask_trade_size_ << " @ "
                               << best_nonself_ask_price_ << " tgt: " << _new_target_
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
                    << " tgt: " << _new_target_
                    << " thresh_t: " << current_tradevarset_.l1ask_keep_
                    << " SHC: " << p_dep_market_view_->shortcode() << " Mkt: " << best_nonself_bid_size_
                    << " @ " << best_nonself_bid_price_ << " ---- " << best_nonself_ask_price_ << " @ "
                    << " tMktSz: " << best_nonself_ask_size_ << DBGLOG_ENDL_FLUSH;
      }
    }
  }

  return true;
}



void OptionsRiskManager::GetDeltaHedge(int _product_index_, int position_) {
  if (position_ == 0) {  // nothing to be done, cancel all remaining orders
    order_manager_vec_[_product_index_]->CancelAllOrders();
  } else {
    if (position_ > 0) {
      // long hence cancel all bid orders
      order_manager_vec_[_product_index_]->CancelAllBidOrders();

      // size already placed at best or above
      int t_size_already_placed_ = order_manager_vec_[_product_index_]->SumAskSizeConfirmedEqAboveIntPrice(
                                       underlying_market_view_vec_[_product_index_]->bestask_int_price()) +
                                   order_manager_vec_[_product_index_]->SumAskSizeUnconfirmedEqAboveIntPrice(
                                       underlying_market_view_vec_[_product_index_]->bestask_int_price());

      int trade_size_to_place_ =
          MathUtils::GetFlooredMultipleOf(std::min(position_, paramset_vec_[_product_index_]->unit_trade_size_),
                                          underlying_market_view_vec_[_product_index_]->min_order_size());

      if (trade_size_to_place_ > t_size_already_placed_) {  // if the required size is not present then add more
        SendTradeAndLog(underlying_market_view_vec_[_product_index_]->bestask_int_price(),
                        trade_size_to_place_ - t_size_already_placed_, kTradeTypeSell, 'B', _product_index_, -1);
        // done_for_this_round_ = true;
      }

      // cancelling any extra orders to avoid overfill
      order_manager_vec_[_product_index_]->KeepAskSizeInPriceRange(position_);

    } else {  // my_position_ < 0

      // short hence cancel all sell orders
      order_manager_vec_[_product_index_]->CancelAllAskOrders();
      //    bool done_for_this_round_ = false;

      // we should place at best, limit the size by some %age of mkt_size to not effect the market, by default 50%
      int trade_size_to_place_ =
          MathUtils::GetFlooredMultipleOf(std::min(-position_, paramset_vec_[_product_index_]->unit_trade_size_),
                                          underlying_market_view_vec_[_product_index_]->min_order_size());

      // size already placed at best or above
      int t_size_already_placed_ = order_manager_vec_[_product_index_]->SumBidSizeConfirmedEqAboveIntPrice(
                                       underlying_market_view_vec_[_product_index_]->bestbid_int_price()) +
                                   order_manager_vec_[_product_index_]->SumBidSizeUnconfirmedEqAboveIntPrice(
                                       underlying_market_view_vec_[_product_index_]->bestbid_int_price());

      if (trade_size_to_place_ > t_size_already_placed_) {  // if the required size is not present then add more
        SendTradeAndLog(underlying_market_view_vec_[_product_index_]->bestbid_int_price(),
                        trade_size_to_place_ - t_size_already_placed_, kTradeTypeBuy, 'B', _product_index_, -1);
        // done_for_this_round_ = true;
      }

      // cancelling any extra orders to avoid overfill
      order_manager_vec_[_product_index_]->KeepBidSizeInPriceRange(-position_);
    }
  }
}

void OptionsRiskManager::GetAggressiveDeltaHedge(int _product_index_, int position_) {
  if (position_ == 0) {
    order_manager_vec_[_product_index_]->CancelAllOrders();
  } else {
    if (position_ > 0) {
      order_manager_vec_[_product_index_]->CancelAllBidOrders();
      order_manager_vec_[_product_index_]->CancelAsksBelowIntPrice(
          underlying_market_view_vec_[_product_index_]->bestbid_int_price());
      int t_size_ordered_ = order_manager_vec_[_product_index_]->SumAskSizeConfirmedEqAboveIntPrice(
                                underlying_market_view_vec_[_product_index_]->bestbid_int_price()) +
                            order_manager_vec_[_product_index_]->SumAskSizeUnconfirmedEqAboveIntPrice(
                                underlying_market_view_vec_[_product_index_]->bestbid_int_price());

      int trade_size_required_ =
          MathUtils::GetFlooredMultipleOf(std::min(position_, paramset_vec_[_product_index_]->unit_trade_size_),
                                          underlying_market_view_vec_[_product_index_]->min_order_size());

      if (t_size_ordered_ < trade_size_required_) {
        SendTradeAndLog(underlying_market_view_vec_[_product_index_]->bestbid_int_price(),
                        trade_size_required_ - t_size_ordered_, kTradeTypeSell, 'A', _product_index_, -1);
      }
    } else {
      order_manager_vec_[_product_index_]->CancelAllAskOrders();  // short hence cancel all sell orders
      order_manager_vec_[_product_index_]->CancelBidsBelowIntPrice(
          underlying_market_view_vec_[_product_index_]->bestask_int_price());  // cancel all non bestlevel bid orders
      // ideal size to place ask for is what the position is, except limited to unit_trade_size_ ( for sanity and market
      // effect reasons )
      int t_size_ordered_ = order_manager_vec_[_product_index_]->SumBidSizeConfirmedEqAboveIntPrice(
                                underlying_market_view_vec_[_product_index_]->bestask_int_price()) +
                            order_manager_vec_[_product_index_]->SumBidSizeUnconfirmedEqAboveIntPrice(
                                underlying_market_view_vec_[_product_index_]->bestask_int_price());
      int trade_size_required_ =
          MathUtils::GetFlooredMultipleOf(std::min(-position_, paramset_vec_[_product_index_]->unit_trade_size_),
                                          underlying_market_view_vec_[_product_index_]->min_order_size());
      if (t_size_ordered_ < trade_size_required_) {  // if the required size is not present then add more
        SendTradeAndLog(underlying_market_view_vec_[_product_index_]->bestask_int_price(),
                        trade_size_required_ - t_size_ordered_, kTradeTypeBuy, 'A', _product_index_, -1);
      }
    }
  }
}

void OptionsRiskManager::PassiveGetFlatTradingLogic(int _product_index_, int _option_index_) {
  OptionVars* current_exec_vars_;
  if (_option_index_ < 0)
    current_exec_vars_ = underlying_data_vec_[_product_index_];
  else
    current_exec_vars_ = options_data_matrix_[_product_index_][_option_index_];

  SecurityMarketView* p_dep_market_view_ = current_exec_vars_->smv_;
  SmartOrderManager* p_smart_order_manager_ = current_exec_vars_->som_;
  int position_ = current_exec_vars_->position_;

  if (position_ == 0) {  // nothing to be done, cancel all remaining orders
    p_smart_order_manager_->CancelAllOrders();
  } else {
    if (position_ > 0) {
      // long hence cancel all bid orders
      p_smart_order_manager_->CancelAllBidOrders();

      // size already placed at best or above
      int t_size_already_placed_ =
          p_smart_order_manager_->SumAskSizeConfirmedEqAboveIntPrice(p_dep_market_view_->bestask_int_price()) +
          p_smart_order_manager_->SumAskSizeUnconfirmedEqAboveIntPrice(p_dep_market_view_->bestask_int_price());

      int trade_size_to_place_ = MathUtils::GetFlooredMultipleOf(
          std::min(position_, paramset_vec_[_product_index_]->unit_trade_size_), p_dep_market_view_->min_order_size());

      if (trade_size_to_place_ > t_size_already_placed_) {  // if the required size is not present then add more
        SendTradeAndLog(p_dep_market_view_->bestask_int_price(), trade_size_to_place_ - t_size_already_placed_,
                        kTradeTypeSell, 'B', _product_index_, _option_index_);
        // done_for_this_round_ = true;
      }

      // cancelling any extra orders to avoid overfill
      p_smart_order_manager_->KeepAskSizeInPriceRange(position_);

    } else {  // my_position_ < 0

      // short hence cancel all sell orders
      p_smart_order_manager_->CancelAllAskOrders();
      //    bool done_for_this_round_ = false;

      // we should place at best, limit the size by some %age of mkt_size to not effect the market, by default 50%
      int trade_size_to_place_ = MathUtils::GetFlooredMultipleOf(
          std::min(-position_, paramset_vec_[_product_index_]->unit_trade_size_), p_dep_market_view_->min_order_size());

      // size already placed at best or above
      int t_size_already_placed_ =
          p_smart_order_manager_->SumBidSizeConfirmedEqAboveIntPrice(p_dep_market_view_->bestbid_int_price()) +
          p_smart_order_manager_->SumBidSizeUnconfirmedEqAboveIntPrice(p_dep_market_view_->bestbid_int_price());

      if (trade_size_to_place_ > t_size_already_placed_) {  // if the required size is not present then add more
        SendTradeAndLog(p_dep_market_view_->bestbid_int_price(), trade_size_to_place_ - t_size_already_placed_,
                        kTradeTypeBuy, 'B', _product_index_, _option_index_);
        // done_for_this_round_ = true;
      }

      // cancelling any extra orders to avoid overfill
      p_smart_order_manager_->KeepBidSizeInPriceRange(-position_);
    }
  }
}

void OptionsRiskManager::AggressiveGetFlatTradingLogic(int _product_index_, int _option_index_) {
  OptionVars* current_exec_vars_;
  if (_option_index_ < 0)
    current_exec_vars_ = underlying_data_vec_[_product_index_];
  else
    current_exec_vars_ = options_data_matrix_[_product_index_][_option_index_];

  SecurityMarketView* p_dep_market_view_ = current_exec_vars_->smv_;
  SmartOrderManager* p_smart_order_manager_ = current_exec_vars_->som_;
  int position_ = current_exec_vars_->position_;

  if (position_ == 0) {
    p_smart_order_manager_->CancelAllOrders();
  } else {
    if (position_ > 0) {
      if (watch_.msecs_from_midnight() - current_exec_vars_->last_agg_sell_msecs_ <
          paramset_vec_[_product_index_]->aggflat_cooloff_interval_)
        return;

      p_smart_order_manager_->CancelAllBidOrders();
      p_smart_order_manager_->CancelAsksBelowIntPrice(p_dep_market_view_->bestbid_int_price());
      int t_size_ordered_ =
          p_smart_order_manager_->SumAskSizeConfirmedEqAboveIntPrice(p_dep_market_view_->bestbid_int_price()) +
          p_smart_order_manager_->SumAskSizeUnconfirmedEqAboveIntPrice(p_dep_market_view_->bestbid_int_price());

      int trade_size_required_ = MathUtils::GetFlooredMultipleOf(
          std::min(position_, paramset_vec_[_product_index_]->unit_trade_size_), p_dep_market_view_->min_order_size());

      if (t_size_ordered_ < trade_size_required_) {
        SendTradeAndLog(p_dep_market_view_->bestbid_int_price(), trade_size_required_ - t_size_ordered_, kTradeTypeSell,
                        'A', _product_index_, _option_index_);
        current_exec_vars_->last_agg_sell_msecs_ = watch_.msecs_from_midnight();
      }
    } else {
      if (watch_.msecs_from_midnight() - current_exec_vars_->last_agg_buy_msecs_ <
          paramset_vec_[_product_index_]->aggflat_cooloff_interval_)
        return;

      p_smart_order_manager_->CancelAllAskOrders();  // short hence cancel all sell orders
      p_smart_order_manager_->CancelBidsBelowIntPrice(
          p_dep_market_view_->bestask_int_price());  // cancel all non bestlevel bid orders
      // ideal size to place ask for is what the position is, except limited to unit_trade_size_ ( for sanity and market
      // effect reasons )
      int t_size_ordered_ =
          p_smart_order_manager_->SumBidSizeConfirmedEqAboveIntPrice(p_dep_market_view_->bestask_int_price()) +
          p_smart_order_manager_->SumBidSizeUnconfirmedEqAboveIntPrice(p_dep_market_view_->bestask_int_price());
      int trade_size_required_ = MathUtils::GetFlooredMultipleOf(
          std::min(-position_, paramset_vec_[_product_index_]->unit_trade_size_), p_dep_market_view_->min_order_size());
      if (t_size_ordered_ < trade_size_required_) {  // if the required size is not present then add more
        SendTradeAndLog(p_dep_market_view_->bestask_int_price(), trade_size_required_ - t_size_ordered_, kTradeTypeBuy,
                        'A', _product_index_, _option_index_);
        current_exec_vars_->last_agg_buy_msecs_ = watch_.msecs_from_midnight();
      }
    }
  }
}

void OptionsRiskManager::SendTradeAndLog(int _int_px_, int _size_, TradeType_t _buysell_, char _level_indicator_,
                                         int _product_index_, int _option_index_) {
  OptionVars* current_exec_vars_;
  if (_option_index_ < 0)
    current_exec_vars_ = underlying_data_vec_[_product_index_];
  else
    current_exec_vars_ = options_data_matrix_[_product_index_][_option_index_];

  SecurityMarketView* p_dep_market_view_ = current_exec_vars_->smv_;
  SmartOrderManager* p_smart_order_manager_ = current_exec_vars_->som_;

  p_smart_order_manager_->SendTradeIntPx(_int_px_, _size_, _buysell_, _level_indicator_);
  if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
    DBGLOG_TIME_CLASS_FUNC << "SendTrade " << GetTradeTypeChar(_buysell_) << " " << p_dep_market_view_->shortcode()
                           << " of " << _size_ << " @ IntPx: " << _int_px_
                           << " mkt: " << p_dep_market_view_->bestbid_size() << " @ "
                           << p_dep_market_view_->bestbid_price() << " X " << p_dep_market_view_->bestask_price()
                           << " @ " << p_dep_market_view_->bestask_size() << DBGLOG_ENDL_FLUSH;
  }
}

void OptionsRiskManager::CallPlaceCancelNonBestLevels(int _product_index_) {
  order_manager_vec_[_product_index_]->CancelBidsBelowIntPrice(
      underlying_market_view_vec_[_product_index_]->bestbid_int_price());
  order_manager_vec_[_product_index_]->CancelAsksBelowIntPrice(
      underlying_market_view_vec_[_product_index_]->bestask_int_price());
}

// PnL computation is done here as OnExec is called before sim base pnl is updated and this is called afterwards

void OptionsRiskManager::OnPositionChange(int t_new_position_, int position_diff_, const unsigned int _security_id_) {
  int _product_index_ = security_id_prod_idx_map_[_security_id_].first;
  int _option_index_ = security_id_prod_idx_map_[_security_id_].second;
  if (_option_index_ < 0) {
    OptionVars* current_exec_vars_ = underlying_data_vec_[_product_index_];

    double pnl_diff_ = order_manager_vec_[_product_index_]->base_pnl().total_pnl() - current_exec_vars_->total_pnl_;

    current_exec_vars_->total_pnl_ += pnl_diff_;
    if (current_exec_vars_->total_pnl_ < current_exec_vars_->min_pnl_) {
      current_exec_vars_->min_pnl_ = current_exec_vars_->total_pnl_;
    }

    pnl_per_underlying_[_product_index_] += pnl_diff_;

    // Aggressive get flat if too much loss
    if (pnl_per_underlying_[_product_index_] < -paramset_vec_[_product_index_]->global_max_loss_*1.3) {
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
       DBGLOG_TIME_CLASS_FUNC_LINE << current_exec_vars_->smv_->shortcode()
                                    << " getflat_aggressive_due_to_max_loss: " << pnl_per_underlying_[_product_index_]
                                    << " Strategy: " << runtime_id_ << DBGLOG_ENDL_FLUSH;
      }
      underlying_data_vec_[_product_index_]->getflat_due_to_max_loss_ = true;
      underlying_data_vec_[_product_index_]->getflat_aggressive_ = true;
    }
    else if (pnl_per_underlying_[_product_index_] < -paramset_vec_[_product_index_]->global_max_loss_) {
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME_CLASS_FUNC_LINE << current_exec_vars_->smv_->shortcode()
                                    << " getflat_due_to_max_loss: " << pnl_per_underlying_[_product_index_]
                                    << " Strategy: " << runtime_id_ << DBGLOG_ENDL_FLUSH;
      }
      underlying_data_vec_[_product_index_]->getflat_due_to_max_loss_ = true;
    }
    if (pnl_per_underlying_[_product_index_] < min_pnl_per_underlying_[_product_index_]) {
      min_pnl_per_underlying_[_product_index_] = pnl_per_underlying_[_product_index_];
    }

    total_pnl_ += pnl_diff_;
    if (total_pnl_ < -global_max_loss_) {
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME_CLASS_FUNC_LINE << " Global Max loss Triggered : " << total_pnl_ << " Strategy: " << runtime_id_
                                    << DBGLOG_ENDL_FLUSH;
      }
      is_max_loss_reached_global_ = true;
    }
    if (total_pnl_ < min_pnl_) {
      min_pnl_ = total_pnl_;
    }

  } else {
    OptionVars* current_exec_vars_ = options_data_matrix_[_product_index_][_option_index_];

    double pnl_diff_ = current_exec_vars_->som_->base_pnl().total_pnl() - current_exec_vars_->total_pnl_;
    current_exec_vars_->total_pnl_ += pnl_diff_;

    if (current_exec_vars_->total_pnl_ < -paramset_vec_[_product_index_]->max_loss_) {
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME_CLASS_FUNC_LINE << current_exec_vars_->smv_->shortcode()
                                    << " getflat_due_to_max_loss: " << current_exec_vars_->total_pnl_
                                    << " Strategy: " << runtime_id_ << DBGLOG_ENDL_FLUSH;
      }
      current_exec_vars_->getflat_due_to_max_loss_ = true;
    }

    if (current_exec_vars_->total_pnl_ < current_exec_vars_->min_pnl_) {
      current_exec_vars_->min_pnl_ = current_exec_vars_->total_pnl_;
    }

    pnl_per_underlying_[_product_index_] += pnl_diff_;
    if (pnl_per_underlying_[_product_index_] < -paramset_vec_[_product_index_]->global_max_loss_) {
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME_CLASS_FUNC_LINE << underlying_data_vec_[_product_index_]->smv_->shortcode()
                                    << " getflat_due_to_max_loss: " << pnl_per_underlying_[_product_index_]
                                    << " Strategy: " << runtime_id_ << DBGLOG_ENDL_FLUSH;
      }
      underlying_data_vec_[_product_index_]->getflat_due_to_max_loss_ = true;
    }

    if (pnl_per_underlying_[_product_index_] < min_pnl_per_underlying_[_product_index_]) {
      min_pnl_per_underlying_[_product_index_] = pnl_per_underlying_[_product_index_];
    }

    total_pnl_ += pnl_diff_;
    if (total_pnl_ < -global_max_loss_) {
      is_max_loss_reached_global_ = true;
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME_CLASS_FUNC_LINE << " Global Max loss Triggered : " << total_pnl_ << " Strategy: " << runtime_id_
                                    << DBGLOG_ENDL_FLUSH;
      }
    }
    if (total_pnl_ < min_pnl_) {
      min_pnl_ = total_pnl_;
    }
  }
}

void OptionsRiskManager::OnExec(const int _new_position_, const int _exec_quantity_, const TradeType_t _buysell_,
                                const double _price_, const int r_int_price_, const int _security_id_) {
  int _product_index_ = security_id_prod_idx_map_[_security_id_].first;
  int _option_index_ = security_id_prod_idx_map_[_security_id_].second;

  OptionVars* current_exec_vars_;

  if (_option_index_ < 0) {
    current_exec_vars_ = underlying_data_vec_[_product_index_];

    current_exec_vars_->last_traded_price_ = _price_;
    delta_hedged_[_product_index_] = -_new_position_;
    delta_unhedged_[_product_index_] = total_delta_per_underlying_[_product_index_] - delta_hedged_[_product_index_];

    options_risk_premium_->OnRiskChange(_product_index_, delta_unhedged_[_product_index_],
                                        total_gamma_per_underlying_[_product_index_],
                                        total_vega_per_underlying_[_product_index_]);

  } else {
    current_exec_vars_ = options_data_matrix_[_product_index_][_option_index_];

    int position_change_ = _new_position_ - current_exec_vars_->position_;
    current_exec_vars_->last_traded_price_ = _price_;
    delta_unhedged_[_product_index_] += (current_exec_vars_->option_->greeks_.delta_) * position_change_;
    total_delta_per_underlying_[_product_index_] = delta_unhedged_[_product_index_] + delta_hedged_[_product_index_];
    total_gamma_per_underlying_[_product_index_] += (current_exec_vars_->option_->greeks_.gamma_) * position_change_;
    total_vega_per_underlying_[_product_index_] += (current_exec_vars_->option_->greeks_.vega_) * position_change_;
    total_theta_per_underlying_[_product_index_] += (current_exec_vars_->option_->greeks_.theta_) * position_change_;

    options_risk_premium_->OnRiskChange(_product_index_, delta_unhedged_[_product_index_],
                                        total_gamma_per_underlying_[_product_index_],
                                        total_vega_per_underlying_[_product_index_]);
  }

  current_exec_vars_->position_ = _new_position_;

  if (_new_position_ > current_exec_vars_->position_) {
    if (_new_position_ >= 0) {
      current_exec_vars_->last_buy_msecs_ = watch_.msecs_from_midnight();
      current_exec_vars_->last_buy_int_price_ = r_int_price_;
    }
    current_exec_vars_->last_sell_msecs_ = 0;
  } else if (_new_position_ < current_exec_vars_->position_) {
    if (_new_position_ <= 0) {
      current_exec_vars_->last_sell_msecs_ = watch_.msecs_from_midnight();
      current_exec_vars_->last_sell_int_price_ = r_int_price_;
    }
    current_exec_vars_->last_buy_msecs_ = 0;
  }

  GetDeltaHedgeLogic(_product_index_);
  CallPlaceCancelNonBestLevels(_product_index_);

  total_pos_ += (std::abs(delta_unhedged_[_product_index_]) - std::abs(prev_delta_unhedged_[_product_index_])) /
                underlying_data_vec_[_product_index_]->smv_->min_order_size_;
  prev_delta_unhedged_[_product_index_] = delta_unhedged_[_product_index_];
  total_base_pnl_->UpdateTotalRisk(total_pos_);

  mult_base_pnl_vec[_product_index_]->UpdateTotalRisk(delta_unhedged_[_product_index_] /
                                                      underlying_data_vec_[_product_index_]->smv_->min_order_size_);
}

void OptionsRiskManager::OnTimePeriodUpdate(const int num_pages_to_add_) {
  for (int idx = 0; idx < num_underlying_; idx++) {
    if (paramset_vec_[idx]->staggered_getflat_msecs_vec_.size() > 0) {
      if (watch_.msecs_from_midnight() >= paramset_vec_[idx]->staggered_getflat_msecs_vec_[0]) {
        for (unsigned int j = 0; j < options_data_matrix_[idx].size(); j++) {
          options_risk_premium_->risk_matrix_[idx][j].max_position_ /= 2;
        }
        paramset_vec_[idx]->staggered_getflat_msecs_vec_.erase(
            paramset_vec_[idx]->staggered_getflat_msecs_vec_.begin());
      }
    }
    GetDeltaHedgeLogic(idx);
    CallPlaceCancelNonBestLevels(idx);

    if (paramset_vec_[idx]->aggressive_getflat_msecs_ > 0) {
      if (watch_.msecs_from_midnight() >= paramset_vec_[idx]->aggressive_getflat_msecs_) {
        for (unsigned int j = 0; j < options_data_matrix_[idx].size(); j++) {
          options_data_matrix_[idx][j]->external_getflat_ = true;
          options_data_matrix_[idx][j]->getflat_due_to_external_getflat_ = true;
          options_data_matrix_[idx][j]->getflat_aggressive_ = true;
        }
      }
    }
  }

  // First checking the condition and then only looping through every product (once in 15 min only)
  if (!livetrading_) {
    if (sample_index_ < pnl_sampling_timestamps_.size() &&
        watch_.msecs_from_midnight() >= pnl_sampling_timestamps_[sample_index_]) {
      double total_sample_pnl_ = 0;
      for (int idx = 0; idx < num_underlying_; idx++) {
        double underlying_sample_pnl_ = order_manager_vec_[idx]->base_pnl().total_pnl();
        for (unsigned int j = 0; j < options_data_matrix_[idx].size(); j++) {
          underlying_sample_pnl_ += (int)options_data_matrix_[idx][j]->som_->base_pnl().total_pnl();
        }
        pnl_samples_vec_[idx].push_back((int)underlying_sample_pnl_);
        total_sample_pnl_ += underlying_sample_pnl_;
      }
      pnl_samples_.push_back((int)total_sample_pnl_);
      sample_index_++;
    }
  }
}

inline void OptionsRiskManager::OnControlUpdate(const ControlMessage& _control_message_, const char* symbol,
                                                const int trader_id) {
  switch (_control_message_.message_code_) {
    case kControlMessageCodeGetflat: {
      if (strlen(_control_message_.strval_1_) == 0)  // Its global command
      {
        if (!getting_flat_) {
          getting_flat_ = true;
          for (int i = 0; i < num_underlying_; i++) {
            PassiveGetFlatTradingLogic(i, -1);
            underlying_data_vec_[i]->external_getflat_ = true;
            for (unsigned int j = 0; j < options_data_matrix_[i].size(); j++) {
              options_data_matrix_[i][j]->external_getflat_ = true;
            }
            ProcessGetFlat(i);
          }
        }
      } else {
        // Command for specific product
        for (int i = 0; i < num_underlying_; i++) {
          if (strcmp(_control_message_.strval_1_, underlying_data_vec_[i]->smv_->shortcode().c_str()) == 0) {
            PassiveGetFlatTradingLogic(i, -1);
            underlying_data_vec_[i]->external_getflat_ = true;
            for (unsigned int j = 0; j < options_data_matrix_[i].size(); j++) {
              options_data_matrix_[i][j]->external_getflat_ = true;
            }
            ProcessGetFlat(i);
            break;
          }
        }

        // No underlying matched , so command must be for individual contract
        for (int i = 0; i < num_underlying_; i++) {
          for (unsigned int j = 0; j < options_data_matrix_[i].size(); j++) {
            if (strcmp(_control_message_.strval_1_, options_data_matrix_[i][j]->smv_->shortcode().c_str()) == 0) {
              options_data_matrix_[i][j]->external_getflat_ = true;
              ProcessGetFlat(i, j);
              break;
            }
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
          for (int i = 0; i < num_underlying_; i++) {
            PassiveGetFlatTradingLogic(i, -1);
            underlying_data_vec_[i]->external_getflat_ = true;
            for (unsigned int j = 0; j < options_data_matrix_[i].size(); j++) {
              options_data_matrix_[i][j]->external_getflat_ = true;
              options_data_matrix_[i][j]->getflat_due_to_external_getflat_ = true;
              options_data_matrix_[i][j]->getflat_aggressive_ = true;
            }
            ProcessGetFlat(i);
          }
          aggressively_getting_flat_ = true;
        }
      } else {
        // Command for specific product
        for (int i = 0; i < num_underlying_; i++) {
          if (strcmp(_control_message_.strval_1_, underlying_data_vec_[i]->smv_->shortcode().c_str()) == 0) {
            if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
              DBGLOG_TIME_CLASS << "getflat_due_to_external_aggressive_getflat_ " << trader_id
                                << " for product: " << underlying_data_vec_[i]->smv_->shortcode() << DBGLOG_ENDL_FLUSH;
              if (livetrading_) {
                DBGLOG_DUMP;
              }
            }
            PassiveGetFlatTradingLogic(i, -1);
            underlying_data_vec_[i]->external_getflat_ = true;
            for (unsigned int j = 0; j < options_data_matrix_[i].size(); j++) {
              options_data_matrix_[i][j]->external_getflat_ = true;
              options_data_matrix_[i][j]->getflat_due_to_external_getflat_ = true;
              options_data_matrix_[i][j]->getflat_aggressive_ = true;
            }
            ProcessGetFlat(i);
            break;
          }
        }

        // No underlying matched , so command must be for individual contract
        for (int i = 0; i < num_underlying_; i++) {
          for (unsigned int j = 0; j < options_data_matrix_[i].size(); j++) {
            if (strcmp(_control_message_.strval_1_, options_data_matrix_[i][j]->smv_->shortcode().c_str()) == 0) {
              if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
                DBGLOG_TIME_CLASS << "getflat_due_to_external_aggressive_getflat_ " << trader_id
                                  << " for product: " << options_data_matrix_[i][j]->smv_->shortcode()
                                  << DBGLOG_ENDL_FLUSH;
                if (livetrading_) {
                  DBGLOG_DUMP;
                }
              }
              options_data_matrix_[i][j]->external_getflat_ = true;
              options_data_matrix_[i][j]->getflat_due_to_external_getflat_ = true;
              options_data_matrix_[i][j]->getflat_aggressive_ = true;
              ProcessGetFlat(i, j);
              break;
            }
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
        getting_flat_ = false;

        for (int i = 0; i < num_underlying_; i++) {
          underlying_data_vec_[i]->external_getflat_ = false;
          for (unsigned int j = 0; j < options_data_matrix_[i].size(); j++) {
            options_data_matrix_[i][j]->external_getflat_ = false;
            options_data_matrix_[i][j]->getflat_due_to_external_getflat_ = false;
            options_data_matrix_[i][j]->getflat_aggressive_ = false;
          }
          ProcessGetFlat(i);
        }

        aggressively_getting_flat_ = false;
      } else {
        // Command for specific product
        for (int i = 0; i < num_underlying_; i++) {
          if (strcmp(_control_message_.strval_1_, underlying_data_vec_[i]->smv_->shortcode().c_str()) == 0) {
            if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
              DBGLOG_TIME_CLASS << "start command " << trader_id
                                << " for product: " << underlying_data_vec_[i]->smv_->shortcode() << DBGLOG_ENDL_FLUSH;
              if (livetrading_) {
                DBGLOG_DUMP;
              }
            }
            underlying_data_vec_[i]->external_getflat_ = false;
            for (unsigned int j = 0; j < options_data_matrix_[i].size(); j++) {
              options_data_matrix_[i][j]->external_getflat_ = false;
              options_data_matrix_[i][j]->getflat_due_to_external_getflat_ = false;
              options_data_matrix_[i][j]->getflat_aggressive_ = false;
            }
            ProcessGetFlat(i);
            break;
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
    case kControlMessageCodeForceAllIndicatorReady: {
    } break;
    case kControlMessageCodeFreezeTrading:
    case kControlMessageCodeUnFreezeTrading:
    case kControlMessageCodeCancelAllFreezeTrading:
    case kControlMessageCodeSetTradeSizes:
    case kControlMessageCodeAddPosition: {
      if (strlen(_control_message_.strval_1_) == 0) {
        if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
          DBGLOG_TIME_CLASS << "AddPosition Called without shcname of filename : " << runtime_id_ << DBGLOG_ENDL_FLUSH;
          if (livetrading_) {
            DBGLOG_DUMP;
          }
        }
      } else {
        // Command for specific product
        for (int i = 0; i < num_underlying_; i++) {
          if (strcmp(_control_message_.strval_1_, underlying_data_vec_[i]->smv_->shortcode().c_str()) == 0) {
            if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
              DBGLOG_TIME_CLASS << "Add position command " << trader_id
                                << " for product: " << underlying_data_vec_[i]->smv_->shortcode() << DBGLOG_ENDL_FLUSH;
            }
            if (livetrading_) {
              DBGLOG_DUMP;
            }
            SetPositionOffset(_control_message_.intval_1_, i, -1, underlying_data_vec_[i]->smv_->mid_price());
            break;
          }

          for (unsigned int j = 0; j < options_data_matrix_[i].size(); j++) {
            if (strcmp(_control_message_.strval_1_, options_data_matrix_[i][j]->smv_->shortcode().c_str()) == 0) {
              if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
                DBGLOG_TIME_CLASS << "Add position command " << trader_id
                                  << " for product: " << options_data_matrix_[i][j]->smv_->shortcode()
                                  << DBGLOG_ENDL_FLUSH;
              }
              if (livetrading_) {
                DBGLOG_DUMP;
              }
              SetPositionOffset(_control_message_.intval_1_, i, j, options_data_matrix_[i][j]->smv_->mid_price());
              break;
            }
          }
        }

        // No shortcode matched ... This  may be a filename
        ifstream f(_control_message_.strval_1_);
        if (f.good()) {
          if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
            DBGLOG_TIME_CLASS << "Adding position to " << trader_id << " for all products from "
                              << _control_message_.strval_1_ << DBGLOG_ENDL_FLUSH;
          }
          if (livetrading_) {
            DBGLOG_DUMP;
          }

          LoadPositions(_control_message_.strval_1_);
        }
      }
    } break;
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
    case kControlMessageCodeShowIndicators:
    case kControlMessageDisableMarketManager:
    case kControlMessageEnableMarketManager:
    case kControlMessageCodeEnableLogging:
    case kControlMessageCodeDisableLogging:
      break;
    case kControlMessageCodeShowOrders:
    case kControlMessageCodeEnableZeroLoggingMode:
    case kControlMessageCodeSetStartTime:
    case kControlMessageCodeSetEndTime:
    case kControlMessageCodeDisableZeroLoggingMode: {
      if (livetrading_) {
        DBGLOG_DUMP;
      }
      break;
    }

    case kControlMessageCodeSetMaxLoss: {
      if (strlen(_control_message_.strval_1_) == 0) {
        if (_control_message_.intval_1_ > global_max_loss_ / FAT_FINGER_FACTOR &&
            _control_message_.intval_1_ < global_max_loss_ * FAT_FINGER_FACTOR) {
          global_max_loss_ = _control_message_.intval_1_;
          if (total_pnl_ >= -global_max_loss_) {
            is_max_loss_reached_global_ = false;
          }
        }

        if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
          DBGLOG_TIME_CLASS << "SetMaxLoss " << trader_id
                            << " called for abs_max_loss_ = " << _control_message_.intval_1_ << " and MaxLoss set to "
                            << global_max_loss_ << DBGLOG_ENDL_FLUSH;

          if (livetrading_) {
            DBGLOG_DUMP;
          }
        }
      } else {
        for (int i = 0; i < num_underlying_; i++) {
          if (strcmp(_control_message_.strval_1_, underlying_data_vec_[i]->smv_->shortcode().c_str()) == 0) {
            if (_control_message_.intval_1_ > paramset_vec_[i]->max_loss_ / FAT_FINGER_FACTOR &&
                _control_message_.intval_1_ < paramset_vec_[i]->max_loss_ * FAT_FINGER_FACTOR) {
              paramset_vec_[i]->max_loss_ = _control_message_.intval_1_;
              if (pnl_per_underlying_[i] >= -paramset_vec_[i]->global_max_loss_) {
                underlying_data_vec_[i]->getflat_due_to_max_loss_ = false;
              }

              if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
                DBGLOG_TIME_CLASS << "SetMaxLoss " << trader_id
                                  << " Product: " << underlying_data_vec_[i]->smv_->shortcode().c_str()
                                  << " called for abs_max_loss = " << _control_message_.intval_1_
                                  << " and MaxLoss set to " << paramset_vec_[i]->max_loss_ << DBGLOG_ENDL_FLUSH;
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

    case kControlMessageSetDeltaThreshold: {
      if (strlen(_control_message_.strval_1_) == 0)  // Its global command
      {
        for (int i = 0; i < num_underlying_; i++) {
          if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
            DBGLOG_TIME_CLASS << "set delta hedge threshold  " << trader_id
                              << " for product: " << underlying_data_vec_[i]->smv_->shortcode() << DBGLOG_ENDL_FLUSH;
            if (livetrading_) {
              DBGLOG_DUMP;
            }
          }
          lower_threshold_vec_[i] = _control_message_.intval_1_;
          paramset_vec_[i]->delta_hedge_logic_ = 2;
          GetDeltaHedgeLogic(i);
        }
      } else {
        // Command for specific product
        for (int i = 0; i < num_underlying_; i++) {
          if (strcmp(_control_message_.strval_1_, underlying_data_vec_[i]->smv_->shortcode().c_str()) == 0) {
            if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
              DBGLOG_TIME_CLASS << "set delta hedge threshold  " << trader_id
                                << " for product: " << underlying_data_vec_[i]->smv_->shortcode() << DBGLOG_ENDL_FLUSH;
              if (livetrading_) {
                DBGLOG_DUMP;
              }
            }
            paramset_vec_[i]->delta_hedge_logic_ = 2;
            lower_threshold_vec_[i] = _control_message_.intval_1_;
            GetDeltaHedgeLogic(i);
            break;
          }
        }
      }
    } break;

    case kControlMessageCodeSetMaxPosition: {  // Reducing max position by fraction of integer value

      if (_control_message_.intval_1_ == 0) {
        if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
          DBGLOG_TIME_CLASS << "set max position  can't be 0 " << trader_id << DBGLOG_ENDL_FLUSH;
          if (livetrading_) {
            DBGLOG_DUMP;
          }
        }
        break;
      }

      if (strlen(_control_message_.strval_1_) == 0)  // Its global command
      {
        for (int i = 0; i < num_underlying_; i++) {
          if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
            DBGLOG_TIME_CLASS << "set max position  " << trader_id
                              << " for product: " << underlying_data_vec_[i]->smv_->shortcode() << DBGLOG_ENDL_FLUSH;
            if (livetrading_) {
              DBGLOG_DUMP;
            }
          }
          for (unsigned int j = 0; j < options_data_matrix_[i].size(); j++) {
            options_risk_premium_->risk_matrix_[i][j].max_position_ /= _control_message_.intval_1_;
          }
        }
      } else {
        // Command for specific product
        for (int i = 0; i < num_underlying_; i++) {
          if (strcmp(_control_message_.strval_1_, underlying_data_vec_[i]->smv_->shortcode().c_str()) == 0) {
            if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
              DBGLOG_TIME_CLASS << "set max position  " << trader_id
                                << " for product: " << underlying_data_vec_[i]->smv_->shortcode() << DBGLOG_ENDL_FLUSH;
              if (livetrading_) {
                DBGLOG_DUMP;
              }
            }
            for (unsigned int j = 0; j < options_data_matrix_[i].size(); j++) {
              options_risk_premium_->risk_matrix_[i][j].max_position_ /= _control_message_.intval_1_;
            }
            break;
          }
        }
        // No underlying matched , so command must be for individual contract
        for (int i = 0; i < num_underlying_; i++) {
          for (unsigned int j = 0; j < options_data_matrix_[i].size(); j++) {
            if (strcmp(_control_message_.strval_1_, underlying_data_vec_[i]->smv_->shortcode().c_str()) == 0) {
              if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
                DBGLOG_TIME_CLASS << "set max position  " << trader_id
                                  << " for product: " << underlying_data_vec_[i]->smv_->shortcode()
                                  << DBGLOG_ENDL_FLUSH;
                if (livetrading_) {
                  DBGLOG_DUMP;
                }
              }
              options_risk_premium_->risk_matrix_[i][j].max_position_ /= _control_message_.intval_1_;
              break;
            }
          }
        }
      }
    } break;

    case kControlMessageCodeSetOpenTradeLoss:  // TODO :: Add opentradeloss control message functionality here
    default:
      break;
  }
}

void OptionsRiskManager::GetFlatTradingLogic(int _product_index_, int _option_index_) {
  if ((_option_index_ < 0) && (paramset_vec_[_product_index_]->delta_hedge_logic_ != -_option_index_)) return;

  OptionVars* current_exec_vars_ = (_option_index_ < 0 ? underlying_data_vec_[_product_index_]
                                                       : options_data_matrix_[_product_index_][_option_index_]);

  if (current_exec_vars_->getflat_aggressive_)
    AggressiveGetFlatTradingLogic(_product_index_, _option_index_);
  else
    PassiveGetFlatTradingLogic(_product_index_, _option_index_);
}

void OptionsRiskManager::ReportResults(HFSAT::BulkFileWriter& trades_writer_, bool _conservative_close_) {
  int t_total_volume_ = 0;
  int t_total_lots_ = 0;
  int t_supporting_orders_filled_ = 0;
  int t_bestlevel_orders_filled_ = 0;
  int t_aggressive_orders_filled_ = 0;
  int t_improve_orders_filled_ = 0;
  int t_total_pnl_ = 0;
  // t_total_pnl_ = total_pnl_;

  for (int index_ = 0; index_ < num_underlying_; index_++) {
    for (unsigned int const_index_ = 0; const_index_ < options_data_matrix_[index_].size(); const_index_++) {
      DBGLOG_TIME_CLASS_FUNC_LINE << " Product: " << options_data_matrix_[index_][const_index_]->smv_->shortcode()
                                  << " " << options_data_matrix_[index_][const_index_]->position_ << " "
                                  << options_data_matrix_[index_][const_index_]->smv_->mid_price() << " "
                                  << options_data_matrix_[index_][const_index_]->som_->base_pnl().total_pnl() << " : "
                                  << DBGLOG_ENDL_FLUSH;
    }
  }

  std::cout << std::fixed;
  std::cout << std::setprecision(2);

  for (int index_ = 0; index_ < num_underlying_; index_++) {
    int lots_underlying_ = 0;
    double lots_underlying_delta_normalized_ = 0;
    int pnl_underlying_ = 0;
    int supporting_orders_filled_underlying_ = 0;
    int bestlevel_orders_filled_underlying_ = 0;
    int improve_orders_filled__underlying_ = 0;
    int aggressive_orders_filled_underlying_ = 0;

    for (unsigned int const_index_ = 0; const_index_ < options_data_matrix_[index_].size(); const_index_++) {
      if (options_data_matrix_[index_][const_index_]->som_ != NULL) {
        t_total_pnl_ += options_data_matrix_[index_][const_index_]->som_->base_pnl().total_pnl();
        pnl_underlying_ += options_data_matrix_[index_][const_index_]->som_->base_pnl().total_pnl();

        SmartOrderManager& t_order_manager_ = *options_data_matrix_[index_][const_index_]->som_;
        t_total_volume_ += t_order_manager_.trade_volume();
        t_total_lots_ +=
            t_order_manager_.trade_volume() / options_data_matrix_[index_][const_index_]->smv_->min_order_size_;
        lots_underlying_ +=
            t_order_manager_.trade_volume() / options_data_matrix_[index_][const_index_]->smv_->min_order_size_;
        lots_underlying_delta_normalized_ +=
            (t_order_manager_.trade_volume() / options_data_matrix_[index_][const_index_]->smv_->min_order_size_) *
            std::fabs(options_data_matrix_[index_][const_index_]->option_->greeks_.delta_);

        t_supporting_orders_filled_ +=
            t_order_manager_.SupportingOrderFilledPercent() *
            (t_order_manager_.trade_volume() / options_data_matrix_[index_][const_index_]->smv_->min_order_size_);
        supporting_orders_filled_underlying_ +=
            t_order_manager_.SupportingOrderFilledPercent() * t_order_manager_.trade_volume();
        t_bestlevel_orders_filled_ +=
            t_order_manager_.BestLevelOrderFilledPercent() *
            (t_order_manager_.trade_volume() / options_data_matrix_[index_][const_index_]->smv_->min_order_size_);
        bestlevel_orders_filled_underlying_ +=
            t_order_manager_.BestLevelOrderFilledPercent() * t_order_manager_.trade_volume();
        t_improve_orders_filled_ +=
            t_order_manager_.ImproveOrderFilledPercent() *
            (t_order_manager_.trade_volume() / options_data_matrix_[index_][const_index_]->smv_->min_order_size_);
        improve_orders_filled__underlying_ +=
            t_order_manager_.ImproveOrderFilledPercent() * t_order_manager_.trade_volume();
        t_aggressive_orders_filled_ +=
            t_order_manager_.AggressiveOrderFilledPercent() *
            (t_order_manager_.trade_volume() / options_data_matrix_[index_][const_index_]->smv_->min_order_size_);
        aggressive_orders_filled_underlying_ +=
            t_order_manager_.AggressiveOrderFilledPercent() * t_order_manager_.trade_volume();

        std::cout << options_data_matrix_[index_][const_index_]->smv_->shortcode() << " "
                  << t_order_manager_.base_pnl().total_pnl() << " "
                  << t_order_manager_.trade_volume() / options_data_matrix_[index_][const_index_]->smv_->min_order_size_
                  << " " << (int)t_order_manager_.SupportingOrderFilledPercent() << " "
                  << (int)t_order_manager_.BestLevelOrderFilledPercent() << " "
                  << t_order_manager_.ImproveOrderFilledPercent() << " "
                  << t_order_manager_.AggressiveOrderFilledPercent() << " "
                  << options_data_matrix_[index_][const_index_]->min_pnl_ << std::endl;
        if (livetrading_) {
          dbglogger_ << options_data_matrix_[index_][const_index_]->smv_->shortcode() << " "
                     << t_order_manager_.base_pnl().total_pnl() << " "
                     << t_order_manager_.trade_volume() /
                            options_data_matrix_[index_][const_index_]->smv_->min_order_size_
                     << " " << (int)t_order_manager_.SupportingOrderFilledPercent() << " "
                     << (int)t_order_manager_.BestLevelOrderFilledPercent() << " "
                     << t_order_manager_.ImproveOrderFilledPercent() << " "
                     << t_order_manager_.AggressiveOrderFilledPercent() << " "
                     << options_data_matrix_[index_][const_index_]->min_pnl_ << "\n";
        } else {
          trades_writer_ << "SIMRESULT " << runtime_id_ << " "
                         << options_data_matrix_[index_][const_index_]->smv_->shortcode() << " "
                         << t_order_manager_.base_pnl().total_pnl() << " "
                         << t_order_manager_.trade_volume() /
                                options_data_matrix_[index_][const_index_]->smv_->min_order_size_
                         << " " << (int)t_order_manager_.SupportingOrderFilledPercent() << " "
                         << (int)t_order_manager_.BestLevelOrderFilledPercent() << " "
                         << t_order_manager_.ImproveOrderFilledPercent() << " "
                         << t_order_manager_.AggressiveOrderFilledPercent() << " "
                         << options_data_matrix_[index_][const_index_]->min_pnl_ << "\n";
        }
      }
    }
    t_total_pnl_ += order_manager_vec_[index_]->base_pnl().total_pnl();
    pnl_underlying_ += order_manager_vec_[index_]->base_pnl().total_pnl();

    pnl_per_underlying_[index_] = pnl_underlying_;

    SmartOrderManager& t_order_manager_ = *order_manager_vec_[index_];
    t_total_volume_ += t_order_manager_.trade_volume();
    t_total_lots_ += t_order_manager_.trade_volume() / underlying_market_view_vec_[index_]->min_order_size_;
    lots_underlying_ += t_order_manager_.trade_volume() / underlying_market_view_vec_[index_]->min_order_size_;
    lots_underlying_delta_normalized_ +=
        t_order_manager_.trade_volume() / underlying_market_view_vec_[index_]->min_order_size_;

    t_supporting_orders_filled_ +=
        t_order_manager_.SupportingOrderFilledPercent() *
        (t_order_manager_.trade_volume() / underlying_market_view_vec_[index_]->min_order_size_);
    supporting_orders_filled_underlying_ +=
        t_order_manager_.SupportingOrderFilledPercent() * t_order_manager_.trade_volume();
    t_bestlevel_orders_filled_ +=
        t_order_manager_.BestLevelOrderFilledPercent() *
        (t_order_manager_.trade_volume() / underlying_market_view_vec_[index_]->min_order_size_);
    bestlevel_orders_filled_underlying_ +=
        t_order_manager_.BestLevelOrderFilledPercent() * t_order_manager_.trade_volume();
    t_improve_orders_filled_ +=
        t_order_manager_.ImproveOrderFilledPercent() *
        (t_order_manager_.trade_volume() / underlying_market_view_vec_[index_]->min_order_size_);
    improve_orders_filled__underlying_ +=
        t_order_manager_.ImproveOrderFilledPercent() * t_order_manager_.trade_volume();
    t_aggressive_orders_filled_ +=
        t_order_manager_.AggressiveOrderFilledPercent() *
        (t_order_manager_.trade_volume() / underlying_market_view_vec_[index_]->min_order_size_);
    aggressive_orders_filled_underlying_ +=
        t_order_manager_.AggressiveOrderFilledPercent() * t_order_manager_.trade_volume();

    std::cout << underlying_market_view_vec_[index_]->shortcode() << " " << t_order_manager_.base_pnl().total_pnl()
              << " " << t_order_manager_.trade_volume() / underlying_market_view_vec_[index_]->min_order_size_ << " "
              << (int)t_order_manager_.SupportingOrderFilledPercent() << " "
              << (int)t_order_manager_.BestLevelOrderFilledPercent() << " "
              << t_order_manager_.ImproveOrderFilledPercent() << " " << t_order_manager_.AggressiveOrderFilledPercent()
              << " " << underlying_data_vec_[index_]->min_pnl_ << std::endl;
    if (livetrading_) {
      dbglogger_ << underlying_market_view_vec_[index_]->shortcode() << " " << t_order_manager_.base_pnl().total_pnl()
                 << " " << t_order_manager_.trade_volume() / underlying_market_view_vec_[index_]->min_order_size_ << " "
                 << (int)t_order_manager_.SupportingOrderFilledPercent() << " "
                 << (int)t_order_manager_.BestLevelOrderFilledPercent() << " "
                 << t_order_manager_.ImproveOrderFilledPercent() << " "
                 << t_order_manager_.AggressiveOrderFilledPercent() << " " << underlying_data_vec_[index_]->min_pnl_
                 << "\n";
    } else {
      trades_writer_ << "SIMRESULT " << runtime_id_ << " " << underlying_market_view_vec_[index_]->shortcode() << " "
                     << t_order_manager_.base_pnl().total_pnl() << " "
                     << t_order_manager_.trade_volume() / underlying_market_view_vec_[index_]->min_order_size_ << " "
                     << (int)t_order_manager_.SupportingOrderFilledPercent() << " "
                     << (int)t_order_manager_.BestLevelOrderFilledPercent() << " "
                     << t_order_manager_.ImproveOrderFilledPercent() << " "
                     << t_order_manager_.AggressiveOrderFilledPercent() << " " << underlying_data_vec_[index_]->min_pnl_
                     << "\n";
    }

    if (lots_underlying_ > 0) {
      supporting_orders_filled_underlying_ /= lots_underlying_ * underlying_market_view_vec_[index_]->min_order_size_;
      bestlevel_orders_filled_underlying_ /= lots_underlying_ * underlying_market_view_vec_[index_]->min_order_size_;
      improve_orders_filled__underlying_ /= lots_underlying_ * underlying_market_view_vec_[index_]->min_order_size_;
      aggressive_orders_filled_underlying_ /= lots_underlying_ * underlying_market_view_vec_[index_]->min_order_size_;
    }

    std::string shc_ = (underlying_market_view_vec_[index_]->shortcode())
                           .substr(4, underlying_market_view_vec_[index_]->shortcode().size() - 9);

    std::cout << shc_ << " " << pnl_underlying_ << " " << lots_underlying_delta_normalized_ << " "
              << supporting_orders_filled_underlying_ << " " << bestlevel_orders_filled_underlying_ << " "
              << improve_orders_filled__underlying_ << " " << aggressive_orders_filled_underlying_ << " "
              << min_pnl_per_underlying_[index_] << std::endl;

    if (livetrading_) {
      dbglogger_ << shc_ << " " << pnl_underlying_ << " " << lots_underlying_delta_normalized_ << " "
                 << supporting_orders_filled_underlying_ << " " << bestlevel_orders_filled_underlying_ << " "
                 << improve_orders_filled__underlying_ << " " << aggressive_orders_filled_underlying_ << " "
                 << min_pnl_per_underlying_[index_] << "\n";
    } else {
      trades_writer_ << "SIMRESULT " << runtime_id_ << " " << shc_ << " " << pnl_underlying_ << " "
                     << lots_underlying_delta_normalized_ << " " << supporting_orders_filled_underlying_ << " "
                     << bestlevel_orders_filled_underlying_ << " " << improve_orders_filled__underlying_ << " "
                     << aggressive_orders_filled_underlying_ << " " << min_pnl_per_underlying_[index_] << "\n";
    }
  }

  if (t_total_volume_ > 0) {
    t_supporting_orders_filled_ /= t_total_lots_;
    t_bestlevel_orders_filled_ /= t_total_lots_;
    t_improve_orders_filled_ /= t_total_lots_;
    t_aggressive_orders_filled_ /= t_total_lots_;
  }

  trades_writer_ << "SIMRESULT " << runtime_id_ << " ALL " << t_total_pnl_ << " " << t_total_lots_ << " "
                 << t_supporting_orders_filled_ << " " << t_bestlevel_orders_filled_ << " "
                 << t_aggressive_orders_filled_ << " " << t_improve_orders_filled_ << " " << min_pnl_ << "\n";

  if (livetrading_) {
    DBGLOG_DUMP;
  } else {
    trades_writer_ << "\n";
    trades_writer_.DumpCurrentBuffer();
  }

  int num_messages_ = 0;

  for (int index_ = 0; index_ < num_underlying_; index_++) {
    for (unsigned int const_index_ = 0; const_index_ < options_data_matrix_[index_].size(); const_index_++) {
      if (options_data_matrix_[index_][const_index_]->som_ != NULL) {
        double percent_mkt_volume_traded_ = 0;
        if (options_data_matrix_[index_][const_index_]->total_volume_traded_ != 0) {
          percent_mkt_volume_traded_ = options_data_matrix_[index_][const_index_]->som_->trade_volume() /
                                       options_data_matrix_[index_][const_index_]->total_volume_traded_;
        }

        SmartOrderManager& t_order_manager_ = *options_data_matrix_[index_][const_index_]->som_;
        num_messages_ += (t_order_manager_.SendOrderCount() + t_order_manager_.CxlOrderCount() +
                          t_order_manager_.ModifyOrderCount());
        if (livetrading_) {
          dbglogger_ << "STATS: " << options_data_matrix_[index_][const_index_]->smv_->shortcode() << "." << runtime_id_
                     << " EOD_MSG_COUNT: " << (t_order_manager_.SendOrderCount() + t_order_manager_.CxlOrderCount() +
                                               t_order_manager_.ModifyOrderCount())
                     << " PERCENT_MKT_VOL_TRADED: "
                     << "0 OTL_HIT: " << options_data_matrix_[index_][const_index_]->num_opentrade_loss_hits_
                     << " UTS: "
                     << paramset_vec_[index_]->unit_trade_size_ /
                            options_data_matrix_[index_][const_index_]->smv_->min_order_size_
                     << " LOTSIZE: " << options_data_matrix_[index_][const_index_]->smv_->min_order_size_ << "\n";
        } else {
          trades_writer_ << "STATS: " << options_data_matrix_[index_][const_index_]->smv_->shortcode() << "."
                         << runtime_id_ << " EOD_MSG_COUNT: "
                         << (t_order_manager_.SendOrderCount() + t_order_manager_.CxlOrderCount() +
                             t_order_manager_.ModifyOrderCount())
                         << " PERCENT_MKT_VOL_TRADED: " << percent_mkt_volume_traded_
                         << " OTL_HIT: " << options_data_matrix_[index_][const_index_]->num_opentrade_loss_hits_
                         << " UTS: "
                         << paramset_vec_[index_]->unit_trade_size_ /
                                options_data_matrix_[index_][const_index_]->smv_->min_order_size_
                         << " LOTSIZE: " << options_data_matrix_[index_][const_index_]->smv_->min_order_size_ << "\n";
        }
      }
    }
    SmartOrderManager& t_order_manager_ = *order_manager_vec_[index_];
    num_messages_ +=
        (t_order_manager_.SendOrderCount() + t_order_manager_.CxlOrderCount() + t_order_manager_.ModifyOrderCount());
    if (livetrading_) {
      dbglogger_ << "STATS: " << underlying_market_view_vec_[index_]->shortcode() << "." << runtime_id_
                 << " EOD_MSG_COUNT: " << (t_order_manager_.SendOrderCount() + t_order_manager_.CxlOrderCount() +
                                           t_order_manager_.ModifyOrderCount())
                 << " PERCENT_MKT_VOL_TRADED: "
                 << "0 OTL_HIT: 0 UTS: "
                 << paramset_vec_[index_]->unit_trade_size_ / underlying_market_view_vec_[index_]->min_order_size_
                 << " LOTSIZE: " << underlying_market_view_vec_[index_]->min_order_size_ << "\n";
    } else {
      trades_writer_ << "STATS: " << underlying_market_view_vec_[index_]->shortcode() << "." << runtime_id_
                     << " EOD_MSG_COUNT: " << (t_order_manager_.SendOrderCount() + t_order_manager_.CxlOrderCount() +
                                               t_order_manager_.ModifyOrderCount())
                     << " PERCENT_MKT_VOL_TRADED: "
                     << "0 OTL_HIT: 0 UTS: "
                     << paramset_vec_[index_]->unit_trade_size_ / underlying_market_view_vec_[index_]->min_order_size_
                     << " LOTSIZE: " << underlying_market_view_vec_[index_]->min_order_size_ << "\n";
    }
  }

  if (!livetrading_) {
    if (pnl_samples_.size() > 0) {
      for (int index_ = 0; index_ < num_underlying_; index_++) {
        std::string shc_ = (underlying_market_view_vec_[index_]->shortcode())
                               .substr(4, underlying_market_view_vec_[index_]->shortcode().size() - 9);
        trades_writer_ << "PNLSAMPLES " << shc_ << "." << runtime_id_ << " ";
        for (auto i = 0u; i < pnl_samples_vec_[index_].size(); i++) {
          trades_writer_ << pnl_sampling_timestamps_[i] << " " << pnl_samples_vec_[index_][i] << " ";
        }
        trades_writer_ << "\n";
      }
      trades_writer_ << "PNLSAMPLES "
                     << "ALL"
                     << "." << runtime_id_ << " ";
      for (auto i = 0u; i < pnl_samples_.size(); i++) {
        trades_writer_ << pnl_sampling_timestamps_[i] << " " << pnl_samples_[i] << " ";
      }
      trades_writer_ << "\n";
    } else {
      for (int index_ = 0; index_ < num_underlying_; index_++) {
        std::string shc_ = (underlying_market_view_vec_[index_]->shortcode())
                               .substr(4, underlying_market_view_vec_[index_]->shortcode().size() - 9);
        trades_writer_ << "PNLSAMPLES " << shc_ << "." << runtime_id_ << " " << trading_end_utc_mfm_ << " "
                       << pnl_per_underlying_[index_] << "\n";
      }
      trades_writer_ << "PNLSAMPLES "
                     << "ALL"
                     << "." << runtime_id_ << " " << trading_end_utc_mfm_ << " " << t_total_pnl_ << "\n";
    }
  }

  if (livetrading_) {
    DBGLOG_DUMP;
  } else {
    trades_writer_.DumpCurrentBuffer();
  }

  printf("SIMRESULT %d %d %d %d %d %d\n", t_total_pnl_, t_total_lots_, t_supporting_orders_filled_,
         t_bestlevel_orders_filled_, t_aggressive_orders_filled_, t_improve_orders_filled_);
}

void OptionsRiskManager::LoadPositions(std::string position_file_) {
  std::ifstream open_position_file;
  open_position_file.open(position_file_.c_str(), std::ifstream::in);
  if (open_position_file.is_open()) {
    while (open_position_file.good()) {
      const int kL1AvgBufferLen = 1024;
      char readline_buffer_[kL1AvgBufferLen];
      bzero(readline_buffer_, kL1AvgBufferLen);
      open_position_file.getline(readline_buffer_, kL1AvgBufferLen);
      PerishableStringTokenizer st_(readline_buffer_, 1024);
      const std::vector<const char*>& tokens_ = st_.GetTokens();

      if (tokens_.size() == 0) continue;

      if (strncmp(tokens_[0], "SHC:", 4) != 0) continue;

      int position_ = atoi(tokens_[5]);
      double last_traded_price_ = atof(tokens_[7]);
      int sec_id_ = sec_name_indexer_.GetIdFromSecname(tokens_[3]);

      if ((sec_id_ == -1) || (security_id_prod_idx_map_.find(sec_id_) == security_id_prod_idx_map_.end())) {
        DBGLOG_TIME_CLASS_FUNC_LINE << "Load Positions failed for " << tokens_[3] << " " << position_
                                    << " since we are not trading this today." << DBGLOG_ENDL_FLUSH;
        continue;
      }

      std::pair<int, int> indexes_ = security_id_prod_idx_map_[sec_id_];
      SetPositionOffset(position_, indexes_.first, indexes_.second, last_traded_price_);
    }
  }
}

bool OptionsRiskManager::SetPositionOffset(int t_position_offset_, int _product_index_, int _option_index_,
                                           double t_exec_price_) {
  OptionVars* current_exec_vars_;
  if (_option_index_ < 0)
    current_exec_vars_ = underlying_data_vec_[_product_index_];
  else
    current_exec_vars_ = options_data_matrix_[_product_index_][_option_index_];

  if (!current_exec_vars_->is_ready_) {
    DBGLOG_TIME_CLASS_FUNC_LINE << "Exec Vars Not Ready : SetPositionOffset Failed for"
                                << current_exec_vars_->smv_->shortcode() << " " << t_position_offset_
                                << DBGLOG_ENDL_FLUSH;
    return false;
  }

  DBGLOG_TIME_CLASS_FUNC_LINE << "SetPositionOffset " << current_exec_vars_->smv_->shortcode() << " "
                              << t_position_offset_ << DBGLOG_ENDL_FLUSH;

  t_position_offset_ = MathUtils::GetFlooredMultipleOf(t_position_offset_, current_exec_vars_->smv_->min_order_size());

  char hostname_[128];
  hostname_[127] = '\0';
  gethostname(hostname_, 127);
  std::ostringstream t_oss_;

  if (t_position_offset_ == 0) {
    return true;
  } else {
    current_exec_vars_->som_->AddPosition(t_position_offset_, t_exec_price_);
  }

  return true;
}

void OptionsRiskManager::SendMail(std::string _mail_content_, std::string _mail_subject_) {
  HFSAT::Email email_;
  email_.setSubject(_mail_subject_);
  email_.addRecepient("nseall@tworoads.co.in");
  email_.addSender("nseall@tworoads.co.in");
  email_.content_stream << _mail_content_ << "<br/>";
  email_.sendMail();
}
}
