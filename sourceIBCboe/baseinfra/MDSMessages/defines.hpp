/**
    \file MDSMessages/defines.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
*/
#ifndef BASE_MDSMESSAGES_DEFINES_H
#define BASE_MDSMESSAGES_DEFINES_H

#include "dvccode/CDef/defines.hpp"

namespace HFSAT {

typedef enum {
  kMDUType_PLNone = 0,
  kMDUType_PLNew,
  kMDUType_PLDelete,
  kMDUType_PLChange,
  kMDUType_PLDeleteFrom,
  kMDUType_PLDeleteThrough,
  kMDUType_PLOverlay
} MDUType_t;
}

#endif  // BASE_MDSMESSAGES_DEFINES_H
