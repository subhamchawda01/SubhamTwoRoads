/* =====================================================================================

   Filename:  ExecLogicCode/mean_reverting_trading.cpp

   Description:

   Version:  1.0
   Created:  Monday 30 May 2016 05:06:09  UTC
   Revision:  none
   Compiler:  g++

   Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011

   Address:  Suite No 162, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   Phone:  +91 80 4190 3551

   =====================================================================================
*/
#include <numeric>
#include "dvctrade/ExecLogic/mean_reverting_trading.hpp"
#define INVALID_MEAN_REVERTING_ORDER_PRICE -1
#define MR_LT_DURATION 60
#define MR_ST_DURATION 5
namespace HFSAT {

MeanRevertingTrading::MeanRevertingTrading(DebugLogger& _dbglogger_, const Watch& _watch_,
                                           const std::vector<SecurityMarketView*> _dep_market_view_fut_vec_,
                                           const std::vector<SmartOrderManager*> _order_manager_fut_vec_,
                                           const std::string& _global_paramfilename_, const bool _livetrading_,
                                           MultBasePNL* _mult_base_pnl_, int _trading_start_utc_mfm_,
                                           int _trading_end_utc_mfm_)

    // subscribe px types of each product
    // Make Indicators
    // listen to those indicators, e.g. indicator -> add_unweighted_indicator_listener(1u, this);
    // subscribe to watch
    // multbaspnl.listener(this)
    : MultExecInterface(_dbglogger_, _watch_, _dep_market_view_fut_vec_, _order_manager_fut_vec_,
                        _global_paramfilename_, _livetrading_, _trading_start_utc_mfm_, _trading_end_utc_mfm_),
      mult_base_pnl_(_mult_base_pnl_) {
  num_total_products_ = _dep_market_view_fut_vec_.size();
  mult_base_pnl_->AddListener(this);
  _watch_.subscribe_BigTimePeriod(this);
  Initialize();
  SetDontTradeForEarnings();
  SetDontTradeForBan();
  if( global_param_set_->use_abnormal_volume_getflat_)
  {
    LoadMarketShareFromDB();
    InitializeImpliedTriggerValues();
  }
}

void MeanRevertingTrading::InitializeImpliedTriggerValues()
{
  for( int t_ctr_ = 0; t_ctr_ < num_total_products_; t_ctr_++ )
  {
    if( !dont_trade_[t_ctr_] ) //only do this for products that are trading today
    {
      if( daily_mkt_share_vec_[t_ctr_] < 0.01 ) {
        daily_mkt_share_implied_trigger_[t_ctr_] = 5.0*daily_mkt_share_vec_[t_ctr_];
      } else if( daily_mkt_share_vec_[t_ctr_] < 0.025 ) {
        daily_mkt_share_implied_trigger_[t_ctr_] = 0.05 + 3.333*(daily_mkt_share_vec_[t_ctr_] - 0.01 );
      } else if( daily_mkt_share_vec_[t_ctr_] < 0.075 ) {
        daily_mkt_share_implied_trigger_[t_ctr_] = 0.1 + 2.0*(daily_mkt_share_vec_[t_ctr_] - 0.025 );
      } else {
        daily_mkt_share_implied_trigger_[t_ctr_] = 0.2 + 1.33*(daily_mkt_share_vec_[t_ctr_] - 0.075);
      }
    }
  }
}

// we would need to add portfolio here as well
/**
 * Format of product file:
 * INDEX
 * I1
 * I2
 * INDEXEND
 * PRODUCT
 * P1
 * P2
 * PRODUCTEND
 * PORTFOLIO
 * PF1
 * PF2
 * PORTFOLIOEND
 */
void MeanRevertingTrading::CollectORSShortCodes(DebugLogger& _dbglogger_, std::string product_filename,
                                                std::vector<std::string>& source_shortcode_vec_,
                                                std::vector<std::string>& ors_source_needed_vec_) {
  std::ifstream paramlistfile_;
  paramlistfile_.open(product_filename);
  //  bool paramset_file_list_read_ = false;
  if (paramlistfile_.is_open()) {
    const int kParamfileListBufflerLen = 1024;
    char readlinebuffer_[kParamfileListBufflerLen];
    bzero(readlinebuffer_, kParamfileListBufflerLen);
    while (paramlistfile_.good()) {
      bzero(readlinebuffer_, kParamfileListBufflerLen);
      paramlistfile_.getline(readlinebuffer_, kParamfileListBufflerLen);
      std::string this_product_ = std::string(readlinebuffer_);
      if (!this_product_.empty()) {
        VectorUtils::UniqueVectorAdd(source_shortcode_vec_, this_product_);
        VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, this_product_);
      }
    }
  }
}

void MeanRevertingTrading::CollectTradingShortCodes(DebugLogger& _dbglogger_, std::string product_filename,
                                                    std::vector<std::string>& _trading_product_vec_) {
  std::ifstream paramlistfile_;
  paramlistfile_.open(product_filename);
  //  bool paramset_file_list_read_ = false;
  if (paramlistfile_.is_open()) {
    const int kParamfileListBufflerLen = 1024;
    char readlinebuffer_[kParamfileListBufflerLen];
    bzero(readlinebuffer_, kParamfileListBufflerLen);
    while (paramlistfile_.good()) {
      bzero(readlinebuffer_, kParamfileListBufflerLen);
      paramlistfile_.getline(readlinebuffer_, kParamfileListBufflerLen);
      std::string this_product_ = std::string(readlinebuffer_);
      if (!this_product_.empty()) {
        VectorUtils::UniqueVectorAdd(_trading_product_vec_, this_product_);
      }
    }
  }
}

void MeanRevertingTrading::AddIndicatorListener() {
  if (global_param_set_->use_book_indicator_) {
    /*      for (auto i = 0u; i < shortcode_vec_.size(); i++)
          {
            BidAskToPayNotionalDynamicSD* t_book_indicator_ = BidAskToPayNotionalDynamicSD::GetUniqueInstance(
       dbglogger_, watch_, *dep_market_view_vec_[i], 5, 1000000, 300, kPriceTypeMidprice);
            t_book_indicator_->add_unweighted_indicator_listener(shortcode_vec_.size()+i, this);
          } */
  }
  if (global_param_set_->use_trade_indicator_) {
    for (auto i = 0u; i < shortcode_vec_.size(); i++) {
      DiffTRSizeAvgTPxBasepx* t_trade_indicator_ = DiffTRSizeAvgTPxBasepx::GetUniqueInstance(
          dbglogger_, watch_, *dep_market_view_vec_[i], 2, kPriceTypeMidprice);
      t_trade_indicator_->add_unweighted_indicator_listener(2 * shortcode_vec_.size() + i, this);
    }
  }
}

void MeanRevertingTrading::OnTimePeriodUpdate(const int num_pages_to_add) {}

void MeanRevertingTrading::OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_) {
  if (_indicator_index_ >= shortcode_vec_.size() && _indicator_index_ < 2 * shortcode_vec_.size()) {
    book_indicator_values_[_indicator_index_ - shortcode_vec_.size()] = _new_value_;
  } else if (_indicator_index_ >= 2 * shortcode_vec_.size() && _indicator_index_ < 3 * shortcode_vec_.size()) {
    trade_indicator_values_[_indicator_index_ - 2 * shortcode_vec_.size()] = _new_value_;
  }
}

//Supports StartTrading/StopTrading/DumpPositions
void MeanRevertingTrading::OnControlUpdate(const ControlMessage& _control_message_, const char* symbol,
                                           const int trader_id) {
  ControlMessageCode_t code = _control_message_.message_code_;
  if (code == kControlMessageCodeGetflat) {
    if (strlen(_control_message_.strval_1_) == 0) {
      GetAllFlat();
    } else {
      for (int i = 0; i < num_total_products_; i++) {
        if (strcmp(_control_message_.strval_1_, shortcode_vec_[i].c_str()) == 0) {
          GetFlatTradingLogic(i);
          break;
        }
      }
    }
  }
  else if(code == kControlMessageCodeStartTrading) {
    if (strlen(_control_message_.strval_1_) == 0) {
      for (int i = 0; i < num_total_products_; i++) {
	should_be_getting_flat_[i] = false;
      }
    } else {
      for (int i = 0; i < num_total_products_; i++) {
        if (strcmp(_control_message_.strval_1_, shortcode_vec_[i].c_str()) == 0) {
          should_be_getting_flat_[i] = false;
          break;
        }
      }
    }
  }
  else if(code == kControlMessageCodeDumpPositions) { 
    for (int i = 0; i < num_total_products_; i++) {
      dbglogger_ << shortcode_vec_[i].c_str() << " Pos " << product_position_in_lots_[i] << '\n';
    }
    DBGLOG_DUMP;
  }
}

