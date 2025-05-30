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

struct EobiOrderLevel {
  int size_;
  int order_count_;

  EobiOrderLevel(int t_size_, int t_order_count_) {
    size_ = t_size_;
    order_count_ = t_order_count_;
  }
};

#define EOBID_ORDER_BOOK_SIZE 4095
#define EOBID_START_INDEX 2047

class EobiOrderBook {
 private:
  std::vector<EobiOrderLevel> bid_levels_;
  std::vector<EobiOrderLevel> ask_levels_;

  int last_msg_seq_num_;
  int last_security_id_;

  EUREX_MDS::EUREXLSCommonStruct* eurex_mds_;

  int price_adjustment_;

  uint8_t contract_code_;

  HFSAT::FastPriceConvertor* fast_price_convertor_;

  bool price_adjustment_set_;

  int ask_side_trade_size_;
  int bid_side_trade_size_;

  int top_bid_index_;
  int top_ask_index_;

  int eobid_order_book_size_;
  int eobid_start_index_;

 public:
  EobiOrderBook(std::string& t_shortcode_, uint8_t t_security_id_);

  EUREX_MDS::EUREXLSCommonStruct* OrderAdd(uint8_t t_side_, double t_price_, int t_size_, bool t_intermediate_);
  EUREX_MDS::EUREXLSCommonStruct* OrderDelete(uint8_t t_side_, double t_price_, int t_size_, bool t_delete_order_,
                                              bool t_intermediate_);
  EUREX_MDS::EUREXLSCommonStruct* OrderMassDelete();
  EUREX_MDS::EUREXLSCommonStruct* FullOrderExecution(uint8_t t_side_, double t_price_, int t_size_);
  EUREX_MDS::EUREXLSCommonStruct* PartialOrderExecution(uint8_t t_side_, double t_price_, int t_size_);
  EUREX_MDS::EUREXLSCommonStruct* ExecutionSummary(uint8_t t_side_, double t_price_, int t_size_);
  void RecenterBook();
};
}
