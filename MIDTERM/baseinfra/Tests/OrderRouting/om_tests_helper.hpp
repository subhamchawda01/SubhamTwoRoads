/**
   \file Tests/baseinfra/OrderRouting/om_tests_helper.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#pragma once

#include "baseinfra/OrderRouting/order_manager_listeners.hpp"

namespace HFTEST {
using namespace HFSAT;

class DummyExchangeRejectListener : public ExchangeRejectsListener {
  /// Dummy class  just keeps track of number of calls made to freeze
 public:
  int num_calls_so_far() { return num_calls_so_far_; }

  /// automated to ensure controlled behavior
  virtual void OnGetFreezeDueToExchangeRejects(HFSAT::BaseUtils::FreezeEnforcedReason const& freeze_reason) {
    num_calls_so_far_++;
  }
  /// this one's manual to assure one took the control
  virtual void OnResetByManualInterventionOverRejects() {}

 private:
  int num_calls_so_far_;
};
}
