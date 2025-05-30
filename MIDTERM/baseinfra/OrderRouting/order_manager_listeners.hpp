/**
    \file OrderRouting/order_manager_listeners.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#pragma once

#include "dvccode/CDef/defines.hpp"
#include "baseinfra/BaseUtils/query_trading_auto_freeze_manager.hpp"

namespace HFSAT {

/** @brief interface for listeners of change in position */
class PositionChangeListener {
 public:
  virtual ~PositionChangeListener(){};
  virtual void OnPositionChange(int _new_position_, int position_diff_, const unsigned int security_id_) = 0;
};

/** @brief interface for listeners of execution of self orders */
class ExecutionListener {
 public:
  virtual ~ExecutionListener(){};
  virtual void OnExec(const int _new_position_, const int _exec_quantity_, const TradeType_t _buysell_,
                      const double _price_, const int r_int_price_, const int _security_id_) = 0;
};

/** @brief interface for listeners of cxlreject of self orders */
class CancelRejectListener {
 public:
  virtual ~CancelRejectListener(){};
  virtual void OnCancelReject(const TradeType_t _buysell_, const double _price_, const int r_int_price_,
                              const int _security_id_) = 0;
};

class RejectDueToFundsListener {
 public:
  virtual ~RejectDueToFundsListener(){};
  virtual void OnRejectDueToFunds(const TradeType_t _buysell_) = 0;
  virtual void OnWakeUpifRejectDueToFunds() = 0;
};

class FokFillRejectListener {
 public:
  virtual ~FokFillRejectListener(){};
  virtual void OnFokReject(const TradeType_t _buysell_, const double _price_, const int intpx_,
                           const int _size_remaining_) = 0;
  virtual void OnFokFill(const TradeType_t _buysell_, const double _price_, const int intpx_,
                         const int _size_exec_) = 0;
};

class ExchangeRejectsListener {
 public:
  virtual ~ExchangeRejectsListener(){};
  virtual void OnGetFreezeDueToExchangeRejects(
      HFSAT::BaseUtils::FreezeEnforcedReason const& freeze_reason) = 0;  // automated to ensure controlled behaviour
  virtual void OnResetByManualInterventionOverRejects() = 0;  // this one's manual to assure one took the control
};

class ORSRejectsFreezeListener {
 public:
  virtual ~ORSRejectsFreezeListener(){};
  virtual void OnGetFreezeDueToORSRejects() = 0;              // automated to ensure controlled behaviour
  virtual void OnResetByManualInterventionOverRejects() = 0;  // this one's manual to assure one took the control
};

/** @brief interface for listeners of any order management activity */
class OrderChangeListener {
 public:
  virtual ~OrderChangeListener(){};
  virtual void OnOrderChange() = 0;
};

/** @brief interface for listeners of change in global_position_ .. PromOrderManager */
class GlobalPositionChangeListener {
 public:
  virtual ~GlobalPositionChangeListener(){};
  virtual void OnGlobalPositionChange(const unsigned int _security_id_, int _new_global_position_) = 0;
};

class GlobalOrderExecListener {
 public:
  virtual ~GlobalOrderExecListener(){};
  virtual void OnGlobalOrderExec(const unsigned int _security_id_, const TradeType_t _buysell_, const int _size_,
                                 const double _trade_px_) = 0;
};

/** @brief interface for listeners of global order management activity .. PromOrderManager
* this call tells us that some order management activity has happened ...
* can be augmented to include the TradeType_t and int_price at which it has happened
* We are only interested in Bids above bestbid_int_price() and Asks below bestask_int_price() */
class GlobalOrderChangeListener {
 public:
  virtual ~GlobalOrderChangeListener(){};
  virtual void OnGlobalOrderChange(const unsigned int _security_id_, const TradeType_t _buysell_,
                                   const int _int_price_) = 0;
};
}