void MeanRevertingTrading::OnMarketUpdate(const unsigned int _security_id_,
                                          const MarketUpdateInfo& _market_update_info_) {
  // if book has crossed last price then update last price and recompute affected portfolio prices
  double t_diff_ = 0;
  if (dep_market_view_vec_[_security_id_]->bestbid_price() > last_inst_prices_[_security_id_] &&
      dep_market_view_vec_[_security_id_]->bestbid_size() > 0) {
    t_diff_ = dep_market_view_vec_[_security_id_]->bestbid_price() - last_inst_prices_[_security_id_];
  }
  if (dep_market_view_vec_[_security_id_]->bestask_price() < last_inst_prices_[_security_id_] &&
      dep_market_view_vec_[_security_id_]->bestask_size() > 0 &&
      dep_market_view_vec_[_security_id_]->bestask_price() > 0) {
    t_diff_ = dep_market_view_vec_[_security_id_]->bestask_price() - last_inst_prices_[_security_id_];
  }

  if (fabs(t_diff_) > 1e-5) {
    UpdatePortPxForId(_security_id_, t_diff_);
    TradingLogic();
  }
  double current_unrealized_pnl_ = 0;
  for (int i = 0; i < num_total_products_; i++) {
    current_unrealized_pnl_ += order_manager_vec_[i]->base_pnl().total_pnl();
  }
  if (current_unrealized_pnl_ < global_min_pnl_) {
    global_min_pnl_ = current_unrealized_pnl_;
  }
}

void MeanRevertingTrading::UpdatePortPxForId(int _security_id_, double px_diff_) {
  last_inst_prices_[_security_id_] = last_inst_prices_[_security_id_] + px_diff_;
  std::vector<std::pair<int, double>>::iterator t_iter_;
  for (t_iter_ = predictor_vec_[_security_id_].begin(); t_iter_ != predictor_vec_[_security_id_].end(); t_iter_++) {
    int t_port_idx_ = (*t_iter_).first;
    double t_port_wt_ = (*t_iter_).second;
    last_port_prices_[t_port_idx_] = last_port_prices_[t_port_idx_] + t_port_wt_ * px_diff_;
    // target prices might have changed for this .. so recompute best prices to place at
    SetBestPrices(t_port_idx_);
  }
}

void MeanRevertingTrading::SetBestPrices(int _security_id_) {
  double t_buy_k_ = inst_base_threshold_[_security_id_];
  double t_sell_k_ = inst_base_threshold_[_security_id_];
  if (product_position_in_lots_[_security_id_] > 0) {
    t_buy_k_ = t_buy_k_ +
               inst_increase_threshold_[_security_id_] * product_position_in_lots_[_security_id_] /
                   inst_unitlots_[_security_id_];
    t_sell_k_ = t_sell_k_ -
                inst_decrease_threshold_[_security_id_] * product_position_in_lots_[_security_id_] /
                    inst_unitlots_[_security_id_];
    
    t_sell_k_ = std::max(0.0, t_sell_k_ - global_param_set_->time_hysterisis_factor_ * (watch_.msecs_from_midnight()/1000.0 - seconds_at_last_buy_[_security_id_]));
  } else if (product_position_in_lots_[_security_id_] < 0) {
    t_buy_k_ = t_buy_k_ +
               inst_decrease_threshold_[_security_id_] * product_position_in_lots_[_security_id_] /
                   inst_unitlots_[_security_id_];
    t_sell_k_ = t_sell_k_ -
                inst_increase_threshold_[_security_id_] * product_position_in_lots_[_security_id_] /
                    inst_unitlots_[_security_id_];

    t_buy_k_ = std::max(0.0, t_buy_k_ - global_param_set_->time_hysterisis_factor_ * (watch_.msecs_from_midnight()/1000.0 - seconds_at_last_sell_[_security_id_]));
  }

/*  //code to incorporate trend effect on stocks
  if(product_position_in_lots_[_security_id_] >= 0) {
    t_buy_k_ += trend_adjust_bid_threshold_[_security_id_]; 
  }
  if(product_position_in_lots_[_security_id_] <= 0) {
    t_sell_k_ += trend_adjust_ask_threshold_[_security_id_]; 
  } */
  
  bid_int_price_to_place_at_[_security_id_] =
      std::min(dep_market_view_vec_[_security_id_]->bestbid_int_price(),
               (int)floor((last_port_prices_[_security_id_] * inst_betas_[_security_id_] -
                           t_buy_k_ * stdev_residuals_[_security_id_]) /
                          dep_market_view_vec_[_security_id_]->min_price_increment()));
  // to avoid excessive messages
  if (bid_int_price_to_place_at_[_security_id_] < dep_market_view_vec_[_security_id_]->bestbid_int_price() * 0.998) {
    bid_int_price_to_place_at_[_security_id_] = INVALID_MEAN_REVERTING_ORDER_PRICE;
  }

  ask_int_price_to_place_at_[_security_id_] =
      std::max(dep_market_view_vec_[_security_id_]->bestask_int_price(),
               (int)ceil((last_port_prices_[_security_id_] * inst_betas_[_security_id_] +
                          t_sell_k_ * stdev_residuals_[_security_id_]) /
                         dep_market_view_vec_[_security_id_]->min_price_increment()));

  if (ask_int_price_to_place_at_[_security_id_] > dep_market_view_vec_[_security_id_]->bestask_int_price() * 1.002) {
    ask_int_price_to_place_at_[_security_id_] = INVALID_MEAN_REVERTING_ORDER_PRICE;
  }
  if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
    DBGLOG_TIME_CLASS_FUNC_LINE << bid_int_price_to_place_at_[_security_id_] << " "
                                << ask_int_price_to_place_at_[_security_id_] << "\n";
  }
}

