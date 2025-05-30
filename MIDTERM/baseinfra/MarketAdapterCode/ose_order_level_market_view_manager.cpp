/**
   \file MarketAdapterCode/ose_order_level_market_view_manager.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   Phone: +91 80 4190 3551
*/
#include "baseinfra/OrderRouting/prom_order_manager.hpp"
#include "baseinfra/MarketAdapter/ose_order_level_market_view_manager.hpp"

namespace HFSAT {

OSEOrderLevelMarketViewManager::OSEOrderLevelMarketViewManager(
    DebugLogger &t_dbglogger_, const Watch &t_watch_, const SecurityNameIndexer &t_sec_name_indexer_,
    const std::vector<SecurityMarketView *> &t_security_market_view_map_)
    : BaseMarketViewManager(t_dbglogger_, t_watch_, t_sec_name_indexer_, t_security_market_view_map_),
      osemaporder_mempool_(100),
      security_id_to_ose_omv_map_(),
      trade_time_manager_(HFSAT::TradeTimeManager::GetUniqueInstance(
          const_cast<SecurityNameIndexer &>(t_sec_name_indexer_), t_watch_.YYYYMMDD()))

{
  security_id_to_ose_omv_map_.resize(security_market_view_map_.size());

  for (auto i = 0u; i < security_market_view_map_.size(); i++) {
    SecurityMarketView *p_this_smv_ = t_security_market_view_map_[i];

    // if remove_self_orders_from_book is true then we need to listen to prom_order_manager as well
    if ((p_this_smv_ != NULL) && (p_this_smv_->remove_self_orders_from_book())) {
      PromOrderManager *p_this_pom_ = PromOrderManager::GetCreatedInstance(p_this_smv_->shortcode());
      if (p_this_pom_ != NULL) {
        p_this_pom_->AddGlobalOrderChangeListener(this);
      } else {
        DBGLOG_TIME_CLASS_FUNC_LINE << "NULL PromOrderManager for a security ( " << p_this_smv_->shortcode()
                                    << " ) whose remove_self_orders_from_book() was true"
                                    << DBGLOG_ENDL_FLUSH;  // p_this_pom_ will be so if the constructor of PLMVM is
                                                           // called before PromOrderManager is initialized
      }
    }
  }
}

void OSEOrderLevelMarketViewManager::SetTimeToSkipUntilFirstEvent(const ttime_t r_start_time_) {
  for (auto i = 0u; i < security_market_view_map_.size(); i++) {
    SecurityMarketView *p_this_smv_ = security_market_view_map_[i];
    p_this_smv_->set_skip_listener_notification_end_time(r_start_time_);
  }
}

void OSEOrderLevelMarketViewManager::OnOrderLevelNew(const unsigned int t_security_id_, uint64_t t_order_id_,
                                                     const TradeType_t t_buysell_, const int t_level_added_,
                                                     const int t_price_, const int t_new_size_,
                                                     const bool t_is_intermediate_message_) {
  if (t_level_added_ > MAX_OSE_ORDER_BOOK_DEPTH) return;
  if (t_price_ == 0) return;

  if (dbglogger_.CheckLoggingLevel(BOOK_TEST)) {
    DBGLOG_TIME_CLASS_FUNC_LINE << "( secid= " << t_security_id_ << " orderid= " << t_order_id_
                                << " bs= " << GetTradeTypeChar(t_buysell_) << " lvl= " << t_level_added_
                                << " px= " << t_price_ << " sz= " << t_new_size_
                                << " IsInter= " << (t_is_intermediate_message_ ? 'Y' : 'N') << " )"
                                << DBGLOG_ENDL_FLUSH;
  }

  SecurityMarketView &this_smv_ =
      *(security_market_view_map_[t_security_id_]);  // CRASH_ALERT could crash if garbage t_security_id_ come in
  OSEOrderMarketView &this_omv_ = security_id_to_ose_omv_map_[t_security_id_];
  // DBGLOG_TIME_CLASS_FUNC<<"after check\n";
  // dbglogger_.DumpCurrentBuffer();
  //  DBGLOG_TIME_CLASS_FUNC << "Order depth book:" << DBGLOG_ENDL_FLUSH ;
  //  DBGLOG_TIME_CLASS_FUNC << ShowOrderDepthBook ( this_omv_ ) << DBGLOG_ENDL_FLUSH ;

  OSEMapOrder *p_ose_map_order_ = osemaporder_mempool_.Alloc();

  if (!p_ose_map_order_) {
    p_ose_map_order_ = (OSEMapOrder *)malloc(sizeof(OSEMapOrder));
  }
  int t_int_price_ = this_smv_.GetIntPx(t_price_);
  p_ose_map_order_->set(t_order_id_, t_int_price_, t_price_, t_new_size_, t_buysell_);

  int level_to_add_ = 1;

  switch (t_buysell_) {
    case kTradeTypeBuy: {
      // Compute the level to add this order to in the security market view.
      for (level_to_add_ = 1;
           level_to_add_ <= (int)this_smv_.market_update_info_.bidlevels_.size() &&
           t_int_price_ < this_smv_.market_update_info_.bidlevels_[level_to_add_ - 1].limit_int_price_;
           ++level_to_add_)
        ;

      // create structure

      if (level_to_add_ > (int)this_smv_.market_update_info_.bidlevels_.size() ||
          t_int_price_ != this_smv_.market_update_info_.bidlevels_[level_to_add_ - 1].limit_int_price_) {
        //==========================	==== Fetching orders from order depth book, SMV might have lost on sanitize
        // with price level wipeout ================== @ravi//
        // TODO_OPT should be optimized -- only using in hitorical mode for now
        // TODO start with a new level and search backwards untill price is different
        std::vector<OSEMapOrder *>::iterator _itr_ = this_omv_.bid_order_depth_book_.begin();

        int total_bid_size_ = 0;
        int total_bid_order_count_ = 0;

        for (_itr_ = this_omv_.bid_order_depth_book_.begin(); _itr_ != this_omv_.bid_order_depth_book_.end(); _itr_++) {
          if ((*_itr_) && (*_itr_)->int_price_ == t_int_price_) {
            total_bid_size_ += (*_itr_)->size_;
            total_bid_order_count_++;
          }

          if ((*_itr_) && (*_itr_)->int_price_ > t_int_price_) break;
        }
        //============================================================================================================================//

        MarketUpdateInfoLevelStruct new_level_(0, t_int_price_, t_price_, t_new_size_ + total_bid_size_,
                                               1 + total_bid_order_count_, watch_.tv());

        // Add a new level as computed previously to the smv.
        if (level_to_add_ == 1) {
          this_smv_.AddTopBid(new_level_);  // Efficient?

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
            this_smv_.UpdateL1Prices();
            // dbglogger_<<"the time in watch "<<watch_.tv().ToString() << "\n";
            // dbglogger_.DumpCurrentBuffer();
            if (IsNormalTradeTime(t_security_id_, watch_.tv())) this_smv_.NotifyL1PriceListeners();

            if (this_smv_.pl_change_listeners_present_) {
              if (IsNormalTradeTime(t_security_id_, watch_.tv()))
                this_smv_.NotifyOnPLChangeListeners(t_security_id_, this_smv_.market_update_info_, kTradeTypeBuy,
                                                    level_to_add_ - 1, new_level_.limit_int_price_,
                                                    new_level_.limit_int_price_level_, 0, new_level_.limit_size_, 0,
                                                    new_level_.limit_ordercount_, t_is_intermediate_message_, 'N');
            }
            if (IsNormalTradeTime(t_security_id_, watch_.tv())) this_smv_.OnL1SizeUpdate();
            //		      this_smv_.l1_changed_since_last_ = false;
            this_smv_.l1_changed_since_last_ = true;
            this_smv_.l2_changed_since_last_ = true;
            if (this_smv_.l2_changed_since_last_) {
              // Since a new price level on bid side has come
              // We trust this latest message and make sure that all other prices
              // lower than this on ask side must be depricated
              this_smv_.SanitizeAskSideWithPrice(t_int_price_);
              this_smv_.Sanitize();  // Sanitize both levels again for duplicates etc
              if (IsNormalTradeTime(t_security_id_, watch_.tv())) this_smv_.OnL2Update();
              // this_smv_.l2_changed_since_last_ = false;
            }
          } else {
            if (this_smv_.pl_change_listeners_present_) {
              if (IsNormalTradeTime(t_security_id_, watch_.tv()))
                this_smv_.NotifyOnPLChangeListeners(t_security_id_, this_smv_.market_update_info_, kTradeTypeBuy,
                                                    level_to_add_ - 1, new_level_.limit_int_price_,
                                                    new_level_.limit_int_price_level_, 0, new_level_.limit_size_, 0,
                                                    new_level_.limit_ordercount_, t_is_intermediate_message_, 'N');
            }

            // dbglogger_ << " Intermediate Msg anitizing...L2.."<< DBGLOG_ENDL_FLUSH ;
            // TODO {} it never comes here for ose since all messages are
            // non-intermediate
            this_smv_.l1_changed_since_last_ = true;
          }

          if (dbglogger_.CheckLoggingLevel(BOOK_TEST)) {
            DBGLOG_TIME_CLASS_FUNC << " AddTopBid level:" << level_to_add_ << DBGLOG_ENDL_FLUSH;
            DBGLOG_TIME << this_smv_.ShowMarket() << DBGLOG_ENDL_FLUSH;
          }

        } else {
          if (level_to_add_ > (int)this_smv_.market_update_info_.bidlevels_.size() + 1) return;

          if (this_smv_.computing_price_levels()) {
            new_level_.limit_int_price_level_ =
                this_smv_.market_update_info_.bidlevels_[0].limit_int_price_ - new_level_.limit_int_price_;
          }

          this_smv_.AddNonTopBid(level_to_add_, new_level_);

          if (!t_is_intermediate_message_) {
            if (this_smv_.l1_changed_since_last_) {
              if (this_smv_.pl_change_listeners_present_) {
                if (IsNormalTradeTime(t_security_id_, watch_.tv()))
                  this_smv_.NotifyOnPLChangeListeners(t_security_id_, this_smv_.market_update_info_, kTradeTypeBuy,
                                                      level_to_add_ - 1, new_level_.limit_int_price_,
                                                      new_level_.limit_int_price_level_, 0, new_level_.limit_size_, 0,
                                                      new_level_.limit_ordercount_, t_is_intermediate_message_, 'N');
              }

              this_smv_.UpdateL1Prices();
              if (IsNormalTradeTime(t_security_id_, watch_.tv())) this_smv_.NotifyL1PriceListeners();
              this_smv_.OnL1SizeUpdate();
              //  this_smv_.l1_changed_since_last_ = false;
            }
            this_smv_.Sanitize();  // Sanitize both levels again  for duplicates etc
            if (IsNormalTradeTime(t_security_id_, watch_.tv())) this_smv_.OnL2Update();
            // this_smv_.l2_changed_since_last_ = false;
            this_smv_.l2_changed_since_last_ = true;  // so that we sanitize
          } else {
            if (this_smv_.pl_change_listeners_present_) {
              if (IsNormalTradeTime(t_security_id_, watch_.tv()))
                this_smv_.NotifyOnPLChangeListeners(t_security_id_, this_smv_.market_update_info_, kTradeTypeBuy,
                                                    level_to_add_ - 1, new_level_.limit_int_price_,
                                                    new_level_.limit_int_price_level_, 0, new_level_.limit_size_, 0,
                                                    new_level_.limit_ordercount_, t_is_intermediate_message_, 'N');
            }
            // TODO {} since all messages are non-intermediate this will never come here w
            // which in turn will never call OnL2Update
            // this_smv_.l2_changed_since_last_ = true;
          }

          if (dbglogger_.CheckLoggingLevel(BOOK_TEST)) {
            DBGLOG_TIME_CLASS_FUNC << "AddNonTopBid level:" << level_to_add_ << DBGLOG_ENDL_FLUSH;
            DBGLOG_TIME_CLASS_FUNC << this_smv_.ShowMarket() << DBGLOG_ENDL_FLUSH;
          }
        }
      } else {
        // Price level already exists in the smv.
        MarketUpdateInfoLevelStruct *this_level_ = &(this_smv_.market_update_info_.bidlevels_[level_to_add_ - 1]);

        this_level_->limit_size_ += t_new_size_;
        ++(this_level_->limit_ordercount_);

        if (level_to_add_ == 1) {
          this_smv_.SetBestLevelBidVariablesOnQuote();
          if (!t_is_intermediate_message_) {
            if (this_smv_.pl_change_listeners_present_) {
              if (IsNormalTradeTime(t_security_id_, watch_.tv()))
                this_smv_.NotifyOnPLChangeListeners(t_security_id_, this_smv_.market_update_info_, kTradeTypeBuy,
                                                    level_to_add_ - 1, this_level_->limit_int_price_,
                                                    this_level_->limit_int_price_level_,
                                                    this_level_->limit_size_ - t_new_size_, this_level_->limit_size_,
                                                    this_level_->limit_ordercount_ - 1, this_level_->limit_ordercount_,
                                                    t_is_intermediate_message_, 'C');
            }

            this_smv_.UpdateL1Prices();
            if (IsNormalTradeTime(t_security_id_, watch_.tv())) this_smv_.NotifyL1PriceListeners();
            if (IsNormalTradeTime(t_security_id_, watch_.tv())) this_smv_.OnL1SizeUpdate();
            //   this_smv_.l1_changed_since_last_ = false;
            this_smv_.l2_changed_since_last_ = true;
            if (this_smv_.l2_changed_since_last_) {
              this_smv_.SanitizeAskSideWithPrice(t_int_price_);
              this_smv_.Sanitize();  // Sanitize both levels again  for duplicates etc
              if (IsNormalTradeTime(t_security_id_, watch_.tv())) this_smv_.OnL2Update();
              //	  this_smv_.l2_changed_since_last_ = false;
            }
          } else {
            if (this_smv_.pl_change_listeners_present_) {
              if (IsNormalTradeTime(t_security_id_, watch_.tv()))
                this_smv_.NotifyOnPLChangeListeners(t_security_id_, this_smv_.market_update_info_, kTradeTypeBuy,
                                                    level_to_add_ - 1, this_level_->limit_int_price_,
                                                    this_level_->limit_int_price_level_,
                                                    this_level_->limit_size_ - t_new_size_, this_level_->limit_size_,
                                                    this_level_->limit_ordercount_ - 1, this_level_->limit_ordercount_,
                                                    t_is_intermediate_message_, 'C');
            }

            this_smv_.l1_changed_since_last_ = true;
          }
        } else {
          if (!t_is_intermediate_message_) {
            if (this_smv_.l1_changed_since_last_) {
              if (this_smv_.pl_change_listeners_present_) {
                if (IsNormalTradeTime(t_security_id_, watch_.tv()))
                  this_smv_.NotifyOnPLChangeListeners(t_security_id_, this_smv_.market_update_info_, kTradeTypeBuy,
                                                      level_to_add_ - 1, this_level_->limit_int_price_,
                                                      this_level_->limit_int_price_level_,
                                                      this_level_->limit_size_ - t_new_size_, this_level_->limit_size_,
                                                      this_level_->limit_ordercount_ - 1,
                                                      this_level_->limit_ordercount_, t_is_intermediate_message_, 'C');
              }

              this_smv_.UpdateL1Prices();
              if (IsNormalTradeTime(t_security_id_, watch_.tv())) this_smv_.NotifyL1PriceListeners();
              if (IsNormalTradeTime(t_security_id_, watch_.tv())) this_smv_.OnL1SizeUpdate();
              //	  this_smv_.l1_changed_since_last_ = false;
            }
            // this_smv_.SanitizeAskSideWithPrice ( t_int_price_ );
            this_smv_.Sanitize();  // Sanitize both levels again  for duplicates etc
            if (IsNormalTradeTime(t_security_id_, watch_.tv())) this_smv_.OnL2Update();
            // this_smv_.l2_changed_since_last_ = false;
          } else {
            if (this_smv_.pl_change_listeners_present_) {
              if (IsNormalTradeTime(t_security_id_, watch_.tv()))
                this_smv_.NotifyOnPLChangeListeners(t_security_id_, this_smv_.market_update_info_, kTradeTypeBuy,
                                                    level_to_add_ - 1, this_level_->limit_int_price_,
                                                    this_level_->limit_int_price_level_,
                                                    this_level_->limit_size_ - t_new_size_, this_level_->limit_size_,
                                                    this_level_->limit_ordercount_ - 1, this_level_->limit_ordercount_,
                                                    t_is_intermediate_message_, 'C');
            }

            this_smv_.l2_changed_since_last_ = true;
          }
        }

        if (dbglogger_.CheckLoggingLevel(BOOK_TEST)) {
          DBGLOG_TIME_CLASS_FUNC << " Updating Bid level: " << level_to_add_ << DBGLOG_ENDL_FLUSH;
          DBGLOG_TIME_CLASS_FUNC << this_smv_.ShowMarket() << DBGLOG_ENDL_FLUSH;
        }
      }
    } break;

    case kTradeTypeSell: {
      // L1 OrderBook update
      for (level_to_add_ = 1;
           level_to_add_ <= (int)this_smv_.market_update_info_.asklevels_.size() &&
           t_int_price_ > this_smv_.market_update_info_.asklevels_[level_to_add_ - 1].limit_int_price_;
           ++level_to_add_)
        ;

      if (level_to_add_ > (int)this_smv_.market_update_info_.asklevels_.size() ||
          t_int_price_ != this_smv_.market_update_info_.asklevels_[level_to_add_ - 1].limit_int_price_) {
        // New price added. A new level will also be added.

        //============================== Fetching orders from order depth book, SMV might have lost on sanitize with
        // price level wipeout ====================//
        // TODO start with a new level and search backwards untill price is different

        std::vector<OSEMapOrder *>::iterator _itr_ = this_omv_.ask_order_depth_book_.begin();

        int total_ask_size_ = 0;
        int total_ask_order_count_ = 0;

        for (_itr_ = this_omv_.ask_order_depth_book_.begin(); _itr_ != this_omv_.ask_order_depth_book_.end(); _itr_++) {
          if ((*_itr_) && (*_itr_)->int_price_ == t_int_price_) {
            total_ask_size_ += (*_itr_)->size_;
            total_ask_order_count_++;
          }

          if ((*_itr_) && (*_itr_)->int_price_ < t_int_price_) break;
        }
        //============================================================================================================================//

        // create structure
        MarketUpdateInfoLevelStruct new_level_(0, t_int_price_, t_price_, t_new_size_ + total_ask_size_,
                                               1 + total_ask_order_count_, watch_.tv());

        // Add a new level as computed previously to the smv.
        if (level_to_add_ == 1) {
          this_smv_.AddTopAsk(new_level_);  // Efficient?

          if (this_smv_.computing_price_levels() &&
              this_smv_.market_update_info_.asklevels_.size() > 1) {  // update the levels of lower levels
            // another option is if limit_int_price_ is computed then this computation can be done at runtime
            int level_diff_top_two = this_smv_.market_update_info_.asklevels_[1].limit_int_price_ -
                                     this_smv_.market_update_info_.asklevels_[0]
                                         .limit_int_price_;  // this should be added to the levels number of all levels
            for (unsigned int i = 1; i < this_smv_.market_update_info_.asklevels_.size(); i++) {  // excluding top layer
              this_smv_.market_update_info_.asklevels_[i].limit_int_price_level_ += level_diff_top_two;
            }
          }

          this_smv_.SetBestLevelAskVariablesOnQuote();
          if (!t_is_intermediate_message_) {
            if (this_smv_.pl_change_listeners_present_) {
              if (IsNormalTradeTime(t_security_id_, watch_.tv()))
                this_smv_.NotifyOnPLChangeListeners(t_security_id_, this_smv_.market_update_info_, kTradeTypeSell,
                                                    level_to_add_ - 1, new_level_.limit_int_price_,
                                                    new_level_.limit_int_price_level_, 0, new_level_.limit_size_, 0,
                                                    new_level_.limit_ordercount_, t_is_intermediate_message_, 'N');
            }

            this_smv_.UpdateL1Prices();
            if (IsNormalTradeTime(t_security_id_, watch_.tv())) this_smv_.NotifyL1PriceListeners();
            if (IsNormalTradeTime(t_security_id_, watch_.tv())) this_smv_.OnL1SizeUpdate();
            //   this_smv_.l1_changed_since_last_ = false;
            this_smv_.l2_changed_since_last_ = true;
            if (this_smv_.l2_changed_since_last_) {
              // We neeed to clear all levels in the bid side which is less than this
              this_smv_.SanitizeBidSideWithPrice(t_int_price_);
              this_smv_.Sanitize();  // Sanitize both levels again  for duplicates etc
              if (IsNormalTradeTime(t_security_id_, watch_.tv())) this_smv_.OnL2Update();
              // this_smv_.l2_changed_since_last_ = false;
            }
          } else {
            if (this_smv_.pl_change_listeners_present_) {
              if (IsNormalTradeTime(t_security_id_, watch_.tv()))
                this_smv_.NotifyOnPLChangeListeners(t_security_id_, this_smv_.market_update_info_, kTradeTypeSell,
                                                    level_to_add_ - 1, new_level_.limit_int_price_,
                                                    new_level_.limit_int_price_level_, 0, new_level_.limit_size_, 0,
                                                    new_level_.limit_ordercount_, t_is_intermediate_message_, 'N');
            }

            this_smv_.l1_changed_since_last_ = true;
          }

          if (dbglogger_.CheckLoggingLevel(BOOK_TEST)) {
            DBGLOG_TIME_CLASS_FUNC << " AddTopAsk level: " << level_to_add_ << DBGLOG_ENDL_FLUSH;
            DBGLOG_TIME_CLASS_FUNC << this_smv_.ShowMarket() << DBGLOG_ENDL_FLUSH;
          }
        } else {
          if (level_to_add_ > (int)this_smv_.market_update_info_.asklevels_.size() + 1) return;

          if (this_smv_.computing_price_levels()) {
            new_level_.limit_int_price_level_ =
                new_level_.limit_int_price_ - this_smv_.market_update_info_.asklevels_[0].limit_int_price_;
          }

          this_smv_.AddNonTopAsk(level_to_add_, new_level_);

          if (!t_is_intermediate_message_) {
            if (this_smv_.pl_change_listeners_present_) {
              if (IsNormalTradeTime(t_security_id_, watch_.tv()))
                this_smv_.NotifyOnPLChangeListeners(t_security_id_, this_smv_.market_update_info_, kTradeTypeSell,
                                                    level_to_add_ - 1, new_level_.limit_int_price_,
                                                    new_level_.limit_int_price_level_, 0, new_level_.limit_size_, 0,
                                                    new_level_.limit_ordercount_, t_is_intermediate_message_, 'N');
            }

            if (this_smv_.l1_changed_since_last_) {
              this_smv_.UpdateL1Prices();
              if (IsNormalTradeTime(t_security_id_, watch_.tv())) this_smv_.NotifyL1PriceListeners();
              if (IsNormalTradeTime(t_security_id_, watch_.tv())) this_smv_.OnL1SizeUpdate();
              // this_smv_.l1_changed_since_last_ = false;
            }

            this_smv_.Sanitize();  // Sanitize both levels
            if (IsNormalTradeTime(t_security_id_, watch_.tv())) this_smv_.OnL2Update();
            //		      this_smv_.l2_changed_since_last_ = false;
            this_smv_.l2_changed_since_last_ = true;
          } else {
            if (this_smv_.pl_change_listeners_present_) {
              if (IsNormalTradeTime(t_security_id_, watch_.tv()))
                this_smv_.NotifyOnPLChangeListeners(t_security_id_, this_smv_.market_update_info_, kTradeTypeSell,
                                                    level_to_add_ - 1, new_level_.limit_int_price_,
                                                    new_level_.limit_int_price_level_, 0, new_level_.limit_size_, 0,
                                                    new_level_.limit_ordercount_, t_is_intermediate_message_, 'N');
            }

            //  this_smv_.l2_changed_since_last_ = true;
          }

          if (dbglogger_.CheckLoggingLevel(BOOK_TEST)) {
            DBGLOG_TIME_CLASS_FUNC << " AddNonTopAsk level : " << level_to_add_ << DBGLOG_ENDL_FLUSH;
            DBGLOG_TIME_CLASS_FUNC << this_smv_.ShowMarket() << DBGLOG_ENDL_FLUSH;
          }
        }
      } else {
        // Price level already exists in the smv.
        MarketUpdateInfoLevelStruct *this_level_ = &(this_smv_.market_update_info_.asklevels_[level_to_add_ - 1]);

        this_level_->limit_size_ += t_new_size_;
        ++(this_level_->limit_ordercount_);

        if (level_to_add_ == 1) {
          this_smv_.SetBestLevelAskVariablesOnQuote();  // TODO .. optimization : no need to assign bestask_price_ and
                                                        // bestask_int_price_ since they have not changed
          if (!t_is_intermediate_message_) {
            if (this_smv_.pl_change_listeners_present_) {
              if (IsNormalTradeTime(t_security_id_, watch_.tv()))
                this_smv_.NotifyOnPLChangeListeners(t_security_id_, this_smv_.market_update_info_, kTradeTypeSell,
                                                    level_to_add_ - 1, this_level_->limit_int_price_,
                                                    this_level_->limit_int_price_level_,
                                                    this_level_->limit_size_ - t_new_size_, this_level_->limit_size_,
                                                    this_level_->limit_ordercount_ - 1, this_level_->limit_ordercount_,
                                                    t_is_intermediate_message_, 'C');
            }

            this_smv_.UpdateL1Prices();
            if (IsNormalTradeTime(t_security_id_, watch_.tv())) this_smv_.NotifyL1PriceListeners();
            if (IsNormalTradeTime(t_security_id_, watch_.tv())) this_smv_.OnL1SizeUpdate();
            // this_smv_.l1_changed_since_last_ = false;
            this_smv_.l2_changed_since_last_ = true;

            if (this_smv_.l2_changed_since_last_) {
              this_smv_.SanitizeBidSideWithPrice(t_int_price_);
              this_smv_.Sanitize();  // Sanitize both levels again  for duplicates etc
              if (IsNormalTradeTime(t_security_id_, watch_.tv())) this_smv_.OnL2Update();
              //  this_smv_.l2_changed_since_last_ = false;
            }
          } else {
            if (this_smv_.pl_change_listeners_present_) {
              if (IsNormalTradeTime(t_security_id_, watch_.tv()))
                this_smv_.NotifyOnPLChangeListeners(t_security_id_, this_smv_.market_update_info_, kTradeTypeSell,
                                                    level_to_add_ - 1, this_level_->limit_int_price_,
                                                    this_level_->limit_int_price_level_,
                                                    this_level_->limit_size_ - t_new_size_, this_level_->limit_size_,
                                                    this_level_->limit_ordercount_ - 1, this_level_->limit_ordercount_,
                                                    t_is_intermediate_message_, 'C');
            }

            this_smv_.l1_changed_since_last_ = true;
          }
        } else {
          if (!t_is_intermediate_message_) {
            if (this_smv_.l1_changed_since_last_) {
              if (this_smv_.pl_change_listeners_present_) {
                if (IsNormalTradeTime(t_security_id_, watch_.tv()))
                  this_smv_.NotifyOnPLChangeListeners(t_security_id_, this_smv_.market_update_info_, kTradeTypeSell,
                                                      level_to_add_ - 1, this_level_->limit_int_price_,
                                                      this_level_->limit_int_price_level_,
                                                      this_level_->limit_size_ - t_new_size_, this_level_->limit_size_,
                                                      this_level_->limit_ordercount_ - 1,
                                                      this_level_->limit_ordercount_, t_is_intermediate_message_, 'C');
              }

              this_smv_.UpdateL1Prices();
              if (IsNormalTradeTime(t_security_id_, watch_.tv())) this_smv_.NotifyL1PriceListeners();
              if (IsNormalTradeTime(t_security_id_, watch_.tv())) this_smv_.OnL1SizeUpdate();
              // this_smv_.l1_changed_since_last_ = false;
            }
            this_smv_.Sanitize();
            if (IsNormalTradeTime(t_security_id_, watch_.tv())) this_smv_.OnL2Update();
            // this_smv_.l2_changed_since_last_ = false;
            this_smv_.l2_changed_since_last_ = true;
          } else {
            if (this_smv_.pl_change_listeners_present_) {
              if (IsNormalTradeTime(t_security_id_, watch_.tv()))
                this_smv_.NotifyOnPLChangeListeners(t_security_id_, this_smv_.market_update_info_, kTradeTypeSell,
                                                    level_to_add_ - 1, this_level_->limit_int_price_,
                                                    this_level_->limit_int_price_level_,
                                                    this_level_->limit_size_ - t_new_size_, this_level_->limit_size_,
                                                    this_level_->limit_ordercount_ - 1, this_level_->limit_ordercount_,
                                                    t_is_intermediate_message_, 'C');
            }

            this_smv_.l2_changed_since_last_ = true;
          }
        }

        if (dbglogger_.CheckLoggingLevel(BOOK_TEST)) {
          DBGLOG_TIME_CLASS_FUNC << " Updating Ask level : " << level_to_add_ << DBGLOG_ENDL_FLUSH;
          DBGLOG_TIME_CLASS_FUNC << this_smv_.ShowMarket() << DBGLOG_ENDL_FLUSH;
        }
      }
    } break;

    default: { } break; }

