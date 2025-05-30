// =====================================================================================
//
//       Filename:  baseinfra/MarketAdapter/cme_market_book_arbitrator.hpp
//
//    Description:  Utility Functions for fetching the Sample Data values from the last 30 days
//
//        Version:  1.0
//        Created:  01/15/2016 12:38:39 PM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
//
//        Address:  Suite No 162, Evoma, #14, Bhattarhalli,
//                  Old Madras Road, Near Garden City College,
//                  KR Puram, Bangalore 560049, India
//          Phone:  +91 80 4190 3551
//
// =====================================================================================

#pragma once

#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CDef/random_channel.hpp"

#include "dvccode/CommonDataStructures/security_name_indexer.hpp"

#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/CommonTradeUtils/watch.hpp"
#include "dvccode/Utils/multicast_sender_socket.hpp"

#include "dvccode/ORSMessages/ors_message_listener.hpp"

#include "baseinfra/LoggedSources/shortcode_ors_message_filesource_map.hpp"

#include "baseinfra/MarketAdapter/cme_market_per_security.hpp"

#define MAX_ORDERS_TO_MONITOR 2048
#define MAX_CONF_EXEC_USECS_TO_CONSIDER 5000

namespace HFSAT {

struct CMEORSOrder {
  int64_t orderid;
  double price;
  int size;
  int saos;
  ttime_t seqd_time;
  ttime_t conf_time;
  ttime_t exec_time;

  // Information in String
  std::string ToString() {
    std::stringstream st;
    st << "OrderID: " << orderid << " SAOS: " << saos << " Price: " << price << " size: " << size
       << " Seqd: " << seqd_time.ToString() << " Conf: " << conf_time.ToString() << " Exec: " << exec_time.ToString()
       << " Conf-Exec: " << (exec_time - conf_time).ToString();
    return st.str();
  }

  CMEORSOrder(int64_t t_orderid, int t_saos, double t_price, int t_size)
      : orderid(t_orderid), price(t_price), size(t_size), saos(t_saos) {}
};

class CMEMarketBookArbitrator : public OrderExecutedListener,
                                public OrderConfirmedListener,
                                public BaseMarketOrderManager,
                                public OrderLevelListener {
  /**
   * Class to listen to ors messages for CME (and market-data) and estimate of following events
   * a ) Level on one side being cleared
   * b ) trade on one side
   */
 public:
  virtual ~CMEMarketBookArbitrator() {}
  CMEMarketBookArbitrator(DebugLogger& dbglogger, Watch& watch, SecurityNameIndexer& sec_name_indexer,
                          const std::vector<HFSAT::MarketOrdersView*>& sid_to_mov_ptr_map);

  void OrderExecuted(const int t_server_assigned_client_id, const int client_assigned_order_sequence,
                     const int t_server_assigned_order_sequence, const unsigned int security_id, const double price,
                     const TradeType_t r_buysell, const int size_remaining, const int size_executed,
                     const int client_position, const int global_position, const int r_int_price,
                     const int32_t t_server_assigned_message_sequence, const uint64_t exchange_order_id,
                     const ttime_t time_set_by_server);

  void OrderConfirmed(const int t_server_assigned_client_id, const int t_client_assigned_order_sequence,
                      const int t_server_assigned_order_sequence, const unsigned int t_security_id, const double price,
                      const TradeType_t r_buysell, const int size_remaining, const int size_executed,
                      const int _client_position, const int global_position, const int r_int_price,
                      const int32_t t_server_assigned_message_sequence, const uint64_t exchange_order_id,
                      const ttime_t time_set_by_server);

  void OrderORSConfirmed(const int t_server_assigned_client_id, const int t_client_assigned_order_sequence,
                         const int t_server_assigned_order_sequence, const unsigned int security_id, const double price,
                         const TradeType_t r_buysell, const int size_remaining, const int size_executed,
                         const int r_int_price, const int32_t server_assigned_message_sequence,
                         const uint64_t exchange_order_id, const ttime_t time_set_by_server) {}

  /// Market data
  ///
  void SetTimeToSkipUntilFirstEvent(const ttime_t start_time) {}

  void OnOrderAdd(const uint32_t security_id, const TradeType_t buysell, const uint64_t order_id,
                  const uint32_t priority, const double price, const uint32_t size);

  void OnOrderModify(const uint32_t security_id, const TradeType_t buysell, const uint64_t order_id,
                     const uint32_t priority, const double price, const uint32_t size);

  void OnVolumeDelete(const uint32_t security_id, const TradeType_t buysell, const uint64_t order_id,
                      const uint32_t size);

  void OnOrderDelete(const uint32_t security_id, const TradeType_t buysell, const uint64_t order_id);

  void OnOrderExec(const uint32_t security_id, const TradeType_t buysell, const uint64_t order_id,
                   const double exec_price, const uint32_t size_exec, const uint32_t size_remaining);

  void OnOrderExecWithPx(const uint32_t security_id, const uint64_t bid_order_id, const uint64_t ask_order_id,
                         const double exec_price, const uint32_t size_exec, const uint32_t bid_remaining,
                         const uint32_t ask_remaining);

  void OnOrderSpExec(const uint32_t security_id, const TradeType_t buysell, const uint64_t order_id,
                     const double exec_price, const uint32_t size_exec, const uint32_t size_remaining);

  void ResetBook(const unsigned int security_id);

  void OnResetBegin(const unsigned int security_id) {}

  void OnResetEnd(const unsigned int security_id) {}

  void SetMultiSenderSocket(HFSAT::MulticastSenderSocket* mcast_sock);

 protected:
 private:
  void LoadLiveSourceProductCodes(std::map<std::string, int>& shc_to_code_map, const char* file_name);
  bool PopulateCMELSCommonStruct(const int t_server_assigned_client_id, const int client_assigned_order_sequence,
                                 const int t_server_assigned_order_sequence, const unsigned int security_id,
                                 const double price, const TradeType_t r_buysell, const int size_remaining,
                                 const int size_executed, const int client_position, const int global_position,
                                 const int r_int_price, const int32_t t_server_assigned_message_sequence,
                                 const uint64_t exchange_order_id, const ttime_t time_set_by_server);

  DebugLogger& dbglogger_;
  Watch& watch_;

  // TODO change it from map to vector
  std::map<uint64_t, CMEORSOrder*> saos_order_map_;
  std::map<int, uint64_t> saos_orderid_map_;

  std::map<uint64_t, ttime_t> mkt_bid_order_time_map_;
  std::map<uint64_t, ttime_t> mkt_ask_order_time_map_;
  SimpleMempool<CMEOrder> cme_order_mempool_;
  std::vector<CMEMarketPerSecurity*> cme_markets_;

  const std::vector<MarketOrdersView*> market_orders_view_map_;

  HFSAT::MulticastSenderSocket* multicast_sender_socket_;
  std::map<std::string, int> cme_ls_shc_to_code_map_;
  CME_MDS::CMELSCommonStruct cme_ls_data_;  // Struct to multicast arb info
};
}
