/**
   \file MarketAdapterCode/eurex_price_level_market_view_manager.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   Phone: +91 80 4190 3551
*/

#include "dvccode/CDef/security_definitions.hpp"
#include "baseinfra/MarketAdapter/l1_price_market_view_manager.hpp"

namespace HFSAT {

L1PriceMarketViewManager::L1PriceMarketViewManager(DebugLogger& t_dbglogger, const Watch& t_watch,
                                                   const SecurityNameIndexer& t_sec_name_indexer,
                                                   const std::vector<SecurityMarketView*>& t_security_market_view_map)
    : BaseMarketViewManager(t_dbglogger, t_watch, t_sec_name_indexer, t_security_market_view_map) {}

void L1PriceMarketViewManager::OnL1Change(const unsigned int t_security_id, const TradeType_t t_buysell,
                                          const double price, const int new_size, const int new_ordercount,
                                          const bool is_intermediate_message) {
  SecurityMarketView& this_smv = *(security_market_view_map_[t_security_id]);

  switch (t_buysell) {
    case kTradeTypeBuy: {
      MarketUpdateInfoLevelStruct new_level_(0, this_smv.GetIntPx(price), price, new_size, new_ordercount, watch_.tv());

      this_smv.ReplaceTopBid(new_level_);
      this_smv.SetBestLevelBidVariablesOnQuote();

      if (!is_intermediate_message) {
        // Uncross

        this_smv.UpdateL1Prices();
        // Notify PL Indicatore listneres
        if (this_smv.pl_change_listeners_present_) {
          this_smv.NotifyOnPLChangeListeners(t_security_id, this_smv.market_update_info_, kTradeTypeBuy, 0,
                                             new_level_.limit_int_price_, new_level_.limit_int_price_level_, 0,
                                             new_size, 0, new_ordercount, is_intermediate_message, 'N');
          // dbglogger_ << "NOTIF..L1SIZEUPDATE ONCHANGE NEW"<<'\n' ; dbglogger_ .DumpCurrentBuffer ();
        }
        if (!this_smv.market_update_info_.bidlevels_.empty() && !this_smv.market_update_info_.asklevels_.empty()) {
          this_smv.NotifyL1SizeListeners();
        }
        // this_smv.OnL1SizeUpdate ( );
        this_smv.l1_changed_since_last_ = false;
        this_smv.NotifyOnReadyListeners();
      } else {
        // Notify PL Indicatore listneres
        if (this_smv.pl_change_listeners_present_) {
          this_smv.NotifyOnPLChangeListeners(t_security_id, this_smv.market_update_info_, kTradeTypeBuy, 0,
                                             new_level_.limit_int_price_, new_level_.limit_int_price_level_, 0,
                                             new_size, 0, new_ordercount, is_intermediate_message, 'N');
        }
        this_smv.l1_changed_since_last_ = true;
      }

    } break;
    case kTradeTypeSell: {
      // since most new levels will be at top .. so special case for this ...
      // using O(1) PushFront not O(DEF_MARKET_DEPTH) PushAt
      // also needed to set bestlevel variables

      MarketUpdateInfoLevelStruct new_level_(0, this_smv.GetIntPx(price), price, new_size, new_ordercount, watch_.tv());
      this_smv.ReplaceTopAsk(new_level_);
      this_smv.SetBestLevelAskVariablesOnQuote();

      if (!is_intermediate_message) {
        // Uncross

        this_smv.UpdateL1Prices();

        if (this_smv.pl_change_listeners_present_) {
          this_smv.NotifyOnPLChangeListeners(t_security_id, this_smv.market_update_info_, kTradeTypeSell, 0,
                                             new_level_.limit_int_price_, new_level_.limit_int_price_level_, 0,
                                             new_size, 0, new_ordercount, is_intermediate_message, 'N');
          // dbglogger_ << "NOTIF..L1SIZEUPDATE NEW SELL "<<'\n' ; dbglogger_ .DumpCurrentBuffer ();
        }

        if (!this_smv.market_update_info_.bidlevels_.empty() && !this_smv.market_update_info_.asklevels_.empty()) {
          this_smv.NotifyL1SizeListeners();
        }
        // this_smv.OnL1SizeUpdate( );
        this_smv.l1_changed_since_last_ = false;
        this_smv.NotifyOnReadyListeners();
      } else  // Non-intermediate msg
      {
        if (this_smv.pl_change_listeners_present_) {
          this_smv.NotifyOnPLChangeListeners(t_security_id, this_smv.market_update_info_, kTradeTypeSell, 0,
                                             new_level_.limit_int_price_, new_level_.limit_int_price_level_, 0,
                                             new_size, 0, new_ordercount, is_intermediate_message, 'N');
        }
        this_smv.l1_changed_since_last_ = true;
      }
    } break;
    default: { break; }
  }
}

void L1PriceMarketViewManager::DeleteTop(const unsigned int t_security_id, const TradeType_t t_buysell,
                                         const double price, const bool is_intermediate_message) {
  SecurityMarketView& this_smv = *(security_market_view_map_[t_security_id]);

  switch (t_buysell) {
    case kTradeTypeBuy: {
      // top level changed .. .to optimize using PopFront and not O(DEF_MARKET_DEPTH) PopAt

      if (this_smv.use_order_level_book_ == true) {
        if (this_smv.last_message_was_trade_ == true) {
          // If on a trade Entire Level 1 is cleared then we need to unset the "last_message_was_trade_" flag from here
          if (this_smv.last_traded_tradetype_ == kTradeTypeSell ||
              this_smv.last_traded_tradetype_ == kTradeTypeNoInfo) {
            this_smv.last_message_was_trade_ = false;
          }
        }
      }

      int old_size_at_this_price = 0;
      int old_bid_int_price = 0;
      int old_bid_inpricelevel = 0;
      int old_ordercount_at_this_price = 0;

      if (this_smv.pl_change_listeners_present_) {
        register unsigned int t_level = 0;
        if (t_level < this_smv.market_update_info_.bidlevels_.size()) {
          old_size_at_this_price = this_smv.market_update_info_.bidlevels_[t_level].limit_size_;
          old_bid_int_price = this_smv.market_update_info_.bidlevels_[t_level].limit_int_price_;
          old_bid_inpricelevel = std::max(0, this_smv.market_update_info_.bidlevels_[t_level].limit_int_price_level_);
          old_ordercount_at_this_price = this_smv.market_update_info_.bidlevels_[t_level].limit_ordercount_;
        }
      }

      this_smv.RemoveTopBid();

      this_smv.SetBestLevelBidVariablesOnQuote();
      if (!is_intermediate_message) {
        this_smv.UpdateL1Prices();
        // On PL_Delete Notification
        if (this_smv.pl_change_listeners_present_) {
          this_smv.NotifyOnPLChangeListeners(t_security_id, this_smv.market_update_info_, kTradeTypeBuy, 0,
                                             old_bid_int_price, old_bid_inpricelevel, old_size_at_this_price, 0,
                                             old_ordercount_at_this_price, 0, is_intermediate_message, 'D');
        }

        // this_smv.OnL1SizeUpdate( );
        this_smv.l1_changed_since_last_ = false;

        this_smv.NotifyOnReadyListeners();
      } else  // NOn-intermediate messages
      {
        if (this_smv.pl_change_listeners_present_) {
          this_smv.NotifyOnPLChangeListeners(t_security_id, this_smv.market_update_info_, kTradeTypeBuy, 0,
                                             old_bid_int_price, old_bid_inpricelevel, old_size_at_this_price, 0,
                                             old_ordercount_at_this_price, 0, is_intermediate_message, 'D');
        }
        // L1 Price did not get updated
        this_smv.l1_changed_since_last_ = true;
      }
    } break;
    case kTradeTypeSell: {
      // top level changed
      if (this_smv.use_order_level_book_ == true) {
        if (this_smv.last_message_was_trade_ == true) {
          // If on a trade Entire Level 1 is cleared then we need to unset the "last_message_was_trade_" flag from here
          if (this_smv.last_traded_tradetype_ == kTradeTypeBuy || this_smv.last_traded_tradetype_ == kTradeTypeNoInfo) {
            this_smv.last_message_was_trade_ = false;
          }
        }
      }

      if (this_smv.computing_price_levels() && this_smv.market_update_info_.asklevels_.size() > 1) {
        // update limit_int_price_level_ field everywhere

        int levels_from_top_to_deduct =
            this_smv.market_update_info_.asklevels_[1].limit_int_price_ -
            this_smv.market_update_info_.asklevels_[0].limit_int_price_;  // ask so lvl(1) is more than lvl(0)
        for (unsigned int i = 1; i < this_smv.market_update_info_.asklevels_.size(); i++) {
          this_smv.market_update_info_.asklevels_[i].limit_int_price_level_ -= levels_from_top_to_deduct;
        }
      }
      int old_size_at_this_price = 0;
      int old_ask_int_price = 0;
      int old_ask_inpricelevel = 0;
      int old_ordercount_at_this_price = 0;

      if (this_smv.pl_change_listeners_present_) {
        register unsigned int t_level = 0;
        if (t_level < this_smv.market_update_info_.asklevels_.size()) {
          old_size_at_this_price = this_smv.market_update_info_.asklevels_[t_level].limit_size_;
          old_ask_int_price = this_smv.market_update_info_.asklevels_[t_level].limit_int_price_;
          old_ask_inpricelevel = std::max(0, this_smv.market_update_info_.asklevels_[t_level].limit_int_price_level_);
          old_ordercount_at_this_price = this_smv.market_update_info_.asklevels_[t_level].limit_ordercount_;
        }
      }

      this_smv.RemoveTopAsk();
      this_smv.SetBestLevelAskVariablesOnQuote();

      if (!is_intermediate_message) {
        this_smv.UpdateL1Prices();
        if (this_smv.pl_change_listeners_present_) {
          // For PL indicators we send the update irrespective of intermediate_message
          // On PL_Delete Notification
          // dbglogger_ << "Func: " << __func__ << " OnDelFIXFAST: 6 SELL : "<< t_levelremoved_ - 1 <<'\n';
          // dbglogger_ << this_smv.ShowMarket ( ) ;
          // dbglogger_ .DumpCurrentBuffer ( ) ;
          this_smv.NotifyOnPLChangeListeners(t_security_id, this_smv.market_update_info_, kTradeTypeSell, 0,
                                             old_ask_int_price, old_ask_inpricelevel, old_size_at_this_price, 0,
                                             old_ordercount_at_this_price, 0, is_intermediate_message, 'D');
        }

        // this_smv.OnL1SizeUpdate( );
        this_smv.l1_changed_since_last_ = false;
        this_smv.NotifyOnReadyListeners();
      } else  // Non-intermediate_message
      {
        if (this_smv.pl_change_listeners_present_) {
          // For PL indicators we send the update irrespective of intermediate_message
          // On PL_Delete Notification
          // dbglogger_ << "Func: " << __func__ << " OnDelFIXFAST: 7 SELL : "<< t_levelremoved_ - 1 <<'\n';
          // dbglogger_ << this_smv.ShowMarket ( ) ;
          // dbglogger_ .DumpCurrentBuffer ( ) ;

          this_smv.NotifyOnPLChangeListeners(t_security_id, this_smv.market_update_info_, kTradeTypeSell, 0,
                                             old_ask_int_price, old_ask_inpricelevel, old_size_at_this_price, 0,
                                             old_ordercount_at_this_price, 0, is_intermediate_message, 'D');
        }
        this_smv.l1_changed_since_last_ = true;
      }
    } break;
    default:
      break;
  }

  // std::cerr << " Packet delete : " << t_security_id << " " << t_levelremoved_ << " " << price << " " <<
  // this_smv.ShowMarket() ;//debuginfo
}

void L1PriceMarketViewManager::OnTrade(const unsigned int t_security_id, const double t_trade_price,
                                       const int t_trade_size, const TradeType_t t_buysell) {
  security_market_view_map_[t_security_id]->OnTrade(t_trade_price, t_trade_size, t_buysell);
}
}
