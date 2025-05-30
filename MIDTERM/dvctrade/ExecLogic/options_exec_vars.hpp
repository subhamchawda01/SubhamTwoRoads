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

namespace HFSAT {
struct OptionsExecVars {
 public:
  OptionsExecVars()
      : smv_(nullptr),
        som_(nullptr),
        getflat_due_to_external_getflat_(false),
        getflat_due_to_max_loss_(false),
        getflat_due_to_market_status_(false),
        getflat_due_to_close_(false),
        getflat_due_to_max_opentrade_loss_(false),
        allowed_to_cancel_orders_(true),
        getflat_aggressive_(false),
        should_be_getting_flat_(false),
        old_should_be_getting_flat_(false),
        is_ready_(false),
        size_to_join_(0),
        best_nonself_bid_int_price_(-1),
        best_nonself_ask_int_price_(-1),
        best_nonself_bid_size_(-1),
        best_nonself_ask_size_(-1),
        last_buy_int_price_(0),
        last_sell_int_price_(0),
        last_buy_msecs_(0),
        last_sell_msecs_(0),
        last_agg_buy_msecs_(0),
        last_agg_sell_msecs_(0),
        last_imp_buy_msecs_(0),
        last_imp_sell_msecs_(0),
        last_bid_agg_msecs_(0),
        last_ask_agg_msecs_(0),
        last_bid_imp_msecs_(0),
        last_ask_imp_msecs_(0),
        best_nonself_bid_price_(0.0),
        best_nonself_ask_price_(0.0),
        best_nonself_mid_price_(0.0),
        last_getflat_due_to_max_opentrade_loss_triggered_at_msecs_(0),
        num_opentrade_loss_hits_(0),
        place_at_stable_prices_(false),
        spread_(0.0),
        option_type_(HFSAT::OptionType_t::CALL),
        strike_(0.0),
        position_(0),
        greek_adjusted_position_factor_(1),
        option_vars(nullptr),
        moving_avg_spread_(-1),
        trading_start_utc_mfm_(0),
        trading_end_utc_mfm_(0),
        trading_status_(kMktTradingStatusOpen) {}

  SecurityMarketView* smv_;
  SmartOrderManager* som_;
  std::string shortcode_;

  bool getflat_due_to_external_getflat_;
  bool getflat_due_to_max_loss_;
  bool getflat_due_to_market_status_;
  bool getflat_due_to_close_;
  bool getflat_due_to_max_opentrade_loss_;
  bool allowed_to_cancel_orders_;
  bool getflat_aggressive_;
  bool should_be_getting_flat_;
  bool old_should_be_getting_flat_;
  bool is_ready_;

  int size_to_join_;
  int best_nonself_bid_int_price_;
  int best_nonself_ask_int_price_;
  int best_nonself_bid_size_;
  int best_nonself_ask_size_;

  int last_buy_int_price_;
  int last_sell_int_price_;
  int last_buy_msecs_;
  int last_sell_msecs_;

  int last_agg_buy_msecs_;
  int last_agg_sell_msecs_;

  int last_imp_buy_msecs_;
  int last_imp_sell_msecs_;

  int last_bid_agg_msecs_;
  int last_ask_agg_msecs_;
  int last_bid_imp_msecs_;
  int last_ask_imp_msecs_;

  double best_nonself_bid_price_;
  double best_nonself_ask_price_;
  double best_nonself_mid_price_;

  int last_getflat_due_to_max_opentrade_loss_triggered_at_msecs_;
  int num_opentrade_loss_hits_;

  bool place_at_stable_prices_;

  double spread_;
  OptionType_t option_type_;
  double strike_;

  int position_;
  int greek_adjusted_position_factor_;
  Option* option_vars;

  double moving_avg_spread_;

  int trading_start_utc_mfm_;
  int trading_end_utc_mfm_;

  MktStatus_t trading_status_;
};
}
