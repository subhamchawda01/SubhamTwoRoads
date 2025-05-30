/**
   \file MarketAdapterCode/indexed_fpga_market_view_manager.cpp

   \author: (c) Copyright K2 Research LLC 2010
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/

#include "baseinfra/MarketAdapter/indexed_fpga_market_view_manager.hpp"

namespace HFSAT {

#define LOW_ACCESS_INDEX 50
#define MSECS_TO_MASK_SYN_DEL 100

IndexedFpgaMarketViewManager::IndexedFpgaMarketViewManager(
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

BookManagerErrorCode_t IndexedFpgaMarketViewManager::UpdateBids(const unsigned int t_security_id_,
                                                                FPGAHalfBook* t_half_book_) {
  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);

  bool l1_changed_ = false;

  for (size_t i = 0; i < 5; i++) {
    // If the size is -1 for this level,
    // it means there is not update at this level
    // and it is same as what was previously there.
    if (t_half_book_->sizes_[i] == -1) {
      continue;
    }

    int bid_int_price_ = smv_.GetIntPx(t_half_book_->prices_[i]);
    bool bid_px_zero = false;
    // If the price is 0 => pick old price
    if (bid_int_price_ == 0) {
      bid_px_zero = true;
      t_half_book_->prices_[i] = smv_.bid_price(i);  // old price at this level
      bid_int_price_ = smv_.GetIntPx(t_half_book_->prices_[i]);
    }
    int bid_index_ = smv_.base_bid_index_ -
                     (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_ - bid_int_price_);
    if (t_half_book_->sizes_[i] == 0 && t_half_book_->num_orders_[i] == 0 && bid_px_zero) {
      // If the price, size, num_of_orders is 0 => delete all levels below it
      while (bid_index_ >= 0) {
        if (bid_index_ < (int)smv_.market_update_info_.bidlevels_.size()) {
          smv_.market_update_info_.bidlevels_[bid_index_].limit_size_ = 0;
        }
        bid_index_--;
      }
      continue;
    }

    if (i == 0) {
      BookManagerErrorCode_t error_ = AdjustBidIndex(t_security_id_, bid_int_price_, bid_index_);

      if (error_ == kBookManagerReturn) {
        return error_;
      }

      // If we have masked this level due to synthetic delete, then don't update the base_bid_index
      if (watch_.msecs_from_midnight() < smv_.market_update_info_.bidlevels_[bid_index_].mask_time_msecs_) {
        continue;
      }

      // Delete the levels above this level
      while ((int)smv_.base_bid_index_ > bid_index_) {
        smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_ = 0;
        smv_.base_bid_index_--;
      }

      l1_changed_ = true;
      smv_.base_bid_index_ = bid_index_;
      // sec_id_to_bid_indexes_[ t_security_id_ ][ i ] = bid_index_;
    } else {
      if (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_ <= 0 || bid_index_ < 0) {
        return kBookManagerReturn;
      }

      //            if ( sec_id_to_bid_indexes_[ t_security_id_ ][ i ] != bid_index_ )
      //              {
      //                for ( int temp_bid_index_ = sec_id_to_bid_indexes_[ t_security_id_ ][ i - 1 ] - 1;
      //                temp_bid_index_ > bid_index_; temp_bid_index_-- )
      //                  {
      //                    smv_.market_update_info_.bidlevels_[ temp_bid_index_ ].limit_size_ = 0;
      //                  }
      //                sec_id_to_bid_indexes_[ t_security_id_ ][ i ] = bid_index_;
      //              }
    }
    // If we have masked this level due to synthetic delete, then don't update this level as this was deleted
    if (watch_.msecs_from_midnight() >= smv_.market_update_info_.bidlevels_[bid_index_].mask_time_msecs_) {
      smv_.market_update_info_.bidlevels_[bid_index_].limit_size_ = t_half_book_->sizes_[i];
      smv_.market_update_info_.bidlevels_[bid_index_].limit_ordercount_ = t_half_book_->num_orders_[i];
    }
  }

  if (l1_changed_) {
    return kBookManagerL1Changed;
  }

  return kBookManagerOK;
}

BookManagerErrorCode_t IndexedFpgaMarketViewManager::UpdateAsks(const unsigned int t_security_id_,
                                                                FPGAHalfBook* t_half_book_) {
  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);

  bool l1_changed_ = false;

  for (size_t i = 0; i < 5; i++) {
    // If the size is -1 for this level,
    // it means there is not update at this level
    // and it is same as what was previously there.
    if (t_half_book_->sizes_[i] == -1) {
      continue;
    }

    int ask_int_price_ = smv_.GetIntPx(t_half_book_->prices_[i]);
    bool ask_px_zero = false;
    // If the price is 0 => pick old price
    if (ask_int_price_ == 0) {
      ask_px_zero = true;
      t_half_book_->prices_[i] = smv_.ask_price(i);  // old price at this level
      ask_int_price_ = smv_.GetIntPx(t_half_book_->prices_[i]);
    }
    int ask_index_ = smv_.base_ask_index_ +
                     (smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_ - ask_int_price_);
    if (t_half_book_->sizes_[i] == 0 && t_half_book_->num_orders_[i] == 0 && ask_px_zero) {
      // If the price, size, num_of_orders is 0 => delete all levels below it
      while (ask_index_ >= 0) {
        if (ask_index_ < (int)smv_.market_update_info_.asklevels_.size()) {
          smv_.market_update_info_.asklevels_[ask_index_].limit_size_ = 0;
        }
        ask_index_--;
      }
      continue;
    }

    // If this is the first level, check if there is any violation of index limits
    if (i == 0) {
      BookManagerErrorCode_t error_ = AdjustAskIndex(t_security_id_, ask_int_price_, ask_index_);

      if (error_ == kBookManagerReturn) {
        return error_;
      }

      // If we have masked this level due to synthetic delete, then don't update the base_ask_index
      if (watch_.msecs_from_midnight() < smv_.market_update_info_.asklevels_[ask_index_].mask_time_msecs_) {
        continue;
      }

      // Delete the levels above this level
      while ((int)smv_.base_ask_index_ > ask_index_) {
        smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_ = 0;
        smv_.base_ask_index_--;
      }

      l1_changed_ = true;
      smv_.base_ask_index_ = ask_index_;
      //            sec_id_to_ask_indexes_[ t_security_id_ ][ i ] = ask_index_;
    } else {
      if (smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_ <= 0 || ask_index_ < 0) {
        return kBookManagerReturn;
      }

      //            if ( sec_id_to_ask_indexes_[ t_security_id_ ][ i ] != ask_index_ )
      //              {
      //                for ( int temp_ask_index_ = sec_id_to_ask_indexes_[ t_security_id_ ][ i - 1 ] - 1;
      //                temp_ask_index_ > ask_index_; temp_ask_index_-- )
      //                  {
      //                    smv_.market_update_info_.asklevels_[ temp_ask_index_ ].limit_size_ = 0;
      //                  }
      //                sec_id_to_ask_indexes_[ t_security_id_ ][ i ] = ask_index_;
      //              }
    }

    // If we have masked this level due to synthetic delete, then don't update this level as this was deleted
    if (watch_.msecs_from_midnight() >= smv_.market_update_info_.asklevels_[ask_index_].mask_time_msecs_) {
      smv_.market_update_info_.asklevels_[ask_index_].limit_size_ = t_half_book_->sizes_[i];
      smv_.market_update_info_.asklevels_[ask_index_].limit_ordercount_ = t_half_book_->num_orders_[i];
    }
  }

  if (l1_changed_) {
    return kBookManagerL1Changed;
  }

  return kBookManagerOK;
}

void IndexedFpgaMarketViewManager::OnHalfBookChange(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                                    FPGAHalfBook* t_half_book_, bool is_intermediate_) {
  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);

  if (!smv_.initial_book_constructed_) {
    // Build the initial index only from the first level
    // For that, check if the level one info is preset
    // in the current update.
    if (t_half_book_->sizes_[0] > 0) {
      BuildIndex(t_security_id_, t_buysell_, smv_.GetIntPx(t_half_book_->prices_[0]));

      smv_.initial_book_constructed_ = true;
    } else {
      return;
    }
  }

  // This is used for OnPL notifications
  int prev_size_ = 1;
  int prev_ordercount_ = 1;
  int prev_int_price_ = 1;

  int new_size_ = 1;
  int new_ordercount_ = 1;
  int new_int_price_ = 1;

  char pl_change_type_ = 'C';

  bool l1_changed_ = false;
  BookManagerErrorCode_t update_error_ = kBookManagerOK;
  BookManagerErrorCode_t sanity_error_ = kBookManagerOK;

  switch (t_buysell_) {
    case kTradeTypeBuy: {
      prev_int_price_ = smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_;
      prev_ordercount_ = smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_ordercount_;
      prev_size_ = smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_;

      update_error_ = UpdateBids(t_security_id_, t_half_book_);

      new_int_price_ = smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_;
      new_ordercount_ = smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_ordercount_;
      new_size_ = smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_;

      if (new_int_price_ > prev_int_price_) {
        pl_change_type_ = 'N';
      } else if (new_int_price_ < prev_int_price_) {
        pl_change_type_ = 'D';
      }

      if (update_error_ == kBookManagerReturn) {
        return;
      }

      if (!is_intermediate_) {
        sanity_error_ = SanitizeAskSide(t_security_id_);
      }
    } break;

    case kTradeTypeSell: {
      prev_int_price_ = smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_;
      prev_ordercount_ = smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_ordercount_;
      prev_size_ = smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_;

      update_error_ = UpdateAsks(t_security_id_, t_half_book_);

      new_int_price_ = smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_;
      new_ordercount_ = smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_ordercount_;
      new_size_ = smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_;

      if (new_int_price_ < prev_int_price_) {
        pl_change_type_ = 'N';
      } else if (new_int_price_ > prev_int_price_) {
        pl_change_type_ = 'D';
      }

      if (update_error_ == kBookManagerReturn) {
        return;
      }

      if (!is_intermediate_) {
        sanity_error_ = SanitizeBidSide(t_security_id_);
      }
    } break;

    default:
      break;
  }

  if (sanity_error_ == kBookManagerReturn) {
    return;
  }

  if (update_error_ == kBookManagerL1Changed) {
    switch (t_buysell_) {
      case kTradeTypeBuy: {
        UpdateBestBidVariables(t_security_id_, smv_.base_bid_index_);
        UpdateBestBidVariablesUsingOurOrders(t_security_id_);
      } break;
      case kTradeTypeSell: {
        UpdateBestAskVariables(t_security_id_, smv_.base_ask_index_);
        UpdateBestAskVariablesUsingOurOrders(t_security_id_);
      } break;
      default:
        break;
    }

    smv_.UpdateL1Prices();
    l1_changed_ = true;
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

  if (smv_.pl_change_listeners_present_ && !is_intermediate_) {
    smv_.NotifyOnPLChangeListeners(t_security_id_, smv_.market_update_info_, t_buysell_, 0, new_int_price_, 0,
                                   prev_size_, new_size_, prev_ordercount_, new_ordercount_, false, pl_change_type_);
  }

  if (!is_intermediate_) {
    if (l1_changed_) {
      smv_.NotifyL1PriceListeners();
    } else {
      smv_.NotifyL2Listeners();
      smv_.NotifyL2OnlyListeners();  // TODO: check if we need additional flag here.
    }

    smv_.NotifyOnReadyListeners();
  }
}

void IndexedFpgaMarketViewManager::OnTrade(const unsigned int t_security_id_, const double t_trade_price_,
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

  smv_.StorePreTrade();

  switch (t_buysell_) {
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

              smv_.market_update_info_.bestask_int_price_ =
                  smv_.market_update_info_.asklevels_[next_ask_index_].limit_int_price_;
              smv_.market_update_info_.bestask_ordercount_ =
                  smv_.market_update_info_.asklevels_[next_ask_index_].limit_ordercount_;
              smv_.market_update_info_.bestask_price_ =
                  smv_.market_update_info_.asklevels_[next_ask_index_].limit_price_;
              smv_.market_update_info_.bestask_size_ = smv_.market_update_info_.asklevels_[next_ask_index_].limit_size_;
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
            smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_ - t_trade_size_;
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

              smv_.market_update_info_.bestbid_int_price_ =
                  smv_.market_update_info_.bidlevels_[next_bid_index_].limit_int_price_;
              smv_.market_update_info_.bestbid_ordercount_ =
                  smv_.market_update_info_.bidlevels_[next_bid_index_].limit_ordercount_;
              smv_.market_update_info_.bestbid_price_ =
                  smv_.market_update_info_.bidlevels_[next_bid_index_].limit_price_;
              smv_.market_update_info_.bestbid_size_ = smv_.market_update_info_.bidlevels_[next_bid_index_].limit_size_;
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

      if (t_trade_size_ >= smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_) {
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
            smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_ - t_trade_size_;
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
  smv_.trade_print_info_.buysell_ = t_buysell_;

  smv_.SetTradeVarsForIndicatorsIfRequired();

  if (smv_.is_ready_) {
    smv_.NotifyTradeListeners();
    smv_.NotifyOnReadyListeners();
  }

  sec_id_to_prev_update_was_quote_[t_security_id_] = false;
}

void IndexedFpgaMarketViewManager::OnPriceLevelDeleteSynthetic(const unsigned int t_security_id_,
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
      }
    } break;
    default: { return; }
  }

  smv_.UpdateL1Prices();

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
}
