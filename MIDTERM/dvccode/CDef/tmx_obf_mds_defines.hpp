/**
 \file dvccode/CDef/tmx_obf_mds_defines.hpp

 \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
 Address:
 Suite 217, Level 2, Prestige Omega,
 No 104, EPIP Zone, Whitefield,
 Bangalore - 560066
 India
 +91 80 4060 0717
 */
#pragma once

#include <string>
#include <sstream>
#include <inttypes.h>
#include <sys/time.h>
#include <iomanip>
#include <limits>
#include "dvccode/CDef/defines.hpp"
namespace TMX_OBF_MDS {

#define TMX_OBF_MDS_CONTRACT_TEXT_SIZE 18

struct TMXOBFOFOrderInfo {
  double price;
  int size;
  uint8_t side;
  int32_t priority;

  TMXOBFOFOrderInfo() : price(0.0), size(-1), side('-'), priority(0) {}

  TMXOBFOFOrderInfo(double t_price, int t_size, uint8_t t_side, uint32_t t_priority) {
    price = t_price;
    size = t_size;
    side = t_side;
    priority = t_priority;
  }
};

struct TMXRecoveryOrderInfo {
  double price;
  int size;
  uint8_t side;
  int32_t priority;
  uint32_t msg_seq_number;
  uint64_t order_id;

  TMXRecoveryOrderInfo() : price(0.0), size(-1), side('-'), priority(0), msg_seq_number(0), order_id(0) {}

  TMXRecoveryOrderInfo(double t_price, int t_size, uint8_t t_side, uint32_t t_priority, uint32_t t_msg_seq_number,
                       uint64_t t_order_id) {
    price = t_price;
    size = t_size;
    side = t_side;
    priority = t_priority;
    msg_seq_number = t_msg_seq_number;
    order_id = t_order_id;
  }
};

enum class TMXMsgType {
  kTMXAdd = 'A',
  kTMXDelete = 'D',
  kTMXModify = 'M',
  kTMXExec = 'E',
  kTMXExecWithTrade = 'C',

  kTMXTradingStatus = 'O',
  kTMXEquilibriumPrice = 'Z',
  kTMXPriceNotification = 'P',
  kTMXResetBegin = 'Q',
  kTMXResetEnd = 'R'
};

struct TMXAdd {
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

struct TMXModify {
  uint64_t order_id;
  double price;
  uint32_t size;
  uint64_t old_order_id;
  double old_price;
  uint32_t old_size;
  char side;

  inline std::string ToString() {
    std::ostringstream temp_oss;
    temp_oss << "OrderId: " << order_id << "\n";
    temp_oss << "Price: " << std::fixed << std::setprecision(6) << price << "\n";
    temp_oss << "Size: " << size << "\n";
    temp_oss << "PrevOrderId: " << old_order_id << "\n";
    temp_oss << "PrevPrice: " << std::fixed << std::setprecision(6) << old_price << "\n";
    temp_oss << "PrevSize: " << old_size << "\n";
    temp_oss << "Side: " << side << "\n";
    return temp_oss.str();
  }
};

struct TMXDelete {
  uint64_t order_id;
  char side;

  inline std::string ToString() {
    std::ostringstream temp_oss;
    temp_oss << "OrderId: " << order_id << "\n";
    temp_oss << "Side: " << side << "\n";
    return temp_oss.str();
  }
};

struct TMXExec {
  uint64_t order_id;
  uint32_t size_exec;
  uint64_t deal_num;
  double price;
  char side;

  inline std::string ToString() {
    std::ostringstream temp_oss;
    temp_oss << "OrderId: " << order_id << "\n";
    temp_oss << "Side: " << side << "\n";
    temp_oss << "SizeExec: " << size_exec << "\n";
    temp_oss << "Price: " << std::fixed << std::setprecision(6) << price << "\n";
    temp_oss << "DealNum: " << deal_num << "\n";
    return temp_oss.str();
  }
};

struct TMXExecWithTradeInfo {
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

struct TMXTradingStatus {
  char state[21];

  inline std::string ToString() {
    std::ostringstream temp_oss;
    temp_oss << "TradingStatus: " << state << "\n";
    return temp_oss.str();
  }
};

struct TMXEquilibriumPx {
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

struct TMXPriceNotification {
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

struct TMXCommonStruct {
  timeval time_;
  TMXMsgType msg_type;
  char contract[TMX_OBF_MDS_CONTRACT_TEXT_SIZE];
  bool intermediate_;
  uint32_t msg_seq_number_;
  uint32_t packet_number_;
  union {
    TMXAdd add;
    TMXModify modify;
    TMXDelete del;
    TMXExec exec;
    TMXExecWithTradeInfo exec_info;
    TMXTradingStatus status;
    TMXEquilibriumPx equi_px;
    TMXPriceNotification price_notif;
  } data;
  inline void SetIntermediate(bool flag) {
    // No intermediate
  }
  char* getContract() { return contract; }

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }

  bool isTradeMsg() const { return (msg_type == TMXMsgType::kTMXExec || msg_type == TMXMsgType::kTMXExecWithTrade); }
  inline double GetTradeDoublePrice() { return data.exec.price; }
  inline HFSAT::TradeType_t GetTradeAggressorSide() {
    return (data.exec.side == 'B') ? HFSAT::TradeType_t::kTradeTypeBuy : (data.exec.side == 'S')
                                                                             ? HFSAT::TradeType_t::kTradeTypeSell
                                                                             : HFSAT::TradeType_t::kTradeTypeNoInfo;
  }
  inline uint32_t GetTradeSize() { return data.exec.size_exec; }
  inline std::string ToString() {
    std::ostringstream temp_oss;
    temp_oss << "\n============== TMX Message ================\n\n";
    temp_oss << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
    temp_oss << "Contract: " << contract << '\n';
    temp_oss << "Intermediate: " << intermediate_ << '\n';
    temp_oss << "Msg_seq_number: " << msg_seq_number_ << '\n';
    temp_oss << "Packet_number: " << packet_number_ << '\n';

    switch (msg_type) {
      case TMXMsgType::kTMXAdd: {
        temp_oss << "MSGType: ADD\n";
        temp_oss << data.add.ToString();
        break;
      }
      case TMXMsgType::kTMXModify: {
        temp_oss << "MSGType: MODIFY\n";
        temp_oss << data.modify.ToString();
        break;
      }
      case TMXMsgType::kTMXDelete: {
        temp_oss << "MSGType: DELETE\n";
        temp_oss << data.del.ToString();
        break;
      }
      case TMXMsgType::kTMXExec: {
        temp_oss << "MSGType: EXEC\n";
        temp_oss << data.exec.ToString();
        break;
      }
      case TMXMsgType::kTMXExecWithTrade: {
        temp_oss << "MSGType: EXEC_WITH_TRADE_INFO\n";
        temp_oss << data.exec_info.ToString();
        break;
      }
      case TMXMsgType::kTMXTradingStatus: {
        temp_oss << "MSGType: TRADING_STATUS\n";
        temp_oss << data.status.ToString();
        break;
      }
      case TMXMsgType::kTMXEquilibriumPrice: {
        temp_oss << "MSGType: EQUILIBRIUM_PRICE\n";
        temp_oss << data.equi_px.ToString();
        break;
      }
      case TMXMsgType::kTMXPriceNotification: {
        temp_oss << "MSGType: PRICE_NOTOFICATION\n";
        temp_oss << data.price_notif.ToString();
        break;
      }
      case TMXMsgType::kTMXResetBegin: {
        temp_oss << "MSGType: RESET_BEGIN\n";
        break;
      }
      case TMXMsgType::kTMXResetEnd: {
        temp_oss << "MSGType: RESET_END\n";
        break;
      }
      default: { temp_oss << "msg_type:" << static_cast<int>(msg_type) << " not found.\n"; }
    }
    temp_oss << "===================================================\n";
    return temp_oss.str();
  }
};

/**************************************************************************************************
 * PriceFeed Structs
 **************************************************************************************************/

enum class TMXPriceFeedmsgType { TMX_PF_DELTA = 1, TMX_PF_TRADE = 2 };

struct TMXPFDeltaStruct {
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

struct TMXPFTradeStruct {
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

struct TMXPFCommonStruct {
  char contract_[TMX_OBF_MDS_CONTRACT_TEXT_SIZE];
  TMXPriceFeedmsgType msg_;
  timeval time_;
  union {
    TMXPFDeltaStruct delta_;
    TMXPFTradeStruct trade_;
  } data_;

  char* getContract() { return contract_; }
  inline bool isTradeMsg() { return (TMXPriceFeedmsgType::TMX_PF_TRADE == msg_); }
  inline void SetIntermediate(bool flag) {
    switch (msg_) {
      case TMXPriceFeedmsgType::TMX_PF_DELTA: {
        data_.delta_.intermediate_ = flag;
      } break;
      default:
        break;
    }
  }
  inline std::string ToString() {
    switch (msg_) {
      case TMXPriceFeedmsgType::TMX_PF_DELTA: {
        std::ostringstream temp_oss;
        temp_oss << "\n========= TMX Delta Message =========\n\n";
        temp_oss << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
        temp_oss << "Contract: " << contract_ << "\n";
        return (temp_oss.str() + data_.delta_.ToString() + "=======================================================\n");
      } break;
      case TMXPriceFeedmsgType::TMX_PF_TRADE: {
        std::ostringstream temp_oss;
        temp_oss << "\n========== TMX Trade Message =============\n\n";
        temp_oss << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
        temp_oss << "Contract: " << contract_ << "\n";
        return (temp_oss.str() + data_.trade_.ToString() + "=======================================================\n");
      } break;
      default: {
        std::ostringstream temp_oss;
        temp_oss << "NOT IMPLEMENTEND FOR this EVENT: " << static_cast<int>(msg_) << "\n";
        return temp_oss.str();
      } break;
    }
    return " ";
  }
};
}