  int level_skipped_ = 0;

  // Simply add this order to the order depth book.
  switch (t_buysell_) {
    case kTradeTypeBuy: {
      // resize vector if needed
      if (t_level_added_ > (int)this_omv_.bid_order_depth_book_.size()) {
        this_omv_.bid_order_depth_book_.resize(t_level_added_);
      }

      std::vector<OSEMapOrder *>::iterator _itr_ = this_omv_.bid_order_depth_book_.begin();
      for (_itr_ = this_omv_.bid_order_depth_book_.begin(), level_skipped_ = 1; level_skipped_ < t_level_added_;
           ++_itr_, ++level_skipped_)
        ;

      this_omv_.bid_order_depth_book_.insert(_itr_, p_ose_map_order_);
    } break;

    case kTradeTypeSell: {
      // resize vector if needed
      if (t_level_added_ > (int)this_omv_.ask_order_depth_book_.size()) {
        this_omv_.ask_order_depth_book_.resize(t_level_added_);
      }

      std::vector<OSEMapOrder *>::iterator _itr_ = this_omv_.ask_order_depth_book_.begin();
      for (_itr_ = this_omv_.ask_order_depth_book_.begin(), level_skipped_ = 1; level_skipped_ < t_level_added_;
           ++_itr_, ++level_skipped_)
        ;

      this_omv_.ask_order_depth_book_.insert(_itr_, p_ose_map_order_);
    } break;

    default: { } break; }