void MeanRevertingTrading::OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                                        const MarketUpdateInfo& _market_update_info_) {

  if (fabs(_trade_print_info_.trade_price_ - last_inst_prices_[_security_id_] > 1e-5)) {
    UpdatePortPxForId(_security_id_, _trade_print_info_.trade_price_ - last_inst_prices_[_security_id_]);
  }

  // update  vectors and recompute betas
  if (watch_.msecs_from_midnight() - msecs_at_last_vec_processing_ >= 60000) {
    for (int t_ctr_ = 0; t_ctr_ < num_total_products_; t_ctr_++) {
      if (dont_trade_[t_ctr_] == false) {
        inst_prices_[t_ctr_].push_back(last_inst_prices_[t_ctr_]);
        port_prices_[t_ctr_].push_back(last_port_prices_[t_ctr_]);
        inst_prices_[t_ctr_].erase(inst_prices_[t_ctr_].begin());
        port_prices_[t_ctr_].erase(port_prices_[t_ctr_].begin());
        double t_new_beta_ = std::inner_product(inst_prices_[t_ctr_].begin(), inst_prices_[t_ctr_].end(),
                                                port_prices_[t_ctr_].begin(), 0.0) /
                             std::inner_product(port_prices_[t_ctr_].begin(), port_prices_[t_ctr_].end(),
                                                port_prices_[t_ctr_].begin(), 0.0);
        //	std::cout << " Beta_change_for " << dep_market_view_vec_[t_ctr_]->shortcode() << " is " << (
        // t_new_beta_/inst_betas_[t_ctr_] - 1.0 )*100 << " Beta " << t_new_beta_ << '\n';
        inst_betas_[t_ctr_] = t_new_beta_;

        double t_del_residual_ = residuals_[t_ctr_][0];
        residual_sum_[t_ctr_] -= t_del_residual_;
        residual_sumsqr_[t_ctr_] -= (t_del_residual_ * t_del_residual_);
        double t_add_residual_ = last_inst_prices_[t_ctr_] - inst_betas_[t_ctr_] * last_port_prices_[t_ctr_];
        residual_sum_[t_ctr_] += t_add_residual_;
        residual_sumsqr_[t_ctr_] += (t_add_residual_ * t_add_residual_);
        residuals_[t_ctr_].push_back(t_add_residual_);
        residuals_[t_ctr_].erase(residuals_[t_ctr_].begin());
        stdev_residuals_[t_ctr_] = sqrt(residual_sumsqr_[t_ctr_] / global_param_set_->hist_error_length_ -
                                        residual_sum_[t_ctr_] / global_param_set_->hist_error_length_ *
                                            residual_sum_[t_ctr_] / global_param_set_->hist_error_length_);
        // //set adjust thresholds. TODO - parameterize better at a later stage
        // unsigned int t_vec_offset_ = std::max(0u, global_param_set_->hist_price_length_ - 6); //5 minute offset
        // if( last_inst_prices_[t_ctr_] >= inst_prices_[t_ctr_][t_vec_offset_] - 1e-5)
        // {
        //   trend_adjust_bid_threshold_[t_ctr_] = -0.5;
        // }
        // else
        // {
        //   trend_adjust_bid_threshold_[t_ctr_] = 0.5;
        // }
        // if( last_inst_prices_[t_ctr_] <= inst_prices_[t_ctr_][t_vec_offset_] + 1e-5)
        // {
        //   trend_adjust_ask_threshold_[t_ctr_] = -0.5;
        // }
        // else
        // {
        //   trend_adjust_ask_threshold_[t_ctr_] = 0.5;
        // }
      }
    }
    msecs_at_last_vec_processing_ = watch_.msecs_from_midnight();
  }

  if( global_param_set_->use_abnormal_volume_getflat_ )
  {
    if( !dont_trade_[_security_id_] )
    {
      traded_notional_since_last_update_[_security_id_] +=
        _trade_print_info_.size_traded_ * _trade_print_info_.trade_price_;
    }

    //recomputation of mkt share moving averages is done every minute
    if (watch_.msecs_from_midnight() - msecs_at_last_trade_processing_ >= 60000) {
      //tradeready is set to false if mkt book has seen no trades [ avoiding initial spurious values ]
      bool t_tradeready_ = true;
      for (unsigned int t_ctr_ = 0; t_ctr_ < traded_notional_since_last_update_.size(); t_ctr_++) {
        if (!dont_trade_[t_ctr_] && (dep_market_view_vec_[t_ctr_]->market_update_info_.trades_count_[kTradeTypeBuy] + 
  				   dep_market_view_vec_[t_ctr_]->market_update_info_.trades_count_[kTradeTypeSell] <= 0)) {
          t_tradeready_ = false;
        }
      }

      if (t_tradeready_) {
        msecs_at_last_trade_processing_ = watch_.msecs_from_midnight();
        double t_tot_notional_ =
            std::accumulate(traded_notional_since_last_update_.begin(), traded_notional_since_last_update_.end(), 0.0);
        //iterate over all instruments
        for (unsigned int t_ctr_ = 0; t_ctr_ < traded_notional_since_last_update_.size(); t_ctr_++) {        
          //if product is not set to trade - nothing needs to be done
  	  if( dont_trade_[t_ctr_] )
	    continue;
	    
  	  short_term_mkt_share_vec_[t_ctr_] =
	    short_term_mkt_share_decay_factor_ * short_term_mkt_share_vec_[t_ctr_] +
	    (1.0 - short_term_mkt_share_decay_factor_) * traded_notional_since_last_update_[t_ctr_] / t_tot_notional_;

	  long_term_mkt_share_vec_[t_ctr_] =
	    long_term_mkt_share_decay_factor_ * long_term_mkt_share_vec_[t_ctr_] +
	    (1.0 - long_term_mkt_share_decay_factor_) * traded_notional_since_last_update_[t_ctr_] / t_tot_notional_;

          dbglogger_ << watch_.tv() << ' ' << shortcode_vec_[t_ctr_] << " STMS " << short_term_mkt_share_vec_[t_ctr_] 
	  	     << " LTMS " << long_term_mkt_share_vec_[t_ctr_] << " DMS " << daily_mkt_share_vec_[t_ctr_] 
		     << " LTP " << last_inst_prices_[t_ctr_] << " Pos " << product_position_in_lots_[t_ctr_] 
		     << " Pnl " << order_manager_vec_[t_ctr_]->base_pnl().total_pnl() << '\n';

          if(!getflat_due_to_abnormal_volume_[t_ctr_] && 
             short_term_mkt_share_vec_[t_ctr_] > daily_mkt_share_implied_trigger_[t_ctr_] &&
             short_term_mkt_share_vec_[t_ctr_] > global_param_set_->abnormal_share_set_threshold_ ) 
	  {
            dbglogger_ << watch_.msecs_from_midnight() << " Sec " << dep_market_view_vec_[t_ctr_]->shortcode() 
 	  	       << " Volume getflat set to true \n";
            getflat_due_to_abnormal_volume_[t_ctr_] = true;
            lt_mkt_share_at_trigger_[t_ctr_] = long_term_mkt_share_vec_[t_ctr_] ; 
          }
          else if(getflat_due_to_abnormal_volume_[t_ctr_] && 
                  ( short_term_mkt_share_vec_[t_ctr_] < std::min(lt_mkt_share_at_trigger_[t_ctr_], daily_mkt_share_vec_[t_ctr_]) && 
	  	    long_term_mkt_share_vec_[t_ctr_] < std::min(lt_mkt_share_at_trigger_[t_ctr_], daily_mkt_share_vec_[t_ctr_] ))) 
          {
            dbglogger_ << watch_.msecs_from_midnight() << " Sec " << dep_market_view_vec_[t_ctr_]->shortcode() 
   		       << " Volume getflat set to false \n";
            getflat_due_to_abnormal_volume_[t_ctr_] = false; 
          }
          traded_notional_since_last_update_[t_ctr_] = 0.0;
        }//end for
      }//end is tradeready
    }//end interval processing
  }//end abnormal getflat setting check
  // getflat due to strat max loss computation
  double t_total_pnl_ = 0.0;
  for (int t_ctr_ = 0; t_ctr_ < num_total_products_; t_ctr_++) {
    t_total_pnl_ += order_manager_vec_[t_ctr_]->base_pnl().total_pnl();
  }
  if (t_total_pnl_ < -1.0 * global_param_set_->strat_max_loss_) {
    for (int t_ctr_ = 0; t_ctr_ < num_total_products_; t_ctr_++) {
      should_be_getting_flat_[t_ctr_] = true;
    }
  }
/*  dbglogger_ << watch_.msecs_from_midnight() << " Sec " << dep_market_view_vec_[_security_id_]->shortcode() << " Mid Px " 
            << (dep_market_view_vec_[_security_id_]->bestbid_price()+dep_market_view_vec_[_security_id_]->bestask_price())*0.5
            << " BIP " << 0.05*bid_int_price_to_place_at_[_security_id_] << " AIP " << 0.05*ask_int_price_to_place_at_[_security_id_] 
	    << " Port_Px " << last_port_prices_[_security_id_] << " Beta " << inst_betas_[_security_id_]
            << " Stdev " << stdev_residuals_[_security_id_] << " Pos " << product_position_in_lots_[_security_id_] 
	    << " Dont_Trade " << ( dont_trade_[_security_id_]?'Y':'N') << " GetFlat " << ( should_be_getting_flat_[_security_id_]?'Y':'N') << '\n';
  DBGLOG_DUMP;  */
  TradingLogic();
}

void MeanRevertingTrading::OnExec(const int _new_position_, const int _exec_quantity_, const TradeType_t _buysell_,
                                  const double _price_, const int r_int_price_, const int _security_id_) {
  // update our position variables here
  product_position_in_lots_[_security_id_] = _new_position_ / dep_market_view_vec_[_security_id_]->min_order_size();

  if (_buysell_ == HFSAT::kTradeTypeBuy) {
    seconds_at_last_buy_[_security_id_] = watch_.msecs_from_midnight() / 1000;
  } else {
    seconds_at_last_sell_[_security_id_] = watch_.msecs_from_midnight() / 1000;
  }

  SetBestPrices(_security_id_);
  int t_tot_lots_ = 0;
  double t_tot_pnl_ = 0.0;

  for (int t_ctr_ = 0; t_ctr_ < num_total_products_; t_ctr_++) {
    t_tot_lots_ += product_position_in_lots_[t_ctr_];
    t_tot_pnl_ += order_manager_vec_[t_ctr_]->base_pnl().ReportConservativeTotalPNL(true);
  }

  TradingLogic();
}

