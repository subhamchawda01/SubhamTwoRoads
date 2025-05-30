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
#include "baseinfra/MarketAdapter/ose_l1_price_market_view_manager.hpp"

namespace HFSAT {

OSEL1PriceMarketViewManager::OSEL1PriceMarketViewManager(
    DebugLogger& t_dbglogger_, const Watch& t_watch_, const SecurityNameIndexer& t_sec_name_indexer_,
    const std::vector<SecurityMarketView*>& t_security_market_view_map_)
    : BaseMarketViewManager(t_dbglogger_, t_watch_, t_sec_name_indexer_, t_security_market_view_map_),
      sid_to_prom_order_manager_(t_security_market_view_map_.size(), NULL) {
  for (auto i = 0u; i < security_market_view_map_.size(); i++) {
    SecurityMarketView* p_this_smv_ = t_security_market_view_map_[i];

    if ((p_this_smv_ != NULL) && (p_this_smv_->remove_self_orders_from_book())) {
      PromOrderManager* p_this_pom_ = PromOrderManager::GetCreatedInstance(p_this_smv_->shortcode());
      if (p_this_pom_ != NULL) {
        sid_to_prom_order_manager_[i] = p_this_pom_;
      } else {
        if (HFSAT::SecurityDefinitions::GetContractExchSource(p_this_smv_->shortcode(), t_watch_.YYYYMMDD()) !=
                HFSAT::kExchSourceTMX &&
            dbglogger_.CheckLoggingLevel(SMVSELF_ERROR)) {
          DBGLOG_TIME_CLASS_FUNC << " NULL PromOrderManager for a security ( " << p_this_smv_->shortcode()
                                 << " ) whose remove_self_orders_from_book() was true"
                                 << DBGLOG_ENDL_FLUSH;  // p_this_pom_ will be so if the constructor of PLMVM is called
                                                        // before PromOrderManager is initialized
        }
      }
    }
  }
}

void OSEL1PriceMarketViewManager::OnL1Change(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                             const double t_price_, const int t_new_size_, const int t_new_ordercount_,
                                             const bool t_is_intermediate_message_) {
  if ((int)(t_price_) <= 0 || (int)(t_price_) > 999999) return;

  SecurityMarketView& this_smv_ = *(security_market_view_map_[t_security_id_]);

  // int t_int_price_ =  t_price_  ;

  switch (t_buysell_) {
    case kTradeTypeBuy: {
      MarketUpdateInfoLevelStruct new_level_(0, this_smv_.GetIntPx(t_price_), t_price_, t_new_size_, t_new_ordercount_,
                                             watch_.tv());

      this_smv_.ReplaceTopBid(new_level_);
      this_smv_.SetBestLevelBidVariablesOnQuote();

      if (this_smv_.market_update_info_.bidlevels_.size() > 0 && this_smv_.market_update_info_.asklevels_.size() > 0) {
        if (this_smv_.market_update_info_.bidlevels_[0].limit_int_price_ >=
            this_smv_.market_update_info_.asklevels_[0].limit_int_price_) {
          return;
        }
      }

      if (!t_is_intermediate_message_) {
        // Uncross

        this_smv_.UpdateL1Prices();
        // Notify PL Indicatore listneres
        if (this_smv_.pl_change_listeners_present_) {
          this_smv_.NotifyOnPLChangeListeners(t_security_id_, this_smv_.market_update_info_, kTradeTypeBuy, 0,
                                              new_level_.limit_int_price_, new_level_.limit_int_price_level_, 0,
                                              t_new_size_, 0, t_new_ordercount_, t_is_intermediate_message_, 'N');
          // dbglogger_ << "NOTIF..L1SIZEUPDATE ONCHANGE NEW"<<'\n' ; dbglogger_ .DumpCurrentBuffer ();
        }
        if (!this_smv_.market_update_info_.bidlevels_.empty() && !this_smv_.market_update_info_.asklevels_.empty()) {
          this_smv_.NotifyL1SizeListeners();
        }
        // this_smv_.OnL1SizeUpdate ( );
        this_smv_.l1_changed_since_last_ = false;
        this_smv_.NotifyOnReadyListeners();
      } else {
        // Notify PL Indicatore listneres
        if (this_smv_.pl_change_listeners_present_) {
          this_smv_.NotifyOnPLChangeListeners(t_security_id_, this_smv_.market_update_info_, kTradeTypeBuy, 0,
                                              new_level_.limit_int_price_, new_level_.limit_int_price_level_, 0,
                                              t_new_size_, 0, t_new_ordercount_, t_is_intermediate_message_, 'N');
        }
        this_smv_.l1_changed_since_last_ = true;
      }

    } break;
    case kTradeTypeSell: {
      // since most new levels will be at top .. so special case for this ...
      // using O(1) PushFront not O(DEF_MARKET_DEPTH) PushAt
      // also needed to set bestlevel variables

      MarketUpdateInfoLevelStruct new_level_(0, this_smv_.GetIntPx(t_price_), t_price_, t_new_size_, t_new_ordercount_,
                                             watch_.tv());
      this_smv_.ReplaceTopAsk(new_level_);
      this_smv_.SetBestLevelAskVariablesOnQuote();

      if (this_smv_.market_update_info_.bidlevels_.size() > 0 && this_smv_.market_update_info_.asklevels_.size() > 0) {
        if (this_smv_.market_update_info_.bidlevels_[0].limit_int_price_ >=
            this_smv_.market_update_info_.asklevels_[0].limit_int_price_) {
          return;
        }
      }

      if (!t_is_intermediate_message_) {
        // Uncross

        this_smv_.UpdateL1Prices();

        if (this_smv_.pl_change_listeners_present_) {
          this_smv_.NotifyOnPLChangeListeners(t_security_id_, this_smv_.market_update_info_, kTradeTypeSell, 0,
                                              new_level_.limit_int_price_, new_level_.limit_int_price_level_, 0,
                                              t_new_size_, 0, t_new_ordercount_, t_is_intermediate_message_, 'N');
          // dbglogger_ << "NOTIF..L1SIZEUPDATE NEW SELL "<<'\n' ; dbglogger_ .DumpCurrentBuffer ();
        }

        if (!this_smv_.market_update_info_.bidlevels_.empty() && !this_smv_.market_update_info_.asklevels_.empty()) {
          this_smv_.NotifyL1SizeListeners();
        }
        // this_smv_.OnL1SizeUpdate( );
        this_smv_.l1_changed_since_last_ = false;
        this_smv_.NotifyOnReadyListeners();
      } else  // Non-intermediate msg
      {
        if (this_smv_.pl_change_listeners_present_) {
          this_smv_.NotifyOnPLChangeListeners(t_security_id_, this_smv_.market_update_info_, kTradeTypeSell, 0,
                                              new_level_.limit_int_price_, new_level_.limit_int_price_level_, 0,
                                              t_new_size_, 0, t_new_ordercount_, t_is_intermediate_message_, 'N');
        }
        this_smv_.l1_changed_since_last_ = true;
      }

      // if ( dbglogger_.CheckLoggingLevel ( BOOK_TEST ) )
      //    {
      //      DBGLOG_TIME_CLASS_FUNC << " AddTopAsk " << t_is_intermediate_message_ << "\n" << this_smv_.ShowMarket( )
      //      << DBGLOG_ENDL_FLUSH ;
      //    }
    } break;
    default: { break; }
  }
  //  std::cerr << " Packet new: " << t_security_id_ << " " << t_level_added_ << " " << t_price_ << " " << t_new_size_
  //  << " " << t_new_ordercount_  << this_smv_.ShowMarket() ;//debuginfo
}

void OSEL1PriceMarketViewManager::OnTrade(const unsigned int t_security_id_, const double t_trade_price_,
                                          const int t_trade_size_, const TradeType_t t_buysell_) {
  SecurityMarketView& this_smv_ = *(security_market_view_map_[t_security_id_]);

  if ((int)(t_trade_price_) <= 0 || (int)(t_trade_price_) > 999999) return;

  if (this_smv_.market_update_info_.bidlevels_.size() > 0 && this_smv_.market_update_info_.asklevels_.size() > 0) {
    if (this_smv_.market_update_info_.bidlevels_[0].limit_int_price_ >=
        this_smv_.market_update_info_.asklevels_[0].limit_int_price_) {
      return;
    }
  }

  // SecurityMarketView & this_smv_ = * ( security_market_view_map_[ t_security_id_] );
  security_market_view_map_[t_security_id_]->OnTrade(t_trade_price_, t_trade_size_, t_buysell_);
}
}
