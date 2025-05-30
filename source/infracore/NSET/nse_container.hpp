/**
 \file NSET/nse_container.hpp

 \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
 Address:
 Suite 217, Level 2, Prestige Omega,
 No 104, EPIP Zone, Whitefield,
 Bangalore - 560066
 India
 +91 80 4060 0717
 */

#pragma once

#include "infracore/BasicOrderRoutingServer/defines.hpp"
#include "infracore/NSET/NSETemplates/DataDefines.hpp"

namespace HFSAT {
namespace NSE {

class NSEContainer {
 private:
 public:
  InstrumentDesc inst_desc_[DEF_MAX_SEC_ID];
  // SpreadInstrumentDesc* spread_inst_desc_[DEF_MAX_SEC_ID];

  uint32_t today_date;

  NSEContainer();
  ~NSEContainer();
};
}
}
