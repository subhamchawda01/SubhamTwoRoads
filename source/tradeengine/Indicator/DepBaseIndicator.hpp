// =====================================================================================
// 
//       Filename:  DepBaseIndicator.hpp
// 
//    Description:  
// 
//        Version:  1.0
//        Created:  03/17/2019 04:12:32 AM
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

#include "tradeengine/Indicator/CommonIndicator.hpp"

namespace HFSAT { namespace Indicator {

  class DepBaseIndicator : public CommonIndicator{

    public :

      DepBaseIndicator(HFSAT::SecurityMarketView * smv,
                       std::string price_type,
                       double weight,
                       std::string iname,
                       std::vector<std::string> iproperty_vector,
                       std::vector<IndicatorListener*> ilistener_vec):
        CommonIndicator(smv, price_type, weight, iname, iproperty_vector, ilistener_vec) {

      }

      void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_){}
      void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_, const MarketUpdateInfo& _market_update_info_){}
      void OnIndicatorUpdate( uint32_t const & index, double const & i_value ){}

      ~DepBaseIndicator() {}

  };

} }
