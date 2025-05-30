#pragma once
#include <math.h>
#include "nse_execution_listener.hpp"

namespace NSE_SIMPLEEXEC {
class NseExecutionListenerManager {
public:
  std::vector<NseExecutionListener *> generalized_exec_logics_;

protected:
  NseExecutionListenerManager() {}

public:
  static NseExecutionListenerManager &GetUniqueInstance() {
    static NseExecutionListenerManager uniqueinstance_;
    return uniqueinstance_;
  }

  ~NseExecutionListenerManager() {}

  void SubscribeNseExecutionListener(NseExecutionListener *t_exec_logic_) {
    HFSAT::VectorUtils::UniqueVectorAdd(generalized_exec_logics_,
                                        t_exec_logic_);
  }

  void NotifyNseExecutionListener(std::string t_order_id_, int t_traded_qty_,
                                  double t_price_) {
    for (auto exec_logic_ : generalized_exec_logics_) {
      exec_logic_->OnExec(t_order_id_, t_traded_qty_, t_price_);
    }
  }
};
}
