// =====================================================================================
// 
//       Filename:  IndicatorListener.hpp
// 
//    Description:  
// 
//        Version:  1.0
//        Created:  03/17/2019 03:44:07 AM
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

namespace HFSAT { namespace Indicator {

  class IndicatorListener{

    public : 

      virtual ~IndicatorListener(){}
      virtual void OnIndicatorUpdate( uint32_t const & index, double const & i_value ) = 0 ;

  };

}}