  //  DBGLOG_TIME_CLASS_FUNC << "Order depth book:" << DBGLOG_ENDL_FLUSH;
  //    DBGLOG_TIME_CLASS_FUNC << ShowOrderDepthBook ( this_omv_ ) << DBGLOG_ENDL_FLUSH ;
  //    DBGLOG_TIME_CLASS_FUNC << this_smv_.ShowMarket () << DBGLOG_ENDL_FLUSH ;
}

void OSEOrderLevelMarketViewManager::OnOrderLevelDelete(const unsigned int t_security_id_, uint64_t t_order_id_,
                                                        const TradeType_t t_buysell_, const int t_level_removed_,
                                                        const int t_price_, const bool t_is_intermediate_message_) {
  if (t_level_removed_ > MAX_OSE_ORDER_BOOK_DEPTH) return;

  if (dbglogger_.CheckLoggingLevel(BOOK_TEST)) {
    DBGLOG_TIME_CLASS_FUNC_LINE << " ( secid= " << t_security_id_ << " orderid= " << t_order_id_
                                << " bs= " << GetTradeTypeChar(t_buysell_) << " lvl= " << t_level_removed_
                                << " px= " << t_price_ << " IsInter= " << (t_is_intermediate_message_ ? 'Y' : 'N')
                                << " )" << DBGLOG_ENDL_FLUSH;
  }

  SecurityMarketView &this_smv_ =
      *(security_market_view_map_[t_security_id_]);  // CRASH_ALERT could crash if garbage t_security_id_ come in
  OSEOrderMarketView &this_omv_ = security_id_to_ose_omv_map_[t_security_id_];

  //    DBGLOG_TIME_CLASS_FUNC << "Before Order depth book:" << DBGLOG_ENDL_FLUSH ;
  //    DBGLOG_TIME_CLASS_FUNC << ShowOrderDepthBook ( this_omv_ ) << DBGLOG_ENDL_FLUSH ;

  int bid_level_to_be_removed_ = 1;
  int ask_level_to_removed_ = 1;
  // int t_int_price = this_smv_.GetIntPx( t_price_ );
  // Time to update the smv for this order deletion.
  switch (t_buysell_) {
    case kTradeTypeBuy: {
      int level_removed_ = 0;
      int t_int_price_ = 0;

      // LEVEL RECONCILIATION
      //============================== delete order will have order id, fetch the same from order depth book
      //=====================//
      bool found_bid_order_id_ = false;

      std::vector<OSEMapOrder *>::iterator _itr_ = this_omv_.bid_order_depth_book_.begin();

      for (_itr_ = this_omv_.bid_order_depth_book_.begin(); _itr_ != this_omv_.bid_order_depth_book_.end();
           _itr_++, bid_level_to_be_removed_++) {
        if ((*_itr_) && ((*_itr_)->order_id_ == t_order_id_)) {
          found_bid_order_id_ = true;
          break;
        }
      }

      //============================================================================================================================//

      if (t_level_removed_ != bid_level_to_be_removed_) {
        // dbglogger_ << " Our Level : " << bid_level_to_be_removed_ << " Exch Level : " << t_level_removed_ << " Bid
        // Size : " << this_omv_.bid_order_depth_book_.size() << "\n";
        // dbglogger_.DumpCurrentBuffer ( );
      }
      if (!found_bid_order_id_) {
        bid_level_to_be_removed_ = t_level_removed_;  // should not really get here
        // //dbglogger_<<"should not get here\n";
        // // dbglogger_.DumpCurrentBuffer();
      }

      // bid_level_to_be_removed_ = t_level_removed_ ;
      //  t_level_removed_ = bid_level_to_be_removed_;

      if (bid_level_to_be_removed_ >= (int)this_omv_.bid_order_depth_book_.size()) {
        // dbglogger_ << " Level Doesn't exist, bid book size : " << this_omv_.bid_order_depth_book_.size() << " level
        // removed : " << bid_level_to_be_removed_ << "\n" ;
        // // dbglogger_.DumpCurrentBuffer() ;
        return;
      }

      if (!this_omv_.bid_order_depth_book_[bid_level_to_be_removed_ - 1]) {
        // should only happen if we have discarded msg of level higher than MAX_DEPTH_BOOK and we receive a delete for
        // that at level lower than MAX_DEPTH cause of other deletes lower
        // than max depth
        // dbglogger_ << " Level Doesn't Exist " << t_level_removed_ << " Bid Book Size : " <<
        // this_omv_.bid_order_depth_book_.size() << "\n" ;
        return;
      }

      if (this_omv_.bid_order_depth_book_[bid_level_to_be_removed_ - 1]->order_id_ != t_order_id_) {
        // dbglogger_ << " Order Id Doesn't Match For Delete. Order Id : " <<  t_order_id_ << " Level : " <<
        // bid_level_to_be_removed_ << "\n";
        return;
      }

      if (bid_level_to_be_removed_ < 1 || bid_level_to_be_removed_ > (int)this_omv_.bid_order_depth_book_.size()) {
        break;
      }

      OSEMapOrder *p_ose_map_order_ = this_omv_.bid_order_depth_book_[bid_level_to_be_removed_ - 1];

      if (!p_ose_map_order_) {
        return;
      }

      if (p_ose_map_order_->order_id_ != t_order_id_) {
        return;
      }

      // Find the price of the order to be deleted, so we can look it up in the smv.
      t_int_price_ = this_omv_.bid_order_depth_book_[bid_level_to_be_removed_ - 1]->int_price_;

      // We compute which level is to be removed.
      for (level_removed_ = 0;
           level_removed_ < (int)this_smv_.market_update_info_.bidlevels_.size() &&
           this_smv_.market_update_info_.bidlevels_[level_removed_].limit_int_price_ != t_int_price_;
           ++level_removed_)
        ;

      if (level_removed_ == (int)this_smv_.market_update_info_.bidlevels_.size()) {
        //================================ Order Should be deleted from the order depth book even SMV doesn't have it
        //============//
        int level_skipped_ = 0;

        if (bid_level_to_be_removed_ < 1 || bid_level_to_be_removed_ > (int)this_omv_.bid_order_depth_book_.size()) {
          return;
        }

        std::vector<OSEMapOrder *>::iterator _itr_ = this_omv_.bid_order_depth_book_.begin();
        for (_itr_ = this_omv_.bid_order_depth_book_.begin(), level_skipped_ = 1;
             level_skipped_ < bid_level_to_be_removed_; ++_itr_, ++level_skipped_)
          ;
        if (_itr_ != this_omv_.bid_order_depth_book_.end()) {
          osemaporder_mempool_.DeAlloc((*_itr_));
          this_omv_.bid_order_depth_book_.erase(_itr_);
        }
        //============================================================================================================================//

        return;
      }

      ++level_removed_;  // Needed because levels in remove operations on the smv are 1 indexed.

      // 1. If there is only one ordercount left, and a new delete message come we should be removing the whole
      // price level 2. If the the size to be removed is equal/more than the current size remove the entire level
      if (this_smv_.market_update_info_.bidlevels_[level_removed_ - 1].limit_size_ <= p_ose_map_order_->size_ ||
          this_smv_.market_update_info_.bidlevels_[level_removed_ - 1].limit_ordercount_ == 1) {
        // Case where we remove the entire price level.

        if (level_removed_ == 1) {
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
            register unsigned int t_level_ = level_removed_ - 1;
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
          if (!t_is_intermediate_message_) {
            if (this_smv_.pl_change_listeners_present_) {
              if (IsNormalTradeTime(t_security_id_, watch_.tv()))
                this_smv_.NotifyOnPLChangeListeners(t_security_id_, this_smv_.market_update_info_, kTradeTypeBuy,
                                                    level_removed_ - 1, old_bid_int_price, old_bid_int_price_level,
                                                    old_size_at_this_price, 0, old_ordercount_at_this_price, 0,
                                                    t_is_intermediate_message_, 'D');
            }

            this_smv_.UpdateL1Prices();
            if (IsNormalTradeTime(t_security_id_, watch_.tv())) this_smv_.NotifyL1PriceListeners();
            if (IsNormalTradeTime(t_security_id_, watch_.tv())) this_smv_.OnL1SizeUpdate();
            // this_smv_.l1_changed_since_last_ = false;
            this_smv_.l2_changed_since_last_ = true;
            if (this_smv_.l2_changed_since_last_) {
              // We make sure all levels in the ask side have price higher than the new L1 price
              this_smv_.SanitizeAskSideWithPrice(t_int_price_);
              this_smv_.Sanitize();  // Sanitize both levels again  for duplicates etc
              if (IsNormalTradeTime(t_security_id_, watch_.tv())) this_smv_.OnL2Update();
              //		    this_smv_.l2_changed_since_last_ = false;
            }
          } else {
            if (this_smv_.pl_change_listeners_present_) {
              if (IsNormalTradeTime(t_security_id_, watch_.tv()))
                this_smv_.NotifyOnPLChangeListeners(t_security_id_, this_smv_.market_update_info_, kTradeTypeBuy,
                                                    level_removed_ - 1, old_bid_int_price, old_bid_int_price_level,
                                                    old_size_at_this_price, 0, old_ordercount_at_this_price, 0,
                                                    t_is_intermediate_message_, 'D');
            }

            this_smv_.l1_changed_since_last_ = true;
          }

          if (dbglogger_.CheckLoggingLevel(BOOK_TEST)) {
            DBGLOG_TIME_CLASS_FUNC << " RemoveTopBid level : " << level_removed_ << DBGLOG_ENDL_FLUSH;
            DBGLOG_TIME_CLASS_FUNC << this_smv_.ShowMarket() << DBGLOG_ENDL_FLUSH;
          }
        } else {
          int old_size_at_this_price = 0;
          int old_bid_int_price = 0;
          int old_bid_int_price_level = 0;
          int old_ordercount_at_this_price = 0;

          if (this_smv_.pl_change_listeners_present_) {
            register unsigned int t_level_ = level_removed_ - 1;
            if (t_level_ < this_smv_.market_update_info_.bidlevels_.size()) {
              old_size_at_this_price = this_smv_.market_update_info_.bidlevels_[t_level_].limit_size_;
              old_bid_int_price = this_smv_.market_update_info_.bidlevels_[t_level_].limit_int_price_;
              old_bid_int_price_level =
                  std::max(0, this_smv_.market_update_info_.bidlevels_[t_level_].limit_int_price_level_);
              old_ordercount_at_this_price = this_smv_.market_update_info_.bidlevels_[t_level_].limit_ordercount_;
            }
          }

          this_smv_.RemoveNonTopBid(level_removed_);

          if (!t_is_intermediate_message_) {
            if (this_smv_.l1_changed_since_last_) {
              if (this_smv_.pl_change_listeners_present_) {
                if (IsNormalTradeTime(t_security_id_, watch_.tv()))
                  this_smv_.NotifyOnPLChangeListeners(t_security_id_, this_smv_.market_update_info_, kTradeTypeBuy,
                                                      level_removed_ - 1, old_bid_int_price, old_bid_int_price_level,
                                                      old_size_at_this_price, 0, old_ordercount_at_this_price, 0,
                                                      t_is_intermediate_message_, 'D');
              }

              this_smv_.UpdateL1Prices();
              if (IsNormalTradeTime(t_security_id_, watch_.tv())) {
                this_smv_.NotifyL1PriceListeners();
                this_smv_.OnL1SizeUpdate();
              }
              // this_smv_.l1_changed_since_last_ = false;
            }
            this_smv_.Sanitize();
            if (IsNormalTradeTime(t_security_id_, watch_.tv())) this_smv_.OnL2Update();
            // this_smv_.l2_changed_since_last_ = false;
            this_smv_.l2_changed_since_last_ = true;
          } else {
            if (this_smv_.pl_change_listeners_present_) {
              if (IsNormalTradeTime(t_security_id_, watch_.tv()))
                this_smv_.NotifyOnPLChangeListeners(t_security_id_, this_smv_.market_update_info_, kTradeTypeBuy,
                                                    level_removed_ - 1, old_bid_int_price, old_bid_int_price_level,
                                                    old_size_at_this_price, 0, old_ordercount_at_this_price, 0,
                                                    t_is_intermediate_message_, 'D');
            }

            this_smv_.l2_changed_since_last_ = true;
          }

          if (dbglogger_.CheckLoggingLevel(BOOK_TEST)) {
            DBGLOG_TIME_CLASS_FUNC << " RemoveNonTopBid level : " << level_removed_ << DBGLOG_ENDL_FLUSH;
            DBGLOG_TIME_CLASS_FUNC << this_smv_.ShowMarket() << DBGLOG_ENDL_FLUSH;
          }
        }
      } else {
        // Entire level is not to be removed, we simply modify the level size and ordercount.
        MarketUpdateInfoLevelStruct *this_level_ = &(this_smv_.market_update_info_.bidlevels_[level_removed_ - 1]);

        this_level_->limit_size_ -= p_ose_map_order_->size_;
        --(this_level_->limit_ordercount_);

        if (level_removed_ == 1) {
          this_smv_.SetBestLevelBidVariablesOnQuote();  // TODO_OPT : no need to assign bestbid_price_ and
                                                        // bestbid_int_price_ since they have not changed
          if (!t_is_intermediate_message_) {
            if (this_smv_.pl_change_listeners_present_) {
              if (IsNormalTradeTime(t_security_id_, watch_.tv()))
                this_smv_.NotifyOnPLChangeListeners(t_security_id_, this_smv_.market_update_info_, kTradeTypeBuy,
                                                    level_removed_ - 1, this_level_->limit_int_price_,
                                                    this_level_->limit_int_price_level_,
                                                    this_level_->limit_size_ + p_ose_map_order_->size_,
                                                    this_level_->limit_size_, this_level_->limit_ordercount_ + 1,
                                                    this_level_->limit_ordercount_, t_is_intermediate_message_, 'C');
            }

            this_smv_.UpdateL1Prices();
            if (IsNormalTradeTime(t_security_id_, watch_.tv())) {
              this_smv_.NotifyL1PriceListeners();
              this_smv_.OnL1SizeUpdate();
            }
            // this_smv_.l1_changed_since_last_ = false;
            this_smv_.l1_changed_since_last_ = true;
            this_smv_.l2_changed_since_last_ = true;
            if (this_smv_.l2_changed_since_last_) {
              this_smv_.SanitizeAskSideWithPrice(t_int_price_);
              this_smv_.Sanitize();
              if (IsNormalTradeTime(t_security_id_, watch_.tv())) this_smv_.OnL2Update();
              // this_smv_.l2_changed_since_last_ = false;
            }
          } else {
            if (this_smv_.pl_change_listeners_present_) {
              if (IsNormalTradeTime(t_security_id_, watch_.tv()))
                this_smv_.NotifyOnPLChangeListeners(t_security_id_, this_smv_.market_update_info_, kTradeTypeBuy,
                                                    level_removed_ - 1, this_level_->limit_int_price_,
                                                    this_level_->limit_int_price_level_,
                                                    this_level_->limit_size_ + p_ose_map_order_->size_,
                                                    this_level_->limit_size_, this_level_->limit_ordercount_ + 1,
                                                    this_level_->limit_ordercount_, t_is_intermediate_message_, 'C');
            }

            this_smv_.UpdateL1Prices();
            if (IsNormalTradeTime(t_security_id_, watch_.tv())) this_smv_.NotifyL1PriceListeners();
            this_smv_.l1_changed_since_last_ = true;
          }
        } else {
          if (!t_is_intermediate_message_) {
            if (this_smv_.l1_changed_since_last_) {
              if (this_smv_.pl_change_listeners_present_) {
                if (IsNormalTradeTime(t_security_id_, watch_.tv()))
                  this_smv_.NotifyOnPLChangeListeners(t_security_id_, this_smv_.market_update_info_, kTradeTypeBuy,
                                                      level_removed_ - 1, this_level_->limit_int_price_,
                                                      this_level_->limit_int_price_level_,
                                                      this_level_->limit_size_ + p_ose_map_order_->size_,
                                                      this_level_->limit_size_, this_level_->limit_ordercount_ + 1,
                                                      this_level_->limit_ordercount_, t_is_intermediate_message_, 'C');
              }

              this_smv_.UpdateL1Prices();
              if (IsNormalTradeTime(t_security_id_, watch_.tv())) {
                this_smv_.NotifyL1PriceListeners();
                this_smv_.OnL1SizeUpdate();
              }
              // this_smv_.l1_changed_since_last_ = false;
            }
            this_smv_.Sanitize();
            if (IsNormalTradeTime(t_security_id_, watch_.tv())) this_smv_.OnL2Update();
            // this_smv_.l2_changed_since_last_ = false;
          } else {
            if (this_smv_.pl_change_listeners_present_) {
              if (IsNormalTradeTime(t_security_id_, watch_.tv()))
                this_smv_.NotifyOnPLChangeListeners(t_security_id_, this_smv_.market_update_info_, kTradeTypeBuy,
                                                    level_removed_ - 1, this_level_->limit_int_price_,
                                                    this_level_->limit_int_price_level_,
                                                    this_level_->limit_size_ + p_ose_map_order_->size_,
                                                    this_level_->limit_size_, this_level_->limit_ordercount_ + 1,
                                                    this_level_->limit_ordercount_, t_is_intermediate_message_, 'C');
            }

            this_smv_.UpdateL1Prices();
            if (IsNormalTradeTime(t_security_id_, watch_.tv())) this_smv_.NotifyL1PriceListeners();
            this_smv_.l2_changed_since_last_ = true;
          }
        }

        if (dbglogger_.CheckLoggingLevel(BOOK_TEST)) {
          DBGLOG_TIME_CLASS_FUNC << " Updating Bid level: " << level_removed_ << DBGLOG_ENDL_FLUSH;
          DBGLOG_TIME_CLASS_FUNC << this_smv_.ShowMarket() << DBGLOG_ENDL_FLUSH;
        }
      }
    } break;

    case kTradeTypeSell: {
      int level_removed_ = 0;
      int t_int_price_ = 0;

      //============================== delete order will have order id, fetch the same from order depth book
      //================== @ravi//
      bool found_ask_order_id_ = false;

      std::vector<OSEMapOrder *>::iterator _itr_ = this_omv_.ask_order_depth_book_.begin();

      for (_itr_ = this_omv_.ask_order_depth_book_.begin(); _itr_ != this_omv_.ask_order_depth_book_.end();
           _itr_++, ask_level_to_removed_++) {
        if ((*_itr_) && ((*_itr_)->order_id_ == t_order_id_)) {
          found_ask_order_id_ = true;
          break;
        }
      }
      //============================================================================================================================//

      if (t_level_removed_ != ask_level_to_removed_) {
        // dbglogger_ << " Our Level : " << ask_level_to_removed_ << " Exch Level : " << t_level_removed_ << " Ask Size
        // : " << this_omv_.ask_order_depth_book_.size() << "\n";
        // dbglogger_.DumpCurrentBuffer ();
      }

      if (!found_ask_order_id_) ask_level_to_removed_ = t_level_removed_;  // shoudn't really get here

      // ask_level_to_removed_ = t_level_removed_ ;
      // t_level_removed_ = ask_level_to_removed_ ;

      if (ask_level_to_removed_ >= (int)this_omv_.ask_order_depth_book_.size()) {
        // dbglogger_ << " Level Doesn't exist, ask book size : " << this_omv_.ask_order_depth_book_.size() << " level
        // removed : " << ask_level_to_removed_ << "\n" ;
        // dbglogger_.DumpCurrentBuffer() ;
        return;
      }

      // null check
      if (!this_omv_.ask_order_depth_book_[ask_level_to_removed_ - 1]) {
        // should only happen if we have discarded msg of level higher than MAX_DEPTH_BOOK and we receive a delete for
        // that at level lower than MAX_DEPTH cause of other deletes lower
        // than max depth
        // dbglogger_ << " Level Doesn't Exist " << t_level_removed_ << " Bid Book Size : " <<
        // this_omv_.ask_order_depth_book_.size() << "\n" ;
        return;
      }

      if (this_omv_.ask_order_depth_book_[ask_level_to_removed_ - 1]->order_id_ != t_order_id_) {
        // dbglogger_ << " Order Id Doesn't Match For Delete. Order Id : " <<  t_order_id_ << " Level : " <<
        // ask_level_to_removed_ << "\n";
        return;
      }

      if (ask_level_to_removed_ < 1 || ask_level_to_removed_ > (int)this_omv_.ask_order_depth_book_.size()) {
        break;
      }

      OSEMapOrder *p_ose_map_order_ = this_omv_.ask_order_depth_book_[ask_level_to_removed_ - 1];

      if (!p_ose_map_order_) {
        return;
      }

      if (p_ose_map_order_->order_id_ != t_order_id_) {
        return;
      }

      // Find the price of the order to be deleted, so we can look it up in the smv.
      t_int_price_ = p_ose_map_order_->int_price_;

      // We compute which level is to be removed.
      for (level_removed_ = 0;
           level_removed_ < (int)this_smv_.market_update_info_.asklevels_.size() &&
           this_smv_.market_update_info_.asklevels_[level_removed_].limit_int_price_ != t_int_price_;
           ++level_removed_)
        ;

      if (level_removed_ == (int)this_smv_.market_update_info_.asklevels_.size()) {
        //================================ Order Should be deleted from the order depth book even SMV doesn't have it
        //============//
        int level_skipped_ = 0;

        if (ask_level_to_removed_ < 1 || ask_level_to_removed_ > (int)this_omv_.ask_order_depth_book_.size()) {
          return;
        }

        std::vector<OSEMapOrder *>::iterator _itr_ = this_omv_.ask_order_depth_book_.begin();
        for (_itr_ = this_omv_.ask_order_depth_book_.begin(), level_skipped_ = 1;
             level_skipped_ < ask_level_to_removed_; ++_itr_, ++level_skipped_)
          ;
        if (_itr_ != this_omv_.ask_order_depth_book_.end()) {
          osemaporder_mempool_.DeAlloc((*_itr_));
          this_omv_.ask_order_depth_book_.erase(_itr_);
        }

        //============================================================================================================================//

        return;
      }

      ++level_removed_;  // Needed because levels in remove operations on the smv are 1 indexed.

      // 1. If there is only one ordercount left, and a new delete message come we should be removing the whole
      // price level 2. If the the size to be removed is equal/more than the current size remove the entire level
      if (this_smv_.market_update_info_.asklevels_[level_removed_ - 1].limit_size_ <= p_ose_map_order_->size_ ||
          this_smv_.market_update_info_.asklevels_[level_removed_ - 1].limit_ordercount_ == 1) {
        if (level_removed_ == 1) {
          if (this_smv_.computing_price_levels() && this_smv_.market_update_info_.asklevels_.size() > 1) {
            // update limit_int_price_level_ field everywhere
            int levels_from_top_to_deduct = this_smv_.market_update_info_.asklevels_[1].limit_int_price_ -
                                            this_smv_.market_update_info_.asklevels_[0].limit_int_price_;
            for (unsigned int i = 1; i < this_smv_.market_update_info_.asklevels_.size(); i++) {
              this_smv_.market_update_info_.asklevels_[i].limit_int_price_level_ -= levels_from_top_to_deduct;
            }
          }

          int old_size_at_this_price = 0;
          int old_ask_int_price = 0;
          int old_ask_int_price_level = 0;
          int old_ordercount_at_this_price = 0;

          if (this_smv_.pl_change_listeners_present_) {
            register unsigned int t_level_ = level_removed_ - 1;
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
          if (!t_is_intermediate_message_) {
            if (this_smv_.pl_change_listeners_present_) {
              if (IsNormalTradeTime(t_security_id_, watch_.tv()))
                this_smv_.NotifyOnPLChangeListeners(t_security_id_, this_smv_.market_update_info_, kTradeTypeSell,
                                                    level_removed_ - 1, old_ask_int_price, old_ask_int_price_level,
                                                    old_size_at_this_price, 0, old_ordercount_at_this_price, 0,
                                                    t_is_intermediate_message_, 'D');
            }

            this_smv_.UpdateL1Prices();
            if (IsNormalTradeTime(t_security_id_, watch_.tv())) {
              this_smv_.NotifyL1PriceListeners();
              this_smv_.OnL1SizeUpdate();
            }
            //@ So that We always sanitize nontopask level
            // this_smv_.l1_changed_since_last_ = false;
            this_smv_.l1_changed_since_last_ = true;
            this_smv_.l2_changed_since_last_ = true;

            if (this_smv_.l2_changed_since_last_) {
              this_smv_.SanitizeBidSideWithPrice(t_int_price_);
              this_smv_.Sanitize();  // Sanitize both levels again  for duplicates etc
              if (IsNormalTradeTime(t_security_id_, watch_.tv())) this_smv_.OnL2Update();
              // this_smv_.l2_changed_since_last_ = false;
            }
          } else {
            if (this_smv_.pl_change_listeners_present_) {
              if (IsNormalTradeTime(t_security_id_, watch_.tv()))
                this_smv_.NotifyOnPLChangeListeners(t_security_id_, this_smv_.market_update_info_, kTradeTypeSell,
                                                    level_removed_ - 1, old_ask_int_price, old_ask_int_price_level,
                                                    old_size_at_this_price, 0, old_ordercount_at_this_price, 0,
                                                    t_is_intermediate_message_, 'D');
            }

            this_smv_.l1_changed_since_last_ = true;
          }

          if (dbglogger_.CheckLoggingLevel(BOOK_TEST)) {
            DBGLOG_TIME_CLASS_FUNC << " RemoveTopAsk level : " << level_removed_ << DBGLOG_ENDL_FLUSH;
            DBGLOG_TIME_CLASS_FUNC << this_smv_.ShowMarket() << DBGLOG_ENDL_FLUSH;
          }
        } else {
          int old_size_at_this_price = 0;
          int old_ask_int_price = 0;
          int old_ask_int_price_level = 0;
          int old_ordercount_at_this_price = 0;

          if (this_smv_.pl_change_listeners_present_) {
            register unsigned int t_level_ = level_removed_ - 1;
            if (t_level_ < this_smv_.market_update_info_.asklevels_.size()) {
              old_size_at_this_price = this_smv_.market_update_info_.asklevels_[t_level_].limit_size_;
              old_ask_int_price = this_smv_.market_update_info_.asklevels_[t_level_].limit_int_price_;
              old_ask_int_price_level =
                  std::max(0, this_smv_.market_update_info_.asklevels_[t_level_].limit_int_price_level_);
              old_ordercount_at_this_price = this_smv_.market_update_info_.asklevels_[t_level_].limit_ordercount_;
            }
          }

          this_smv_.RemoveNonTopAsk(level_removed_);

          if (!t_is_intermediate_message_) {
            if (this_smv_.l1_changed_since_last_) {
              if (this_smv_.pl_change_listeners_present_) {
                if (IsNormalTradeTime(t_security_id_, watch_.tv()))
                  this_smv_.NotifyOnPLChangeListeners(t_security_id_, this_smv_.market_update_info_, kTradeTypeSell,
                                                      level_removed_ - 1, old_ask_int_price, old_ask_int_price_level,
                                                      old_size_at_this_price, 0, old_ordercount_at_this_price, 0,
                                                      t_is_intermediate_message_, 'D');
              }

              this_smv_.UpdateL1Prices();
              if (IsNormalTradeTime(t_security_id_, watch_.tv())) {
                this_smv_.NotifyL1PriceListeners();
                this_smv_.OnL1SizeUpdate();
              }
              //	      this_smv_.l1_changed_since_last_ = false;
            }
            this_smv_.Sanitize();
            if (IsNormalTradeTime(t_security_id_, watch_.tv())) this_smv_.OnL2Update();
            // Was false  but we want to sanitize
            this_smv_.l2_changed_since_last_ = true;
          } else {
            if (this_smv_.pl_change_listeners_present_) {
              if (IsNormalTradeTime(t_security_id_, watch_.tv()))
                this_smv_.NotifyOnPLChangeListeners(t_security_id_, this_smv_.market_update_info_, kTradeTypeSell,
                                                    level_removed_ - 1, old_ask_int_price, old_ask_int_price_level,
                                                    old_size_at_this_price, 0, old_ordercount_at_this_price, 0,
                                                    t_is_intermediate_message_, 'D');
            }

            this_smv_.l2_changed_since_last_ = true;
          }

          if (dbglogger_.CheckLoggingLevel(BOOK_TEST)) {
            DBGLOG_TIME_CLASS_FUNC << " RemoveNonTopAsk level : " << level_removed_ << DBGLOG_ENDL_FLUSH;
            DBGLOG_TIME_CLASS_FUNC << this_smv_.ShowMarket() << DBGLOG_ENDL_FLUSH;
          }
        }
      } else {
        // Entire level is not to be removed, we simply modify the level size and ordercount.
        MarketUpdateInfoLevelStruct *this_level_ = &(this_smv_.market_update_info_.asklevels_[level_removed_ - 1]);

        this_level_->limit_size_ -= p_ose_map_order_->size_;
        --(this_level_->limit_ordercount_);

        if (level_removed_ == 1) {
          this_smv_.SetBestLevelAskVariablesOnQuote();  // TODO_OPT : no need to assign bestask_price_ and
                                                        // bestask_int_price_ since they have not changed
          if (!t_is_intermediate_message_) {
            if (this_smv_.pl_change_listeners_present_) {
              if (IsNormalTradeTime(t_security_id_, watch_.tv()))
                this_smv_.NotifyOnPLChangeListeners(t_security_id_, this_smv_.market_update_info_, kTradeTypeSell,
                                                    level_removed_ - 1, this_level_->limit_int_price_,
                                                    this_level_->limit_int_price_level_,
                                                    this_level_->limit_size_ + p_ose_map_order_->size_,
                                                    this_level_->limit_size_, this_level_->limit_ordercount_ + 1,
                                                    this_level_->limit_ordercount_, t_is_intermediate_message_, 'C');
            }

            this_smv_.UpdateL1Prices();
            if (IsNormalTradeTime(t_security_id_, watch_.tv())) {
              this_smv_.NotifyL1PriceListeners();
              this_smv_.OnL1SizeUpdate();
            }
            //		  this_smv_.l1_changed_since_last_ = false;
            this_smv_.l1_changed_since_last_ = true;
            this_smv_.l2_changed_since_last_ = true;

            if (this_smv_.l2_changed_since_last_) {
              this_smv_.SanitizeBidSideWithPrice(t_int_price_);
              this_smv_.Sanitize();
              if (IsNormalTradeTime(t_security_id_, watch_.tv())) this_smv_.OnL2Update();
              // this_smv_.l2_changed_since_last_ = false;
            }
          } else {
            if (this_smv_.pl_change_listeners_present_) {
              if (IsNormalTradeTime(t_security_id_, watch_.tv()))
                this_smv_.NotifyOnPLChangeListeners(t_security_id_, this_smv_.market_update_info_, kTradeTypeSell,
                                                    level_removed_ - 1, this_level_->limit_int_price_,
                                                    this_level_->limit_int_price_level_,
                                                    this_level_->limit_size_ + p_ose_map_order_->size_,
                                                    this_level_->limit_size_, this_level_->limit_ordercount_ + 1,
                                                    this_level_->limit_ordercount_, t_is_intermediate_message_, 'C');
            }

            this_smv_.UpdateL1Prices();
            if (IsNormalTradeTime(t_security_id_, watch_.tv())) this_smv_.NotifyL1PriceListeners();
            this_smv_.l1_changed_since_last_ = true;
          }
        } else {
          if (!t_is_intermediate_message_) {
            if (this_smv_.l1_changed_since_last_) {
              if (this_smv_.pl_change_listeners_present_) {
                if (IsNormalTradeTime(t_security_id_, watch_.tv()))
                  this_smv_.NotifyOnPLChangeListeners(t_security_id_, this_smv_.market_update_info_, kTradeTypeSell,
                                                      level_removed_ - 1, this_level_->limit_int_price_,
                                                      this_level_->limit_int_price_level_,
                                                      this_level_->limit_size_ + p_ose_map_order_->size_,
                                                      this_level_->limit_size_, this_level_->limit_ordercount_ + 1,
                                                      this_level_->limit_ordercount_, t_is_intermediate_message_, 'C');
              }

              this_smv_.UpdateL1Prices();
              if (IsNormalTradeTime(t_security_id_, watch_.tv())) {
                this_smv_.NotifyL1PriceListeners();
                this_smv_.OnL1SizeUpdate();
              }
              //		      this_smv_.l1_changed_since_last_ = false;
              this_smv_.l1_changed_since_last_ = true;
            }
            this_smv_.Sanitize();
            if (IsNormalTradeTime(t_security_id_, watch_.tv())) this_smv_.OnL2Update();
            //		  this_smv_.l2_changed_since_last_ = false;
          } else {
            if (this_smv_.pl_change_listeners_present_) {
              if (IsNormalTradeTime(t_security_id_, watch_.tv()))
                this_smv_.NotifyOnPLChangeListeners(t_security_id_, this_smv_.market_update_info_, kTradeTypeSell,
                                                    level_removed_ - 1, this_level_->limit_int_price_,
                                                    this_level_->limit_int_price_level_,
                                                    this_level_->limit_size_ + p_ose_map_order_->size_,
                                                    this_level_->limit_size_, this_level_->limit_ordercount_ + 1,
                                                    this_level_->limit_ordercount_, t_is_intermediate_message_, 'C');
            }

            this_smv_.UpdateL1Prices();
            if (IsNormalTradeTime(t_security_id_, watch_.tv())) this_smv_.NotifyL1PriceListeners();
            this_smv_.l2_changed_since_last_ = true;
          }
        }

        if (dbglogger_.CheckLoggingLevel(BOOK_TEST)) {
          DBGLOG_TIME_CLASS_FUNC << " Updating Ask level: " << level_removed_ << DBGLOG_ENDL_FLUSH;
          DBGLOG_TIME_CLASS_FUNC << this_smv_.ShowMarket() << DBGLOG_ENDL_FLUSH;
        }
      }

    } break;

    default: { } break; }

  int level_skipped_ = 0;

  // Remove this order from the order depth book.
  switch (t_buysell_) {
    case kTradeTypeBuy: {
      if (bid_level_to_be_removed_ < 1 || bid_level_to_be_removed_ > (int)this_omv_.bid_order_depth_book_.size()) {
        break;
      }

      std::vector<OSEMapOrder *>::iterator _itr_ = this_omv_.bid_order_depth_book_.begin();
      for (_itr_ = this_omv_.bid_order_depth_book_.begin(), level_skipped_ = 1;
           level_skipped_ < bid_level_to_be_removed_; ++_itr_, ++level_skipped_)
        ;
      if (_itr_ != this_omv_.bid_order_depth_book_.end()) {
        osemaporder_mempool_.DeAlloc((*_itr_));

        this_omv_.bid_order_depth_book_.erase(_itr_);
      }
      // erase () should not destroy the object pointed to by _itr_, since using pointers. VERIFIED.
    } break;

    case kTradeTypeSell: {
      if (ask_level_to_removed_ < 1 || ask_level_to_removed_ > (int)this_omv_.ask_order_depth_book_.size()) {
        break;
      }

      std::vector<OSEMapOrder *>::iterator _itr_ = this_omv_.ask_order_depth_book_.begin();
      for (_itr_ = this_omv_.ask_order_depth_book_.begin(), level_skipped_ = 1; level_skipped_ < ask_level_to_removed_;
           ++_itr_, ++level_skipped_)
        ;
      if (_itr_ != this_omv_.ask_order_depth_book_.end()) {
        osemaporder_mempool_.DeAlloc((*_itr_));

        this_omv_.ask_order_depth_book_.erase(_itr_);
      }
      // erase () should not destroy the object pointed to by _itr_, since using pointers. VERIFIED.
    } break;

    default: { } break; }

  //    DBGLOG_TIME_CLASS_FUNC << "After Order depth book:" << DBGLOG_ENDL_FLUSH ;
  //    DBGLOG_TIME_CLASS_FUNC << ShowOrderDepthBook ( this_omv_ ) << DBGLOG_ENDL_FLUSH ;
  //    DBGLOG_TIME_CLASS_FUNC << this_smv_.ShowMarket() << DBGLOG_ENDL_FLUSH ;
}

void OSEOrderLevelMarketViewManager::OnOrderLevelChange(const unsigned int t_security_id_, uint64_t t_order_id_,
                                                        const TradeType_t t_buysell_, const int t_level_changed_,
                                                        const int t_price_, const int t_size_diff_,
                                                        const bool t_is_intermediate_message_) {
  if (dbglogger_.CheckLoggingLevel(BOOK_TEST)) {
    DBGLOG_TIME_CLASS_FUNC_LINE << " ( secid= " << t_security_id_ << " orderid= " << t_order_id_
                                << " bs= " << GetTradeTypeChar(t_buysell_) << " lvl= " << t_level_changed_
                                << " px= " << t_price_ << " IsInter= " << (t_is_intermediate_message_ ? 'Y' : 'N')
                                << " )" << DBGLOG_ENDL_FLUSH;
  }

  OSEOrderMarketView &this_omv_ = security_id_to_ose_omv_map_[t_security_id_];

  OSEMapOrder *p_ose_map_order_ = NULL;
  uint64_t old_order_id_ = 0;
  int t_old_price_ = -1;
  int t_old_size_ = -1;
  switch (t_buysell_) {
    case kTradeTypeBuy: {
      int bid_level_to_be_changed_ = 1;
      bool found_bid_order_id_ = false;
      std::vector<OSEMapOrder *>::iterator _itr_ = this_omv_.bid_order_depth_book_.begin();
      for (_itr_ = this_omv_.bid_order_depth_book_.begin(); _itr_ != this_omv_.bid_order_depth_book_.end();
           _itr_++, bid_level_to_be_changed_++) {
        if ((*_itr_) && ((*_itr_)->order_id_ == t_order_id_)) {
          found_bid_order_id_ = true;
          break;
        }
      }
      //============================================================================================================================//

      if (t_level_changed_ != bid_level_to_be_changed_) {
        // dbglogger_ << " Our Level : " << bid_level_to_be_changed_ << " Exch Level : " << t_level_changed_ << " Bid
        // Size : " << this_omv_.bid_order_depth_book_.size() << "\n";
        // dbglogger_.DumpCurrentBuffer ( );
      }

      if (!found_bid_order_id_) bid_level_to_be_changed_ = t_level_changed_;  // should not really get here

      if (bid_level_to_be_changed_ < 1 || bid_level_to_be_changed_ > (int)this_omv_.bid_order_depth_book_.size()) {
        break;
      }
      p_ose_map_order_ = this_omv_.bid_order_depth_book_[bid_level_to_be_changed_ - 1];
    } break;
    case kTradeTypeSell: {
      bool found_ask_order_id_ = false;
      int ask_level_to_change_ = 1;
      std::vector<OSEMapOrder *>::iterator _itr_ = this_omv_.ask_order_depth_book_.begin();
      for (_itr_ = this_omv_.ask_order_depth_book_.begin(); _itr_ != this_omv_.ask_order_depth_book_.end();
           _itr_++, ask_level_to_change_++) {
        if ((*_itr_) && ((*_itr_)->order_id_ == t_order_id_)) {
          found_ask_order_id_ = true;
          break;
        }
      }
      //============================================================================================================================//

      if (t_level_changed_ != ask_level_to_change_) {
        // dbglogger_ << " Our Level : " << ask_level_to_change_ << " Exch Level : " << t_level_changed_ << " Ask Size :
        // " << this_omv_.ask_order_depth_book_.size() << "\n";
        // dbglogger_.DumpCurrentBuffer ();
      }

      if (!found_ask_order_id_) ask_level_to_change_ = t_level_changed_;  // shoudn't really get here
      if (ask_level_to_change_ < 1 || ask_level_to_change_ > (int)this_omv_.ask_order_depth_book_.size()) {
        break;
      }

      p_ose_map_order_ = this_omv_.ask_order_depth_book_[ask_level_to_change_ - 1];
    } break;
    default: { } break; }
  if (p_ose_map_order_ != NULL) {
    // We need to save these quantities, because OnOrderLevelDelete will delete p_ose_map_order_.
    old_order_id_ = p_ose_map_order_->order_id_;
    if (old_order_id_ != t_order_id_) {
      // dbglogger_<< "Order not found at the specified position : "<<t_level_changed_ <<"\n";
      return;
    }
    t_old_price_ = p_ose_map_order_->d_price_;
    t_old_size_ = p_ose_map_order_->size_;

    // Here we make an assumption that the book was consistent before we received this CHANGE message.
    // This means no missed messages.
    OnOrderLevelDelete(t_security_id_, old_order_id_, t_buysell_, t_level_changed_, t_old_price_,
                       t_is_intermediate_message_);
    OnOrderLevelNew(t_security_id_, t_order_id_, t_buysell_, t_level_changed_, t_price_, (t_size_diff_ + t_old_size_),
                    t_is_intermediate_message_);
  }
}

void OSEOrderLevelMarketViewManager::OnOrderLevelSnapNew(const unsigned int t_security_id_, uint64_t t_order_id_,
                                                         const TradeType_t t_buysell_, const int t_level_added_,
                                                         const int t_price_, const int t_new_size_,
                                                         const bool t_is_intermediate_message_) {
  // std::cout<<~t_int_price_<<"\n";
  if (dbglogger_.CheckLoggingLevel(BOOK_TEST)) {
    DBGLOG_TIME_CLASS_FUNC_LINE << " ( secid= " << t_security_id_ << " orderid= " << t_order_id_
                                << " bs= " << GetTradeTypeChar(t_buysell_) << " lvl= " << t_level_added_
                                << " px= " << t_price_ << " " << t_price_ << " sz= " << t_new_size_
                                << " IsInter= " << (t_is_intermediate_message_ ? 'Y' : 'N') << " )"
                                << DBGLOG_ENDL_FLUSH;
  }
  if (t_price_ == 0) return;

  SecurityMarketView &this_smv_ =
      *(security_market_view_map_[t_security_id_]);  // CRASH_ALERT could crash if garbage t_security_id_ come in
  OSEOrderMarketView &this_omv_ = security_id_to_ose_omv_map_[t_security_id_];

  int t_int_price_ = this_smv_.GetIntPx(t_price_);

  OSEMapOrder *p_ose_map_order_ = osemaporder_mempool_.Alloc();

  if (!p_ose_map_order_) {
    p_ose_map_order_ = (OSEMapOrder *)malloc(sizeof(OSEMapOrder));
  }

  p_ose_map_order_->set(t_order_id_, t_price_, t_int_price_, t_new_size_, t_buysell_);

  int level_to_add_ = 1;

  switch (t_buysell_) {
    case kTradeTypeBuy: {
      // Compute the level to add this order to in the security market view.
      for (level_to_add_ = 1;
           level_to_add_ <= (int)this_smv_.market_update_info_.bidlevels_.size() &&
           t_int_price_ < this_smv_.market_update_info_.bidlevels_[level_to_add_ - 1].limit_int_price_;
           ++level_to_add_)
        ;

      // create structure
      MarketUpdateInfoLevelStruct new_level_(0, t_int_price_, t_price_, t_new_size_, 1, watch_.tv());

      if (level_to_add_ > (int)this_smv_.market_update_info_.bidlevels_.size() ||
          t_int_price_ != this_smv_.market_update_info_.bidlevels_[level_to_add_ - 1].limit_int_price_) {
        // New price added. A new level will also be added.

        // Add a new level as computed previously to the smv.
        if (level_to_add_ == 1) {
          this_smv_.AddTopBid(new_level_);  // Efficient?

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
            this_smv_.UpdateL1Prices();
            // dbglogger_<<"the time in watch "<<watch_.tv().ToString() << "\n";
            // dbglogger_.DumpCurrentBuffer();
            if (IsNormalTradeTime(t_security_id_, watch_.tv())) this_smv_.NotifyL1PriceListeners();
            if (IsNormalTradeTime(t_security_id_, watch_.tv())) this_smv_.OnL1SizeUpdate();
            this_smv_.l1_changed_since_last_ = false;
            if (this_smv_.l2_changed_since_last_) {
              if (IsNormalTradeTime(t_security_id_, watch_.tv())) this_smv_.OnL2Update();
              this_smv_.l2_changed_since_last_ = false;
            }
          }

          else {
            this_smv_.l1_changed_since_last_ = true;
          }

          if (dbglogger_.CheckLoggingLevel(BOOK_TEST)) {
            DBGLOG_TIME_CLASS_FUNC << " AddTopBid level:" << level_to_add_ << DBGLOG_ENDL_FLUSH;
            if (IsNormalTradeTime(t_security_id_, watch_.tv()))
              DBGLOG_TIME_CLASS_FUNC << this_smv_.ShowMarket() << DBGLOG_ENDL_FLUSH;
          }

        } else {
          this_smv_.AddNonTopBid(level_to_add_, new_level_);

          if (!t_is_intermediate_message_) {
            if (this_smv_.l1_changed_since_last_) {
              this_smv_.UpdateL1Prices();
              if (IsNormalTradeTime(t_security_id_, watch_.tv())) this_smv_.NotifyL1PriceListeners();
              if (IsNormalTradeTime(t_security_id_, watch_.tv())) this_smv_.OnL1SizeUpdate();
              this_smv_.l1_changed_since_last_ = false;
            }

            if (IsNormalTradeTime(t_security_id_, watch_.tv())) this_smv_.OnL2Update();
            this_smv_.l2_changed_since_last_ = false;
          } else {
            this_smv_.l2_changed_since_last_ = true;
          }

          if (dbglogger_.CheckLoggingLevel(BOOK_TEST)) {
            DBGLOG_TIME_CLASS_FUNC << "AddNonTopBid level:" << level_to_add_ << DBGLOG_ENDL_FLUSH;
            if (IsNormalTradeTime(t_security_id_, watch_.tv()))
              DBGLOG_TIME_CLASS_FUNC << this_smv_.ShowMarket() << DBGLOG_ENDL_FLUSH;
          }
        }
      } else {
        // Price level already exists in the smv.
        MarketUpdateInfoLevelStruct *this_level_ = &(this_smv_.market_update_info_.bidlevels_[level_to_add_ - 1]);

        this_level_->limit_size_ += t_new_size_;
        ++(this_level_->limit_ordercount_);

        if (level_to_add_ == 1) {
          this_smv_.SetBestLevelBidVariablesOnQuote();  // TODO_OPT : no need to assign bestbid_price_ and
                                                        // bestbid_int_price_ since they have not changed
          if (!t_is_intermediate_message_) {
            this_smv_.UpdateL1Prices();
            // dbglogger_<<"the time in watch "<<watch_.tv().ToString() << "\n";
            // dbglogger_.DumpCurrentBuffer();
            if (IsNormalTradeTime(t_security_id_, watch_.tv())) this_smv_.NotifyL1PriceListeners();
            if (IsNormalTradeTime(t_security_id_, watch_.tv())) this_smv_.OnL1SizeUpdate();

            this_smv_.l1_changed_since_last_ = false;
            if (this_smv_.l2_changed_since_last_) {
              if (IsNormalTradeTime(t_security_id_, watch_.tv())) this_smv_.OnL2Update();
              this_smv_.l2_changed_since_last_ = false;
            }

          } else {
            this_smv_.l1_changed_since_last_ = true;
          }
        } else {
          if (!t_is_intermediate_message_) {
            if (this_smv_.l1_changed_since_last_) {
              this_smv_.UpdateL1Prices();
              if (IsNormalTradeTime(t_security_id_, watch_.tv())) this_smv_.NotifyL1PriceListeners();
              if (IsNormalTradeTime(t_security_id_, watch_.tv())) this_smv_.OnL1SizeUpdate();

              this_smv_.l1_changed_since_last_ = false;
            }
            if (IsNormalTradeTime(t_security_id_, watch_.tv())) this_smv_.OnL2Update();
            this_smv_.l2_changed_since_last_ = false;

          } else {
            this_smv_.l2_changed_since_last_ = true;
          }
        }

        if (dbglogger_.CheckLoggingLevel(BOOK_TEST)) {
          DBGLOG_TIME_CLASS_FUNC << " Updating Bid level: " << level_to_add_ << DBGLOG_ENDL_FLUSH;
          if (IsNormalTradeTime(t_security_id_, watch_.tv()))
            DBGLOG_TIME_CLASS_FUNC << this_smv_.ShowMarket() << DBGLOG_ENDL_FLUSH;
        }
      }
    } break;

    case kTradeTypeSell: {
      // Compute the level to add this order to in the security market view.
      for (level_to_add_ = 1;
           level_to_add_ <= (int)this_smv_.market_update_info_.asklevels_.size() &&
           t_int_price_ > this_smv_.market_update_info_.asklevels_[level_to_add_ - 1].limit_int_price_;
           ++level_to_add_)
        ;

      // create structure
      MarketUpdateInfoLevelStruct new_level_(0, t_int_price_, t_price_, t_new_size_, 1, watch_.tv());

      if (level_to_add_ > (int)this_smv_.market_update_info_.asklevels_.size() ||
          t_int_price_ != this_smv_.market_update_info_.asklevels_[level_to_add_ - 1].limit_int_price_) {
        // New price added. A new level will also be added.

        // Add a new level as computed previously to the smv.
        if (level_to_add_ == 1) {
          this_smv_.AddTopAsk(new_level_);  // Efficient?

          if (this_smv_.computing_price_levels() &&
              this_smv_.market_update_info_.asklevels_.size() > 1) {  // update the levels of lower levels
            // another option is if limit_int_price_ is computed then this computation can be done at runtime
            int level_diff_top_two = this_smv_.market_update_info_.asklevels_[1].limit_int_price_ -
                                     this_smv_.market_update_info_.asklevels_[0]
                                         .limit_int_price_;  // this should be added to the levels number of all levels
            for (unsigned int i = 1; i < this_smv_.market_update_info_.asklevels_.size(); i++) {  // excluding top layer
              this_smv_.market_update_info_.asklevels_[i].limit_int_price_level_ += level_diff_top_two;
            }
          }

          this_smv_.SetBestLevelAskVariablesOnQuote();
          if (!t_is_intermediate_message_) {
            this_smv_.UpdateL1Prices();
            if (IsNormalTradeTime(t_security_id_, watch_.tv())) this_smv_.NotifyL1PriceListeners();
            if (IsNormalTradeTime(t_security_id_, watch_.tv())) this_smv_.OnL1SizeUpdate();
            this_smv_.l1_changed_since_last_ = false;
            if (this_smv_.l2_changed_since_last_) {
              if (IsNormalTradeTime(t_security_id_, watch_.tv())) this_smv_.OnL2Update();
              this_smv_.l2_changed_since_last_ = false;
            }
          } else {
            this_smv_.l1_changed_since_last_ = true;
          }

          if (dbglogger_.CheckLoggingLevel(BOOK_TEST)) {
            DBGLOG_TIME_CLASS_FUNC << " AddTopAsk level: " << level_to_add_ << "\n" << this_smv_.ShowMarket()
                                   << DBGLOG_ENDL_FLUSH;
          }
        } else {
          this_smv_.AddNonTopAsk(level_to_add_, new_level_);

          if (!t_is_intermediate_message_) {
            if (this_smv_.l1_changed_since_last_) {
              this_smv_.UpdateL1Prices();
              if (IsNormalTradeTime(t_security_id_, watch_.tv())) this_smv_.NotifyL1PriceListeners();
              if (IsNormalTradeTime(t_security_id_, watch_.tv())) this_smv_.OnL1SizeUpdate();
              this_smv_.l1_changed_since_last_ = false;
            }
            if (IsNormalTradeTime(t_security_id_, watch_.tv())) this_smv_.OnL2Update();
            this_smv_.l2_changed_since_last_ = false;
          } else {
            this_smv_.l2_changed_since_last_ = true;
          }

          if (dbglogger_.CheckLoggingLevel(BOOK_TEST)) {
            DBGLOG_TIME_CLASS_FUNC << "AddNonTopAsk level : " << level_to_add_ << "\n" << this_smv_.ShowMarket()
                                   << DBGLOG_ENDL_FLUSH;
          }
        }
      } else {
        // Price level already exists in the smv.
        MarketUpdateInfoLevelStruct *this_level_ = &(this_smv_.market_update_info_.asklevels_[level_to_add_ - 1]);

        this_level_->limit_size_ += t_new_size_;
        ++(this_level_->limit_ordercount_);

        if (level_to_add_ == 1) {
          this_smv_.SetBestLevelAskVariablesOnQuote();  // TODO .. optimization : no need to assign bestask_price_ and
                                                        // bestask_int_price_ since they have not changed
          if (!t_is_intermediate_message_) {
            this_smv_.UpdateL1Prices();
            if (IsNormalTradeTime(t_security_id_, watch_.tv())) this_smv_.NotifyL1PriceListeners();
            if (IsNormalTradeTime(t_security_id_, watch_.tv())) this_smv_.OnL1SizeUpdate();
            this_smv_.l1_changed_since_last_ = false;
            if (this_smv_.l2_changed_since_last_) {
              if (IsNormalTradeTime(t_security_id_, watch_.tv())) this_smv_.OnL2Update();
              this_smv_.l2_changed_since_last_ = false;
            }
          } else {
            this_smv_.l1_changed_since_last_ = true;
          }
        } else {
          if (!t_is_intermediate_message_) {
            if (this_smv_.l1_changed_since_last_) {
              this_smv_.UpdateL1Prices();
              if (IsNormalTradeTime(t_security_id_, watch_.tv())) this_smv_.NotifyL1PriceListeners();
              if (IsNormalTradeTime(t_security_id_, watch_.tv())) this_smv_.OnL1SizeUpdate();
              this_smv_.l1_changed_since_last_ = false;
            }
            if (IsNormalTradeTime(t_security_id_, watch_.tv())) this_smv_.OnL2Update();
            this_smv_.l2_changed_since_last_ = false;
          } else {
            this_smv_.l2_changed_since_last_ = true;
          }
        }

        if (dbglogger_.CheckLoggingLevel(BOOK_TEST)) {
          if (IsNormalTradeTime(t_security_id_, watch_.tv()))
            DBGLOG_TIME_CLASS_FUNC << " Updating Ask level : " << level_to_add_ << "\n" << this_smv_.ShowMarket()
                                   << DBGLOG_ENDL_FLUSH;
        }
      }
    } break;

    default: { } break; }

  std::vector<OSEMapOrder *>::iterator _itr_;

  // Simply add this order to the order depth book.
  switch (t_buysell_) {
    case kTradeTypeBuy: {
      // resize vector if needed
      if (t_level_added_ > (int)this_omv_.bid_order_depth_book_.size()) {
        this_omv_.bid_order_depth_book_.resize(t_level_added_);
      }

      this_omv_.bid_order_depth_book_[t_level_added_ - 1] = p_ose_map_order_;
    } break;

    case kTradeTypeSell: {
      // resize vector if needed
      if (t_level_added_ > (int)this_omv_.ask_order_depth_book_.size()) {
        this_omv_.ask_order_depth_book_.resize(t_level_added_);
      }

      this_omv_.ask_order_depth_book_[t_level_added_ - 1] = p_ose_map_order_;
    } break;

    default: { } break; }

  // dbglogger_ << "Order depth book:\n" << ShowOrderDepthBook ( this_omv_ ) << DBGLOG_ENDL_FLUSH ;

  // dbglogger_ << this_smv_.ShowMarket () << DBGLOG_ENDL_FLUSH ;
}
void OSEOrderLevelMarketViewManager::OnTrade(const unsigned int t_security_id_, const int t_trade_price_,
                                             const int t_trade_size_, const TradeType_t t_buysell_) {
  // We never have to do anything here.
  // When a trade occurs, exchange will make the necessary changes to the book in terms of Change and Delete msgs.
  if (IsNormalTradeTime(t_security_id_, watch_.tv()))
    security_market_view_map_[t_security_id_]->OnTrade(t_trade_price_, t_trade_size_, t_buysell_);
}

