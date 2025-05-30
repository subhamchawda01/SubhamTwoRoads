/**
   \file MarketAdapterCode/indexed_ntp_market_view_manager.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 162, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#include "baseinfra/MarketAdapter/indexed_ntp_market_view_manager.hpp"
#define CCPROFILING_TRADEINIT 1

namespace HFSAT {

#define LOW_ACCESS_INDEX 50

IndexedNtpMarketViewManager::IndexedNtpMarketViewManager(DebugLogger& dbglogger, const Watch& watch,
                                                         const SecurityNameIndexer& sec_name_indexer,
                                                         const std::vector<SecurityMarketView*>& smv_map)
    : BaseMarketViewManager(dbglogger, watch, sec_name_indexer, smv_map),
      sec_id_to_prev_update_was_quote_(sec_name_indexer.NumSecurityId(), false),
      last_intermediate_update_(sec_name_indexer.NumSecurityId(), kTradeTypeNoInfo) {}

void IndexedNtpMarketViewManager::SanitizeBook(SecurityMarketView& smv, bool intermediate) {
  if (!intermediate) {
    int sec_id = smv.security_id();
    if (last_intermediate_update_[sec_id] == kTradeTypeBuy) {
      smv.SanitizeAskSide(smv.GetL1BidIntPrice());

    } else if (last_intermediate_update_[sec_id] == kTradeTypeSell) {
      smv.SanitizeBidSide(smv.GetL1AskIntPrice());
    }

    last_intermediate_update_[sec_id] = kTradeTypeNoInfo;
  }
}

void IndexedNtpMarketViewManager::DropIndexedBookForSource(HFSAT::ExchSource_t t_exch_source, const int sec_id) {
  SecurityMarketView& smv = *(security_market_view_map_[sec_id]);

  HFSAT::ExchSource_t this_exch_source =
      HFSAT::SecurityDefinitions::GetContractExchSource(smv.shortcode(), watch_.YYYYMMDD());

  if (this_exch_source != t_exch_source) return;

  smv.market_update_info_.bidlevels_.clear();
  smv.market_update_info_.asklevels_.clear();
}

/*
 * NTP market data price level new notes:
 * 1. Implicit delete for levels beyond 10
 */
