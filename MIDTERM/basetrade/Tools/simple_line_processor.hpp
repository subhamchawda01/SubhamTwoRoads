/**
   \file Tools/simple_line_processor.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066, India
   +91 80 4060 0717
*/
#ifndef BASE_TOOLS_SIMPLE_LINE_PROCESSOR_HPP
#define BASE_TOOLS_SIMPLE_LINE_PROCESSOR_HPP

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <vector>

namespace HFSAT {

class SimpleLineProcessor {
 public:
  virtual ~SimpleLineProcessor(){};

  virtual void AddWord(double _item_) = 0;
  virtual void FinishLine() = 0;
  virtual void Close() = 0;
};
}

#endif  // BASE_TOOLS_SIMPLE_LINE_PROCESSOR_HPP
