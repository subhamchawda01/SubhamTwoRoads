/**
 \file dvccode/CDef/krx_mds_defines.hpp

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
// namespace HFSAT {
namespace KRX_MDS {

#define KRX_SYMBOL_LEN 12
#define KRX_BOOK_DEPTH 5

enum class MsgType { KRX_BOOK, KRX_TRADE, KRX_QUOTE };

struct KRXTradeStruct {
  double price;
  int size;
  char side;

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "Price: " << std::fixed << std::setprecision(6) << price << "\n";
    t_temp_oss_ << "Size: " << size << "\n";
    t_temp_oss_ << "Side: " << side << "\n";
    return t_temp_oss_.str();
  }
};

struct KRXBookStruct {
  double bid_prices[KRX_BOOK_DEPTH];
  int bid_sizes[KRX_BOOK_DEPTH];
  int bid_orders[KRX_BOOK_DEPTH];

  double ask_prices[KRX_BOOK_DEPTH];
  int ask_sizes[KRX_BOOK_DEPTH];
  int ask_orders[KRX_BOOK_DEPTH];

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    for (auto i = 0u; i < 5; i++) {
      t_temp_oss_ << "( " << std::setw(4) << std::setfill(' ') << bid_orders[i] << " " << std::setw(4)
                  << std::setfill(' ') << bid_sizes[i] << " " << std::setw(8) << std::setfill(' ')
                  << std::setprecision(2) << std::fixed << bid_prices[i] << "    X    " << std::setprecision(2)
                  << std::fixed << ask_prices[i] << " " << std::setw(6) << std::setfill(' ') << ask_sizes[i] << " "
                  << std::setw(6) << std::setfill(' ') << ask_orders[i] << std::setw(4) << std::setfill(' ') << " )\n";
    }
    return t_temp_oss_.str();
  }
};

struct KRXCommonStruct {
  MsgType msg_type;
  timeval time_;
  char contract[KRX_SYMBOL_LEN];
  union {
    KRXTradeStruct trade;
    KRXBookStruct book;
  } data;

  const char* getContract() const { return contract; }

  bool IsTradeMsg() const { return msg_type == MsgType::KRX_TRADE; }

  std::string ToString() {
    std::ostringstream oss;
    switch (msg_type) {
      case MsgType::KRX_TRADE: {
        oss << "\n================KRX TRADE Message==============\n";
        oss << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
        oss << "Contract: " << contract << "\n";
        return (oss.str() + data.trade.ToString() + "================================================\n");
      } break;
      case MsgType::KRX_BOOK: {
        oss << "\n================KRX BOOK Message===============\n";
        oss << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
        oss << "Contract: " << contract << "\n";
        return (oss.str() + data.book.ToString() + "===============================================\n");
      } break;
      case MsgType::KRX_QUOTE: {  // TODO: Check why this type?
        oss << "\n================KRX QUOTE Message===============\n";
        oss << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
        oss << "Contract: " << contract << "\n";
        return (oss.str() + data.book.ToString() + "===============================================\n");
      } break;
      default: {
        std::ostringstream temp_oss;
        temp_oss << "KRX: NOT IMPLEMENTED FOR THIS EVENT " << int(msg_type) << std::endl;
        return temp_oss.str();
      } break;
    }
    return oss.str();
  }
};
}
//}
