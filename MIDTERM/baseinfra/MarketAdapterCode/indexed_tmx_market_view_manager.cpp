/**
   \file MarketAdapterCode/indexed_tmx_market_view_manager.cpp

   \author: (c) Copyright K2 Research LLC 2010
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#include "baseinfra/MarketAdapter/indexed_tmx_market_view_manager.hpp"
#define CCPROFILING_TRADEINIT 1

namespace HFSAT {

#define LOW_ACCESS_INDEX 50

IndexedTmxMarketViewManager::IndexedTmxMarketViewManager(
    DebugLogger& t_dbglogger_, const Watch& t_watch_, SecurityNameIndexer& t_sec_name_indexer_,
    const std::vector<SecurityMarketView*>& t_security_market_view_map_)
    : BaseMarketViewManager(t_dbglogger_, t_watch_, t_sec_name_indexer_, t_security_market_view_map_),
      sec_id_to_prev_update_was_quote_(t_sec_name_indexer_.NumSecurityId(), false),
      sec_id_to_bid_indexes_(t_sec_name_indexer_.NumSecurityId()),
      sec_id_to_ask_indexes_(t_sec_name_indexer_.NumSecurityId()) {
  for (auto i = 0u; i < t_sec_name_indexer_.NumSecurityId(); i++) {
    sec_id_to_bid_indexes_[i].resize(FULL_BOOK_DEPTH, 0);
    sec_id_to_ask_indexes_[i].resize(FULL_BOOK_DEPTH, 0);
  }
}

void IndexedTmxMarketViewManager::OnFullBookChange(const unsigned int t_security_id_, const FullBook* t_full_book_) {
  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);

  if (!smv_.initial_book_constructed_) {
    if (t_full_book_->bid_size_[0] != 0) {
      BuildIndex(t_security_id_, kTradeTypeBuy, smv_.GetIntPx(t_full_book_->bid_price_[0]));

      smv_.initial_book_constructed_ = true;
    } else {
      return;
    }
  }

  bool l1_changed_ = false;

  for (size_t i = 0; i < FULL_BOOK_DEPTH; i++) {
    if (t_full_book_->bid_size_[i] == 0 && t_full_book_->ask_size_[i] == 0) {
      continue;
    }

    if (t_full_book_->bid_size_[i] != 0) {
      // Update bids
      int bid_int_price_ = smv_.GetIntPx(t_full_book_->bid_price_[i]);

      int bid_index_ = smv_.base_bid_index_ -
                       (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_ - bid_int_price_);

      // If this is the first level, check if there is any violation of index limits
      if (i == 0) {
        // There are 0 levels on bid side
        if (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_ <= 0) {
          // TODO: Check if this is possible for TMX
          if (bid_index_ < LOW_ACCESS_INDEX) {
            RebuildIndexLowAccess(t_security_id_, kTradeTypeBuy, bid_int_price_);
            bid_index_ = smv_.base_bid_index_;
          } else if (bid_index_ >= (int)smv_.max_tick_range_) {
            RebuildIndexHighAccess(t_security_id_, kTradeTypeBuy, bid_int_price_);
            bid_index_ = smv_.base_bid_index_;
          }

          smv_.base_bid_index_ = bid_index_;
        } else if (bid_index_ < 0) {
          return;
        }

        if (bid_index_ >= (int)smv_.max_tick_range_) {
          RebuildIndexHighAccess(t_security_id_, kTradeTypeBuy, bid_int_price_);
          bid_index_ = smv_.base_bid_index_;
        }

        // Delete the levels above this level
        while ((int)smv_.base_bid_index_ > bid_index_) {
          smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_ = 0;
          smv_.base_bid_index_--;
        }

        l1_changed_ = true;
        smv_.base_bid_index_ = bid_index_;
        sec_id_to_bid_indexes_[t_security_id_][i] = bid_index_;
      } else {
        if (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_ <= 0 || bid_index_ < 0) {
          return;
        }

        if (sec_id_to_bid_indexes_[t_security_id_][i] != bid_index_) {
          for (int temp_bid_index_ = sec_id_to_bid_indexes_[t_security_id_][i - 1] - 1; temp_bid_index_ > bid_index_;
               temp_bid_index_--) {
            smv_.market_update_info_.bidlevels_[temp_bid_index_].limit_size_ = 0;
          }
          sec_id_to_bid_indexes_[t_security_id_][i] = bid_index_;
        }
      }

      smv_.market_update_info_.bidlevels_[bid_index_].limit_size_ = t_full_book_->bid_size_[i];
      smv_.market_update_info_.bidlevels_[bid_index_].limit_ordercount_ = t_full_book_->bid_ordercount_[i];
    } else if (i == 0)  // there are no bids right now.
    {
      smv_.is_ready_ = false;
      DBGLOG_TIME_CLASS_FUNC_LINE << " " << smv_.secname() << " There are no bids right now " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }

    if (t_full_book_->ask_size_[i]) {
      // Update asks
      int ask_int_price_ = smv_.GetIntPx(t_full_book_->ask_price_[i]);

      int ask_index_ = smv_.base_ask_index_ +
                       (smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_ - ask_int_price_);

      // If this is the first level, check if there is any violation of index limits
      if (i == 0) {
        // There are 0 levels on bid side
        if (smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_ <= 0) {
          // TODO: Check if this is possible for TMX
          if (ask_index_ < LOW_ACCESS_INDEX) {
            RebuildIndexLowAccess(t_security_id_, kTradeTypeSell, ask_int_price_);
            ask_index_ = smv_.base_ask_index_;
          } else if (ask_index_ >= (int)smv_.max_tick_range_) {
            RebuildIndexHighAccess(t_security_id_, kTradeTypeSell, ask_int_price_);
            ask_index_ = smv_.base_ask_index_;
          }

          smv_.base_ask_index_ = ask_index_;
        } else if (ask_index_ < 0) {
          return;
        }

        if (ask_index_ >= (int)smv_.max_tick_range_) {
          RebuildIndexHighAccess(t_security_id_, kTradeTypeSell, ask_int_price_);
          ask_index_ = smv_.base_ask_index_;
        }

        // Delete the levels above this level
        while ((int)smv_.base_ask_index_ > ask_index_) {
          smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_ = 0;
          smv_.base_ask_index_--;
        }

        l1_changed_ = true;
        smv_.base_ask_index_ = ask_index_;
        sec_id_to_ask_indexes_[t_security_id_][i] = ask_index_;
      } else {
        if (smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_ <= 0 || ask_index_ < 0) {
          return;
        }

        if (sec_id_to_ask_indexes_[t_security_id_][i] != ask_index_) {
          for (int temp_ask_index_ = sec_id_to_ask_indexes_[t_security_id_][i - 1] - 1; temp_ask_index_ > ask_index_;
               temp_ask_index_--) {
            smv_.market_update_info_.asklevels_[temp_ask_index_].limit_size_ = 0;
          }
          sec_id_to_ask_indexes_[t_security_id_][i] = ask_index_;
        }
      }

      smv_.market_update_info_.asklevels_[ask_index_].limit_size_ = t_full_book_->ask_size_[i];
      smv_.market_update_info_.asklevels_[ask_index_].limit_ordercount_ = t_full_book_->ask_ordercount_[i];
    } else if (i == 0)  // there are no asks right now.
    {
      smv_.is_ready_ = false;
      DBGLOG_TIME_CLASS_FUNC_LINE << " " << smv_.secname() << " There are no asks right now " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
  }

  if (l1_changed_) {
    UpdateBestBidVariables(t_security_id_, smv_.base_bid_index_);

    UpdateBestAskVariables(t_security_id_, smv_.base_ask_index_);

    UpdateBestVariablesUsingOurOrders(t_security_id_);
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

#if CCPROFILING_TRADEINIT
  HFSAT::CpucycleProfiler::GetUniqueInstance(30).End(10);
#endif

  if (l1_changed_) {
    smv_.NotifyL1PriceListeners();
  } else {
    smv_.NotifyL2Listeners();
    smv_.NotifyL2OnlyListeners();  // TODO: check if we need additional flag here.
  }

  smv_.NotifyOnReadyListeners();
}

void IndexedTmxMarketViewManager::OnTrade(const unsigned int t_security_id_, const double t_trade_price_,
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

              UpdateBestAskVariablesUsingOurOrders(t_security_id_);
            } else {
              smv_.market_update_info_.bestask_size_ -= t_trade_size_;
            }
          }
        } else {
          smv_.market_update_info_.bestask_int_price_ = int_trade_price_;
          smv_.market_update_info_.bestask_ordercount_ = 1;
          smv_.market_update_info_.bestask_size_ = 1;
          smv_.market_update_info_.bestask_price_ = smv_.GetDoublePx(int_trade_price_);
          if (!smv_.price_to_yield_map_.empty()) {
            smv_.hybrid_market_update_info_.bestask_int_price_ = int_trade_price_;
            smv_.hybrid_market_update_info_.bestask_ordercount_ = 1;
            smv_.hybrid_market_update_info_.bestask_size_ = 1;
            smv_.hybrid_market_update_info_.bestask_price_ = smv_.price_to_yield_map_[int_trade_price_];
          }
        }
        break;
      }

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
          if (!smv_.price_to_yield_map_.empty()) {
            smv_.hybrid_market_update_info_.bestbid_int_price_ = int_trade_price_;
            smv_.hybrid_market_update_info_.bestbid_ordercount_ = 1;
            smv_.hybrid_market_update_info_.bestbid_price_ = smv_.price_to_yield_map_[int_trade_price_];
            smv_.hybrid_market_update_info_.bestbid_size_ = 1;
          }
        }
        break;
      }

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
  HFSAT::CpucycleProfiler::GetUniqueInstance(30).End(10);
#endif

  if (smv_.is_ready_) {
    smv_.NotifyTradeListeners();
    smv_.NotifyOnReadyListeners();
  }

  sec_id_to_prev_update_was_quote_[t_security_id_] = false;
}

void IndexedTmxMarketViewManager::OnL1Change(const unsigned int t_security_id, double bid_px, int bid_sz, int bid_count,
                                             double ask_px, int ask_sz, int ask_count) {}
}