void MeanRevertingTrading::ReportResults(HFSAT::BulkFileWriter& trades_writer_, bool _conservative_close_) {
  int total_pnl_ = 0;
  int total_volume_ = 0;
  for (int i = 0; i < num_total_products_; i++) {
    if (order_manager_vec_[i]->trade_volume() > 0) {
      std::cout << watch_.YYYYMMDD() << ' ' << shortcode_vec_[i] << " "
                << order_manager_vec_[i]->base_pnl().ReportConservativeTotalPNL(_conservative_close_) << " "
                << " " << order_manager_vec_[i]->trade_volume() / (dep_market_view_vec_[i]->min_order_size()) << " " 
		<< " OrderManager Count " << ( order_manager_vec_[i]->SendOrderCount() + order_manager_vec_[i]->CxlOrderCount() + 
		order_manager_vec_[i]->ModifyOrderCount() ) << "\n";
    }
    total_pnl_ += order_manager_vec_[i]->base_pnl().ReportConservativeTotalPNL(_conservative_close_);
    total_volume_ += order_manager_vec_[i]->trade_volume() / (dep_market_view_vec_[i]->min_order_size());
  }
  std::cout << "Total "
            << " " << watch_.YYYYMMDD() << " " << total_pnl_ << " " << total_volume_ << " " << global_min_pnl_ << "\n";
}

// Supported just because of inheritance - not used
int MeanRevertingTrading::my_global_position() const {
  // sum all position in pos_vec_
  int pos_ = 0;
  for (int i = 0; i < num_total_products_; i++) {
    pos_ += product_position_in_lots_[i];
  }
  return (pos_);
}

void MeanRevertingTrading::Initialize() {
  global_min_pnl_ = 0;  // initialize global minimum PNL

  product_position_in_lots_.resize(num_total_products_, 0);
  if( livetrading_ ) {
    should_be_getting_flat_.resize(num_total_products_, true);
  } else {
    should_be_getting_flat_.resize(num_total_products_, false);
  }

  bid_int_price_to_place_at_.resize(num_total_products_, INVALID_MEAN_REVERTING_ORDER_PRICE);
  ask_int_price_to_place_at_.resize(num_total_products_, INVALID_MEAN_REVERTING_ORDER_PRICE);

  seconds_at_last_buy_.resize(num_total_products_, 0);
  seconds_at_last_sell_.resize(num_total_products_, 0);

  book_indicator_values_.resize(num_total_products_, 0);
  trade_indicator_values_.resize(num_total_products_, 0);

  traded_notional_since_last_update_.resize(num_total_products_, 0);
  short_term_mkt_share_vec_.resize(num_total_products_, 0);
  long_term_mkt_share_vec_.resize(num_total_products_, 0);
  daily_mkt_share_vec_.resize(num_total_products_, 0);
  msecs_at_last_trade_processing_ = 0;
  long_term_mkt_share_decay_factor_ = exp(-log(2) / MR_LT_DURATION);
  short_term_mkt_share_decay_factor_ = exp(-log(2) / MR_ST_DURATION);

  for (int i = 0; i < num_total_products_; i++) {
    dep_market_view_vec_[i]->subscribe_price_type(this, kPriceTypeMidprice);
    shortcode_vec_.push_back(dep_market_view_vec_[i]->shortcode());
  }

  // we create these only for non-index constituents
  residuals_.resize(num_total_products_);
  residual_sum_.resize(num_total_products_);
  residual_sumsqr_.resize(num_total_products_);
  stdev_residuals_.resize(num_total_products_);
  inst_prices_.resize(num_total_products_);
  port_prices_.resize(num_total_products_);
  inst_betas_.resize(num_total_products_);
  last_inst_prices_.resize(num_total_products_);
  last_port_prices_.resize(num_total_products_);
  for (int i = 0; i < num_total_products_; i++) {
    residuals_[i].clear();
    inst_prices_[i].clear();
    port_prices_[i].clear();
  }
  msecs_at_last_vec_processing_ = 0;

  predictor_vec_.resize(num_total_products_, std::vector<std::pair<int, double>>());
  dont_trade_.resize(num_total_products_, false);

  inst_return_vol_.resize(num_total_products_,1.0);
  ReadHistValues();
  AddIndicatorListener();

  getflat_due_to_abnormal_volume_.resize(num_total_products_, false);
  lt_mkt_share_at_trigger_.resize(num_total_products_, 0.0);
  daily_mkt_share_implied_trigger_.resize(num_total_products_, 0.0);

  //  trend_adjust_bid_threshold_.resize(num_total_products_, 0.0);
  //  trend_adjust_ask_threshold_.resize(num_total_products_, 0.0);  

  //set inst specific parameters
  inst_unitlots_.resize(num_total_products_);
  inst_maxlots_.resize(num_total_products_);
  inst_base_threshold_.resize(num_total_products_);
  inst_increase_threshold_.resize(num_total_products_);
  inst_decrease_threshold_.resize(num_total_products_);
  for( int i = 0; i < num_total_products_; i++ ) {
    if( global_param_set_->use_notional_scaling_ )
    {
      //max introduces since last_inst_prices_ for an instrument is not necessarily present in histfile.
      double t_inst_notional_per_lot_ = std::max(100000.0, last_inst_prices_[i]*(dep_market_view_vec_[i]->min_order_size()));
      inst_unitlots_[i] = std::max(1, (int)round(global_param_set_->notional_for_unit_lot_*global_param_set_->inst_uts_/t_inst_notional_per_lot_));
      inst_maxlots_[i] = std::max(1, (int)round(global_param_set_->notional_for_unit_lot_*global_param_set_->inst_maxlots_/(inst_return_vol_[i]*t_inst_notional_per_lot_)));
      if(inst_maxlots_[i] == 1)
      {
        inst_base_threshold_[i] = global_param_set_->base_threshold_ + (global_param_set_->inst_maxlots_ - 1)*global_param_set_->increase_threshold_/2.0;
        inst_increase_threshold_[i] = global_param_set_->increase_threshold_;
        inst_decrease_threshold_[i] = global_param_set_->decrease_threshold_ + ( inst_base_threshold_[i] - global_param_set_->base_threshold_ );
      }
      else
      {
        inst_base_threshold_[i] = global_param_set_->base_threshold_;
        inst_increase_threshold_[i] = global_param_set_->increase_threshold_*global_param_set_->inst_maxlots_/inst_maxlots_[i];
        inst_decrease_threshold_[i] = global_param_set_->decrease_threshold_*global_param_set_->inst_maxlots_/inst_maxlots_[i];
      }
    }
    else
    {
      inst_unitlots_[i] = global_param_set_->inst_uts_;
      inst_maxlots_[i] = global_param_set_->inst_maxlots_;
      inst_base_threshold_[i] = global_param_set_->base_threshold_;
      inst_increase_threshold_[i] = global_param_set_->increase_threshold_;
      inst_decrease_threshold_[i] = global_param_set_->decrease_threshold_;
    }
    dbglogger_ << dep_market_view_vec_[i]->shortcode() << " IL " << inst_unitlots_[i] << " IM " << inst_maxlots_[i]
	       << " BT " << inst_base_threshold_[i] << " IT " << inst_increase_threshold_[i] << " DT " << inst_decrease_threshold_[i] << '\n';
  }
}

