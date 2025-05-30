/**
    \file OrderRouting/market_model.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#ifndef BASE_ORDERROUTING_MARKET_MODEL_H
#define BASE_ORDERROUTING_MARKET_MODEL_H
#include <iostream>

#define DEF_SIZE_PROB_LEVEL 16384

namespace HFSAT {

struct MarketModel {
  int com_usecs_;
  int conf_usecs_;
  int cxl_usecs_;
  int ors_mkt_diff_;
  int send_delay_diff;
  int cxl_delay_diff;

  MarketModel()
      : com_usecs_(10000),
        conf_usecs_(10000),
        cxl_usecs_(10000),
        ors_mkt_diff_(0),
        send_delay_diff(0),
        cxl_delay_diff(0) {}

  MarketModel(const MarketModel& new_market_model)
      : com_usecs_(new_market_model.com_usecs_),
        conf_usecs_(new_market_model.conf_usecs_),
        cxl_usecs_(new_market_model.cxl_usecs_),
        ors_mkt_diff_(new_market_model.ors_mkt_diff_),
        send_delay_diff(new_market_model.send_delay_diff),
        cxl_delay_diff(new_market_model.cxl_delay_diff) {}
};

inline std::ostream& operator<<(std::ostream& strm, const MarketModel& a) {
  return strm << a.com_usecs_ << ' ' << a.conf_usecs_ << ' ' << a.cxl_usecs_ << ' ' << a.ors_mkt_diff_ << ' '
              << a.send_delay_diff << ' ' << a.cxl_delay_diff;
}
}
#endif  // BASE_ORDERROUTING_MARKET_MODEL_H
