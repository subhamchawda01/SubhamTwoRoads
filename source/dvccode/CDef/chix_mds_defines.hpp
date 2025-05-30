/**
    \file itch_defines.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551

*/
#pragma once

#include <string.h>
#include <iostream>
#include <stdio.h>
#include <sstream>
#include <iomanip>
#include <inttypes.h>
#include <sys/time.h>

namespace HFSAT {
enum PITCH_MSG_TYPE {
  TimestampchiX = 0x20,
  AddOrderMessageLong = 0x40,
  AddOrderMessageShort = 0x22,
  OrderExecchiX = 0x23,
  OrderExecWithPriceSize = 0x24,
  OrderReduceLong = 0x25,
  OrderReduceShort = 0x26,
  OrderModifyLong = 0x27,
  OrderModifyShort = 0x28,
  OrderDeletechiX = 0x29,
  TradeMessageLong = 0x41,
  TradeMessageShort = 0x2B,
  TradeMessageBreak = 0x2C,
  EndOfSession = 0x2D,

  ExpandedAddOrder = 0x2F,
  TradingStatus = 0x31,
  OffBookTradeMessage = 0x32,
  OffBookTradeBreakMessage = 0x33,
  StatisticsMessage = 0x34,
  UnitClear = 0x97,

};

#define BATSCHI_MDS_CONTRACT_TEXT_SIZE 9

namespace BATSCHI_PL_MDS {
enum l1MSGType { kL1BUY = 0, kL1SELL, kL1TRADE };

struct BatsChiPLCommonStruct {
  struct timeval time_;
  char contract_[BATSCHI_MDS_CONTRACT_TEXT_SIZE];
  uint8_t buy_sell_;
  uint8_t trade_;
  uint8_t intermediate_;
  double price;
  uint32_t size;
  uint32_t order_count_;
  inline bool isTradeMsg() { return (trade_); }
  inline double GetTradeDoublePrice() { return price; }
  inline HFSAT::TradeType_t GetTradeAggressorSide() {
    return (buy_sell_ == 'B') ? HFSAT::TradeType_t::kTradeTypeBuy : (buy_sell_ == 'S')
                                                                        ? HFSAT::TradeType_t::kTradeTypeSell
                                                                        : HFSAT::TradeType_t::kTradeTypeNoInfo;
  }
  inline uint32_t GetTradeSize() { return size; }
  inline void SetIntermediate(bool flag) {
    // TODO: Verify this
    if (!trade_) {
      intermediate_ = flag;
    }
  }
  char* getContract() { return contract_; }
  inline void print() { fprintf(stdout, "%s\n", ToString().c_str()); }
  inline std::string ToString() {
    std::string _this_buysell_ = buy_sell_ ? "SELL" : "BUY";

    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "\n============== BatsChi PLMessage ================\n\n";
    t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
    t_temp_oss_ << "Contract:   " << contract_ << "\n";
    t_temp_oss_ << "Buy_Sell:   " << _this_buysell_ << "\n";
    t_temp_oss_ << "IsTrade:    " << (int)trade_ << "\n";
    t_temp_oss_ << "Price:      " << std::fixed << std::setprecision(6) << price << "\n";
    t_temp_oss_ << "Size:       " << size << "\n";
    t_temp_oss_ << "OrderCount: " << order_count_ << "\n";
    t_temp_oss_ << "Inter:      " << (int)intermediate_ << "\n";
    return t_temp_oss_.str();
  }
};
}

namespace BATSCHI_MDS {
enum msgType { BATSCHI_TRADE, BATSCHI_ORDER, UNDEF };

struct BATSCHITradeStruct {
  char contract_[BATSCHI_MDS_CONTRACT_TEXT_SIZE];  ///< contract name
  uint64_t trd_qty_;                               ///< quantity in this trade
  double trd_px_;                                  ///< trade price
  uint64_t trd_id_;
  char trd_type_;

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "Contract: " << contract_ << "\n"
                << "TrdQty:   " << (uint32_t)trd_qty_ << "\n"
                << "TrdPx:    " << trd_px_ << "\n"
                << "TrdId:    " << trd_id_ << "\n"
                << "TrdType:  " << trd_type_ << "\n";

