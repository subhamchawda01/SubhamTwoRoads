/**
    \file BasicOrderRoutingServer/defines.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
 */
#ifndef BASE_BASICORDERROUTINGSERVER_DEFINES_H
#define BASE_BASICORDERROUTINGSERVER_DEFINES_H

#include "dvccode/CDef/defines.hpp"

#define kExchangeAccountLen 16
#define kExchangeSenderSubLen 16

#define BEST_BID_PRICE_INIT 0
#define BEST_ASK_PRICE_INIT 100000000

#define USE_ORS_MARKET_DATA_THREAD 0
#define USE_SHM_LOOPBACK_LOGGING false

#define MAX_INTERNAL_MATCH_ARRARY_SIZE 16

#define SIZE_REMAINING_QTY_INVALID -1000  // invalid size qty to handle the size remaining with HK and OSE

#define PROFILE_ORS 0
#define PROFILE_ORS_NSE 0

namespace HFSAT {
namespace ORS {

struct PositionInfo {
  int position;
  int bid_size;
  int ask_size;
  int combined_security_id;
  double combined_bucket_weight_factor;
  int ordered_vol;
  int ordered_count;
  int traded_vol_;
  int traded_count_;
};

struct SACIPositionInfo {
  int position;
};

struct SACILimitsInfo {
  int worstcase_pos;
  int sec_id;  // Sec Id corresponding to worstcase_pos
};

struct SelfTradeInfo {
  int bid_price;
  int bid_size;
  int ask_price;
  int ask_size;
};
}
}
#endif  // BASE_BASICORDERROUTINGSERVER_DEFINES_H
