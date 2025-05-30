/**
   \file BSE/bse_container.hpp

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

namespace HFSAT {
namespace ORS {

struct InstrumentDescBSE {
  int32_t token_;
  int32_t product_id_;
  InstrumentDescBSE() {
    token_ = 0;
    product_id_ = 0;
  }

  std::string ToString() {
    std::stringstream ss;
    ss << " Token: " << token_;
    ss << " ProductId: " << product_id_;
    std::string str = ss.str();
    return str;
  }

  inline void SetInstrumentDesc(int32_t token, int32_t product_id) {
    token_ = token;
    product_id_ = product_id;
  }

  int32_t GetToken() { return token_; }
  int32_t ProductId() { return product_id_; }
};

class BSEContainer {
 private:
 public:
  InstrumentDescBSE inst_desc_[DEF_MAX_SEC_ID];

  uint32_t today_date;
  BSEContainer();
  ~BSEContainer();
};
}
}
