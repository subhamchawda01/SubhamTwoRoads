/**
   \file MarketAdapterCode/indexed_cme_market_view_manager.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#include "baseinfra/MarketAdapter/indexed_cme_market_view_manager.hpp"
#define CCPROFILING_TRADEINIT 1

namespace HFSAT {

#define LOW_ACCESS_INDEX 50
#define MSECS_TO_MASK_SYN_DEL 100

IndexedCmeMarketViewManager::IndexedCmeMarketViewManager(
    DebugLogger& t_dbglogger_, const Watch& t_watch_, const SecurityNameIndexer& t_sec_name_indexer_,
    const std::vector<SecurityMarketView*>& t_security_market_view_map_)
    : BaseMarketViewManager(t_dbglogger_, t_watch_, t_sec_name_indexer_, t_security_market_view_map_),
      sec_id_to_prev_update_was_quote_(t_sec_name_indexer_.NumSecurityId(), false) {}

void IndexedCmeMarketViewManager::DropIndexedBookForSource(HFSAT::ExchSource_t t_exch_source_,
                                                           const int t_security_id_) {
  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);

  HFSAT::ExchSource_t this_exch_source_ =
      HFSAT::SecurityDefinitions::GetContractExchSource(smv_.shortcode(), watch_.YYYYMMDD());

  if (this_exch_source_ != t_exch_source_) return;

  smv_.market_update_info_.bidlevels_.clear();
  smv_.market_update_info_.asklevels_.clear();
}

/*
 * CME market data price level new notes:
 * 1. Implicit delete for levels beyond 10
 */
