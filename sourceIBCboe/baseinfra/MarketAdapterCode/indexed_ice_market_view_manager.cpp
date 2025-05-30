/**
   \file MarketAdapterCode/indexed_ice_market_view_manager.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 162, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#include "baseinfra/MarketAdapter/indexed_ice_market_view_manager.hpp"
#define CCPROFILING_TRADEINIT 0

namespace HFSAT {

#define LOW_ACCESS_INDEX 50

IndexedIceMarketViewManager::IndexedIceMarketViewManager(
    DebugLogger& t_dbglogger_, const Watch& t_watch_, const SecurityNameIndexer& t_sec_name_indexer_,
    const std::vector<SecurityMarketView*>& t_security_market_view_map_)
    : BaseMarketViewManager(t_dbglogger_, t_watch_, t_sec_name_indexer_, t_security_market_view_map_),
      sec_id_to_prev_update_was_quote_(t_sec_name_indexer_.NumSecurityId(), false) {}

void IndexedIceMarketViewManager::DropIndexedBookForSource(HFSAT::ExchSource_t t_exch_source_,
                                                           const int t_security_id_) {
  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);

  HFSAT::ExchSource_t this_exch_source_ =
      HFSAT::SecurityDefinitions::GetContractExchSource(smv_.shortcode(), watch_.YYYYMMDD());

  if (this_exch_source_ != t_exch_source_) return;

  smv_.market_update_info_.bidlevels_.clear();
  smv_.market_update_info_.asklevels_.clear();
}

/*
 * ICE market data price level new notes:
 * 1. Implicit delete for levels beyond 5
 */
void IndexedIceMarketViewManager::OnPriceLevelNew(const unsigned int t_security_id_, const TradeType_t t_buysell_,
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

      // If level == 5, clear all levels between level 4 and the current level
      if (t_level_added_ == 5) {
        int level_ = 1;
        int index4_ = (int)smv_.base_bid_index_ - 1;
        for (; index4_ > bid_index_; index4_--) {
          if (smv_.market_update_info_.bidlevels_[index4_].limit_size_ > 0) {
            level_++;
          }

          if (level_ == 4) {
            break;
          }
        }

        index4_--;

        for (; index4_ > bid_index_; index4_--) {
          smv_.market_update_info_.bidlevels_[index4_].limit_size_ = 0;

          // Reset the relevant bits, don't care about levels > 5 anyways
          if ((smv_.base_bid_index_ - index4_) < smv_.bitmask_size_) {
            smv_.bid_access_bitmask_ &= (~(smv_.bitmask_lookup_table_[(smv_.base_bid_index_ - index4_)]));
          }
        }
      }

      // Store old size and order count for OnPL change listeners. ( Not sure if having a bool check will help )
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

      //          if ( smv_.bid_int_price( t_level_added_ - 1 ) != int_price_ )
      //            {
      //              std::cout << smv_.shortcode ( ) << " bid_int_price_ at level: " << t_level_added_ << " is: " <<
      //              smv_.bid_int_price ( t_level_added_ - 1 )
      //                  << ". It does not match with: " << int_price_ << " at: " << watch_.tv ( ) << std::endl;
      //            }
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

      // If level == 5, clear all levels between level 4 and the current level
      if (t_level_added_ == 5) {
        int level_ = 1;
        int index4_ = (int)smv_.base_ask_index_ - 1;
        for (; index4_ > ask_index_; index4_--) {
          if (smv_.market_update_info_.asklevels_[index4_].limit_size_ > 0) {
            level_++;
          }

          if (level_ == 4) {
            break;
          }
        }

        index4_--;

        for (; index4_ > ask_index_; index4_--) {
          smv_.market_update_info_.asklevels_[index4_].limit_size_ = 0;

          // Reset the relevant bits, don't care about levels > 5 anyways
          if ((smv_.base_ask_index_ - index4_) < smv_.bitmask_size_) {
            smv_.ask_access_bitmask_ &= (!(smv_.bitmask_lookup_table_[(smv_.base_ask_index_ - index4_)]));
          }
        }
      }

      old_size_ = smv_.market_update_info_.asklevels_[ask_index_].limit_size_;
      old_ordercount_ = smv_.market_update_info_.asklevels_[ask_index_].limit_ordercount_;

      smv_.market_update_info_.asklevels_[ask_index_].limit_size_ = t_new_size_;
      smv_.market_update_info_.asklevels_[ask_index_].limit_ordercount_ = t_new_ordercount_;

      if (ask_index_ >= (int)smv_.base_ask_index_) {
        smv_.l1_changed_since_last_ = true;

        // Bitmask - again make sure we never change the order of base indexe assignment
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
        UpdateBestBidVariables(t_security_id_, smv_.base_bid_index_);

        smv_.UpdateL1Prices();
      } break;
      case kTradeTypeSell: {
        UpdateBestAskVariables(t_security_id_, smv_.base_ask_index_);
        smv_.UpdateL1Prices();
      } break;
      default:
        break;
    }
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
  HFSAT::CpucycleProfiler::GetUniqueInstance(30).End(8);
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

