/**
    \file dvccode/CDef/basic_ors_defines.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
 */
#ifndef BASE_BASIC_ORS_DEFINES_H
#define BASE_BASIC_ORS_DEFINES_H

#include <iomanip>
#include <sstream>
#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/ors_defines.hpp"

#define kExchangeAccountLen 16
#define kExchangeSenderSubLen 16

#define BEST_BID_PRICE_INIT 0
#define BEST_ASK_PRICE_INIT 100000000

#define USE_ORS_MARKET_DATA_THREAD 0
#define USE_SHM_LOOPBACK_LOGGING false

#define MAX_INTERNAL_MATCH_ARRARY_SIZE 16

#define SIZE_REMAINING_QTY_INVALID -1000  // invalid size qty to handle the size remaining with HK and OSE

namespace HFSAT {
namespace ORS {

struct ExchSimResponseStruct {
  char symbol[kSecNameLen];
  ORRType_t event;
  int saos;
  double price;
  TradeType_t buysell;
  int size_remaining;
  int size_executed;

  std::string ToString() const {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "Symbol:       " << symbol << "\n";
    t_temp_oss_ << "EventType : " << HFSAT::ToString(event) << "\n";
    t_temp_oss_ << "Saos:       " << saos << "\n";
    t_temp_oss_ << "Price:       " << std::fixed << std::setprecision(6) << price << "\n";
    t_temp_oss_ << "TradeType:       " << buysell << "\n";
    t_temp_oss_ << "SR:       " << size_remaining << "\n";
    t_temp_oss_ << "SE:       " << size_executed << "\n";
    return t_temp_oss_.str();
  }
};
}
}
#endif  // BASE_BASIC_ORS_DEFINES_H