void IndexedCmeMarketViewManager::OnPriceLevelNew(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                                  const int t_level_added_, const double t_price_,
                                                  const int t_new_size_, const int t_new_ordercount_,
                                                  const bool t_is_intermediate_message_) {
  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);

  int int_price_ = smv_.GetIntPx(t_price_);

  smv_.StorePreBook(t_buysell_);

  if (!smv_.initial_book_constructed_) {
    BuildIndex(t_security_id_, t_buysell_, int_price_);

    smv_.initial_book_constructed_ = true;
  }

  int old_size_ = 0;
  int old_ordercount_ = 0;

  switch (t_buysell_) {
    case kTradeTypeBuy: {
      int bid_index_ = smv_.base_bid_index_ -
                       (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_ - int_price_);

      // There are 0 levels on bid side
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
        }

        smv_.base_bid_index_ = bid_index_;
      } else if (bid_index_ < 0) {
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
      }

      // If we have masked this level due to synthetic delete, then don't update this level as this was deleted
      if (watch_.msecs_from_midnight() >= 0 &&
          watch_.msecs_from_midnight() < smv_.market_update_info_.bidlevels_[bid_index_].mask_time_msecs_) {
        break;
      }

      // Sanitize bid side for level 1 message
      if (t_level_added_ == 1 && bid_index_ < (int)smv_.base_bid_index_) {
        for (int index_ = smv_.base_bid_index_; index_ > bid_index_; index_--) {
          smv_.market_update_info_.bidlevels_[index_].limit_size_ = 0;
        }

        // bitshift, make sure the code lines are in order - @ravi
        if ((smv_.base_bid_index_ - bid_index_) < smv_.bitmask_size_) {
          smv_.bid_access_bitmask_ <<= (smv_.base_bid_index_ - bid_index_);
        } else {
          smv_.bid_access_bitmask_ = BIT_RESET_ALL;
        }

        smv_.base_bid_index_ = bid_index_;

        if (smv_.base_bid_index_ < LOW_ACCESS_INDEX) {
          RebuildIndexLowAccess(t_security_id_, t_buysell_, int_price_);
        }
      }

      // If level == 10, clear all levels between level 9 and the current level
      if (t_level_added_ == 10) {
        int level_ = 1;
        int index9_ = (int)smv_.base_bid_index_ - 1;
        for (; index9_ > bid_index_; index9_--) {
          if (smv_.market_update_info_.bidlevels_[index9_].limit_size_ > 0) {
            level_++;
          }

          if (level_ == 9) {
            break;
          }
        }

        index9_--;

        for (; index9_ > bid_index_; index9_--) {
          smv_.market_update_info_.bidlevels_[index9_].limit_size_ = 0;

          // Reset the relevant bits, don't care about levels > 10 anyways
          if ((smv_.base_bid_index_ - index9_) < smv_.bitmask_size_) {
            smv_.bid_access_bitmask_ &= (~(smv_.bitmask_lookup_table_[(smv_.base_bid_index_ - index9_)]));
          }
        }
      }

      // Store old size and order count for OnPL change listeners. (Not sure if having a bool check will help)
      old_size_ = smv_.market_update_info_.bidlevels_[bid_index_].limit_size_;
      old_ordercount_ = smv_.market_update_info_.bidlevels_[bid_index_].limit_ordercount_;

      smv_.market_update_info_.bidlevels_[bid_index_].limit_size_ = t_new_size_;
      smv_.market_update_info_.bidlevels_[bid_index_].limit_ordercount_ = t_new_ordercount_;

      if (bid_index_ >= (int)smv_.base_bid_index_) {
        smv_.l1_changed_since_last_ = true;

        // Bitmask - again make sure we never change the order of base indexe assignment
        if ((bid_index_ - smv_.base_bid_index_) < smv_.bitmask_size_) {
          smv_.bid_access_bitmask_ >>= ((bid_index_ - smv_.base_bid_index_));
        } else {
          smv_.bid_access_bitmask_ = BIT_RESET_ALL;
        }

        if (bid_index_ > (int)smv_.base_bid_index_) {
          smv_.bid_level_change_bitmask_ = 0xFFFF;
        } else {
          smv_.bid_level_change_bitmask_ = smv_.bid_level_change_bitmask_ | 0x0001;
        }

        smv_.base_bid_index_ = bid_index_;

        // Update BitMask - Actually this will just setup 1st bit
        if ((smv_.base_bid_index_ - bid_index_) < smv_.bitmask_size_) {
          smv_.bid_access_bitmask_ |= smv_.bitmask_lookup_table_[smv_.base_bid_index_ - bid_index_];
        }

        if (int_price_ >= smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_) {
          if (smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_ <= 0) {
            // We should not do sanitization, if the other side is not ready
            smv_.is_ready_ = false;
            break;
          }

          // Sanitise ASK side
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

          // The ask side is empty
          if (index_ < 0) {
            smv_.ask_access_bitmask_ = BIT_RESET_ALL;
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

          // Check if we need to re-align the index
          if (smv_.base_ask_index_ < LOW_ACCESS_INDEX) {
            RebuildIndexLowAccess(t_security_id_, kTradeTypeSell,
                                  smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_);
          }

          // Update ASK side best variables
          smv_.market_update_info_.bestask_int_price_ =
              smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_;
          smv_.market_update_info_.bestask_ordercount_ =
              smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_ordercount_;
          smv_.market_update_info_.bestask_price_ =
              smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_price_;
          smv_.market_update_info_.bestask_size_ =
              smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_;
          if (!smv_.price_to_yield_map_.empty()) {
            smv_.hybrid_market_update_info_.bestask_int_price_ =
                smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_;
            smv_.hybrid_market_update_info_.bestask_ordercount_ =
                smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_ordercount_;
            smv_.hybrid_market_update_info_.bestask_price_ =
                smv_.price_to_yield_map_[smv_.hybrid_market_update_info_.bestask_int_price_];
            smv_.hybrid_market_update_info_.bestask_size_ =
                smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_;
          }
        }
      } else {
        smv_.l2_changed_since_last_ = true;

        // Update BitMask
        if ((smv_.base_bid_index_ - bid_index_) < smv_.bitmask_size_) {
          smv_.bid_access_bitmask_ |= smv_.bitmask_lookup_table_[smv_.base_bid_index_ - bid_index_];
        }

        smv_.bid_level_change_bitmask_ = smv_.bid_level_change_bitmask_ | (1 << (smv_.base_bid_index_ - bid_index_));
      }

    } break;
    case kTradeTypeSell: {
      int ask_index_ = smv_.base_ask_index_ +
                       (smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_ - int_price_);

      // There are 0 levels on ask side
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
        }

        smv_.base_ask_index_ = ask_index_;
      } else if (ask_index_ < 0) {
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
      }

      // If we have masked this level due to synthetic delete, then don't update this level as this was deleted
      if (watch_.msecs_from_midnight() >= 0 &&
          watch_.msecs_from_midnight() < smv_.market_update_info_.asklevels_[ask_index_].mask_time_msecs_) {
        break;
      }

      // Sanitize ASK side for level 1 message
      if (t_level_added_ == 1 && ask_index_ < (int)smv_.base_ask_index_) {
        for (int index_ = smv_.base_ask_index_; index_ > ask_index_; index_--) {
          smv_.market_update_info_.asklevels_[index_].limit_size_ = 0;
        }

        // bitshift, make sure the code lines are in order - @ravi
        if ((smv_.base_ask_index_ - ask_index_) < smv_.bitmask_size_) {
          smv_.ask_access_bitmask_ <<= (smv_.base_ask_index_ - ask_index_);
        } else {
          smv_.ask_access_bitmask_ = BIT_RESET_ALL;
        }

        smv_.base_ask_index_ = ask_index_;

        if (smv_.base_ask_index_ < LOW_ACCESS_INDEX) {
          RebuildIndexLowAccess(t_security_id_, t_buysell_, int_price_);
        }
      }

      // If level == 10, clear all levels between level 9 and the current level
      if (t_level_added_ == 10) {
        int level_ = 1;
        int index9_ = (int)smv_.base_ask_index_ - 1;
        for (; index9_ > ask_index_; index9_--) {
          if (smv_.market_update_info_.asklevels_[index9_].limit_size_ > 0) {
            level_++;
          }

          if (level_ == 9) {
            break;
          }
        }

        index9_--;

        for (; index9_ > ask_index_; index9_--) {
          smv_.market_update_info_.asklevels_[index9_].limit_size_ = 0;

          // Reset the relevant bits, don't care about levels > 10 anyways
          if ((smv_.base_ask_index_ - index9_) < smv_.bitmask_size_) {
            smv_.ask_access_bitmask_ &= (!(smv_.bitmask_lookup_table_[(smv_.base_ask_index_ - index9_)]));
          }
        }
      }

      old_size_ = smv_.market_update_info_.asklevels_[ask_index_].limit_size_;
      old_ordercount_ = smv_.market_update_info_.asklevels_[ask_index_].limit_ordercount_;

      smv_.market_update_info_.asklevels_[ask_index_].limit_size_ = t_new_size_;
      smv_.market_update_info_.asklevels_[ask_index_].limit_ordercount_ = t_new_ordercount_;

      if (ask_index_ >= (int)smv_.base_ask_index_) {
        smv_.l1_changed_since_last_ = true;

        // Bitmask - again make sure we never change the order of base indexed assignment
        if ((ask_index_ - smv_.base_ask_index_) < smv_.bitmask_size_) {
          smv_.ask_access_bitmask_ >>= ((ask_index_ - smv_.base_ask_index_));
        } else {
          smv_.ask_access_bitmask_ = BIT_RESET_ALL;
        }

        if (ask_index_ > (int)smv_.base_ask_index_) {
          smv_.ask_level_change_bitmask_ = 0xFFFF;
        } else {
          smv_.ask_level_change_bitmask_ = smv_.ask_level_change_bitmask_ | 0x0001;
        }

        smv_.base_ask_index_ = ask_index_;

        // Update BitMask
        if ((smv_.base_ask_index_ - ask_index_) < smv_.bitmask_size_) {
          smv_.ask_access_bitmask_ |= smv_.bitmask_lookup_table_[smv_.base_ask_index_ - ask_index_];
        }

        if (int_price_ <= smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_) {
          if (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_ <= 0) {
            // We should not do sanitization, if the other side is not ready
            smv_.is_ready_ = false;
            break;
          }

          // Sanitise BID side
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

          // bid side is empty
          if (index_ < 0) {
            smv_.is_ready_ = false;
            smv_.bid_access_bitmask_ = BIT_RESET_ALL;
            DBGLOG_TIME_CLASS_FUNC_LINE << " " << smv_.secname() << " Bid side empty after sanitization "
                                        << DBGLOG_ENDL_FLUSH;
            DBGLOG_DUMP;
            break;
          }

          if ((smv_.base_bid_index_ - last_index_) < smv_.bitmask_size_) {
            smv_.bid_access_bitmask_ <<= (smv_.base_bid_index_ - last_index_);
          } else {
            smv_.bid_access_bitmask_ = BIT_RESET_ALL;
          }

          // Check if we need to re-align the centre
          if (smv_.base_bid_index_ < LOW_ACCESS_INDEX) {
            RebuildIndexLowAccess(t_security_id_, kTradeTypeBuy,
                                  smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_);
          }

          // Update the BID side best variables
          UpdateBestBidVariables(t_security_id_, smv_.base_bid_index_);
        }
      } else {
        smv_.l2_changed_since_last_ = true;

        // Update BitMask
        if ((smv_.base_ask_index_ - ask_index_) < smv_.bitmask_size_) {
          smv_.ask_access_bitmask_ |= smv_.bitmask_lookup_table_[smv_.base_ask_index_ - ask_index_];
        }

        smv_.ask_level_change_bitmask_ = smv_.ask_level_change_bitmask_ | (1 << (smv_.base_ask_index_ - ask_index_));
      }

    } break;
    default:
      break;
  }

  if (smv_.l1_changed_since_last_) {
    switch (t_buysell_) {
      case kTradeTypeBuy: {
        // Update best bid variables
        UpdateBestBidVariables(t_security_id_, smv_.base_bid_index_);

        UpdateBestBidVariablesUsingOurOrders(t_security_id_);
        smv_.UpdateL1Prices();
      } break;
      case kTradeTypeSell: {
        // Update best ask variables
        UpdateBestAskVariables(t_security_id_, smv_.base_ask_index_);

        UpdateBestAskVariablesUsingOurOrders(t_security_id_);
        smv_.UpdateL1Prices();
      } break;
      default:
        break;
    }
  }

  // if best variables and book top are not in sync, update the best variables even if this isn't an l1 update
  if (smv_.market_update_info_.bestbid_int_price_ !=
          smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_ ||
      smv_.market_update_info_.bestask_int_price_ !=
          smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_) {
    UpdateBestBidVariables(t_security_id_, smv_.base_bid_index_);
    UpdateBestAskVariables(t_security_id_, smv_.base_ask_index_);

    UpdateBestBidVariablesUsingOurOrders(t_security_id_);
    UpdateBestAskVariablesUsingOurOrders(t_security_id_);
    smv_.UpdateL1Prices();
  }

  sec_id_to_prev_update_was_quote_[t_security_id_] = true;

  if (!smv_.is_ready_) {
    if (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_ > 0 &&
        smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_ > 0 && smv_.ArePricesComputed()) {
      smv_.is_ready_ = true;
    } else {
      return;
    }
  }

  if (smv_.market_update_info_.trade_update_implied_quote_) {
    smv_.market_update_info_.trade_update_implied_quote_ = false;
    smv_.trade_print_info_.num_trades_++;
  }

  if (t_is_intermediate_message_) {
    return;
  }