void MeanRevertingTrading::ReadHistValues() {
  char t_filename_[1024];
  //    sprintf(t_filename_,"/spare/local/tradeinfo/NSE_Files/MRHist/%s.%d",(global_param_set_->histfile_prefix_).c_str(),
  //    watch_.YYYYMMDD());
  sprintf(t_filename_, "%s%d", (global_param_set_->histfile_prefix_).c_str(), watch_.YYYYMMDD());
  //    std::cout << " File " << t_filename_ << '\n';
  std::ifstream histfile_;
  histfile_.open(t_filename_);
  HFSAT::SecurityNameIndexer& sec_name_indexer_ = HFSAT::SecurityNameIndexer::GetUniqueInstance();
  if (histfile_.is_open()) {
    const int kParamfileListBufflerLen = 51200;
    char readlinebuffer_[kParamfileListBufflerLen];
    bzero(readlinebuffer_, kParamfileListBufflerLen);
    int t_stk_id_ = 0;
    while (histfile_.good()) {
      bzero(readlinebuffer_, kParamfileListBufflerLen);
      histfile_.getline(readlinebuffer_, kParamfileListBufflerLen);
      std::string this_line_ = std::string(readlinebuffer_);
      PerishableStringTokenizer st_(readlinebuffer_, kParamfileListBufflerLen);
      const std::vector<const char*>& tokens_ = st_.GetTokens();
      if (tokens_.size() == 0) {
        continue;
      }
      if (strcmp(tokens_[0], "Port:") == 0) {
        // format of this line will be target_stock num_predictors pred_stock_1 weight_1 .... pred_stock_n weight_n
        t_stk_id_ = sec_name_indexer_.GetIdFromString(tokens_[1]);
        if (t_stk_id_ < 0) {
          // data for a stock which is not in our trading config  .. continue
          continue;
        } else {
          unsigned int t_num_preds_ = atoi(tokens_[2]);
          if (tokens_.size() != 3 + 2 * t_num_preds_) {
            std::cerr << "Malformed first line of stock data in hist file .. exiting \n";
            exit(-1);
          }
          for (unsigned int t_ctr_ = 0; t_ctr_ < t_num_preds_; t_ctr_++) {
            int t_pred_id_ = sec_name_indexer_.GetIdFromString(tokens_[3 + 2 * t_ctr_]);
            double t_weight_ = atof(tokens_[4 + 2 * t_ctr_]);
            if (t_pred_id_ < 0) {
              std::cerr << " Predictor " << tokens_[3 + 2 * t_ctr_] << " not present in strat config .. exiting \n";
              exit(-1);
            }
            std::pair<int, double> pred_entry_(t_stk_id_, t_weight_);
            predictor_vec_[t_pred_id_].push_back(pred_entry_);
          }
        }
      } else if (strcmp(tokens_[0], "HIST_PRICES") == 0 &&
                 tokens_.size() == (2 * global_param_set_->hist_price_length_ + 1) && t_stk_id_ >= 0) {
        // format of line is stk_px{t-1} port_px{t-1} ... stk_px{0} port_px{0}
        for (unsigned int t_ctr_ = 0; t_ctr_ < global_param_set_->hist_price_length_; t_ctr_++) {
          inst_prices_[t_stk_id_].push_back(atof(tokens_[t_ctr_ * 2 + 1]));
          port_prices_[t_stk_id_].push_back(atof(tokens_[t_ctr_ * 2 + 2]));
        }
        inst_betas_[t_stk_id_] = std::inner_product(inst_prices_[t_stk_id_].begin(), inst_prices_[t_stk_id_].end(),
                                                    port_prices_[t_stk_id_].begin(), 0.0) /
                                 std::inner_product(port_prices_[t_stk_id_].begin(), port_prices_[t_stk_id_].end(),
                                                    port_prices_[t_stk_id_].begin(), 0.0);
        last_port_prices_[t_stk_id_] = atof(tokens_[2 * global_param_set_->hist_price_length_]);
        // last_inst_prices_[t_stk_id_] = atof(tokens_[2 * global_param_set_->hist_price_length_ - 1]);
        // dbglogger_ << "Instrument: " << dep_market_view_vec_[t_stk_id_]->shortcode() << " beta "
        //           << inst_betas_[t_stk_id_] << " Pxs " << last_inst_prices_[t_stk_id_] << ':'
        //           << last_port_prices_[t_stk_id_] << '\n';
      } else if ((strcmp(tokens_[0], "HIST_ERROR") == 0) &&
                 (tokens_.size() == global_param_set_->hist_error_length_ + 1) && (t_stk_id_ >= 0)) {
        for (unsigned int t_ctr_ = 0; t_ctr_ < global_param_set_->hist_error_length_; t_ctr_++) {
          residuals_[t_stk_id_].push_back(atof(tokens_[t_ctr_ + 1]));
        }
        residual_sum_[t_stk_id_] = std::accumulate(residuals_[t_stk_id_].begin(), residuals_[t_stk_id_].end(), 0.0);
        residual_sumsqr_[t_stk_id_] = std::inner_product(residuals_[t_stk_id_].begin(), residuals_[t_stk_id_].end(),
                                                         residuals_[t_stk_id_].begin(), 0.0);
        stdev_residuals_[t_stk_id_] = sqrt(residual_sumsqr_[t_stk_id_] / global_param_set_->hist_error_length_ -
                                           residual_sum_[t_stk_id_] / global_param_set_->hist_error_length_ *
                                               residual_sum_[t_stk_id_] / global_param_set_->hist_error_length_);
        // dbglogger_ << "Instrument: " << dep_market_view_vec_[t_stk_id_]->shortcode() << " last error "
        //           << residuals_[t_stk_id_][global_param_set_->hist_error_length_ - 1] << " stdev "
        //           << stdev_residuals_[t_stk_id_] << '\n';
      } else if (strcmp(tokens_[0], "LAST_PRICE") == 0) {
        // format is LAST_PRICE INST1 PX1 .. INSTN PXN
        for (unsigned int t_ctr_ = 1; t_ctr_ < tokens_.size(); t_ctr_ = t_ctr_ + 2) {
          int t_sec_id_ = sec_name_indexer_.GetIdFromString(tokens_[t_ctr_]);
          if (t_sec_id_ >= 0) {
            last_inst_prices_[t_sec_id_] = atof(tokens_[t_ctr_ + 1]);
          }
        }
      } else if (strcmp(tokens_[0], "INST_VOLATILITY") == 0) {
        // format is INST_VOLATILITY INST1 VOL1 .. INSTN VOLN 
        // values are normalized for banknifty vol being 1
        for (unsigned int t_ctr_ = 1; t_ctr_ < tokens_.size(); t_ctr_ = t_ctr_ + 2) {
          int t_sec_id_ = sec_name_indexer_.GetIdFromString(tokens_[t_ctr_]);
          if (t_sec_id_ >= 0) {
            inst_return_vol_[t_sec_id_] = atof(tokens_[t_ctr_ + 1]);
          }
        }
      } else if (t_stk_id_ >= 0 && tokens_.size() > 0) {
        std::cerr << "Error - hist file format incorrect " << tokens_[0] << " " << tokens_.size() << '\n';
      }
    }
  }
  histfile_.close();

  // disable trading in products which are not specified in hist file
  for (int t_ctr_ = 0; t_ctr_ < num_total_products_; t_ctr_++) {
    if (residuals_[t_ctr_].size() != global_param_set_->hist_error_length_ ||
        inst_prices_[t_ctr_].size() != global_param_set_->hist_price_length_) {
      dont_trade_[t_ctr_] = true;
      //	std::cout << "Trading in " << shortcode_vec_[t_ctr_] << " disabled\n";
    }
  }

  // validate portfolio constituents -- debug mode
  for (int t_ctr_ = 0; t_ctr_ < num_total_products_; t_ctr_++) {
    DumpPortfolioConstituents(t_ctr_);
  }
}

void MeanRevertingTrading::DumpPortfolioConstituents(int sec_id) {
  dbglogger_ << " Instrument " << shortcode_vec_[sec_id] << " DumpPortConst \n";
  dbglogger_ << " Last port px " << last_port_prices_[sec_id] << '\n';
  double t_comp_px_ = 0;
  for (int t_ctr_ = 0; t_ctr_ < num_total_products_; t_ctr_++) {
    std::vector<std::pair<int, double>>::iterator t_iter_;
    for (t_iter_ = predictor_vec_[t_ctr_].begin(); t_iter_ != predictor_vec_[t_ctr_].end(); t_iter_++) {
      if ((*t_iter_).first == sec_id) {
        dbglogger_ << "Port constituent " << shortcode_vec_[t_ctr_] << " weight " << (*t_iter_).second << " inst px "
                   << last_inst_prices_[t_ctr_] << '\n';
        t_comp_px_ = t_comp_px_ + (*t_iter_).second * last_inst_prices_[t_ctr_];
      }
    }
  }
  dbglogger_ << " Computed port px " << t_comp_px_ << "\n\n";
}