    return t_temp_oss_.str();
  }
};

struct BATSCHIOrderStruct {
  char contract_[BATSCHI_MDS_CONTRACT_TEXT_SIZE];
  double price_;
  uint32_t size_;
  uint32_t action_;  /// update action
  char type_;        /// entry type
  bool intermediate_;
  uint64_t order_id_;
  uint64_t match_id_;  // This could be many different things without the possibility of a
  // single appropriate and descriptive name, it has been called extra_info_.
  // The contextual information is inferred based on the field action_

  std::string getActionString() {
    switch (PITCH_MSG_TYPE(action_)) {
      case TimestampchiX:
        return "Timestamp";
        break;
      case AddOrderMessageLong:
        return "AddOrd";
        break;
      case AddOrderMessageShort:
        return "AddOrd";
        break;
      case OrderExecchiX:
        return "OrderExec";
        break;
      case OrderExecWithPriceSize:
        return "OrderExec";
        break;
      case OrderReduceLong:
        return "OrderReduce";
        break;
      case OrderReduceShort:
        return "OrderReduce";
        break;
      case OrderModifyLong:
        return "OrderModify";
        break;
      case OrderModifyShort:
        return "OrderModify";
        break;
      case OrderDeletechiX:
        return "OrderDel";
        break;
      case TradeMessageLong:
        return "Trade";
        break;
      case TradeMessageShort:
        return "Trade";
        break;
      case TradeMessageBreak:
        return "Trade";
        break;
      case EndOfSession:
        return "EndOfSession";
        break;

      case ExpandedAddOrder:
        return "ExpandedAddOrder";
        break;
      case TradingStatus:
        return "TradingStatus";
        break;
      case OffBookTradeMessage:
        return "OffBookTradeMessage";
        break;
      case OffBookTradeBreakMessage:
        return "OffBookTradeBreakMessage";
        break;
      case StatisticsMessage:
        return "StatisticsMessage";
        break;
      case UnitClear:
        return "UnitClear";
        break;
    }
    return "undefined";
  }

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "Contract: " << contract_ << "\n"
                << "Price:    " << std::fixed << std::setprecision(6) << price_ << "\n"
                << "Size:     " << size_ << "\n"
                << "Action:   " << getActionString() << "\n"
                << "Type:     " << type_ << "\n"
                << "Intermediate: " << intermediate_ << "\n"
                << "OrderId:     " << order_id_ << "\n"
                << "MatchId:     " << match_id_ << "\n";
    return t_temp_oss_.str();
  }
};

struct BATSCHICommonStruct {
  msgType msg_;
  struct timeval time_;
  union {
    BATSCHITradeStruct batschi_trds_;
    BATSCHIOrderStruct batschi_ordr_;
  } data_;
  inline bool isTradeMsg() { return (BATSCHI_TRADE == msg_); }
  inline double GetTradeDoublePrice() { return data_.batschi_trds_.trd_px_; }
  inline HFSAT::TradeType_t GetTradeAggressorSide() { return HFSAT::TradeType_t::kTradeTypeNoInfo; }
  inline uint32_t GetTradeSize() { return data_.batschi_trds_.trd_qty_; }
  inline void SetIntermediate(bool flag) {
    switch (msg_) {
      case BATSCHI_TRADE:
        break;
      case BATSCHI_ORDER:
        data_.batschi_ordr_.intermediate_ = flag;
        break;
      default:
        break;
    }
  }
  char* getContract() {
    switch (msg_) {
      case BATSCHI_TRADE:
        return data_.batschi_trds_.contract_;
        break;
      case BATSCHI_ORDER:
        return data_.batschi_ordr_.contract_;
        break;
      default:
        return NULL;
    }
  }

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }

  inline std::string ToString() {
    switch (msg_) {
      case BATSCHI_TRADE: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "\n============== BATSCHI Trade Message ================\n\n";
        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";

        return (t_temp_oss_.str() + data_.batschi_trds_.ToString() +
                "===================================================\n");
      } break;
      case BATSCHI_ORDER: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "\n============== BATSCHI Order Message ================\n\n";
        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";

        return (t_temp_oss_.str() + data_.batschi_ordr_.ToString() +
                "===================================================\n");
      } break;
      default: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "NOT IMPLEMENTED FOR THIS EVENT : " << msg_ << '\n';

        return t_temp_oss_.str();
      } break;
    }
  }
};
}
}
