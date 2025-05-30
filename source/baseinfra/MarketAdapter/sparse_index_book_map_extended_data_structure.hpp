/**
   \file MarketAdapter/sparse_index_book_map_extended_data_structure.hpp

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

class SparseIndexBookMapExtendedDataStructure : public BaseSparseIndexDataStructure {
 private:
  std::vector<std::map<int, MarketUpdateInfoLevelStruct*>> sid_2_underlying_map_;

 public:
  SparseIndexBookMapExtendedDataStructure(DebugLogger& dbglogger_, const Watch& watch, int num_sec,
                                          LockFreeSimpleMempool<MarketUpdateInfoLevelStruct>& p_mempool);

  MarketUpdateInfoLevelStruct* AddOrderToPriceLevel(int security_id_, int int_price_, double price_, unsigned int size);
  MarketUpdateInfoLevelStruct* ChangeSizeAtPriceLevel(int security_id_, int int_price_, int size,
                                                      bool decrement_ordercount);

  MarketUpdateInfoLevelStruct* GetHigherPriceLevel(int security_id_, int int_price_);
  MarketUpdateInfoLevelStruct* GetLowerPriceLevel(int security_id_, int int_price_);
  MarketUpdateInfoLevelStruct* GetPLAtIntPrice(int security_id_, int limit_int_price_);
};
}
