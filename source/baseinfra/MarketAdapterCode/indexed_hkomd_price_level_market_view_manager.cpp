/**
   \file MarketAdapter/indexed_hkomd_price_level_market_view_manager.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

/* aggregate order book management

   BookUpdateMessage :    MsgType:353
   UpdateAction :: New(0)/Delete(2)/Change(1)/Clear(74) [ If subscribed to DF, clear message is sent MsgType:335 ]
   PriceLeveL :: 1 - 10 ( 255 => Aggregated Remaining Liquidity ( 11 + levels )
   Side : Bid(0)/Offer(1)
   NumberOfOrders : NULL for price_level == 255
   AggregateQuantity
   NoEntries

   TradeMessage : MsgType:350
   OrderID : 0 ( not available )
   Price
   TradeID
   Side 0(not available)/1(not defined)/2(buy order)/3(sell order)
   DealType 0(None)/1(Printable)/2(Occurred at Cross)/4(Reported Trade)
   TradeCondition 0(none)/1(late trade)/2(internal trade / crossing)/8(buy write)/16(off market)
   max_non_stale_levels_ 10

   356(trade Amendment)/360(trade statistics)/363(series statistics).....
 */

#include "baseinfra/MarketAdapter/indexed_hkomd_price_level_market_view_manager.hpp"
#define CCPROFILING_TRADEINIT 0

namespace HFSAT {

#define LOW_ACCESS_INDEX 50

IndexedHKOMDPriceLevelMarketViewManager::IndexedHKOMDPriceLevelMarketViewManager(
    DebugLogger& t_dbglogger_, const Watch& t_watch_, const SecurityNameIndexer& t_sec_name_indexer_,
    const std::vector<SecurityMarketView*>& t_security_market_view_map_)
    : BaseMarketViewManager(t_dbglogger_, t_watch_, t_sec_name_indexer_, t_security_market_view_map_),
      l1_price_changed_(t_sec_name_indexer_.NumSecurityId(), false),
      l1_size_changed_(t_sec_name_indexer_.NumSecurityId(), false) {}

void IndexedHKOMDPriceLevelMarketViewManager::OnPriceLevelNew(
    const unsigned int t_security_id_, const TradeType_t t_buysell_, const int t_level_added_, const double t_price_,
    const int t_new_size_, const int t_new_ordercount_,
    //								    const bool t_trade_delete_,
    const bool t_is_intermediate_message_) {
  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);

  int int_price_ = smv_.GetIntPx(t_price_);

  if (!smv_.initial_book_constructed_) {
    BuildIndex(t_security_id_, t_buysell_, int_price_);
  }

  int bid_index_ = 0;
  int ask_index_ = 0;

  int old_size_ = 0;
  int old_ordercount_ = 0;

