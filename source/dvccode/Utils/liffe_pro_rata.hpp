/**
   \file dvccode/Utils/liffe_pro_rata.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066, India
   +91 80 4060 0717
*/

#pragma once

#include <vector>
#include <iostream>

namespace HFSAT {

class LIFFEProRata {
 private:
 public:
  LIFFEProRata();

  static bool GetLFIFills(std::vector<int> sizes_, std::vector<int> fills_, int trade_size_, bool improve_priority_);
  static bool GetLFLFills(std::vector<int> sizes_, std::vector<int> fills_, int trade_size_, bool improve_priority_);
  static bool GetFills(std::vector<int> sizes_, std::vector<int> fills_, int trade_size_, bool improve_priority_,
                       int exp_);
};
}