void MeanRevertingTrading::PlaceAndCancelOrders() {
  // iterate sequentially for all products
  for (int t_ctr_ = 0; t_ctr_ < num_total_products_; t_ctr_++) {
    // buy order handling
    if (bid_int_price_to_place_at_[t_ctr_] == INVALID_MEAN_REVERTING_ORDER_PRICE) {
      order_manager_vec_[t_ctr_]->CancelAllBidOrders();
    } else {
      PlaceSingleBuyOrder(t_ctr_, bid_int_price_to_place_at_[t_ctr_]);
    }

    // ask order handling
    if (ask_int_price_to_place_at_[t_ctr_] == INVALID_MEAN_REVERTING_ORDER_PRICE) {
      order_manager_vec_[t_ctr_]->CancelAllAskOrders();
    } else {
      PlaceSingleSellOrder(t_ctr_, ask_int_price_to_place_at_[t_ctr_]);
    }
  }
}

void MeanRevertingTrading::PlaceSingleBuyOrder(int index, int int_order_px_) {
  //    std::cout << watch_.msecs_from_midnight() << "PSBO " << dep_market_view_vec_[index]->shortcode() << ' ' <<
  //    int_order_px_ << " Mkt " << dep_market_view_vec_[index]->bestbid_int_price() << " --- " <<
  //    dep_market_view_vec_[index]->bestask_int_price() << '\n';
  HFSAT::SmartOrderManager* t_om_ = order_manager_vec_[index];
  int t_order_vec_top_bid_index_ = t_om_->GetOrderVecTopBidIndex();
  int t_order_vec_bottom_bid_index_ = t_om_->GetOrderVecBottomBidIndex();
  int t_existing_int_price_ = -1;

  if (t_order_vec_top_bid_index_ != t_order_vec_bottom_bid_index_) {
    DBGLOG_TIME_CLASS_FUNC << "More than one bid orders for " << index << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    exit(-1);
  }

  if (t_om_->GetUnSequencedBids().size() > 1) {
    DBGLOG_TIME_CLASS_FUNC << " More than one unsequenced order for " << index << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    exit(-1);
  }

  if (t_om_->GetUnSequencedBids().size() > 0) {
    return;                                       // don't do anything if unseq bid order is live
  } else if (t_order_vec_top_bid_index_ != -1) {  // -1 denotes no active orders in ordermanager
    t_existing_int_price_ = t_om_->GetBidIntPrice(t_order_vec_top_bid_index_);
  }

  int t_order_size_to_place_ =
      std::max(0, std::min(inst_unitlots_[index],
			   inst_maxlots_[index] - product_position_in_lots_[index])) *
      dep_market_view_vec_[index]->min_order_size();

  /*if (should_be_getting_flat_[index]) {
    t_order_size_to_place_ = std::max(0, std::min(inst_unitlots_[index], -1 * product_position_in_lots_[index])) *
                             dep_market_view_vec_[index]->min_order_size();
			     }*/


  if (t_order_size_to_place_ == 0) {
    DBGLOG_TIME_CLASS_FUNC << " Order Size Place request for size 0 " << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    exit(-1);
  }

  HFSAT::BaseOrder* t_order_ = NULL;
  if (t_existing_int_price_ != -1) {
    t_order_ = t_om_->GetBottomBidOrderAtIntPx(t_existing_int_price_);
  }

  // if there is no order active place an order
  if (t_existing_int_price_ == -1) {
    DBGLOG_TIME_CLASS_FUNC << " Send Buy Order for " << shortcode_vec_[index] << ' ' << t_order_size_to_place_ << " @ "
                           << int_order_px_ << " best bid " << dep_market_view_vec_[index]->bestbid_int_price()
                           << " Port_Px " << last_port_prices_[index] << " Beta " << inst_betas_[index] 
			   << " Stdev " << stdev_residuals_[index] << " Pos " << product_position_in_lots_[index] << DBGLOG_ENDL_FLUSH;
    t_om_->SendTradeIntPx(int_order_px_, t_order_size_to_place_, HFSAT::kTradeTypeBuy, 'B', HFSAT::kOrderDay);
  }
  // else if we need to modify price
  else if (t_existing_int_price_ != int_order_px_) {
    //      DBGLOG_TIME_CLASS_FUNC << " Modify Buy Order Price for " << shortcode_vec_[index] << " Old_Px " <<
    //      t_existing_int_price_ << " New_Px " << int_order_px_ << DBGLOG_ENDL_FLUSH;
    t_om_->ModifyOrderAndLog(t_order_, dep_market_view_vec_[index]->GetDoublePx(int_order_px_), int_order_px_,
                             t_order_size_to_place_);
  }
}

void MeanRevertingTrading::PlaceSingleSellOrder(int index, int int_order_px_) {
  //    std::cout << watch_.msecs_from_midnight() << "PSSO " << dep_market_view_vec_[index]->shortcode() << ' ' <<
  //    int_order_px_ << " Mkt " << dep_market_view_vec_[index]->bestbid_int_price() << " --- " <<
  //    dep_market_view_vec_[index]->bestask_int_price() << '\n';
  HFSAT::SmartOrderManager* t_om_ = order_manager_vec_[index];
  int t_order_vec_top_ask_index_ = t_om_->GetOrderVecTopAskIndex();
  int t_order_vec_bottom_ask_index_ = t_om_->GetOrderVecBottomAskIndex();
  int t_existing_int_price_ = -1;

  if (t_order_vec_top_ask_index_ != t_order_vec_bottom_ask_index_) {
    DBGLOG_TIME_CLASS_FUNC << "More than one ask orders for " << shortcode_vec_[index] << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    exit(-1);
  }

  if (t_om_->GetUnSequencedAsks().size() > 1) {
    DBGLOG_TIME_CLASS_FUNC << " More than one unsequenced sell order for " << shortcode_vec_[index]
                           << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    exit(-1);
  }

  if (t_om_->GetUnSequencedAsks().size() > 0) {
    return;                                       // don't do anything if unseq bid order is live
  } else if (t_order_vec_top_ask_index_ != -1) {  // -1 denotes no active orders in ordermanager
    t_existing_int_price_ = t_om_->GetAskIntPrice(t_order_vec_top_ask_index_);
  }

  int t_order_size_to_place_ =
      std::max(0, std::min(inst_unitlots_[index], 
			   inst_maxlots_[index] + product_position_in_lots_[index])) *
      dep_market_view_vec_[index]->min_order_size();

  /*  if (should_be_getting_flat_[index]) {
    t_order_size_to_place_ = std::max(0, std::min(inst_unitlots_[index], product_position_in_lots_[index])) *
                             dep_market_view_vec_[index]->min_order_size();
			     }*/

  if (t_order_size_to_place_ == 0) {
    DBGLOG_TIME_CLASS_FUNC << " Order Size Place request for size 0 " << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    exit(-1);
  }

  HFSAT::BaseOrder* t_order_ = NULL;
  if (t_existing_int_price_ != -1) {
    t_order_ = t_om_->GetBottomAskOrderAtIntPx(t_existing_int_price_);
  }

  // if there is no order active place an order
  if (t_existing_int_price_ == -1) {
    DBGLOG_TIME_CLASS_FUNC << " Send Sell Order for " << shortcode_vec_[index] << ' ' << t_order_size_to_place_ << " @ "
                           << int_order_px_ << " best ask " << dep_market_view_vec_[index]->bestask_int_price()
                           << " Port_Px " << last_port_prices_[index] << " Inst_Beta " << inst_betas_[index] 
			   << " Stdev_Residuals " << stdev_residuals_[index] << " Pos " << product_position_in_lots_[index] << DBGLOG_ENDL_FLUSH;
    t_om_->SendTradeIntPx(int_order_px_, t_order_size_to_place_, HFSAT::kTradeTypeSell, 'B', HFSAT::kOrderDay);
  }
  // else if we need to modify price
  else if (t_existing_int_price_ != int_order_px_) {
    //      DBGLOG_TIME_CLASS_FUNC << " Modify Sell Order Price for " << shortcode_vec_[index] << " Old_Px " <<
    //      t_existing_int_price_ << " New_Px " << int_order_px_ << DBGLOG_ENDL_FLUSH;
    t_om_->ModifyOrderAndLog(t_order_, dep_market_view_vec_[index]->GetDoublePx(int_order_px_), int_order_px_,
                             t_order_size_to_place_);
  }
}

