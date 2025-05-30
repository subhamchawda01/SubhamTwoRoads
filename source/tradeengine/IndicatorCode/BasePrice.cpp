#include "tradeengine/Indicator/BasePrice.hpp"

BasePrice::BasePrice(HFSAT::SecurityMarketView* smv, HFSAT::Watch& _watch_, HFSAT::DebugLogger& _dbglogger_,
                     double weight)
    : smv_(smv),
      watch_(_watch_),
      dbglogger_(_dbglogger_),
      base_bid_price_(0),
      base_ask_price_(0),
      weight_(weight),
      is_ready_(false) {
  InitializeDataSubscription();
}

BasePrice::~BasePrice() {}