  switch (t_buysell_) {
    case kTradeTypeBuy: {
      bid_index_ = (int)smv_.base_bid_index_ -
                   (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_ - int_price_);

      // if there are no levels currently
      if (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_ <= 0) {
        if (bid_index_ < LOW_ACCESS_INDEX) {
          RebuildIndexLowAccess(t_security_id_, t_buysell_, int_price_);
          bid_index_ = smv_.base_bid_index_;
        } else if (bid_index_ >= (int)smv_.max_tick_range_) {
          RebuildIndexHighAccess(t_security_id_, t_buysell_, int_price_);

          if ((bid_index_ - smv_.base_bid_index_) < smv_.bitmask_size_) {
            smv_.bid_access_bitmask_ >>= (bid_index_ - smv_.base_bid_index_);
          } else {
            smv_.bid_access_bitmask_ = BIT_RESET_ALL;
          }
          bid_index_ = smv_.base_bid_index_;
        } else {
          // bid_index_ falls within the range. So, set base_bid_index_ as bid_index_
          smv_.base_bid_index_ = bid_index_;
        }
        l1_price_changed_[t_security_id_] = true;
      } else if (bid_index_ < 0) {
        // Skip the message if we don't have any info for this level
        return;
      }

      if (bid_index_ >= (int)smv_.max_tick_range_) {
        RebuildIndexHighAccess(t_security_id_, t_buysell_, int_price_);

        if ((bid_index_ - smv_.base_bid_index_) < smv_.bitmask_size_) {
          smv_.bid_access_bitmask_ >>= (bid_index_ - smv_.base_bid_index_);
        } else {
          smv_.bid_access_bitmask_ = BIT_RESET_ALL;
        }

        bid_index_ = smv_.base_bid_index_;
        l1_price_changed_[t_security_id_] = true;
      }

      old_size_ = smv_.market_update_info_.bidlevels_[bid_index_].limit_size_;
      old_ordercount_ = smv_.market_update_info_.bidlevels_[bid_index_].limit_ordercount_;

      // Update the size and order count
      smv_.market_update_info_.bidlevels_[bid_index_].limit_size_ = t_new_size_;
      smv_.market_update_info_.bidlevels_[bid_index_].limit_ordercount_ = t_new_ordercount_;

      if (bid_index_ > (int)smv_.base_bid_index_) {
        // Bitmask - again make sure we never change the order of base indexed assignment
        if ((bid_index_ - smv_.base_bid_index_) < smv_.bitmask_size_) {
          smv_.bid_access_bitmask_ >>= ((bid_index_ - smv_.base_bid_index_));
        } else {
          smv_.bid_access_bitmask_ = BIT_RESET_ALL;
        }

        smv_.bid_level_change_bitmask_ = 0xFFFF;
        smv_.base_bid_index_ = bid_index_;

        // Update BitMask - Actually this will just setup 1st bit
        if ((smv_.base_bid_index_ - bid_index_) < smv_.bitmask_size_) {
          smv_.bid_access_bitmask_ |= smv_.bitmask_lookup_table_[smv_.base_bid_index_ - bid_index_];
        }
        // Sanitise ASK side
        if (SanitizeAskSide(t_security_id_) == kBookManagerReturn) {
          // Not returning here, anyways the book is not ready as of now, so price wouldn't be propagated
          smv_.is_ready_ = false;
        }
        l1_price_changed_[t_security_id_] = true;
      } else if (bid_index_ == (int)smv_.base_bid_index_) {
        // Update BitMask - Actually this will just setup 1st bit
        if ((smv_.base_bid_index_ - bid_index_) < smv_.bitmask_size_) {
          smv_.bid_access_bitmask_ |= smv_.bitmask_lookup_table_[smv_.base_bid_index_ - bid_index_];
        }

        smv_.bid_level_change_bitmask_ = smv_.bid_level_change_bitmask_ | 0x0001;
        l1_size_changed_[t_security_id_] = true;
      } else {
        // Update BitMask
        if ((smv_.base_bid_index_ - bid_index_) < smv_.bitmask_size_) {
          smv_.bid_access_bitmask_ |= smv_.bitmask_lookup_table_[smv_.base_bid_index_ - bid_index_];
        }

        smv_.bid_level_change_bitmask_ = smv_.bid_level_change_bitmask_ | (1 << (smv_.base_bid_index_ - bid_index_));
        smv_.l2_changed_since_last_ = true;
      }

      if (l1_price_changed_[t_security_id_] || l1_size_changed_[t_security_id_]) {
        UpdateBestBidVariables(t_security_id_, smv_.base_bid_index_);

        UpdateBestBidVariablesUsingOurOrders(t_security_id_);
      }
    } break;
    case kTradeTypeSell: {
      ask_index_ = (int)smv_.base_ask_index_ +
                   (smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_ - int_price_);

      // There are 0 levels on the ask side
      if (smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_ <= 0) {
        if (ask_index_ < LOW_ACCESS_INDEX) {
          RebuildIndexLowAccess(t_security_id_, t_buysell_, int_price_);
          ask_index_ = smv_.base_ask_index_;
        } else if (ask_index_ >= (int)smv_.max_tick_range_) {
          RebuildIndexHighAccess(t_security_id_, t_buysell_, int_price_);

          if ((ask_index_ - smv_.base_ask_index_) < smv_.bitmask_size_) {
            smv_.ask_access_bitmask_ >>= (ask_index_ - smv_.base_ask_index_);
          } else {
            smv_.ask_access_bitmask_ = BIT_RESET_ALL;
          }

          ask_index_ = smv_.base_ask_index_;
        } else {
          // ask_index_ falls within the range. So, set base_ask_index_ as ask_index_
          smv_.base_ask_index_ = ask_index_;
        }

        l1_price_changed_[t_security_id_] = true;
      } else if (ask_index_ < 0) {
        // Skip the message if we don't have info for this level
        return;
      }

      if (ask_index_ >= (int)smv_.max_tick_range_) {
        RebuildIndexHighAccess(t_security_id_, t_buysell_, int_price_);

        if ((ask_index_ - smv_.base_ask_index_) < smv_.bitmask_size_) {
          smv_.ask_access_bitmask_ >>= (ask_index_ - smv_.base_ask_index_);
        } else {
          smv_.ask_access_bitmask_ = BIT_RESET_ALL;
        }

        ask_index_ = smv_.base_ask_index_;

        l1_price_changed_[t_security_id_] = true;
      }

      old_size_ = smv_.market_update_info_.asklevels_[ask_index_].limit_size_;
      old_ordercount_ = smv_.market_update_info_.asklevels_[ask_index_].limit_ordercount_;

      // Update the size and order count at corresponding level
      smv_.market_update_info_.asklevels_[ask_index_].limit_ordercount_ = t_new_ordercount_;
      smv_.market_update_info_.asklevels_[ask_index_].limit_size_ = t_new_size_;

      if (ask_index_ > (int)smv_.base_ask_index_) {
        // Bitmask - again make sure we never change the order of base indexe assignment
        if ((ask_index_ - smv_.base_ask_index_) < smv_.bitmask_size_) {
          smv_.ask_access_bitmask_ >>= ((ask_index_ - smv_.base_ask_index_));
        } else {
          smv_.ask_access_bitmask_ = BIT_RESET_ALL;
        }

        smv_.ask_level_change_bitmask_ = 0xFFFF;

        smv_.base_ask_index_ = ask_index_;

        // Update BitMask
        if ((smv_.base_ask_index_ - ask_index_) < smv_.bitmask_size_) {
          smv_.ask_access_bitmask_ |= smv_.bitmask_lookup_table_[smv_.base_ask_index_ - ask_index_];
        }

        // Sanitise BID side
        if (SanitizeBidSide(t_security_id_) == kBookManagerReturn) {
          // Not returning here, anyways the book is not ready as of now, so price wouldn't be propagated
          smv_.is_ready_ = false;
        }

        l1_price_changed_[t_security_id_] = true;

      } else if (ask_index_ == (int)smv_.base_ask_index_) {
        // Update BitMask
        if ((smv_.base_ask_index_ - ask_index_) < smv_.bitmask_size_) {
          smv_.ask_access_bitmask_ |= smv_.bitmask_lookup_table_[smv_.base_ask_index_ - ask_index_];
        }

        smv_.ask_level_change_bitmask_ = smv_.ask_level_change_bitmask_ | 0x0001;

        l1_size_changed_[t_security_id_] = true;
      } else {
        // Update BitMask
        if ((smv_.base_ask_index_ - ask_index_) < smv_.bitmask_size_) {
          smv_.ask_access_bitmask_ |= smv_.bitmask_lookup_table_[smv_.base_ask_index_ - ask_index_];
        }

        smv_.ask_level_change_bitmask_ = smv_.ask_level_change_bitmask_ | (1 << (smv_.base_ask_index_ - ask_index_));

        smv_.l2_changed_since_last_ = true;
      }

      if (l1_price_changed_[t_security_id_] || l1_size_changed_[t_security_id_]) {
        UpdateBestAskVariables(t_security_id_, smv_.base_ask_index_);

        UpdateBestAskVariablesUsingOurOrders(t_security_id_);
      }
    } break;
    default:
      break;
  }
  if (l1_price_changed_[t_security_id_] || l1_size_changed_[t_security_id_]) {
    smv_.UpdateL1Prices();
  }

