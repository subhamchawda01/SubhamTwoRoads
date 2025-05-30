/**
 \file NSET/nse_container.cpp

 \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
 Address:
 Suite 217, Level 2, Prestige Omega,
 No 104, EPIP Zone, Whitefield,
 Bangalore - 560066
 India
 +91 80 4060 0717
 */

#include "infracore/NSET/nse_container.hpp"

#include "dvccode/CommonTradeUtils/date_time.hpp"

namespace HFSAT {
namespace NSE {

NSEContainer::NSEContainer() : today_date(DateTime::GetCurrentIsoDateLocal()) {
  for (int i = 0; i < DEF_MAX_SEC_ID; i++) {
    inst_desc_[i].token_ = -1;
  }
}

NSEContainer::~NSEContainer() {
#if 0
  for (int i = 0; i < DEF_MAX_SEC_ID; i++) {
    if (inst_desc_[i]) {
      delete inst_desc_[i];
      inst_desc_[i] = nullptr;
    }
    if (spread_inst_desc_[i]) {
      delete spread_inst_desc_[i];
      spread_inst_desc_[i] = nullptr;
    }
  }
#endif
}
}
}