void IndexedNtpMarketViewManager::OnPriceLevelNew(const unsigned int sec_id, const TradeType_t buysell,
                                                  const int level_added, const double price, const int new_size,
                                                  const int new_ordercount, const bool intermediate) {
  SecurityMarketView& smv_ = *(security_market_view_map_[sec_id]);

  int int_price_ = smv_.GetIntPx(price);

  if (!smv_.initial_book_constructed_) {
    BuildIndex(sec_id, buysell, int_price_);
    smv_.initial_book_constructed_ = true;
  }

  int old_size = 0;
  int old_ordercount = 0;

  switch (buysell) {
    case kTradeTypeBuy: {
      int bid_index_ = smv_.GetBidIndex(int_price_);

      // There are 0 levels on bid side
      if (smv_.GetL1BidSize() <= 0) {
        if (bid_index_ < LOW_ACCESS_INDEX) {
          RebuildIndexLowAccess(sec_id, buysell, int_price_);
          bid_index_ = smv_.base_bid_index_;

        } else if (bid_index_ >= (int)smv_.max_tick_range_) {
          RebuildIndexHighAccess(sec_id, buysell, int_price_);
          smv_.DownBidBitmask(bid_index_);
          bid_index_ = smv_.base_bid_index_;
        }

        smv_.base_bid_index_ = bid_index_;

      } else if (bid_index_ < 0) {
        // Skip the message if we don't have any info for this level
        return;
      }

      if (bid_index_ >= (int)smv_.max_tick_range_) {
        RebuildIndexHighAccess(sec_id, buysell, int_price_);
        smv_.DownBidBitmask(bid_index_);
        bid_index_ = smv_.base_bid_index_;
      }

      smv_.SanitizeBidLevel1(level_added, bid_index_);
      smv_.SanitizeBidLastLevel(level_added, 10, bid_index_);

      // Store old size and order count for OnPL change listeners. (Not sure if having a bool check will help)
      old_size = smv_.GetBidSize(bid_index_);
      old_ordercount = smv_.GetBidOrders(bid_index_);

      smv_.UpdateBidLevel(bid_index_, new_size, new_ordercount);

      if (bid_index_ >= (int)smv_.base_bid_index_) {
        if (bid_index_ > (int)smv_.base_bid_index_) {
          smv_.l1_price_changed_ = true;

        } else {
          smv_.l1_size_changed_ = true;
        }

        smv_.DownBidBitmask(bid_index_);
        smv_.NewBidLevelmask(bid_index_);

        smv_.base_bid_index_ = bid_index_;

        smv_.AddBidBitmask(bid_index_);

        // In NTP/PUMA feed, it's possible that we receive book delta messages of bid/ask side in
        // different order. This creates a problem, since delete level doesn't provide price information
        // Therefore, skip sanitization code for intermediate messages
        if (!intermediate) {
          smv_.SanitizeAskSide(int_price_);
          last_intermediate_update_[sec_id] = kTradeTypeNoInfo;

        } else {
          last_intermediate_update_[sec_id] = kTradeTypeBuy;
        }

      } else {
        smv_.l2_changed_since_last_ = true;
        smv_.AddBidBitmask(bid_index_);
        smv_.bid_level_change_bitmask_ = smv_.bid_level_change_bitmask_ | (1 << (smv_.base_bid_index_ - bid_index_));
      }
      break;
    }

    case kTradeTypeSell: {
      int ask_index_ = smv_.GetAskIndex(int_price_);

      // There are 0 levels on ask side
      if (smv_.GetL1AskSize() <= 0) {
        if (ask_index_ < LOW_ACCESS_INDEX) {
          RebuildIndexLowAccess(sec_id, buysell, int_price_);
          ask_index_ = smv_.base_ask_index_;
        } else if (ask_index_ >= (int)smv_.max_tick_range_) {
          RebuildIndexHighAccess(sec_id, buysell, int_price_);
          smv_.DownAskBitmask(ask_index_);
          ask_index_ = smv_.base_ask_index_;
        }

        smv_.base_ask_index_ = ask_index_;
      } else if (ask_index_ < 0) {
        return;
      }

      if (ask_index_ >= (int)smv_.max_tick_range_) {
        RebuildIndexHighAccess(sec_id, buysell, int_price_);
        smv_.DownAskBitmask(ask_index_);
        ask_index_ = smv_.base_ask_index_;
      }

      smv_.SanitizeAskLevel1(level_added, ask_index_);
      smv_.SanitizeAskLastLevel(level_added, 10, ask_index_);

      old_size = smv_.GetAskSize(ask_index_);
      old_ordercount = smv_.GetAskOrders(ask_index_);

      smv_.UpdateAskLevel(ask_index_, new_size, new_ordercount);

      if (ask_index_ >= (int)smv_.base_ask_index_) {
        if (ask_index_ > (int)smv_.base_ask_index_) {
          smv_.l1_price_changed_ = true;

        } else {
          smv_.l1_size_changed_ = true;
        }

        smv_.DownAskBitmask(ask_index_);
        smv_.NewAskLevelmask(ask_index_);

        smv_.base_ask_index_ = ask_index_;

        smv_.AddAskBitmask(ask_index_);

        // In NTP/PUMA feed, it's possible that we receive book delta messages of bid/ask side in
        // different order. This creates a problem, since delete level doesn't provide price information
        // Therefore, skip sanitization code for intermediate messages
        if (!intermediate) {
          smv_.SanitizeBidSide(int_price_);
          last_intermediate_update_[sec_id] = kTradeTypeNoInfo;

        } else {
          last_intermediate_update_[sec_id] = kTradeTypeSell;
        }
      } else {
        smv_.l2_changed_since_last_ = true;
        smv_.AddAskBitmask(ask_index_);
        smv_.ask_level_change_bitmask_ = smv_.ask_level_change_bitmask_ | (1 << (smv_.base_ask_index_ - ask_index_));
      }
      break;
    }
    default:
      break;
  }

  SanitizeBook(smv_, intermediate);

  if (smv_.l1_price_changed_ || smv_.l1_size_changed_) {
    if (buysell == kTradeTypeBuy ||
        (buysell == kTradeTypeSell &&
         smv_.market_update_info_.bestbid_int_price_ >= smv_.market_update_info_.bestask_int_price_)) {
      smv_.UpdateBestBid(smv_.base_bid_index_);
      UpdateBestBidVariablesUsingOurOrders(sec_id);
      smv_.UpdateL1Prices();
    }
    if (buysell == kTradeTypeSell ||
        (buysell == kTradeTypeBuy &&
         smv_.market_update_info_.bestbid_int_price_ >= smv_.market_update_info_.bestask_int_price_)) {
      smv_.UpdateBestAsk(smv_.base_ask_index_);
      UpdateBestAskVariablesUsingOurOrders(sec_id);
      smv_.UpdateL1Prices();
    }
  }

  sec_id_to_prev_update_was_quote_[sec_id] = true;

  if (!smv_.is_ready_) {
    if (smv_.GetL1BidSize() > 0 && smv_.GetL1AskSize() > 0 && smv_.ArePricesComputed()) {
      smv_.is_ready_ = true;

    } else {
      return;
    }
  }

  if (smv_.market_update_info_.trade_update_implied_quote_) {
    smv_.market_update_info_.trade_update_implied_quote_ = false;
    smv_.trade_print_info_.num_trades_++;
  }

  if (intermediate) {
    return;
  }

#if CCPROFILING_TRADEINIT
  HFSAT::CpucycleProfiler::GetUniqueInstance(30).End(19);
#endif

  if (smv_.pl_change_listeners_present_) {
    smv_.NotifyOnPLChangeListeners(sec_id, smv_.market_update_info_, buysell, level_added - 1, int_price_, 0, old_size,
                                   new_size, old_ordercount, new_ordercount, intermediate, old_size == 0 ? 'N' : 'C');
  }

  if (smv_.l1_price_changed_) {
    smv_.NotifyL1PriceListeners();
    smv_.l1_price_changed_ = false;
    smv_.l1_size_changed_ = false;

  } else if (smv_.l1_size_changed_) {
    smv_.NotifyL1SizeListeners();
    smv_.l1_size_changed_ = false;

  } else {
    smv_.NotifyL2Listeners();
  }

  if (smv_.l2_changed_since_last_) {
    smv_.NotifyL2OnlyListeners();
    smv_.l2_changed_since_last_ = false;
  }

  smv_.NotifyOnReadyListeners();

  smv_.bid_level_change_bitmask_ = 0x0000;
  smv_.ask_level_change_bitmask_ = 0x0000;
}

