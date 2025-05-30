// =====================================================================================
// 
//       Filename:  unqiue_instance_for_storing_details_across.cpp
// 
//    Description:  
// 
//        Version:  1.0
//        Created:  01/18/2023 09:01:21 AM
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

#include "dvccode/Utils/unqiue_instance_for_storing_details_across.hpp"

namespace HFSAT {
DetailsDumping* DetailsDumping::unique_ptr = nullptr;

DetailsDumping::DetailsDumping() {}
DetailsDumping::~DetailsDumping() {}

DetailsDumping& DetailsDumping::GetUniqueInstance() {
  if (unique_ptr == nullptr) {
    unique_ptr = new DetailsDumping();
  }
  return *(unique_ptr);
}

}