/// Called in the following situations - ( i ) Trade happens in market ; (ii) L1 book
/// of some instrument changes; ( iii ) order gets executed.
///@Returns with the buy and sell side order price set.
void MeanRevertingTrading::TradingLogic() {
  if (watch_.msecs_from_midnight() < trading_start_utc_mfm_) {
    return;
  }

  // Set getflat appropriately.
  // Subcase 1: after specified endtime, getflat is set appropriately
  if (watch_.msecs_from_midnight() > trading_end_utc_mfm_) {
    for (int t_ctr_ = 0; t_ctr_ < num_total_products_; t_ctr_++) {
      should_be_getting_flat_[t_ctr_] = true;
    }
  }
  // Subcase 2:on breach of max loss or max opentrade loss*, getflat is set appropriately
  for (int t_ctr_ = 0; t_ctr_ < num_total_products_; t_ctr_++) {
    if (order_manager_vec_[t_ctr_]->base_pnl().total_pnl() < -1 * global_param_set_->product_maxloss_) {
      should_be_getting_flat_[t_ctr_] = true;
    }
  }

  // keep aggregate position under constraint
  int t_gpos_ = my_global_position();
  bool disallow_long_trades_ = (t_gpos_ > global_param_set_->portfolio_maxlots_ ? true : false);
  bool disallow_short_trades_ = (-1 * t_gpos_ > global_param_set_->portfolio_maxlots_ ? true : false);

  // logic for setting price bands here ..
  for (int t_ctr_ = 0; t_ctr_ < num_total_products_; t_ctr_++) {
    // set bid price first
    if (dont_trade_[t_ctr_] || product_position_in_lots_[t_ctr_] >= inst_maxlots_[t_ctr_] ||
        disallow_long_trades_ || (global_param_set_->use_book_indicator_ && book_indicator_values_[t_ctr_] < 0) ||
        (global_param_set_->use_trade_indicator_ && trade_indicator_values_[t_ctr_] < 0) ||
        (watch_.msecs_from_midnight() / 1000 - seconds_at_last_buy_[t_ctr_] < global_param_set_->cooloff_secs_ &&
         product_position_in_lots_[t_ctr_] >= 0) ||
        (bid_int_price_to_place_at_[t_ctr_] == INVALID_MEAN_REVERTING_ORDER_PRICE)) {
      bid_int_price_to_place_at_[t_ctr_] = INVALID_MEAN_REVERTING_ORDER_PRICE;
    } else {
      bid_int_price_to_place_at_[t_ctr_] =
          std::min(dep_market_view_vec_[t_ctr_]->bestbid_int_price(), bid_int_price_to_place_at_[t_ctr_]);
    }
    // set ask price
    if (dont_trade_[t_ctr_] || product_position_in_lots_[t_ctr_] + inst_maxlots_[t_ctr_] <= 0 ||
        disallow_short_trades_ || (global_param_set_->use_book_indicator_ && book_indicator_values_[t_ctr_] > 0) ||
        (global_param_set_->use_trade_indicator_ && trade_indicator_values_[t_ctr_] > 0) ||
        (watch_.msecs_from_midnight() / 1000 - seconds_at_last_sell_[t_ctr_] < global_param_set_->cooloff_secs_ &&
         product_position_in_lots_[t_ctr_] <= 0) ||
        (ask_int_price_to_place_at_[t_ctr_] == INVALID_MEAN_REVERTING_ORDER_PRICE)) {
      ask_int_price_to_place_at_[t_ctr_] = INVALID_MEAN_REVERTING_ORDER_PRICE;
    } else {
      ask_int_price_to_place_at_[t_ctr_] =
          std::max(dep_market_view_vec_[t_ctr_]->bestask_int_price(), ask_int_price_to_place_at_[t_ctr_]);
    }

    // specific handling of getflat cases
    if (should_be_getting_flat_[t_ctr_] || getflat_due_to_abnormal_volume_[t_ctr_]) {
      if (product_position_in_lots_[t_ctr_] == 0) {
        bid_int_price_to_place_at_[t_ctr_] = INVALID_MEAN_REVERTING_ORDER_PRICE;
        ask_int_price_to_place_at_[t_ctr_] = INVALID_MEAN_REVERTING_ORDER_PRICE;
      } else if (product_position_in_lots_[t_ctr_] > 0) {
        bid_int_price_to_place_at_[t_ctr_] = INVALID_MEAN_REVERTING_ORDER_PRICE;
        ask_int_price_to_place_at_[t_ctr_] = dep_market_view_vec_[t_ctr_]->bestask_int_price();
      } else {
        ask_int_price_to_place_at_[t_ctr_] = INVALID_MEAN_REVERTING_ORDER_PRICE;
        bid_int_price_to_place_at_[t_ctr_] = dep_market_view_vec_[t_ctr_]->bestbid_int_price();
      }
    }
  }
  PlaceAndCancelOrders();
}
void MeanRevertingTrading::GetFlatTradingLogic(int _product_index_) {
  int thisproduct_positions_ = product_position_in_lots_[_product_index_];
  if (thisproduct_positions_ == 0) {
    order_manager_vec_[_product_index_]->CancelAllOrders();
    should_be_getting_flat_[_product_index_] = true;
  } else if (thisproduct_positions_ > 0) {
    order_manager_vec_[_product_index_]->CancelAllBidOrders();
    should_be_getting_flat_[_product_index_] = true;
  } else if (thisproduct_positions_ < 0) {
    order_manager_vec_[_product_index_]->CancelAllAskOrders();
    should_be_getting_flat_[_product_index_] = true;
  }
}
void MeanRevertingTrading::GetAllFlat() {
  for (int i = 0; i < num_total_products_; i++) {
    GetFlatTradingLogic(i);
  }
}

void MeanRevertingTrading::LoadMarketShareFromDB()
{
#define NUM_DAYS_FOR_MR_MKTSHARE_CALCULATIONS 5
  //first get set of dates over which we compute historical market share
  std::vector<int> t_alldates_;
  
  sqlite3* dbconn;
  if (sqlite3_open_v2(DEF_MIDTERM_DB, &dbconn, SQLITE_OPEN_READONLY, NULL) != SQLITE_OK) {
    std::cerr << "Could not open DB .. error " << sqlite3_errmsg(dbconn) << '\n';
    dbglogger_ << "Could not open DB .. error " << sqlite3_errmsg(dbconn) << '\n';
    exit(-1);
  }
  char sql_stat[1024];
  sprintf(sql_stat, "select day from ALLDATES where day < %d order by day desc limit %d", watch_.YYYYMMDD(), NUM_DAYS_FOR_MR_MKTSHARE_CALCULATIONS);
  sqlite3_stmt* sql_prep_;
  if (sqlite3_prepare_v2(dbconn, sql_stat, -1, &sql_prep_, NULL) != SQLITE_OK) {
    std::cerr << "SQLite Error in preparing statement " << sqlite3_errmsg(dbconn) << '\n';
    dbglogger_ << "SQLite Error in preparing statement " << sqlite3_errmsg(dbconn) << '\n';
    exit(-1);
  }

  while (sqlite3_step(sql_prep_) == SQLITE_ROW) {
    t_alldates_.push_back(sqlite3_column_int(sql_prep_, 0));
  }
  
  std::vector<double> sum_mkt_share_(num_total_products_, 0.0);
  std::vector<double> curr_day_notional_(num_total_products_, 0.0);
  double curr_day_total_notional_ = 0.0;
  int num_valid_dates_ = 0;

  //for each day in above list and each stock we need to extract notional information
  for( auto t_day_iter_ = t_alldates_.begin(); t_day_iter_ != t_alldates_.end(); t_day_iter_++ ) {
    std::fill(curr_day_notional_.begin(), curr_day_notional_.end(), 0.0);
    curr_day_total_notional_ = 0.0;
    for( int t_ctr_ = 0; t_ctr_ < num_total_products_; t_ctr_++ )
    {
      //we ignore stocks/idx fut which are not going to be traded for mkt share 
      //calculation.
      if( !dont_trade_[t_ctr_] )
      {
	std::string t_str_(dep_market_view_vec_[t_ctr_]->shortcode());
	sprintf(sql_stat, "select notional from EQUITY_NOTIONAL where stock == \"%s\" and day == \"%d\" ", t_str_.substr(4, t_str_.length() - 9).c_str(), *t_day_iter_);
	if (sqlite3_prepare_v2(dbconn, sql_stat, -1, &sql_prep_, NULL) != SQLITE_OK) {
	  std::cerr << "SQLite Error in preparing statement " << sqlite3_errmsg(dbconn) << '\n';
	  dbglogger_ << "SQLite Error in preparing statement " << sqlite3_errmsg(dbconn) << '\n';
	  exit(-1);
	}
	
	if (sqlite3_step(sql_prep_) == SQLITE_ROW) {
	  curr_day_notional_[t_ctr_] =  sqlite3_column_double(sql_prep_, 0);
	  curr_day_total_notional_ += curr_day_notional_[t_ctr_];
	}
      }	
    }
    //offchance that data is not there in DB for day
    if( curr_day_total_notional_ > 0 )
    {
      num_valid_dates_ += 1;
      for( int t_ctr_ = 0; t_ctr_ < num_total_products_; t_ctr_++ )
      {
	if( !dont_trade_[t_ctr_] )
	{
	  if( curr_day_notional_[t_ctr_] <= 1e-5)
	  {
	    std::cerr << "Notional info selective unavailable for stock " << dep_market_view_vec_[t_ctr_]->shortcode() << '\n';
	    dbglogger_ << "Notional info selective unavailable for stock " << dep_market_view_vec_[t_ctr_]->shortcode() << '\n';	 
	  }
	  sum_mkt_share_[t_ctr_] += (curr_day_notional_[t_ctr_]/curr_day_total_notional_);
	}
      }
    }
  }
  //compute average mkt share info for each stock
  for( int t_ctr_ = 0; t_ctr_ < num_total_products_; t_ctr_++ )
  {
    if (!dont_trade_[t_ctr_])
    {
      dbglogger_ << "Avg market share for " << dep_market_view_vec_[t_ctr_]->shortcode() << ' ' <<  sum_mkt_share_[t_ctr_]/num_valid_dates_ << '\n';
      short_term_mkt_share_vec_[t_ctr_] = sum_mkt_share_[t_ctr_]/num_valid_dates_;
      long_term_mkt_share_vec_[t_ctr_] = sum_mkt_share_[t_ctr_]/num_valid_dates_;
      daily_mkt_share_vec_[t_ctr_] = sum_mkt_share_[t_ctr_]/num_valid_dates_;
    }
  }
}

