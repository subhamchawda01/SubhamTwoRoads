// =====================================================================================
//
//       Filename:  instrument_info.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  03/12/2015 12:49:19 PM
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

/*
 * This contains all the information related to the product being traded
 */

#pragma once

#include <stdlib.h>

#include "baseinfra/MarketAdapter/security_market_view.hpp"
#include "baseinfra/SmartOrderRouting/smart_order_manager.hpp"
#include "dvctrade/ModelMath/base_model_math.hpp"
#include "dvctrade/ExecLogic/trade_vars.hpp"

namespace HFSAT {

struct InstrumentInfo {
 public:
  InstrumentInfo()
      : market_view_(nullptr),
        order_manager_(nullptr),
        model_math_(nullptr),
        throttle_manager_(nullptr),
        trading_start_utc_mfm_(0),
        trading_end_utc_mfm_(0),
        getflat_mult_order_(false),
        getflat_due_to_external_getflat_(false),
        getflat_due_to_close_(false),
        getflat_due_to_max_loss_(false),
        getflat_due_to_market_status_(false),
        getflat_aggressive_(false),
        allowed_to_cancel_orders_(true),
        is_trading_today_(true),
        place_at_stable_prices_(false),
        size_to_join_(0),
        best_nonself_bid_int_price_(0),
        best_nonself_ask_int_price_(100000),
        best_nonself_bid_size_(0),
        best_nonself_ask_size_(0),
        last_buy_int_price_(0),
        last_sell_int_price_(0),
        last_buy_msecs_(0),
        last_sell_msecs_(0),
        last_agg_buy_msecs_(0),
        last_agg_sell_msecs_(0),
        last_bid_agg_msecs_(0),
        last_ask_agg_msecs_(0),
        last_bid_imp_msecs_(0),
        last_ask_imp_msecs_(0),
        position_(0),
        l1_size_upper_bound_(1000000000),
        l1_size_lower_bound_(0),
        l1_order_upper_bound_(1000000000),
        l1_order_lower_bound_(0),
        l1_norm_factor_(0.0),
        l1_order_norm_factor_(0.0),
        moving_avg_l1_size_(0.0),
        moving_avg_l1_order_(0.0),
        best_nonself_bid_price_(0.0),
        best_nonself_ask_price_(0.0),
        best_nonself_mid_price_(0.0),
        best_nonself_mkt_price_(0.0),
        theoretical_price_(0.0),
        beta_adjusted_position_(0),
        notional_risk_(0),
        m_stdev_(0.0),
        hist_avg_spread_(0),
        moving_avg_spread_(0),
        spread_(0.0),
        returns_(0.0),
        hist_retuns_stdev_(0.0),
        hist_dollar_adjusted_change_stdev_(0.0),
        volume_ratio_stop_trading_lower_threshold_(0.0),
        volume_ratio_stop_trading_upper_threshold_(100),
        trading_status_(kMktTradingStatusOpen) {}

 public:
  SecurityMarketView* market_view_;
  SmartOrderManager* order_manager_;
  BaseModelMath* model_math_;
  ThrottleManager* throttle_manager_;

  TradeVars_t closeout_zeropos_tradevarset_;
  TradeVars_t closeout_long_tradevarset_;
  TradeVars_t closeout_short_tradevarset_;
  TradeVars_t global_tradevarset_;
  TradeVars_t instr_tradevarset_;

  std::string shortcode_;
  // Params
  int trading_start_utc_mfm_;
  int trading_end_utc_mfm_;

  // getflat
  bool getflat_mult_order_;
  bool getflat_due_to_external_getflat_;
  bool getflat_due_to_close_;
  bool getflat_due_to_max_loss_;
  bool getflat_due_to_market_status_;
  bool getflat_aggressive_;
  bool allowed_to_cancel_orders_;
  bool is_trading_today_;
  bool place_at_stable_prices_;

  //
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

  int last_bid_agg_msecs_;
  int last_ask_agg_msecs_;
  int last_bid_imp_msecs_;
  int last_ask_imp_msecs_;

  int position_;

  double l1_size_upper_bound_;
  double l1_size_lower_bound_;

  double l1_order_upper_bound_;
  double l1_order_lower_bound_;

  double l1_norm_factor_;
  double l1_order_norm_factor_;
  double moving_avg_l1_size_;
  double moving_avg_l1_order_;

  double best_nonself_bid_price_;
  double best_nonself_ask_price_;
  double best_nonself_mid_price_;
  double best_nonself_mkt_price_;

  double theoretical_price_;
  double beta_adjusted_position_;
  double notional_risk_;

  double m_stdev_;
  double hist_avg_spread_;
  double moving_avg_spread_;
  double spread_;
  double returns_;
  double hist_retuns_stdev_;
  double hist_dollar_adjusted_change_stdev_;

  double volume_ratio_stop_trading_lower_threshold_;
  double volume_ratio_stop_trading_upper_threshold_;

  //
  MktStatus_t trading_status_;
};
}
