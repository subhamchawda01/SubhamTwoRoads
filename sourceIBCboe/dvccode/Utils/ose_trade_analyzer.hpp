// =====================================================================================
//
//       Filename:  ose_trade_analyzer.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  04/10/2014 08:13:01 AM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
//
//        Address:  Suite No 162, Evoma, #14, Bhattarhalli,
//                  Old Madras Road, Near Garden City College,
//                  KR Puram, Bangalore 560049, India
//          Phone:  +91 80 4190 3551
//
// =====================================================================================

#pragma once

#include <set>
#include <vector>
#include <iostream>
#include <string>
#include <inttypes.h>

#define HISTORY_MAP_INDEX_MASK \
  0x000000000000FFFF  // additional level of hasing, keeps map flippin upto minimal, can be tuned for optimization
#define TOTAL_HISTORY_MAP_INDEX_SIZE 65536

namespace HFSAT {
namespace Utils {

class OSETradeAnalyzer {
 private:
  OSE_MDS::OSEPriceFeedCommonStruct* ose_trade_message_;
  bool last_packet_analysis_active_;

  typedef std::set<uint64_t> BidOrdersHistory;
  typedef std::set<uint64_t> AskOrdersHistory;

  std::vector<BidOrdersHistory*> bid_history_;
  std::vector<AskOrdersHistory*> ask_history_;

  std::vector<OSE_MDS::OSEPriceFeedCommonStruct*> trade_order_packets_;

  OSETradeAnalyzer(const OSETradeAnalyzer& _disabled_copy_constructor_);

 public:
  OSETradeAnalyzer()
      : ose_trade_message_(NULL),
        last_packet_analysis_active_(false),
        bid_history_(),
        ask_history_(),
        trade_order_packets_()

  {
    ose_trade_message_ = new OSE_MDS::OSEPriceFeedCommonStruct();

    for (unsigned int allocation_counter = 0; allocation_counter < TOTAL_HISTORY_MAP_INDEX_SIZE; allocation_counter++) {
      bid_history_.push_back(new BidOrdersHistory());
      ask_history_.push_back(new AskOrdersHistory());
    }
  }

  void StoreDeltaPacketSeq(OSE_MDS::OSEPriceFeedCommonStruct* _logged_source_delta_packet_) {
    int map_index_ = (_logged_source_delta_packet_->exch_order_seq_) & HISTORY_MAP_INDEX_MASK;

    if (0 == _logged_source_delta_packet_->type_) {
      bid_history_[map_index_]->insert(_logged_source_delta_packet_->exch_order_seq_);

    } else if (1 == _logged_source_delta_packet_->type_) {
      ask_history_[map_index_]->insert(_logged_source_delta_packet_->exch_order_seq_);
    }
  }

  void StoreAndAnalyzeTradePacket(OSE_MDS::OSEPriceFeedCommonStruct& _logged_source_trade_packet_) {
    for (unsigned int trade_order_counter = 0; trade_order_counter < trade_order_packets_.size();
         trade_order_counter++) {
      if (trade_order_packets_[trade_order_counter] != NULL) {
        delete trade_order_packets_[trade_order_counter];
        trade_order_packets_[trade_order_counter] = NULL;
      }
    }

    // Clear TradeOrder History
    trade_order_packets_.clear();

    // Update the packet we have
    memcpy((void*)(ose_trade_message_), (void*)(&_logged_source_trade_packet_),
           sizeof(OSE_MDS::OSEPriceFeedCommonStruct));

    // Trigger Packet Active
    last_packet_analysis_active_ = true;
  }

  // TODO Optimization possible here as after every push check we can decide at this point or not, simply call flush
  // function but make sure bid/ask counters are > 0
  //      void OnTradeOrderInput ( HFSAT::TradeType_t & _agg_side_, OSE_MDS::OSEPriceFeedCommonStruct *
  //      _trade_order_packet_ ) {
  void OnTradeOrderInput(OSE_MDS::OSEPriceFeedCommonStruct* _trade_order_packet_) {
    OSE_MDS::OSEPriceFeedCommonStruct* this_new_trade_order = new OSE_MDS::OSEPriceFeedCommonStruct();
    memcpy((void*)(this_new_trade_order), (void*)(_trade_order_packet_), sizeof(OSE_MDS::OSEPriceFeedCommonStruct));
    trade_order_packets_.push_back(this_new_trade_order);
  }

  bool FlushAnyPendingTradeBeforeDelta(OSE_MDS::OSEPriceFeedCommonStruct& _any_history_packet_left_,
                                       HFSAT::TradeType_t& _agg_side_) {
    if (last_packet_analysis_active_) {
      memcpy((void*)(&_any_history_packet_left_), (void*)(ose_trade_message_),
             sizeof(OSE_MDS::OSEPriceFeedCommonStruct));

      // Default
      _agg_side_ = HFSAT::kTradeTypeNoInfo;

      // Logic 1 - There will be 1 Incoming Order which will aggressively match with the no of orders in our existing
      // book
      for (unsigned int trade_order_counter = 0; trade_order_counter < trade_order_packets_.size();
           trade_order_counter++) {
        // Logic 2 - Mismatch of size and the opposite side is the agg side
        if ((trade_order_packets_[trade_order_counter]->size) != (ose_trade_message_->size)) {
          _agg_side_ =
              (trade_order_packets_[trade_order_counter]->type_) == 0 ? HFSAT::kTradeTypeSell : HFSAT::kTradeTypeBuy;
          last_packet_analysis_active_ = false;
          return true;
        }
      }

      // Logic 3 - If Any order is found in the history means opposite of that side is the agg side
      for (unsigned int trade_order_counter = 0; trade_order_counter < trade_order_packets_.size();
           trade_order_counter++) {
        int map_index = trade_order_packets_[trade_order_counter]->exch_order_seq_ & HISTORY_MAP_INDEX_MASK;

        if ((bid_history_[map_index]->find(trade_order_packets_[trade_order_counter]->exch_order_seq_) !=
             bid_history_[map_index]->end()) ||
            (ask_history_[map_index]->find(trade_order_packets_[trade_order_counter]->exch_order_seq_) !=
             ask_history_[map_index]->end())) {
          _agg_side_ =
              trade_order_packets_[trade_order_counter]->type_ == 0 ? HFSAT::kTradeTypeSell : HFSAT::kTradeTypeBuy;

          last_packet_analysis_active_ = false;
          return true;
        }
      }

      // TODO Logic 4 - If We always receive confirmation even for the aggressive orders and if thats received earlier
      // than trade, store & check the timestamp of the earliest order participated in the trade, opposite side is the
      // aggside, store timestamps for that

      last_packet_analysis_active_ = false;
      return true;
    }

    last_packet_analysis_active_ = false;
    return false;
  }
};
}
}
