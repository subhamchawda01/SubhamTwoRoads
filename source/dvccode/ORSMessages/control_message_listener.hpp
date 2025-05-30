/**
    \file dvccode/ORSMessages/control_message_listener.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_ORSMESSAGES_CONTROL_MESSAGE_LISTENER_H
#define BASE_ORSMESSAGES_CONTROL_MESSAGE_LISTENER_H

#include "dvccode/CDef/control_messages.hpp"
#include "dvccode/CDef/signal_msg.hpp"
#include "dvccode/CDef/mds_messages.hpp"

namespace HFSAT {

class ControlMessageListener {
 public:
  virtual ~ControlMessageListener() {}
  virtual void OnControlUpdate(const ControlMessage& _control_message_, const char* symbol_,
                               const int trader_id) = 0;  // messages from the trader
  virtual void OnPriceLevelUpdate(void* ptr_to_price_level_update, int length_of_bytes,
                                  HFSAT::MDS_MSG::MDSMessageExchType exch_type) {}
};

class SignalDataListener{
 public:
  virtual ~SignalDataListener() {}
  virtual void OnSignalDataUpdate(const char* symbol_, IVCurveData &iv_curve_data) = 0;
};

}

#endif  // BASE_ORSMESSAGES_CONTROL_MESSAGE_LISTENER_H
