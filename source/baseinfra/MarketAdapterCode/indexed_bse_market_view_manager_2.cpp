/**
    \file MarketAdapter/indexed_eobi_market_view_manager.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */

#include "baseinfra/MarketAdapter/indexed_bse_market_view_manager_2.hpp"

namespace HFSAT {
#define BSE_V2_SANITIZE_TIME_THRESHOLD 50  // 50 msecs
#define BSE_TRIGGER_MKT_UPDATE_ON_TRADE 0  // if set to non-1 this will suppress OnMarketUpdate call on trade
                                           // if a trade message reflecting the same book state has been sent.
#define BSE_V2_EPSILON 1e-5
#define DISABLE_TRIGGER_ON_WIDE_SPREAD 0
#define WIDE_SPREAD_FOR_DISABLE_TRIGGER \
  0.01  // don't call trigger trade if spread > WIDE_SPREAD_FOR_DISABLE_TRIGGER*px and px >
        // MIN_PRICE_FOR_DISABLE_TRIGGER
#define MIN_PRICE_FOR_DISABLE_TRIGGER 20  //

IndexedBSEMarketViewManager2::IndexedBSEMarketViewManager2(
    DebugLogger &t_dbglogger_, const Watch &t_watch_, const SecurityNameIndexer &t_sec_name_indexer_,
    const std::vector<SecurityMarketView *> &t_security_market_view_map_, bool _use_self_book_)
    : BaseMarketViewManager(t_dbglogger_, t_watch_, t_sec_name_indexer_, t_security_market_view_map_),
      trade_time_manager_(TradeTimeManager::GetUniqueInstance(t_sec_name_indexer_, t_watch_.YYYYMMDD())),
      order_history_instance(t_sec_name_indexer_.NumSecurityId()),
      self_order_history_instance(t_sec_name_indexer_.NumSecurityId()),
      px_level_manager_(t_dbglogger_, t_watch_, t_sec_name_indexer_.NumSecurityId()),
      use_self_book_(_use_self_book_),
      is_auction_vec_(t_sec_name_indexer_.NumSecurityId(),true),
      market_order_auction_price_(std::numeric_limits<long>::max()*1.0/100000000),
      big_trades_listener_vec_(t_sec_name_indexer_.NumSecurityId()) {
//  dbglogger_ << "IndexedBSEMarketViewManager2::IndexedBSEMarketViewManager2 constructor called\n";
  // Set px level manager for all BSE shortcodes
  self_trader_ids_.clear();
  for (unsigned int t_ctr_ = 0; t_ctr_ < t_sec_name_indexer_.NumSecurityId(); t_ctr_++) {
    if (strncmp((t_security_market_view_map_[t_ctr_]->shortcode()).c_str(), "BSE_", 4) == 0) {
      t_security_market_view_map_[t_ctr_]->SetPxLevelManager(&px_level_manager_);
      SecurityMarketView &smv_ = *(t_security_market_view_map_[t_ctr_]);
      market_view_ptr_ = &smv_;
      SetSMVBestVars(t_ctr_);
      px_level_manager_.CheckAndSetPredictiveUncross(t_ctr_, t_security_market_view_map_[t_ctr_]->shortcode());
      smv_.using_predictive_uncross_ = px_level_manager_.UsingPredictiveUncross(t_ctr_);
      HFSAT::BSETradingLimit_t *t_trading_limit_ =
          HFSAT::BSESecurityDefinitions::GetTradingLimits(t_security_market_view_map_[t_ctr_]->shortcode());
      if ((t_trading_limit_ != NULL) && (t_trading_limit_->upper_limit_ != 0) &&
          (t_trading_limit_->lower_limit_ != 0)) {
        smv_.upper_int_price_limit_ = t_trading_limit_->upper_limit_;
        smv_.lower_int_price_limit_ = t_trading_limit_->lower_limit_;
      } else {
        //dbglogger_ << "Error: Price Limit not present for " << smv_.shortcode() << "\n";
      }

/*
      std::cout << "Price Limit for " << smv_.shortcode() << " : "
                 << smv_.lower_int_price_limit_ * smv_.min_price_increment_ << "-"
                 << smv_.upper_int_price_limit_ * smv_.min_price_increment_ << "\n";
*/
    }
  }
}

// debug function - not intented to be used in prod
void IndexedBSEMarketViewManager2::CheckL1Consistency(int security_id_) {
  SecurityMarketView &smv_ = *market_view_ptr_;
  // Check consistency of security markey view market_update_info struct with
  // sparse_index_book_px_level_manager state - should be the state at every call
  // to listeners and end of processing of messages
  if (smv_.market_update_info_.bestbid_size_ != px_level_manager_.GetSynBidSize(security_id_, 0) ||
      smv_.market_update_info_.bestask_size_ != px_level_manager_.GetSynAskSize(security_id_, 0) ||
      fabs(smv_.market_update_info_.bestbid_price_ - px_level_manager_.GetSynBidPrice(security_id_, 0)) >
          BSE_V2_EPSILON ||
      fabs(smv_.market_update_info_.bestask_price_ - px_level_manager_.GetSynAskPrice(security_id_, 0)) >
          BSE_V2_EPSILON) {
/*
    std::cout << " PxMgrDiff_L1 " << smv_.market_update_info_.bestbid_size_ << " @ "
               << smv_.market_update_info_.bestbid_price_ << " --- " << smv_.market_update_info_.bestask_price_ << " @ "
               << smv_.market_update_info_.bestask_size_ << " <> " << px_level_manager_.GetSynBidSize(security_id_, 0)
               << " @ " << px_level_manager_.GetSynBidPrice(security_id_, 0) << " --- "
               << px_level_manager_.GetSynAskPrice(security_id_, 0) << " @ "
               << px_level_manager_.GetSynAskSize(security_id_, 0) << '\n';
*/
  }
}


void IndexedBSEMarketViewManager2::OnOrderAdd(const uint32_t t_security_id_, const TradeType_t t_buysell_,
                                              const uint64_t order_id_, const double t_price_, const uint32_t t_size_,
                                              const bool t_is_intermediate_) {

/*    std::cout << watch_.tv() << " OrderID: " <<  order_id_ << " OrderAdd@" << t_price_ << " of size " << t_size_ << " " 
	      << ((t_buysell_ == HFSAT::TradeType_t::kTradeTypeSell) ? "SELL":"BUY") << std::endl;*/

  if((is_auction_vec_[t_security_id_])  && (CheckValidTime(t_security_id_))) {
    std::cout << "IndexedBSEMarketViewManager2::OnOrderAdd:  Auction closed" << std::endl;
    is_auction_vec_[t_security_id_] = false;
  }

  // Market order in auction period has prices close to int_max/10^8
  if((is_auction_vec_[t_security_id_]) && ((t_price_ > (market_order_auction_price_-100)) || (t_price_ < (-1*market_order_auction_price_+100)))) {
    return;
  }

  SecurityMarketView &smv_ = *(security_market_view_map_[t_security_id_]);
  market_view_ptr_ = &smv_;
  int int_price_ = smv_.GetIntPx(t_price_);
/*  if(int_price_ > smv_.upper_int_price_limit_ || int_price_ < smv_.lower_int_price_limit_){
    std::cout << "IndexedBSEMarketViewManager2::OnOrderAdd: Prices are OOB, px :: " << t_price_ << std::endl;
    return;
  }*/
  MarketUpdateInfoLevelStruct *t_best_bid_level_ = px_level_manager_.GetSynBidPL(t_security_id_, 0);
  MarketUpdateInfoLevelStruct *t_best_ask_level_ = px_level_manager_.GetSynAskPL(t_security_id_, 0);

//  std::cout << "is_book_crossed_: " << smv_.is_book_crossed_  << " At_px: " << px_level_manager_.IsBookCrossed(t_security_id_) << "\n";
            //<< " Time : " << watch_.msecs_from_midnight() - px_level_manager_.InitBookCrossTime(t_security_id_)
	    //<< " " << BSE_V2_SANITIZE_TIME_THRESHOLD << "\n";

  if(!is_auction_vec_[t_security_id_]) {
    if(t_buysell_ == HFSAT::kTradeTypeBuy && t_best_ask_level_ != NULL &&
       t_price_ > t_best_ask_level_->limit_price_ - BSE_V2_EPSILON && t_best_ask_level_->limit_int_price_ != kInvalidIntPrice) {

/*    std::cout << std::fixed << " SanitizeBookOnOrderAdd triggered at SELL " 
             << t_price_ << " > " << t_best_ask_level_->limit_price_ - BSE_V2_EPSILON << watch_.tv() << '\n';
    std::cout << "Order History size at px: " << t_price_
             << " SIZE: " << order_history_instance.GetSizeAtPrice(t_security_id_, t_price_, HFSAT::kTradeTypeBuy)
              << " X " <<  order_history_instance.GetSizeAtPrice(t_security_id_, t_price_, HFSAT::kTradeTypeSell)
             << std::endl;*/

      SanitizeBook(t_security_id_, HFSAT::kTradeTypeSell, t_price_ + 2 * BSE_V2_EPSILON);

    } else if(t_buysell_ == HFSAT::kTradeTypeSell && t_best_bid_level_ != NULL &&
      t_price_ < t_best_bid_level_->limit_price_ + BSE_V2_EPSILON && t_best_bid_level_->limit_int_price_ != kInvalidIntPrice) {

/*    std::cout << std::fixed << " SanitizeBookOnOrderAdd triggered at BUY " 
             << t_price_ << " < " << t_best_bid_level_->limit_price_ + BSE_V2_EPSILON << watch_.tv() << '\n';*/

      SanitizeBook(t_security_id_, HFSAT::kTradeTypeBuy, t_price_ - 2 * BSE_V2_EPSILON);

    } else if (smv_.is_book_crossed_) {
//      std::cout << std::fixed << " SanitizeBookOnOrderAdd Book Crossed " << '\n';
      SanitizeBookOnCrossedBook(t_security_id_);
    }
  }

  if ((smv_.lower_int_price_limit_ != -1) &&
      ((int_price_ > smv_.upper_int_price_limit_) || (int_price_ < smv_.lower_int_price_limit_))) {
    HFSAT::BSETradingLimit_t *t_trading_limit_ =
        HFSAT::BSESecurityDefinitions::ChangeTradingLimits(smv_.shortcode(), int_price_);
    if (t_trading_limit_ != NULL) {

      std::cout << "Price Limit crossed for shortcode " << smv_.shortcode()
                 << " PrevUL: " << smv_.upper_int_price_limit_ * 0.05
                 << " PrevLL: " << smv_.lower_int_price_limit_ * 0.05 << " OrderPrice: " << t_price_
                 << " NewUL: " << t_trading_limit_->upper_limit_ * 0.05
                 << " NewLL: " << t_trading_limit_->lower_limit_ * 0.05 << '\n';

      smv_.upper_int_price_limit_ = t_trading_limit_->upper_limit_;
      smv_.lower_int_price_limit_ = t_trading_limit_->lower_limit_;
      smv_.NotifyCircuitListeners();
    } else {
      //std::cout << "ERROR: Trading limit existed before but not now for " << smv_.shortcode() << '\n';
    }
  }

  if (use_self_book_) {
    // noop if self order
    if (self_order_history_instance.IsOrderSeen(t_security_id_, order_id_)) {
      return;
    }
  }

  LevelChangeType_t event_type_ = k_nochange;
  bool is_crossing_index_ = false;

  //  bool is_order_seen = order_history_instance.IsOrderSeen(t_security_id_, order_id_);
  //  if (is_order_seen) {
  //    dbglogger_ << "Incorrect AddOrder state - existing orderid added " << order_id_ << '\n';
  //    return;
  //  }

  smv_.market_update_info_.tradetype = t_buysell_;
  switch (t_buysell_) {
    case kTradeTypeBuy: {
      MarketUpdateInfoLevelStruct *t_syn_ask_level_ = smv_.syn_best_ask_level_info_;
      // check and trigger trades on agg add
      if (t_syn_ask_level_ != NULL && t_syn_ask_level_->limit_int_price_ <= int_price_) {
        is_crossing_index_ = true;
      }
      event_type_ = px_level_manager_.AddOrderToPxLevel(t_security_id_, t_buysell_, int_price_, t_price_, t_size_);
      SetSMVBestVars(t_security_id_);
    } break;
    case kTradeTypeSell: {
      MarketUpdateInfoLevelStruct *t_syn_bid_level_ = smv_.syn_best_bid_level_info_;
      // check and trigger trades on agg add
      if (t_syn_bid_level_ != NULL && t_syn_bid_level_->limit_int_price_ >= int_price_) {
        is_crossing_index_ = true;
      }
      event_type_ = px_level_manager_.AddOrderToPxLevel(t_security_id_, t_buysell_, int_price_, t_price_, t_size_);
      SetSMVBestVars(t_security_id_);
    } break;
    default:
      break;
  }

  if (!is_crossing_index_ || !(smv_.using_predictive_uncross_))
    NotifyListenersOnLevelChange(t_security_id_, event_type_);

  smv_.is_book_crossed_ = px_level_manager_.IsBookCrossed(t_security_id_);
  if(order_history_instance.IsOrderSeen(t_security_id_, order_id_)){
    OrderDetailsStruct *t_order_ = order_history_instance.GetOrderDetails(t_security_id_, order_id_);
    OnOrderDelete(t_security_id_, t_buysell_, order_id_, t_order_->order_price, true, true);
  }
  order_history_instance.AddOrderToOrderHistory(t_security_id_, t_buysell_, t_price_, t_size_, order_id_);

//  std::cout << smv_.ShowMarket() << std::endl;
  //CheckL1Consistency(t_security_id_);
}

// Notify listeners on Trade
void IndexedBSEMarketViewManager2::NotifyListenersOnTrade(const uint32_t t_security_id_, const int t_trade_int_price_,
                                                          const int t_trade_size_, const TradeType_t t_buysell_,
                                                          bool is_intermediate, int _num_levels_cleared_) {
  SecurityMarketView &smv_ = *market_view_ptr_;
  smv_.is_ready_ = true;

  double trade_price_ = smv_.GetDoublePx(t_trade_int_price_);

  smv_.trade_print_info_.trade_price_ = trade_price_;
  smv_.trade_print_info_.size_traded_ = t_trade_size_;
  smv_.trade_print_info_.int_trade_price_ = t_trade_int_price_;
  smv_.trade_print_info_.buysell_ = t_buysell_;
  smv_.trade_print_info_.is_intermediate_ = is_intermediate;
  smv_.trade_print_info_.num_levels_cleared_ = _num_levels_cleared_;

  //  CheckL1Consistency(t_security_id_);
  if (/*(CheckValidTime(t_security_id_)) && */((t_trade_int_price_ <= smv_.upper_int_trade_range_limit_) &&
                                           (t_trade_int_price_ >= smv_.lower_int_trade_range_limit_))) {
    smv_.NotifyTradeListeners();
    smv_.NotifyOnReadyListeners();
  }
}

void IndexedBSEMarketViewManager2::SetTimeToSkipUntilFirstEvent(const ttime_t r_start_time_) {
  for (auto i = 0u; i < security_market_view_map_.size(); i++) {
    SecurityMarketView *smv_ = security_market_view_map_[i];
    smv_->set_skip_listener_notification_end_time(r_start_time_);
  }
}

void IndexedBSEMarketViewManager2::SetSMVBestVars(int t_security_id_) {
  SecurityMarketView &smv_ = *market_view_ptr_;
  MarketUpdateInfoLevelStruct *best_bid_level_ = px_level_manager_.GetSynBidPL(t_security_id_, 0);
  MarketUpdateInfoLevelStruct *best_ask_level_ = px_level_manager_.GetSynAskPL(t_security_id_, 0);

  smv_.market_update_info_.bestbid_price_ = (best_bid_level_ != NULL ? best_bid_level_->limit_price_ : kInvalidPrice);
  smv_.market_update_info_.bestbid_int_price_ =
      (best_bid_level_ != NULL ? best_bid_level_->limit_int_price_ : kInvalidIntPrice);
  smv_.market_update_info_.bestbid_size_ = (best_bid_level_ != NULL ? best_bid_level_->limit_size_ : kInvalidSize);
  smv_.market_update_info_.bestbid_ordercount_ = (best_bid_level_ != NULL ? best_bid_level_->limit_ordercount_ : 0);
  smv_.market_update_info_.bestask_price_ = (best_ask_level_ != NULL ? best_ask_level_->limit_price_ : kInvalidPrice);
  smv_.market_update_info_.bestask_int_price_ =
      (best_ask_level_ != NULL ? best_ask_level_->limit_int_price_ : kInvalidIntPrice);
  smv_.market_update_info_.bestask_size_ = (best_ask_level_ != NULL ? best_ask_level_->limit_size_ : kInvalidSize);
  smv_.market_update_info_.bestask_ordercount_ = (best_ask_level_ != NULL ? best_ask_level_->limit_ordercount_ : 0);
  smv_.syn_best_bid_level_info_ = best_bid_level_;
  smv_.syn_best_ask_level_info_ = best_ask_level_;

  // mid/mkt prices and spread increments might be used in pnl etc classes so those need to be set as well
  if (smv_.price_type_subscribed_[kPriceTypeMidprice]) {
    if (best_bid_level_ == NULL || best_ask_level_ == NULL) {
      // smv_.is_ready_ = false;
      smv_.market_update_info_.mid_price_ = kInvalidPrice;
    } else {
      smv_.market_update_info_.mid_price_ =
          (smv_.market_update_info_.bestbid_price_ + smv_.market_update_info_.bestask_price_) * 0.5;
    }
  }

  smv_.market_update_info_.spread_increments_ =
      smv_.market_update_info_.bestask_int_price_ - smv_.market_update_info_.bestbid_int_price_;

  if (smv_.price_type_subscribed_[kPriceTypeMktSizeWPrice]) {
    if (best_bid_level_ == NULL || best_ask_level_ == NULL) {
      // smv_.is_ready_ = false;
      smv_.market_update_info_.mkt_size_weighted_price_ = kInvalidPrice;
    } else {
      if (smv_.market_update_info_.spread_increments_ <= 1) {
        smv_.market_update_info_.mkt_size_weighted_price_ =
            (smv_.market_update_info_.bestbid_price_ * smv_.market_update_info_.bestask_size_ +
             smv_.market_update_info_.bestask_price_ * smv_.market_update_info_.bestbid_size_) /
            (smv_.market_update_info_.bestbid_size_ + smv_.market_update_info_.bestask_size_);
      } else {
        smv_.market_update_info_.mkt_size_weighted_price_ =
            ((smv_.market_update_info_.bestbid_price_ * smv_.market_update_info_.bestask_size_ +
              smv_.market_update_info_.bestask_price_ * smv_.market_update_info_.bestbid_size_) /
                 (smv_.market_update_info_.bestbid_size_ + smv_.market_update_info_.bestask_size_) +
             (smv_.market_update_info_.mid_price_)) /
            2.0;
      }
    }
  }

  // set is_ready
  //  if (!smv_.is_ready_ && best_bid_level_ != NULL && best_ask_level_ != NULL) {
  //    smv_.is_ready_ = true;
  //  }

  smv_.market_update_info_.spread_increments_ =
      smv_.market_update_info_.bestask_int_price_ - smv_.market_update_info_.bestbid_int_price_;
}

// Notifying listeners on a level change. Happens after order add/modify or delete.
void IndexedBSEMarketViewManager2::NotifyListenersOnLevelChange(const uint32_t t_security_id_,
                                                                LevelChangeType_t event_type_) {
  SecurityMarketView &smv_ = *market_view_ptr_;
  smv_.is_ready_ = true;

  //  CheckL1Consistency(t_security_id_);
//  if (CheckValidTime(t_security_id_)) {
    // Notify relevant listeners about the update
    if (event_type_ == k_l1price) {
      smv_.NotifyL1PriceListeners();
    } else if (event_type_ == k_l1size) {
      smv_.NotifyL1SizeListeners();
    } else if (event_type_ == k_l2change) {
      smv_.NotifyL2Listeners();
      smv_.NotifyL2OnlyListeners();
    }
//  }
}

// Handling Order Modify message:
void IndexedBSEMarketViewManager2::OnOrderModify(const uint32_t t_security_id_, const TradeType_t t_buysell_, const uint64_t t_prev_order_id_,
                                                 const uint64_t order_id_, const double t_price_, const uint32_t t_size_, 
						 const double t_prev_price_, const uint32_t t_prev_size_) {

/*    std::cout << watch_.tv() << " Prev, New OrderID: " << t_prev_order_id_ << " " << order_id_ 
	      << " Prev_px: " << t_prev_price_ << " Prev_size: " << t_prev_size_
	      << " OrderModify@" << t_price_ << " of size " << t_size_ 
	      << " " << ((t_buysell_ == HFSAT::TradeType_t::kTradeTypeSell) ? "SELL":"BUY") << std::endl;*/

  if((is_auction_vec_[t_security_id_])  && (CheckValidTime(t_security_id_))) {
    std::cout << "IndexedBSEMarketViewManager2::OnOrderModify:  Auction closed" << std::endl;
    is_auction_vec_[t_security_id_] = false;
  }

  SecurityMarketView &smv_ = *(security_market_view_map_[t_security_id_]);
  market_view_ptr_ = &smv_;
 
//  if(smv_.GetIntPx(t_price_) > smv_.upper_int_price_limit_ || smv_.GetIntPx(t_price_) < smv_.lower_int_price_limit_){/* ||
//    smv_.GetIntPx(t_prev_price_) > smv_.upper_int_price_limit_ || smv_.GetIntPx(t_prev_price_) < smv_.lower_int_price_limit_){*/

    //std::cout << "IndexedBSEMarketViewManager2::OnOrderModify: Prices are OOB, prev_px px :: "
    //          << t_prev_price_ << " " << t_price_ << std::endl;
  //return;
//}
/*  
  if(smv_.GetIntPx(t_prev_price_) > smv_.upper_int_price_limit_ || smv_.GetIntPx(t_prev_price_) < smv_.lower_int_price_limit_){
    std::cout << "IndexedBSEMarketViewManager2::OnOrderModify: PREV_Prices are OOB, prev_px px :: "
              << t_prev_price_ << " " << t_price_ << std::endl;
  }
*/
//  std::cout << "is_book_crossed_: " << smv_.is_book_crossed_ << " At_px: " << px_level_manager_.IsBookCrossed(t_security_id_) << "\n";
          
  if(t_price_ <= 0){ // || t_prev_price_ <= 0){
    //std::cout << "OnOrderModify Price Issue: prev_px px ::" << t_prev_price_ << " " << t_price_ << std::endl; 
    return ;
  }

  MarketUpdateInfoLevelStruct *t_best_bid_level_ = px_level_manager_.GetSynBidPL(t_security_id_, 0);
  MarketUpdateInfoLevelStruct *t_best_ask_level_ = px_level_manager_.GetSynAskPL(t_security_id_, 0);
/*  if(t_best_bid_level_ != NULL && t_best_ask_level_ != NULL){
    std::cout << std::fixed 
    	      << "best level: " << t_best_bid_level_->limit_price_ + BSE_V2_EPSILON
	      << " X " << t_best_ask_level_->limit_price_ - BSE_V2_EPSILON << std::endl;
  }else{
    std::cout << "best Level is NULL\n";
  }
*/

  //Since BSE provides details of Orders which will stay in current book 
  //we need to check if book is crossed or not due to current market update.
  //If the book is getting crossed then we need to sanitize the opposite side book.
 
  if(!is_auction_vec_[t_security_id_]) {
    if(t_buysell_ == HFSAT::kTradeTypeBuy && t_best_ask_level_ != NULL &&
       t_price_ > t_best_ask_level_->limit_price_ - BSE_V2_EPSILON && t_best_ask_level_->limit_int_price_ != kInvalidIntPrice){

/*    std::cout << std::fixed << " SanitizeBookOnOrderModify triggered at SELL " 
	      << t_price_ << " > " << t_best_ask_level_->limit_price_ - BSE_V2_EPSILON << watch_.tv() << '\n';
    std::cout << "Order History size at px: " << t_price_
	      << " SIZE: " << order_history_instance.GetSizeAtPrice(t_security_id_, t_price_, HFSAT::kTradeTypeBuy)
              << " X " <<  order_history_instance.GetSizeAtPrice(t_security_id_, t_price_, HFSAT::kTradeTypeSell)
	      << std::endl;*/

      SanitizeBook(t_security_id_, HFSAT::kTradeTypeSell, t_price_ + 2 * BSE_V2_EPSILON);

    }else if(t_buysell_ == HFSAT::kTradeTypeSell && t_best_bid_level_ != NULL &&
      t_price_ < t_best_bid_level_->limit_price_ + BSE_V2_EPSILON && t_best_bid_level_->limit_int_price_ != kInvalidIntPrice) {

/*    std::cout << std::fixed << " SanitizeBookOnOrderModify triggered at BUY " 
	      << t_price_ << " < " << t_best_bid_level_->limit_price_ + BSE_V2_EPSILON << watch_.tv() << '\n';*/
      SanitizeBook(t_security_id_, HFSAT::kTradeTypeBuy, t_price_ - 2 * BSE_V2_EPSILON);

    }else if (smv_.is_book_crossed_) {
//      std::cout << std::fixed << " SanitizeBookOnOrderModify Book Crossed " << '\n';
      SanitizeBookOnCrossedBook(t_security_id_);
    }
  }

  OrderDetailsStruct *t_order_ = order_history_instance.GetOrderDetails(t_security_id_, t_prev_order_id_); // order_id_);
  if (t_order_ == NULL) {
    // Order isn't present in our history. Simulate an OnOrderAdd
    //std::cout << "Order: " << t_prev_order_id_ << " not present in our history" << std::endl;
    OnOrderAdd(t_security_id_, t_buysell_, order_id_, t_price_, t_size_, true);
    return;
  }
/*
  if(t_order_->order_price != t_prev_price_ || t_order_->order_size != (int32_t)t_prev_size_){
    std::cout << std::fixed << "DIFF PRICE/SIZE:: Actual: " << t_order_->order_price << " " << t_order_->order_size
	      << " Data_Details: " << t_prev_price_ << " " << t_prev_size_ << std::endl;
  }
*/
  //If Order_id changes then update the order details in order history
  if(t_prev_order_id_ != order_id_){
    //std::cout << "UpdateOrderId: prev, new_order_id:: " << t_prev_order_id_ << " " << order_id_ << std::endl;
    order_history_instance.UpdateOrderId(t_security_id_, t_buysell_, t_prev_order_id_, order_id_, t_prev_price_, t_prev_size_);
    t_order_ = order_history_instance.GetOrderDetails(t_security_id_, order_id_);

    if (t_order_ == NULL) {
      // Order isn't present in our history. Simulate an OnOrderAdd
//      std::cout << "2ndTimeOrder: " << t_prev_order_id_ << " not present in our history" << std::endl;
      OnOrderAdd(t_security_id_, t_buysell_, order_id_, t_price_, t_size_, true);
      return;
    }
  }

  bool is_trade_triggered_ = false;
  LevelChangeType_t event_type_;

  int int_price_ = smv_.GetIntPx(t_price_);
  if ((smv_.lower_int_price_limit_ != -1) &&
      ((int_price_ > smv_.upper_int_price_limit_) || (int_price_ < smv_.lower_int_price_limit_))) {
    HFSAT::BSETradingLimit_t *t_trading_limit_ =
        HFSAT::BSESecurityDefinitions::ChangeTradingLimits(smv_.shortcode(), int_price_);
    if (t_trading_limit_ != NULL) {

      std::cout << "Price Limit crossed for shortcode " << smv_.shortcode()
                 << " PrevUL: " << smv_.upper_int_price_limit_ * 0.05
                 << " PrevLL: " << smv_.lower_int_price_limit_ * 0.05 << " OrderPrice: " << t_price_
                 << " NewUL: " << t_trading_limit_->upper_limit_ * 0.05
                 << " NewLL: " << t_trading_limit_->lower_limit_ * 0.05 << '\n';

      smv_.upper_int_price_limit_ = t_trading_limit_->upper_limit_;
      smv_.lower_int_price_limit_ = t_trading_limit_->lower_limit_;
      smv_.NotifyCircuitListeners();
    } else {
      //std::cout << "ERROR: Trading limit existed before but not now for " << smv_.shortcode() << '\n';
    }
  }

  smv_.market_update_info_.tradetype = t_buysell_;
  // Modify buy order
  if (t_order_->is_buy_order) {
    // store pre-modify L1 ask values
    if (smv_.DblPxCompare(t_order_->order_price, t_price_)) {
      event_type_ = px_level_manager_.ModifySizeAtPxLevel(t_security_id_, t_buysell_, int_price_,
                                                          t_size_ - t_order_->order_size, false);
    } else {
      event_type_ =
          px_level_manager_.ModifyOrderAtDiffLevels(t_security_id_, t_buysell_, smv_.GetIntPx(t_order_->order_price),
                                                    t_order_->order_size, int_price_, t_price_, t_size_);
    }
    SetSMVBestVars(t_security_id_);
  } else {
    // Modify sell order
    // store pre-modify L1 bid values
    if (smv_.DblPxCompare(t_order_->order_price, t_price_)) {
      event_type_ = px_level_manager_.ModifySizeAtPxLevel(t_security_id_, t_buysell_, int_price_,
                                                          t_size_ - t_order_->order_size, false);
    } else {
      event_type_ =
          px_level_manager_.ModifyOrderAtDiffLevels(t_security_id_, t_buysell_, smv_.GetIntPx(t_order_->order_price),
                                                    t_order_->order_size, int_price_, t_price_, t_size_);
    }
    SetSMVBestVars(t_security_id_);
  }

  t_order_->order_size = t_size_;
  t_order_->order_price = t_price_;

  if (!is_trade_triggered_) {
    NotifyListenersOnLevelChange(t_security_id_, event_type_);
  }
  smv_.is_book_crossed_ = px_level_manager_.IsBookCrossed(t_security_id_);
//   std::cout << smv_.ShowMarket() << std::endl;
  //CheckL1Consistency(t_security_id_);
}

void IndexedBSEMarketViewManager2::OnOrderDelete(const uint32_t t_security_id_, const TradeType_t t_buysell_,
                                                 const uint64_t order_id_, const double t_price_,
                                                 const bool t_delete_order_, const bool t_is_intermediate_) {

/*  std::cout << watch_.tv() << " OrderID: " <<  order_id_ << " OrderDelete@" << t_price_ //<< " of size " << t_size_
              << " " << ((t_buysell_ == HFSAT::TradeType_t::kTradeTypeSell) ? "SELL":"BUY") << std::endl;*/

  if((is_auction_vec_[t_security_id_])  && (CheckValidTime(t_security_id_))) {
  std::cout << "IndexedBSEMarketViewManager2::OnOrderDelete:  Auction closed" << std::endl;
    is_auction_vec_[t_security_id_] = false;
  }

  SecurityMarketView &smv_ = *(security_market_view_map_[t_security_id_]);
  market_view_ptr_ = &smv_;

/*  if(smv_.GetIntPx(t_price_) > smv_.upper_int_price_limit_ || smv_.GetIntPx(t_price_) < smv_.lower_int_price_limit_){
    //std::cout << "IndexedBSEMarketViewManager2::OnOrderDelete: Prices are OOB, px :: " << t_price_ << std::endl;
    return;
  }*/

  //std::cout << "is_book_crossed_: " << smv_.is_book_crossed_ << " At_px: " << px_level_manager_.IsBookCrossed(t_security_id_) << "\n";

  if ((!is_auction_vec_[t_security_id_]) && (smv_.is_book_crossed_)) {
//    std::cout << std::fixed << " SanitizeBookOnOrderDelete Book Crossed " << '\n';
    SanitizeBookOnCrossedBook(t_security_id_);
  }

  OrderDetailsStruct *t_order_ = order_history_instance.GetOrderDetails(t_security_id_, order_id_);
  LevelChangeType_t event_type_ = k_nochange;
  if (t_order_ == NULL) {
    // Order is not seen. Delete call is an invalid one. Return ..
    // Removing debug information since this happens a few times.
    //std::cout << " OrderDelete called for order not present in map " << order_id_ << '\n';
    return;
  } else {
    
    event_type_ = px_level_manager_.ModifySizeAtPxLevel(
        t_security_id_, t_buysell_, smv_.GetIntPx(t_order_->order_price), 0 - t_order_->order_size, true);

  }
/*
    std::cout << watch_.tv() << " OrderID: " <<  order_id_ << " OrderDelete@" << t_price_ << " of size " <<
    t_order_->order_size << " " << ((t_buysell_ == HFSAT::TradeType_t::kTradeTypeSell) ? "SELL":"BUY") << std::endl;
*/
  smv_.market_update_info_.tradetype = t_buysell_;
  order_history_instance.DeleteOrderFromHistory(t_security_id_, t_order_->order_id);

  SetSMVBestVars(t_security_id_);
  NotifyListenersOnLevelChange(t_security_id_, event_type_);
  smv_.is_book_crossed_ = px_level_manager_.IsBookCrossed(t_security_id_);

  //std::cout << smv_.ShowMarket() << std::endl;
  //CheckL1Consistency(t_security_id_);
  //  if (!t_is_intermediate_) {
  //  CheckL1Consistency(t_security_id_);
  //  }
}

void IndexedBSEMarketViewManager2::OnOrderMassDelete(const uint32_t t_security_id_){}

void IndexedBSEMarketViewManager2::OnTrade(const uint32_t t_security_id_, const TradeType_t t_buysell_, const uint64_t order_id_, 
					   const double t_trade_price_, const int32_t t_trade_size_){
//  std::cout << watch_.tv() << " Order ID: " << order_id_ << " OnTrade@" << t_trade_price_ << " of size " << t_trade_size_ << std::endl;

  if((is_auction_vec_[t_security_id_])  && (CheckValidTime(t_security_id_))) {
    std::cout << "IndexedBSEMarketViewManager2::OnTrade:  Auction closed" << std::endl;
    is_auction_vec_[t_security_id_] = false;
  }

  SecurityMarketView &smv_ = *(security_market_view_map_[t_security_id_]);
  market_view_ptr_ = &smv_;

/*  if(smv_.GetIntPx(t_trade_price_) > smv_.upper_int_price_limit_ || smv_.GetIntPx(t_trade_price_) < smv_.lower_int_price_limit_){
    //std::cout << "IndexedBSEMarketViewManager2::OnTrade: Prices are OOB, px :: " << t_trade_price_ << std::endl;
    return;
  }*/


  bool t_book_crossed_ = smv_.is_book_crossed_;
  if(!is_auction_vec_[t_security_id_]) {
    SanitizeBookOnCrossedTrade(t_security_id_, t_trade_price_);
  }

  int t_trade_int_price_ = smv_.GetIntPx(t_trade_price_);

  bool is_order_seen_ = order_history_instance.IsOrderSeen(t_security_id_, order_id_);

  //std::cout << "is_order_seen_ : " << is_order_seen_ << std::endl; 
  TradeType_t trd_type_ = kTradeTypeBuy;
  LevelChangeType_t event_type_ = k_nochange;

  if (is_order_seen_ && t_buysell_ == HFSAT::kTradeTypeBuy) {
    OrderDetailsStruct *t_order_ = order_history_instance.GetOrderDetails(t_security_id_, order_id_);
    int t_del_size_ = std::min(t_order_->order_size, t_trade_size_);
    event_type_ = px_level_manager_.ModifySizeAtPxLevel(t_security_id_, HFSAT::kTradeTypeBuy,
                                                        smv_.GetIntPx(t_order_->order_price), -t_del_size_,
                                                        (t_del_size_ == t_order_->order_size));
    trd_type_ = kTradeTypeSell;
    t_order_->order_size -= t_del_size_;
    if (t_order_->order_size == 0) {
      order_history_instance.DeleteOrderFromHistory(t_security_id_, t_order_->order_id);
    }
  }else if (is_order_seen_ && t_buysell_ == HFSAT::kTradeTypeSell) {
    OrderDetailsStruct *t_order_ = order_history_instance.GetOrderDetails(t_security_id_, order_id_);
    int t_del_size_ = std::min(t_order_->order_size, t_trade_size_);
    LevelChangeType_t t_event_type_ = px_level_manager_.ModifySizeAtPxLevel(
        t_security_id_, HFSAT::kTradeTypeSell, smv_.GetIntPx(t_order_->order_price), -t_del_size_,
        (t_del_size_ == t_order_->order_size));
    if ((event_type_ != k_l1price) && (event_type_ != k_l1size)) {
      event_type_ = t_event_type_;
    }
    trd_type_ = kTradeTypeBuy;
    t_order_->order_size -= t_del_size_;
    if (t_order_->order_size == 0) {
      order_history_instance.DeleteOrderFromHistory(t_security_id_, t_order_->order_id);
    }
  }
  SetSMVBestVars(t_security_id_);

  // For cases of pre-emptive trade (ie. no hidden orders ) we choose not to propagate trades when book
  // is crossed.
  if (!t_book_crossed_ && smv_.using_predictive_uncross_) {
    // Trades not reflected in book will get passed as Buy trades.
    // TODO - heuristic can be improved a bit.
    NotifyListenersOnTrade(t_security_id_, t_trade_int_price_, t_trade_size_, trd_type_, false,
                           ((event_type_ == k_l1price) ? 1 : 0));
    NotifyListenersOnLevelChange(t_security_id_, k_l1price);
  }

  if (!smv_.using_predictive_uncross_) {
    if (!is_order_seen_) { 
      NotifyListenersOnTrade(t_security_id_, t_trade_int_price_, t_trade_size_, trd_type_, false,
                             ((event_type_ == k_l1price) ? 1 : 0));
    } else {
      // book should be crossed and trade type is dictated by direction of initial cross
      if (px_level_manager_.IsCrossingSideBid(t_security_id_)) {
        trd_type_ = HFSAT::kTradeTypeBuy;
      } else {
        trd_type_ = HFSAT::kTradeTypeSell;
      }
      NotifyListenersOnTrade(t_security_id_, t_trade_int_price_, t_trade_size_, trd_type_, false,
                             ((event_type_ == k_l1price) ? 1 : 0));
    }
  }
  smv_.is_book_crossed_ = px_level_manager_.IsBookCrossed(t_security_id_);
  //std::cout << smv_.ShowMarket() << std::endl;
  //CheckL1Consistency(t_security_id_);
}

void IndexedBSEMarketViewManager2::OnTradeExecRange(const unsigned int t_security_id_, double const t_low_exec_band_,
                                                    double const t_high_exec_band_) {
  SecurityMarketView &smv_ = *(security_market_view_map_[t_security_id_]);
  //std::cout << "LP: " << t_low_exec_band_ << " HP: " << t_high_exec_band_ << std::endl;
  smv_.upper_int_trade_range_limit_ = smv_.GetIntPx(t_high_exec_band_);
  smv_.lower_int_trade_range_limit_ = smv_.GetIntPx(t_low_exec_band_);
}

double IndexedBSEMarketViewManager2::GetSanitizePx(int t_security_id_, TradeType_t sanitize_side_,
                                                   MarketUpdateInfoLevelStruct *best_crossed_bid_level_,
                                                   MarketUpdateInfoLevelStruct *best_crossed_ask_level_) {
  //SecurityMarketView &smv_ = *market_view_ptr_;
  double t_best_syn_bpx_;
  double t_best_syn_apx_;

  MarketUpdateInfoLevelStruct *t_bid_level_ = NULL;
  MarketUpdateInfoLevelStruct *t_ask_level_ = NULL;
  int t_bid_size_to_delete_ = 0;
  int t_ask_size_to_delete_ = 0;
  px_level_manager_.SetUncrossSizesandPrices(t_security_id_, t_bid_level_, t_ask_level_, t_bid_size_to_delete_,
                                             t_ask_size_to_delete_);
  if (t_bid_level_ != NULL) {
    t_best_syn_bpx_ = t_bid_level_->limit_price_;
  } else {
    t_best_syn_bpx_ = kInvalidPrice;
  }
  if (t_ask_level_ != NULL) {
    t_best_syn_apx_ = t_ask_level_->limit_price_;
  } else {
    t_best_syn_apx_ = kInvalidPrice;
  }

  if (sanitize_side_ == HFSAT::kTradeTypeBuy) {
    return std::max(t_best_syn_bpx_, best_crossed_ask_level_->limit_price_);
  } else {
    // deal with separate case of one side being empty after sanitize
    if (t_best_syn_apx_ > kInvalidPrice + BSE_V2_EPSILON && t_best_syn_apx_ < best_crossed_bid_level_->limit_price_) {
      return t_best_syn_apx_;
    } else {
      return best_crossed_bid_level_->limit_price_;
    }
  }
}

void IndexedBSEMarketViewManager2::SanitizeBookOnCrossedBook(const uint32_t t_security_id_) {
  if (px_level_manager_.IsBookCrossed(t_security_id_)){
    MarketUpdateInfoLevelStruct *t_best_bid_level_ = px_level_manager_.GetBidPL(t_security_id_, 0);
    MarketUpdateInfoLevelStruct *t_best_ask_level_ = px_level_manager_.GetAskPL(t_security_id_, 0);
    TradeType_t side_to_sanitize_ =
        (t_best_bid_level_->mod_time_ < t_best_ask_level_->mod_time_ ? kTradeTypeBuy : kTradeTypeSell);
    // sanitize price can be adjusted for desired tradeoff
    double t_px_to_sanitize_ = GetSanitizePx(t_security_id_, side_to_sanitize_, t_best_bid_level_, t_best_ask_level_);
//    std::cout << " SanitizeBookOnCrossedBook triggered at " << watch_.tv() << '\n';
    SanitizeBook(t_security_id_, side_to_sanitize_, t_px_to_sanitize_);
    // check and uncross book after Sanitize - by deleting orders of opposite side is needed
    t_best_bid_level_ = px_level_manager_.GetBidPL(t_security_id_, 0);
    t_best_ask_level_ = px_level_manager_.GetAskPL(t_security_id_, 0);
    if (t_best_bid_level_ && t_best_ask_level_) {
      if (side_to_sanitize_ == HFSAT::kTradeTypeBuy &&
          t_best_bid_level_->limit_price_ > t_best_ask_level_->limit_price_ - BSE_V2_EPSILON) {
/*        std::cout << " Second Sanitize called on Sell; Buy Sanitize Px " << t_px_to_sanitize_ << " Sell L1 "
                   << t_best_ask_level_->limit_price_ << '\n';
*/        SanitizeBook(t_security_id_, HFSAT::kTradeTypeSell, t_px_to_sanitize_ + BSE_V2_EPSILON);
      } else if (side_to_sanitize_ == HFSAT::kTradeTypeSell &&
                 t_best_ask_level_->limit_price_ < t_best_bid_level_->limit_price_ + BSE_V2_EPSILON) {
/*        std::cout << " Second Sanitize called on Buy; Sell Sanitize Px " << t_px_to_sanitize_ << " Buy L1 "
                   << t_best_bid_level_->limit_price_ << '\n';
*/        SanitizeBook(t_security_id_, HFSAT::kTradeTypeBuy, t_px_to_sanitize_ - BSE_V2_EPSILON);
      }
    }
  }
}

// Sanitize on Sub-best Trade is ignored only in one specific subcase of using predictive logic and sub-best trade
// condition is
// holding for aggressor side eg orig book 1000 @ 141.05 --- 141.2 @ 2000
//                                          500 @ 141.0  --- 141.3 @ 1000
// Agg buy 2500@141.25 will cause incorrect sanitization of B 500@141.25 on trade
void IndexedBSEMarketViewManager2::SanitizeBookOnCrossedTrade(const uint32_t t_security_id_, double t_tradepx_) {
  // Called when underlying book is uncrossed. level corresponding to t_tradepx_ is not sanitized
  SecurityMarketView &smv_ = *market_view_ptr_;
  MarketUpdateInfoLevelStruct *t_best_bid_level_ = px_level_manager_.GetSynBidPL(t_security_id_, 0);
  MarketUpdateInfoLevelStruct *t_best_ask_level_ = px_level_manager_.GetSynAskPL(t_security_id_, 0);
  bool t_book_crossed_ = px_level_manager_.IsBookCrossed(t_security_id_);
  // sub best ask trade
  if ((!t_book_crossed_ || !(smv_.using_predictive_uncross_) || px_level_manager_.IsCrossingSideBid(t_security_id_)) &&
      t_best_ask_level_ != NULL && t_tradepx_ > t_best_ask_level_->limit_price_ + BSE_V2_EPSILON &&
      t_best_ask_level_->limit_int_price_ != kInvalidIntPrice) {
    if (!dbglogger_.CheckLoggingLevel(SKIP_PACKET_DROPS_INFO)) {
      dbglogger_ << " SanitizeBookOnCrossedTrade triggered at " << watch_.tv() << '\n';
    }
    SanitizeBook(t_security_id_, HFSAT::kTradeTypeSell, t_tradepx_ - 2 * BSE_V2_EPSILON);
  } else if ((!t_book_crossed_ || !(smv_.using_predictive_uncross_) ||
              !(px_level_manager_.IsCrossingSideBid(t_security_id_))) &&
             t_best_bid_level_ != NULL && t_tradepx_ < t_best_bid_level_->limit_price_ - BSE_V2_EPSILON) {
    if (!dbglogger_.CheckLoggingLevel(SKIP_PACKET_DROPS_INFO)) {
      dbglogger_ << " SanitizeBookOnCrossedTrade triggered at " << watch_.tv() << '\n';
    }
    SanitizeBook(t_security_id_, HFSAT::kTradeTypeBuy, t_tradepx_ + 2 * BSE_V2_EPSILON);
  }
}

// Deletes the old side if book has remained crossed for long enough. False positives will increase as
// BSE_V2_SANITIZE_TIME_THRESHOLD
// is decreased. Most false positives correspond to cases where there is a long delay between an Aggressive limit order
// add and subsequent
// trade. Ex SBIN_FUT_0 1496720702.542972
void IndexedBSEMarketViewManager2::SanitizeBook(const uint32_t t_security_id_, HFSAT::TradeType_t t_sanitize_side_,
                                                double t_sanitize_price_, bool delete_order_) {
  SecurityMarketView &smv_ = *market_view_ptr_;
  MarketUpdateInfoLevelStruct *t_best_bid_level_ = px_level_manager_.GetBidPL(t_security_id_, 0);
  MarketUpdateInfoLevelStruct *t_best_ask_level_ = px_level_manager_.GetAskPL(t_security_id_, 0);

  if (!dbglogger_.CheckLoggingLevel(SKIP_SANITIZE_INFO)) {
  DBGLOG_TIME_CLASS_FUNC_LINE << " Book state before Sanitize: "
                              << (t_best_bid_level_ != NULL ? t_best_bid_level_->limit_size_ : kInvalidSize) << " @ "
                              << (t_best_bid_level_ != NULL ? t_best_bid_level_->limit_price_ : kInvalidPrice)
                              << " --- "
                              << (t_best_ask_level_ != NULL ? t_best_ask_level_->limit_price_ : kInvalidPrice) << " @ "
                              << (t_best_ask_level_ != NULL ? t_best_ask_level_->limit_size_ : kInvalidSize) << ' '
                              << smv_.shortcode() << DBGLOG_ENDL_FLUSH;
  }

  int order_count_ = 0;
  int level_ = 0;

  if (t_sanitize_side_ == kTradeTypeBuy) {
    MarketUpdateInfoLevelStruct *t_bid_level_ = px_level_manager_.GetBidPL(t_security_id_, level_);
    while (t_bid_level_ != NULL && t_bid_level_->limit_price_ > t_sanitize_price_ - BSE_V2_EPSILON) {
      order_count_ += t_bid_level_->limit_ordercount_;
      level_++;
      t_bid_level_ = px_level_manager_.GetBidPL(t_security_id_, level_);
    }
  } else {
    MarketUpdateInfoLevelStruct *t_ask_level_ = px_level_manager_.GetAskPL(t_security_id_, level_);
    while (t_ask_level_ != NULL && t_ask_level_->limit_price_ < t_sanitize_price_ + BSE_V2_EPSILON) {
      order_count_ += t_ask_level_->limit_ordercount_;
      level_++;
      t_ask_level_ = px_level_manager_.GetAskPL(t_security_id_, level_);
    }
  }

  // Delete offending orders in maps and px_level_manager
  std::vector<OrderDetailsStruct *> &order_cache_ = order_history_instance.GetOrderCache(t_security_id_);
  ska::flat_hash_map<uint64_t, OrderDetailsStruct *> &order_history_ =
      order_history_instance.GetOrderMaps(t_security_id_);

  auto t_vectiter = order_cache_.begin();
  for (; t_vectiter != order_cache_.end();) {
    if (order_count_ <= 0) break;
    OrderDetailsStruct *t_live_order_ = *t_vectiter;
    if (t_live_order_ == NULL) {
      t_vectiter++;
      continue;
    }
    // live order contradicting sanitize price is deleted
    if ((t_live_order_->is_buy_order && t_sanitize_side_ == kTradeTypeBuy &&
         (t_live_order_->order_price > t_sanitize_price_ - BSE_V2_EPSILON)) ||
        (!t_live_order_->is_buy_order && t_sanitize_side_ == kTradeTypeSell &&
         (t_live_order_->order_price < t_sanitize_price_ + BSE_V2_EPSILON))) {
      order_count_--;
      t_vectiter++;
      px_level_manager_.ModifySizeAtPxLevel(
          t_security_id_, (t_live_order_->is_buy_order ? HFSAT::kTradeTypeBuy : HFSAT::kTradeTypeSell),
          smv_.GetIntPx(t_live_order_->order_price), -t_live_order_->order_size, true);
      //        dbglogger_ << "Sanitizing Orderid " << t_live_order_->order_id << ' ' << t_live_order_->order_size
      //		   << " @ " << t_live_order_->order_price << '\n';
      if(delete_order_){
        order_history_instance.DeleteOrderFromHistory(t_security_id_, t_live_order_->order_id);
      }
    } else {
      t_vectiter++;
    }
  }

  auto t_mapiter_ = order_history_.begin();

  for (; t_mapiter_ != order_history_.end();) {
    if (order_count_ <= 0) break;
    OrderDetailsStruct *t_live_order_ = (*t_mapiter_).second;
    // live order contradicting bestprice is deleted
    if ((t_live_order_->is_buy_order && t_sanitize_side_ == kTradeTypeBuy &&
         (t_live_order_->order_price > t_sanitize_price_ - BSE_V2_EPSILON)) ||
        (!t_live_order_->is_buy_order && t_sanitize_side_ == kTradeTypeSell &&
         (t_live_order_->order_price < t_sanitize_price_ + BSE_V2_EPSILON))) {
      order_count_--;
      t_mapiter_++;
      px_level_manager_.ModifySizeAtPxLevel(
          t_security_id_, (t_live_order_->is_buy_order ? HFSAT::kTradeTypeBuy : HFSAT::kTradeTypeSell),
          smv_.GetIntPx(t_live_order_->order_price), -t_live_order_->order_size, true);
      //        dbglogger_ << "Sanitizing Orderid " << t_live_order_->order_id << ' ' << t_live_order_->order_size
      //	 	   << " @ " << t_live_order_->order_price << '\n';
      if(delete_order_){
        ska::flat_hash_map<uint64_t, OrderDetailsStruct *>::iterator iter =
            order_history_instance.DeleteOrderFromHistory(t_security_id_, t_live_order_->order_id);
        if (iter != order_history_.end()) t_mapiter_ = iter;
      }
    } else {
      t_mapiter_++;
    }
  }

  t_best_bid_level_ = px_level_manager_.GetBidPL(t_security_id_, 0);
  t_best_ask_level_ = px_level_manager_.GetAskPL(t_security_id_, 0);
  SetSMVBestVars(t_security_id_);

  if (!dbglogger_.CheckLoggingLevel(SKIP_SANITIZE_INFO)) {
  DBGLOG_TIME_CLASS_FUNC_LINE << "Book state after Sanitize: "
                              << (t_best_bid_level_ != NULL ? t_best_bid_level_->limit_size_ : kInvalidSize) << " @ "
                              << (t_best_bid_level_ != NULL ? t_best_bid_level_->limit_price_ : kInvalidPrice)
                              << " --- "
                              << (t_best_ask_level_ != NULL ? t_best_ask_level_->limit_price_ : kInvalidPrice) << " @ "
                              << (t_best_ask_level_ != NULL ? t_best_ask_level_->limit_size_ : kInvalidSize)
                              << DBGLOG_ENDL_FLUSH;
  }
  //  CheckL1Consistency(t_security_id_);
}

// Checks and returns if current time is trading time for the product
bool IndexedBSEMarketViewManager2::CheckValidTime(int sec_id) {
  return trade_time_manager_.isValidTimeToTrade(sec_id, watch_.tv().tv_sec % 86400);
}

// add entry to self_order_history_instance and delete order from MD feed if already reflected
void IndexedBSEMarketViewManager2::OrderConfirmed(
    const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
    const int _server_assigned_order_sequence_, const unsigned int _security_id_, const double _price_,
    const TradeType_t r_buysell_, const int _size_remaining_, const int _size_executed_, const int _client_position_,
    const int _global_position_, const int r_int_price_, const int32_t server_assigned_message_sequence,
    const uint64_t exchange_order_id, const ttime_t time_set_by_server) {
  if (use_self_book_) {
    // we don't want to filter orders from other strategies if self_trader_ids is populated
    if (!self_trader_ids_.empty() && self_trader_ids_.find(t_server_assigned_client_id_) == self_trader_ids_.end()) {
      return;
    }
    self_order_history_instance.AddOrderToOrderHistory(_security_id_, r_buysell_, _price_, _size_remaining_,
                                                       exchange_order_id);
    // if order is reflected in the mkt book delete it from the book
    if (order_history_instance.IsOrderSeen(_security_id_, exchange_order_id)) {
      OnOrderDelete(_security_id_, r_buysell_, exchange_order_id, 0, true, true);
    }
  }
}

void IndexedBSEMarketViewManager2::OnExecutionSummary(const uint32_t t_security_id_, TradeType_t t_aggressor_side_, const double t_trade_price_,
                          const uint32_t t_trade_size_){

  return;

//  std::cout << watch_.tv() << " OnExecutionSummary@" << t_trade_price_ << " of size " << t_trade_size_ << " Agg_Side: "
//              << ((t_aggressor_side_ == HFSAT::TradeType_t::kTradeTypeSell) ? "SELL":"BUY")<< std::endl;

  SecurityMarketView &smv_ = *(security_market_view_map_[t_security_id_]);
  market_view_ptr_ = &smv_;

  if(smv_.GetIntPx(t_trade_price_) > smv_.upper_int_price_limit_ || smv_.GetIntPx(t_trade_price_) < smv_.lower_int_price_limit_){
//    std::cout << "IndexedBSEMarketViewManager2::OnTrade: Prices are OOB, px :: " << t_trade_price_ << std::endl;

    return;
  }

  // smv_.ShowMarket();
  MarketUpdateInfoLevelStruct *t_best_bid_level_ = px_level_manager_.GetSynBidPL(t_security_id_, 0);
  MarketUpdateInfoLevelStruct *t_best_ask_level_ = px_level_manager_.GetSynAskPL(t_security_id_, 0);
  
  if(t_aggressor_side_ == HFSAT::kTradeTypeBuy && t_best_ask_level_ != NULL && 
     t_trade_price_ > t_best_ask_level_->limit_price_ + BSE_V2_EPSILON && t_best_ask_level_->limit_int_price_ != kInvalidIntPrice){

//    std::cout << " SanitizeBookOnExecutionSummaryTrade triggered at " << watch_.tv() << '\n';
    SanitizeBook(t_security_id_, HFSAT::kTradeTypeSell, t_trade_price_ - 2 * BSE_V2_EPSILON);

  }else if(t_aggressor_side_ == HFSAT::kTradeTypeSell && t_best_bid_level_ != NULL && 
    t_trade_price_ < t_best_bid_level_->limit_price_ - BSE_V2_EPSILON && t_best_bid_level_->limit_int_price_ != kInvalidIntPrice) {

//    std::cout << " SanitizeBookOnExecutionSummaryTrade triggered at " << watch_.tv() << '\n';
    SanitizeBook(t_security_id_, HFSAT::kTradeTypeBuy, t_trade_price_ + 2 * BSE_V2_EPSILON);

  }


//  std::cout << "After OnExecutionSummary: \n";
  // smv_.ShowMarket();
  NotifyListenersOnLevelChange(t_security_id_, k_l1price);
  smv_.is_book_crossed_ = px_level_manager_.IsBookCrossed(t_security_id_);
}

// delete entry from self_order_history_instance
void IndexedBSEMarketViewManager2::OrderCanceled(const int t_server_assigned_client_id_,
                                                 const int _client_assigned_order_sequence_,
                                                 const int _server_assigned_order_sequence_,
                                                 const unsigned int _security_id_, const double _price_,
                                                 const TradeType_t r_buysell_, const int _size_remaining_,
                                                 const int _client_position_, const int _global_position_,
                                                 const int r_int_price_, const int32_t server_assigned_message_sequence,
                                                 const uint64_t exchange_order_id, const ttime_t time_set_by_server) {
  if (use_self_book_) {
    if (self_order_history_instance.IsOrderSeen(_security_id_, exchange_order_id)) {
      self_order_history_instance.DeleteOrderFromHistory(_security_id_, exchange_order_id);
    }
  }
}

void IndexedBSEMarketViewManager2::OrderExecuted(
    const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
    const int _server_assigned_order_sequence_, const unsigned int _security_id_, const double _price_,
    const TradeType_t r_buysell_, const int _size_remaining_, const int _size_executed_, const int _client_position_,
    const int _global_position_, const int r_int_price_, const int32_t server_assigned_message_sequence,
    const uint64_t exchange_order_id, const ttime_t time_set_by_server) {
  if (use_self_book_) {
    if (self_order_history_instance.IsOrderSeen(_security_id_, exchange_order_id) && _size_remaining_ == 0) {
      self_order_history_instance.DeleteOrderFromHistory(_security_id_, exchange_order_id);
    }
  }
}
void IndexedBSEMarketViewManager2::OnIndexPrice(const unsigned int t_security_id_,const double t_spot_price_){
  //std::cout << "IndexedBSEMarketViewManager2::OnIndexPrice = " << t_spot_price_ << std::endl;
  SecurityMarketView &smv_ = *(security_market_view_map_[t_security_id_]);
  // std::cout << "trd , spt :: " << smv_.trade_print_info_.trade_price_ << " , " << t_spot_price_ << std::endl;
  // std::cout <<"SC: " << smv_.secname() <<" ID: " << smv_.security_id() << " EXCH: " << smv_.exch_source() << std::endl;
    // std::cout << "IF LOOP " << std::endl;
    smv_.market_update_info_.mid_price_ = t_spot_price_;
    smv_.market_update_info_.bestask_price_ = t_spot_price_;
    smv_.market_update_info_.bestbid_price_ = t_spot_price_;
    smv_.trade_print_info_.trade_price_ = t_spot_price_;
    smv_.trade_print_info_.size_traded_ = 1;
    smv_.is_ready_ = true;
    smv_.NotifySpotIndexListeners();
}

void IndexedBSEMarketViewManager2::OnOpenInterestUpdate(const unsigned int t_security_id_, double const t_oi_price_){
  //std::cout << "IndexedNSEMarketViewManager2::OnOpenInterestUpdate = " <<  t_security_id_ << " " << t_oi_price_ << std::endl;
  SecurityMarketView &smv_ = *(security_market_view_map_[t_security_id_]);
  smv_.NotifyOpenInterestListeners(t_oi_price_);
}

}
