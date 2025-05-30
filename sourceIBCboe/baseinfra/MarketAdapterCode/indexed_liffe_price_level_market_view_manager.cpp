/**
    \file MarketAdapter/indexed_eobi_price_level_market_view_manager.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */

#include "baseinfra/MarketAdapter/indexed_liffe_price_level_market_view_manager.hpp"
#define CCPROFILING_TRADEINIT 0
namespace HFSAT {

#define LOW_ACCESS_INDEX 50

IndexedLiffePriceLevelMarketViewManager::IndexedLiffePriceLevelMarketViewManager(
    DebugLogger& t_dbglogger_, const Watch& t_watch_, const SecurityNameIndexer& t_sec_name_indexer_,
    const std::vector<SecurityMarketView*>& t_security_market_view_map_)
    : BaseMarketViewManager(t_dbglogger_, t_watch_, t_sec_name_indexer_, t_security_market_view_map_),
      l1_price_changed_(t_sec_name_indexer_.NumSecurityId(), false),
      l1_size_changed_(t_sec_name_indexer_.NumSecurityId(), false),
      valid_trade_seen_(t_sec_name_indexer_.NumSecurityId(), false),
      last_trade_price_(t_sec_name_indexer_.NumSecurityId(), 0),
      last_l1_delete_was_bid_(t_sec_name_indexer_.NumSecurityId(), false),
      last_l1_delete_int_price_(t_sec_name_indexer_.NumSecurityId(), 0),
      last_update_was_delete_l1_(t_sec_name_indexer_.NumSecurityId(), false),
      sec_id_to_prev_update_was_quote_(t_sec_name_indexer_.NumSecurityId(), false),
      liffe_trade_time_manager_(
          HFSAT::LiffeTradeTimeManager::GetUniqueInstance(t_sec_name_indexer_, t_watch_.YYYYMMDD())) {}

void IndexedLiffePriceLevelMarketViewManager::OnPriceLevelNew(const unsigned int t_security_id_,
                                                              const TradeType_t t_buysell_, const int t_level_added_,
                                                              const double t_price_, const int t_new_size_,
                                                              const int t_new_ordercount_,
                                                              const bool t_is_intermediate_message_) {
  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);

  int int_price_ = smv_.GetIntPx(t_price_);

  // check for off market limit orders or preopen status
  if (!valid_trade_seen_[t_security_id_] ||
      int_price_ > last_trade_price_[t_security_id_] + (int)smv_.initial_tick_size_ / 2 ||
      int_price_ < last_trade_price_[t_security_id_] - (int)smv_.initial_tick_size_ / 2) {
    return;
  }

  if (!smv_.initial_book_constructed_) {
    BuildIndex(t_security_id_, t_buysell_, int_price_);
  }

  int bid_index_ = 0;
  int ask_index_ = 0;

  int old_size_ = 0;

  bool update_best_variables_ = true;