#if CCPROFILING_TRADEINIT
  HFSAT::CpucycleProfiler::GetUniqueInstance(30).End(5);
#endif

  if (smv_.pl_change_listeners_present_) {
    smv_.NotifyOnPLChangeListeners(t_security_id_, smv_.market_update_info_, t_buysell_, t_level_added_ - 1, int_price_,
                                   0, old_size_, t_new_size_, old_ordercount_, t_new_ordercount_,
                                   t_is_intermediate_message_, old_size_ == 0 ? 'N' : 'C');
  }

  if (smv_.l1_changed_since_last_) {
    smv_.NotifyL1PriceListeners();

    smv_.l1_changed_since_last_ = false;
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

void IndexedCmeMarketViewManager::OnPriceLevelChange(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                                     const int t_level_changed_, const double t_price_,
                                                     const int t_new_size_, const int t_new_ordercount_,
                                                     const bool t_is_intermediate_message_) {
  OnPriceLevelNew(t_security_id_, t_buysell_, t_level_changed_, t_price_, t_new_size_, t_new_ordercount_,
                  t_is_intermediate_message_);
}

void IndexedCmeMarketViewManager::OnPriceLevelDelete(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                                     const int t_level_removed_, const double t_price_,
                                                     const bool t_is_intermediate_message_) {
  // Skip levels more than 10
  if (t_level_removed_ > 10) {
    return;
  }

  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);

  if (!smv_.initial_book_constructed_) {
    return;
  }

  int old_size_ = 0;
  int old_ordercount_ = 0;
  int int_price_ = smv_.GetIntPx(t_price_);

  switch (t_buysell_) {
    case kTradeTypeBuy: {
      int bid_index_ = smv_.base_bid_index_ -
                       (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_ - int_price_);

      // Skip the message if we don't have any info for this level
      if (bid_index_ < 0 || bid_index_ > (int)smv_.base_bid_index_) {
        break;
      }

      // If we have masked this level due to synthetic delete, then don't update this level as this was deleted
      if (watch_.msecs_from_midnight() >= 0 &&
          watch_.msecs_from_midnight() < smv_.market_update_info_.bidlevels_[bid_index_].mask_time_msecs_) {
        break;
      }

      old_size_ = smv_.market_update_info_.bidlevels_[bid_index_].limit_size_;
      old_ordercount_ = smv_.market_update_info_.bidlevels_[bid_index_].limit_ordercount_;

      smv_.market_update_info_.bidlevels_[bid_index_].limit_size_ = 0;
      smv_.market_update_info_.bidlevels_[bid_index_].limit_ordercount_ = 0;

      smv_.bid_level_change_bitmask_ = 0xFFFF;

      if (bid_index_ == (int)smv_.base_bid_index_) {
        int next_bid_index_ = smv_.base_bid_index_ - 1;
        for (; next_bid_index_ >= 0 && smv_.market_update_info_.bidlevels_[next_bid_index_].limit_size_ <= 0;
             next_bid_index_--)
          ;

        if (next_bid_index_ < 0) {
          smv_.bid_access_bitmask_ = BIT_RESET_ALL;
          smv_.is_ready_ = false;
          return;
        }

        // Base Shifted, time to shift bitmask
        if ((smv_.base_bid_index_ - next_bid_index_) < smv_.bitmask_size_) {
          smv_.bid_access_bitmask_ <<= (smv_.base_bid_index_ - next_bid_index_);
        } else {
          smv_.bid_access_bitmask_ = BIT_RESET_ALL;
        }

        smv_.base_bid_index_ = next_bid_index_;

        smv_.l1_changed_since_last_ = true;

        if (smv_.base_bid_index_ < LOW_ACCESS_INDEX) {
          RebuildIndexLowAccess(t_security_id_, t_buysell_,
                                smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_);
        }
      } else {
        if ((smv_.base_bid_index_ - bid_index_) < smv_.bitmask_size_) {
          smv_.bid_access_bitmask_ &= (~(smv_.bitmask_lookup_table_[(smv_.base_bid_index_ - bid_index_)]));
        }

        smv_.l2_changed_since_last_ = true;
      }
    } break;
    case kTradeTypeSell: {
      int ask_index_ = smv_.base_ask_index_ +
                       (smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_ - int_price_);

      // Skip the message if we don't have any info for this level
      if (ask_index_ < 0 || ask_index_ > (int)smv_.base_ask_index_) {
        break;
      }

      // If we have masked this level due to synthetic delete, then don't update this level as this was deleted
      if (watch_.msecs_from_midnight() >= 0 &&
          watch_.msecs_from_midnight() < smv_.market_update_info_.bidlevels_[ask_index_].mask_time_msecs_) {
        break;
      }

      old_size_ = smv_.market_update_info_.asklevels_[ask_index_].limit_size_;
      old_ordercount_ = smv_.market_update_info_.asklevels_[ask_index_].limit_ordercount_;

      smv_.market_update_info_.asklevels_[ask_index_].limit_size_ = 0;
      smv_.market_update_info_.asklevels_[ask_index_].limit_ordercount_ = 0;

      smv_.ask_level_change_bitmask_ = 0xFFFF;

      if (ask_index_ == (int)smv_.base_ask_index_) {
        int next_ask_index_ = smv_.base_ask_index_ - 1;
        for (; next_ask_index_ >= 0 && smv_.market_update_info_.asklevels_[next_ask_index_].limit_size_ <= 0;
             next_ask_index_--)
          ;

        if (next_ask_index_ < 0) {
          smv_.ask_access_bitmask_ = BIT_RESET_ALL;
          smv_.is_ready_ = false;
          return;
        }

        // Base Shifted, time to shift bitmask
        if ((smv_.base_ask_index_ - next_ask_index_) < smv_.bitmask_size_) {
          smv_.ask_access_bitmask_ <<= (smv_.base_ask_index_ - next_ask_index_);
        } else {
          smv_.ask_access_bitmask_ = BIT_RESET_ALL;
        }

        smv_.base_ask_index_ = next_ask_index_;

        smv_.l1_changed_since_last_ = true;

        if (smv_.base_ask_index_ < LOW_ACCESS_INDEX) {
          RebuildIndexLowAccess(t_security_id_, t_buysell_,
                                smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_);
        }
      } else {
        smv_.l2_changed_since_last_ = true;

        if ((smv_.base_ask_index_ - ask_index_) < smv_.bitmask_size_) {
          smv_.ask_access_bitmask_ &= (~(smv_.bitmask_lookup_table_[(smv_.base_ask_index_ - ask_index_)]));
        }
      }
    } break;
    default:
      break;
  }

  if (smv_.l1_changed_since_last_) {
    switch (t_buysell_) {
      case kTradeTypeBuy: {
        // Update the best Bid variables
        UpdateBestBidVariables(t_security_id_, smv_.base_bid_index_);

        UpdateBestBidVariablesUsingOurOrders(t_security_id_);
        smv_.UpdateL1Prices();
      } break;
      case kTradeTypeSell: {
        // Update the best ask variables
        UpdateBestAskVariables(t_security_id_, smv_.base_ask_index_);

        UpdateBestAskVariablesUsingOurOrders(t_security_id_);
        smv_.UpdateL1Prices();
      } break;
      default:
        break;
    }
  }

  sec_id_to_prev_update_was_quote_[t_security_id_] = true;

  if (smv_.market_update_info_.trade_update_implied_quote_) {
    smv_.market_update_info_.trade_update_implied_quote_ = false;
    smv_.trade_print_info_.num_trades_++;
  }

  if (!smv_.is_ready_ || t_is_intermediate_message_) {
    return;
  }

#if CCPROFILING_TRADEINIT
  HFSAT::CpucycleProfiler::GetUniqueInstance(30).End(5);
#endif

  if (smv_.pl_change_listeners_present_) {
    smv_.NotifyOnPLChangeListeners(t_security_id_, smv_.market_update_info_, t_buysell_, t_level_removed_ - 1,
                                   int_price_, 0, old_size_, 0, old_ordercount_, 0, t_is_intermediate_message_, 'D');
  }

  if (smv_.l1_changed_since_last_) {
    smv_.NotifyL1PriceListeners();

    smv_.l1_changed_since_last_ = false;
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

void IndexedCmeMarketViewManager::OnTrade(const unsigned int t_security_id_, const double t_trade_price_,
                                          const int t_trade_size_, const TradeType_t t_buysell_) {
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
      // the last time the update was a book message
      smv_.market_update_info_.last_book_mkt_size_weighted_price_ = smv_.market_update_info_.mkt_size_weighted_price_;
    }
  }

  if (!smv_.market_update_info_.trade_update_implied_quote_) {
    smv_.market_update_info_.trade_update_implied_quote_ = true;
  }

  TradeType_t buysell_ = t_buysell_;

  if (buysell_ == kTradeTypeNoInfo) {
    buysell_ = ProcessTradeTypeNoInfo(buysell_);
    // return for now, since we don't want to process trades with TradeTypeNoInfo.
    return;
  }

  smv_.StorePreTrade();

  switch (buysell_) {
    case kTradeTypeBuy:  // Aggressive buy
    {
      int trade_ask_index_ =
          smv_.base_ask_index_ +
          (smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_ - int_trade_price_);

      if (trade_ask_index_ > (int)smv_.base_ask_index_ || trade_ask_index_ < 0) {
        break;
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

              UpdateBestAskVariables(t_security_id_, next_ask_index_);

            } else {
              smv_.market_update_info_.bestask_size_ -= t_trade_size_;
            }
          }
        } else {
          smv_.market_update_info_.bestask_int_price_ = int_trade_price_;
          smv_.market_update_info_.bestask_ordercount_ = 1;
          smv_.market_update_info_.bestask_price_ = smv_.GetDoublePx(int_trade_price_);
          smv_.market_update_info_.bestask_size_ = 1;
          if (!smv_.price_to_yield_map_.empty()) {
            smv_.hybrid_market_update_info_.bestask_int_price_ = int_trade_price_;
            smv_.hybrid_market_update_info_.bestask_ordercount_ = 1;
            smv_.hybrid_market_update_info_.bestask_price_ = smv_.price_to_yield_map_[int_trade_price_];
            smv_.hybrid_market_update_info_.bestask_size_ = 1;
          }
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

        // Update ask side
        UpdateBestAskVariables(t_security_id_, next_ask_index_);

      } else {
        smv_.market_update_info_.bestask_int_price_ =
            smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_;
        smv_.market_update_info_.bestask_ordercount_ =
            smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_ordercount_;
        smv_.market_update_info_.bestask_price_ =
            smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_price_;
        smv_.market_update_info_.bestask_size_ =
            smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_ - t_trade_size_;
        if (!smv_.price_to_yield_map_.empty()) {
          smv_.hybrid_market_update_info_.bestask_int_price_ =
              smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_;
          smv_.hybrid_market_update_info_.bestask_ordercount_ =
              smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_ordercount_;
          smv_.hybrid_market_update_info_.bestask_price_ =
              smv_.price_to_yield_map_[smv_.hybrid_market_update_info_.bestask_int_price_];
          smv_.hybrid_market_update_info_.bestask_size_ =
              smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_ - t_trade_size_;
        }
      }
      UpdateBestAskVariablesUsingOurOrders(t_security_id_);
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

              UpdateBestBidVariables(t_security_id_, next_bid_index_);

            } else {
              smv_.market_update_info_.bestbid_size_ -= t_trade_size_;
            }
          }
        } else {
          smv_.market_update_info_.bestbid_int_price_ = int_trade_price_;
          smv_.market_update_info_.bestbid_ordercount_ = 1;
          smv_.market_update_info_.bestbid_price_ = smv_.GetDoublePx(int_trade_price_);
          smv_.market_update_info_.bestbid_size_ = 1;
          if (!smv_.price_to_yield_map_.empty()) {
            smv_.hybrid_market_update_info_.bestbid_int_price_ = int_trade_price_;
            smv_.hybrid_market_update_info_.bestbid_ordercount_ = 1;
            smv_.hybrid_market_update_info_.bestbid_price_ =
                smv_.price_to_yield_map_[smv_.hybrid_market_update_info_.bestbid_int_price_];
            smv_.hybrid_market_update_info_.bestbid_size_ = 1;
          }
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

        UpdateBestBidVariables(t_security_id_, next_bid_index_);

      } else {
        smv_.market_update_info_.bestbid_int_price_ =
            smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_;
        smv_.market_update_info_.bestbid_ordercount_ =
            smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_ordercount_;
        smv_.market_update_info_.bestbid_price_ =
            smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_price_;
        smv_.market_update_info_.bestbid_size_ =
            smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_ - t_trade_size_;
        if (!smv_.price_to_yield_map_.empty()) {
          smv_.hybrid_market_update_info_.bestbid_int_price_ =
              smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_;
          smv_.hybrid_market_update_info_.bestbid_ordercount_ =
              smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_ordercount_;
          smv_.hybrid_market_update_info_.bestbid_price_ =
              smv_.price_to_yield_map_[smv_.hybrid_market_update_info_.bestbid_int_price_];
          smv_.hybrid_market_update_info_.bestbid_size_ =
              smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_ - t_trade_size_;
        }
      }
      UpdateBestBidVariablesUsingOurOrders(t_security_id_);
    } break;
    default:
      break;
  }

  smv_.UpdateL1Prices();

  // Set the trade variables
  smv_.trade_print_info_.trade_price_ = t_trade_price_;
  smv_.trade_print_info_.size_traded_ = t_trade_size_;
  smv_.trade_print_info_.int_trade_price_ = int_trade_price_;
  smv_.trade_print_info_.buysell_ = buysell_;

  smv_.SetTradeVarsForIndicatorsIfRequired();

