#pragma once
#include <string>

#include "dvccode/CombinedControlUtils/combined_control_messages.hpp"

namespace HFSAT {

// Combined Control Listener listens to the control messages sent to the combined writer
class CombinedControlMessageListener {
 public:
  virtual ~CombinedControlMessageListener() {}

  virtual void OnCombinedControlMessageReceived(CombinedControlMessage combined_control_request_) = 0;
};
}