  switch (t_buysell_) {
    case kTradeTypeBuy: {
      bid_index_ = (int)smv_.base_bid_index_ -
                   (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_ - int_price_);

      // There are 0 levels on the bid side
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

      // due to legacy L1/L2 duplication - avoid propagating if information is unchanged
      if (old_size_ == t_new_size_) {
        return;
      }

      // Update the size and order count
      smv_.market_update_info_.bidlevels_[bid_index_].limit_size_ = t_new_size_;
      smv_.market_update_info_.bidlevels_[bid_index_].limit_ordercount_ = t_new_ordercount_;

      if (bid_index_ > (int)smv_.base_bid_index_) {
        // Bitmask - again make sure we never change the order of base index assignment
        if ((bid_index_ - smv_.base_bid_index_) < smv_.bitmask_size_) {
          smv_.bid_access_bitmask_ >>= ((bid_index_ - smv_.base_bid_index_));
        } else {
          smv_.bid_access_bitmask_ = BIT_RESET_ALL;
        }

        smv_.base_bid_index_ = bid_index_;

        // Update BitMask - Actually this will just setup 1st bit
        if ((smv_.base_bid_index_ - bid_index_) < smv_.bitmask_size_) {
          smv_.bid_access_bitmask_ |= smv_.bitmask_lookup_table_[smv_.base_bid_index_ - bid_index_];
        }

        l1_price_changed_[t_security_id_] = true;
      } else if (bid_index_ == (int)smv_.base_bid_index_) {
        // Update BitMask - Actually this will just setup 1st bit
        if ((smv_.base_bid_index_ - bid_index_) < smv_.bitmask_size_) {
          smv_.bid_access_bitmask_ |= smv_.bitmask_lookup_table_[smv_.base_bid_index_ - bid_index_];
        }

        l1_size_changed_[t_security_id_] = true;
      } else {
        // Update BitMask
        if ((smv_.base_bid_index_ - bid_index_) < smv_.bitmask_size_) {
          smv_.bid_access_bitmask_ |= smv_.bitmask_lookup_table_[smv_.base_bid_index_ - bid_index_];
        }

        smv_.l2_changed_since_last_ = true;
      }

      // Sanitise ASK side
      if (int_price_ >= smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_) {
        if (smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_ <= 0) {
          // We should not do sanitization, if the other side is not ready
          smv_.is_ready_ = false;
          break;
        }

        int index_ = smv_.base_ask_index_;
        int last_index_ = index_;

        for (; index_ >= 0; index_--) {
          if (smv_.market_update_info_.asklevels_[index_].limit_int_price_ > int_price_ &&
              smv_.market_update_info_.asklevels_[index_].limit_size_ > 0) {
            smv_.base_ask_index_ = index_;
            break;
          }
          smv_.market_update_info_.asklevels_[index_].limit_size_ = 0;
          smv_.market_update_info_.asklevels_[index_].limit_ordercount_ = 0;
        }

        if (index_ < 0) {
          smv_.is_ready_ = false;
          DBGLOG_TIME_CLASS_FUNC_LINE << " " << smv_.secname() << " Ask side empty after sanitization "
                                      << DBGLOG_ENDL_FLUSH;
          DBGLOG_DUMP;
          return;
        }

        if ((smv_.base_ask_index_ - last_index_) < smv_.bitmask_size_) {
          smv_.ask_access_bitmask_ <<= (smv_.base_ask_index_ - last_index_);
        } else {
          smv_.ask_access_bitmask_ = BIT_RESET_ALL;
        }

        // Check if we need to re-centre the base_ask_index_
        if (smv_.base_ask_index_ < LOW_ACCESS_INDEX) {
          RebuildIndexLowAccess(t_security_id_, kTradeTypeSell,
                                smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_);
        }

        // Update best variables
        UpdateBestAskVariables(t_security_id_, smv_.base_ask_index_);
        l1_price_changed_[t_security_id_] = true;
      }

      if (update_best_variables_ && (l1_price_changed_[t_security_id_] || l1_size_changed_[t_security_id_])) {
        UpdateBestBidVariables(t_security_id_, smv_.base_bid_index_);
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

      // due to legacy L1/L2 duplication - avoid propagating if information is unchanged
      if (old_size_ == t_new_size_) {
        return;
      }

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

        smv_.base_ask_index_ = ask_index_;

        // Update BitMask
        if ((smv_.base_ask_index_ - ask_index_) < smv_.bitmask_size_) {
          smv_.ask_access_bitmask_ |= smv_.bitmask_lookup_table_[smv_.base_ask_index_ - ask_index_];
        }

        l1_price_changed_[t_security_id_] = true;
      } else if (ask_index_ == (int)smv_.base_ask_index_) {
        l1_size_changed_[t_security_id_] = true;

        // Update BitMask
        if ((smv_.base_ask_index_ - ask_index_) < smv_.bitmask_size_) {
          smv_.ask_access_bitmask_ |= smv_.bitmask_lookup_table_[smv_.base_ask_index_ - ask_index_];
        }
      } else {
        smv_.l2_changed_since_last_ = true;

        // Update BitMask
        if ((smv_.base_ask_index_ - ask_index_) < smv_.bitmask_size_) {
          smv_.ask_access_bitmask_ |= smv_.bitmask_lookup_table_[smv_.base_ask_index_ - ask_index_];
        }
      }

      // Sanitise BID side
      if (int_price_ <= smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_) {
        if (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_ <= 0) {
          // We should not do sanitization, if the other side is not ready
          smv_.is_ready_ = false;
          break;
        }

        int index_ = smv_.base_bid_index_;
        int last_index_ = index_;

        for (; index_ >= 0; index_--) {
          if (smv_.market_update_info_.bidlevels_[index_].limit_int_price_ < int_price_ &&
              smv_.market_update_info_.bidlevels_[index_].limit_size_ > 0) {
            smv_.base_bid_index_ = index_;
            break;
          }
          smv_.market_update_info_.bidlevels_[index_].limit_size_ = 0;
          smv_.market_update_info_.bidlevels_[index_].limit_ordercount_ = 0;
        }

        if (index_ < 0) {
          smv_.is_ready_ = false;
          DBGLOG_TIME_CLASS_FUNC_LINE << " " << smv_.secname() << " Bid side empty after sanitization "
                                      << DBGLOG_ENDL_FLUSH;
          DBGLOG_DUMP;
          return;
        }

        if ((smv_.base_bid_index_ - last_index_) < smv_.bitmask_size_) {
          smv_.bid_access_bitmask_ <<= (smv_.base_bid_index_ - last_index_);
        } else {
          smv_.bid_access_bitmask_ = BIT_RESET_ALL;
        }

        // Check if we need to re-centre the base_bid_index_
        if (smv_.base_bid_index_ < LOW_ACCESS_INDEX) {
          RebuildIndexLowAccess(t_security_id_, kTradeTypeBuy,
                                smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_);
        }

        // Update best variables
        UpdateBestBidVariables(t_security_id_, smv_.base_bid_index_);

        l1_price_changed_[t_security_id_] = true;
      }

      if (update_best_variables_ && (l1_price_changed_[t_security_id_] || l1_size_changed_[t_security_id_])) {
        UpdateBestAskVariables(t_security_id_, smv_.base_ask_index_);
      }
    } break;
    default:
      break;
  }

