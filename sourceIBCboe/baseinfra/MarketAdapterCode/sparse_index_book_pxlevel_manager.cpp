/**
   \file MarketAdapterCode/sparse_index_book_pxlevel_manager.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#define USE_HASH_DS 1
#include "baseinfra/MarketAdapter/sparse_index_book_pxlevel_manager.hpp"
#include "dvccode/CDef/nse_security_definition.hpp"
#include "dvccode/CDef/bse_security_definition.hpp"

namespace HFSAT {
SparseIndexBookPxLevelManager::SparseIndexBookPxLevelManager(DebugLogger& dbglogger, const Watch& watch, int num_sec_id)
    : dbglogger_(dbglogger), watch_(watch), num_securities_(num_sec_id), one_pxlevel_mempool_() {
  change_status_ = k_nochange;
// change to derived class here
#if USE_HASH_DS
  underlying_bid_ds_ = new SparseIndexBookHashExtendedDataStructure(dbglogger, watch, num_securities_,
                                                                    one_pxlevel_mempool_, kTradeTypeBuy);
  underlying_ask_ds_ = new SparseIndexBookHashExtendedDataStructure(dbglogger, watch, num_securities_,
                                                                    one_pxlevel_mempool_, kTradeTypeSell);
#else
  underlying_bid_ds_ =
      new SparseIndexBookMapExtendedDataStructure(dbglogger, watch, num_securities_, one_pxlevel_mempool_);
  underlying_ask_ds_ =
      new SparseIndexBookMapExtendedDataStructure(dbglogger, watch, num_securities_, one_pxlevel_mempool_);
#endif

  is_book_crossed_.resize(num_sec_id, false);
  init_book_cross_time_.resize(num_sec_id, 0);
  is_crossing_side_bid_.resize(num_sec_id, false);

  sid_2_base_bid_levels_.resize(num_sec_id,
                                std::vector<MarketUpdateInfoLevelStruct*>(MAX_NUM_SORTED_BOOK_LEVELS, NULL));
  sid_2_base_ask_levels_.resize(num_sec_id,
                                std::vector<MarketUpdateInfoLevelStruct*>(MAX_NUM_SORTED_BOOK_LEVELS, NULL));

  MarketUpdateInfoLevelStruct t_struct(SP_INDEX_BOOK_INVALID_PX_LEVEL, kInvalidIntPrice, kInvalidPrice, 0, 0,
                                       watch_.tv());
  sid_2_best_uncrossed_bid_level_.resize(num_sec_id, t_struct);
  sid_2_best_uncrossed_ask_level_.resize(num_sec_id, t_struct);

  num_valid_bid_levels_.resize(num_sec_id, 0);
  num_valid_ask_levels_.resize(num_sec_id, 0);

  using_predictive_uncross_.resize(num_sec_id, false);
}

SparseIndexBookPxLevelManager::~SparseIndexBookPxLevelManager() {}

void SparseIndexBookPxLevelManager::CheckAndSetPredictiveUncross(int sec_id_, std::string shortcode_) {

  if(shortcode_.find("BSE_") != std::string::npos){
    using_predictive_uncross_[sec_id_] = !HFSAT::BSESecurityDefinitions::IsHiddenOrderAvailable(shortcode_);
  }else{
    using_predictive_uncross_[sec_id_] = !HFSAT::NSESecurityDefinitions::IsHiddenOrderAvailable(shortcode_);
  }
}

// size_ is negative if size at pxlevel needs to be decremented (eg on Delete)
LevelChangeType_t SparseIndexBookPxLevelManager::ModifySizeAtPxLevel(int security_id_, TradeType_t buysell_,
                                                                     int int_price_, int size_,
                                                                     bool decrement_ordercount_) {
  //  dbglogger_ << "SparseIndexBookPxLevelManager::ModifySizeAtPxLevel " << security_id_ << ' ' <<
  //  (buysell_==HFSAT::kTradeTypeBuy?"BUY":"SELL")
  //	     << ' ' << size_ << " @ " << int_price_ << ' ' << (decrement_ordercount_?"DELETE":"") << '\n';
  StorePreEventState(security_id_);
  MarketUpdateInfoLevelStruct* updated_level_ = NULL;
  MarketUpdateInfoLevelStruct* prev_bid_best_lvl_ = NULL;
  MarketUpdateInfoLevelStruct* prev_ask_best_lvl_ = NULL;

  if (!using_predictive_uncross_[security_id_]) {
    if (init_book_cross_time_[security_id_] > 0) {
      if (is_crossing_side_bid_[security_id_]) {
        prev_bid_best_lvl_ = GetBidPL(security_id_, 1);
        prev_ask_best_lvl_ = GetAskPL(security_id_, 0);
      } else {
        prev_bid_best_lvl_ = GetBidPL(security_id_, 0);
        prev_ask_best_lvl_ = GetAskPL(security_id_, 1);
      }
    } else {
      prev_bid_best_lvl_ = GetBidPL(security_id_, 0);
      prev_ask_best_lvl_ = GetAskPL(security_id_, 0);
    }
  }

  if (buysell_ == HFSAT::kTradeTypeBuy) {
    updated_level_ = underlying_bid_ds_->ChangeSizeAtPriceLevel(security_id_, int_price_, size_, decrement_ordercount_);
  } else {
    updated_level_ = underlying_ask_ds_->ChangeSizeAtPriceLevel(security_id_, int_price_, size_, decrement_ordercount_);
  }

  if(updated_level_ == NULL) return k_nochange; 

  // check to see if L2 callback condition is met
  if (updated_level_->limit_int_price_level_ >= 0 &&
      updated_level_->limit_int_price_level_ < NUM_LEVELS_FOR_L2_CALLBACK) {
    pass_check_for_l2_callback_ = true;
  }

  if (updated_level_->limit_ordercount_ == 0) {
    // Level needs to be deleted if it is contained in the vectorized book
    CheckAndAssignPriceLevel(security_id_, buysell_, updated_level_);
    one_pxlevel_mempool_.DeAlloc(updated_level_);
  }

  // Synthetic uncrossed vars might change on a delete
  if (using_predictive_uncross_[security_id_]) {
    CheckAndUncrossBook(security_id_, buysell_);
  } else {
    CheckAndUncrossBookForHiddenOrders(security_id_, buysell_, prev_bid_best_lvl_, prev_ask_best_lvl_);
  }

  //  CheckConsistency(security_id_);

  SetEventChangeState(security_id_);
  return change_status_;
}

LevelChangeType_t SparseIndexBookPxLevelManager::ModifyOrderAtDiffLevels(int security_id_, TradeType_t buysell_,
                                                                         int old_int_price_, unsigned int old_size_,
                                                                         int new_int_price_, double new_price_,
                                                                         unsigned int new_size_) {
  //  dbglogger_ << "SparseIndexBookPxLevelManager::ModifyOrderAtDiffLevels " << security_id_ << ' ' <<
  //  (buysell_==HFSAT::kTradeTypeBuy?"BUY":"SELL")
  //	     << ' ' << old_size_ << " @ " << old_int_price_ << " modified to " << new_size_ << " @ " << new_int_price_
  //<< '\n';
  StorePreEventState(security_id_);
  MarketUpdateInfoLevelStruct* added_level_ = NULL;
  MarketUpdateInfoLevelStruct* deleted_level_ = NULL;
  MarketUpdateInfoLevelStruct* prev_bid_best_lvl_ = NULL;
  MarketUpdateInfoLevelStruct* prev_ask_best_lvl_ = NULL;
  BaseSparseIndexDataStructure* underlying_ds_ =
      (buysell_ == HFSAT::kTradeTypeBuy) ? underlying_bid_ds_ : underlying_ask_ds_;

  deleted_level_ = underlying_ds_->ChangeSizeAtPriceLevel(security_id_, old_int_price_, -old_size_, true);
  if(deleted_level_ == NULL) return k_nochange;

  // check to see if L2 callback condition is met
  if (deleted_level_->limit_int_price_level_ >= 0 &&
      deleted_level_->limit_int_price_level_ < NUM_LEVELS_FOR_L2_CALLBACK) {
    pass_check_for_l2_callback_ = true;
  }
  if (deleted_level_->limit_ordercount_ == 0) {
    CheckAndAssignPriceLevel(security_id_, buysell_, deleted_level_);
  }

  if (!using_predictive_uncross_[security_id_]) {
    if (init_book_cross_time_[security_id_] > 0) {
      if (is_crossing_side_bid_[security_id_]) {
        prev_bid_best_lvl_ = GetBidPL(security_id_, 1);
        prev_ask_best_lvl_ = GetAskPL(security_id_, 0);
      } else {
        prev_bid_best_lvl_ = GetBidPL(security_id_, 0);
        prev_ask_best_lvl_ = GetAskPL(security_id_, 1);
      }
    } else {
      prev_bid_best_lvl_ = GetBidPL(security_id_, 0);
      prev_ask_best_lvl_ = GetAskPL(security_id_, 0);
    }
  }

  added_level_ = underlying_ds_->AddOrderToPriceLevel(security_id_, new_int_price_, new_price_, new_size_);

  if (added_level_->limit_int_price_level_ == SP_INDEX_BOOK_INVALID_PX_LEVEL) {
    CheckAndAssignPriceLevel(security_id_, buysell_, added_level_);
  }

  // we need to check for L2 callback for added level as well
  if (!pass_check_for_l2_callback_ && added_level_->limit_int_price_level_ >= 0 &&
      added_level_->limit_int_price_level_ < NUM_LEVELS_FOR_L2_CALLBACK) {
    pass_check_for_l2_callback_ = true;
  }

  if (using_predictive_uncross_[security_id_]) {
    CheckAndUncrossBook(security_id_, buysell_);
  } else {
    CheckAndUncrossBookForHiddenOrders(security_id_, buysell_, prev_bid_best_lvl_, prev_ask_best_lvl_);
  }

  // Deallocate deleted level if applicable
  if (deleted_level_->limit_ordercount_ == 0) {
    one_pxlevel_mempool_.DeAlloc(deleted_level_);
  }

  //  CheckConsistency(security_id_);

  SetEventChangeState(security_id_);
  return change_status_;
}

LevelChangeType_t SparseIndexBookPxLevelManager::AddOrderToPxLevel(int security_id_, TradeType_t buysell_,
                                                                   int int_price_, double price_, unsigned int size_) {
  //  dbglogger_ << "SparseIndexBookPxLevelManager::AddOrderToPxLevel " << security_id_ << ' ' <<
  //  (buysell_==HFSAT::kTradeTypeBuy?"BUY":"SELL")
  //	     << ' ' << size_ << " @ " << price_ << '\n';
  StorePreEventState(security_id_);
  MarketUpdateInfoLevelStruct* added_level_ = NULL;
  MarketUpdateInfoLevelStruct* prev_bid_best_lvl_ = NULL;
  MarketUpdateInfoLevelStruct* prev_ask_best_lvl_ = NULL;

  if (!using_predictive_uncross_[security_id_]) {
    if (init_book_cross_time_[security_id_] > 0) {
      if (is_crossing_side_bid_[security_id_]) {
        prev_bid_best_lvl_ = GetBidPL(security_id_, 1);
        prev_ask_best_lvl_ = GetAskPL(security_id_, 0);
      } else {
        prev_bid_best_lvl_ = GetBidPL(security_id_, 0);
        prev_ask_best_lvl_ = GetAskPL(security_id_, 1);
      }
    } else {
      prev_bid_best_lvl_ = GetBidPL(security_id_, 0);
      prev_ask_best_lvl_ = GetAskPL(security_id_, 0);
    }
  }

  if (buysell_ == HFSAT::kTradeTypeBuy) {
    added_level_ = underlying_bid_ds_->AddOrderToPriceLevel(security_id_, int_price_, price_, size_);
  } else {
    added_level_ = underlying_ask_ds_->AddOrderToPriceLevel(security_id_, int_price_, price_, size_);
  }

  if (added_level_->limit_int_price_level_ == SP_INDEX_BOOK_INVALID_PX_LEVEL) {
    // Case corresponds to situation where new level is created o/w price_level would have
    // been assigned previously
    CheckAndAssignPriceLevel(security_id_, buysell_, added_level_);
  }

  // check to see if L2 callback condition is met
  if (added_level_->limit_int_price_level_ >= 0 && added_level_->limit_int_price_level_ < NUM_LEVELS_FOR_L2_CALLBACK) {
    pass_check_for_l2_callback_ = true;
  }

  if (using_predictive_uncross_[security_id_]) {
    CheckAndUncrossBook(security_id_, buysell_);
  } else {
    CheckAndUncrossBookForHiddenOrders(security_id_, buysell_, prev_bid_best_lvl_, prev_ask_best_lvl_);
  }

  //  CheckConsistency(security_id_);

  SetEventChangeState(security_id_);
  return change_status_;
}

// Uncrosses book by penalizing aggressive side
void SparseIndexBookPxLevelManager::CheckAndUncrossBookForHiddenOrders(
    int security_id_, TradeType_t last_ord_ttype_, MarketUpdateInfoLevelStruct* prev_bid_best_lvl_,
    MarketUpdateInfoLevelStruct* prev_ask_best_lvl_) {
  if (num_valid_bid_levels_[security_id_] > 0 && num_valid_ask_levels_[security_id_] > 0 &&
      sid_2_base_bid_levels_[security_id_][0]->limit_int_price_ >=
          sid_2_base_ask_levels_[security_id_][0]->limit_int_price_) {
    MarketUpdateInfoLevelStruct* best_crossed_bid_pl_ = GetBidPL(security_id_, 0);
    MarketUpdateInfoLevelStruct* best_crossed_ask_pl_ = GetAskPL(security_id_, 0);
    if (init_book_cross_time_[security_id_] == 0) {
      init_book_cross_time_[security_id_] = watch_.msecs_from_midnight();
      if (last_ord_ttype_ == HFSAT::kTradeTypeBuy) {
        is_crossing_side_bid_[security_id_] = true;
      } else {
        is_crossing_side_bid_[security_id_] = false;
      }
    }
    is_book_crossed_[security_id_] = true;

    // if this crossed instance started with agg buy - we retain ask side of crossed book and
    // retain bid side not conflicting with best ask
    if (is_crossing_side_bid_[security_id_]) {
      // Set uncrossed Ask L1
      sid_2_best_uncrossed_ask_level_[security_id_].limit_int_price_level_ =
          best_crossed_ask_pl_->limit_int_price_level_;
      sid_2_best_uncrossed_ask_level_[security_id_].limit_price_ = best_crossed_ask_pl_->limit_price_;
      sid_2_best_uncrossed_ask_level_[security_id_].limit_int_price_ = best_crossed_ask_pl_->limit_int_price_;
      sid_2_best_uncrossed_ask_level_[security_id_].limit_size_ = best_crossed_ask_pl_->limit_size_;
      sid_2_best_uncrossed_ask_level_[security_id_].limit_ordercount_ = best_crossed_ask_pl_->limit_ordercount_;

      MarketUpdateInfoLevelStruct* best_uncrossed_bid_pl_ = NULL;
      if ((prev_bid_best_lvl_ == NULL) ||
          (prev_bid_best_lvl_->limit_int_price_ < best_crossed_ask_pl_->limit_int_price_)) {
        best_uncrossed_bid_pl_ = prev_bid_best_lvl_;
      } else {
        // Set uncrossed Bid L1 - check to see if linear access is better latency wise
        best_uncrossed_bid_pl_ =
            underlying_bid_ds_->GetLowerPriceLevel(security_id_, best_crossed_ask_pl_->limit_int_price_);
      }
      if (best_uncrossed_bid_pl_ && best_uncrossed_bid_pl_->limit_size_ != kInvalidSize) {
        sid_2_best_uncrossed_bid_level_[security_id_].limit_int_price_level_ =
            best_uncrossed_bid_pl_->limit_int_price_level_;
        sid_2_best_uncrossed_bid_level_[security_id_].limit_price_ = best_uncrossed_bid_pl_->limit_price_;
        sid_2_best_uncrossed_bid_level_[security_id_].limit_int_price_ = best_uncrossed_bid_pl_->limit_int_price_;
        sid_2_best_uncrossed_bid_level_[security_id_].limit_size_ = best_uncrossed_bid_pl_->limit_size_;
        sid_2_best_uncrossed_bid_level_[security_id_].limit_ordercount_ = best_uncrossed_bid_pl_->limit_ordercount_;
      } else {
        sid_2_best_uncrossed_bid_level_[security_id_].limit_int_price_level_ = SP_INDEX_BOOK_INVALID_PX_LEVEL;
        sid_2_best_uncrossed_bid_level_[security_id_].limit_price_ = kInvalidPrice;
        sid_2_best_uncrossed_bid_level_[security_id_].limit_int_price_ = kInvalidIntPrice;
        sid_2_best_uncrossed_bid_level_[security_id_].limit_size_ = kInvalidSize;
        sid_2_best_uncrossed_bid_level_[security_id_].limit_ordercount_ = 0;
      }
    } else {
      // Set uncrossed Bid L1
      sid_2_best_uncrossed_bid_level_[security_id_].limit_int_price_level_ =
          best_crossed_bid_pl_->limit_int_price_level_;
      sid_2_best_uncrossed_bid_level_[security_id_].limit_price_ = best_crossed_bid_pl_->limit_price_;
      sid_2_best_uncrossed_bid_level_[security_id_].limit_int_price_ = best_crossed_bid_pl_->limit_int_price_;
      sid_2_best_uncrossed_bid_level_[security_id_].limit_size_ = best_crossed_bid_pl_->limit_size_;
      sid_2_best_uncrossed_bid_level_[security_id_].limit_ordercount_ = best_crossed_bid_pl_->limit_ordercount_;

      // Set uncrossed Ask L1 - check to see if linear access is better latency wise
      MarketUpdateInfoLevelStruct* best_uncrossed_ask_pl_ = NULL;
      if ((prev_ask_best_lvl_ == NULL) ||
          (prev_ask_best_lvl_->limit_int_price_ > best_crossed_bid_pl_->limit_int_price_)) {
        best_uncrossed_ask_pl_ = prev_ask_best_lvl_;
      } else {
        // Set uncrossed Bid L1 - check to see if linear access is better latency wise
        best_uncrossed_ask_pl_ =
            underlying_ask_ds_->GetHigherPriceLevel(security_id_, best_crossed_bid_pl_->limit_int_price_);
      }
      if (best_uncrossed_ask_pl_ && best_uncrossed_ask_pl_->limit_size_ != kInvalidSize) {
        sid_2_best_uncrossed_ask_level_[security_id_].limit_int_price_level_ =
            best_uncrossed_ask_pl_->limit_int_price_level_;
        sid_2_best_uncrossed_ask_level_[security_id_].limit_price_ = best_uncrossed_ask_pl_->limit_price_;
        sid_2_best_uncrossed_ask_level_[security_id_].limit_int_price_ = best_uncrossed_ask_pl_->limit_int_price_;
        sid_2_best_uncrossed_ask_level_[security_id_].limit_size_ = best_uncrossed_ask_pl_->limit_size_;
        sid_2_best_uncrossed_ask_level_[security_id_].limit_ordercount_ = best_uncrossed_ask_pl_->limit_ordercount_;
      } else {
        sid_2_best_uncrossed_ask_level_[security_id_].limit_int_price_level_ = SP_INDEX_BOOK_INVALID_PX_LEVEL;
        sid_2_best_uncrossed_ask_level_[security_id_].limit_price_ = kInvalidPrice;
        sid_2_best_uncrossed_ask_level_[security_id_].limit_int_price_ = kInvalidIntPrice;
        sid_2_best_uncrossed_ask_level_[security_id_].limit_size_ = kInvalidSize;
        sid_2_best_uncrossed_ask_level_[security_id_].limit_ordercount_ = 0;
      }
    }
  }  // end case crossed book
  else {
    // In case book was previously crossed, we reset vars
    if (init_book_cross_time_[security_id_] > 0) {
      init_book_cross_time_[security_id_] = 0;
      is_book_crossed_[security_id_] = false;
    }
  }
}

void SparseIndexBookPxLevelManager::SetUncrossSizesandPrices(int security_id_, MarketUpdateInfoLevelStruct*& bid_level_,
                                                             MarketUpdateInfoLevelStruct*& ask_level_,
                                                             int& bid_size_at_level_, int& ask_size_at_level_) {
  int t_bid_index_ = 0;
  int t_ask_index_ = 0;
  std::vector<MarketUpdateInfoLevelStruct*>& bid_book_ = sid_2_base_bid_levels_[security_id_];
  std::vector<MarketUpdateInfoLevelStruct*>& ask_book_ = sid_2_base_ask_levels_[security_id_];
  bid_level_ = bid_book_[t_bid_index_];
  ask_level_ = ask_book_[t_ask_index_];
  bid_size_at_level_ = bid_level_->limit_size_;
  ask_size_at_level_ = ask_level_->limit_size_;

  int num_bid_valid_lvl_ = underlying_bid_ds_->num_levels_vec_[security_id_];
  int num_ask_valid_lvl_ = underlying_ask_ds_->num_levels_vec_[security_id_];

  while (bid_level_ != NULL && ask_level_ != NULL && bid_level_->limit_int_price_ >= ask_level_->limit_int_price_) {
    if (bid_size_at_level_ == ask_size_at_level_) {
      t_bid_index_++;
      t_ask_index_++;

      if (t_bid_index_ >= num_valid_bid_levels_[security_id_]) {
        if (t_bid_index_ < num_bid_valid_lvl_) {
          bid_level_ = underlying_bid_ds_->GetLowerPriceLevel(security_id_, bid_level_->limit_int_price_);
        } else {
          bid_level_ = NULL;
        }
        if ((bid_level_ != NULL) && (t_bid_index_ < MAX_NUM_SORTED_BOOK_LEVELS)) {
          num_valid_bid_levels_[security_id_]++;
          bid_level_->limit_int_price_level_ = t_bid_index_;
          bid_book_[t_bid_index_] = bid_level_;
        }
      } else {
        bid_level_ = bid_book_[t_bid_index_];
      }

      if (t_ask_index_ >= num_valid_ask_levels_[security_id_]) {
        if (t_ask_index_ < num_ask_valid_lvl_) {
          ask_level_ = underlying_ask_ds_->GetHigherPriceLevel(security_id_, ask_level_->limit_int_price_);
        } else {
          ask_level_ = NULL;
        }
        if ((ask_level_ != NULL) && (t_ask_index_ < MAX_NUM_SORTED_BOOK_LEVELS)) {
          num_valid_ask_levels_[security_id_]++;
          ;
          ask_level_->limit_int_price_level_ = t_ask_index_;
          ask_book_[t_ask_index_] = ask_level_;
        }
      } else {
        ask_level_ = ask_book_[t_ask_index_];
      }

      bid_size_at_level_ = (bid_level_ == NULL) ? 0 : bid_level_->limit_size_;
      ask_size_at_level_ = (ask_level_ == NULL) ? 0 : ask_level_->limit_size_;
      if (bid_level_ == NULL || ask_level_ == NULL) {
        break;
      }
    } else if (bid_size_at_level_ < ask_size_at_level_) {
      ask_size_at_level_ -= bid_size_at_level_;
      t_bid_index_++;

      if (t_bid_index_ >= num_valid_bid_levels_[security_id_]) {
        if (t_bid_index_ < num_bid_valid_lvl_) {
          bid_level_ = underlying_bid_ds_->GetLowerPriceLevel(security_id_, bid_level_->limit_int_price_);
        } else {
          bid_level_ = NULL;
        }
        if ((bid_level_ != NULL) && (t_bid_index_ < MAX_NUM_SORTED_BOOK_LEVELS)) {
          num_valid_bid_levels_[security_id_]++;
          ;
          bid_level_->limit_int_price_level_ = t_bid_index_;
          bid_book_[t_bid_index_] = bid_level_;
        }
      } else {
        bid_level_ = bid_book_[t_bid_index_];
      }

      if (bid_level_ == NULL) {
        break;
      }
      bid_size_at_level_ = bid_level_->limit_size_;
    } else {
      bid_size_at_level_ -= ask_size_at_level_;
      t_ask_index_++;

      if (t_ask_index_ >= num_valid_ask_levels_[security_id_]) {
        if (t_ask_index_ < num_ask_valid_lvl_) {
          ask_level_ = underlying_ask_ds_->GetHigherPriceLevel(security_id_, ask_level_->limit_int_price_);
        } else {
          ask_level_ = NULL;
        }
        if ((ask_level_ != NULL) && (t_ask_index_ < MAX_NUM_SORTED_BOOK_LEVELS)) {
          num_valid_ask_levels_[security_id_]++;
          ;
          ask_level_->limit_int_price_level_ = t_ask_index_;
          ask_book_[t_ask_index_] = ask_level_;
        }
      } else {
        ask_level_ = ask_book_[t_ask_index_];
      }

      if (ask_level_ == NULL) {
        break;
      }
      ask_size_at_level_ = ask_level_->limit_size_;
    }
  }
}

int SparseIndexBookPxLevelManager::FindCrossedLevels(int security_id_, int agg_ord_sz_, int agg_ord_int_px_,
                                                     TradeType_t buysell_, int& size_remaining_,
                                                     bool& is_valid_book_end_, int& last_traded_int_price_) {
  int t_index_ = 0;
  is_valid_book_end_ = false;
  last_traded_int_price_ = -1;
  size_remaining_ = agg_ord_sz_;

  if (buysell_ == kTradeTypeSell) {
    std::vector<MarketUpdateInfoLevelStruct*>& t_book_ = sid_2_base_bid_levels_[security_id_];
    MarketUpdateInfoLevelStruct* bid_level_ = t_book_[t_index_];

    while (bid_level_ != NULL && bid_level_->limit_int_price_ >= agg_ord_int_px_) {
      int bid_size_at_level_ = bid_level_->limit_size_;
      if (bid_size_at_level_ >= size_remaining_) {
        if (bid_size_at_level_ == size_remaining_) {
          t_index_++;
        }
        size_remaining_ = 0;
        last_traded_int_price_ = bid_level_->limit_int_price_;
        return t_index_;
      } else {
        size_remaining_ -= bid_size_at_level_;
        t_index_++;
        last_traded_int_price_ = bid_level_->limit_int_price_;
        if (t_index_ >= num_valid_bid_levels_[security_id_]) {
          is_valid_book_end_ = true;
          return num_valid_bid_levels_[security_id_];
        } else {
          bid_level_ = t_book_[t_index_];
        }
      }
    }
  } else {
    std::vector<MarketUpdateInfoLevelStruct*>& t_book_ = sid_2_base_ask_levels_[security_id_];
    MarketUpdateInfoLevelStruct* ask_level_ = t_book_[t_index_];

    while (ask_level_ != NULL && ask_level_->limit_int_price_ <= agg_ord_int_px_) {
      int ask_size_at_level_ = ask_level_->limit_size_;
      if (ask_size_at_level_ >= size_remaining_) {
        if (ask_size_at_level_ == size_remaining_) {
          t_index_++;
        }
        size_remaining_ = 0;
        last_traded_int_price_ = ask_level_->limit_int_price_;
        return t_index_;
      } else {
        size_remaining_ -= ask_size_at_level_;
        t_index_++;
        last_traded_int_price_ = ask_level_->limit_int_price_;
        if (t_index_ >= num_valid_ask_levels_[security_id_]) {
          is_valid_book_end_ = true;
          return num_valid_ask_levels_[security_id_];
        } else {
          ask_level_ = t_book_[t_index_];
        }
      }
    }
  }
  // This means aggressive order has cleared some levels and made a new level in a hole
  return (t_index_);
}

// Uncross book if it gets crossed
void SparseIndexBookPxLevelManager::CheckAndUncrossBook(int security_id_, TradeType_t last_ord_ttype_) {
  if (num_valid_bid_levels_[security_id_] > 0 && num_valid_ask_levels_[security_id_] > 0 &&
      sid_2_base_bid_levels_[security_id_][0]->limit_int_price_ >=
          sid_2_base_ask_levels_[security_id_][0]->limit_int_price_) {
    if (init_book_cross_time_[security_id_] == 0) {
      init_book_cross_time_[security_id_] = watch_.msecs_from_midnight();
      if (last_ord_ttype_ == HFSAT::kTradeTypeBuy) {
        is_crossing_side_bid_[security_id_] = true;
      } else {
        is_crossing_side_bid_[security_id_] = false;
      }
    }
    is_book_crossed_[security_id_] = true;

    MarketUpdateInfoLevelStruct* t_bid_level_ = NULL;
    MarketUpdateInfoLevelStruct* t_ask_level_ = NULL;
    int t_bid_size_to_delete_ = 0;
    int t_ask_size_to_delete_ = 0;
    SetUncrossSizesandPrices(security_id_, t_bid_level_, t_ask_level_, t_bid_size_to_delete_, t_ask_size_to_delete_);

    if (t_bid_level_ != NULL) {
      sid_2_best_uncrossed_bid_level_[security_id_].limit_size_ = t_bid_size_to_delete_;
      sid_2_best_uncrossed_bid_level_[security_id_].limit_price_ = t_bid_level_->limit_price_;
      sid_2_best_uncrossed_bid_level_[security_id_].limit_int_price_ = t_bid_level_->limit_int_price_;
      // limit ordercount values are estimates - accurate value computation would require latency hits
      sid_2_best_uncrossed_bid_level_[security_id_].limit_ordercount_ =
          std::max(1, (int)(t_bid_size_to_delete_ * 1.0 / t_bid_level_->limit_size_ * t_bid_level_->limit_ordercount_));

      if (t_bid_level_->limit_int_price_level_ < num_valid_bid_levels_[security_id_]) {
        sid_2_best_uncrossed_bid_level_[security_id_].limit_int_price_level_ = t_bid_level_->limit_int_price_level_;
      } else {
        sid_2_best_uncrossed_bid_level_[security_id_].limit_int_price_level_ = DEEP_LEVEL_MARKER;
      }
    } else {
      sid_2_best_uncrossed_bid_level_[security_id_].limit_int_price_level_ = SP_INDEX_BOOK_INVALID_PX_LEVEL;
      sid_2_best_uncrossed_bid_level_[security_id_].limit_price_ = kInvalidPrice;
      sid_2_best_uncrossed_bid_level_[security_id_].limit_int_price_ = kInvalidIntPrice;
      sid_2_best_uncrossed_bid_level_[security_id_].limit_size_ = kInvalidSize;
      sid_2_best_uncrossed_bid_level_[security_id_].limit_ordercount_ = 0;
    }

    if (t_ask_level_ != NULL) {
      sid_2_best_uncrossed_ask_level_[security_id_].limit_size_ = t_ask_size_to_delete_;
      sid_2_best_uncrossed_ask_level_[security_id_].limit_price_ = t_ask_level_->limit_price_;
      sid_2_best_uncrossed_ask_level_[security_id_].limit_int_price_ = t_ask_level_->limit_int_price_;
      // limit ordercount values are estimates - accurate value computation would require latency hits
      sid_2_best_uncrossed_ask_level_[security_id_].limit_ordercount_ =
          std::max(1, (int)(t_ask_size_to_delete_ * 1.0 / t_ask_level_->limit_size_ * t_ask_level_->limit_ordercount_));

      if (t_ask_level_->limit_int_price_level_ < num_valid_ask_levels_[security_id_]) {
        sid_2_best_uncrossed_ask_level_[security_id_].limit_int_price_level_ = t_ask_level_->limit_int_price_level_;
      } else {
        sid_2_best_uncrossed_ask_level_[security_id_].limit_int_price_level_ = DEEP_LEVEL_MARKER;
      }
    } else {
      sid_2_best_uncrossed_ask_level_[security_id_].limit_int_price_level_ = SP_INDEX_BOOK_INVALID_PX_LEVEL;
      sid_2_best_uncrossed_ask_level_[security_id_].limit_price_ = kInvalidPrice;
      sid_2_best_uncrossed_ask_level_[security_id_].limit_int_price_ = kInvalidIntPrice;
      sid_2_best_uncrossed_ask_level_[security_id_].limit_size_ = kInvalidSize;
      sid_2_best_uncrossed_ask_level_[security_id_].limit_ordercount_ = 0;
    }

  }  // end case crossed book
  else {
    // In case book was previously crossed, we reset vars
    if (init_book_cross_time_[security_id_] > 0) {
      init_book_cross_time_[security_id_] = 0;
      is_book_crossed_[security_id_] = false;
    }
  }
}

// Sets consistent price levels for affected instrument's affected's side
void SparseIndexBookPxLevelManager::CheckAndAssignPriceLevel(int security_id_, TradeType_t buysell,
                                                             MarketUpdateInfoLevelStruct* affected_level_) {
  int t_num_book_levels_;
  int num_valid_lvl_;
  if (buysell == kTradeTypeBuy) {
    t_num_book_levels_ = num_valid_bid_levels_[security_id_];
    num_valid_lvl_ = underlying_bid_ds_->num_levels_vec_[security_id_];
  } else {
    t_num_book_levels_ = num_valid_ask_levels_[security_id_];
    num_valid_lvl_ = underlying_ask_ds_->num_levels_vec_[security_id_];
  }
  std::vector<MarketUpdateInfoLevelStruct*>& t_book_side_ =
      (buysell == kTradeTypeBuy ? sid_2_base_bid_levels_[security_id_] : sid_2_base_ask_levels_[security_id_]);

  // Case 1. A new level is created
  if (affected_level_->limit_int_price_level_ == SP_INDEX_BOOK_INVALID_PX_LEVEL) {
    // Case 1.1 New level is too far into side for us to track.
    if (t_num_book_levels_ >= MIN_NUM_SORTED_BOOK_LEVELS &&
        ((buysell == kTradeTypeBuy &&
          t_book_side_[t_num_book_levels_ - 1]->limit_int_price_ > affected_level_->limit_int_price_) ||
         (buysell == kTradeTypeSell &&
          t_book_side_[t_num_book_levels_ - 1]->limit_int_price_ < affected_level_->limit_int_price_)) &&
        (((t_num_book_levels_ + 1) != num_valid_lvl_) || (t_num_book_levels_ == MAX_NUM_SORTED_BOOK_LEVELS))) {
      affected_level_->limit_int_price_level_ = DEEP_LEVEL_MARKER;
    }
    // Case 1.2 New level is going to be inserted in book
    else {
      // If last level needs to be cleared
      if (t_num_book_levels_ == MAX_NUM_SORTED_BOOK_LEVELS) {
        t_book_side_[t_num_book_levels_ - 1]->limit_int_price_level_ = DEEP_LEVEL_MARKER;
      }
      // shift levels lower till appropriate point
      int t_curr_position_ = t_num_book_levels_;
      while (t_curr_position_ > 0 &&
             ((buysell == kTradeTypeBuy &&
               t_book_side_[t_curr_position_ - 1]->limit_int_price_ < affected_level_->limit_int_price_) ||
              (buysell == kTradeTypeSell &&
               t_book_side_[t_curr_position_ - 1]->limit_int_price_ > affected_level_->limit_int_price_))) {
        if (t_curr_position_ < MAX_NUM_SORTED_BOOK_LEVELS) {
          t_book_side_[t_curr_position_] = t_book_side_[t_curr_position_ - 1];
          t_book_side_[t_curr_position_]->limit_int_price_level_ = t_curr_position_;
        }
        t_curr_position_ -= 1;
      }
      affected_level_->limit_int_price_level_ = t_curr_position_;
      t_book_side_[t_curr_position_] = affected_level_;
      t_num_book_levels_ =
          (t_num_book_levels_ == MAX_NUM_SORTED_BOOK_LEVELS ? MAX_NUM_SORTED_BOOK_LEVELS : (t_num_book_levels_ + 1));
    }  // end Case 1.2
  }    // end Case 1

  // Case 2. A level is deleted
  else if (affected_level_->limit_ordercount_ == 0) {
    // Case 2.1 level was too deep and not in the top levels view
    if (affected_level_->limit_int_price_level_ == DEEP_LEVEL_MARKER) {
      return;
    }
    // Case 2.2 it has a valid level and needs to be deleted
    else {
      int t_current_index_ = affected_level_->limit_int_price_level_;
      // current_index 0..val num_book_levels_ val+1
      while (t_current_index_ < t_num_book_levels_ - 1) {
        t_book_side_[t_current_index_] = t_book_side_[t_current_index_ + 1];
        t_book_side_[t_current_index_]->limit_int_price_level_ = t_current_index_;
        t_current_index_ += 1;
      }

      affected_level_->limit_int_price_level_ = SP_INDEX_BOOK_INVALID_PX_LEVEL;
    }
    t_num_book_levels_ -= 1;
    if (t_num_book_levels_ < MIN_NUM_SORTED_BOOK_LEVELS) {
      // populate levels from underlying data structure
      // Get Bid/Ask PL can't be used here since num valid level only changes at the end of the function
      while ((t_num_book_levels_ < MAX_NUM_SORTED_BOOK_LEVELS) && (t_num_book_levels_ < num_valid_lvl_)) {
        MarketUpdateInfoLevelStruct* t_px_level_ = NULL;
        if (buysell == kTradeTypeBuy) {
          t_px_level_ = underlying_bid_ds_->GetLowerPriceLevel(
              security_id_,
              (t_num_book_levels_ > 0 ? t_book_side_[t_num_book_levels_ - 1]->limit_int_price_ : SP_MAX_PRICE));
        } else {
          t_px_level_ = underlying_ask_ds_->GetHigherPriceLevel(
              security_id_,
              (t_num_book_levels_ > 0 ? t_book_side_[t_num_book_levels_ - 1]->limit_int_price_ : SP_MIN_PRICE));
        }
        if (t_px_level_ != NULL) {
          t_book_side_[t_num_book_levels_] = t_px_level_;
          t_px_level_->limit_int_price_level_ = t_num_book_levels_;
          t_num_book_levels_ += 1;
        } else {
          // There aren't MAX_NUM_SORTED_BOOK_LEVELS on that book side
          break;
        }
      }  // end while
    }    // end repopulating vectorized book
  }      // end Case 2

  if (buysell == kTradeTypeBuy) {
    num_valid_bid_levels_[security_id_] = t_num_book_levels_;
  } else {
    num_valid_ask_levels_[security_id_] = t_num_book_levels_;
  }
}

void SparseIndexBookPxLevelManager::CheckConsistency(int security_id_) {
  double last_bid_px_ = 0;
  double last_ask_px_ = 0;
  // Check base book consistency
  for (int t_ctr_ = 0; t_ctr_ < num_valid_bid_levels_[security_id_]; t_ctr_++) {
    double t_curr_px_ = GetBidPrice(security_id_, t_ctr_);
    int t_curr_sz_ = GetBidSize(security_id_, t_ctr_);
    if (last_bid_px_ > 1e-5 && last_bid_px_ <= t_curr_px_ + 1e-5) {
      dbglogger_ << "Base_bid_px Consistency fails " << last_bid_px_ << ' ' << t_curr_px_ << '\n';
    }
    if (t_curr_sz_ <= 0) {
      dbglogger_ << "Base_bid_sz Consistency fails " << t_curr_sz_ << '\n';
    }
    last_bid_px_ = t_curr_px_;
  }
  for (int t_ctr_ = 0; t_ctr_ < num_valid_ask_levels_[security_id_]; t_ctr_++) {
    double t_curr_px_ = GetAskPrice(security_id_, t_ctr_);
    int t_curr_sz_ = GetAskSize(security_id_, t_ctr_);
    if (last_ask_px_ > 1e-5 && last_ask_px_ >= t_curr_px_ - 1e-5) {
      dbglogger_ << "Base_ask_px Consistency fails " << last_ask_px_ << ' ' << t_curr_px_ << '\n';
    }
    if (t_curr_sz_ <= 0) {
      dbglogger_ << "Base_ask_sz Consistency fails " << t_curr_sz_ << '\n';
    }
    last_ask_px_ = t_curr_px_;
  }

  // Check uncrossed book consistency
  last_bid_px_ = 0;
  last_ask_px_ = 0;
  for (int t_ctr_ = 0; t_ctr_ < MAX_NUM_SORTED_BOOK_LEVELS; t_ctr_++) {
    double t_curr_px_ = GetSynBidPrice(security_id_, t_ctr_);
    if (fabs(t_curr_px_ - kInvalidPrice) < 1e-5) {
      break;
    }
    int t_curr_sz_ = GetSynBidSize(security_id_, t_ctr_);
    if (last_bid_px_ > 1e-5 && last_bid_px_ <= t_curr_px_ + 1e-5) {
      dbglogger_ << "Syn_bid_px Consistency fails at level " << t_ctr_ << ' ' << last_bid_px_ << ' ' << t_curr_px_
                 << '\n';
    }
    if (t_curr_sz_ <= 0) {
      dbglogger_ << "Syn_bid_sz Consistency fails " << t_curr_sz_ << '\n';
    }
    last_bid_px_ = t_curr_px_;
  }
  for (int t_ctr_ = 0; t_ctr_ < MAX_NUM_SORTED_BOOK_LEVELS; t_ctr_++) {
    double t_curr_px_ = GetSynAskPrice(security_id_, t_ctr_);
    if (fabs(t_curr_px_ - kInvalidPrice) < 1e-5) {
      break;
    }
    int t_curr_sz_ = GetSynAskSize(security_id_, t_ctr_);
    if (last_ask_px_ > 1e-5 && last_ask_px_ >= t_curr_px_ - 1e-5) {
      dbglogger_ << "Syn_ask_px Consistency fails at level " << t_ctr_ << ' ' << last_ask_px_ << ' ' << t_curr_px_
                 << '\n';
    }
    if (t_curr_sz_ <= 0) {
      dbglogger_ << "Syn_ask_sz Consistency fails " << t_curr_sz_ << '\n';
    }
    last_ask_px_ = t_curr_px_;
  }

  // Check Uncrossed book consistency
  if (GetSynBidPrice(security_id_, 0) >= GetSynAskPrice(security_id_, 0) - 1e-5 &&
      GetSynAskSize(security_id_, 0) != kInvalidSize) {
    dbglogger_ << "Syn_Cross Consistency fails " << GetSynBidPrice(security_id_, 0) << " -- "
               << GetSynAskSize(security_id_, 0) << '\n';
  }

  // Check correct setting of is_crossed var
  if (is_book_crossed_[security_id_]) {
    if (fabs(GetBidPrice(security_id_, 0) - kInvalidPrice) < 1e-5 ||
        fabs(GetAskPrice(security_id_, 0) - kInvalidPrice) < 1e-5 || GetBidSize(security_id_, 0) == kInvalidSize ||
        GetAskSize(security_id_, 0) == kInvalidSize ||
        GetBidPrice(security_id_, 0) < GetAskPrice(security_id_, 0) - 1e-5) {
      dbglogger_ << "is_book_cross Consistency false positive " << GetBidSize(security_id_, 0) << " @ "
                 << GetBidPrice(security_id_, 0) << " --- " << GetAskPrice(security_id_, 0) << " @ "
                 << GetAskSize(security_id_, 0) << '\n';
    }
  } else {
    if (fabs(GetBidPrice(security_id_, 0) - kInvalidPrice) > 1e-5 &&
        fabs(GetAskPrice(security_id_, 0) - kInvalidPrice) > 1e-5 && GetBidSize(security_id_, 0) != kInvalidSize &&
        GetAskSize(security_id_, 0) != kInvalidSize &&
        GetBidPrice(security_id_, 0) > GetAskPrice(security_id_, 0) - 1e-5) {
      dbglogger_ << "is_book_cross Consistency false negative " << GetBidSize(security_id_, 0) << " @ "
                 << GetBidPrice(security_id_, 0) << " --- " << GetAskPrice(security_id_, 0) << " @ "
                 << GetAskSize(security_id_, 0) << '\n';
    }
  }
}

void SparseIndexBookPxLevelManager::StorePreEventState(int security_id_) {
  preevent_bestbid_price_ = GetSynBidPrice(security_id_, 0);
  preevent_bestbid_size_ = GetSynBidSize(security_id_, 0);
  preevent_bestask_price_ = GetSynAskPrice(security_id_, 0);
  preevent_bestask_size_ = GetSynAskSize(security_id_, 0);
  pass_check_for_l2_callback_ = false;
}

void SparseIndexBookPxLevelManager::SetEventChangeState(int security_id_) {
  // l1_price case
  if (fabs(preevent_bestbid_price_ - GetSynBidPrice(security_id_, 0)) > 1e-5 ||
      fabs(preevent_bestask_price_ - GetSynAskPrice(security_id_, 0)) > 1e-5) {
    change_status_ = k_l1price;
  } else if (preevent_bestbid_size_ != GetSynBidSize(security_id_, 0) ||
             preevent_bestask_size_ != GetSynAskSize(security_id_, 0)) {
    change_status_ = k_l1size;
  } else if (pass_check_for_l2_callback_ || IsBookCrossed(security_id_)) {
    change_status_ = k_l2change;
  } else {
    change_status_ = k_nochange;
  }
}

std::string SparseIndexBookPxLevelManager::GetStatusStringForm(LevelChangeType_t ctype_) {
  switch (ctype_) {
    case k_l1price:
      return std::string("L1Price");
      break;
    case k_l1size:
      return std::string("L1Size");
      break;
    case k_l2change:
      return std::string("L2Change");
      break;
    default:
      return std::string("NoChange");
  }
}

void SparseIndexBookPxLevelManager::DumpBook(int sec_id_, int t_level_) {
  int t_levels_ = std::max(num_valid_bid_levels_[sec_id_], num_valid_ask_levels_[sec_id_]);
  t_levels_ = std::min(t_levels_, t_level_);
  dbglogger_ << "----------------- Book dump -- " << num_valid_bid_levels_[sec_id_] << ' '
             << num_valid_ask_levels_[sec_id_] << '\n';
  for (int t_counter_ = 0; t_counter_ < t_levels_; t_counter_++) {
    MarketUpdateInfoLevelStruct* t_level_ = GetSynBidPL(sec_id_, t_counter_);
    if (t_level_) {
      dbglogger_ << " [ " << t_level_->limit_ordercount_ << " ] " << t_level_->limit_size_ << " @ "
                 << t_level_->limit_price_;
    } else {
      dbglogger_ << " NA  @   NA ";
    }
    dbglogger_ << " --- ";
    t_level_ = GetSynAskPL(sec_id_, t_counter_);
    if (t_level_) {
      dbglogger_ << t_level_->limit_price_ << " @ " << t_level_->limit_size_ << " [ " << t_level_->limit_ordercount_
                 << " ] ";
    } else {
      dbglogger_ << " NA  @   NA ";
    }
    dbglogger_ << '\n';
  }
  dbglogger_ << "--------------------------------------\n\n";
}

MarketUpdateInfoLevelStruct* SparseIndexBookPxLevelManager::GetDeepPL(int security_id_, TradeType_t buysell,
                                                                      int int_price, int depth) {
  MarketUpdateInfoLevelStruct* tgt_pl_ = NULL;
  int t_counter_ = depth;
  int t_int_price_ = int_price;
  if (buysell == HFSAT::kTradeTypeBuy) {
    do {
      tgt_pl_ = underlying_bid_ds_->GetLowerPriceLevel(security_id_, t_int_price_);
      t_counter_ -= 1;
      if (tgt_pl_ != NULL) {
        t_int_price_ = tgt_pl_->limit_int_price_;
      }
    } while (tgt_pl_ != NULL && t_counter_ > 0);
  } else {
    do {
      tgt_pl_ = underlying_ask_ds_->GetHigherPriceLevel(security_id_, t_int_price_);
      t_counter_ -= 1;
      if (tgt_pl_ != NULL) {
        t_int_price_ = tgt_pl_->limit_int_price_;
      }
    } while (tgt_pl_ != NULL && t_counter_ > 0);
  }
  return tgt_pl_;
}

double SparseIndexBookPxLevelManager::GetBidPrice(int security_id_, int level_) {
  MarketUpdateInfoLevelStruct* tgt_pl_ = GetBidPL(security_id_, level_);
  if (tgt_pl_ != NULL) {
    return tgt_pl_->limit_price_;
  }
  return kInvalidPrice;
}

double SparseIndexBookPxLevelManager::GetAskPrice(int security_id_, int level_) {
  MarketUpdateInfoLevelStruct* tgt_pl_ = GetAskPL(security_id_, level_);
  if (tgt_pl_ != NULL) {
    return tgt_pl_->limit_price_;
  }
  return kInvalidPrice;
}

int SparseIndexBookPxLevelManager::GetBidSize(int security_id_, int level_) {
  MarketUpdateInfoLevelStruct* tgt_pl_ = GetBidPL(security_id_, level_);
  if (tgt_pl_ != NULL) {
    return tgt_pl_->limit_size_;
  }
  return kInvalidSize;
}

int SparseIndexBookPxLevelManager::GetAskSize(int security_id_, int level_) {
  MarketUpdateInfoLevelStruct* tgt_pl_ = GetAskPL(security_id_, level_);
  if (tgt_pl_ != NULL) {
    return tgt_pl_->limit_size_;
  }
  return kInvalidSize;
}

double SparseIndexBookPxLevelManager::GetSynBidPrice(int security_id_, int level_) {
  MarketUpdateInfoLevelStruct* tgt_pl_ = GetSynBidPL(security_id_, level_);
  if (tgt_pl_ != NULL) {
    return tgt_pl_->limit_price_;
  }
  return kInvalidPrice;
}

double SparseIndexBookPxLevelManager::GetSynAskPrice(int security_id_, int level_) {
  MarketUpdateInfoLevelStruct* tgt_pl_ = GetSynAskPL(security_id_, level_);
  if (tgt_pl_ != NULL) {
    return tgt_pl_->limit_price_;
  }
  return kInvalidPrice;
}

int SparseIndexBookPxLevelManager::GetSynBidSize(int security_id_, int level_) {
  MarketUpdateInfoLevelStruct* tgt_pl_ = GetSynBidPL(security_id_, level_);
  if (tgt_pl_ != NULL) {
    return tgt_pl_->limit_size_;
  }
  return kInvalidSize;
}

int SparseIndexBookPxLevelManager::GetSynAskSize(int security_id_, int level_) {
  MarketUpdateInfoLevelStruct* tgt_pl_ = GetSynAskPL(security_id_, level_);
  if (tgt_pl_ != NULL) {
    return tgt_pl_->limit_size_;
  }
  return kInvalidSize;
}

int SparseIndexBookPxLevelManager::GetSynBidIntPrice(int security_id_, int level_) {
  MarketUpdateInfoLevelStruct* tgt_pl_ = GetSynBidPL(security_id_, level_);
  if (tgt_pl_ != NULL) {
    return tgt_pl_->limit_int_price_;
  }
  return kInvalidIntPrice;
}

int SparseIndexBookPxLevelManager::GetSynAskIntPrice(int security_id_, int level_) {
  MarketUpdateInfoLevelStruct* tgt_pl_ = GetSynAskPL(security_id_, level_);
  if (tgt_pl_ != NULL) {
    return tgt_pl_->limit_int_price_;
  }
  return kInvalidIntPrice;
}

MarketUpdateInfoLevelStruct* SparseIndexBookPxLevelManager::GetPLAtIntPrice(int security_id_, TradeType_t buysell_,
                                                                            int limit_int_price_) {
  if (buysell_ == HFSAT::kTradeTypeBuy) {
    return underlying_bid_ds_->GetPLAtIntPrice(security_id_, limit_int_price_);
  } else {
    return underlying_ask_ds_->GetPLAtIntPrice(security_id_, limit_int_price_);
  }
}
}
