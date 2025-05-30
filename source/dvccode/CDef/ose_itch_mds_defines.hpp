/**
 \file dvccode/CDef/ose_itch_mds_defines.hpp

 \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
 Address:
 Suite 217, Level 2, Prestige Omega,
 No 104, EPIP Zone, Whitefield,
 Bangalore - 560066
 India
 +91 80 4060 0717
 */
#pragma once

#include <limits>
#include <string>
#include <sstream>
#include <iomanip>
#include <inttypes.h>
#include <sys/time.h>

namespace OSE_ITCH_MDS {

#define OSE_ITCH_MDS_CONTRACT_TEXT_SIZE 32

enum OSEMsgType : uint8_t {
  kOSEAdd = 'A',
  kOSEDelete = 'D',
  kOSEExec = 'E',
  kOSEExecWithTrade = 'C',

  kOSETradingStatus = 'O',
  kOSEEquilibriumPrice = 'Z',
  kOSEPriceNotification = 'P',

  kOSEResetBegin = '8',
  kOSEResetEnd = '9'
};

struct OSEOrderInfo {
  double price;
  int size;
  uint8_t side;
  int32_t priority;

  OSEOrderInfo() : price(0.0), size(-1), side('-'), priority(0) {}

  OSEOrderInfo(double t_price, int t_size, uint8_t t_side, uint32_t t_priority) {
    price = t_price;
    size = t_size;
    side = t_side;
    priority = t_priority;
  }
};

struct OSERecoveryOrderInfo {
  double price;
  int size;
  uint8_t side;
  int32_t priority;
  uint32_t msg_seq_number;
  uint64_t order_id;

  OSERecoveryOrderInfo() : price(0.0), size(-1), side('-'), priority(0), msg_seq_number(0), order_id(0) {}

  OSERecoveryOrderInfo(double t_price, int t_size, uint8_t t_side, uint32_t t_priority, uint32_t t_msg_seq_number,
                       uint64_t t_order_id) {
    price = t_price;
    size = t_size;
    side = t_side;
    priority = t_priority;
    msg_seq_number = t_msg_seq_number;
    order_id = t_order_id;
  }
};

struct OSEAdd {
  uint64_t order_id;
  double price;
  uint32_t size;
  uint32_t priority;
  char side;

  inline std::string ToString() {
    std::ostringstream temp_oss;
    temp_oss << "OrderId: " << order_id << "\n";
    temp_oss << "Price: " << std::fixed << std::setprecision(6) << price << "\n";
    temp_oss << "Size: " << size << "\n";
    temp_oss << "Priority: " << priority << "\n";
    temp_oss << "Side: " << side << "\n";
    return temp_oss.str();
  }
};

struct OSEDelete {
  uint64_t order_id;
  char side;

  inline std::string ToString() {
    std::ostringstream temp_oss;
    temp_oss << "OrderId: " << order_id << "\n";
    temp_oss << "Side: " << side << "\n";
    return temp_oss.str();
  }
};

struct OSEExec {
  uint64_t order_id;
  uint32_t size_exec;
  uint64_t deal_num;
  uint64_t combo_group_id;
  char side;

  inline std::string ToString() {
    std::ostringstream temp_oss;
    temp_oss << "OrderId: " << order_id << "\n";
    temp_oss << "SizeExec: " << size_exec << "\n";
    temp_oss << "DealNum: " << deal_num << "\n";
    temp_oss << "Side: " << side << "\n";
    temp_oss << "ComboGroupID: " << combo_group_id << "\n";
    return temp_oss.str();
  }
};

struct OSEExecWithTradeInfo {
  uint64_t order_id;
  double price;
  uint32_t size_exec;
  uint32_t deal_num;
  uint64_t combo_group_id;
  char side;
  char trade_at_cross;

  inline std::string ToString() {
    std::ostringstream temp_oss;
    temp_oss << "OrderId: " << order_id << "\n";
    temp_oss << "Price: " << std::fixed << std::setprecision(6) << price << "\n";
    temp_oss << "SizeExec: " << size_exec << "\n";
    temp_oss << "DealNum: " << deal_num << "\n";
    temp_oss << "Side: " << side << "\n";
    temp_oss << "ComboGroupID: " << combo_group_id << "\n";
    return temp_oss.str();
  }
};

struct OSETradingStatus {
  char state[21];

