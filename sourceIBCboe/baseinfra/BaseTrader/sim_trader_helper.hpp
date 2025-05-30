/**
    \file SimMarketMaker/sim_trader_helper.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/

#pragma once

#include "baseinfra/SimMarketMaker/base_sim_market_maker.hpp"
#include "baseinfra/BaseTrader/base_sim_trader.hpp"

namespace HFSAT {

class SimTraderHelper {
 public:
  inline static BaseTrader* GetSimTrader(const std::string& account, BaseSimMarketMaker* smm) {
    return new HFSAT::BaseSimTrader(account, smm);
  }
};
}
