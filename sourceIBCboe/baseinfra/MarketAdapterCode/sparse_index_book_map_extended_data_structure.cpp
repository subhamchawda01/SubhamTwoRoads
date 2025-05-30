/**
   \file MarketAdapterCode/sparse_index_book_map_extended_data_structure.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2017
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#include "baseinfra/MarketAdapter/sparse_index_book_map_extended_data_structure.hpp"

namespace HFSAT {
SparseIndexBookMapExtendedDataStructure::SparseIndexBookMapExtendedDataStructure(
    DebugLogger& dbglogger, const Watch& watch, int num_sec, LockFreeSimpleMempool<MarketUpdateInfoLevelStruct>& p_mempool)
    : BaseSparseIndexDataStructure(dbglogger, watch, num_sec, p_mempool) {
  sid_2_underlying_map_.resize(num_sec, std::map<int, MarketUpdateInfoLevelStruct*>());
}

MarketUpdateInfoLevelStruct* SparseIndexBookMapExtendedDataStructure::AddOrderToPriceLevel(int security_id_,
                                                                                           int int_price_,
                                                                                           double price_,
                                                                                           unsigned int size) {
  MarketUpdateInfoLevelStruct* px_level_ = NULL;
  auto it = sid_2_underlying_map_[security_id_].find(int_price_);
  if (it != sid_2_underlying_map_[security_id_].end()) {
    px_level_ = it->second;
    px_level_->limit_size_ += size;
    px_level_->limit_ordercount_ += 1;
    px_level_->mod_time_ = watch_.tv();
  } else {
    px_level_ = one_pxlevel_mempool_.Alloc();
    px_level_->limit_ordercount_ = 1;
    px_level_->limit_size_ = size;
    px_level_->limit_int_price_level_ = SP_INDEX_BOOK_INVALID_PX_LEVEL;
    px_level_->limit_int_price_ = int_price_;
    px_level_->limit_price_ = price_;
    px_level_->mod_time_ = watch_.tv();
    num_levels_vec_[security_id_]++;
    sid_2_underlying_map_[security_id_].insert(std::pair<int, MarketUpdateInfoLevelStruct*>(int_price_, px_level_));
  }
  return px_level_;
}

MarketUpdateInfoLevelStruct* SparseIndexBookMapExtendedDataStructure::ChangeSizeAtPriceLevel(
    int security_id_, int int_price_, int size, bool decrement_ordercount) {
  // It is a logical error if this function is called for a non-existent level
  auto it = sid_2_underlying_map_[security_id_].find(int_price_);
  if (it != sid_2_underlying_map_[security_id_].end()) {
    MarketUpdateInfoLevelStruct* px_level_ = it->second;
    if (size > 0) {
      px_level_->mod_time_ = watch_.tv();
    }
    px_level_->limit_size_ += size;
    if (decrement_ordercount) {
      px_level_->limit_ordercount_ -= 1;
    }
    if (px_level_->limit_ordercount_ == 0) {
      // debug statements - remove later
      if (px_level_->limit_size_ != 0) {
        DBGLOG_TIME_CLASS_FUNC_LINE << " Inconsistent state empty pxlevel has nonempty size " << px_level_->limit_size_
                                    << DBGLOG_ENDL_FLUSH;
      }
      num_levels_vec_[security_id_]--;
      sid_2_underlying_map_[security_id_].erase(it);
    }
    return it->second;
  } else {
    DBGLOG_TIME_CLASS_FUNC_LINE << " Invalid Usage: ChangeSizeAtPriceLevel called for non-existent level @ "
                                << int_price_ << DBGLOG_ENDL_FLUSH;
    return NULL;  // exception state
  }
}

MarketUpdateInfoLevelStruct* SparseIndexBookMapExtendedDataStructure::GetHigherPriceLevel(int security_id_,
                                                                                          int int_price_) {
  auto it = sid_2_underlying_map_[security_id_].upper_bound(int_price_);
  if (it != sid_2_underlying_map_[security_id_].end()) {
    return it->second;
  } else {
    return NULL;
  }
}

MarketUpdateInfoLevelStruct* SparseIndexBookMapExtendedDataStructure::GetLowerPriceLevel(int security_id_,
                                                                                         int int_price_) {
  auto it = sid_2_underlying_map_[security_id_].lower_bound(int_price_);
  if (it != sid_2_underlying_map_[security_id_].begin() && it != sid_2_underlying_map_[security_id_].end() &&
      it->first == int_price_) {
    it--;
  }

  if (it != sid_2_underlying_map_[security_id_].end() && it->first < int_price_) {
    return it->second;
  } else {
    return NULL;
  }
}

MarketUpdateInfoLevelStruct* SparseIndexBookMapExtendedDataStructure::GetPLAtIntPrice(int security_id_,
                                                                                      int int_price_) {
  auto it = sid_2_underlying_map_[security_id_].find(int_price_);
  if (it != sid_2_underlying_map_[security_id_].end()) {
    return it->second;
  } else {
    return NULL;
  }
}
}
