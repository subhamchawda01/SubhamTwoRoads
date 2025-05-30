/**
   \file ExecLogic/options_exec_vars.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */
#pragma once

#include "dvctrade/InitCommon/options_paramset.hpp"
#include "dvctrade/OptionsHelper/option.hpp"
#include "baseinfra/SmartOrderRouting/smart_order_manager.hpp"
#include "baseinfra/OptionsUtils/option_object.hpp"

namespace HFSAT {

class OptionsRiskChangeListener {
 public:
  virtual ~OptionsRiskChangeListener() {}
  virtual void OnOptionRiskChange(unsigned int _security_id_, double change_delta_, double change_gamma_,
                                  double change_vega_, double change_theta_) = 0;
};

class OptionVars : public TimePeriodListener {
 public:
  OptionVars(DebugLogger& _dbglogger_, const Watch& _watch_, SecurityMarketView* _smv_, SecurityMarketView* _fut_smv_,
             SmartOrderManager* _som_, int _trading_start_utc_mfm_, int _trading_end_utc_mfm_)
      : watch_(_watch_),
        smv_(_smv_),
        fut_smv_(_fut_smv_),
        som_(_som_),
        external_getflat_(true),
        getflat_due_to_external_getflat_(false),
        getflat_due_to_max_loss_(false),
        getflat_due_to_close_(false),
        enable_market_data_interrupt_(true),
        getflat_due_to_market_data_interrupt_(false),
        getflat_due_to_max_opentrade_loss_(false),
        getflat_aggressive_(false),
        should_be_getting_flat_(false),
        old_should_be_getting_flat_(false),
        getflat_due_to_min_int_price_(false),
        is_ready_(false),
        best_nonself_bid_int_price_(-1),
        best_nonself_ask_int_price_(-1),
        best_nonself_bid_size_(-1),
        best_nonself_ask_size_(-1),
        best_nonself_bid_price_(0.0),
        best_nonself_ask_price_(0.0),
        last_buy_int_price_(0),
        last_sell_int_price_(0),
        last_buy_msecs_(0),
        last_sell_msecs_(0),
        last_agg_buy_msecs_(0),
        last_agg_sell_msecs_(0),
        last_getflat_due_to_max_opentrade_loss_triggered_at_msecs_(0),
        num_opentrade_loss_hits_(0),
        position_(0),
        total_pnl_(0.0),
        min_pnl_(0.0),
        option_(nullptr),
        last_traded_price_(0.0),
        total_volume_traded_(0),
        trading_start_utc_mfm_(_trading_start_utc_mfm_),
        trading_end_utc_mfm_(_trading_end_utc_mfm_),
        risk_change_listener_() {
    watch_.subscribe_FifteenMinutesPeriod(this);
    option_ = HFSAT::OptionObject::GetUniqueInstance(_dbglogger_, watch_, smv_->shortcode());
  }

  // Constructor for Variables for future
  OptionVars(DebugLogger& _dbglogger_, const Watch& _watch_, SecurityMarketView* _smv_, SmartOrderManager* _som_,
             int _trading_start_utc_mfm_, int _trading_end_utc_mfm_)
      : watch_(_watch_),
        smv_(_smv_),
        fut_smv_(nullptr),
        som_(_som_),
        external_getflat_(true),
        getflat_due_to_external_getflat_(false),
        getflat_due_to_max_loss_(false),
        getflat_due_to_close_(false),
        enable_market_data_interrupt_(true),
        getflat_due_to_market_data_interrupt_(false),
        getflat_due_to_max_opentrade_loss_(false),
        getflat_aggressive_(false),
        should_be_getting_flat_(false),
        old_should_be_getting_flat_(false),
        getflat_due_to_min_int_price_(false),
        is_ready_(false),
        best_nonself_bid_int_price_(-1),
        best_nonself_ask_int_price_(-1),
        best_nonself_bid_size_(-1),
        best_nonself_ask_size_(-1),
        best_nonself_bid_price_(0.0),
        best_nonself_ask_price_(0.0),
        last_buy_int_price_(0),
        last_sell_int_price_(0),
        last_buy_msecs_(0),
        last_sell_msecs_(0),
        last_agg_buy_msecs_(0),
        last_agg_sell_msecs_(0),
        last_getflat_due_to_max_opentrade_loss_triggered_at_msecs_(0),
        num_opentrade_loss_hits_(0),
        position_(0),
        total_pnl_(0.0),
        min_pnl_(0.0),
        option_(nullptr),
        last_traded_price_(0.0),
        total_volume_traded_(0),
        trading_start_utc_mfm_(_trading_start_utc_mfm_),
        trading_end_utc_mfm_(_trading_end_utc_mfm_),
        risk_change_listener_() {}

  const Watch& watch_;

  SecurityMarketView* smv_;
  SecurityMarketView* fut_smv_;
  SmartOrderManager* som_;

  bool external_getflat_;
  bool getflat_due_to_external_getflat_;
  bool getflat_due_to_max_loss_;
  bool getflat_due_to_close_;
  bool enable_market_data_interrupt_;
  bool getflat_due_to_market_data_interrupt_;
  bool getflat_due_to_max_opentrade_loss_;
  bool getflat_aggressive_;
  bool should_be_getting_flat_;
  bool old_should_be_getting_flat_;
  bool getflat_due_to_min_int_price_;
  bool is_ready_;

  int best_nonself_bid_int_price_;
  int best_nonself_ask_int_price_;
  int best_nonself_bid_size_;
  int best_nonself_ask_size_;
  double best_nonself_bid_price_;
  double best_nonself_ask_price_;

  int last_buy_int_price_;
  int last_sell_int_price_;
  int last_buy_msecs_;
  int last_sell_msecs_;

  int last_agg_buy_msecs_;
  int last_agg_sell_msecs_;

  int last_getflat_due_to_max_opentrade_loss_triggered_at_msecs_;
  int num_opentrade_loss_hits_;

  int position_;

  double total_pnl_;
  double min_pnl_;
  OptionObject* option_;
  double last_traded_price_;
  double total_volume_traded_;  // To keep track of market volume during the time we traded (only computed on SIM)

  int trading_start_utc_mfm_;
  int trading_end_utc_mfm_;

  OptionsRiskChangeListener* risk_change_listener_;  // Risk Manager (Only one listener now. Can be changed to multiple)

  void Initialize() {
    is_ready_ = true;
    OnTimePeriodUpdate(0);  // This will initialize the tradevarset
  }

  void AddRiskChangeListener(OptionsRiskChangeListener* _risk_change_listener_) {
    risk_change_listener_ = _risk_change_listener_;
  }

  //  Greeks are calculated every fifteen minutes (Can be improved to include huge future and option price move)
  void OnTimePeriodUpdate(const int num_pages_to_add_) {
    double old_delta_ = option_->greeks_.delta_;
    double old_gamma_ = option_->greeks_.gamma_;
    double old_vega_ = option_->greeks_.vega_;
    double old_theta_ = option_->greeks_.theta_;

    if (is_ready_ == true) {
      option_->ComputeGreeks((fut_smv_->bestbid_price() + fut_smv_->bestask_price()) / 2,
                             (smv_->bestbid_price() + smv_->bestask_price()) / 2);
      risk_change_listener_->OnOptionRiskChange(
          smv_->security_id(), option_->greeks_.delta_ - old_delta_, option_->greeks_.gamma_ - old_gamma_,
          option_->greeks_.vega_ - old_vega_, option_->greeks_.theta_ - old_theta_);
    }
  }
};
}
