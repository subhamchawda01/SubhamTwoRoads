#pragma once
#include <math.h>

namespace NSE_SIMPLEEXEC {

class NseExecutionListener {
public:
  virtual ~NseExecutionListener() {}
  virtual void OnExec(std::string _order_id_, int _traded_qty_,
                      double _price_) = 0;
};
}