  inline std::string ToString() {
    std::ostringstream temp_oss;
    temp_oss << "TradingStatus: " << state << "\n";
    return temp_oss.str();
  }
};

struct OSEEquilibriumPx {
  double price;
  uint32_t bid_size;
  uint32_t ask_size;

  inline std::string ToString() {
    std::ostringstream temp_oss;
    temp_oss << "Price: " << std::fixed << std::setprecision(6) << price << "\n";
    temp_oss << "BestBidQty: " << bid_size << "\n";
    temp_oss << "BestAskQty: " << ask_size << "\n";
    return temp_oss.str();
  }
};

struct OSEPriceNotification {
  uint32_t deal_num;
  uint64_t combo_group_id;
  uint32_t size_exec;
  double price;
  char trade_at_cross;
  char side;

  inline std::string ToString() {
    std::ostringstream temp_oss;
    temp_oss << "Price: " << std::fixed << std::setprecision(6) << price << "\n";
    temp_oss << "SizeExec: " << size_exec << "\n";
    temp_oss << "Side: " << side << "\n";
    temp_oss << "DealNum: " << deal_num << "\n";
    temp_oss << "ComboGroupID: " << combo_group_id << "\n";
    temp_oss << "TradeAtCross: " << trade_at_cross << "\n";

    return temp_oss.str();
  }
};

struct OSECommonStruct {
  timeval time_;
  OSEMsgType msg_type;
  bool intermediate;
  char contract[OSE_ITCH_MDS_CONTRACT_TEXT_SIZE];
  uint32_t msg_seq_number;

  union {
    OSEAdd add;
    OSEDelete del;
    OSEExec exec;
    OSEExecWithTradeInfo exec_info;
    OSETradingStatus status;
    OSEEquilibriumPx equi_px;
    OSEPriceNotification price_notif;
  } data;

  char* getContract() { return contract; }

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }

  bool isTradeMsg() const { return (msg_type == kOSEExec || msg_type == kOSEExecWithTrade); }
  inline void SetIntermediate(bool flag) {
    if (!isTradeMsg()) {
      intermediate = flag;
    }
  }
  inline double GetTradeDoublePrice() {
    return -1;  // Currently not supported in OSE
  }
  inline HFSAT::TradeType_t GetTradeAggressorSide() {
    return (data.exec.side == 'B') ? HFSAT::TradeType_t::kTradeTypeBuy : (data.exec.side == 'S')
                                                                             ? HFSAT::TradeType_t::kTradeTypeSell
                                                                             : HFSAT::TradeType_t::kTradeTypeNoInfo;
  }
  inline uint32_t GetTradeSize() { return data.exec.size_exec; }
  inline std::string ToString() {
    std::ostringstream temp_oss;
    temp_oss << "\n============== OSE Message ================\n\n";
    temp_oss << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
    temp_oss << "Contract: " << contract << '\n';
    temp_oss << "Intermediate: " << (intermediate ? 'Y' : 'N') << '\n';
    temp_oss << "MSGSeqNo: " << msg_seq_number << '\n';

    switch (msg_type) {
      case kOSEAdd: {
        temp_oss << "MSGType: ADD\n";
        temp_oss << data.add.ToString();
        break;
      }
      case kOSEDelete: {
        temp_oss << "MSGType: DELETE\n";
        temp_oss << data.del.ToString();
        break;
      }
      case kOSEExec: {
        temp_oss << "MSGType: EXEC\n";
        temp_oss << data.exec.ToString();
        break;
      }
      case kOSEExecWithTrade: {
        temp_oss << "MSGType: EXEC_WITH_TRADE_INFO\n";
        temp_oss << data.exec_info.ToString();
        break;
      }
      case kOSETradingStatus: {
        temp_oss << "MSGType: TRADING_STATUS\n";
        temp_oss << data.status.ToString();
        break;
      }
      case kOSEEquilibriumPrice: {
        temp_oss << "MSGType: EQUILIBRIUM_PRICE\n";
        temp_oss << data.equi_px.ToString();
        break;
      }
      case kOSEPriceNotification: {
        temp_oss << "MSGType: PRICE_NOTOFICATION\n";
        temp_oss << data.price_notif.ToString();
        break;
      }
      case kOSEResetBegin: {
        temp_oss << "MSGType: RESET BEGIN\n";

        break;
      }
      case kOSEResetEnd: {
        temp_oss << "MSGType: RESET END\n";
        break;
      }
      default: { temp_oss << "msg_type:" << msg_type << " not found.\n"; }
    }
    temp_oss << "===================================================\n";
    return temp_oss.str();
  }
};