std::string OSEOrderLevelMarketViewManager::ShowOrderDepthBook(const OSEOrderMarketView &this_omv_) const {
  std::ostringstream t_temp_oss_;

  unsigned max_levels_ = std::max(this_omv_.bid_order_depth_book_.size(), this_omv_.ask_order_depth_book_.size());

  for (unsigned index_ = 0; index_ < max_levels_; ++index_) {
    t_temp_oss_ << "lvl:" << (index_ + 1) << "    ";

    if (index_ < this_omv_.bid_order_depth_book_.size() && this_omv_.bid_order_depth_book_[index_]) {
      t_temp_oss_ << this_omv_.bid_order_depth_book_[index_]->ToString();
    } else {
      t_temp_oss_ << "                              ";
    }

    t_temp_oss_ << "\t";

    if (index_ < this_omv_.ask_order_depth_book_.size() && this_omv_.ask_order_depth_book_[index_]) {
      t_temp_oss_ << this_omv_.ask_order_depth_book_[index_]->ToString();
    } else {
      t_temp_oss_ << "                              ";
    }

    t_temp_oss_ << "\n";
  }

  return t_temp_oss_.str();
}

void OSEOrderLevelMarketViewManager::resetBook(int sec_id_) {
  if (sec_id_ < 0 || sec_id_ >= (int)security_id_to_ose_omv_map_.size()) {
    // Probably garbage security or one that we are not interested in.
    return;
  }

  // Since the OSEMapOrder *s are allocated by the simple mempool, we need to free each of thenm individually.
  for (std::vector<OSEMapOrder *>::iterator _order_itr_ =
           security_id_to_ose_omv_map_[sec_id_].bid_order_depth_book_.begin();
       _order_itr_ != security_id_to_ose_omv_map_[sec_id_].bid_order_depth_book_.end(); ++_order_itr_) {
    if (*_order_itr_) {
      osemaporder_mempool_.DeAlloc((*_order_itr_));
    }
  }

  for (std::vector<OSEMapOrder *>::iterator _order_itr_ =
           security_id_to_ose_omv_map_[sec_id_].ask_order_depth_book_.begin();
       _order_itr_ != security_id_to_ose_omv_map_[sec_id_].ask_order_depth_book_.end(); ++_order_itr_) {
    if (*_order_itr_) {
      osemaporder_mempool_.DeAlloc((*_order_itr_));
    }
  }

  // Now empty the vector.
  security_id_to_ose_omv_map_[sec_id_].bid_order_depth_book_.clear();
  security_id_to_ose_omv_map_[sec_id_].ask_order_depth_book_.clear();

  SecurityMarketView &this_smv_ = *(security_market_view_map_[sec_id_]);

  this_smv_.EmptyBook();
}
}