void IndexedIceMarketViewManager::OnPriceLevelChange(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                                     const int t_level_changed_, const double t_price_,
                                                     const int t_new_size_, const int t_new_ordercount_,
                                                     const bool t_is_intermediate_message_) {
  OnPriceLevelNew(t_security_id_, t_buysell_, t_level_changed_, t_price_, t_new_size_, t_new_ordercount_,
                  t_is_intermediate_message_);
}

/*
 * ICE Price level delete notes:
 * 1. We don't get price information here.
 */
void IndexedIceMarketViewManager::OnPriceLevelDelete(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                                     const int t_level_removed_, const double t_price_,
                                                     const bool t_is_intermediate_message_) {
  // Skip levels more than 5
  if (t_level_removed_ > 5) {
    return;
  }

  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);

  if (!smv_.initial_book_constructed_) {
    return;
  }

  int old_size_ = 0;
  int old_ordercount_ = 0;
  int int_price_ = 0;

  switch (t_buysell_) {
    case kTradeTypeBuy: {
      int bid_index_ = smv_.base_bid_index_;

      // TODO: Optimize following function - only iterate through first 15-20 levels to reduce the effect of worst case
      for (int level_ = 1; level_ < t_level_removed_; level_++) {
        // Get next index
        do {
          bid_index_--;
        } while (bid_index_ >= 0 && smv_.market_update_info_.bidlevels_[bid_index_].limit_size_ <= 0);

        if (bid_index_ < 0) {
          smv_.is_ready_ = false;
          return;
        }
      }

      int_price_ = smv_.market_update_info_.bidlevels_[bid_index_].limit_int_price_;

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
      int ask_index_ = smv_.base_ask_index_;

      // TODO: Optimize
      for (int level_ = 1; level_ < t_level_removed_; level_++) {
        // Get next index
        do {
          ask_index_--;
        } while (ask_index_ >= 0 && smv_.market_update_info_.asklevels_[ask_index_].limit_size_ <= 0);

        if (ask_index_ < 0) {
          smv_.is_ready_ = false;
          return;
        }
      }

      int_price_ = smv_.market_update_info_.asklevels_[ask_index_].limit_int_price_;

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
        UpdateBestBidVariables(t_security_id_, smv_.base_bid_index_);

        smv_.UpdateL1Prices();
      } break;
      case kTradeTypeSell: {
        UpdateBestAskVariables(t_security_id_, smv_.base_ask_index_);
        smv_.UpdateL1Prices();
      } break;
      default:
        break;
    }
  }

  if (smv_.market_update_info_.trade_update_implied_quote_) {
    smv_.market_update_info_.trade_update_implied_quote_ = false;
    smv_.trade_print_info_.num_trades_++;
  }

  sec_id_to_prev_update_was_quote_[t_security_id_] = true;

  if (!smv_.is_ready_ || t_is_intermediate_message_) {
    return;
  }

#if CCPROFILING_TRADEINIT
  HFSAT::CpucycleProfiler::GetUniqueInstance(30).End(8);
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

/*
 * ICE Trade message notes:
 * 1. We don't get aggressor side here.
 * 2. Discard levels above the trade price
 */
void IndexedIceMarketViewManager::OnTrade(const unsigned int t_security_id_, const double t_trade_price_,
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
    if (int_trade_price_ <= smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_) {
      buysell_ = kTradeTypeSell;
    } else if (int_trade_price_ >= smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_) {
      buysell_ = kTradeTypeBuy;
    }
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
            smv_.hybrid_market_update_info_.bestask_price_ =
                smv_.price_to_yield_map_[smv_.hybrid_market_update_info_.bestask_int_price_];
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
  HFSAT::CpucycleProfiler::GetUniqueInstance(30).End(8);
#endif

  if (smv_.is_ready_) {
    smv_.NotifyTradeListeners();
    smv_.NotifyOnReadyListeners();
  }

  sec_id_to_prev_update_was_quote_[t_security_id_] = false;
}

void IndexedIceMarketViewManager::OnResetBegin(const unsigned int t_security_id_) {
  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);
  smv_.InitializeSMVForIndexedBook();
}
void IndexedIceMarketViewManager::OnResetEnd(const unsigned int t_security_id_) {}
}
