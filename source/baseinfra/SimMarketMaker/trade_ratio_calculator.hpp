// =====================================================================================
//
//       Filename:  SimMarketMaker/trade_ratio_calculator.cpp
//
//    Description:  Used to calculate trade ratio for a product on given day
//                  Which is further used in calculation of qA and qB
//
//
//        Version:  1.0
//        Created:  Tuesday 10 September 2013 07:02:58  GMT
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
#include "baseinfra/MarketAdapter/security_market_view.hpp"
namespace HFSAT {
class TradeRatioCalculator {
 public:
  static void BidAskTradeRatio(const SecurityMarketView& dep_market_view_, const Watch& watch_,
                               const double& percentile_, double& rw_bid_trade_ratio_, double& rw_ask_trade_ratio_);
};
}
