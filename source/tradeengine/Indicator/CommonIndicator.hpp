// =====================================================================================
// 
//       Filename:  CommonIndicator.hpp
// 
//    Description:  
// 
//        Version:  1.0
//        Created:  03/17/2019 03:51:41 AM
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

#include "baseinfra/MarketAdapter/security_market_view_change_listener.hpp"
#include "tradeengine/Indicator/IndicatorListener.hpp"

namespace HFSAT { namespace Indicator {

  class CommonIndicator : public HFSAT::SecurityMarketViewChangeListener{

    protected :
      HFSAT::SecurityMarketView * smv_;
      std::string price_type_;
      double weight_;
      std::string iname_;
      std::vector<std::string> iproperty_vector_;
      std::vector<IndicatorListener*> ilistener_vec_;

    public : 

      CommonIndicator( HFSAT::SecurityMarketView * smv,
                       std::string price_type,
                       double weight,
                       std::string iname,
                       std::vector<std::string> iproperty_vector,
                       std::vector<IndicatorListener*> ilistener_vec): 
        smv_(smv),
        price_type_(price_type),
        weight_(weight),
        iname_(iname),
        iproperty_vector_(iproperty_vector),
        ilistener_vec(ilistener_vec){
      }

      virtual ~CommonIndicator() {}      
      virtual void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_){}
      virtual void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_, const MarketUpdateInfo& _market_update_info_){}
      virtual void OnIndicatorUpdate( uint32_t const & index, double const & i_value ){}

  };

} }
