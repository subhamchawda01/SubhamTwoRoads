// =====================================================================================
//
//       Filename:  target_price_reporter.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  Wednesday 14 May 2014 09:30:54  GMT
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

#include "baseinfra/MarketAdapter/market_defines.hpp"

// TODO: move all smv functions accessing book to this interface
namespace HFSAT {

class TargetPriceReporter {
 public:
  virtual ~TargetPriceReporter() {}
  virtual double GetTargetPrice(const PriceType_t t_price_type_) const = 0;
};
}