#if CCPROFILING_TRADEINIT
  HFSAT::CpucycleProfiler::GetUniqueInstance(30).End(5);
#endif

  if (smv_.is_ready_) {
    smv_.NotifyTradeListeners();
    smv_.NotifyOnReadyListeners();
  }

  sec_id_to_prev_update_was_quote_[t_security_id_] = false;
}

void IndexedCmeMarketViewManager::OnPriceLevelOverlay(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                                      const int t_level_overlayed_, const double t_price_,
                                                      const int t_new_size_, const int t_new_ordercount_,
                                                      const bool t_is_intermediate_message_) {
  // happens only in EUREX, since for older EUREX data we use CME book, writing it.

  double old_price = GetPriceForLevel(t_security_id_, t_buysell_, t_level_overlayed_);

  OnPriceLevelDelete(t_security_id_, t_buysell_, t_level_overlayed_, old_price, true);
  OnPriceLevelNew(t_security_id_, t_buysell_, t_level_overlayed_, t_price_, t_new_size_, t_new_ordercount_,
                  t_is_intermediate_message_);
}

void IndexedCmeMarketViewManager::OnPriceLevelDeleteFrom(const unsigned int t_security_id_,
                                                         const TradeType_t t_buysell_, const int t_min_level_deleted_,
                                                         const bool t_is_intermediate_message_) {
  double old_price = GetPriceForLevel(t_security_id_, t_buysell_, t_min_level_deleted_);
  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);
  int int_price = smv_.GetIntPx(old_price);

  switch (t_buysell_) {
    case kTradeTypeBuy: {
      int bid_index = smv_.base_bid_index_ -
                      (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_ - int_price);
      if (bid_index < 0 || bid_index > (int)smv_.max_tick_range_) {
        // not doing anything if the bid index is beyond existing book
        return;
      } else {
        int last_index = bid_index;
        for (; last_index >= 0; last_index--) {
          // It could have been better if we maintain the maximum valid level as well,
          // but since its going to be used only on historical data, not much benefit we are getting here

          int price_level = smv_.base_bid_index_ - last_index;  // dummy actually
          bool intermediate = (t_is_intermediate_message_ | (last_index != 0));
          OnPriceLevelDelete(t_security_id_, kTradeTypeBuy, price_level,
                             smv_.market_update_info_.bidlevels_[last_index].limit_price_, intermediate);
        }
      }
    } break;
    case kTradeTypeSell: {
      int ask_index = smv_.base_ask_index_ +
                      (smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_ - int_price);
      if (ask_index < 0 || ask_index > (int)smv_.max_tick_range_) {
        return;
      } else {
        int last_index = ask_index;
        for (; last_index >= 0; last_index--) {
          // It could have been better if we maintain the maximum valid level as well,
          // but since its going to be used only on historical data, not much benefit we are getting here
          int price_level = smv_.base_ask_index_ - last_index;  // dummy actually
          bool intermediate = (t_is_intermediate_message_ | (last_index != 0));
          OnPriceLevelDelete(t_security_id_, kTradeTypeSell, price_level,
                             smv_.market_update_info_.asklevels_[last_index].limit_price_, intermediate);
        }
      }
    } break;
    default: { break; }
  }
}