  if (!smv_.is_ready_) {
    if (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_ > 0 &&
        smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_ > 0 && smv_.ArePricesComputed()) {
      smv_.is_ready_ = true;
    }
  }

  if (!smv_.is_ready_ || t_is_intermediate_message_) {
    return;
  }

#if CCPROFILING_TRADEINIT
  HFSAT::CpucycleProfiler::GetUniqueInstance(30).End(13);
#endif

  if (smv_.pl_change_listeners_present_) {
    if (t_buysell_ == kTradeTypeBuy && (int)smv_.base_bid_index_ >= bid_index_ &&
        (int)smv_.base_bid_index_ - bid_index_ <= 10) {
      int level_ = smv_.base_bid_index_ - bid_index_;
      int int_price_level_ = smv_.base_bid_index_ - bid_index_;
      smv_.NotifyOnPLChangeListeners(t_security_id_, smv_.market_update_info_, t_buysell_, level_, int_price_,
                                     int_price_level_, old_size_,
                                     smv_.market_update_info_.bidlevels_[bid_index_].limit_size_, old_ordercount_,
                                     smv_.market_update_info_.bidlevels_[bid_index_].limit_ordercount_,
                                     t_is_intermediate_message_, old_size_ == 0 ? 'N' : 'C');
    } else if (t_buysell_ == kTradeTypeSell && (int)smv_.base_ask_index_ >= ask_index_ &&
               (int)smv_.base_ask_index_ - ask_index_ <= 10) {
      int level_ = smv_.base_ask_index_ - ask_index_;
      int int_price_level_ = smv_.base_ask_index_ - ask_index_;
      smv_.NotifyOnPLChangeListeners(t_security_id_, smv_.market_update_info_, t_buysell_, level_, int_price_,
                                     int_price_level_, old_size_,
                                     smv_.market_update_info_.asklevels_[ask_index_].limit_size_, old_ordercount_,
                                     smv_.market_update_info_.asklevels_[ask_index_].limit_ordercount_,
                                     t_is_intermediate_message_, old_size_ == 0 ? 'N' : 'C');
    }
  }

