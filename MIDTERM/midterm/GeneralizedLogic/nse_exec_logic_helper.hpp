#pragma once
#include <math.h>
#include "dvccode/ExternalData/external_time_listener.hpp"

namespace NSE_SIMPLEEXEC {
class NseExecLogicHelper {
public:
  NseExecLogicHelper() {}

  virtual ~NseExecLogicHelper() {}
  virtual NSE_SIMPLEEXEC::SimpleNseExecLogic *
  Setup(HFSAT::SmartOrderManager *&om_, std::string &shortcode_,
        HFSAT::SecurityMarketView *smv_, NSE_SIMPLEEXEC::ParamSet *param_, HFSAT::ExternalDataListener *filesource) = 0;
};
}