void IndexedCmeMarketViewManager::OnPriceLevelDeleteSynthetic(const unsigned int t_security_id_,
                                                              const TradeType_t t_buysell_, const double t_price_,
                                                              const bool t_is_intermediate_message_) {
  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);

  if (!smv_.initial_book_constructed_) {
    // simply ignore the synthetic delete if book isn't constructed yet
    return;
  }

  // This is used for OnPL notifications
  int prev_size_ = 1;
  int prev_ordercount_ = 1;

  int new_size_ = 1;
  int new_ordercount_ = 1;
  int new_int_price_ = 1;

  char pl_change_type_ = 'C';

  bool l1_changed_ = false;

  switch (t_buysell_) {
    case kTradeTypeBuy: {
      int bid_int_price_ = smv_.GetIntPx(t_price_);

      int bid_index_ = smv_.base_bid_index_ -
                       (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_ - bid_int_price_);

      // This is used for OnPL notifications
      prev_ordercount_ = smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_ordercount_;
      prev_size_ = smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_;

      if (bid_index_ <= (int)smv_.base_bid_index_) {
        l1_changed_ = true;

        // Reconfigure the base_bid_index_
        int next_index_ = bid_index_ - 1;
        for (; next_index_ >= 0 && smv_.market_update_info_.bidlevels_[next_index_].limit_size_ <= 0; next_index_--)
          ;

        // Update the base_bid_index if next_index is valid. Otherwise, ignore.
        if (next_index_ >= 0) {
          int next_bid_int_price_ = smv_.market_update_info_.bidlevels_[next_index_].limit_int_price_;

          BookManagerErrorCode_t error_ = AdjustBidIndex(t_security_id_, next_bid_int_price_, next_index_);

          if (error_ == kBookManagerReturn) {
            return;
          }

          while (bid_index_ > next_index_ && bid_index_ >= 0) {
            smv_.market_update_info_.bidlevels_[bid_index_].limit_size_ = 0;
            smv_.market_update_info_.bidlevels_[bid_index_].limit_ordercount_ = 0;
            smv_.market_update_info_.bidlevels_[bid_index_].mask_time_msecs_ =
                watch_.msecs_from_midnight() + MSECS_TO_MASK_SYN_DEL;
            bid_index_--;
          }

          smv_.base_bid_index_ = (unsigned int)next_index_;

          UpdateBestBidVariables(t_security_id_, smv_.base_bid_index_);
          UpdateBestBidVariablesUsingOurOrders(t_security_id_);

          new_int_price_ = smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_;
          new_ordercount_ = smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_ordercount_;
          new_size_ = smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_;
        } else {
          return;
        }
      } else {
        if (bid_index_ >= 0 && bid_index_ < (int)smv_.max_tick_range_) {
          smv_.market_update_info_.bidlevels_[bid_index_].mask_time_msecs_ =
              watch_.msecs_from_midnight() + MSECS_TO_MASK_SYN_DEL;
        }
        return;
      }
    } break;
    case kTradeTypeSell: {
      int ask_int_price_ = smv_.GetIntPx(t_price_);

      int ask_index_ = smv_.base_ask_index_ +
                       (smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_ - ask_int_price_);

      // This is used for OnPL notifications
      prev_ordercount_ = smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_ordercount_;
      prev_size_ = smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_;

      if (ask_index_ <= (int)smv_.base_ask_index_) {
        l1_changed_ = true;

        // Reconfigure the base_ask_index_
        int next_index_ = ask_index_ - 1;
        for (; next_index_ >= 0 && smv_.market_update_info_.asklevels_[next_index_].limit_size_ <= 0; next_index_--)
          ;

        // Update the base_ask_index if next_index is valid. Otherwise, ignore.
        if (next_index_ >= 0) {
          int next_ask_int_price_ = smv_.market_update_info_.asklevels_[next_index_].limit_int_price_;

          BookManagerErrorCode_t error_ = AdjustAskIndex(t_security_id_, next_ask_int_price_, next_index_);

          if (error_ == kBookManagerReturn) {
            return;
          }

          while (ask_index_ > next_index_ && ask_index_ >= 0) {
            smv_.market_update_info_.asklevels_[ask_index_].limit_size_ = 0;
            smv_.market_update_info_.asklevels_[ask_index_].limit_ordercount_ = 0;
            smv_.market_update_info_.asklevels_[ask_index_].mask_time_msecs_ =
                watch_.msecs_from_midnight() + MSECS_TO_MASK_SYN_DEL;
            ask_index_--;
          }

          smv_.base_ask_index_ = (unsigned int)next_index_;

          UpdateBestAskVariables(t_security_id_, smv_.base_ask_index_);
          UpdateBestAskVariablesUsingOurOrders(t_security_id_);

          new_int_price_ = smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_;
          new_ordercount_ = smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_ordercount_;
          new_size_ = smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_;

        } else {
          return;
        }
      } else {
        if (ask_index_ >= 0 && ask_index_ < (int)smv_.max_tick_range_) {
          smv_.market_update_info_.asklevels_[ask_index_].mask_time_msecs_ =
              watch_.msecs_from_midnight() + MSECS_TO_MASK_SYN_DEL;
        }
        return;
      }
    } break;
    default: { return; }
  }

  smv_.UpdateL1Prices();

