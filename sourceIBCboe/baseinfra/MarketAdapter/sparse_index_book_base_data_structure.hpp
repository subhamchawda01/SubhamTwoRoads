/**
   \file MarketAdapter/sparse_index_book_base_data_structure.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2017
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#pragma once

#include <vector>

#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CommonTradeUtils/watch.hpp"
#include "dvccode/CommonDataStructures/lockfree_simple_mempool.hpp"
#include "baseinfra/MarketAdapter/basic_market_view_structs.hpp"
#define SP_INDEX_BOOK_INVALID_PX_LEVEL -1

namespace HFSAT {

class BaseSparseIndexDataStructure {
 protected:
  DebugLogger& dbglogger_;
  const Watch& watch_;
  int num_securities_;

  LockFreeSimpleMempool<MarketUpdateInfoLevelStruct>& one_pxlevel_mempool_;

  // Returns true if top bid/ask index is changed

 public:
  std::vector<int> num_levels_vec_;
  BaseSparseIndexDataStructure(DebugLogger& dbglogger, const Watch& watch, int num_sec_,
                               LockFreeSimpleMempool<MarketUpdateInfoLevelStruct>& p_mempool)
      : dbglogger_(dbglogger),
        watch_(watch),
        num_securities_(num_sec_),
        one_pxlevel_mempool_(p_mempool),
        num_levels_vec_(num_sec_) {}

  virtual ~BaseSparseIndexDataStructure(){};

  // Returns pointer to price level where order is added or deleted

  // AddOrderToPriceLevel creates a new price level if level did not exist before o/w it
  // updates PL parameters.
  virtual MarketUpdateInfoLevelStruct* AddOrderToPriceLevel(int security_id_, int int_price_, double price_,
                                                            unsigned int size) = 0;

  virtual MarketUpdateInfoLevelStruct* ChangeSizeAtPriceLevel(int security_id_, int int_price_, int size,
                                                              bool decrement_ordercount) = 0;

  // Returns pointer to first price level at a lower/higher price
  // Return value is NULL if no such price level exists
  virtual MarketUpdateInfoLevelStruct* GetHigherPriceLevel(int security_id_, int int_price_) = 0;
  virtual MarketUpdateInfoLevelStruct* GetLowerPriceLevel(int security_id_, int int_price_) = 0;
  virtual MarketUpdateInfoLevelStruct* GetPLAtIntPrice(int security_id_, int limit_int_price_) = 0;
};
}