void IndexedNtpMarketViewManager::OnPriceLevelChange(const unsigned int sec_id, const TradeType_t buysell,
                                                     const int level_changed, const double price, const int new_size,
                                                     const int new_ordercount, const bool intermediate_message) {
  OnPriceLevelNew(sec_id, buysell, level_changed, price, new_size, new_ordercount, intermediate_message);
}

/*
 * NTP Price level delete notes:
 * 1. We don't get price information here.
 */
void IndexedNtpMarketViewManager::OnPriceLevelDelete(const unsigned int sec_id, const TradeType_t buysell,
                                                     const int level_removed, const double price,
                                                     const bool intermediate) {
  // Skip levels more than 10
  if (level_removed > 10) {
    return;
  }

  SecurityMarketView& smv = *(security_market_view_map_[sec_id]);

  if (!smv.initial_book_constructed_) {
    return;
  }

  int old_size = 0;
  int old_ordercount = 0;
  int int_price = 0;

  switch (buysell) {
    case kTradeTypeBuy: {
      int bid_index = smv.base_bid_index_;

      // TODO: Optimize following function - only iterate through first 15-20 levels to reduce the effect of worst case
      for (int temp_level = 1; temp_level < level_removed; temp_level++) {
        // Get next index
        do {
          bid_index--;
        } while (bid_index >= 0 && smv.GetBidSize(bid_index) <= 0);

        if (bid_index < 0) {
          smv.is_ready_ = false;
          return;
        }
      }

      int_price = smv.GetBidIntPrice(bid_index);
      old_size = smv.GetBidSize(bid_index);
      old_ordercount = smv.GetBidOrders(bid_index);

      smv.DeleteBid(int_price);
      break;
    }

    case kTradeTypeSell: {
      int ask_index = smv.base_ask_index_;

      // TODO: Optimize
      for (int temp_level = 1; temp_level < level_removed; temp_level++) {
        // Get next index
        do {
          ask_index--;
        } while (ask_index >= 0 && smv.GetAskSize(ask_index) <= 0);

        if (ask_index < 0) {
          smv.is_ready_ = false;
          return;
        }
      }

      int_price = smv.GetAskIntPrice(ask_index);

      old_size = smv.GetAskSize(ask_index);
      old_ordercount = smv.GetAskOrders(ask_index);

      smv.DeleteAsk(int_price);
      break;
    }
    default:
      break;
  }

  SanitizeBook(smv, intermediate);

  sec_id_to_prev_update_was_quote_[sec_id] = true;

  if (smv.market_update_info_.trade_update_implied_quote_) {
    smv.market_update_info_.trade_update_implied_quote_ = false;
    smv.trade_print_info_.num_trades_++;
  }

#if CCPROFILING_TRADEINIT
  HFSAT::CpucycleProfiler::GetUniqueInstance(30).End(19);
#endif

  smv.NotifyDeleteListeners(buysell, int_price, level_removed, old_size, old_ordercount, intermediate);
}

