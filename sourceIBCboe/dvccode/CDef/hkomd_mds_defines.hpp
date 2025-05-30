// =====================================================================================
//
//       Filename:  hkomd_mds_defines.hpp
//
//    Description:  order level mds for hkomd
//
//        Version:  1.0
//        Created:  Friday 24 January 2014 12:13:20  GMT
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

#include <string>
#include <sstream>
#include <inttypes.h>
#include <sys/time.h>

namespace HKOMD_MDS {
#define HKOMD_MDS_CONTRACT_TEXT_SIZE 32
#define HKOMD_PF_MDS_CONTRACT_TEXT_SIZE 16

enum msgType { HKOMD_ORDER = 1, HKOMD_TRADE = 2 };

struct HKOMDOrder {
  uint32_t msg_seq_num_;
  char contract_[HKOMD_MDS_CONTRACT_TEXT_SIZE];
  uint8_t side_;
  uint32_t action_;
  double price_;
  uint32_t quantity_;
  uint64_t order_id_;
  uint32_t level_;
  char intermediate_;

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "Seq. No: " << msg_seq_num_ << "\n";
    t_temp_oss_ << "Contract: " << contract_ << "\n";
    t_temp_oss_ << "Side: " << int(side_) << "\n";
    switch (action_) {
      case '0':
        t_temp_oss_ << "Action: "
                    << "ADD"
                    << "\n";
        break;
      case '1':
        t_temp_oss_ << "Action: "
                    << "MODIFY"
                    << "\n";
        break;
      case '2':
        t_temp_oss_ << "Action: "
                    << "Delete"
                    << "\n";
        break;
      case '3':
        t_temp_oss_ << "Action: "
                    << "Clear"
                    << "\n";
        break;
      case '4':
        t_temp_oss_ << "Action: "
                    << "Reset"
                    << "\n";
        break;
      default:
        t_temp_oss_ << "Action: "
                    << "------"
                    << "\n";
        break;
    }
    t_temp_oss_ << "Price: " << std::fixed << std::setprecision(6) << price_ << "\n";
    t_temp_oss_ << "Size: " << quantity_ << "\n";
    t_temp_oss_ << "Order ID: " << order_id_ << "\n";
    t_temp_oss_ << "Level: " << level_ << "\n";
    switch (intermediate_) {
      case 'Y':
        t_temp_oss_ << "INTERMEDIATE: Yes"
                    << "\n";
        break;
      case 'N':
        t_temp_oss_ << "INTERMEDIATE: NO"
                    << "\n";
        break;
      case 'I':
        t_temp_oss_ << "INTERMEDIATE: I"
                    << "\n";
        break;
      default:
        t_temp_oss_ << "INTERMEDIATE " << int(intermediate_) << "\n";
        break;
    }
    return t_temp_oss_.str();
  }
};

struct HKOMDTrade {
  uint32_t msg_seq_num_;
  char contract_[HKOMD_MDS_CONTRACT_TEXT_SIZE];
  uint8_t side_;  // 2 for buy and 3 for sell
  double price_;
  uint64_t quantity_;
  uint8_t deal_type_;
  uint16_t deal_info_;
  uint64_t order_id_;
  uint64_t trade_id_;

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "SeqNo: " << msg_seq_num_ << "\n";
    t_temp_oss_ << "Contract: " << contract_ << "\n";
    t_temp_oss_ << "Side: " << int(side_) << "\n";
    t_temp_oss_ << "Price: " << std::fixed << std::setprecision(6) << price_ << "\n";
    t_temp_oss_ << "Size: " << quantity_ << "\n";

    switch (deal_type_) {
      case 0:
        t_temp_oss_ << "Deal_type_: None\n";
        break;
      case 1:
        t_temp_oss_ << "Deal_type_: Printable\n";
        break;
      case 2:
        t_temp_oss_ << "Deal_type_: Cross\n";
        break;
      case 3:
        t_temp_oss_ << "Deal_type_: Cross_and_Printable\n";
        break;
      case 4:
        t_temp_oss_ << "Deal_type_: Reported\n";
        break;
      default:
        t_temp_oss_ << "Deal_type_: " << int(deal_type_) << "\n";
        break;
    }
    t_temp_oss_ << "Deal_info_: " << int(deal_info_) << "\n";
    t_temp_oss_ << "OrderID: " << order_id_ << "\n";
    t_temp_oss_ << "TradeID: " << trade_id_ << "\n";  // we can choose to ignore it too
    return t_temp_oss_.str();
  }
};

struct HKOMDCommonStruct {
  msgType msg_;
  timeval time_;
  timeval packet_time_;
  union {
    HKOMDOrder order_;
    HKOMDTrade trade_;
  } data_;

  char* getContract() {
    switch (msg_) {
      case HKOMD_ORDER:
        return data_.order_.contract_;
        break;
      case HKOMD_TRADE:
        return data_.trade_.contract_;
        break;
      default:
        return NULL;
    }
  }