#if CCPROFILING_TRADEINIT
  HFSAT::CpucycleProfiler::GetUniqueInstance(30).End(5);
#endif

  if (smv_.pl_change_listeners_present_) {
    smv_.NotifyOnPLChangeListeners(t_security_id_, smv_.market_update_info_, t_buysell_, 0, new_int_price_, 0,
                                   prev_size_, new_size_, prev_ordercount_, new_ordercount_, false, pl_change_type_);
  }

  if (l1_changed_) {
    smv_.NotifyL1PriceListeners();
  } else {
    smv_.NotifyL2Listeners();
    smv_.NotifyL2OnlyListeners();  // TODO: check if we need additional flag here.
  }

  smv_.NotifyOnReadyListeners();
}

void IndexedCmeMarketViewManager::OnPriceLevelDeleteThrough(const unsigned int t_security_id_,
                                                            const TradeType_t t_buysell_,
                                                            const int t_max_level_deleted_,
                                                            const bool t_is_intermediate_message_) {
  double old_price = GetPriceForLevel(t_security_id_, t_buysell_, t_max_level_deleted_);
  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);
  int int_price = smv_.GetIntPx(old_price);
  switch (t_buysell_) {
    case kTradeTypeBuy: {
      int bid_index = smv_.base_bid_index_ -
                      (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_ - int_price);
      if (bid_index < 0 || bid_index > (int)smv_.max_tick_range_) {
        // not doing anything if the bid index is beyond existing book
        return;
      } else {
        int last_index = smv_.base_bid_index_;

        for (; last_index >= bid_index; last_index--) {
          int price_level = smv_.base_bid_index_ - last_index;  // dummy actually
          bool intermediate = (t_is_intermediate_message_ | (last_index != bid_index));
          OnPriceLevelDelete(t_security_id_, kTradeTypeBuy, price_level,
                             smv_.market_update_info_.bidlevels_[last_index].limit_price_, intermediate);
        }
      }
    } break;
    case kTradeTypeSell: {
      int ask_index = smv_.base_ask_index_ +
                      (smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_ - int_price);
      if (ask_index < 0 || ask_index > (int)smv_.max_tick_range_) {
        return;
      } else {
        int last_index = smv_.base_ask_index_;
        for (; last_index >= ask_index; last_index--) {
          int price_level = smv_.base_ask_index_ - last_index;  // dummy actually
          bool intermediate = (t_is_intermediate_message_ | (last_index != ask_index));

          OnPriceLevelDelete(t_security_id_, kTradeTypeSell, price_level,
                             smv_.market_update_info_.asklevels_[last_index].limit_price_, intermediate);
        }
      }
    } break;
    default: { break; }
  }
}