void IndexedNtpMarketViewManager::OnPriceLevelDeleteThru(const unsigned int sec_id, const TradeType_t buysell,
                                                         const bool intermediate) {
  SecurityMarketView& smv = *(security_market_view_map_[sec_id]);

  if (!smv.initial_book_constructed_) {
    return;
  }

  switch (buysell) {
    case kTradeTypeBuy: {
      int bid_index = smv.base_bid_index_;

      for (int index = smv.base_bid_index_; index + 20 >= bid_index; index--) {
        smv.ResetBidLevel(index);
      }

      smv.bid_access_bitmask_ = BIT_RESET_ALL;
      break;
    }

    case kTradeTypeSell: {
      int ask_index = smv.base_ask_index_;

      for (int index = smv.base_ask_index_; index + 20 >= ask_index; index--) {
        smv.ResetAskLevel(index);
      }

      smv.ask_access_bitmask_ = BIT_RESET_ALL;
      break;
    }
    default:
      break;
  }

  smv.is_ready_ = false;
}

void IndexedNtpMarketViewManager::OnPriceLevelDeleteFrom(const unsigned int sec_id, const TradeType_t buysell,
                                                         const int max_level_removed, const double price,
                                                         const bool intermediate) {
  SecurityMarketView& smv = *(security_market_view_map_[sec_id]);

  if (!smv.initial_book_constructed_) {
    return;
  }

  switch (buysell) {
    case kTradeTypeBuy: {
      int bid_index = smv.base_bid_index_;
      int levels_deleted = 0;

      smv.bid_level_change_bitmask_ = 0xFFFF;

      while (bid_index >= 0 && levels_deleted < max_level_removed) {
        if (smv.GetBidSize(bid_index) > 0) {
          smv.ResetBidLevel(bid_index);
          levels_deleted++;
        }
        bid_index--;
      }

      for (; bid_index >= 0 && smv.GetBidSize(bid_index) <= 0; bid_index--)
        ;

      if (bid_index < 0) {
        smv.bid_access_bitmask_ = BIT_RESET_ALL;
        smv.is_ready_ = false;
        break;
      }

      smv.UpBidBitmask(bid_index);
      smv.base_bid_index_ = bid_index;

      if (smv.base_bid_index_ < LOW_ACCESS_INDEX) {
        RebuildIndexLowAccess(sec_id, kTradeTypeBuy, smv.GetL1BidIntPrice());
      }

      smv.UpdateBestBid(smv.base_bid_index_);
      UpdateBestBidVariablesUsingOurOrders(sec_id);
      break;
    }

    case kTradeTypeSell: {
      int ask_index = smv.base_ask_index_;
      int levels_deleted = 0;

      smv.ask_level_change_bitmask_ = 0xFFFF;

      while (ask_index >= 0 && levels_deleted < max_level_removed) {
        if (smv.GetAskSize(ask_index) > 0) {
          smv.ResetAskLevel(ask_index);
          levels_deleted++;
        }
        ask_index--;
      }

      for (; ask_index >= 0 && smv.GetAskSize(ask_index) <= 0; ask_index--)
        ;

      if (ask_index < 0) {
        smv.ask_access_bitmask_ = BIT_RESET_ALL;
        smv.is_ready_ = false;
        break;
      }

      smv.UpAskBitmask(ask_index);

      smv.base_ask_index_ = ask_index;

      if (smv.base_ask_index_ < LOW_ACCESS_INDEX) {
        RebuildIndexLowAccess(sec_id, kTradeTypeSell, smv.GetL1AskIntPrice());
      }

      smv.UpdateBestAsk(smv.base_ask_index_);
      UpdateBestAskVariablesUsingOurOrders(sec_id);
      break;
    }
    default:
      break;
  }

  SanitizeBook(smv, intermediate);

  sec_id_to_prev_update_was_quote_[sec_id] = true;

#if CCPROFILING_TRADEINIT
  HFSAT::CpucycleProfiler::GetUniqueInstance(30).End(19);
#endif

  if (!intermediate && smv.is_ready_) {
    smv.UpdateL1Prices();
    smv.NotifyL1PriceListeners();
    smv.NotifyOnReadyListeners();

    smv.bid_level_change_bitmask_ = 0x0000;
    smv.ask_level_change_bitmask_ = 0x0000;
  }
}

