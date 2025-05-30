/**
    \file dvccode/CDef/trading_location_manager.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_CDEF_TRADING_LOCATION_MANAGER_H
#define BASE_CDEF_TRADING_LOCATION_MANAGER_H

#include <string>
#include "dvccode/CDef/defines.hpp"

namespace HFSAT {

namespace TradingLocationUtils {

TradingLocation_t GetTradingLocationFromLOC_NAME(const std::string location);

TradingLocation_t GetTradingLocationFromIP(std::string ip_address);
TradingLocation_t GetTradingLocationFromHostname();

const char* GetTradingLocationName(const TradingLocation_t r_trading_location_);

unsigned int GetMSecsBetweenTradingLocations(const TradingLocation_t dest_trading_location_,
                                             const TradingLocation_t src_trading_location_);
unsigned int GetUSecsBetweenTradingLocations(const TradingLocation_t dest_trading_location_,
                                             const TradingLocation_t src_trading_location_);
int GetAddedDelay(const TradingLocation_t trading_location_file_read_, const TradingLocation_t r_trading_location_);
bool UseFasterDataForShortcodeAtLocation(std::string t_shortcode_, const TradingLocation_t t_trading_location_,
                                         const int& _trading_date_, bool t_use_fake_faster_data_);
unsigned int GetUsecsForFasterDataForShortcodeAtLocation(std::string t_shortcode_,
                                                         const TradingLocation_t t_trading_location_,
                                                         const int& _trading_date_ = 0,
                                                         bool t_use_fake_faster_data_ = false);

inline TradingLocation_t GetTradingLocationExch(const ExchSource_t r_exch_source_) {
  switch (r_exch_source_) {
    case kExchSourceInvalid:
      return kTLocMAX;
      break;  // Error case
    case kExchSourceCME:
      return kTLocCHI;
      break;
    case kExchSourceEUREX:
      return kTLocFR2;
      break;
    case kExchSourceEOBI:
      return kTLocFR2;
      break;
    case kExchSourceBMF:
      return kTLocBMF;
      break;
    case kExchSourceBMFEQ:
      return kTLocBMF;
      break;
    case kExchSourceNTP:
      return kTLocBMF;
      break;
    case kExchSourceTMX:
      return kTLocTMX;
      break;
    case kExchSourceMEFF:
      return kTLocMAD;
      break;  // Not present yet
    case kExchSourceIDEM:
      return kTLocMIL;
      break;  // Not present yet
    case kExchSourceHONGKONG:
      return kTLocHK;
      break;
    case kExchSourceREUTERS:
      return kTLocIX4;
      break;  // Not present yet
    case kExchSourceICE:
      return kTLocBSL;
      break;
    case kExchSourceEBS:
      return kTLocM1;
      break;  // Not present yet, same as EUREX, nothing called EBS in our codebase
    case kExchSourceLIFFE:
      return kTLocBSL;
      break;
    case kExchSourceRTS:
      return kTLocM1;
      break;
    case kExchSourceMICEX:
      return kTLocM1;
      break;
    case kExchSourceMICEX_EQ:
      return kTLocM1;
      break;
    case kExchSourceMICEX_CR:
      return kTLocM1;
      break;
    case kExchSourceLSE:
      return kTLocMAX;
      break;  // Not present yet
    case kExchSourceBATSCHI:
      return kTLocBSL;
      break;
    case kExchSourceJPY:
      return kTLocJPY;
      break;
    case kExchSourceJPY_L1:
      return kTLocJPY;
      break;
    case kExchSourcePUMA:
      return kTLocBMF;
      break;
    case kExchSourceTSE:
      return kTLocJPY;
      break;
    case kExchSourceCFE:
      return kTLocCFE;
      break;
    case kExchSourceESPEED:
      return kTLocCRT;
      break;
    case kExchSourceASX:
      return kTLocSYD;
      break;
    case kExchSourceNSE:
      return kTLocNSE;
      break;
    case kExchSourceSGX:
      return kTLocSPR;
      break;
    case kExchSourceBSE:
      return kTLocBSE;
      break;
    case kExchSourceKRX:
      return kTLocKRX;
      break;
    case kExchSourceCME_FPGA:
      return kTLocCHI;
      break;
    default:
      return kTLocMAX;
      break;
  }
}

bool IsCurrentLocationPrimary(const ExchSource_t r_exch_source_);
}
}

#endif  // BASE_CDEF_TRADING_LOCATION_MANAGER_H
