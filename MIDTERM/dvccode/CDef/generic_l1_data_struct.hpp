#ifndef _GENERIC_L1_DATA_STRUCT_
#define _GENERIC_L1_DATA_STRUCT_

#include <string>

#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/ttime.hpp"

namespace HFSAT {

#define GENERIC_L1_DATA_EXCHANGE_SYMBOL_SIZE 17

enum GenericL1DataType { L1_DELTA = 1, L1_TRADE = 2 };
struct GenericL1DataTrade {
 public:
  const std::string ToString();
  TradeType_t side;
  double price;
  int size;
};

struct GenericL1DataDelta {
 public:
  const std::string ToString();

  double bestbid_price;
  int bestbid_size;
  int bestbid_ordercount;

  double bestask_price;
  int bestask_size;
  int bestask_ordercount;
};

struct GenericL1DataStruct {
 public:
  const std::string ToString();
  const char* getContract() { return symbol; }
  inline bool isTradeMsg() { return (L1_TRADE == type); }
  inline double GetTradeDoublePrice() { return trade.price; }
  inline HFSAT::TradeType_t GetTradeAggressorSide() {
    return (trade.side == 'B') ? HFSAT::TradeType_t::kTradeTypeBuy : (trade.side == 'S')
                                                                         ? HFSAT::TradeType_t::kTradeTypeSell
                                                                         : HFSAT::TradeType_t::kTradeTypeNoInfo;
  }
  inline uint32_t GetTradeSize() { return trade.size; }
  inline void SetIntermediate(bool flag) {
    // No intermediate flag
  }
  char symbol[GENERIC_L1_DATA_EXCHANGE_SYMBOL_SIZE];
  ttime_t time;

  GenericL1DataType type;
  union {
    GenericL1DataDelta delta;
    GenericL1DataTrade trade;
  };
};
}

#endif
