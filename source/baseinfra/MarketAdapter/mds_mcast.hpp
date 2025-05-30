/**
   \file MarketAdapter/mds_mcast.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 162, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */
#pragma once

#include "dvccode/CDef/mds_messages.hpp"
#include "baseinfra/MDSMessages/mds_message_listener.hpp"
#include "dvccode/Utils/multicast_sender_socket.hpp"

namespace HFSAT {

class MDSMcast : public PriceLevelGlobalListener,
                 public NTPPriceLevelGlobalListener,
                 public CFEPriceLevelGlobalListener,
                 public OrderGlobalListenerNSE {
 public:
  virtual ~MDSMcast() {}

  MDSMcast(std::string ip, int port, std::string iface, int usecs_sleep);

  void OnPriceLevelNew(const unsigned int t_security_id_, const TradeType_t t_buysell_, const int t_level_added_,
                       const double t_price_, const int t_new_size_, const int t_new_ordercount_,
                       const bool t_is_intermediate_message_) {}

  void OnPriceLevelDelete(const unsigned int t_security_id_, const TradeType_t t_buysell_, const int t_level_removed_,
                          const double t_price_, const bool t_is_intermediate_message_) {}

  void OnPriceLevelChange(const unsigned int t_security_id_, const TradeType_t t_buysell_, const int t_level_changed_,
                          const double t_price_, const int t_new_size_, const int t_new_ordercount_,
                          const bool t_is_intermediate_message_) {}

  void OnPriceLevelDeleteFrom(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                              const int t_min_level_deleted_, const bool t_is_intermediate_message_) {}

  void OnPriceLevelDeleteFrom(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                              const int t_max_level_removed_, const double t_price_,
                              const bool t_is_intermediate_message_) {}

  void OnPriceLevelDeleteThrough(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                 const int t_max_level_deleted_, const bool t_is_intermediate_message_) {}

  void OnPriceLevelOverlay(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                           const int t_level_overlayed_, const double t_price_, const int t_new_size_,
                           const int t_new_ordercount_, const bool t_is_intermediate_message_) {}
  void OnPriceLevelDeleteThru(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                              const bool t_is_intermediate_message_) {}

  void OnTrade(const unsigned int t_security_id_, const double t_trade_price_, const int t_trade_size_,
               const TradeType_t t_buysell_) {}

  void OnMarketStatusUpdate(const unsigned int t_security_id_, const MktStatus_t t_this_mkt_status_) {}

  void OnOTCTrade(const unsigned int t_security_id_, const double t_trade_price_, const int t_trade_size_) {}

  void OnTrade(const unsigned int t_security_id_, const double t_trade_price_, const int t_trade_size_) {}

  virtual void OnSpreadTrade(const unsigned int t_security_id_, const double t_trade_price_, const int t_trade_size_) {}

  virtual void OnOrderAdd(const uint32_t t_security_id_, const TradeType_t t_buysell_, const uint64_t order_id_,
                          const double t_price_, const uint32_t t_size_, const bool t_is_intermediate_) {}

  virtual void OnOrderModify(const uint32_t t_security_id_, const TradeType_t t_buysell_, const uint64_t order_id_,
                             const double t_price_, const uint32_t t_size_, const double t_prev_price_,
                             const uint32_t t_prev_size_) {}

  virtual void OnOrderDelete(const uint32_t t_security_id_, const TradeType_t t_buysell_, const uint64_t order_id_,
                             const double t_price_, const int t_size_, const bool t_delete_order_,
                             const bool t_is_intermediate_) {}

  virtual void OnTrade(const unsigned int t_security_id_, double const t_trade_price_, const int t_trade_size_,
                       uint64_t const buy_order_num, uint64_t const sell_order_num, const int32_t buy_size_remaining,
                       const int32_t sell_size_remaining) {}

  virtual void OnHiddenTrade(const unsigned int t_security_id_, double const t_trade_price_, const int t_trade_size_,
                             uint64_t const buy_order_num, uint64_t const sell_order_num,
                             const int32_t buy_size_remaining, const int32_t sell_size_remaining) {}

  virtual void OnTradeExecRange(const unsigned int t_security_id_, double const t_low_exec_band_, const int t_high_exec_band_) {}

  void OnPriceLevelUpdate(void* ptr_to_price_level_update, int length_of_bytes,
                          HFSAT::MDS_MSG::MDSMessageExchType exch_type);

  void SetTimestampAndMulticast(char* buffer, int final_len, HFSAT::MDS_MSG::MDSMessageExchType exch_type,
                                void* ptr_to_price_level_update, int length_of_bytes, int exch_type_sz, int data_tv_sec,
                                int data_tv_usec);

  virtual void OnOrderModify(const uint32_t t_security_id_, const TradeType_t t_buysell_, const uint64_t order_id_,
                             const double t_price_, const uint32_t t_size_) {}
  virtual void OnOrderDelete(const uint32_t t_security_id_, const TradeType_t t_buysell_, const uint64_t order_id_,
                             const double t_price_, const bool t_delete_order_, const bool t_is_intermediate_) {}
  virtual void OnTrade(const unsigned int t_security_id_, double const t_trade_price_, const int t_trade_size_,
                       uint64_t const buy_order_num, uint64_t const sell_order_num) {}
  virtual void OnHiddenTrade(const unsigned int t_security_id_, double const t_trade_price_, const int t_trade_size_,
                             uint64_t const buy_order_num, uint64_t const sell_order_num) {}

 private:
  HFSAT::MulticastSenderSocket* socket_;
  int usecs_sleep_;
  ttime_t last_data_time_;
};
}
