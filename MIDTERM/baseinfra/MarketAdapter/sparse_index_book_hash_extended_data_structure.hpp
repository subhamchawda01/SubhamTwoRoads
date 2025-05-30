/**
   \file MarketAdapter/sparse_index_book_hash_extended_data_structure.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2017
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#pragma once

#include "baseinfra/MarketAdapter/sparse_index_book_base_data_structure.hpp"

namespace HFSAT {

struct LevelNode {
  MarketUpdateInfoLevelStruct* px_level;
  LevelNode* next_level;
  LevelNode() {
    px_level = nullptr;
    next_level = nullptr;
  }
};

class SparseIndexBookHashExtendedDataStructure : public BaseSparseIndexDataStructure {
 private:
  SimpleMempool<LevelNode> level_node_mempool_;
  std::vector<std::vector<LevelNode>> sid_to_price_hashed_map_;
  int book_sz_;
  HFSAT::TradeType_t side_;
  int block_sz_;
  std::vector<std::vector<uint32_t>> sid_to_block_marker_;

  int GetPriceHashIndex(int price);

 public:
  SparseIndexBookHashExtendedDataStructure(DebugLogger& dbglogger_, const Watch& watch, int num_sec,
                                           SimpleMempool<MarketUpdateInfoLevelStruct>& p_mempool, TradeType_t side);

  MarketUpdateInfoLevelStruct* AddOrderToPriceLevel(int security_id_, int int_price_, double price_, unsigned int size);
  MarketUpdateInfoLevelStruct* ChangeSizeAtPriceLevel(int security_id_, int int_price_, int size,
                                                      bool decrement_ordercount);

  MarketUpdateInfoLevelStruct* GetHigherPriceLevel(int security_id_, int int_price_);
  MarketUpdateInfoLevelStruct* GetLowerPriceLevel(int security_id_, int int_price_);
  MarketUpdateInfoLevelStruct* GetPLAtIntPrice(int security_id_, int int_price_);
};
}
