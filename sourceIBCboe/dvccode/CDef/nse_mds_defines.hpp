// =====================================================================================
//
//       Filename:  nse_mds_defines.hpp
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

#define NUM_JIFFY_IN_SECOND 65536
#define EPOCH_NSE_DIFF 315532800
#define GMT_IST_SECONDS_DIFF 19800

namespace NSE_UDP_MDS {

struct NSERefData {
  int32_t token;
  int64_t expiry;
  double strike_price;
  char instrument[7];
  char symbol[11];
  char option_type[3];
  char exchange_symbol[32];
  double lower_price_range;
  double upper_price_range;
  double price_multiplier;
  HFSAT::ExchSource_t exch_source;

  NSERefData() : token(0), expiry(0), strike_price(-1), lower_price_range(0), upper_price_range(0) {
    memset(instrument, 0, sizeof(instrument));
    memset(symbol, 0, sizeof(symbol));
    memset(option_type, 0, sizeof(option_type));
    memset(exchange_symbol, 0, sizeof(exchange_symbol));
  }

  void Init(int32_t token_, int64_t expiry_, double strike_price_, const char *instrument_, const char *symbol_,
            const char *option_type_, const char *exchange_symbol_, double price_mul, HFSAT::ExchSource_t ex_src,
            double lower_px = 0, double upper_px = 0) {
    NSERefData();
    token = token_;
    expiry = expiry_;
    strike_price = strike_price_;
    memcpy(instrument, instrument_, sizeof(instrument) - 1);
    memcpy(symbol, symbol_, strlen(symbol_));
    memcpy(option_type, option_type_, sizeof(option_type) - 1);
    memcpy(exchange_symbol, exchange_symbol_, sizeof(exchange_symbol) - 1);
    price_multiplier = price_mul;
    exch_source = ex_src;
    lower_price_range = lower_px;
    upper_price_range = upper_px;
  }

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "token: " << token << "\n";
    t_temp_oss_ << "expiry: " << expiry << "\n";
    t_temp_oss_ << "strike_price: " << strike_price << "\n";
    t_temp_oss_ << "instrument: " << instrument << "\n";
    t_temp_oss_ << "symbol: " << symbol << "\n";
    t_temp_oss_ << "option_type: " << option_type << "\n";
    t_temp_oss_ << "exchange_symbol: " << exchange_symbol << "\n";
    t_temp_oss_ << "lower_price_range_: " << lower_price_range << "\n";
    t_temp_oss_ << "upper_price_range_: " << upper_price_range << "\n";
    return t_temp_oss_.str();
  }
};

#define NSE_BOOK_DEPTH 5
#define NSE_CONTRACT_LENGTH 32

enum PriceFeedmsgType {
  NSE_BOOK = 1,
  NSE_TRADE = 2,
  NSE_GENERAL = 3,
};

struct NSEBookStruct {
  double bid_prices[NSE_BOOK_DEPTH];
  int32_t bid_sizes[NSE_BOOK_DEPTH];
  int32_t bid_orders[NSE_BOOK_DEPTH];

  double ask_prices[NSE_BOOK_DEPTH];
  int32_t ask_sizes[NSE_BOOK_DEPTH];
  int32_t ask_orders[NSE_BOOK_DEPTH];
  int32_t closing_price;
  int32_t open_price;
  int32_t high_price;
  int32_t low_price;

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "CloseingPrice: " << closing_price << "\n";
    t_temp_oss_ << "OpeningPrice: " << open_price << "\n";
    t_temp_oss_ << "HighPrice: " << high_price << "\n";
    t_temp_oss_ << "LowPrice: " << low_price << "\n";

    uint32_t total_bid_size = 0;
    uint32_t total_bid_orders = 0;
    uint32_t total_ask_size = 0;
    uint32_t total_ask_orders = 0;

    for (unsigned int level_counter_ = 0; level_counter_ < NSE_BOOK_DEPTH; level_counter_++) {
      total_bid_size += bid_sizes[level_counter_];
      total_bid_orders += bid_orders[level_counter_];
      total_ask_size += ask_sizes[level_counter_];
      total_ask_orders += ask_orders[level_counter_];
    }

