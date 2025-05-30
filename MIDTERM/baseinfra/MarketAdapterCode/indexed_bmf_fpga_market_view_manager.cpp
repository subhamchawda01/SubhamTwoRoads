/**
   \file MarketAdapterCode/indexed_fpga_market_view_manager.cpp

   \author: (c) Copyright K2 Research LLC 2010
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/

#include "baseinfra/MarketAdapter/indexed_bmf_fpga_market_view_manager.hpp"

namespace HFSAT {

#define LOW_ACCESS_INDEX 50
#define MSECS_TO_MASK_SYN_DEL 100

IndexedBMFFpgaMarketViewManager::IndexedBMFFpgaMarketViewManager(
    DebugLogger& t_dbglogger_, const Watch& t_watch_, SecurityNameIndexer& t_sec_name_indexer_,
    const std::vector<SecurityMarketView*>& t_security_market_view_map_)
    : BaseMarketViewManager(t_dbglogger_, t_watch_, t_sec_name_indexer_, t_security_market_view_map_),
      sec_id_to_prev_update_was_quote_(t_sec_name_indexer_.NumSecurityId(), false),
      sec_id_to_curr_status_(t_sec_name_indexer_.NumSecurityId(), kMktTradingStatusOpen),
      sec_id_prev_best_lvl_info_(t_sec_name_indexer_.NumSecurityId()) {}

BookManagerErrorCode_t IndexedBMFFpgaMarketViewManager::UpdateBook(const unsigned int security_id,
                                                                   FPGA_MDS::BMFFPGABookDeltaStruct* full_book,
                                                                   SecurityMarketView& smv, bool& l1_px_changed,
                                                                   bool& l1_sz_changed) {
  MarketUpdateInfoLevelStruct* bid_level = &smv.market_update_info_.bidlevels_[smv.base_bid_index_];
  int prev_bid_size_ = bid_level->limit_size_;
  int prev_bid_int_price_ = bid_level->limit_int_price_;

  MarketUpdateInfoLevelStruct* ask_level = &smv.market_update_info_.asklevels_[smv.base_ask_index_];
  int prev_ask_size_ = ask_level->limit_size_;
  int prev_ask_int_price_ = ask_level->limit_int_price_;

  sec_id_prev_best_lvl_info_[security_id].Set(prev_bid_int_price_, prev_ask_int_price_, prev_bid_size_, prev_ask_size_,
                                              bid_level->limit_ordercount_, ask_level->limit_ordercount_);

#if BMF_FPGA_BOOK_DEBUG
  std::cout << watch_.tv_ToString() << "BidSize: " << full_book->bid_size << std::endl;
#endif

  for (int i = 0; i < std::min(full_book->bid_size, BMF_FPGA_MAX_BOOK_SIZE); i++) {
    int bid_int_price_ = smv.GetIntPx(full_book->bids[i].price);

    int bid_index_ = smv.base_bid_index_ -
                     (smv.market_update_info_.bidlevels_[smv.base_bid_index_].limit_int_price_ - bid_int_price_);

#if BMF_FPGA_BOOK_DEBUG
    std::cout << watch_.tv_ToString() << "Level:  " << i << " , BidPrice: " << full_book->bids[i].price
              << " Orders: " << full_book->bids[i].number_of_orders << " , BidIntPx: " << bid_int_price_
              << " SMVBase: " << smv.base_bid_index_ << " BidIndex: " << bid_index_ << std::endl;
#endif

    // L1 update
    if (i == 0) {
      BookManagerErrorCode_t error_ = AdjustBidIndex(security_id, bid_int_price_, bid_index_);

      if (error_ == kBookManagerReturn) {
        return error_;
      }

      // Delete the levels above this level
      while ((int)smv.base_bid_index_ > bid_index_) {
        smv.market_update_info_.bidlevels_[smv.base_bid_index_].limit_size_ = 0;
        smv.base_bid_index_--;
      }

      smv.base_bid_index_ = bid_index_;
    } else {
      if (smv.market_update_info_.bidlevels_[smv.base_bid_index_].limit_size_ <= 0 || bid_index_ < 0) {
        return kBookManagerReturn;
      }
    }
    // If we have masked this level due to synthetic delete, then don't update this level as this was deleted
    smv.market_update_info_.bidlevels_[bid_index_].limit_size_ = full_book->bids[i].size;
    smv.market_update_info_.bidlevels_[bid_index_].limit_ordercount_ = full_book->bids[i].number_of_orders;
  }

#if BMF_FPGA_BOOK_DEBUG
  std::cout << watch_.tv_ToString() << "AskSize: " << full_book->ask_size << std::endl;
#endif

  for (int i = 0; i < std::min(full_book->ask_size, BMF_FPGA_MAX_BOOK_SIZE); i++) {
    int ask_int_price_ = smv.GetIntPx(full_book->offers[i].price);

    int ask_index_ = smv.base_ask_index_ +
                     (smv.market_update_info_.asklevels_[smv.base_ask_index_].limit_int_price_ - ask_int_price_);

#if BMF_FPGA_BOOK_DEBUG
    std::cout << watch_.tv_ToString() << "Level:  " << i << " , AskPrice: " << full_book->offers[i].price
              << " Orders: " << full_book->offers[i].number_of_orders << " , AskIntPx: " << ask_int_price_
              << " SMVBase: " << smv.base_ask_index_ << " AskIndex: " << ask_index_ << std::endl;
#endif

    // L1 Update
    if (i == 0) {
      BookManagerErrorCode_t error_ = AdjustAskIndex(security_id, ask_int_price_, ask_index_);

      if (error_ == kBookManagerReturn) {
        return error_;
      }

      // Delete the levels above this level
      while ((int)smv.base_ask_index_ > ask_index_) {
        smv.market_update_info_.asklevels_[smv.base_ask_index_].limit_size_ = 0;
        smv.base_ask_index_--;
      }

      smv.base_ask_index_ = ask_index_;
    } else {
      if (smv.market_update_info_.asklevels_[smv.base_ask_index_].limit_size_ <= 0 || ask_index_ < 0) {
        return kBookManagerReturn;
      }
    }
    // If we have masked this level due to synthetic delete, then don't update this level as this was deleted
    smv.market_update_info_.asklevels_[ask_index_].limit_size_ = full_book->offers[i].size;
    smv.market_update_info_.asklevels_[ask_index_].limit_ordercount_ = full_book->offers[i].number_of_orders;
  }

  bid_level = &smv.market_update_info_.bidlevels_[smv.base_bid_index_];
  int new_bid_size_ = bid_level->limit_size_;
  int new_bid_int_price_ = bid_level->limit_int_price_;

  ask_level = &smv.market_update_info_.asklevels_[smv.base_ask_index_];
  int new_ask_size_ = ask_level->limit_size_;
  int new_ask_int_price_ = ask_level->limit_int_price_;

  if ((new_bid_int_price_ != prev_bid_int_price_) || (new_ask_int_price_ != prev_ask_int_price_)) {
    l1_px_changed = true;
#if BMF_FPGA_BOOK_DEBUG
    std::cout << watch_.tv_ToString() << "L1PxChanged" << std::endl;
#endif
  }

  if ((new_bid_size_ != prev_bid_size_) || (new_ask_size_ != prev_ask_size_)) {
    l1_sz_changed = true;
#if BMF_FPGA_BOOK_DEBUG
    std::cout << watch_.tv_ToString() << "L1SzChanged" << std::endl;
#endif
  }

  if (l1_px_changed || l1_sz_changed) {
    return kBookManagerL1Changed;
  }

  return kBookManagerOK;
}

bool IndexedBMFFpgaMarketViewManager::IsMarketUpdateSame(const SecurityMarketView& smv_,
                                                         BestLevelInfo& prev_best_level) {
  if ((smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_ == prev_best_level.best_bid_int_px) &&
      (smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_ == prev_best_level.best_ask_int_px) &&
      (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_ == prev_best_level.bid_size) &&
      (smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_ == prev_best_level.ask_size) &&
      (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_ordercount_ == prev_best_level.bid_orders) &&
      (smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_ordercount_ == prev_best_level.ask_orders)) {
    return true;
  }
  return false;
}

void IndexedBMFFpgaMarketViewManager::OnFullBookChange(const unsigned int security_id,
                                                       FPGA_MDS::BMFFPGABookDeltaStruct* full_book,
                                                       bool is_intermediate, bool is_mkt_closed) {
  if (sec_id_to_curr_status_[security_id] != kMktTradingStatusOpen) return;

  if (is_mkt_closed) return;

  SecurityMarketView& smv_ = *(security_market_view_map_[security_id]);
#if BMF_FPGA_BOOK_DEBUG
  std::cout << watch_.tv_ToString() << " OnFullBookChange : Base: " << smv_.base_bid_index_ << " : "
            << smv_.base_ask_index_ << " Best: " << smv_.market_update_info_.bestbid_int_price_ << " X "
            << smv_.market_update_info_.bestask_int_price_ << std::endl;
#endif

  if (!smv_.initial_book_constructed_) {
    // Build the initial index only from the first level
    // For that, check if the level one info is preset
    // in the current update.

    if (full_book->bid_size > 0 && full_book->ask_size > 0) {
      // Ignore crossed book update
      if (full_book->bids[0].price >= full_book->offers[0].price) return;
      if (full_book->bid_size > 0) {
        BuildIndex(security_id, kTradeTypeBuy, smv_.GetIntPx(full_book->bids[0].price));
        smv_.initial_book_constructed_ = true;
      } else if (full_book->ask_size > 0) {
        BuildIndex(security_id, kTradeTypeSell, smv_.GetIntPx(full_book->offers[0].price));
        smv_.initial_book_constructed_ = true;
      } else {
        return;
      }
    } else {
      return;
    }
  }

  bool l1_px_changed = false;
  bool l1_size_changed = false;

  BookManagerErrorCode_t update_error_ = kBookManagerOK;

  update_error_ = UpdateBook(security_id, full_book, smv_, l1_px_changed, l1_size_changed);

  if (update_error_ == kBookManagerReturn) {
#if BMF_FPGA_BOOK_DEBUG
    std::cout << watch_.tv_ToString() << " kBookManagerReturn " << std::endl;
#endif
    return;
  }

  if (update_error_ == kBookManagerL1Changed) {
    UpdateBestBidVariables(security_id, smv_.base_bid_index_);
    UpdateBestAskVariables(security_id, smv_.base_ask_index_);
    smv_.UpdateL1Prices();
  }

  if (!IsMarketUpdateSame(smv_, sec_id_prev_best_lvl_info_[security_id])) {
    sec_id_to_prev_update_was_quote_[security_id] = true;
  }

  if (smv_.market_update_info_.trade_update_implied_quote_) {
    smv_.market_update_info_.trade_update_implied_quote_ = false;
    smv_.trade_print_info_.num_trades_++;
  }

  if (!smv_.is_ready_) {
    if (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_ > 0 &&
        smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_ > 0 && smv_.ArePricesComputed()) {
#if BMF_FPGA_BOOK_DEBUG
      std::cout << watch_.tv_ToString() << " SMV Gets Ready! " << std::endl;
#endif
      smv_.is_ready_ = true;
    } else {
#if BMF_FPGA_BOOK_DEBUG
      std::cout << watch_.tv_ToString() << " SMVNotReady : Return " << std::endl;
#endif
      return;
    }
  }

  if (NotificationAllowed(smv_, is_intermediate, security_id)) {
    if (l1_px_changed) {
#if BMF_FPGA_BOOK_DEBUG
      std::cout << watch_.tv_ToString() << " NotifyL1PriceListeners " << std::endl;
#endif
      smv_.NotifyL1PriceListeners();
    } else if (l1_size_changed) {
      smv_.NotifyL1SizeListeners();
#if BMF_FPGA_BOOK_DEBUG
      std::cout << watch_.tv_ToString() << " NotifyL1SizeListeners " << std::endl;
#endif
    } else {
#if BMF_FPGA_BOOK_DEBUG
      std::cout << watch_.tv_ToString() << " NotifyL2Listeners " << std::endl;
#endif
      smv_.NotifyL2Listeners();
      smv_.NotifyL2OnlyListeners();
    }

    smv_.NotifyOnReadyListeners();
  }
}

void IndexedBMFFpgaMarketViewManager::OnTrade(const unsigned int security_id, const double trade_price,
                                              const int trade_size, TradeType_t buy_sell, bool is_mkt_closed) {
#if BMF_FPGA_BOOK_DEBUG
  std::cout << watch_.tv_ToString() << " OnTrade " << std::endl;
#endif

  if (sec_id_to_curr_status_[security_id] != kMktTradingStatusOpen) return;

  SecurityMarketView& smv_ = *(security_market_view_map_[security_id]);

  // not till book is ready
  if ((smv_.market_update_info_.bestbid_int_price_ <= kInvalidIntPrice) ||
      (smv_.market_update_info_.bestask_int_price_ <= kInvalidIntPrice)) {
    return;
  }

  int int_trade_price_ = smv_.GetIntPx(trade_price);

  // same as SecurityMarketView::OnTrade
  if (smv_.trade_print_info_.computing_last_book_tdiff_) {
    if (sec_id_to_prev_update_was_quote_[security_id]) {
      // the last time the update was a book message
      smv_.market_update_info_.last_book_mkt_size_weighted_price_ = smv_.market_update_info_.mkt_size_weighted_price_;
    }
  }

  if (!smv_.market_update_info_.trade_update_implied_quote_) {
    smv_.market_update_info_.trade_update_implied_quote_ = true;
  }

  smv_.StorePreTrade();

  BestLevelInfo& prev_best_level = sec_id_prev_best_lvl_info_[security_id];

  // Determine side as BMF doesn't send side of the trade
  if (int_trade_price_ <= prev_best_level.best_bid_int_px) {
    buy_sell = kTradeTypeSell;
  } else if (int_trade_price_ >= prev_best_level.best_ask_int_px) {
    buy_sell = kTradeTypeBuy;
  } else {
    buy_sell = kTradeTypeNoInfo;
  }

  bool trade_before_quote = IsMarketUpdateSame(smv_, prev_best_level);

  if (trade_before_quote) {
    switch (buy_sell) {
      case kTradeTypeBuy:  // Aggressive buy
      {
        int trade_ask_index_ =
            smv_.base_ask_index_ +
            (smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_ - int_trade_price_);

        if (trade_ask_index_ > (int)smv_.base_ask_index_ || trade_ask_index_ < 0) {
          break;
        }

        // If the last update was trade, handle this trade differently
        if (!sec_id_to_prev_update_was_quote_[security_id]) {
          if (int_trade_price_ <= smv_.market_update_info_.bestask_int_price_) {
            if (int_trade_price_ == smv_.market_update_info_.bestask_int_price_) {
              // Update best variables using already set best variables
              if (trade_size >= smv_.market_update_info_.bestask_size_) {
                int next_ask_index_ = trade_ask_index_ - 1;
                for (; next_ask_index_ >= 0 && smv_.market_update_info_.asklevels_[next_ask_index_].limit_size_ <= 0;
                     next_ask_index_--)
                  ;

                if (next_ask_index_ < 0) {
                  return;
                }

                smv_.market_update_info_.bestask_int_price_ =
                    smv_.market_update_info_.asklevels_[next_ask_index_].limit_int_price_;
                smv_.market_update_info_.bestask_ordercount_ =
                    smv_.market_update_info_.asklevels_[next_ask_index_].limit_ordercount_;
                smv_.market_update_info_.bestask_price_ =
                    smv_.market_update_info_.asklevels_[next_ask_index_].limit_price_;
                smv_.market_update_info_.bestask_size_ =
                    smv_.market_update_info_.asklevels_[next_ask_index_].limit_size_;
                UpdateBestAskVariablesUsingOurOrders(security_id);
              } else {
                smv_.market_update_info_.bestask_size_ -= trade_size;
              }
            }
          } else {
            smv_.market_update_info_.bestask_int_price_ = int_trade_price_;
            smv_.market_update_info_.bestask_ordercount_ = 1;
            smv_.market_update_info_.bestask_size_ = 1;
            smv_.market_update_info_.bestask_price_ = smv_.GetDoublePx(int_trade_price_);
          }
          break;
        }

        if (trade_size >= smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_) {
          int next_ask_index_ = smv_.base_ask_index_ - 1;
          for (; next_ask_index_ >= 0 && smv_.market_update_info_.asklevels_[next_ask_index_].limit_size_ <= 0;
               next_ask_index_--)
            ;

          if (next_ask_index_ < 0) {
            return;
          }

          smv_.market_update_info_.bestask_int_price_ =
              smv_.market_update_info_.asklevels_[next_ask_index_].limit_int_price_;
          smv_.market_update_info_.bestask_ordercount_ =
              smv_.market_update_info_.asklevels_[next_ask_index_].limit_ordercount_;
          smv_.market_update_info_.bestask_price_ = smv_.market_update_info_.asklevels_[next_ask_index_].limit_price_;
          smv_.market_update_info_.bestask_size_ = smv_.market_update_info_.asklevels_[next_ask_index_].limit_size_;
        } else {
          smv_.market_update_info_.bestask_int_price_ =
              smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_;
          smv_.market_update_info_.bestask_ordercount_ =
              smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_ordercount_;
          smv_.market_update_info_.bestask_price_ =
              smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_price_;
          smv_.market_update_info_.bestask_size_ =
              smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_ - trade_size;
        }
        UpdateBestAskVariablesUsingOurOrders(security_id);
      } break;
      case kTradeTypeSell:  // Aggressive sell
      {
        int trade_bid_index_ =
            smv_.base_bid_index_ -
            (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_ - int_trade_price_);

        if (trade_bid_index_ > (int)smv_.base_bid_index_ || trade_bid_index_ < 0) {
          break;
        }

        // If the last update was trade, handle this trade differently
        if (!sec_id_to_prev_update_was_quote_[security_id]) {
          if (int_trade_price_ >= smv_.market_update_info_.bestbid_int_price_) {
            if (int_trade_price_ == smv_.market_update_info_.bestbid_int_price_) {
              // Update best variables using already set best variables
              if (trade_size >= smv_.market_update_info_.bestbid_size_) {
                int next_bid_index_ = trade_bid_index_ - 1;
                for (; next_bid_index_ >= 0 && smv_.market_update_info_.bidlevels_[next_bid_index_].limit_size_ <= 0;
                     next_bid_index_--)
                  ;

                if (next_bid_index_ < 0) {
                  return;
                }

                smv_.market_update_info_.bestbid_int_price_ =
                    smv_.market_update_info_.bidlevels_[next_bid_index_].limit_int_price_;
                smv_.market_update_info_.bestbid_ordercount_ =
                    smv_.market_update_info_.bidlevels_[next_bid_index_].limit_ordercount_;
                smv_.market_update_info_.bestbid_price_ =
                    smv_.market_update_info_.bidlevels_[next_bid_index_].limit_price_;
                smv_.market_update_info_.bestbid_size_ =
                    smv_.market_update_info_.bidlevels_[next_bid_index_].limit_size_;
                UpdateBestBidVariablesUsingOurOrders(security_id);
              } else {
                smv_.market_update_info_.bestbid_size_ -= trade_size;
              }
            }
          } else {
            smv_.market_update_info_.bestbid_int_price_ = int_trade_price_;
            smv_.market_update_info_.bestbid_ordercount_ = 1;
            smv_.market_update_info_.bestbid_price_ = smv_.GetDoublePx(int_trade_price_);
            smv_.market_update_info_.bestbid_size_ = 1;
          }
          break;
        }

        if (trade_size >= smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_) {
          int next_bid_index_ = smv_.base_bid_index_ - 1;
          for (; next_bid_index_ >= 0 && smv_.market_update_info_.bidlevels_[next_bid_index_].limit_size_ <= 0;
               next_bid_index_--)
            ;

          if (next_bid_index_ < 0) {
            return;
          }

          smv_.market_update_info_.bestbid_int_price_ =
              smv_.market_update_info_.bidlevels_[next_bid_index_].limit_int_price_;
          smv_.market_update_info_.bestbid_ordercount_ =
              smv_.market_update_info_.bidlevels_[next_bid_index_].limit_ordercount_;
          smv_.market_update_info_.bestbid_price_ = smv_.market_update_info_.bidlevels_[next_bid_index_].limit_price_;
          smv_.market_update_info_.bestbid_size_ = smv_.market_update_info_.bidlevels_[next_bid_index_].limit_size_;
        } else {
          smv_.market_update_info_.bestbid_int_price_ =
              smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_;
          smv_.market_update_info_.bestbid_ordercount_ =
              smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_ordercount_;
          smv_.market_update_info_.bestbid_price_ =
              smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_price_;
          smv_.market_update_info_.bestbid_size_ =
              smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_ - trade_size;
        }
        UpdateBestBidVariablesUsingOurOrders(security_id);
      } break;
      default:
        break;
    }
  }

  smv_.UpdateL1Prices();

  // Set the trade variables
  smv_.trade_print_info_.trade_price_ = trade_price;
  smv_.trade_print_info_.size_traded_ = trade_size;
  smv_.trade_print_info_.int_trade_price_ = int_trade_price_;
  smv_.trade_print_info_.buysell_ = buy_sell;

  smv_.SetTradeVarsForIndicatorsIfRequired();

  if (NotificationAllowed(smv_, false, security_id)) {
    smv_.NotifyTradeListeners();
    smv_.NotifyOnReadyListeners();
  }

  sec_id_to_prev_update_was_quote_[security_id] = false;
}

bool IndexedBMFFpgaMarketViewManager::NotificationAllowed(const SecurityMarketView& smv, bool is_intermediate,
                                                          const unsigned int security_id) {
  if (is_intermediate || !smv.is_ready_ || (sec_id_to_curr_status_[security_id] != kMktTradingStatusOpen)) {
    return false;
  }
  return true;
}

void IndexedBMFFpgaMarketViewManager::OnMarketStatusUpdate(const unsigned int security_id,
                                                           const MktStatus_t mkt_status) {
#if BMF_FPGA_BOOK_DEBUG
  std::cout << watch_.tv_ToString() << " Status: " << HFSAT::MktTradingStatusStr(mkt_status) << std::endl;
#endif
  sec_id_to_curr_status_[security_id] = mkt_status;
}
}