/*
 * NTP Trade message notes:
 * 1. We don't get aggressor side here.
 * 2. Discard levels above the trade price
 */
void IndexedNtpMarketViewManager::OnTrade(const unsigned int t_security_id_, const double t_trade_price_,
                                          const int t_trade_size_) {
  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);

  // not till book is ready
  if ((smv_.market_update_info_.bestbid_int_price_ <= kInvalidIntPrice) ||
      (smv_.market_update_info_.bestask_int_price_ <= kInvalidIntPrice)) {
    return;
  }

  int int_trade_price_ = smv_.GetIntPx(t_trade_price_);

  // same as SecurityMarketView::OnTrade
  if (smv_.trade_print_info_.computing_last_book_tdiff_) {
    if (sec_id_to_prev_update_was_quote_[t_security_id_]) {
      // noting the mktpx as it was just before the first trade message
      smv_.market_update_info_.last_book_mkt_size_weighted_price_ = smv_.market_update_info_.mkt_size_weighted_price_;
    }
  }

  if (!smv_.market_update_info_.trade_update_implied_quote_) {
    smv_.market_update_info_.trade_update_implied_quote_ = true;
  }

  smv_.StorePreTrade();

  TradeType_t buysell_ = kTradeTypeNoInfo;

  // Determine side
  if (int_trade_price_ <= smv_.GetL1BidIntPrice()) {
    buysell_ = kTradeTypeSell;

  } else if (int_trade_price_ >= smv_.GetL1AskIntPrice()) {
    buysell_ = kTradeTypeBuy;

  } else {
    return;
  }

  switch (buysell_) {
    case kTradeTypeBuy: {  // Aggressive sell

      int trade_ask_index_ = smv_.GetAskIndex(int_trade_price_);

      if (trade_ask_index_ > (int)smv_.base_ask_index_ || trade_ask_index_ < 0) {
        return;
      }

      // If the last update was trade, handle this trade differently
      if (!sec_id_to_prev_update_was_quote_[t_security_id_]) {
        if (int_trade_price_ <= smv_.market_update_info_.bestask_int_price_) {
          if (int_trade_price_ == smv_.market_update_info_.bestask_int_price_) {
            // Update best variables using already set best variables
            if (t_trade_size_ >= smv_.market_update_info_.bestask_size_) {
              int next_ask_index_ = trade_ask_index_ - 1;
              for (; next_ask_index_ >= 0 && smv_.market_update_info_.asklevels_[next_ask_index_].limit_size_ <= 0;
                   next_ask_index_--)
                ;

              if (next_ask_index_ < 0) {
                return;
              }

              smv_.UpdateBestAsk(next_ask_index_);
              UpdateBestAskVariablesUsingOurOrders(t_security_id_);
            } else {
              smv_.market_update_info_.bestask_size_ -= t_trade_size_;
            }
          }
        } else {
          smv_.market_update_info_.bestask_int_price_ = int_trade_price_;
          smv_.market_update_info_.bestask_ordercount_ = 1;
          smv_.market_update_info_.bestask_price_ = smv_.GetDoublePx(int_trade_price_);
          smv_.market_update_info_.bestask_size_ = 1;
        }

        break;
      }

      // At this point, trade_ask_index_ == smv_.base_ask_index_ has to hold.

      if (t_trade_size_ >= smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_) {
        int next_ask_index_ = smv_.base_ask_index_ - 1;
        for (; next_ask_index_ >= 0 && smv_.market_update_info_.asklevels_[next_ask_index_].limit_size_ <= 0;
             next_ask_index_--)
          ;

        if (next_ask_index_ < 0) {
          return;
        }

        smv_.UpdateBestAsk(next_ask_index_);
        UpdateBestAskVariablesUsingOurOrders(t_security_id_);
      } else {
        smv_.market_update_info_.bestask_int_price_ =
            smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_;
        smv_.market_update_info_.bestask_ordercount_ =
            smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_ordercount_;
        smv_.market_update_info_.bestask_price_ =
            smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_price_;
        smv_.market_update_info_.bestask_size_ =
            smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_ - t_trade_size_;
        UpdateBestAskVariablesUsingOurOrders(t_security_id_);
      }
    } break;
    case kTradeTypeSell:  // Aggressive buy
    {
      int trade_bid_index_ =
          smv_.base_bid_index_ -
          (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_ - int_trade_price_);

      if (trade_bid_index_ > (int)smv_.base_bid_index_ || trade_bid_index_ < 0) {
        return;
      }

      // If the last update was trade, handle this trade differently
      if (!sec_id_to_prev_update_was_quote_[t_security_id_]) {
        if (int_trade_price_ >= smv_.market_update_info_.bestbid_int_price_) {
          if (int_trade_price_ == smv_.market_update_info_.bestbid_int_price_) {
            // Update best variables using already set best variables
            if (t_trade_size_ >= smv_.market_update_info_.bestbid_size_) {
              int next_bid_index_ = trade_bid_index_ - 1;
              for (; next_bid_index_ >= 0 && smv_.market_update_info_.bidlevels_[next_bid_index_].limit_size_ <= 0;
                   next_bid_index_--)
                ;

              if (next_bid_index_ < 0) {
                return;
              }

              smv_.UpdateBestBid(next_bid_index_);
              UpdateBestBidVariablesUsingOurOrders(t_security_id_);
            } else {
              smv_.market_update_info_.bestbid_size_ -= t_trade_size_;
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

      // At this point, trade_bid_index_ == smv_.base_bid_index_ has to hold.

      if (t_trade_size_ >= smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_) {
        int next_bid_index_ = smv_.base_bid_index_ - 1;
        for (; next_bid_index_ >= 0 && smv_.market_update_info_.bidlevels_[next_bid_index_].limit_size_ <= 0;
             next_bid_index_--)
          ;

        if (next_bid_index_ < 0) {
          return;
        }

        smv_.UpdateBestBid(next_bid_index_);
        UpdateBestBidVariablesUsingOurOrders(t_security_id_);
      } else {
        smv_.market_update_info_.bestbid_int_price_ =
            smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_;
        smv_.market_update_info_.bestbid_ordercount_ =
            smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_ordercount_;
        smv_.market_update_info_.bestbid_price_ =
            smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_price_;
        smv_.market_update_info_.bestbid_size_ =
            smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_ - t_trade_size_;
        UpdateBestBidVariablesUsingOurOrders(t_security_id_);
      }
    } break;
    default:
      break;
  }

  SanitizeBook(smv_, false);

  smv_.UpdateL1Prices();

  // Set the trade variables
  smv_.trade_print_info_.trade_price_ = t_trade_price_;
  smv_.trade_print_info_.size_traded_ = t_trade_size_;
  smv_.trade_print_info_.int_trade_price_ = int_trade_price_;
  smv_.trade_print_info_.buysell_ = buysell_;

  smv_.SetTradeVarsForIndicatorsIfRequired();

#if CCPROFILING_TRADEINIT
  HFSAT::CpucycleProfiler::GetUniqueInstance(30).End(19);
#endif

  if (smv_.is_ready_) {
    smv_.NotifyTradeListeners();
    smv_.NotifyOnReadyListeners();
  }

  sec_id_to_prev_update_was_quote_[t_security_id_] = false;
}

void IndexedNtpMarketViewManager::OnOTCTrade(const unsigned int sec_id, const double trade_price,
                                             const int trade_size) {
  SecurityMarketView& smv = *(security_market_view_map_[sec_id]);

  // not till book is ready
  if ((smv.market_update_info_.bestbid_int_price_ <= kInvalidIntPrice) ||
      (smv.market_update_info_.bestask_int_price_ <= kInvalidIntPrice)) {
    return;
  }

  smv.trade_print_info_.otc_trade_price_ = trade_price;
  smv.trade_print_info_.otc_trade_size_ = trade_size;

#if CCPROFILING_TRADEINIT
  HFSAT::CpucycleProfiler::GetUniqueInstance(30).End(19);
#endif

  if (smv.is_ready_) {
    smv.NotifyOTCTradeListeners(trade_price, trade_size);
  }
}

void IndexedNtpMarketViewManager::OnMarketStatusUpdate(const unsigned int sec_id, const MktStatus_t mkt_status) {
  SecurityMarketView& smv = *(security_market_view_map_[sec_id]);

#if CCPROFILING_TRADEINIT
  HFSAT::CpucycleProfiler::GetUniqueInstance(30).End(19);
#endif

  smv.NotifyMarketStatusListeners(sec_id, mkt_status);
}
}