struct OSECommonStructOld {
  timeval time_;
  OSEMsgType msg_type;
  char contract[OSE_ITCH_MDS_CONTRACT_TEXT_SIZE];

  union {
    OSEAdd add;
    OSEDelete del;
    OSEExec exec;
    OSEExecWithTradeInfo exec_info;
    OSETradingStatus status;
    OSEEquilibriumPx equi_px;
    OSEPriceNotification price_notif;
  } data;

  char* getContract() { return contract; }

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }

  bool isTradeMsg() const { return (msg_type == kOSEExec || msg_type == kOSEExecWithTrade); }

  inline std::string ToString() {
    std::ostringstream temp_oss;
    temp_oss << "\n============== OSE Message ================\n\n";
    temp_oss << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
    temp_oss << "Contract: " << contract << '\n';

    switch (msg_type) {
      case kOSEAdd: {
        temp_oss << "MSGType: ADD\n";
        temp_oss << data.add.ToString();
        break;
      }
      case kOSEDelete: {
        temp_oss << "MSGType: DELETE\n";
        temp_oss << data.del.ToString();
        break;
      }
      case kOSEExec: {
        temp_oss << "MSGType: EXEC\n";
        temp_oss << data.exec.ToString();
        break;
      }
      case kOSEExecWithTrade: {
        temp_oss << "MSGType: EXEC_WITH_TRADE_INFO\n";
        temp_oss << data.exec_info.ToString();
        break;
      }
      case kOSETradingStatus: {
        temp_oss << "MSGType: TRADING_STATUS\n";
        temp_oss << data.status.ToString();
        break;
      }
      case kOSEEquilibriumPrice: {
        temp_oss << "MSGType: EQUILIBRIUM_PRICE\n";
        temp_oss << data.equi_px.ToString();
        break;
      }
      case kOSEPriceNotification: {
        temp_oss << "MSGType: PRICE_NOTOFICATION\n";
        temp_oss << data.price_notif.ToString();
        break;
      }
      case kOSEResetBegin: {
        temp_oss << "MSGType: RESET BEGIN\n";

        break;
      }
      case kOSEResetEnd: {
        temp_oss << "MSGType: RESET END\n";
        break;
      }
      default: { temp_oss << "msg_type:" << msg_type << " not found.\n"; }
    }
    temp_oss << "===================================================\n";
    return temp_oss.str();
  }
};

/**************************************************************************************************
 * PriceFeed Structs
 **************************************************************************************************/

enum PriceFeedmsgType { OSE_PF_DELTA = 1, OSE_PF_TRADE = 2, OSE_PF_RESET_BEGIN = 3, OSE_PF_RESET_END = 4 };

struct OSEPFDeltaStruct {
  double price_;
  uint64_t quantity_;
  int32_t num_orders_;
  uint32_t msg_seq_num_;
  uint32_t level_;
  uint8_t action_;  // 0 -> New, 1 -> Change, 2 -> Delete, 3 -> Reset, 4 -> Clear
  uint8_t side_;
  char intermediate_;

  inline std::string ToString() {
    std::ostringstream temp_oss;
    temp_oss << "Seq. No: " << msg_seq_num_ << "\n";
    temp_oss << "Side: " << char(side_) << "\n";
    switch (action_) {
      case '0':
        temp_oss << "Action: "
                 << "NEW"
                 << "\n";
        break;
      case '1':
        temp_oss << "Action: "
                 << "CHANGE"
                 << "\n";
        break;
      case '2':
        temp_oss << "Action: "
                 << "DELETE"
                 << "\n";
        break;
      case '3':
        temp_oss << "Action: "
                 << "RESET"
                 << "\n";
        break;
      case '4':
        temp_oss << "Action: "
                 << "CLEAR"
                 << "\n";
        break;
      default:
        temp_oss << "Action: "
                 << "------"
                 << "\n";
        break;
    }

    temp_oss << "Price: " << std::fixed << std::setprecision(6) << price_ << "\n";
    temp_oss << "Size: " << quantity_ << "\n";
    temp_oss << "Num Ord: " << num_orders_ << "\n";
    temp_oss << "Level: " << int(level_) << "\n";

    switch (intermediate_) {
      case 'Y':
        temp_oss << "Intermediate: Yes "
                 << "\n";
        break;
      case 'N':
        temp_oss << "Intermediate: No "
                 << "\n";
        break;
      case 'I':
        temp_oss << "Intermediate: I"
                 << "\n";
        break;
      default:
        temp_oss << "Intermediate: " << int(intermediate_) << "\n";
        break;
    }

    return temp_oss.str();
  }
};

