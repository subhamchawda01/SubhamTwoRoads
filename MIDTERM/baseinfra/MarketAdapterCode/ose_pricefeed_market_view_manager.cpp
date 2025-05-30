// =====================================================================================
//
//       Filename:  MarketAdapterCode/ose_pricefeed_market_view_manager.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  04/29/2013 11:50:08 AM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
//
//        Address:  Suite No 353, Evoma, #14, Bhattarhalli,
//                  Old Madras Road, Near Garden City College,
//                  KR Puram, Bangalore 560049, India
//          Phone:  +91 80 4190 3551
//
// =====================================================================================

#include "dvccode/CDef/security_definitions.hpp"
#include "baseinfra/MarketAdapter/ose_pricefeed_market_view_manager.hpp"

namespace HFSAT {

OSEPriceFeedMarketViewManager::OSEPriceFeedMarketViewManager(
    DebugLogger& t_dbglogger_, const Watch& t_watch_, const SecurityNameIndexer& t_sec_name_indexer_,
    const std::vector<SecurityMarketView*>& t_security_market_view_map_)
    : BaseMarketViewManager(t_dbglogger_, t_watch_, t_sec_name_indexer_, t_security_market_view_map_),
      market_orders_mempool_(100),
      sid_to_bidside_trade_size_(t_sec_name_indexer_.NumSecurityId()),
      sid_to_bidside_trade_price_(t_sec_name_indexer_.NumSecurityId()),
      sid_to_highest_accumulated_bidside_trade_size_(t_sec_name_indexer_.NumSecurityId()),
      sid_to_last_bidside_trade_time_(t_sec_name_indexer_.NumSecurityId()),
      sid_to_askside_trade_size_(t_sec_name_indexer_.NumSecurityId()),
      sid_to_askside_trade_price_(t_sec_name_indexer_.NumSecurityId()),
      sid_to_highest_accumulated_askside_trade_size_(t_sec_name_indexer_.NumSecurityId()),
      sid_to_last_askside_trade_time_(t_sec_name_indexer_.NumSecurityId())

{
  for (auto i = 0u; i < security_market_view_map_.size(); i++) {
    SecurityMarketView* p_this_smv_ = t_security_market_view_map_[i];
    if (p_this_smv_) {
      p_this_smv_->SetMarketOrdersMempool(&market_orders_mempool_);
    }
  }
}

void OSEPriceFeedMarketViewManager::OnPriceLevelNew(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                                    const int t_level_added_, const double t_price_,
                                                    const int t_new_size_, const int t_new_ordercount_,
                                                    const bool t_is_intermediate_message_) {
  SecurityMarketView& this_smv_ = *(security_market_view_map_[t_security_id_]);

  this_smv_.last_message_was_trade_ = true;
  if (t_level_added_ > DEF_MARKET_DEPTH) {
    // update was done to L1, don't want to hold it back as soon as non-intermediate msg is available irrespective of
    // level
    if (this_smv_.l1_changed_since_last_ && !t_is_intermediate_message_) {
      this_smv_.UpdateL1Prices();

      if (!this_smv_.market_update_info_.bidlevels_.empty() && !this_smv_.market_update_info_.asklevels_.empty()) {
        this_smv_.NotifyL1SizeListeners();
      }

      this_smv_.l1_changed_since_last_ = false;

      this_smv_.NotifyOnReadyListeners();
    }

    return;
  }

  int t_int_price_ = this_smv_.GetIntPx(t_price_);

  // // the variables highest_level_changed_ and buysell_highest_level_changed_ are currently commented out,
  // // but if computed then they might save a lot of computation for indicators or others using the book.
  // this_smv_.highest_level_changed_ = (_level_added_ -1);
  // this_smv_.buysell_highest_level_changed_ = t_buysell_;

  switch (t_buysell_) {
    case kTradeTypeBuy: {
      if (t_level_added_ == 1) {  // since most new levels will be at top .. so special case for this ...
        // using O(1) PushFront not O(DEF_MARKET_DEPTH) PushAt
        // also needed to set bestlevel variables

        // create structure

        MarketUpdateInfoLevelStruct new_level_(0, this_smv_.GetIntPx(t_price_), t_price_, t_new_size_,
                                               t_new_ordercount_, watch_.tv());

        this_smv_.AddTopBid(new_level_);  // TODO_OPT ... make this a FixedLengthCircularVector

        if (this_smv_.computing_price_levels() &&
            this_smv_.market_update_info_.bidlevels_.size() > 1) {  // update the levels of lower levels
          // another option is if limit_int_price_ is computed then this computation can be done at runtime
          int level_diff_top_two = this_smv_.market_update_info_.bidlevels_[0].limit_int_price_ -
                                   this_smv_.market_update_info_.bidlevels_[1]
                                       .limit_int_price_;  // this should be added to the levels number of all levels
          for (unsigned int i = 1; i < this_smv_.market_update_info_.bidlevels_.size(); i++) {  // excluding top layer
            this_smv_.market_update_info_.bidlevels_[i].limit_int_price_level_ += level_diff_top_two;
          }
        }

        this_smv_.SetBestLevelBidVariablesOnQuote();

        if (!t_is_intermediate_message_) {
          // #ifdef EUREX_KEEP_UNCROSS_CODE
          // 		  // If it isn't an intermediate message
          // 		  // Uncross
          // 		  bool crossed = this_smv_.Uncross ( );
          // 		  if ( crossed )
          // 		    {
          // 		      this_smv_.SetBestLevelBidVariablesOnQuote ( );
          // 		      this_smv_.SetBestLevelAskVariablesOnQuote ( );
          // 		    }
          // #endif

          this_smv_.UpdateL1Prices();
          // Notify PL Indicatore listneres
          if (this_smv_.pl_change_listeners_present_) {
            this_smv_.NotifyOnPLChangeListeners(t_security_id_, this_smv_.market_update_info_, kTradeTypeBuy,
                                                t_level_added_ - 1, new_level_.limit_int_price_,
                                                new_level_.limit_int_price_level_, 0, t_new_size_, 0, t_new_ordercount_,
                                                t_is_intermediate_message_, 'N');
            // dbglogger_ << "NOTIF..L1SIZEUPDATE ONCHANGE NEW"<<'\n' ; dbglogger_ .DumpCurrentBuffer ();
          }
          if (!this_smv_.market_update_info_.bidlevels_.empty() && !this_smv_.market_update_info_.asklevels_.empty()) {
            this_smv_.NotifyL1SizeListeners();
          }
          // this_smv_.OnL1SizeUpdate ( );
          this_smv_.l1_changed_since_last_ = false;
          if (this_smv_.l2_changed_since_last_) {
            // dbglogger_ << "NOTIF..L2 ONCHANGE NEW"<<'\n' ; dbglogger_ .DumpCurrentBuffer ();
            // @ 1. Clear duplicate entries if it has occured i.e sanitize
            // @ 2. Call L2 listeners and L2 only listeners
            this_smv_.Sanitize();
            this_smv_.OnL2Update();
            this_smv_.OnL2OnlyUpdate();
            this_smv_.SanitizeAskSideWithPrice(t_int_price_);
            this_smv_.l2_changed_since_last_ = false;
          }
          this_smv_.NotifyOnReadyListeners();
        } else {
          // Notify PL Indicatore listneres
          if (this_smv_.pl_change_listeners_present_) {
            this_smv_.NotifyOnPLChangeListeners(t_security_id_, this_smv_.market_update_info_, kTradeTypeBuy,
                                                t_level_added_ - 1, new_level_.limit_int_price_,
                                                new_level_.limit_int_price_level_, 0, t_new_size_, 0, t_new_ordercount_,
                                                t_is_intermediate_message_, 'N');
          }
          this_smv_.l1_changed_since_last_ = true;
        }

      } else {
        if (t_level_added_ > (int)this_smv_.market_update_info_.bidlevels_.size() + 1) return;

        MarketUpdateInfoLevelStruct new_level_(0, this_smv_.GetIntPx(t_price_), t_price_, t_new_size_,
                                               t_new_ordercount_, watch_.tv());
        if (this_smv_.computing_price_levels()) {
          new_level_.limit_int_price_level_ =
              this_smv_.market_update_info_.bidlevels_[0].limit_int_price_ - new_level_.limit_int_price_;
        }

        this_smv_.AddNonTopBid(t_level_added_, new_level_);  // Updating Price Level Book

        /* On a normal new Level we will have only one entry in the order level Book corresponding to the cumulative
         * size and Order Count at that time .
         * Some Times when top Levels get cleared then Lower levels that should come at top are being send as New Level
         * . This check ensures that we do not loose the Order level Info that we had
         * for them . If the same price Levels existed then the Order Level Book is copied and is updated as necessary
         */

        if (!t_is_intermediate_message_) {
          if (this_smv_.l1_changed_since_last_) {
            // #ifdef EUREX_KEEP_UNCROSS_CODE
            // 		      // If it isn't an intermediate message
            // 		      // Uncross
            // 		      bool crossed = this_smv_.Uncross ( );
            // 		      if ( crossed )
            // 			{
            // 			  this_smv_.SetBestLevelBidVariablesOnQuote ( );
            // 			  this_smv_.SetBestLevelAskVariablesOnQuote ( );
            // 			}
            // #endif

            this_smv_.UpdateL1Prices();
            if (this_smv_.pl_change_listeners_present_) {
              this_smv_.NotifyOnPLChangeListeners(t_security_id_, this_smv_.market_update_info_, kTradeTypeBuy,
                                                  t_level_added_ - 1, new_level_.limit_int_price_,
                                                  new_level_.limit_int_price_level_, 0, t_new_size_, 0,
                                                  t_new_ordercount_, t_is_intermediate_message_, 'N');
              // dbglogger_ << "NOTIF..L1SIZEUPDATE NEW "<<'\n' ; dbglogger_ .DumpCurrentBuffer ();
            }
            if (!this_smv_.market_update_info_.bidlevels_.empty() &&
                !this_smv_.market_update_info_.asklevels_.empty()) {
              this_smv_.NotifyL1SizeListeners();
            }
            // this_smv_.OnL1SizeUpdate( );
            this_smv_.l1_changed_since_last_ = false;

          } else {
            // Update without L1 price changes
            if (this_smv_.pl_change_listeners_present_) {
              this_smv_.NotifyOnPLChangeListeners(t_security_id_, this_smv_.market_update_info_, kTradeTypeBuy,
                                                  t_level_added_ - 1, new_level_.limit_int_price_,
                                                  new_level_.limit_int_price_level_, 0, t_new_size_, 0,
                                                  t_new_ordercount_, t_is_intermediate_message_, 'N');
              // dbglogger_ << "NOTIF..L1SIZEUPDATE NEW "<<'\n' ; dbglogger_ .DumpCurrentBuffer ();
            }
          }
          // dbglogger_ << "NOTIF..L2 NEW "<<'\n' ; dbglogger_ .DumpCurrentBuffer ();
          this_smv_.Sanitize();
          this_smv_.OnL2Update();
          this_smv_.OnL2OnlyUpdate();
          this_smv_.NotifyOnReadyListeners();
          this_smv_.SanitizeAskSideWithPrice(t_int_price_);
          this_smv_.l2_changed_since_last_ = false;
        } else {
          if (this_smv_.pl_change_listeners_present_) {
            this_smv_.NotifyOnPLChangeListeners(t_security_id_, this_smv_.market_update_info_, kTradeTypeBuy,
                                                t_level_added_ - 1, new_level_.limit_int_price_,
                                                new_level_.limit_int_price_level_, 0, t_new_size_, 0, t_new_ordercount_,
                                                t_is_intermediate_message_, 'N');
          }
          this_smv_.l2_changed_since_last_ = true;
        }

        // if ( dbglogger_.CheckLoggingLevel ( BOOK_TEST ) )
        // 	{
        // 	  DBGLOG_TIME_CLASS_FUNC << " AddNonTopBid " << t_is_intermediate_message_ << "\n" <<
        // this_smv_.ShowMarket( ) << DBGLOG_ENDL_FLUSH ;
        // 	}
      }
    } break;
    case kTradeTypeSell: {
      if (t_level_added_ == 1) {  // since most new levels will be at top .. so special case for this ...
        // using O(1) PushFront not O(DEF_MARKET_DEPTH) PushAt
        // also needed to set bestlevel variables

        MarketUpdateInfoLevelStruct new_level_(0, this_smv_.GetIntPx(t_price_), t_price_, t_new_size_,
                                               t_new_ordercount_, watch_.tv());
        this_smv_.AddTopAsk(new_level_);

        // update the levels of lower levels
        if (this_smv_.computing_price_levels() && this_smv_.market_update_info_.asklevels_.size() > 1) {
          int level_diff_top_two = this_smv_.market_update_info_.asklevels_[1].limit_int_price_ -
                                   this_smv_.market_update_info_.asklevels_[0].limit_int_price_;
          for (unsigned int i = 1; i < this_smv_.market_update_info_.asklevels_.size(); i++) {
            this_smv_.market_update_info_.asklevels_[i].limit_int_price_level_ += level_diff_top_two;
          }
        }

        this_smv_.SetBestLevelAskVariablesOnQuote();

        if (!t_is_intermediate_message_) {
          this_smv_.UpdateL1Prices();

          if (this_smv_.pl_change_listeners_present_) {
            this_smv_.NotifyOnPLChangeListeners(t_security_id_, this_smv_.market_update_info_, kTradeTypeSell,
                                                t_level_added_ - 1, new_level_.limit_int_price_,
                                                new_level_.limit_int_price_level_, 0, t_new_size_, 0, t_new_ordercount_,
                                                t_is_intermediate_message_, 'N');
            // dbglogger_ << "NOTIF..L1SIZEUPDATE NEW SELL "<<'\n' ; dbglogger_ .DumpCurrentBuffer ();
          }

          if (!this_smv_.market_update_info_.bidlevels_.empty() && !this_smv_.market_update_info_.asklevels_.empty()) {
            this_smv_.NotifyL1SizeListeners();
          }
          // this_smv_.OnL1SizeUpdate( );
          this_smv_.l1_changed_since_last_ = false;
          if (this_smv_.l2_changed_since_last_) {
            // dbglogger_ << "NOTIF..L2 NEW SELL "<<'\n' ; dbglogger_ .DumpCurrentBuffer ();
            this_smv_.Sanitize();
            this_smv_.OnL2Update();
            this_smv_.OnL2OnlyUpdate();
            this_smv_.SanitizeBidSideWithPrice(t_int_price_);

            this_smv_.l2_changed_since_last_ = false;
          }
          this_smv_.NotifyOnReadyListeners();
        } else  // Non-intermediate msg
        {
          if (this_smv_.pl_change_listeners_present_) {
            this_smv_.NotifyOnPLChangeListeners(t_security_id_, this_smv_.market_update_info_, kTradeTypeSell,
                                                t_level_added_ - 1, new_level_.limit_int_price_,
                                                new_level_.limit_int_price_level_, 0, t_new_size_, 0, t_new_ordercount_,
                                                t_is_intermediate_message_, 'N');
          }
          this_smv_.l1_changed_since_last_ = true;
        }

        // if ( dbglogger_.CheckLoggingLevel ( BOOK_TEST ) )
        // 	{
        // 	  DBGLOG_TIME_CLASS_FUNC << " AddTopAsk " << t_is_intermediate_message_ << "\n" << this_smv_.ShowMarket(
        // ) << DBGLOG_ENDL_FLUSH ;
        // 	}
      } else  // higher (level > 0 )
      {
        if (t_level_added_ > (int)this_smv_.market_update_info_.asklevels_.size() + 1) return;

        MarketUpdateInfoLevelStruct new_level_(0, this_smv_.GetIntPx(t_price_), t_price_, t_new_size_,
                                               t_new_ordercount_, watch_.tv());

        if (this_smv_.computing_price_levels()) {
          new_level_.limit_int_price_level_ =
              new_level_.limit_int_price_ - this_smv_.market_update_info_.asklevels_[0].limit_int_price_;
        }
        this_smv_.AddNonTopAsk(t_level_added_, new_level_);

        if (!t_is_intermediate_message_) {
          if (this_smv_.l1_changed_since_last_) {
            // #ifdef EUREX_KEEP_UNCROSS_CODE
            // 		      // If it isn't an intermediate message
            // 		      // Uncross
            // 		      bool crossed = this_smv_.Uncross ( );
            // 		      if ( crossed )
            // 			{
            // 			  this_smv_.SetBestLevelBidVariablesOnQuote ( );
            // 			  this_smv_.SetBestLevelAskVariablesOnQuote ( );
            // 			}
            // #endif

            this_smv_.UpdateL1Prices();

            if (this_smv_.pl_change_listeners_present_) {
              // make sure the market_update_info_ is consistent
              this_smv_.NotifyOnPLChangeListeners(t_security_id_, this_smv_.market_update_info_, kTradeTypeSell,
                                                  t_level_added_ - 1, new_level_.limit_int_price_,
                                                  new_level_.limit_int_price_level_, 0, t_new_size_, 0,
                                                  t_new_ordercount_, t_is_intermediate_message_, 'N');
              // dbglogger_ << "NOTIF..L1SIZEUPDATE NEW SELL "<<'\n' ; dbglogger_ .DumpCurrentBuffer ();
            }

            if (!this_smv_.market_update_info_.bidlevels_.empty() &&
                !this_smv_.market_update_info_.asklevels_.empty()) {
              this_smv_.NotifyL1SizeListeners();
            }
            // this_smv_.OnL1SizeUpdate( );
            this_smv_.l1_changed_since_last_ = false;
          } else  // l1_was_changed_before
          {
            if (this_smv_.pl_change_listeners_present_) {
              this_smv_.NotifyOnPLChangeListeners(t_security_id_, this_smv_.market_update_info_, kTradeTypeSell,
                                                  t_level_added_ - 1, new_level_.limit_int_price_,
                                                  new_level_.limit_int_price_level_, 0, t_new_size_, 0,
                                                  t_new_ordercount_, t_is_intermediate_message_, 'N');
            }
          }
          // dbglogger_ << "NOTIF..L2 SELL NEW"<<'\n' ; dbglogger_ .DumpCurrentBuffer ();
          this_smv_.Sanitize();
          this_smv_.OnL2Update();
          this_smv_.OnL2OnlyUpdate();
          this_smv_.NotifyOnReadyListeners();
          this_smv_.SanitizeBidSideWithPrice(t_int_price_);
          this_smv_.l2_changed_since_last_ = false;
        } else  // Non-intermediate message
        {
          if (this_smv_.pl_change_listeners_present_) {
            this_smv_.NotifyOnPLChangeListeners(t_security_id_, this_smv_.market_update_info_, kTradeTypeSell,
                                                t_level_added_ - 1, new_level_.limit_int_price_,
                                                new_level_.limit_int_price_level_, 0, t_new_size_, 0, t_new_ordercount_,
                                                t_is_intermediate_message_, 'N');
          }

          this_smv_.l2_changed_since_last_ = true;
        }

        // if ( dbglogger_.CheckLoggingLevel ( BOOK_TEST ) )
        // 	{
        // 	  DBGLOG_TIME_CLASS_FUNC << ":" << " AddNonTopAsk " << t_is_intermediate_message_ << "\n" <<
        // this_smv_.ShowMarket( ) << DBGLOG_ENDL_FLUSH ;
        // 	}
      }
    } break;
    default: {}
  }
  //	std::cerr << " Packet new: " << t_security_id_ << " " << t_level_added_ << " " << t_price_ << " " << t_new_size_
  //<< " " << t_new_ordercount_  << this_smv_.ShowMarket() ;//debuginfo
}

void OSEPriceFeedMarketViewManager::OnPriceLevelDelete(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                                       const int t_level_removed_, const double t_price_,
                                                       const bool t_is_intermediate_message_) {
  // if ( dbglogger_.CheckLoggingLevel ( BOOK_TEST ) )
  //   {
  //  	DBGLOG_TIME_CLASS_FUNC_LINE << " ( secid= " << t_security_id_ << " bs= " << GetTradeTypeChar(t_buysell_) << "
  //  lvl= " << t_level_removed_ << " px= " << t_price_ << " IsInter= " << (t_is_intermediate_message_?'Y':'N') << " )"
  //  << DBGLOG_ENDL_FLUSH ;
  //   }

  SecurityMarketView& this_smv_ = *(security_market_view_map_[t_security_id_]);

  this_smv_.last_message_was_trade_ = false;
  if (t_level_removed_ > DEF_MARKET_DEPTH) {
    // update was done to L1, don't want to hold it back as soon as non-intermediate msg is available irrespective of
    // level
    if (this_smv_.l1_changed_since_last_ && !t_is_intermediate_message_) {
      this_smv_.UpdateL1Prices();

      if (!this_smv_.market_update_info_.bidlevels_.empty() && !this_smv_.market_update_info_.asklevels_.empty()) {
        this_smv_.NotifyL1SizeListeners();
      }

      this_smv_.l1_changed_since_last_ = false;

      this_smv_.NotifyOnReadyListeners();
    }

    return;
  }

  int int_price_ = this_smv_.GetIntPx(t_price_);
  // if ( dbglogger_.CheckLoggingLevel ( BOOK_TEST ) )
  //   {
  // 	dbglogger_ << this_smv_.ShowMarket ( ) ;
  // 	dbglogger_.CheckToFlushBuffer ();
  //   }

  // this_smv_.highest_level_changed_ = (_level_removed_ -1);
  // this_smv_.buysell_highest_level_changed_ = t_buysell_;

  switch (t_buysell_) {
    case kTradeTypeBuy: {
      if (t_level_removed_ > (int)this_smv_.market_update_info_.bidlevels_.size()) {
        // dbglogger_ << "Prob Book Reset msg \n";
        return;
      }
      bool update_best_variables_and_notify_ = true;

      if (sid_to_bidside_trade_price_[t_security_id_] == int_price_) {
        sid_to_bidside_trade_size_[t_security_id_] = 0;
        sid_to_last_bidside_trade_time_[t_security_id_] = ttime_t(0, 0);  // is it necessary??
      }

      if (t_level_removed_ ==
          1) {  // top level changed .. .to optimize using PopFront and not O(DEF_MARKET_DEPTH) PopAt

        // int t_int_price_ = this_smv_.GetIntPx( t_price_ ) ;

        //              if ( t_int_price_ != this_smv_.market_update_info_.bidlevels_ [ 0 ].limit_int_price_ ) return ;
        if (this_smv_.market_update_info_.bestbid_int_price_ <
            this_smv_.market_update_info_.bidlevels_[0].limit_int_price_) {
          // the best variables have already changed to updated value
          update_best_variables_and_notify_ = false;
        }
        if (this_smv_.computing_price_levels() && this_smv_.market_update_info_.bidlevels_.size() > 1) {
          // update limit_int_price_level_ field everywhere
          int levels_from_top_to_deduct = this_smv_.market_update_info_.bidlevels_[0].limit_int_price_ -
                                          this_smv_.market_update_info_.bidlevels_[1].limit_int_price_;
          for (unsigned int i = 1; i < this_smv_.market_update_info_.bidlevels_.size(); i++) {
            this_smv_.market_update_info_.bidlevels_[i].limit_int_price_level_ -= levels_from_top_to_deduct;
          }
        }

        int old_size_at_this_price = 0;
        int old_bid_int_price = 0;
        int old_bid_int_price_level = 0;
        int old_ordercount_at_this_price = 0;

        if (this_smv_.pl_change_listeners_present_) {
          register unsigned int t_level_ = t_level_removed_ - 1;
          if (t_level_ < this_smv_.market_update_info_.bidlevels_.size()) {
            old_size_at_this_price = this_smv_.market_update_info_.bidlevels_[t_level_].limit_size_;
            old_bid_int_price = this_smv_.market_update_info_.bidlevels_[t_level_].limit_int_price_;
            old_bid_int_price_level =
                std::max(0, this_smv_.market_update_info_.bidlevels_[t_level_].limit_int_price_level_);
            old_ordercount_at_this_price = this_smv_.market_update_info_.bidlevels_[t_level_].limit_ordercount_;
          }
        }

        this_smv_.RemoveTopBid();

        this_smv_.SetBestLevelBidVariablesOnQuote();
        if (update_best_variables_and_notify_) {
          if (!t_is_intermediate_message_) {
            // #ifdef EUREX_KEEP_UNCROSS_CODE
            // 		  // If it isn't an intermediate message
            // 		  // Uncross
            // 		  bool crossed = this_smv_.Uncross ( );
            // 		  if ( crossed )
            // 		    {
            // 		      this_smv_.SetBestLevelBidVariablesOnQuote ( );
            // 		      this_smv_.SetBestLevelAskVariablesOnQuote ( );
            // 		    }
            // #endif

            this_smv_.UpdateL1Prices();
            // On PL_Delete Notification
            if (this_smv_.pl_change_listeners_present_) {
              // dbglogger_ << "Func: " << __func__ << " OnDelFIXFAST 1: Level removed : "<< t_level_removed_ - 1
              // <<'\n';
              // dbglogger_ << this_smv_.ShowMarket ( ) ;
              // dbglogger_ .DumpCurrentBuffer ( ) ;
              this_smv_.NotifyOnPLChangeListeners(t_security_id_, this_smv_.market_update_info_, kTradeTypeBuy,
                                                  t_level_removed_ - 1, old_bid_int_price, old_bid_int_price_level,
                                                  old_size_at_this_price, 0, old_ordercount_at_this_price, 0,
                                                  t_is_intermediate_message_, 'D');
            }

            if (!this_smv_.market_update_info_.bidlevels_.empty() &&
                !this_smv_.market_update_info_.asklevels_.empty()) {
              this_smv_.NotifyL1SizeListeners();
            }
            // this_smv_.OnL1SizeUpdate( );
            this_smv_.l1_changed_since_last_ = false;
            if (this_smv_.l2_changed_since_last_) {
              this_smv_.Sanitize();
              this_smv_.OnL2Update();
              this_smv_.OnL2OnlyUpdate();
              int t_int_price_ = this_smv_.GetIntPx(t_price_);
              this_smv_.SanitizeAskSideWithPrice(t_int_price_);
              this_smv_.l2_changed_since_last_ = false;
            }

            this_smv_.NotifyOnReadyListeners();
          } else  // NOn-intermediate messages
          {
            if (this_smv_.pl_change_listeners_present_) {
              // dbglogger_ << "Func: " << __func__ << " OnDelFIXFAST: 2 Level removed : "<< t_level_removed_ - 1
              // <<'\n';
              // dbglogger_ << this_smv_.ShowMarket ( ) ;
              // dbglogger_ .DumpCurrentBuffer ( ) ;
              this_smv_.NotifyOnPLChangeListeners(t_security_id_, this_smv_.market_update_info_, kTradeTypeBuy,
                                                  t_level_removed_ - 1, old_bid_int_price, old_bid_int_price_level,
                                                  old_size_at_this_price, 0, old_ordercount_at_this_price, 0,
                                                  t_is_intermediate_message_, 'D');
            }
            // L1 Price did not get updated
            this_smv_.l1_changed_since_last_ = true;
          }
        }

        // if ( dbglogger_.CheckLoggingLevel ( BOOK_TEST ) )
        // 	{
        // 	  dbglogger_ << watch_.tv() << ' ' << typeid( *this ).name() << ':' << __func__ << ":" << " RemoveTopBid
        // " << t_is_intermediate_message_ << "\n" << this_smv_.ShowMarket( ); dbglogger_.CheckToFlushBuffer ( );
        // 	}
      } else {
        // shoudl check for notifying here too?
        int old_size_at_this_price = 0;
        int old_bid_int_price = 0;
        int old_bid_int_price_level = 0;
        int old_ordercount_at_this_price = 0;

        if (this_smv_.pl_change_listeners_present_) {
          register unsigned int t_level_ = t_level_removed_ - 1;
          if (t_level_ < this_smv_.market_update_info_.bidlevels_.size()) {
            old_size_at_this_price = this_smv_.market_update_info_.bidlevels_[t_level_].limit_size_;
            old_bid_int_price = this_smv_.market_update_info_.bidlevels_[t_level_].limit_int_price_;
            old_bid_int_price_level =
                std::max(0, this_smv_.market_update_info_.bidlevels_[t_level_].limit_int_price_level_);
            old_ordercount_at_this_price = this_smv_.market_update_info_.bidlevels_[t_level_].limit_ordercount_;
          }
        }
        this_smv_.RemoveNonTopBid(t_level_removed_);

        if (!t_is_intermediate_message_) {
          if (this_smv_.l1_changed_since_last_) {
            // #ifdef EUREX_KEEP_UNCROSS_CODE
            // 		  // If it isn't an intermediate message
            // 		  // Uncross
            // 		  bool crossed = this_smv_.Uncross ( );
            // 		  if ( crossed )
            // 		    {
            // 		      this_smv_.SetBestLevelBidVariablesOnQuote ( );
            // 		      this_smv_.SetBestLevelAskVariablesOnQuote ( );
            // 		    }
            // #endif

            this_smv_.UpdateL1Prices();
            if (this_smv_.pl_change_listeners_present_) {
              // dbglogger_ << "Func: " << __func__ << " OnDelFIXFAST: 3 Level removed : "<< t_level_removed_ - 1
              // <<'\n';
              // dbglogger_ << this_smv_.ShowMarket ( ) ;
              // For PL indicators we send the update irrespective of intermediate_message
              // On PL_Delete Notification
              this_smv_.NotifyOnPLChangeListeners(t_security_id_, this_smv_.market_update_info_, kTradeTypeBuy,
                                                  t_level_removed_ - 1, old_bid_int_price, old_bid_int_price_level,
                                                  old_size_at_this_price, 0, old_ordercount_at_this_price, 0,
                                                  t_is_intermediate_message_, 'D');
            }

            if (!this_smv_.market_update_info_.bidlevels_.empty() &&
                !this_smv_.market_update_info_.asklevels_.empty()) {
              this_smv_.NotifyL1SizeListeners();
            }
            // this_smv_.OnL1SizeUpdate( );
            this_smv_.l1_changed_since_last_ = false;
          } else {
            if (this_smv_.pl_change_listeners_present_) {
              // dbglogger_ << "Func: " << __func__ << " OnDelFIXFAST: 4 Level removed : "<< t_level_removed_ - 1
              // <<'\n';
              // dbglogger_ << this_smv_.ShowMarket ( ) ;
              // For PL indicators we send the update irrespective of intermediate_message
              // On PL_Delete Notification
              this_smv_.NotifyOnPLChangeListeners(t_security_id_, this_smv_.market_update_info_, kTradeTypeBuy,
                                                  t_level_removed_ - 1, old_bid_int_price, old_bid_int_price_level,
                                                  old_size_at_this_price, 0, old_ordercount_at_this_price, 0,
                                                  t_is_intermediate_message_, 'D');
            }
          }
          this_smv_.Sanitize();
          this_smv_.OnL2Update();
          this_smv_.OnL2OnlyUpdate();
          this_smv_.NotifyOnReadyListeners();
          int t_int_price_ = this_smv_.GetIntPx(t_price_);
          this_smv_.SanitizeAskSideWithPrice(t_int_price_);
          this_smv_.l2_changed_since_last_ = false;
        } else  // NOn-intermediate messages
        {
          if (this_smv_.pl_change_listeners_present_) {
            // dbglogger_ << "Func: " << __func__ << " OnDelFIXFAST: 5 Level removed : "<< t_level_removed_ - 1 <<'\n';
            // dbglogger_ << this_smv_.ShowMarket ( ) ;

            this_smv_.NotifyOnPLChangeListeners(t_security_id_, this_smv_.market_update_info_, kTradeTypeBuy,
                                                t_level_removed_ - 1, old_bid_int_price, old_bid_int_price_level,
                                                old_size_at_this_price, 0, old_ordercount_at_this_price, 0,
                                                t_is_intermediate_message_, 'D');
          }
          this_smv_.l2_changed_since_last_ = true;
        }

        // if ( dbglogger_.CheckLoggingLevel ( BOOK_TEST ) )
        // 	{
        // 	  dbglogger_ << watch_.tv() << ' ' << typeid( *this ).name() << ':' << __func__ << ":" << "
        // RemoveNonTopBid " << t_is_intermediate_message_ << "\n" << this_smv_.ShowMarket( );
        // dbglogger_.CheckToFlushBuffer ( );
        // 	}
      }
    } break;
    case kTradeTypeSell: {
      if (t_level_removed_ > (int)this_smv_.market_update_info_.asklevels_.size()) {
        // dbglogger_ << "Prob Book Reset msg \n";
        return;
      }

      bool update_best_variables_and_notify_ = true;

      if (this_smv_.market_update_info_.bestask_int_price_ >
          this_smv_.market_update_info_.asklevels_[0].limit_int_price_) {
        update_best_variables_and_notify_ = false;
      }

      if (sid_to_askside_trade_price_[t_security_id_] == int_price_) {
        sid_to_askside_trade_size_[t_security_id_] = 0;
        sid_to_last_askside_trade_time_[t_security_id_] = ttime_t(0, 0);  // is it necessary??
      }

      if (t_level_removed_ == 1) {
        int t_int_price_ = this_smv_.GetIntPx(t_price_);

        //              if ( t_int_price_ != this_smv_.market_update_info_.asklevels_ [ 0 ].limit_int_price_ ) return ;

        // top level changed

        if (this_smv_.computing_price_levels() && this_smv_.market_update_info_.asklevels_.size() > 1) {
          // update limit_int_price_level_ field everywhere

          int levels_from_top_to_deduct =
              this_smv_.market_update_info_.asklevels_[1].limit_int_price_ -
              this_smv_.market_update_info_.asklevels_[0].limit_int_price_;  // ask so lvl(1) is more than lvl(0)
          for (unsigned int i = 1; i < this_smv_.market_update_info_.asklevels_.size(); i++) {
            this_smv_.market_update_info_.asklevels_[i].limit_int_price_level_ -= levels_from_top_to_deduct;
          }
        }
        int old_size_at_this_price = 0;
        int old_ask_int_price = 0;
        int old_ask_int_price_level = 0;
        int old_ordercount_at_this_price = 0;

        if (this_smv_.pl_change_listeners_present_) {
          register unsigned int t_level_ = t_level_removed_ - 1;
          if (t_level_ < this_smv_.market_update_info_.asklevels_.size()) {
            old_size_at_this_price = this_smv_.market_update_info_.asklevels_[t_level_].limit_size_;
            old_ask_int_price = this_smv_.market_update_info_.asklevels_[t_level_].limit_int_price_;
            old_ask_int_price_level =
                std::max(0, this_smv_.market_update_info_.asklevels_[t_level_].limit_int_price_level_);
            old_ordercount_at_this_price = this_smv_.market_update_info_.asklevels_[t_level_].limit_ordercount_;
          }
        }

        this_smv_.RemoveTopAsk();

        this_smv_.SetBestLevelAskVariablesOnQuote();

        if (update_best_variables_and_notify_) {
          if (!t_is_intermediate_message_) {
            // #ifdef EUREX_KEEP_UNCROSS_CODE
            // 		  // If it isn't an intermediate message
            // 		  // Uncross
            // 		  bool crossed = this_smv_.Uncross ( );
            // 		  if ( crossed )
            // 		    {
            // 		      this_smv_.SetBestLevelBidVariablesOnQuote ( );
            // 		      this_smv_.SetBestLevelAskVariablesOnQuote ( );
            // 		    }
            // #endif

            this_smv_.UpdateL1Prices();
            if (this_smv_.pl_change_listeners_present_) {
              // For PL indicators we send the update irrespective of intermediate_message
              // On PL_Delete Notification
              // dbglogger_ << "Func: " << __func__ << " OnDelFIXFAST: 6 SELL : "<< t_level_removed_ - 1 <<'\n';
              // dbglogger_ << this_smv_.ShowMarket ( ) ;
              // dbglogger_ .DumpCurrentBuffer ( ) ;
              this_smv_.NotifyOnPLChangeListeners(t_security_id_, this_smv_.market_update_info_, kTradeTypeSell,
                                                  t_level_removed_ - 1, old_ask_int_price, old_ask_int_price_level,
                                                  old_size_at_this_price, 0, old_ordercount_at_this_price, 0,
                                                  t_is_intermediate_message_, 'D');
            }

            if (!this_smv_.market_update_info_.bidlevels_.empty() &&
                !this_smv_.market_update_info_.asklevels_.empty()) {
              this_smv_.NotifyL1SizeListeners();
            }
            // this_smv_.OnL1SizeUpdate( );
            this_smv_.l1_changed_since_last_ = false;
            if (this_smv_.l2_changed_since_last_) {
              this_smv_.Sanitize();
              this_smv_.OnL2Update();
              this_smv_.OnL2OnlyUpdate();
              this_smv_.SanitizeBidSideWithPrice(t_int_price_);
              this_smv_.l2_changed_since_last_ = false;
            }
            this_smv_.NotifyOnReadyListeners();
          } else  // Non-intermediate_message
          {
            if (this_smv_.pl_change_listeners_present_) {
              // For PL indicators we send the update irrespective of intermediate_message
              // On PL_Delete Notification
              // dbglogger_ << "Func: " << __func__ << " OnDelFIXFAST: 7 SELL : "<< t_level_removed_ - 1 <<'\n';
              // dbglogger_ << this_smv_.ShowMarket ( ) ;
              // dbglogger_ .DumpCurrentBuffer ( ) ;

              this_smv_.NotifyOnPLChangeListeners(t_security_id_, this_smv_.market_update_info_, kTradeTypeSell,
                                                  t_level_removed_ - 1, old_ask_int_price, old_ask_int_price_level,
                                                  old_size_at_this_price, 0, old_ordercount_at_this_price, 0,
                                                  t_is_intermediate_message_, 'D');
            }
            this_smv_.l1_changed_since_last_ = true;
          }
        }
        // if ( dbglogger_.CheckLoggingLevel ( BOOK_TEST ) )
        // 	{
        // 	  dbglogger_ << watch_.tv() << ' ' << typeid( *this ).name() << ':' << __func__ << ":"  << "
        // RemoveTopAsk " << t_is_intermediate_message_ << "\n" << this_smv_.ShowMarket( );
        // dbglogger_.CheckToFlushBuffer ( );
        // 	}
      } else {
        // not sure if i need to check for notifying here too
        int old_size_at_this_price = 0;
        int old_ask_int_price = 0;
        int old_ask_int_price_level = 0;
        int old_ordercount_at_this_price = 0;

        if (this_smv_.pl_change_listeners_present_) {
          register unsigned int t_level_ = t_level_removed_ - 1;
          if (t_level_ < this_smv_.market_update_info_.asklevels_.size()) {
            old_size_at_this_price = this_smv_.market_update_info_.asklevels_[t_level_].limit_size_;
            old_ask_int_price = this_smv_.market_update_info_.asklevels_[t_level_].limit_int_price_;
            old_ask_int_price_level =
                std::max(0, this_smv_.market_update_info_.asklevels_[t_level_].limit_int_price_level_);
            old_ordercount_at_this_price = this_smv_.market_update_info_.asklevels_[t_level_].limit_ordercount_;
          }
        }

        this_smv_.RemoveNonTopAsk(t_level_removed_);

        if (!t_is_intermediate_message_) {
          if (this_smv_.l1_changed_since_last_) {
            // #ifdef EUREX_KEEP_UNCROSS_CODE
            // 		  // If it isn't an intermediate message
            // 		  // Uncross
            // 		  bool crossed = this_smv_.Uncross ( );
            // 		  if ( crossed )
            // 		    {
            // 		      this_smv_.SetBestLevelBidVariablesOnQuote ( );
            // 		      this_smv_.SetBestLevelAskVariablesOnQuote ( );
            // 		    }
            // #endif

            this_smv_.UpdateL1Prices();
            if (this_smv_.pl_change_listeners_present_) {
              // For PL indicators we send the update irrespective of intermediate_message
              // On PL_Delete Notification
              // dbglogger_ << "Func: " << __func__ << " OnDelFIXFAST: 8 SELL : "<< t_level_removed_ - 1 <<'\n';
              // dbglogger_ << this_smv_.ShowMarket ( ) ;
              // dbglogger_ .DumpCurrentBuffer ( ) ;

              this_smv_.NotifyOnPLChangeListeners(t_security_id_, this_smv_.market_update_info_, kTradeTypeSell,
                                                  t_level_removed_ - 1, old_ask_int_price, old_ask_int_price_level,
                                                  old_size_at_this_price, 0, old_ordercount_at_this_price, 0,
                                                  t_is_intermediate_message_, 'D');
            }

            if (!this_smv_.market_update_info_.bidlevels_.empty() &&
                !this_smv_.market_update_info_.asklevels_.empty()) {
              this_smv_.NotifyL1SizeListeners();
            }
            // this_smv_.OnL1SizeUpdate( );
            this_smv_.l1_changed_since_last_ = false;
          } else {
            if (this_smv_.pl_change_listeners_present_) {
              // For PL indicators we send the update irrespective of intermediate_message
              // On PL_Delete Notification
              // dbglogger_ << "Func: " << __func__ << " OnDelFIXFAST:9  SELL : "<< t_level_removed_ - 1 <<'\n';
              // dbglogger_ << this_smv_.ShowMarket ( ) ;
              // dbglogger_ .DumpCurrentBuffer ( ) ;

              this_smv_.NotifyOnPLChangeListeners(t_security_id_, this_smv_.market_update_info_, kTradeTypeSell,
                                                  t_level_removed_ - 1, old_ask_int_price, old_ask_int_price_level,
                                                  old_size_at_this_price, 0, old_ordercount_at_this_price, 0,
                                                  t_is_intermediate_message_, 'D');
            }
          }
          this_smv_.Sanitize();
          this_smv_.OnL2Update();
          this_smv_.OnL2OnlyUpdate();
          this_smv_.NotifyOnReadyListeners();
          int t_int_price_ = this_smv_.GetIntPx(t_price_);
          this_smv_.SanitizeBidSideWithPrice(t_int_price_);
          this_smv_.l2_changed_since_last_ = false;
        } else  // Non-intermediate_message
        {
          if (this_smv_.pl_change_listeners_present_) {
            // dbglogger_ << "Func: " << __func__ << " OnDelFIXFAST: 10 SELL : "<< t_level_removed_ - 1 <<'\n';
            // dbglogger_ << this_smv_.ShowMarket ( ) ;
            // dbglogger_ .DumpCurrentBuffer ( ) ;

            this_smv_.NotifyOnPLChangeListeners(t_security_id_, this_smv_.market_update_info_, kTradeTypeSell,
                                                t_level_removed_ - 1, old_ask_int_price, old_ask_int_price_level,
                                                old_size_at_this_price, 0, old_ordercount_at_this_price, 0,
                                                t_is_intermediate_message_, 'D');
          }
          this_smv_.l2_changed_since_last_ = true;
        }

        // if ( dbglogger_.CheckLoggingLevel ( BOOK_TEST ) )
        // 	{
        // 	  dbglogger_ << watch_.tv() << ' ' << typeid( *this ).name() << ':' << __func__ << ":" << "
        // RemoveNonTopAsk " << t_is_intermediate_message_ << "\n" << this_smv_.ShowMarket( );
        // dbglogger_.CheckToFlushBuffer ( );
        // 	}
      }
    } break;
    default: {}
  }

  // std::cerr << " Packet delete : " << t_security_id_ << " " << t_level_removed_ << " " << t_price_ << " " <<
  // this_smv_.ShowMarket() ;//debuginfo
}

/// can be optimized later .. very very few calls anticipated
void OSEPriceFeedMarketViewManager::OnPriceLevelDeleteFrom(const unsigned int t_security_id_,
                                                           const TradeType_t t_buysell_, const int t_min_level_deleted_,
                                                           const bool t_is_intermediate_message_) {
  if (t_min_level_deleted_ > DEF_MARKET_DEPTH) return;

  if (dbglogger_.CheckLoggingLevel(BOOK_INFO)) {
    DBGLOG_TIME_CLASS_FUNC << "called with min level deleted " << t_min_level_deleted_ << DBGLOG_ENDL_FLUSH;
  }

  /// call multiple instances of OnDelete
  SecurityMarketView& this_smv_ = *(security_market_view_map_[t_security_id_]);

  /// debug remove later
  // if ( dbglogger_.CheckLoggingLevel ( BOOK_TEST ) )
  //   {
  // 	dbglogger_ << this_smv_.ShowMarket ( ) << DBGLOG_ENDL_FLUSH ;
  //   }

  if (t_buysell_ == kTradeTypeBuy) {
    while ((int)this_smv_.market_update_info_.bidlevels_.size() >= t_min_level_deleted_) {
      OnPriceLevelDelete(
          t_security_id_, t_buysell_, t_min_level_deleted_,
          this_smv_.market_update_info_.bidlevels_[t_min_level_deleted_ - 1].limit_price_,
          t_is_intermediate_message_ || ((int)this_smv_.market_update_info_.bidlevels_.size() > t_min_level_deleted_));
    }
  } else {
    while ((int)this_smv_.market_update_info_.asklevels_.size() >= t_min_level_deleted_) {
      OnPriceLevelDelete(
          t_security_id_, t_buysell_, t_min_level_deleted_,
          this_smv_.market_update_info_.asklevels_[t_min_level_deleted_ - 1].limit_price_,
          t_is_intermediate_message_ || ((int)this_smv_.market_update_info_.asklevels_.size() > t_min_level_deleted_));
    }
  }

  if (dbglogger_.CheckLoggingLevel(BOOK_TEST)) {
    DBGLOG_TIME_CLASS_FUNC << " processing over \n" << this_smv_.ShowMarket() << DBGLOG_ENDL_FLUSH;
  }
}

/// can be optimized later .. very very few calls anticipated
void OSEPriceFeedMarketViewManager::OnPriceLevelDeleteThrough(const unsigned int t_security_id_,
                                                              const TradeType_t t_buysell_,
                                                              const int t_max_level_deleted_,
                                                              const bool t_is_intermediate_message_) {
  if (dbglogger_.CheckLoggingLevel(BOOK_INFO)) {
    DBGLOG_TIME_CLASS_FUNC << " called with max_level_deleted " << t_max_level_deleted_ << DBGLOG_ENDL_FLUSH;
  }

  SecurityMarketView& this_smv_ = *(security_market_view_map_[t_security_id_]);

  if (dbglogger_.CheckLoggingLevel(BOOK_TEST)) {
    DBGLOG_TIME_CLASS_FUNC << this_smv_.ShowMarket() << DBGLOG_ENDL_FLUSH;
  }

  int t_count_ = 1;
  if (t_buysell_ == kTradeTypeBuy) {
    while (t_count_ <= t_max_level_deleted_ && !(this_smv_.market_update_info_.bidlevels_.empty())) {
      OnPriceLevelDelete(t_security_id_, t_buysell_, 1, this_smv_.market_update_info_.bidlevels_[0].limit_price_,
                         t_is_intermediate_message_ || (t_count_ < t_max_level_deleted_));
      t_count_++;
    }
  } else {
    while (t_count_ <= t_max_level_deleted_ && !(this_smv_.market_update_info_.asklevels_.empty())) {
      OnPriceLevelDelete(t_security_id_, t_buysell_, 1, this_smv_.market_update_info_.asklevels_[0].limit_price_,
                         t_is_intermediate_message_ || (t_count_ < t_max_level_deleted_));
      t_count_++;
    }
  }

  if (dbglogger_.CheckLoggingLevel(BOOK_TEST)) {
    DBGLOG_TIME_CLASS_FUNC << " processing over \n" << this_smv_.ShowMarket() << DBGLOG_ENDL_FLUSH;
  }
}

// Change and Overlay do not depend on the data structure simulating array
// If due to trade_before_quote or PromOrderManager we change the top level, we need to see on a change whether the
// price matches or not
void OSEPriceFeedMarketViewManager::OnPriceLevelChange(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                                       const int t_level_changed_, const double t_price_,
                                                       const int t_new_size_, const int t_new_ordercount_,
                                                       const bool t_is_intermediate_message_) {
  SecurityMarketView& this_smv_ = *(security_market_view_map_[t_security_id_]);

  this_smv_.last_message_was_trade_ = false;
  // During startup phase ignore changes that are greater than size also
  if (t_level_changed_ > DEF_MARKET_DEPTH) {
    // update was done to L1, don't want to hold it back as soon as non-intermediate msg is available irrespective of
    // level
    // There are no intermediate messages in oSE
    if (this_smv_.l1_changed_since_last_ && !t_is_intermediate_message_) {
      this_smv_.UpdateL1Prices();

      if (!this_smv_.market_update_info_.bidlevels_.empty() && !this_smv_.market_update_info_.asklevels_.empty()) {
        this_smv_.NotifyL1SizeListeners();
      }

      this_smv_.l1_changed_since_last_ = false;

      this_smv_.NotifyOnReadyListeners();
    }

    return;
  }

  // if ( dbglogger_.CheckLoggingLevel ( BOOK_TEST ) )
  //   {
  // 	dbglogger_ << this_smv_.ShowMarket ( ) ;
  //   }

  // this_smv_.highest_level_changed_ = (_level_changed_ -1);
  // this_smv_.buysell_highest_level_changed_ = t_buysell_;
  //    int index_to_update_ = t_level_changed_;

  bool update_best_variables_and_notify_ = true;
  switch (t_buysell_) {
    case kTradeTypeBuy: {
      // this would be the case when the level changed is < MKT_DEPTH but the full book is not available, still the last
      // updated should be passed to listener
      //	  if ( t_level_changed_ > ( int )this_smv_.market_update_info_.bidlevels_.size( ) + 1 ) {
      //
      //            if ( this_smv_.l1_changed_since_last_ && !t_is_intermediate_message_ ) {
      //
      //              this_smv_.UpdateL1Prices ( );
      //
      //              if ( !this_smv_. market_update_info_.bidlevels_.empty ( ) &&
      //                   !this_smv_. market_update_info_.asklevels_.empty ( ) )
      //                {
      //                  this_smv_.NotifyL1SizeListeners ( );
      //                }
      //
      //              this_smv_.l1_changed_since_last_ = false;
      //
      //              this_smv_.NotifyOnReadyListeners ( );
      //
      //            }
      //
      //            return;
      //
      //          }

      if (t_level_changed_ > (int)this_smv_.market_update_info_.bidlevels_.size()) {
        // if ( dbglogger_.CheckLoggingLevel ( BOOK_TEST ) )
        // 	{
        // 	  dbglogger_ << watch_.tv() << ' ' << typeid( *this ).name() << ':' << __func__ << ":" << " Calling Bid
        // PxLevelNew \n"; dbglogger_.CheckToFlushBuffer( );
        // 	}

        OnPriceLevelNew(t_security_id_, t_buysell_, t_level_changed_, t_price_, t_new_size_, t_new_ordercount_,
                        t_is_intermediate_message_);
        return;
      } else {  // level was already valid

        MarketUpdateInfoLevelStruct* this_level_ = NULL;
        int old_size_at_this_price = 0;
        int old_ordercount_at_this_price = 0;
        // just copy info to this level struct. if px changed update the int_px_level values
        int diff = this_smv_.DblPxDiffSign(
            t_price_, this_smv_.market_update_info_.bidlevels_[(t_level_changed_ - 1)].limit_price_);
        if (diff != 0) {
          if (diff == 1) {  /// missed an Add

            if (t_level_changed_ == 1) {  /// insert new level on top

              // if ( dbglogger_.CheckLoggingLevel ( BOOK_TEST ) )
              //   {
              //     dbglogger_ << watch_.tv() << ' ' << typeid( *this ).name() << ':' << __func__ << ":" << "BA will
              //     fix crap by adding on top \n"; dbglogger_.CheckToFlushBuffer( );
              //   }

              OnPriceLevelNew(t_security_id_, t_buysell_, t_level_changed_, t_price_, t_new_size_, t_new_ordercount_,
                              t_is_intermediate_message_);
              return;
            } else {
              int i = 0;
              bool found = false;
              for (i = t_level_changed_ - 2; i >= 0; i--) {
                if (this_smv_.DblPxDiffSign(t_price_, this_smv_.market_update_info_.bidlevels_[i].limit_price_) == 0) {
                  found = true;
                  break;
                } else if (this_smv_.DblPxDiffSign(t_price_,
                                                   this_smv_.market_update_info_.bidlevels_[i].limit_price_) == -1) {
                  break;
                }
              }
              if (i >= 0 && found) {
                // if ( dbglogger_.CheckLoggingLevel ( BOOK_TEST ) )
                // 	{
                // 	  dbglogger_ << watch_.tv() << ' ' << typeid( *this ).name() << ':' << __func__ << ":" << " BA
                // found correct level " << i << "\n"; dbglogger_.CheckToFlushBuffer( );
                // 	}

                this_level_ = &(this_smv_.market_update_info_.bidlevels_[i]);
                //			      index_to_update_ = i+1;

              } else if (i >= 0) {
                // if ( dbglogger_.CheckLoggingLevel ( BOOK_TEST ) )
                // 	{
                // 	  dbglogger_ << watch_.tv() << ' ' << typeid( *this ).name() << ':' << __func__ << ":" << " BA
                // will insert new inter level " << i << "\n"; dbglogger_.CheckToFlushBuffer( );
                // 	}

                OnPriceLevelNew(t_security_id_, t_buysell_, i + 2, t_price_, t_new_size_, t_new_ordercount_,
                                t_is_intermediate_message_);
                return;
              } else {
                // if ( dbglogger_.CheckLoggingLevel ( BOOK_TEST ) )
                // 	{
                // 	  dbglogger_ << watch_.tv() << ' ' << typeid( *this ).name() << ':' << __func__ << ":" << " BA
                // will ignore .. cannot fix \n"; dbglogger_.CheckToFlushBuffer( );
                // 	}

                return;
              }
            }
          } else {  /// missed delete

            if (t_level_changed_ == 1) {
              // if ( dbglogger_.CheckLoggingLevel ( BOOK_TEST ) )
              //   {
              //     dbglogger_ << watch_.tv() << ' ' << typeid( *this ).name() << ':' << __func__ << ":" << " BD will
              //     fix crap by deleting on top \n"; dbglogger_.CheckToFlushBuffer( );
              //   }

              OnPriceLevelDelete(t_security_id_, t_buysell_, t_level_changed_,
                                 this_smv_.market_update_info_.bidlevels_[(t_level_changed_ - 1)].limit_price_, true);
              this_level_ = &(this_smv_.market_update_info_.bidlevels_[0]);
            } else {
              int i = 0;
              bool found = false;
              for (i = t_level_changed_; i < (int)this_smv_.market_update_info_.bidlevels_.size(); i++) {
                if (this_smv_.DblPxDiffSign(t_price_, this_smv_.market_update_info_.bidlevels_[i].limit_price_) == 0) {
                  found = true;
                  break;
                } else if (this_smv_.DblPxDiffSign(t_price_,
                                                   this_smv_.market_update_info_.bidlevels_[i].limit_price_) == 1)
                  break;
              }
              if (i < (int)this_smv_.market_update_info_.bidlevels_.size() && found) {
                // if ( dbglogger_.CheckLoggingLevel ( BOOK_TEST ) )
                // 	{
                // 	  dbglogger_ << watch_.tv() << ' ' << typeid( *this ).name() << ':' << __func__ << ":" << " BD
                // found correct level " << i << "\n"; dbglogger_.CheckToFlushBuffer( );
                // 	}

                this_level_ = &(this_smv_.market_update_info_.bidlevels_[i]);
                //			      index_to_update_ = i+1;

              } else if (i < (int)this_smv_.market_update_info_.bidlevels_.size()) {
                // if ( dbglogger_.CheckLoggingLevel ( BOOK_TEST ) )
                // 	{
                // 	  dbglogger_ << watch_.tv() << ' ' << typeid( *this ).name() << ':' << __func__ << ":" << " BD
                // will insert new inter level " << i << "\n"; dbglogger_.CheckToFlushBuffer( );
                // 	}

                OnPriceLevelNew(t_security_id_, t_buysell_, i, t_price_, t_new_size_, t_new_ordercount_,
                                t_is_intermediate_message_);
                return;
              } else {
                // if ( dbglogger_.CheckLoggingLevel ( BOOK_TEST ) )
                // 	{
                // 	  dbglogger_ << watch_.tv() << ' ' << typeid( *this ).name() << ':' << __func__ << ":" << " BD
                // will ignore .. cannot fix \n"; dbglogger_.CheckToFlushBuffer( );
                // 	}
                return;
              }
            }
          }
        } else {
          this_level_ = &(this_smv_.market_update_info_.bidlevels_[(t_level_changed_ - 1)]);
          if (this_smv_.pl_change_listeners_present_) {
            old_size_at_this_price = this_level_->limit_size_;  // Old size
            old_ordercount_at_this_price = this_level_->limit_ordercount_;
          }
        }

        if (diff != 0) {
          this_level_->limit_int_price_ = this_smv_.GetIntPx(t_price_);
          this_level_->limit_price_ = t_price_;
        }

        update_best_variables_and_notify_ = true;
        if (t_level_changed_ == 1) {
          //                DBGLOG_TIME_CLASS_FUNC << " bidchange " << sid_to_bidside_trade_price_ [ t_security_id_ ]
          //                  << " " << this_level_ -> limit_int_price_
          //                  << " " << t_new_size_
          //                  << " " << this_level_ -> limit_size_
          //                  << " " << sid_to_bidside_trade_size_ [ t_security_id_ ]
          //                  << " " << sid_to_last_bidside_trade_time_ [ t_security_id_ ]
          //                  << DBGLOG_ENDL_FLUSH;
          if (sid_to_bidside_trade_price_[t_security_id_] == this_level_->limit_int_price_) {
            if (t_new_size_ < this_level_->limit_size_) {
              if (sid_to_bidside_trade_size_[t_security_id_] == (this_level_->limit_size_ - t_new_size_) ||
                  watch_.tv() >=
                      sid_to_last_bidside_trade_time_[t_security_id_] + ttime_t(SECONDS_TO_WAIT, MSECS_TO_WAIT)) {
                sid_to_highest_accumulated_bidside_trade_size_[t_security_id_] = 0;
              }
              // size has been decreased since last
              if (sid_to_bidside_trade_size_[t_security_id_] >= (this_level_->limit_size_ - t_new_size_) &&
                  watch_.tv() <
                      sid_to_last_bidside_trade_time_[t_security_id_] + ttime_t(SECONDS_TO_WAIT, MSECS_TO_WAIT)) {
                sid_to_bidside_trade_size_[t_security_id_] -= (this_level_->limit_size_ - t_new_size_);
                update_best_variables_and_notify_ = false;
              } else {
                sid_to_highest_accumulated_bidside_trade_size_[t_security_id_] = 0;
                // update_best_variables_and_notify_ = true;
              }

            } else {
              sid_to_highest_accumulated_bidside_trade_size_[t_security_id_] = 0;
              sid_to_bidside_trade_size_[t_security_id_] = 0;
              sid_to_last_bidside_trade_time_[t_security_id_] = ttime_t(0, 0);

              // size increased from last
              // there may be cases where notifying harms
              // update_best_variables_and_notify_ = true;
            }

          } else {
            sid_to_highest_accumulated_bidside_trade_size_[t_security_id_] = 0;
            // the last trade price nad current best variables are not same
            // what might be the situation
            // update_best_variables_and_notify_ = true;
          }
        }

        this_level_->limit_size_ = t_new_size_;
        this_level_->limit_ordercount_ = t_new_ordercount_;
        this_level_->mod_time_ = watch_.tv();

        if (this_smv_.computing_price_levels()) {
          if (diff != 0) {
            this_level_->limit_int_price_level_ =
                this_smv_.market_update_info_.bidlevels_.front().limit_int_price_ - this_level_->limit_int_price_;
            if (t_level_changed_ == 1) {
              for (unsigned int i = 1; i < this_smv_.market_update_info_.bidlevels_.size(); i++) {
                this_smv_.market_update_info_.bidlevels_[i].limit_int_price_level_ =
                    this_level_->limit_int_price_ - this_smv_.market_update_info_.bidlevels_[i].limit_int_price_;
              }
            }
          }
        }

        if (t_level_changed_ == 1) {
          if (update_best_variables_and_notify_) {
            this_smv_.SetBestLevelBidVariablesOnQuote();  // TODO .. optimization : no need to assign bestbid_price_ and
                                                          // bestbid_int_price_ since they have not changed
          }

          if (update_best_variables_and_notify_) {
            if (!t_is_intermediate_message_) {
              // #ifdef EUREX_KEEP_UNCROSS_CODE
              // 		  // If it isn't an intermediate message
              // 		  // Uncross
              // 		  bool crossed = this_smv_.Uncross ( );
              // 		  if ( crossed )
              // 		    {
              // 		      this_smv_.SetBestLevelBidVariablesOnQuote ( );
              // 		      this_smv_.SetBestLevelAskVariablesOnQuote ( );
              // 		    }
              // #endif

              this_smv_.UpdateL1Prices();
              // For PL indicators we send the update irrespective of intermediate_message
              // On PL_change Notification
              // Pass OLd_size as well
              if (this_smv_.pl_change_listeners_present_) {
                this_smv_.NotifyOnPLChangeListeners(
                    t_security_id_, this_smv_.market_update_info_, kTradeTypeBuy, t_level_changed_ - 1,
                    this_level_->limit_int_price_, this_level_->limit_int_price_level_, old_size_at_this_price,
                    t_new_size_, old_ordercount_at_this_price, t_new_ordercount_, t_is_intermediate_message_, 'C');
                // dbglogger_ << "NOTIF..L1SIZEUPDATE CHNAGE "<<'\n' ; dbglogger_ .DumpCurrentBuffer ();
              }

              if (!this_smv_.market_update_info_.bidlevels_.empty() &&
                  !this_smv_.market_update_info_.asklevels_.empty()) {
                this_smv_.NotifyL1SizeListeners();
              }
              // this_smv_.OnL1SizeUpdate( );
              this_smv_.l1_changed_since_last_ = false;
              if (this_smv_.l2_changed_since_last_) {
                // dbglogger_ << "NOTIF..L2UPDATE "<<'\n' ; dbglogger_ .DumpCurrentBuffer ();
                this_smv_.Sanitize();
                this_smv_.OnL2Update();
                this_smv_.OnL2OnlyUpdate();
                this_smv_.l2_changed_since_last_ = false;
              }
              this_smv_.NotifyOnReadyListeners();
            } else  // Non-intermediate_message
            {
              if (this_smv_.pl_change_listeners_present_) {
                this_smv_.NotifyOnPLChangeListeners(
                    t_security_id_, this_smv_.market_update_info_, kTradeTypeBuy, t_level_changed_ - 1,
                    this_level_->limit_int_price_, this_level_->limit_int_price_level_, old_size_at_this_price,
                    t_new_size_, old_ordercount_at_this_price, t_new_ordercount_, t_is_intermediate_message_, 'C');
              }
              this_smv_.l1_changed_since_last_ = true;
            }
          }
        } else  // Higher Level
        {
          // does it matter to have the trade masking effect here??:
          if (!t_is_intermediate_message_) {
            if (this_smv_.l1_changed_since_last_) {
              // #ifdef EUREX_KEEP_UNCROSS_CODE
              // 		  // If it isn't an intermediate message
              // 		  // Uncross
              // 		  bool crossed = this_smv_.Uncross ( );
              // 		  if ( crossed )
              // 		    {
              // 		      this_smv_.SetBestLevelBidVariablesOnQuote ( );
              // 		      this_smv_.SetBestLevelAskVariablesOnQuote ( );
              // 		    }
              // #endif

              this_smv_.UpdateL1Prices();
              if (!this_smv_.market_update_info_.bidlevels_.empty() &&
                  !this_smv_.market_update_info_.asklevels_.empty()) {
                this_smv_.NotifyL1SizeListeners();
              }

              // For PL indicators we send the update irrespective of intermediate_message
              // On PL_change Notification
              // Pass OLd_size as well
              if (this_smv_.pl_change_listeners_present_) {
                this_smv_.NotifyOnPLChangeListeners(
                    t_security_id_, this_smv_.market_update_info_, kTradeTypeBuy, t_level_changed_ - 1,
                    this_level_->limit_int_price_, this_level_->limit_int_price_level_, old_size_at_this_price,
                    t_new_size_, old_ordercount_at_this_price, t_new_ordercount_, t_is_intermediate_message_, 'C');
                // dbglogger_ << "NOTIF..L1SIZEUPDATE ONCHANGE "<<'\n' ; dbglogger_ .DumpCurrentBuffer ();
              }

              // this_smv_.OnL1SizeUpdate( );
              this_smv_.l1_changed_since_last_ = false;
            } else {
              if (this_smv_.pl_change_listeners_present_) {
                this_smv_.NotifyOnPLChangeListeners(
                    t_security_id_, this_smv_.market_update_info_, kTradeTypeBuy, t_level_changed_ - 1,
                    this_level_->limit_int_price_, this_level_->limit_int_price_level_, old_size_at_this_price,
                    t_new_size_, old_ordercount_at_this_price, t_new_ordercount_, t_is_intermediate_message_, 'C');
              }
            }
            // dbglogger_ << "NOTIF..L2UPDATE ONCHANGE "<<'\n' ; dbglogger_ .DumpCurrentBuffer ();
            this_smv_.Sanitize();
            this_smv_.OnL2Update();
            this_smv_.OnL2OnlyUpdate();
            this_smv_.NotifyOnReadyListeners();
            this_smv_.l2_changed_since_last_ = false;
          } else  // Non-intermediate_message
          {
            // For PL indicators we send the update irrespective of intermediate_message
            // On PL_change Notification
            // Pass OLd_size as well
            if (this_smv_.pl_change_listeners_present_) {
              this_smv_.NotifyOnPLChangeListeners(
                  t_security_id_, this_smv_.market_update_info_, kTradeTypeBuy, t_level_changed_ - 1,
                  this_level_->limit_int_price_, this_level_->limit_int_price_level_, old_size_at_this_price,
                  t_new_size_, old_ordercount_at_this_price, t_new_ordercount_, t_is_intermediate_message_, 'C');
            }
            this_smv_.l2_changed_since_last_ = true;
          }
        }

        // if ( dbglogger_.CheckLoggingLevel ( BOOK_TEST ) )
        // 	{
        // 	  dbglogger_ << watch_.tv() << ' ' << typeid( *this ).name() << ':' << __func__ << ":" << " Updating Bid
        // " << t_is_intermediate_message_ << "\n" << this_smv_.ShowMarket( ); dbglogger_.CheckToFlushBuffer( );
        // 	}
      }
    } break;
    case kTradeTypeSell: {
      // this would be the case when the level changed is < MKT_DEPTH but the full book is not available, still the last
      // updated should be passed to listener
      if (t_level_changed_ > (int)this_smv_.market_update_info_.asklevels_.size() + 1) {
        if (this_smv_.l1_changed_since_last_ && !t_is_intermediate_message_) {
          this_smv_.UpdateL1Prices();

          if (!this_smv_.market_update_info_.bidlevels_.empty() && !this_smv_.market_update_info_.asklevels_.empty()) {
            this_smv_.NotifyL1SizeListeners();
          }

          this_smv_.l1_changed_since_last_ = false;

          this_smv_.NotifyOnReadyListeners();
        }

        return;

      }

      else if (t_level_changed_ > (int)this_smv_.market_update_info_.asklevels_.size()) {
        // if ( dbglogger_.CheckLoggingLevel ( BOOK_TEST ) )
        // 	{
        // 	  dbglogger_ << watch_.tv() << ' ' << typeid( *this ).name() << ':' << __func__ << ":" << " Calling Ask
        // PxLevelNew \n"; dbglogger_.CheckToFlushBuffer( );
        // 	}

        OnPriceLevelNew(t_security_id_, t_buysell_, t_level_changed_, t_price_, t_new_size_, t_new_ordercount_,
                        t_is_intermediate_message_);
        return;
      } else {  // level was already valid
        MarketUpdateInfoLevelStruct* this_level_ = NULL;
        int old_size_at_this_price = 0;
        int old_ordercount_at_this_price = 0;
        // just copy info to this level struct. if px changed update the int_px_level values
        int diff = this_smv_.DblPxDiffSign(
            t_price_, this_smv_.market_update_info_.asklevels_[(t_level_changed_ - 1)].limit_price_);
        if (diff != 0) {
          /// missed an Add
          if (diff == -1) {
            /// insert new level on top
            if (t_level_changed_ == 1) {
              // if ( dbglogger_.CheckLoggingLevel ( BOOK_TEST ) )
              //   {
              //     dbglogger_ << watch_.tv() << ' ' << typeid( *this ).name() << ':' << __func__ << ":" << " AA will
              //     fix crap by adding on top \n"; dbglogger_.CheckToFlushBuffer( );
              //   }

              OnPriceLevelNew(t_security_id_, t_buysell_, t_level_changed_, t_price_, t_new_size_, t_new_ordercount_,
                              t_is_intermediate_message_);
              return;
            } else {
              int i = 0;
              bool found = false;
              for (i = t_level_changed_ - 2; i >= 0; i--) {
                if (this_smv_.DblPxDiffSign(t_price_, this_smv_.market_update_info_.asklevels_[i].limit_price_) == 0) {
                  found = true;
                  break;
                } else if (this_smv_.DblPxDiffSign(t_price_,
                                                   this_smv_.market_update_info_.asklevels_[i].limit_price_) == 1) {
                  break;
                }
              }
              if (i >= 0 && found) {
                // if ( dbglogger_.CheckLoggingLevel ( BOOK_TEST ) )
                // 	{
                // 	  dbglogger_ << watch_.tv() << ' ' << typeid( *this ).name() << ':' << __func__ << ":" << " AA
                // found correct level " << i << "\n"; dbglogger_.CheckToFlushBuffer( );
                // 	}
                this_level_ = &(this_smv_.market_update_info_.asklevels_[i]);
                //			      index_to_update_ = i+1;

              } else if (i >= 0) {
                // if ( dbglogger_.CheckLoggingLevel ( BOOK_TEST ) )
                // 	{
                // 	  dbglogger_ << watch_.tv() << ' ' << typeid( *this ).name() << ':' << __func__ << ":" << " AA
                // insert at correct level " << i << "\n"; dbglogger_.CheckToFlushBuffer( );
                // 	}
                OnPriceLevelNew(t_security_id_, t_buysell_, i + 2, t_price_, t_new_size_, t_new_ordercount_,
                                t_is_intermediate_message_);
                return;
              } else {
                // if ( dbglogger_.CheckLoggingLevel ( BOOK_TEST ) )
                // 	{
                // 	  dbglogger_ << watch_.tv() << ' ' << typeid( *this ).name() << ':' << __func__ << ":" << " AA
                // will ignore .. cannot fix \n"; dbglogger_.CheckToFlushBuffer( );
                // 	}
                return;
              }
            }
          } else {
            /// missed delete
            if (t_level_changed_ == 1) {
              // if ( dbglogger_.CheckLoggingLevel ( BOOK_TEST ) )
              //   {
              //     dbglogger_ << watch_.tv() << ' ' << typeid( *this ).name() << ':' << __func__ << ":" << " AD will
              //     fix crap by deleting on top \n"; dbglogger_.CheckToFlushBuffer( );
              //   }
              OnPriceLevelDelete(t_security_id_, t_buysell_, t_level_changed_,
                                 this_smv_.market_update_info_.asklevels_[(t_level_changed_ - 1)].limit_price_, true);
              this_level_ = &(this_smv_.market_update_info_.asklevels_[0]);
            } else {
              int i = 0;
              bool found = false;
              for (i = t_level_changed_; i < (int)this_smv_.market_update_info_.asklevels_.size(); i++) {
                if (this_smv_.DblPxDiffSign(t_price_, this_smv_.market_update_info_.asklevels_[i].limit_price_) == 0) {
                  found = true;
                  break;
                } else if (this_smv_.DblPxDiffSign(t_price_,
                                                   this_smv_.market_update_info_.asklevels_[i].limit_price_) == -1) {
                  break;
                }
              }
              if (i < (int)this_smv_.market_update_info_.asklevels_.size() && found) {
                // if ( dbglogger_.CheckLoggingLevel ( BOOK_TEST ) )
                // 	{
                // 	  dbglogger_ << watch_.tv() << ' ' << typeid( *this ).name() << ':' << __func__ << ":" << " AD
                // found correct level " << i << "\n"; dbglogger_.CheckToFlushBuffer( );
                // 	}
                this_level_ = &(this_smv_.market_update_info_.asklevels_[i]);
                //			      index_to_update_ = i+1;

              } else if (i < (int)this_smv_.market_update_info_.asklevels_.size()) {
                // if ( dbglogger_.CheckLoggingLevel ( BOOK_TEST ) )
                // 	{
                // 	  dbglogger_ << watch_.tv() << ' ' << typeid( *this ).name() << ':' << __func__ << ":" << " AD
                // insert at correct level " << i << "\n"; dbglogger_.CheckToFlushBuffer( );
                // 	}
                OnPriceLevelNew(t_security_id_, t_buysell_, i, t_price_, t_new_size_, t_new_ordercount_,
                                t_is_intermediate_message_);
                return;
              } else {
                // if ( dbglogger_.CheckLoggingLevel ( BOOK_TEST ) )
                // 	{
                // 	  dbglogger_ << watch_.tv() << ' ' << typeid( *this ).name() << ':' << __func__ << ":" << " AD
                // will ignore .. cannot fix \n"; dbglogger_.CheckToFlushBuffer( );
                // 	}
                return;
              }
            }
          }
        } else {
          this_level_ = &(this_smv_.market_update_info_.asklevels_[(t_level_changed_ - 1)]);
          if (this_smv_.pl_change_listeners_present_) {
            old_size_at_this_price = this_level_->limit_size_;
            old_ordercount_at_this_price = this_level_->limit_ordercount_;
          }
        }

        if (diff != 0) {
          this_level_->limit_int_price_ = this_smv_.GetIntPx(t_price_);
          this_level_->limit_price_ = t_price_;
        }

        update_best_variables_and_notify_ = true;
        if (t_level_changed_ == 1) {
          //                DBGLOG_TIME_CLASS_FUNC << " askChange: " << sid_to_askside_trade_price_ [ t_security_id_ ]
          //                  << " " << this_level_ -> limit_int_price_
          //                  << " " << t_new_size_
          //                  << " " << this_level_ -> limit_size_
          //                  << " " << sid_to_askside_trade_size_ [ t_security_id_ ]
          //                  << " " << sid_to_last_askside_trade_time_ [ t_security_id_ ]
          //                  << DBGLOG_ENDL_FLUSH;
          if (sid_to_askside_trade_price_[t_security_id_] == this_level_->limit_int_price_) {
            if (t_new_size_ < this_level_->limit_size_) {
              if (sid_to_askside_trade_size_[t_security_id_] == (this_level_->limit_size_ - t_new_size_) ||
                  watch_.tv() >=
                      sid_to_last_askside_trade_time_[t_security_id_] + ttime_t(SECONDS_TO_WAIT, MSECS_TO_WAIT)) {
                sid_to_highest_accumulated_askside_trade_size_[t_security_id_] = 0;
              }
              // size has been decreased since last
              if (sid_to_askside_trade_size_[t_security_id_] >= (this_level_->limit_size_ - t_new_size_) &&
                  watch_.tv() <
                      sid_to_last_askside_trade_time_[t_security_id_] + ttime_t(SECONDS_TO_WAIT, MSECS_TO_WAIT)) {
                sid_to_askside_trade_size_[t_security_id_] -= (this_level_->limit_size_ - t_new_size_);
                update_best_variables_and_notify_ = false;
              } else {
                sid_to_highest_accumulated_askside_trade_size_[t_security_id_] = 0;
                //  update_best_variables_and_notify_ = true;
              }

            } else {
              sid_to_highest_accumulated_askside_trade_size_[t_security_id_] = 0;
              sid_to_askside_trade_size_[t_security_id_] = 0;  // This means that the trade could not b reflected
              sid_to_last_askside_trade_time_[t_security_id_] = ttime_t(0, 0);
              // size increased from last time
              // there may be cases where notifying harms
              // update_best_variables_and_notify_ = true;
            }
          } else {
            sid_to_highest_accumulated_askside_trade_size_[t_security_id_] = 0;
            // the last trade price nad current best variables are not same
            // what might be the situation
            // update_best_variables_and_notify_ = true;
          }
        }

        this_level_->limit_size_ = t_new_size_;  // here Price Level BOOK is updated
        this_level_->limit_ordercount_ = t_new_ordercount_;
        this_level_->mod_time_ = watch_.tv();

        if (this_smv_.computing_price_levels()) {
          if (diff != 0) {
            this_level_->limit_int_price_level_ =
                this_level_->limit_int_price_ - this_smv_.market_update_info_.asklevels_.front().limit_int_price_;
            if (t_level_changed_ == 1) {
              for (unsigned int i = 1; i < this_smv_.market_update_info_.asklevels_.size(); i++) {
                this_smv_.market_update_info_.asklevels_[i].limit_int_price_level_ =
                    this_smv_.market_update_info_.asklevels_[i].limit_int_price_ - this_level_->limit_int_price_;
              }
            }
          }
        }

        if (t_level_changed_ == 1) {
          if (update_best_variables_and_notify_) {
            this_smv_.SetBestLevelAskVariablesOnQuote();  // TODO .. optimization : no need to assign bestbid_price_ and
                                                          // bestbid_int_price_ since they have not changed
          }
          if (update_best_variables_and_notify_) {
            if (!t_is_intermediate_message_) {
              // #ifdef EUREX_KEEP_UNCROSS_CODE
              // 		  // If it isn't an intermediate message
              // 		  // Uncross
              // 		  bool crossed = this_smv_.Uncross ( );
              // 		  if ( crossed )
              // 		    {
              // 		      this_smv_.SetBestLevelBidVariablesOnQuote ( );
              // 		      this_smv_.SetBestLevelAskVariablesOnQuote ( );
              // 		    }
              // #endif

              this_smv_.UpdateL1Prices();
              // For PL indicators we send the update irrespective of intermediate_message
              // On PL_change Notification
              // Pass OLd_size as well
              if (this_smv_.pl_change_listeners_present_) {
                this_smv_.NotifyOnPLChangeListeners(
                    t_security_id_, this_smv_.market_update_info_, kTradeTypeSell, t_level_changed_ - 1,
                    this_level_->limit_int_price_, this_level_->limit_int_price_level_, old_size_at_this_price,
                    t_new_size_, old_ordercount_at_this_price, t_new_ordercount_, t_is_intermediate_message_, 'C');
                // dbglogger_ << "NOTIF..L1SIZEUPDATE CHNAGE"<<'\n' ; dbglogger_ .DumpCurrentBuffer ();
              }
              if (!this_smv_.market_update_info_.bidlevels_.empty() &&
                  !this_smv_.market_update_info_.asklevels_.empty()) {
                this_smv_.NotifyL1SizeListeners();
              }
              // NotifyOnReadyListeners ( );
              // this_smv_.OnL1SizeUpdate( );
              this_smv_.l1_changed_since_last_ = false;
              if (this_smv_.l2_changed_since_last_) {
                // dbglogger_ << "NOTIF..L2UPDATE ONCHANGE "<<'\n' ; dbglogger_ .DumpCurrentBuffer ();
                this_smv_.Sanitize();
                this_smv_.OnL2Update();
                this_smv_.OnL2OnlyUpdate();
                this_smv_.l2_changed_since_last_ = false;
              }
              this_smv_.NotifyOnReadyListeners();
            } else {
              // For PL indicators we send the update irrespective of intermediate_message
              // On PL_change Notification
              // Pass OLd_size as well
              if (this_smv_.pl_change_listeners_present_) {
                this_smv_.NotifyOnPLChangeListeners(
                    t_security_id_, this_smv_.market_update_info_, kTradeTypeSell, t_level_changed_ - 1,
                    this_level_->limit_int_price_, this_level_->limit_int_price_level_, old_size_at_this_price,
                    t_new_size_, old_ordercount_at_this_price, t_new_ordercount_, t_is_intermediate_message_, 'C');
              }
              this_smv_.l1_changed_since_last_ = true;
            }
          }
        } else {
          if (!t_is_intermediate_message_) {
            if (this_smv_.l1_changed_since_last_) {
              // #ifdef EUREX_KEEP_UNCROSS_CODE
              // 		  // If it isn't an intermediate message
              // 		  // Uncross
              // 		  bool crossed = this_smv_.Uncross ( );
              // 		  if ( crossed )
              // 		    {
              // 		      this_smv_.SetBestLevelBidVariablesOnQuote ( );
              // 		      this_smv_.SetBestLevelAskVariablesOnQuote ( );
              // 		    }
              // #endif

              this_smv_.UpdateL1Prices();
              // For PL indicators we send the update irrespective of intermediate_message
              // On PL_change Notification
              // Pass OLd_size as well
              if (this_smv_.pl_change_listeners_present_) {
                this_smv_.NotifyOnPLChangeListeners(
                    t_security_id_, this_smv_.market_update_info_, kTradeTypeSell, t_level_changed_ - 1,
                    this_level_->limit_int_price_, this_level_->limit_int_price_level_, old_size_at_this_price,
                    t_new_size_, old_ordercount_at_this_price, t_new_ordercount_, t_is_intermediate_message_, 'C');
                // dbglogger_ << "NOTIF..L1SIZEUPDATE ONCHANGE SELL"<<'\n' ; dbglogger_ .DumpCurrentBuffer ();
              }
              if (!this_smv_.market_update_info_.bidlevels_.empty() &&
                  !this_smv_.market_update_info_.asklevels_.empty()) {
                this_smv_.NotifyL1SizeListeners();
              }
              // this_smv_.OnL1SizeUpdate( );
              this_smv_.l1_changed_since_last_ = false;
            } else {
              // For PL indicators we send the update irrespective of intermediate_message
              // On PL_change Notification
              // Pass OLd_size as well
              if (this_smv_.pl_change_listeners_present_) {
                this_smv_.NotifyOnPLChangeListeners(
                    t_security_id_, this_smv_.market_update_info_, kTradeTypeSell, t_level_changed_ - 1,
                    this_level_->limit_int_price_, this_level_->limit_int_price_level_, old_size_at_this_price,
                    t_new_size_, old_ordercount_at_this_price, t_new_ordercount_, t_is_intermediate_message_, 'C');
              }
            }
            // dbglogger_ << "NOTIF..L2 ONCHANGE SELL"<<'\n' ; dbglogger_ .DumpCurrentBuffer ();
            this_smv_.Sanitize();
            this_smv_.OnL2Update();
            this_smv_.OnL2OnlyUpdate();
            this_smv_.NotifyOnReadyListeners();
            this_smv_.l2_changed_since_last_ = false;
          } else  // Non-intermediate_message
          {
            if (this_smv_.pl_change_listeners_present_) {
              this_smv_.NotifyOnPLChangeListeners(
                  t_security_id_, this_smv_.market_update_info_, kTradeTypeSell, t_level_changed_ - 1,
                  this_level_->limit_int_price_, this_level_->limit_int_price_level_, old_size_at_this_price,
                  t_new_size_, old_ordercount_at_this_price, t_new_ordercount_, t_is_intermediate_message_, 'C');
            }
            this_smv_.l2_changed_since_last_ = true;
          }
        }

        // if ( dbglogger_.CheckLoggingLevel ( BOOK_TEST ) )
        // 	{
        // 	  dbglogger_ << watch_.tv() << ' ' << typeid( *this ).name() << ':' << __func__ << ":" << " Updating Ask
        // " << t_is_intermediate_message_ << "\n" << this_smv_.ShowMarket( ) << "\n" ; dbglogger_.CheckToFlushBuffer(
        // );
        // 	}
      }
    } break;
    default: {}
  }
  // std::cerr << " Packet Change: : " << t_security_id_ << " " << t_level_changed_ << " " << t_price_ << " " <<
  // t_new_size_ << " " << t_new_ordercount_  << this_smv_.ShowMarket() ; //debuginfo
}

void OSEPriceFeedMarketViewManager::OnTrade(const unsigned int t_security_id_, const double t_trade_price_,
                                            const int t_trade_size_, const TradeType_t t_buysell_) {
  SecurityMarketView& this_smv_ = *(security_market_view_map_[t_security_id_]);
  int int_trade_price_ = this_smv_.GetIntPx(t_trade_price_);
  if (t_buysell_ == kTradeTypeNoInfo) {
    if (this_smv_.market_update_info_.asklevels_.size() > 0 &&
        (t_trade_price_ >= this_smv_.market_update_info_.asklevels_[0].limit_price_ - 0.0001)) {
      this_smv_.trade_print_info_.buysell_ = kTradeTypeBuy;
    } else if (this_smv_.market_update_info_.bidlevels_.size() > 0 &&
               (t_trade_price_ <= this_smv_.market_update_info_.bidlevels_[0].limit_price_ + 0.0001)) {
      this_smv_.trade_print_info_.buysell_ = kTradeTypeSell;
    } else {
      this_smv_.trade_print_info_.buysell_ = kTradeTypeNoInfo;
    }
    // estimate the trade types:
  }
  if (this_smv_.trade_print_info_.buysell_ == kTradeTypeSell) {
    if (this_smv_.market_update_info_.bestbid_int_price_ - int_trade_price_ >= MAX_TRADE_TICK_DIFFERENCE) {
      // this will happen when we get random trade prices in ose
      return;
    }
    if (sid_to_bidside_trade_price_[t_security_id_] == int_trade_price_) {
      if ((this_smv_.last_message_was_trade_ ||
           watch_.tv() < sid_to_last_bidside_trade_time_[t_security_id_] + ttime_t(SECONDS_TO_WAIT, MSECS_TO_WAIT))) {
        // either the last message was quote or the last reeived trade was within timebound( i.e both were part of same
        // trade)
        sid_to_bidside_trade_size_[t_security_id_] += t_trade_size_;
        sid_to_last_bidside_trade_time_[t_security_id_] = watch_.tv();
      } else {
        sid_to_bidside_trade_size_[t_security_id_] = t_trade_size_;
        sid_to_last_bidside_trade_time_[t_security_id_] = watch_.tv();
      }
      sid_to_highest_accumulated_bidside_trade_size_[t_security_id_] += t_trade_size_;
    } else {
      sid_to_bidside_trade_size_[t_security_id_] = t_trade_size_;
      sid_to_last_bidside_trade_time_[t_security_id_] = watch_.tv();
      sid_to_bidside_trade_price_[t_security_id_] = int_trade_price_;
      sid_to_highest_accumulated_bidside_trade_size_[t_security_id_] = t_trade_size_;
    }

    //      DBGLOG_TIME_CLASS_FUNC
    //        << "BID: "<<  sid_to_bidside_trade_size_ [ t_security_id_ ]
    //        << " " << sid_to_bidside_trade_price_ [ t_security_id_ ]
    //        << " "<< sid_to_last_bidside_trade_time_ [ t_security_id_ ]
    //        << " " << sid_to_highest_accumulated_bidside_trade_size_ [ t_security_id_ ]
    //        << DBGLOG_ENDL_FLUSH;

    if (int_trade_price_ > this_smv_.market_update_info_.bidlevels_[0].limit_int_price_) {
      // improve trade
      this_smv_.market_update_info_.bestbid_price_ = this_smv_.market_update_info_.bidlevels_[0].limit_price_;
      this_smv_.market_update_info_.bestbid_price_ = this_smv_.market_update_info_.bidlevels_[0].limit_int_price_;
      this_smv_.market_update_info_.bestbid_size_ = this_smv_.market_update_info_.bidlevels_[0].limit_size_;
      this_smv_.market_update_info_.bestbid_ordercount_ = this_smv_.market_update_info_.bidlevels_[0].limit_ordercount_;
    }

    for (unsigned i = 0; i < this_smv_.market_update_info_.bidlevels_.size(); i++) {
      if (int_trade_price_ == this_smv_.market_update_info_.bidlevels_[i].limit_int_price_) {
        if (this_smv_.market_update_info_.bidlevels_[i].limit_size_ >
            sid_to_highest_accumulated_bidside_trade_size_[t_security_id_]) {
          this_smv_.market_update_info_.bestbid_price_ = this_smv_.market_update_info_.bidlevels_[i].limit_price_;
          this_smv_.market_update_info_.bestbid_int_price_ =
              this_smv_.market_update_info_.bidlevels_[i].limit_int_price_;
          this_smv_.market_update_info_.bestbid_size_ = this_smv_.market_update_info_.bidlevels_[i].limit_size_ -
                                                        sid_to_highest_accumulated_bidside_trade_size_[t_security_id_];
          this_smv_.market_update_info_.bestbid_ordercount_ =
              this_smv_.market_update_info_.bidlevels_[i].limit_ordercount_;
        } else {
          if ((i + 1) == this_smv_.market_update_info_.bidlevels_.size()) {
            // the side got cleared set the price some ticks below the last trade price
            // size ?? setting it to 1
            this_smv_.market_update_info_.bestbid_int_price_ = int_trade_price_ - 1;
            this_smv_.market_update_info_.bestbid_price_ =
                this_smv_.GetDoublePx(this_smv_.market_update_info_.bestbid_int_price_);
            this_smv_.market_update_info_.bestbid_size_ = 1;
            this_smv_.market_update_info_.bestbid_ordercount_ = 1;  // can it be 0?
          } else {
            // set it to next level
            this_smv_.market_update_info_.bestbid_price_ = this_smv_.market_update_info_.bidlevels_[i + 1].limit_price_;
            this_smv_.market_update_info_.bestbid_int_price_ =
                this_smv_.market_update_info_.bidlevels_[i + 1].limit_int_price_;
            this_smv_.market_update_info_.bestbid_size_ = this_smv_.market_update_info_.bidlevels_[i + 1].limit_size_;
            this_smv_.market_update_info_.bestbid_ordercount_ =
                this_smv_.market_update_info_.bidlevels_[i + 1].limit_ordercount_;
          }
        }
      }
    }
  }

  if (this_smv_.trade_print_info_.buysell_ == kTradeTypeBuy) {
    if (int_trade_price_ - this_smv_.market_update_info_.bestask_int_price_ >= MAX_TRADE_TICK_DIFFERENCE) {
      // this will happen when we get random trade prices in ose
      return;
    }

    if (sid_to_askside_trade_price_[t_security_id_] == int_trade_price_) {
      if (this_smv_.last_message_was_trade_ ||
          watch_.tv() < sid_to_last_askside_trade_time_[t_security_id_] + ttime_t(SECONDS_TO_WAIT, MSECS_TO_WAIT)) {
        sid_to_askside_trade_size_[t_security_id_] += t_trade_size_;
        sid_to_last_askside_trade_time_[t_security_id_] = watch_.tv();
      } else {
        sid_to_askside_trade_size_[t_security_id_] = t_trade_size_;
        sid_to_last_askside_trade_time_[t_security_id_] = watch_.tv();
      }
      sid_to_highest_accumulated_askside_trade_size_[t_security_id_] += t_trade_size_;
    } else {
      sid_to_askside_trade_price_[t_security_id_] = int_trade_price_;
      sid_to_askside_trade_size_[t_security_id_] = t_trade_size_;
      sid_to_last_askside_trade_time_[t_security_id_] = watch_.tv();
      sid_to_highest_accumulated_askside_trade_size_[t_security_id_] = t_trade_size_;
    }

    //      DBGLOG_TIME_CLASS_FUNC
    //        << "This: " <<  sid_to_askside_trade_size_ [ t_security_id_ ]
    //        << " " << sid_to_askside_trade_price_ [ t_security_id_ ]
    //        << " "<< sid_to_last_askside_trade_time_ [ t_security_id_ ]
    //        << " " << sid_to_highest_accumulated_askside_trade_size_ [ t_security_id_ ]
    //        << DBGLOG_ENDL_FLUSH;

    if (int_trade_price_ < this_smv_.market_update_info_.asklevels_[0].limit_int_price_) {
      // improve trade
      this_smv_.market_update_info_.bestask_price_ = this_smv_.market_update_info_.asklevels_[0].limit_price_;
      this_smv_.market_update_info_.bestask_int_price_ = this_smv_.market_update_info_.asklevels_[0].limit_int_price_;
      this_smv_.market_update_info_.bestask_size_ = this_smv_.market_update_info_.asklevels_[0].limit_size_;
      this_smv_.market_update_info_.bestask_ordercount_ = this_smv_.market_update_info_.asklevels_[0].limit_ordercount_;
    }

    for (unsigned i = 0; i < this_smv_.market_update_info_.asklevels_.size(); i++) {
      if (int_trade_price_ == this_smv_.market_update_info_.asklevels_[i].limit_int_price_) {
        if (this_smv_.market_update_info_.asklevels_[i].limit_size_ >
            sid_to_highest_accumulated_askside_trade_size_[t_security_id_]) {
          this_smv_.market_update_info_.bestask_price_ = this_smv_.market_update_info_.asklevels_[i].limit_price_;
          this_smv_.market_update_info_.bestask_int_price_ =
              this_smv_.market_update_info_.asklevels_[i].limit_int_price_;
          this_smv_.market_update_info_.bestask_size_ = this_smv_.market_update_info_.asklevels_[i].limit_size_ -
                                                        sid_to_highest_accumulated_askside_trade_size_[t_security_id_];
          this_smv_.market_update_info_.bestask_ordercount_ =
              this_smv_.market_update_info_.asklevels_[i].limit_ordercount_;  // would have decreased
        } else {
          if ((i + 1) >= this_smv_.market_update_info_.asklevels_.size()) {
            // this side got cleared
            this_smv_.market_update_info_.bestask_int_price_ = int_trade_price_ + 1;
            this_smv_.market_update_info_.bestask_price_ =
                this_smv_.GetDoublePx(this_smv_.market_update_info_.bestbid_int_price_);
            this_smv_.market_update_info_.bestask_size_ = 1;
            this_smv_.market_update_info_.bestask_ordercount_ = 1;
          } else {
            this_smv_.market_update_info_.bestask_price_ = this_smv_.market_update_info_.asklevels_[i + 1].limit_price_;
            this_smv_.market_update_info_.bestask_int_price_ =
                this_smv_.market_update_info_.asklevels_[i + 1].limit_int_price_;
            this_smv_.market_update_info_.bestask_size_ = this_smv_.market_update_info_.asklevels_[i + 1].limit_size_;
            this_smv_.market_update_info_.bestask_ordercount_ =
                this_smv_.market_update_info_.asklevels_[i + 1].limit_ordercount_;
          }
        }
        break;
      }
    }
  }

  this_smv_.trade_print_info_.size_traded_ = t_trade_size_;
  this_smv_.trade_print_info_.trade_price_ = t_trade_price_;
  this_smv_.trade_print_info_.int_trade_price_ = int_trade_price_;
  this_smv_.SetTradeVarsForIndicatorsIfRequired();
  if ((this_smv_.market_update_info_.bidlevels_.size() > 0) && (this_smv_.market_update_info_.asklevels_.size() > 0)) {
    // why are we notifying on only best level trade prices???????
    //	security_market_view_map_[ t_security_id_]->OnTrade ( t_trade_price_, t_trade_size_, t_buysell_ ) ;
    security_market_view_map_[t_security_id_]->UpdateL1Prices();
    security_market_view_map_[t_security_id_]->NotifyTradeListeners();
    security_market_view_map_[t_security_id_]->NotifyOnReadyListeners();
  }
  this_smv_.last_message_was_trade_ = true;
}
}
