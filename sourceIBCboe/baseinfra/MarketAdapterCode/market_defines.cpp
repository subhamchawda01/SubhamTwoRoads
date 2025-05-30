/**
   \file MarketAdapterCode/market_defines.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */
#include "baseinfra/MarketAdapter/market_defines.hpp"

namespace HFSAT {

PriceType_t StringToPriceType_t(std::string t_in_str_) {
  for (PriceType_t i = kPriceTypeMidprice; i < kPriceTypeMax; i = (PriceType_t)(i + 1)) {
    if (t_in_str_.compare(PriceTypeStrings[i]) == 0) {
      return i;
    }
  }

  // added becasue we might often make a mistake in "MidPrice" and "Midprice"
  if (t_in_str_.compare("MidPrice") == 0) return kPriceTypeMidprice;

  return kPriceTypeMax;
}
}