  if (l1_price_changed_[t_security_id_]) {
    smv_.NotifyL1PriceListeners();
    l1_price_changed_[t_security_id_] = false;
    l1_size_changed_[t_security_id_] = false;
  } else if (l1_size_changed_[t_security_id_]) {
    smv_.NotifyL1SizeListeners();

    l1_size_changed_[t_security_id_] = false;
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

void IndexedHKOMDPriceLevelMarketViewManager::OnPriceLevelChange(const unsigned int t_security_id_,
                                                                 const TradeType_t t_buysell_,
                                                                 const int t_level_changed_, const double t_price_,
                                                                 const int t_new_size_, const int t_new_ordercount_,
                                                                 const bool t_is_intermediate_message_) {
  OnPriceLevelNew(t_security_id_, t_buysell_, t_level_changed_, t_price_, t_new_size_, t_new_ordercount_,
                  t_is_intermediate_message_);
}

void IndexedHKOMDPriceLevelMarketViewManager::OnPriceLevelDelete(const unsigned int t_security_id_,
                                                                 const TradeType_t t_buysell_,
                                                                 const int t_level_removed_, const double t_price_,
                                                                 const bool t_is_intermediate_message_) {
  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);

  int int_price_ = smv_.GetIntPx(t_price_);

  if (!smv_.initial_book_constructed_) {
    BuildIndex(t_security_id_, t_buysell_, int_price_);
  }

  int bid_index_ = 0;
  int ask_index_ = 0;

  int old_size_ = 0;
  int old_ordercount_ = 0;

  switch (t_buysell_) {
    case kTradeTypeBuy: {
      smv_.bid_level_change_bitmask_ = 0xFFFF;

      bid_index_ = (int)smv_.base_bid_index_ -
                   (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_ - int_price_);

      // Skip the message if we don't have any info for this level
      if (bid_index_ < 0 || bid_index_ > (int)smv_.base_bid_index_) {
        return;
      }

      old_size_ = smv_.market_update_info_.bidlevels_[bid_index_].limit_size_;
      old_ordercount_ = smv_.market_update_info_.bidlevels_[bid_index_].limit_ordercount_;

      smv_.market_update_info_.bidlevels_[bid_index_].limit_size_ = 0;
      smv_.market_update_info_.bidlevels_[bid_index_].limit_ordercount_ = 0;

      if (bid_index_ == (int)smv_.base_bid_index_) {
        l1_price_changed_[t_security_id_] = true;

        // Reconfigure the base_bid_index_
        int next_index_ = (int)smv_.base_bid_index_ - 1;
        for (; next_index_ >= 0 && smv_.market_update_info_.bidlevels_[next_index_].limit_size_ <= 0; next_index_--)
          ;

        // Update the base_bid_index if next_index is valid. Otherwise, ignore.
        if (next_index_ >= 0) {
          // Base Shifted, time to shift bitmask
          if ((smv_.base_bid_index_ - next_index_) < smv_.bitmask_size_) {
            smv_.bid_access_bitmask_ <<= (smv_.base_bid_index_ - next_index_);
          } else {
            smv_.bid_access_bitmask_ = BIT_RESET_ALL;
          }

          smv_.base_bid_index_ = (unsigned int)next_index_;

          if (smv_.base_bid_index_ < LOW_ACCESS_INDEX) {
            RebuildIndexLowAccess(t_security_id_, t_buysell_,
                                  smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_);
          }

          UpdateBestBidVariables(t_security_id_, smv_.base_bid_index_);

          UpdateBestBidVariablesUsingOurOrders(t_security_id_);
        } else {
          smv_.bid_access_bitmask_ = BIT_RESET_ALL;
          smv_.is_ready_ = false;
        }
      } else {
        if ((smv_.base_bid_index_ - bid_index_) < smv_.bitmask_size_) {
          smv_.bid_access_bitmask_ &= (~(smv_.bitmask_lookup_table_[(smv_.base_bid_index_ - bid_index_)]));
        }
        smv_.l2_changed_since_last_ = true;
      }
    } break;
    case kTradeTypeSell: {
      smv_.ask_level_change_bitmask_ = 0xFFFF;

      ask_index_ = (int)smv_.base_ask_index_ +
                   (smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_ - int_price_);

      // Skip the message if we don't have any info for this level
      if (ask_index_ < 0 || ask_index_ > (int)smv_.base_ask_index_) {
        return;
      }

      old_size_ = smv_.market_update_info_.asklevels_[ask_index_].limit_size_;
      old_ordercount_ = smv_.market_update_info_.asklevels_[ask_index_].limit_ordercount_;

      smv_.market_update_info_.asklevels_[ask_index_].limit_size_ = 0;
      smv_.market_update_info_.asklevels_[ask_index_].limit_ordercount_ = 0;

      if (ask_index_ == (int)smv_.base_ask_index_) {
        l1_price_changed_[t_security_id_] = true;

        // Reconfigure the base_ask_index_
        int next_index_ = (int)smv_.base_ask_index_ - 1;
        for (; next_index_ >= 0 && smv_.market_update_info_.asklevels_[next_index_].limit_size_ <= 0; next_index_--)
          ;

        // Update the base_ask_index if next_index is valid. Otherwise, ignore.
        if (next_index_ >= 0) {
          // Base Shifted, time to shift bitmask
          if ((smv_.base_ask_index_ - next_index_) < smv_.bitmask_size_) {
            smv_.ask_access_bitmask_ <<= (smv_.base_ask_index_ - next_index_);
          } else {
            smv_.ask_access_bitmask_ = BIT_RESET_ALL;
          }

          smv_.base_ask_index_ = (unsigned int)next_index_;

          if (smv_.base_ask_index_ < LOW_ACCESS_INDEX) {
            RebuildIndexLowAccess(t_security_id_, t_buysell_,
                                  smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_);
          }

          UpdateBestAskVariables(t_security_id_, smv_.base_ask_index_);
          UpdateBestAskVariablesUsingOurOrders(t_security_id_);
        } else {
          smv_.ask_access_bitmask_ = BIT_RESET_ALL;
          smv_.is_ready_ = false;
        }
      } else {
        if ((smv_.base_ask_index_ - ask_index_) < smv_.bitmask_size_) {
          smv_.ask_access_bitmask_ &= (~(smv_.bitmask_lookup_table_[(smv_.base_ask_index_ - ask_index_)]));
        }
        smv_.l2_changed_since_last_ = true;
      }
    } break;
    default:
      break;
  }

  if (l1_price_changed_[t_security_id_] || l1_size_changed_[t_security_id_]) {
    smv_.UpdateL1Prices();
  }

  if (!smv_.is_ready_) {
    if (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_ > 0 &&
        smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_ > 0 && smv_.ArePricesComputed()) {
      smv_.is_ready_ = true;
    }
  }

  if (!smv_.is_ready_ || t_is_intermediate_message_) {
    return;
  }

#if CCPROFILING_TRADEINIT
  HFSAT::CpucycleProfiler::GetUniqueInstance(30).End(13);
#endif

  if (smv_.pl_change_listeners_present_) {
    if (t_buysell_ == kTradeTypeBuy && (int)smv_.base_bid_index_ >= bid_index_ &&
        (int)smv_.base_bid_index_ - bid_index_ <= 10) {
      int level_ = smv_.base_bid_index_ - bid_index_;
      int int_price_level_ = smv_.base_bid_index_ - bid_index_;
      smv_.NotifyOnPLChangeListeners(t_security_id_, smv_.market_update_info_, t_buysell_, level_, int_price_,
                                     int_price_level_, old_size_, 0, old_ordercount_, 0, t_is_intermediate_message_,
                                     'D');
    } else if (t_buysell_ == kTradeTypeSell && (int)smv_.base_ask_index_ >= ask_index_ &&
               (int)smv_.base_ask_index_ - ask_index_ <= 10) {
      int level_ = smv_.base_ask_index_ - ask_index_;
      int int_price_level_ = smv_.base_ask_index_ - ask_index_;
      smv_.NotifyOnPLChangeListeners(t_security_id_, smv_.market_update_info_, t_buysell_, level_, int_price_,
                                     int_price_level_, old_size_, 0, old_ordercount_, 0, t_is_intermediate_message_,
                                     'D');
    }
  }

  if (l1_price_changed_[t_security_id_]) {
    smv_.NotifyL1PriceListeners();

    l1_price_changed_[t_security_id_] = false;
    l1_size_changed_[t_security_id_] = false;
  } else if (l1_size_changed_[t_security_id_]) {
    smv_.NotifyL1SizeListeners();

    l1_size_changed_[t_security_id_] = false;
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

void IndexedHKOMDPriceLevelMarketViewManager::OnTrade(const unsigned int t_security_id_, const double t_trade_price_,
                                                      const int t_trade_size_, const TradeType_t t_buysell_) {
  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);
  int int_price_ = smv_.GetIntPx(t_trade_price_);

  // we expect to receive corresoniding PriceLevelChange/PriceLevelDelete after after trade
  // just mask top variables
  if (t_buysell_ == kTradeTypeBuy)  // passive bid side
  {
    int bid_index_ = (int)smv_.base_bid_index_ -
                     (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_ - int_price_);

    if (bid_index_ < 0) {
      return;
    }

    if (bid_index_ < (int)smv_.base_bid_index_) {
      DBGLOG_TIME_CLASS_FUNC << "OnTrade @nonbestbid, security_id: " << t_security_id_ << "trade_px: " << t_trade_price_
                             << " side: " << t_buysell_ << "trade_size: " << t_trade_size_ << DBGLOG_ENDL_FLUSH;

      if (smv_.market_update_info_.bidlevels_[bid_index_].limit_size_ <= t_trade_size_) {
        int index_ = bid_index_ - 1;
        for (; index_ >= 0 && smv_.market_update_info_.bidlevels_[index_].limit_size_ <= 0; index_--)
          ;
        if (index_ > 0) {
          UpdateBestBidVariables(t_security_id_, index_);
          UpdateBestBidVariablesUsingOurOrders(t_security_id_);
        }
      } else {
        smv_.market_update_info_.bestbid_int_price_ = smv_.market_update_info_.bidlevels_[bid_index_].limit_int_price_;
        smv_.market_update_info_.bestbid_price_ = smv_.market_update_info_.bidlevels_[bid_index_].limit_price_;
        smv_.market_update_info_.bestbid_size_ =
            smv_.market_update_info_.bidlevels_[bid_index_].limit_size_ - t_trade_size_;
        smv_.market_update_info_.bestbid_ordercount_ =
            smv_.market_update_info_.bidlevels_[bid_index_].limit_ordercount_;
        UpdateBestBidVariablesUsingOurOrders(t_security_id_);
      }
    } else if (bid_index_ == (int)smv_.base_bid_index_) {
      if (smv_.market_update_info_.bestbid_size_ <= t_trade_size_) {
        int index_ = bid_index_ - 1;
        for (; index_ >= 0 && smv_.market_update_info_.bidlevels_[index_].limit_size_ <= 0; index_--)
          ;
        if (index_ > 0) {
          UpdateBestBidVariables(t_security_id_, index_);
          UpdateBestBidVariablesUsingOurOrders(t_security_id_);
        }
      } else {
        smv_.market_update_info_.bestbid_size_ -= t_trade_size_;
      }
    }
  } else if (t_buysell_ == kTradeTypeSell) {
    int ask_index_ = (int)smv_.base_ask_index_ +
                     (smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_ - int_price_);

    if (ask_index_ < 0) {
      return;
    }

    if (ask_index_ < (int)smv_.base_ask_index_) {
      DBGLOG_TIME_CLASS_FUNC << "OnTrade @nonbestbid, security_id: " << t_security_id_ << "trade_px: " << t_trade_price_
                             << " side: " << t_buysell_ << "trade_size: " << t_trade_size_ << DBGLOG_ENDL_FLUSH;
      if (smv_.market_update_info_.asklevels_[ask_index_].limit_size_ <= t_trade_size_) {
        int index_ = ask_index_ - 1;
        for (; (index_ >= 0) && (smv_.market_update_info_.asklevels_[index_].limit_size_ <= 0); index_--)
          ;
        if (index_ >= 0) {
          UpdateBestAskVariables(t_security_id_, index_);
          UpdateBestAskVariablesUsingOurOrders(t_security_id_);
        }
      } else {
        UpdateBestAskVariables(t_security_id_, ask_index_);
        UpdateBestAskVariablesUsingOurOrders(t_security_id_);
      }
    } else if (ask_index_ == (int)smv_.base_ask_index_) {
      if (smv_.market_update_info_.bestask_size_ <= t_trade_size_) {
        int index_ = ask_index_ - 1;
        for (; (index_ >= 0) && (smv_.market_update_info_.asklevels_[index_].limit_size_ <= 0); index_--)
          ;
        if (index_ >= 0) {
          UpdateBestAskVariables(t_security_id_, index_);
          UpdateBestAskVariablesUsingOurOrders(t_security_id_);
        }
      } else {
        smv_.market_update_info_.bestask_size_ -= t_trade_size_;
      }
    }
  }

  smv_.trade_print_info_.trade_price_ = t_trade_price_;
  smv_.trade_print_info_.size_traded_ = t_trade_size_;
  smv_.trade_print_info_.int_trade_price_ = smv_.GetIntPx(t_trade_price_);
  smv_.trade_print_info_.buysell_ =
      (t_buysell_ == kTradeTypeBuy ? kTradeTypeSell : kTradeTypeBuy);  // since we get passive one
  smv_.SetTradeVarsForIndicatorsIfRequired();

#if CCPROFILING_TRADEINIT
  HFSAT::CpucycleProfiler::GetUniqueInstance(30).End(13);
#endif

  if (smv_.is_ready_) {
    smv_.NotifyTradeListeners();
    smv_.NotifyOnReadyListeners();
  }
}
}