struct OSEPFTradeStruct {
  double price_;
  uint64_t quantity_;
  uint32_t msg_seq_num_;
  uint8_t side_;
  uint8_t deal_type_;
  uint16_t deal_info_;

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "Seq. No: " << msg_seq_num_ << "\n";
    t_temp_oss_ << "Side: " << char(side_) << "\n";
    t_temp_oss_ << "Price: " << std::fixed << std::setprecision(6) << price_ << "\n";
    t_temp_oss_ << "Quantity: " << quantity_ << "\n";
    t_temp_oss_ << "Deal_type: " << int(deal_type_) << "\n";
    t_temp_oss_ << "Deal_info_: " << int(deal_info_) << "\n";

    return t_temp_oss_.str();
  }
};

struct OSEPFCommonStruct {
  char contract_[OSE_ITCH_MDS_CONTRACT_TEXT_SIZE];
  PriceFeedmsgType msg_;
  timeval time_;
  union {
    OSEPFDeltaStruct delta_;
    OSEPFTradeStruct trade_;
  } data_;

  char* getContract() { return contract_; }
  inline bool isTradeMsg() { return (OSE_PF_TRADE == msg_); }
  inline double GetTradeDoublePrice() { return data_.trade_.price_; }
  inline HFSAT::TradeType_t GetTradeAggressorSide() {
    return (data_.trade_.side_ == 'B') ? HFSAT::TradeType_t::kTradeTypeBuy : (data_.trade_.side_ == 'S')
                                                                                 ? HFSAT::TradeType_t::kTradeTypeSell
                                                                                 : HFSAT::TradeType_t::kTradeTypeNoInfo;
  }
  inline uint32_t GetTradeSize() { return data_.trade_.quantity_; }
  inline void SetIntermediate(bool flag) {
    switch (msg_) {
      case OSE_PF_DELTA: {
        data_.delta_.intermediate_ = flag;
      } break;
      case OSE_PF_TRADE: {
      } break;
      case OSE_PF_RESET_BEGIN: {
      } break;
      case OSE_PF_RESET_END: {
      } break;
      default: { } break; }
  }
  inline std::string ToString() {
    switch (msg_) {
      case OSE_PF_DELTA: {
        std::ostringstream temp_oss;
        temp_oss << "\n========= OSE Delta Message =========\n\n";
        temp_oss << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
        temp_oss << "Contract: " << contract_ << "\n";
        return (temp_oss.str() + data_.delta_.ToString() + "=======================================================\n");
      } break;
      case OSE_PF_TRADE: {
        std::ostringstream temp_oss;
        temp_oss << "\n========== OSE Trade Message =============\n\n";
        temp_oss << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
        temp_oss << "Contract: " << contract_ << "\n";
        return (temp_oss.str() + data_.trade_.ToString() + "=======================================================\n");
      } break;
      case OSE_PF_RESET_BEGIN: {
        std::ostringstream temp_oss;
        temp_oss << "\n========== OSE Reset Begin Message =============\n\n";
        temp_oss << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
        temp_oss << "Contract: " << contract_ << "\n";
        return (temp_oss.str() + "=======================================================\n");
      } break;
      case OSE_PF_RESET_END: {
        std::ostringstream temp_oss;
        temp_oss << "\n========== OSE Reset End Message =============\n\n";
        temp_oss << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
        temp_oss << "Contract: " << contract_ << "\n";
        return (temp_oss.str() + "=======================================================\n");
      } break;
      default: {
        std::ostringstream temp_oss;
        temp_oss << "NOT IMPLEMENTEND FOR this EVENT: " << msg_ << "\n";
        return temp_oss.str();
      } break;
    }
    return " ";
  }
};
}
