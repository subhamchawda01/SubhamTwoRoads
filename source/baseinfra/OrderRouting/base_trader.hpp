/**
    \file OrderRouting/base_trader.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_ORDERROUTING_BASE_TRADER_H
#define BASE_ORDERROUTING_BASE_TRADER_H

#include <set>
#include "baseinfra/OrderRouting/base_order.hpp"

#define kInvalidServerAssignedClientId -1

namespace HFSAT {

/// Interface that every trader object might extend ...
/// There are two trader objects that directly extend this : BaseLiveTrader and BaseSimTrader
class BaseTrader {
 public:
  virtual ~BaseTrader(){};

  virtual void SendTrade(const BaseOrder& order) = 0;
  virtual void UpdateThrottle(int throttle_num, int ioc_throttle_num) = 0;
  virtual void FakeSendTrade(){};
  virtual void Cancel(const BaseOrder& order) = 0;
  virtual void Modify(const BaseOrder& order, double _new_price_, int _new_int_price_, int _new_size_requested_) = 0;
  virtual void NotifyRiskLimitsToOrs(const char* sec_name, int max_pos){};
  // Only live trader will override this one
  // clear_storage is only introduced here so we have some way of cleaning set in SIM,
  // Why -> Because we don't want to deal with flags like live trading or not for handling SIM
  virtual void SendRecoveryRequest(const int32_t missing_sequence, const int32_t server_assigned_client_id,
                                   std::set<int32_t>& clear_storage) const {
    clear_storage.clear();
  }

  // Not Required In Live/Sim Mode
  //    virtual void Replay ( const BaseOrder & _order_ ) const = 0;

  virtual int GetClientId() const = 0;
  virtual void IgnoreFromGlobalPos() = 0;
};
}

#endif  // BASE_ORDERROUTING_BASE_TRADER_H