    t_temp_oss_ << "CumulativeBidData : " << total_bid_orders << " : " << total_bid_size << "\n";
    t_temp_oss_ << "CumulativeAskData : " << total_ask_orders << " : " << total_ask_size << "\n\n";

    for (unsigned int level_counter_ = 0; level_counter_ < NSE_BOOK_DEPTH; level_counter_++)
      t_temp_oss_ << "( " << bid_orders[level_counter_] << " ) "
                  << "( " << bid_sizes[level_counter_] << " ) "
                  << " @ " << bid_prices[level_counter_] << " ---- " << ask_prices[level_counter_] << " @ "
                  << " ( " << ask_sizes[level_counter_] << " ) "
                  << " ( " << ask_orders[level_counter_] << " )\n";

    return t_temp_oss_.str();
  }
};

struct NSETradeStruct {
  double price;
  int32_t size;

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "Price: " << std::fixed << std::setprecision(6) << price << "\n";
    t_temp_oss_ << "Size: " << size << "\n";
    return t_temp_oss_.str();
  }
};

struct NSEGeneralStruct {
  int16_t branch_num;
  char broker_num[6];
  char action_code[4];
  char bcast_msg[240];

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "Branch Num: " << branch_num << "\n";
    t_temp_oss_ << "Broker Num: " << broker_num << "\n";
    t_temp_oss_ << "Action Code: " << action_code << "\n";
    t_temp_oss_ << "Bcast Msg: " << bcast_msg << "\n";
    return t_temp_oss_.str();
  }
};

struct NSECommonStruct {
  PriceFeedmsgType msg_;
  int32_t seq_no;
  timeval source_time;
  int16_t status;
  int32_t token;
  int32_t token_2;  // second leg in case of spread
  char symbol[NSE_CONTRACT_LENGTH];

  union {
    NSEBookStruct book;
    NSETradeStruct trade;
    NSEGeneralStruct general;
  } data;

  const char *getContract() const {
    // These changes are for logging the files as "token" instead of file names
    std::ostringstream oss;
    oss << token;
    if (token_2 > 0) {
      oss << "_" << token_2;
    }
    return oss.str().c_str();
    // return std::string ( contract ).c_str();
  }

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    switch (msg_) {
      case NSE_BOOK: {
        t_temp_oss_ << "\n========= NSE Book Message ==============\n";
        t_temp_oss_ << "SeqNum : " << seq_no << "\n";
        t_temp_oss_ << "Time : " << source_time.tv_sec << "." << source_time.tv_usec << "\n";
        t_temp_oss_ << "Status : " << status << "\n";
        t_temp_oss_ << "Token : " << token << "\n";
        t_temp_oss_ << "Token2 : " << token_2 << "\n";
        t_temp_oss_ << "Contract : " << symbol << "\n";
        return (t_temp_oss_.str() + data.book.ToString() + "=======================================================\n");
      } break;
      case NSE_TRADE: {
        t_temp_oss_ << "\n========== NSE Trade Message =============\n";
        t_temp_oss_ << "SeqNum : " << seq_no << "\n";
        t_temp_oss_ << "Time : " << source_time.tv_sec << "." << source_time.tv_usec << "\n";
        t_temp_oss_ << "Status : " << status << "\n";
        t_temp_oss_ << "Token : " << token << "\n";
        t_temp_oss_ << "Contract : " << symbol << "\n";
        return (t_temp_oss_.str() + data.trade.ToString() +
                "=======================================================\n");
      } break;
      case NSE_GENERAL: {
        t_temp_oss_ << "\n========== NSE General Message =============\n";
        t_temp_oss_ << "SeqNum : " << seq_no << "\n";
        t_temp_oss_ << "Time : " << source_time.tv_sec << "." << source_time.tv_usec << "\n";
        return (t_temp_oss_.str() + data.general.ToString() +
                "=======================================================\n");
      } break;
      default: {
        t_temp_oss_ << "NOT IMPLEMENTEND FOR this EVENT: " << msg_ << "\n";
        return t_temp_oss_.str();
      } break;
    }
    return " ";
  }
};
}
