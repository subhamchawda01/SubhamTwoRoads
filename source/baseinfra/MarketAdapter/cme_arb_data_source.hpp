#pragma once

#include "baseinfra/MDSMessages/order_message_listeners.hpp"
#include "baseinfra/MarketAdapter/cme_market_book_arbitrator.hpp"

#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/Utils/multicast_receiver_socket.hpp"
#include "dvccode/ExternalData/simple_external_data_live_listener.hpp"
#include "dvccode/CommonTradeUtils/watch.hpp"
#include "dvccode/ORSMessages/ors_message_listener.hpp"

#define MAX_LIVE_SOCKETS_FD 1024

namespace HFSAT {

typedef enum { kArbDataSrcCMEMktData = 0, kArbDataSrcORSReplyLive, kArbDataSrcMAX } CMEArbDataSourceType_t;

class CMEArbDataSource : public SimpleExternalDataLiveListener {
 public:
  CMEArbDataSource(DebugLogger& _dbglogger_, HFSAT::SecurityNameIndexer& _sec_name_indexer_, Watch& watch,
                   const std::vector<HFSAT::MarketOrdersView*>& sid_to_mov_ptr_map);
  ~CMEArbDataSource();

  void SetDataSourceSockets();
  void GetDataSourceSocketsFdList(std::vector<int32_t>& socket_fd_vec);

  void AddOrderLevelListener(OrderLevelListener* listener);
  void AddOrderConfirmedListener(OrderConfirmedListener* listener);
  void AddOrderExecutedListener(OrderExecutedListener* listener);

  void NotifyOrderAdd(const uint32_t security_id, const TradeType_t buysell, const uint64_t order_id,
                      const uint32_t priority, const double price, const uint32_t size);
  void NotifyOrderModify(const uint32_t security_id, const TradeType_t buysell, const uint64_t order_id,
                         const uint32_t priority, const double price, const uint32_t size);

  void NotifyOrderDelete(const uint32_t security_id, const TradeType_t buysell, const uint64_t order_id);
  void NotifyOrderExec(const uint32_t security_id, const TradeType_t buysell, const uint64_t order_id,
                       const double exec_price, const uint32_t size_exec, const uint32_t size_remaining);

  void ProcessCMEOBFStruct();
  void ProcessORSReplyStruct();

  /* override functions */
  void ProcessAllEvents(int this_socket_fd_) override;
  void CleanUp() override;

 private:
  TradeType_t GetTradeType(const char type);

  DebugLogger& dbglogger_;
  Watch& watch_;
  const HFSAT::SecurityNameIndexer& sec_name_indexer_;
  HFSAT::CMEMarketBookArbitrator* cme_book_arbitrator_;

  std::vector<HFSAT::MulticastReceiverSocket*> multicast_receiver_socket_vec_;  // Multicast Socket Reader
  HFSAT::MulticastSenderSocket* multicast_sender_socket_;                       // Multicast Socket Sender
  std::vector<int32_t> socket_fd_to_actual_socket_index_map_;
  std::vector<HFSAT::CMEArbDataSourceType_t> socket_index_to_data_source_type_map_;
  HFSAT::GenericORSReplyStructLive orsreply_;  // ORS reply struct
  CME_MDS::CMEOBFCommonStruct cstr_obf_;       // CME Order Feed struct
  const int32_t orsreply_pkt_size_;            // So That we don't keep calling sizeof
  const int32_t cstr_obf_pkt_size_;            // So That we don't keep calling sizeof
  int32_t socket_read_size_;

  std::vector<OrderLevelListener*> order_listener_vec_;
  std::vector<OrderConfirmedListener*> order_confirmed_listener_vec_;
  std::vector<OrderExecutedListener*> order_executed_listener_vec_;
};
}
