#ifndef _GENERIC_L1_DATA_STRUCT_
#define _GENERIC_L1_DATA_STRUCT_

#include <string>

#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/ttime.hpp"

namespace HFSAT {

#define GENERIC_L1_DATA_EXCHANGE_SYMBOL_SIZE 32

enum GenericL1DataType { L1_DELTA = 1, L1_TRADE = 2 };

struct GenericL1DataStruct {
 public:
  const std::string ToString();
  const char* getContract() { return symbol; }
  inline bool isTradeMsg() { return (L1_TRADE == type); }
  inline double GetTradeDoublePrice() { return price; }
  inline HFSAT::TradeType_t GetTradeAggressorSide() {
    return (side == 'B') ? HFSAT::TradeType_t::kTradeTypeBuy : (side == 'S')
                                                                         ? HFSAT::TradeType_t::kTradeTypeSell
                                                                         : HFSAT::TradeType_t::kTradeTypeNoInfo;
  }
  inline uint32_t GetTradeSize() { return size; }
  inline void SetIntermediate(bool flag) {
    is_intermediate = flag;
    // No intermediate flag
  }
  bool is_intermediate;
  GenericL1DataType type;
  TradeType_t side;
  int size;
  int bestbid_ordercount;
  int bestask_ordercount;
  int bestbid_size;
  int bestask_size;
  double price;
  double bestbid_price;
  double bestask_price;
  ttime_t time;
  char symbol[GENERIC_L1_DATA_EXCHANGE_SYMBOL_SIZE];
};
}

#endif