/**
 *
 * @param t_security_id
 * @param buysell
 * @param t_level
 * @return
 */
double IndexedCmeMarketViewManager::GetPriceForLevel(const unsigned int t_security_id, const TradeType_t buysell,
                                                     int t_level) {
  // returns the ith non-empty starting from 1
  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id]);
  double old_price_ = 0.0;

  // Find the price level to delete
  switch (buysell) {
    case kTradeTypeBuy: {
      int level_ = 1;
      int index_ = smv_.base_bid_index_;

      for (; index_ >= 0; index_--) {
        if (smv_.market_update_info_.bidlevels_[index_].limit_size_ > 0) {
          if (level_ == t_level) {
            old_price_ = smv_.market_update_info_.bidlevels_[index_].limit_price_;
            break;
          }
          level_++;
        }
      }
    } break;
    case kTradeTypeSell: {
      int level_ = 1;
      int index_ = smv_.base_ask_index_;

      for (; index_ >= 0; index_--) {
        if (smv_.market_update_info_.asklevels_[index_].limit_size_ > 0) {
          if (level_ == t_level) {
            old_price_ = smv_.market_update_info_.asklevels_[index_].limit_price_;
            break;
          }

          level_++;
        }
      }
    } break;
    default:
      break;
  }

  return old_price_;
}

/**
 * @param t_buysell_
 * @return TradeType_t
 */
TradeType_t IndexedCmeMarketViewManager::ProcessTradeTypeNoInfo(TradeType_t t_buysell_) {
  /*
  CME provides TradeTypeNoInfo for trades involving implied orders.
  For eg, in a spread (eg SEC1-SEC2), the spread orders are connected to outright orders
  (of SEC1 and SEC2). So when a spread is traded, there's no 'aggressive' order for these
  outright orders. Hence the tradetype for that trade is TradeTypeNoInfo.

  Since we want to relay this information unchanged to end user (as TradeTypeNoInfo) for
  now, we don't process it.

  In case this information needs to be relayed in a different format, please change here.
  */
  return t_buysell_;
}
}
