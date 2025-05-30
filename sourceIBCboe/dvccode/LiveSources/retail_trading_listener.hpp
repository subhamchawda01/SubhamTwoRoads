// =====================================================================================
//
//       Filename:  retail_trading_listener.hpp
//
//    Description:  i
//
//        Version:  1.0
//        Created:  Thursday 17 April 2014 08:02:51  GMT
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

#pragma once
#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/retail_data_defines.hpp"

namespace HFSAT {

class RetailTradingListener {
 public:
  virtual ~RetailTradingListener() {}
  virtual void OnRetailOfferUpdate(unsigned int _security_id_, const std::string& _shortcode_,
                                   const std::string& _secname_, const int _server_assigned_client_id_,
                                   const HFSAT::CDef::RetailOffer& _retail_offer_) = 0;
  virtual void OnRetailOfferUpdate(const std::string& _secname_, const HFSAT::CDef::RetailOffer& _retail_offer_) = 0;
};
}
