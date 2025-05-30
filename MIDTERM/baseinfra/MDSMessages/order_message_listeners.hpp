/**
    \file MDSMessages/order_message_listeners.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
*/

#pragma once

#include <sys/time.h>
#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/ttime.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"

namespace HFSAT {

class OrderLevelListenerICE {
 public:
  virtual ~OrderLevelListenerICE() {}

  virtual void SetTimeToSkipUntilFirstEvent(const ttime_t t_start_time_) {}

  virtual void OnOrderAdd(const uint32_t t_security_id_, const TradeType_t t_buysell_, const int64_t t_order_id_,
                          const double t_price_, const uint32_t t_size_, const int64_t priority_) = 0;

  virtual void OnOrderModify(const uint32_t t_security_id_, const TradeType_t t_buysell_, const int64_t t_order_id_,
                             const double t_price_, const uint32_t t_size_) = 0;

  virtual void OnOrderDelete(const uint32_t t_security_id_, const int64_t t_order_id_) = 0;

  virtual void OnOrderExec(const uint32_t t_security_id_, const TradeType_t t_buysell_, const int64_t t_order_id_,
                           const double t_traded_price_, const uint32_t t_traded_size_) = 0;

  virtual void ResetBook(const unsigned int t_security_id_) = 0;
};

class OrderLevelListenerHK {
 public:
  virtual ~OrderLevelListenerHK() {}

  virtual void SetTimeToSkipUntilFirstEvent(const ttime_t t_start_time_) {}

  virtual void OnOrderAdd(const uint32_t t_security_id_, const TradeType_t t_buysell_, const int64_t t_order_id_,
                          const double t_price_, const uint32_t t_size_) = 0;

  virtual void OnOrderModify(const uint32_t t_security_id_, const TradeType_t t_buysell_, const int64_t t_order_id_,
                             const double t_price_, const uint32_t t_size_) = 0;

  virtual void OnOrderDelete(const uint32_t t_security_id_, const TradeType_t t_buysell_,
                             const int64_t t_order_id_) = 0;

  virtual void OnOrderExec(const uint32_t t_security_id_, const TradeType_t t_buysell_, const int64_t t_order_id_,
                           const double t_traded_price_, const uint32_t t_traded_size_) = 0;

  virtual void ResetBook(const unsigned int t_security_id_) = 0;
};

class OrderLevelListener {
 public:
  virtual ~OrderLevelListener() {}

  virtual void SetTimeToSkipUntilFirstEvent(const ttime_t start_time) {}

  virtual void OnOrderAdd(const uint32_t security_id, const TradeType_t buysell, const uint64_t order_id,
                          const uint32_t priority, const double price, const uint32_t size) = 0;

  virtual void OnOrderModify(const uint32_t security_id, const TradeType_t buysell, const uint64_t order_id,
                             const uint32_t priority, const double price, const uint32_t size) = 0;

  virtual void OnVolumeDelete(const uint32_t security_id, const TradeType_t buysell, const uint64_t order_id,
                              const uint32_t size) = 0;

  virtual void OnOrderDelete(const uint32_t security_id, const TradeType_t buysell, const uint64_t order_id) = 0;

  virtual void OnOrderExec(const uint32_t security_id, const TradeType_t buysell, const uint64_t order_id,
                           const double exec_price, const uint32_t size_exec, const uint32_t size_remaining) = 0;

  virtual void OnOrderExecWithPx(const uint32_t security_id, const uint64_t bid_order_id, const uint64_t ask_order_id,
                                 const double exec_price, const uint32_t size_exec, const uint32_t bid_remaining,
                                 const uint32_t ask_remaining) = 0;

  virtual void OnOrderSpExec(const uint32_t security_id, const TradeType_t buysell, const uint64_t order_id,
                             const double exec_price, const uint32_t size_exec, const uint32_t size_remaining) = 0;

  virtual void ResetBook(const unsigned int security_id) = 0;

  virtual void OnResetBegin(const unsigned int security_id) = 0;

  virtual void OnResetEnd(const unsigned int security_id) = 0;
  virtual void OnOrderModifyWithPrevOrderId(const uint32_t t_security_id_, const TradeType_t t_buysell_,
                                            const int64_t t_new_order_id_, const double t_price_,
                                            const uint32_t t_size_, const int64_t t_prev_order_id_,
                                            const double t_prev_price_, const uint32_t t_prev_size_) {}
  virtual void OnOrderExecWithTradeInfo(const uint32_t security_id, const uint8_t t_side_, const uint64_t order_id,
                                        const double exec_price, const uint32_t size_exec) {}
  virtual void OnTradingStatus(const uint32_t security_id, std::string status) {}
};
}
