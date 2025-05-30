/**
   \file EobiD/indexed_eobi_order_book.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066, India
   Phone: +91 80 4060 0717
 */

#pragma once

#include <string>
#include <vector>

#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CommonDataStructures/fast_price_convertor.hpp"

namespace HFSAT {

struct EobiOrderLevelForNPF {
  int size_;
  int order_count_;

  EobiOrderLevelForNPF(int t_size_, int t_order_count_) {
    size_ = t_size_;
    order_count_ = t_order_count_;
  }
};

#define EOBID_ORDER_BOOK_SIZE 4095
#define EOBID_START_INDEX 2047

class EobiOrderBookForNPF {
 private:
  std::vector<EobiOrderLevelForNPF> bid_levels_;
  std::vector<EobiOrderLevelForNPF> ask_levels_;

  int last_msg_seq_num_;
  int last_security_id_;

  EUREX_MDS::EUREXCommonStruct* eurex_mds_;

  int price_adjustment_;

  char exchange_symbol_[EUREX_MDS_CONTRACT_TEXT_SIZE];

  HFSAT::FastPriceConvertor* fast_price_convertor_;

  bool price_adjustment_set_;

  int ask_side_trade_size_;
  int bid_side_trade_size_;

  int top_bid_index_;
  int top_ask_index_;

 public:
  EobiOrderBookForNPF(std::string& t_shortcode_, std::string _exchange_symbol_, double min_price_inc = -1.0);

  EUREX_MDS::EUREXCommonStruct* OrderAdd(uint8_t t_side_, double t_price_, int t_size_, bool t_intermediate_);
  EUREX_MDS::EUREXCommonStruct* OrderDelete(uint8_t t_side_, double t_price_, int t_size_, bool t_delete_order_,
                                            bool t_intermediate_);
  EUREX_MDS::EUREXCommonStruct* OrderMassDelete();
  EUREX_MDS::EUREXCommonStruct* FullOrderExecution(uint8_t t_side_, double t_price_, int t_size_);
  EUREX_MDS::EUREXCommonStruct* PartialOrderExecution(uint8_t t_side_, double t_price_, int t_size_);
  EUREX_MDS::EUREXCommonStruct* ExecutionSummary(uint8_t t_side_, double t_price_, int t_size_);
};
}
