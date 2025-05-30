/**
    \file dvccode/CDef/nasdaq_mds_defines.hpp

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
#define NASDAQ_MDS_CONTRACT_TEXT_SIZE 7

enum ITCH_MSG_TYPE {
  Timestamp = 'T',
  SystemEvent = 'S',
  StockDirectory = 'R',
  TradingAction = 'H',
  ShortSalePriceTestRestrictedIndicator = 'Y',
  MarketParticipantPosition = 'L',
  AddOrderMessage = 'A',
  AddOrderWithMpidAttribution = 'F',
  OrderExec = 'E',
  OrderExecWithPrice = 'C',
  OrderCancel = 'X',
  OrderDelete = 'D',
  OrderReplace = 'U',
  Trade = 'P',
  CrossTrade = 'Q',
  BrokenTrade = 'B',
  NetOrderImbalance = 'I'
};

namespace NASDAQ_MDS {

enum msgType { ADD = 1, DEL = 2, EXEC = 3, CANCEL = 4, REPLACE = 5, TRADE = 6 };

struct AddOrd {
  uint64_t order_ref_num;
  uint32_t num_shares;
  uint32_t price;
  char buy_sell;
};

struct DelOrd {
  uint64_t order_ref_num;
};

struct ExecOrd {
  uint64_t order_ref_num;
  uint64_t match_id;
  uint32_t size;
};

struct CancelOrd {
  uint64_t order_ref_num;
  uint32_t size;
};
struct ReplaceOrd {
  uint64_t original_id;
  uint64_t new_id;
  uint32_t size;
  uint32_t price;
};

struct Trade {
  uint32_t price;
  uint32_t size;
  char buy_sell;
};

enum l1MSGType { kL1BUY = 0, kL1SELL, kL1TRADE };
struct NasdaqPLTransportStruct {
#define NASDAQ_PL_BUY_SELL_TRADE_MASK 3
#define NASDAQ_PL_INTERMEDIATE_MASK 4
#define NASDAQ_PL_TRADETYPE_MASK 12

  char contract_[7];
  uint8_t type_;
  uint32_t iprice;  ///< not sure if we need to send this ... but if it is already computed we can send this as well
  uint32_t size;
  uint32_t order_count_;

  // 0 for buy, 1 for sell, 2 for trade
  void set_buy_sell_trade(uint8_t buy_sell_trade) {
    type_ = type_ & ~NASDAQ_PL_BUY_SELL_TRADE_MASK;
    type_ |= buy_sell_trade;
  }
  void set_buy() {
    type_ = type_ & ~NASDAQ_PL_BUY_SELL_TRADE_MASK;
    // type_ |= 0;
  }
  void set_sell() {
    type_ = type_ & ~NASDAQ_PL_BUY_SELL_TRADE_MASK;
    type_ |= 1;
  }
  void set_trade() {
    type_ = type_ & ~NASDAQ_PL_BUY_SELL_TRADE_MASK;
    type_ |= 2;
  }

  void set_intermediate(uint8_t isIntermediate) {
    type_ = type_ & ~NASDAQ_PL_INTERMEDIATE_MASK;
    type_ |= isIntermediate << 2;
  }

  // 0 = buy, 1 = sell, 3 = no info
  void set_aggressor(uint8_t aggressor) {
    type_ = type_ & ~NASDAQ_PL_TRADETYPE_MASK;
    type_ |= aggressor << 3;
  }

  l1MSGType get_buy_sell_trade() const { return (l1MSGType)(type_ & NASDAQ_PL_BUY_SELL_TRADE_MASK); }

  bool get_intermediate() const { return 1 == ((type_ & NASDAQ_PL_INTERMEDIATE_MASK) >> 2); }

  // 0 = buy, 1 = sell, 3 = no info
  uint8_t get_aggressor() const { return (type_ & NASDAQ_PL_TRADETYPE_MASK) >> 3; }
};

struct NasdaqTransportStruct {
  char contract_[7];
  uint8_t msgType_;  // msg_type > 16 means it is intermediate. subtract 16 to get the actual value
  union {
    AddOrd add_ord;
    DelOrd del_ord;
    ExecOrd exec_ord;
    CancelOrd cancel_ord;
    ReplaceOrd replace_ord;
    Trade trade;
  } data_;
};

struct NASDAQPLCommonStruct {
  struct timeval time_;
  NasdaqPLTransportStruct nts_;
  const char* getContract() { return nts_.contract_; }
  inline void print() { fprintf(stdout, "%s\n", ToString().c_str()); }
  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "\n============== NASDAQ PLMessage ================\n\n";
    t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
    t_temp_oss_ << "Contract:   " << nts_.contract_ << "\n";
    t_temp_oss_ << "Buy_Sell:   " << nts_.get_buy_sell_trade() << "\n";
    t_temp_oss_ << "Price:      " << nts_.iprice << "\n";
    t_temp_oss_ << "Size:       " << nts_.size << "\n";
    t_temp_oss_ << "OrderCount: " << nts_.order_count_ << "\n";
    return t_temp_oss_.str();
  }
};

struct NASDAQCommonStruct {
  // time stamp is not included in transport struct since we almost
  // always want to set the time at the logger. hence any value being passed over the wire adds to
  // bandwidth unnecessarily
  struct timeval time_;

  NasdaqTransportStruct nts_;

  char* getContract() { return nts_.contract_; }

  inline void print() { fprintf(stdout, "%s\n", ToString().c_str()); }

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "\n============== NASDAQ Message ================\n\n";
    t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
    t_temp_oss_ << "Contract:   " << nts_.contract_ << "\n";
    bool intermediate = (nts_.msgType_ >= 16);
    if (intermediate) nts_.msgType_ -= 16;
    switch (nts_.msgType_) {
      case ADD:
        t_temp_oss_ << "MsgType:    "
                    << "ADD\n";
        t_temp_oss_ << "OrderId:    " << nts_.data_.add_ord.order_ref_num << "\n";
        t_temp_oss_ << "Size:       " << nts_.data_.add_ord.num_shares << "\n";
        t_temp_oss_ << "Price:      " << nts_.data_.add_ord.price << "\n";
        t_temp_oss_ << "Buy_Sell:   " << nts_.data_.add_ord.buy_sell << "\n";
        break;
      case DEL:
        t_temp_oss_ << "MsgType:    "
                    << "DEL\n";
        t_temp_oss_ << "OrderId:    " << nts_.data_.del_ord.order_ref_num << "\n";
        break;
      case EXEC:
        t_temp_oss_ << "MsgType:    "
                    << "EXEC\n";
        t_temp_oss_ << "OrderId:    " << nts_.data_.exec_ord.order_ref_num << "\n";
        t_temp_oss_ << "Size:       " << nts_.data_.exec_ord.size << "\n";
        t_temp_oss_ << "MatchId:    " << nts_.data_.exec_ord.match_id << "\n";
        break;
      case CANCEL:
        t_temp_oss_ << "MsgType:    "
                    << "CANCEL\n";
        t_temp_oss_ << "OrderId:    " << nts_.data_.cancel_ord.order_ref_num << "\n";
        t_temp_oss_ << "Size:       " << nts_.data_.cancel_ord.size << "\n";
        break;
      case REPLACE:
        t_temp_oss_ << "MsgType:    "
                    << "REPLACE\n";
        t_temp_oss_ << "OldOrderId: " << nts_.data_.replace_ord.original_id << "\n";
        t_temp_oss_ << "OrderId:    " << nts_.data_.replace_ord.new_id << "\n";
        t_temp_oss_ << "Size:       " << nts_.data_.replace_ord.size << "\n";
        t_temp_oss_ << "Price:      " << nts_.data_.replace_ord.price << "\n";
        break;
      case TRADE:
        t_temp_oss_ << "MsgType:    "
                    << "Trade\n";
        t_temp_oss_ << "Size:       " << nts_.data_.trade.size << "\n";
        t_temp_oss_ << "Price:      " << nts_.data_.trade.price << "\n";
        t_temp_oss_ << "Buy_Sell:   " << nts_.data_.trade.buy_sell << "\n";
        break;
      default:
        t_temp_oss_ << "NOT IMPLEMENTED FOR THIS EVENT : " << nts_.msgType_ << "\n";
        break;
    }
    t_temp_oss_ << "Inter:      " << intermediate << "\n";
    return t_temp_oss_.str();
  }
};
}
}