  sec_id_to_prev_update_was_quote_[t_security_id_] = true;
  last_update_was_delete_l1_[t_security_id_] = false;

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
  HFSAT::CpucycleProfiler::GetUniqueInstance(30).End(6);
#endif

  if (smv_.pl_change_listeners_present_) {
    if (t_buysell_ == kTradeTypeBuy && (int)smv_.base_bid_index_ >= bid_index_ &&
        smv_.base_bid_index_ - bid_index_ <= 10) {
      /// check if level = int_price_level should be fixed or left as is
      int level_ = smv_.base_bid_index_ - bid_index_;
      int int_price_level_ = smv_.base_bid_index_ - bid_index_;
      smv_.NotifyOnPLChangeListeners(t_security_id_, smv_.market_update_info_, t_buysell_, level_, int_price_,
                                     int_price_level_, old_size_,
                                     smv_.market_update_info_.bidlevels_[bid_index_].limit_size_, 1, 1,
                                     t_is_intermediate_message_, old_size_ == 0 ? 'N' : 'C');
    } else if (t_buysell_ == kTradeTypeSell && (int)smv_.base_ask_index_ >= ask_index_ &&
               smv_.base_ask_index_ - ask_index_ <= 10) {
      int level_ = smv_.base_ask_index_ - ask_index_;
      int int_price_level_ = smv_.base_ask_index_ - ask_index_;
      smv_.NotifyOnPLChangeListeners(t_security_id_, smv_.market_update_info_, t_buysell_, level_, int_price_,
                                     int_price_level_, old_size_,
                                     smv_.market_update_info_.asklevels_[ask_index_].limit_size_, 1, 1,
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
}

void IndexedLiffePriceLevelMarketViewManager::OnPriceLevelDelete(const unsigned int t_security_id_,
                                                                 const TradeType_t t_buysell_,
                                                                 const int t_level_removed_, const double t_price_,
                                                                 const bool t_is_intermediate_message_) {
  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);

  int int_price_ = smv_.GetIntPx(t_price_);

  // check for off market limit orders or preopen status
  if (!valid_trade_seen_[t_security_id_] ||
      int_price_ > last_trade_price_[t_security_id_] + (int)smv_.initial_tick_size_ / 2 ||
      int_price_ < last_trade_price_[t_security_id_] - (int)smv_.initial_tick_size_ / 2) {
    return;
  }

  if (!smv_.initial_book_constructed_) {
    BuildIndex(t_security_id_, t_buysell_, int_price_);
  }

  int bid_index_ = 0;
  int ask_index_ = 0;

  int old_size_ = 0;

  switch (t_buysell_) {
    case kTradeTypeBuy: {
      bid_index_ = (int)smv_.base_bid_index_ -
                   (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_ - int_price_);

      // Skip the message if we don't have any info for this level
      if (bid_index_ < 0 || bid_index_ > (int)smv_.base_bid_index_) {
        break;
      }

      old_size_ = smv_.market_update_info_.bidlevels_[bid_index_].limit_size_;

      /// duplicate L1/L2 though probably caught in the check above as well frequently
      if (old_size_ == 0) {
        return;
      }

      smv_.market_update_info_.bidlevels_[bid_index_].limit_size_ = 0;
      smv_.market_update_info_.bidlevels_[bid_index_].limit_ordercount_ = 0;

      if (bid_index_ == (int)smv_.base_bid_index_) {
        if (!last_update_was_delete_l1_[t_security_id_]) {
          last_l1_delete_was_bid_[t_security_id_] = true;
          last_l1_delete_int_price_[t_security_id_] = int_price_;

          last_update_was_delete_l1_[t_security_id_] = true;
        }

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

          // Not sure if ordercount would get updated accordingly
          UpdateBestBidVariables(t_security_id_, smv_.base_bid_index_);

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
      ask_index_ = (int)smv_.base_ask_index_ +
                   (smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_ - int_price_);

      // Skip the message if we don't have any info for this level
      if (ask_index_ < 0 || ask_index_ > (int)smv_.base_ask_index_) {
        break;
      }

      old_size_ = smv_.market_update_info_.asklevels_[ask_index_].limit_size_;
      /// duplicate L1/L2 .. also caught in check above
      if (old_size_ == 0) {
        return;
      }

      smv_.market_update_info_.asklevels_[ask_index_].limit_size_ = 0;
      smv_.market_update_info_.asklevels_[ask_index_].limit_ordercount_ = 0;

      if (ask_index_ == (int)smv_.base_ask_index_) {
        if (!last_update_was_delete_l1_[t_security_id_]) {
          last_l1_delete_was_bid_[t_security_id_] = false;
          last_l1_delete_int_price_[t_security_id_] = int_price_;

          last_update_was_delete_l1_[t_security_id_] = true;
        }

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

  sec_id_to_prev_update_was_quote_[t_security_id_] = true;

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
  HFSAT::CpucycleProfiler::GetUniqueInstance(30).End(6);
#endif

  if (smv_.pl_change_listeners_present_) {
    if (t_buysell_ == kTradeTypeBuy && (int)smv_.base_bid_index_ >= bid_index_ &&
        smv_.base_bid_index_ - bid_index_ <= 10) {
      int level_ = smv_.base_bid_index_ - bid_index_;
      int int_price_level_ = smv_.base_bid_index_ - bid_index_;
      smv_.NotifyOnPLChangeListeners(t_security_id_, smv_.market_update_info_, t_buysell_, level_, int_price_,
                                     int_price_level_, old_size_, 0, 1, 0, t_is_intermediate_message_, 'D');
    } else if (t_buysell_ == kTradeTypeSell && (int)smv_.base_ask_index_ >= ask_index_ &&
               smv_.base_ask_index_ - ask_index_ <= 10) {
      int level_ = smv_.base_ask_index_ - ask_index_;
      int int_price_level_ = smv_.base_ask_index_ - ask_index_;
      smv_.NotifyOnPLChangeListeners(t_security_id_, smv_.market_update_info_, t_buysell_, level_, int_price_,
                                     int_price_level_, old_size_, 0, 1, 0, t_is_intermediate_message_, 'D');
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
}

void IndexedLiffePriceLevelMarketViewManager::OnPriceLevelChange(const unsigned int t_security_id_,
                                                                 const TradeType_t t_buysell_,
                                                                 const int t_level_changed_, const double t_price_,
                                                                 const int t_new_size_, const int t_new_ordercount_,
                                                                 const bool t_is_intermediate_message_) {
  OnPriceLevelNew(t_security_id_, t_buysell_, t_level_changed_, t_price_, t_new_size_, t_new_ordercount_,
                  t_is_intermediate_message_);
}

void IndexedLiffePriceLevelMarketViewManager::OnTrade(const unsigned int t_security_id_, const double t_trade_price_,
                                                      const int t_trade_size_, const TradeType_t t_buysell_) {
  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);
  int t_trade_int_price_ = smv_.GetIntPx(t_trade_price_);

  // check for valid trade if applicable - book building and index shifts only constrained to valid prices to
  // avoid initial issues with security state and off market limit orders.
  if (!valid_trade_seen_[t_security_id_]) {
    if (t_trade_size_ > 0 &&
        liffe_trade_time_manager_.isValidTimeToTrade(t_security_id_, watch_.msecs_from_midnight() / 1000)) {
      valid_trade_seen_[t_security_id_] = true;
      last_trade_price_[t_security_id_] = t_trade_int_price_;
    }
    return;
  }

  // not till book is ready
  if ((smv_.market_update_info_.bestbid_int_price_ <= kInvalidIntPrice) ||
      (smv_.market_update_info_.bestask_int_price_ <= kInvalidIntPrice)) {
    return;
  }

  // same as SecurityMarketView::OnTrade - not particularly relevant but maintaining
  // for consistency
  if (smv_.trade_print_info_.computing_last_book_tdiff_) {
    if (sec_id_to_prev_update_was_quote_[t_security_id_]) {
      // the last time the update was a book message
      smv_.market_update_info_.last_book_mkt_size_weighted_price_ =
          smv_.market_update_info_
              .mkt_size_weighted_price_;  // noting the mktpx as it was justbefore the first trade message
    }
  }

  smv_.StorePreTrade();

  smv_.trade_print_info_.buysell_ = t_buysell_;

  if (t_trade_int_price_ <= smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_) {
    smv_.trade_print_info_.buysell_ = kTradeTypeSell;
  } else if (t_trade_int_price_ >= smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_) {
    smv_.trade_print_info_.buysell_ = kTradeTypeBuy;
  } else {
    if (last_l1_delete_was_bid_[t_security_id_] && t_trade_int_price_ <= last_l1_delete_int_price_[t_security_id_]) {
      smv_.trade_print_info_.buysell_ = kTradeTypeSell;
    } else if (!last_l1_delete_was_bid_[t_security_id_] &&
               t_trade_int_price_ >= last_l1_delete_int_price_[t_security_id_]) {
      smv_.trade_print_info_.buysell_ = kTradeTypeBuy;
    }
  }

  // set the primary variables
  smv_.trade_print_info_.trade_price_ = t_trade_price_;
  smv_.trade_print_info_.size_traded_ = t_trade_size_;
  smv_.trade_print_info_.int_trade_price_ = t_trade_int_price_;

  last_trade_price_[t_security_id_] = t_trade_int_price_;

  smv_.trade_print_info_.num_trades_++;
  /// semantics should differ for trade vs quote ordering - TODO
  smv_.SetTradeVarsForIndicatorsIfRequired();

#if CCPROFILING_TRADEINIT
  HFSAT::CpucycleProfiler::GetUniqueInstance(30).End(6);
#endif

  // Same as SecurityMarketView::OnTrade
  if (smv_.is_ready_) {
    smv_.NotifyTradeListeners();
    smv_.NotifyOnReadyListeners();
  }

  sec_id_to_prev_update_was_quote_[t_security_id_] = false;
  last_update_was_delete_l1_[t_security_id_] = false;
}
}