  inline std::string ToString() {
    switch (msg_) {
      case HKOMD_ORDER: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "\n========== HKOMD Order Message =============\n\n";
        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
        t_temp_oss_ << "Packet Time: " << packet_time_.tv_sec << "." << std::setw(6) << std::setfill('0')
                    << packet_time_.tv_usec << "\n";
        return (t_temp_oss_.str() + data_.order_.ToString() +
                "=======================================================\n");
      } break;
      case HKOMD_TRADE: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "\n========== HKOMD Trade Message =============\n\n";
        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
        t_temp_oss_ << "Packet Time: " << packet_time_.tv_sec << "." << std::setw(6) << std::setfill('0')
                    << packet_time_.tv_usec << "\n";
        return (t_temp_oss_.str() + data_.trade_.ToString() +
                "=======================================================\n");
      } break;
        break;
      default: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "NOT IMPLEMENTEND FOR this EVENT: " << msg_ << "\n";
        return t_temp_oss_.str();
      } break;
    }
    return "";
  }
};
/**************************************************************************************************
 * PriceFeed structs
 * */

enum PriceFeedmsgType { HKOMD_PF_DELTA = 1, HKOMD_PF_TRADE = 2 };

struct HKOMDPFDeltaStruct {
  char contract_[HKOMD_PF_MDS_CONTRACT_TEXT_SIZE];
  double price_;
  uint64_t quantity_;
  int32_t num_orders_;
  uint32_t msg_seq_num_;
  uint8_t action_;
  uint8_t level_;
  uint8_t side_;
  char intermediate_;

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "Seq. No: " << msg_seq_num_ << "\n";
    t_temp_oss_ << "Contract: " << contract_ << "\n";
    t_temp_oss_ << "Side: " << int(side_) << "\n";
    switch (action_) {
      case 0:
        t_temp_oss_ << "Action: "
                    << "NEW"
                    << "\n";
        break;
      case 1:
        t_temp_oss_ << "Action: "
                    << "CHANGE"
                    << "\n";
        break;
      case 2:
        t_temp_oss_ << "Action: "
                    << "DELETE"
                    << "\n";
        break;
      case 8:
        t_temp_oss_ << "Action: "
                    << "RESET"
                    << "\n";
        break;
      case 74:
        t_temp_oss_ << "Action: "
                    << "CLEAR"
                    << "\n";
        break;
      default:
        t_temp_oss_ << "Action: "
                    << "------"
                    << "\n";
        break;
    }

    t_temp_oss_ << "Price: " << std::fixed << std::setprecision(6) << price_ << "\n";
    t_temp_oss_ << "Size: " << quantity_ << "\n";
    t_temp_oss_ << "Num Ord: " << num_orders_ << "\n";
    t_temp_oss_ << "Level: " << int(level_) << "\n";

    switch (intermediate_) {
      case 'Y':
        t_temp_oss_ << "INTERMEDIATE: Yes "
                    << "\n";
        break;
      case 'N':
        t_temp_oss_ << "INTERMEDIATE: No "
                    << "\n";
        break;
      case 'I':
        t_temp_oss_ << "INTERMEDIATE: I"
                    << "\n";
        break;
      default:
        t_temp_oss_ << "INTERMEDIATE: " << int(intermediate_) << "\n";
        break;
    }

    return t_temp_oss_.str();
  }
};

struct HKOMDPFTradeStruct {
  char contract_[HKOMD_PF_MDS_CONTRACT_TEXT_SIZE];
  double price_;
  uint64_t quantity_;
  uint32_t msg_seq_num_;
  uint8_t side_;
  uint8_t deal_type_;
  uint16_t deal_info_;

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "Seq. No: " << msg_seq_num_ << "\n";
    t_temp_oss_ << "Contract: " << contract_ << "\n";
    t_temp_oss_ << "Side: " << int(side_) << "\n";
    t_temp_oss_ << "Price: " << std::fixed << std::setprecision(6) << price_ << "\n";
    t_temp_oss_ << "Quantity: " << quantity_ << "\n";
    t_temp_oss_ << "Deal_type: " << int(deal_type_) << "\n";
    t_temp_oss_ << "Deal_info_: " << int(deal_info_) << "\n";

    return t_temp_oss_.str();
  }
};

struct HKOMDPFCommonStruct {
  PriceFeedmsgType msg_;
  timeval time_;
  union {
    HKOMDPFDeltaStruct delta_;
    HKOMDPFTradeStruct trade_;
  } data_;

  char* getContract() {
    switch (msg_) {
      case HKOMD_PF_DELTA:
        return data_.delta_.contract_;
        break;
      case HKOMD_PF_TRADE:
        return data_.trade_.contract_;
        break;
      default:
        return NULL;
    }
    return NULL;
  }

  inline std::string ToString() {
    switch (msg_) {
      case HKOMD_PF_DELTA: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "\n========= HKOMPF Delta Message =========\n\n";
        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
        return (t_temp_oss_.str() + data_.delta_.ToString() +
                "=======================================================\n");
      } break;
      case HKOMD_PF_TRADE: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "\n========== HKOMDPF Trade Message =============\n\n";
        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
        return (t_temp_oss_.str() + data_.trade_.ToString() +
                "=======================================================\n");
      } break;
      default: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "NOT IMPLEMENTEND FOR this EVENT: " << msg_ << "\n";
        return t_temp_oss_.str();
      } break;
    }
    return " ";
  }
};
}