// we load earnings dates from database. GetFlat is set if an earnings is close by.
void MeanRevertingTrading::SetDontTradeForEarnings() {
  std::vector<int> t_alldates_;
  std::map<int,int> all_dates_indices_;
  // Step 1. Create a map with indexes of all dates;
  sqlite3* dbconn;
  if (sqlite3_open_v2(DEF_MIDTERM_DB, &dbconn, SQLITE_OPEN_READONLY, NULL) != SQLITE_OK) {
    std::cerr << "Could not open DB .. error " << sqlite3_errmsg(dbconn) << '\n';
    dbglogger_ << "Could not open DB .. error " << sqlite3_errmsg(dbconn) << '\n';
    exit(-1);
  }
  // Prepare statement extracting all dates
  char sql_stat[1024];
  sprintf(sql_stat, "select day from ALLDATES order by day asc");
  sqlite3_stmt* sql_prep_;
  if (sqlite3_prepare_v2(dbconn, sql_stat, -1, &sql_prep_, NULL) != SQLITE_OK) {
    std::cerr << "SQLite Error in preparing statement " << sqlite3_errmsg(dbconn) << '\n';
    dbglogger_ << "SQLite Error in preparing statement " << sqlite3_errmsg(dbconn) << '\n';
    exit(-1);
  }

  int t_index_ = 0;
  while (sqlite3_step(sql_prep_) == SQLITE_ROW) {
    all_dates_indices_[sqlite3_column_int(sql_prep_, 0)] = t_index_;
    t_alldates_.push_back(sqlite3_column_int(sql_prep_, 0));
    t_index_++;
  }

  for( int t_ctr_ = 0; t_ctr_ < num_total_products_; t_ctr_++ )
  {
    // Step 2. Populate a sorted vector of indices of earnings dates. Current day is compared against this map/index
    // later.
    std::vector<int> earnings_dates_indices_;
    std::string t_str_(dep_market_view_vec_[t_ctr_]->shortcode());
    sprintf(sql_stat, "select DISTINCT day from EARNINGS_DATES where stock == \"%s\" order by day asc", t_str_.substr(4, t_str_.length() - 9).c_str());
    if (sqlite3_prepare_v2(dbconn, sql_stat, -1, &sql_prep_, NULL) != SQLITE_OK) {
      std::cerr << "SQLite Error in preparing statement " << sqlite3_errmsg(dbconn) << '\n';
      dbglogger_ << "SQLite Error in preparing statement " << sqlite3_errmsg(dbconn) << '\n';
      exit(-1);
    }

    while (sqlite3_step(sql_prep_) == SQLITE_ROW) {
      int t_curr_date_ = sqlite3_column_int(sql_prep_, 0);
      if (all_dates_indices_.find(t_curr_date_) != all_dates_indices_.end()) {
	earnings_dates_indices_.push_back(all_dates_indices_[t_curr_date_]);
      } else  // earnings date is on a weekend/holiday or a day absent in alldates ( eg missing logged data etc )
	{
	  std::vector<int>::iterator t_up_ = std::upper_bound(t_alldates_.begin(), t_alldates_.end(), t_curr_date_);
	  // Index of earnings day is t_up_ - t_alldates_.begin() ; we check for duplicates since different holidays can
	  // have same business day succedding it.
	  // edge cases ( earnings prior to first day of record or after last day of record ) are ignored
	  if (t_up_ != t_alldates_.begin() && t_up_ != t_alldates_.end() &&
	      (earnings_dates_indices_.size() == 0 ||
	       earnings_dates_indices_[earnings_dates_indices_.size() - 1] != (t_up_ - t_alldates_.begin()))) {
	    earnings_dates_indices_.push_back(t_up_ - t_alldates_.begin());
	  }
	}
    }

    int t_num_days_to_ = 100;
    int t_num_days_from_ = 100;
    if (all_dates_indices_.find(watch_.YYYYMMDD()) != all_dates_indices_.end()) {
      int current_day_comparison_index_ = all_dates_indices_[watch_.YYYYMMDD()];
      // find closest indices prior to and post this date
      std::vector<int>::iterator t_ubound_ =
        std::upper_bound(earnings_dates_indices_.begin(), earnings_dates_indices_.end(), current_day_comparison_index_);
      if (t_ubound_ != earnings_dates_indices_.end()) {
	t_num_days_to_ = (*t_ubound_ - current_day_comparison_index_);
      }
      if (t_ubound_ != earnings_dates_indices_.begin() && t_ubound_ != earnings_dates_indices_.end()) {
	t_ubound_--;
	t_num_days_from_ = (current_day_comparison_index_ - *t_ubound_);
      }
      if (t_num_days_to_ <= 1 || t_num_days_from_ <= 1) {
	dont_trade_[t_ctr_] = true;
	dbglogger_ << "Earnings GetFlat set for " << t_str_ << '\n';
      }
    }
  }
  dbglogger_.DumpCurrentBuffer();
  sqlite3_close(dbconn);
}

void MeanRevertingTrading::SetDontTradeForBan()
{
  std::ostringstream t_temp_oss;
  t_temp_oss << "/spare/local/tradeinfo/NSE_Files/SecuritiesUnderBan/fo_secban_" << watch_.YYYYMMDD() << ".csv";
  HFSAT::SecurityNameIndexer& sec_name_indexer_ = HFSAT::SecurityNameIndexer::GetUniqueInstance();

  std::ifstream fo_banned_securities_stream;
  fo_banned_securities_stream.open(t_temp_oss.str().c_str(), std::ifstream::in);

  char line_buffer[1024];

  while (fo_banned_securities_stream.good())
  {
    fo_banned_securities_stream.getline(line_buffer, 1024);
    if (std::string(line_buffer).length() < 1) continue;
    char t_secname_[24];
    sprintf(t_secname_,"NSE_%s_FUT0",line_buffer);
    int t_stkid_ = sec_name_indexer_.GetIdFromString(t_secname_);
    if( t_stkid_ >= 0)
    {
      dont_trade_[t_stkid_] = true;
      dbglogger_ << " DontTrade for " << t_secname_ << " set to True \n";
    }
  }
  fo_banned_securities_stream.close();
  dbglogger_.DumpCurrentBuffer();
}
}
