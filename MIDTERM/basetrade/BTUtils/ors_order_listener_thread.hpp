#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <map>
#include <signal.h>

#include <boost/lockfree/queue.hpp>

#include "baseinfra/MarketAdapter/security_market_view_change_listener.hpp"
#include "dvccode/Utils/thread.hpp"
#include "dvccode/CDef/basic_ors_defines.hpp"

#include "dvccode/Utils/thread.hpp"

#include "dvccode/CDef/order.hpp"

#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

#include "baseinfra/BaseTrader/sim_trader_helper.hpp"
#include "baseinfra/SimMarketMaker/sim_market_maker_list.hpp"
#include "baseinfra/SimMarketMaker/sim_market_maker_helper.hpp"

#include "baseinfra/MarketAdapter/indexed_hkomd_price_level_market_view_manager.hpp"
#include "dvccode/ExternalData/historical_dispatcher.hpp"
#include "dvccode/ExternalData/simple_live_dispatcher.hpp"

#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/CommonTradeUtils/watch.hpp"
#include "dvccode/CommonTradeUtils/economic_events_manager.hpp"
#include "dvccode/TradingInfo/network_account_info_manager.hpp"
#include "baseinfra/VolatileTradingInfo/shortcode_ezone_vec.hpp"

#include "dvccode/ORSMessages/ors_message_listener.hpp"
#include "dvccode/ORSMessages/ors_message_livesource.hpp"
#include "dvccode/CDef/ors_messages.hpp"
#include "baseinfra/OrderRouting/prom_order_manager.hpp"
#include "baseinfra/OrderRouting/base_order_manager.hpp"
#include "baseinfra/SmartOrderRouting/prom_pnl.hpp"

#include "baseinfra/MarketAdapter/market_adapter_list.hpp"
#include "baseinfra/MDSMessages/combined_mds_messages_shm_processor.hpp"
#include "baseinfra/MDSMessages/exch_sim_shm_base.hpp"

#include "dvccode/Utils/send_alert.hpp"
#include "dvccode/CDef/email_utils.hpp"
#include "dvccode/Utils/shortcode_request_helper.hpp"
#include "dvccode/Utils/tcp_server_manager.hpp"
#include "dvccode/Utils/tcp_server_socket_listener.hpp"

#define ORS_LISTENER_PORT 9292
#define MAX_TCP_LEN 32000

namespace HFSAT {
class ORSOrderListenerThread : public SecurityMarketViewChangeListener,
                               public OrderNotFoundListener,
                               public OrderSequencedListener,
                               public OrderConfirmedListener,
                               public OrderConfCxlReplacedListener,
                               public OrderCanceledListener,
                               public OrderExecutedListener,
                               public OrderRejectedListener,
                               public OrderCxlSeqdListener,
                               public OrderInternallyMatchedListener,
                               public HFSAT::Utils::TCPServerSocketListener,
                               public HFSAT::ExchSimShmBase,
                               public Thread {
  std::map<int, char*> fd_to_buffer_;
  std::map<int, int> fd_to_read_offset_;
  boost::lockfree::queue<HFSAT::ORS::Order>* queue_;
  std::map<int, HFSAT::PriceLevelSimMarketMaker*>* plsmm_map_;
  HFSAT::ORS::Order order_;
  HFSAT::ORS::ExchSimResponseStruct response_;
  std::map<int, int> ors_saos_to_sim_saos_;
  std::map<int, int> ors_saos_to_client_fd_;  // Change it to meta data if more mapping from saos to x is required
  HFSAT::SecurityNameIndexer& sec_name_indexer_;
  HFSAT::Utils::TCPServerManager* tcp_server_manager_;
  HFSAT::Lock order_lock_;
  HFSAT::Watch& watch_;
  int reply_fd_;

 public:
  ORSOrderListenerThread(HFSAT::DebugLogger& dbglogger, HFSAT::Watch& watch);

  void Run();

  void thread_main() { RunLiveShmSource(); }

  void RunLiveShmSource() override;

  void SubscribeSMV(SecurityMarketView& smv);

  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);
  void SetSimMarketMakerMap(std::map<int, HFSAT::PriceLevelSimMarketMaker*>* plsmm_map);

  void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                    const MarketUpdateInfo& _market_update_info_);

  void OnClientRequest(int32_t client_fd, char* buffer, uint32_t const& length);

  int ProcessORSInput(char const* simulator_reply, int32_t const packet_length, int32_t client_fd);
  void ProcessOrder(HFSAT::ORS::Order const& order);

  void OrderNotFound(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                     const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                     const TradeType_t r_buysell_, const int r_int_price_,
                     const int32_t server_assigned_message_sequence, const uint64_t exchange_order_id,
                     const ttime_t time_set_by_server);
  void OrderSequenced(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                      const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                      const double _price_, const TradeType_t r_buysell_, const int _size_remaining_,
                      const int _size_executed_, const int _client_position_, const int _global_position_,
                      const int r_int_price_, const int32_t server_assigned_message_sequence,
                      const uint64_t exchange_order_id, const ttime_t time_set_by_server);

  /** \brief called by ORSMessageLiveSource when the messagetype is kORRType_Conf */
  void OrderConfirmed(const int _server_assigned_client_id_, const int _client_assigned_order_sequence_,
                      const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                      const double _price_, const TradeType_t _buysell_, const int _size_remaining_,
                      const int _size_executed_, const int _client_position_, const int _global_position_,
                      const int _int_price_, const int32_t server_assigned_message_sequence,
                      const uint64_t exchange_order_id, const ttime_t time_set_by_server);

  void OrderORSConfirmed(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                         const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                         const double _price_, const TradeType_t r_buysell_, const int _size_remaining_,
                         const int _size_executed_, const int r_int_price_,
                         const int32_t server_assigned_message_sequence, const uint64_t exchange_order_id,
                         const ttime_t time_set_by_server);

  /** \brief called by ORSMessageLiveSource when the messagetype is kORRType_CxRe */
  void OrderConfCxlReplaced(const int _server_assigned_client_id_, const int _client_assigned_order_sequence_,
                            const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                            const double _price_, const TradeType_t _buysell_, const int _size_remaining_,
                            const int _size_executed_, const int _client_position_, const int _global_position_,
                            const int _int_price_, const int32_t server_assigned_message_sequence,
                            const uint64_t exchange_order_id, const ttime_t time_set_by_server);

  void OrderCxlSequenced(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                         const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                         const double _price_, const TradeType_t r_buysell_, const int _size_remaining_,
                         const int _client_position_, const int _global_position_, const int r_int_price_,
                         const int32_t server_assigned_message_sequence, const uint64_t exchange_order_id,
                         const ttime_t time_set_by_server);

  /** \brief called by ORSMessageLiveSource when the messagetype is kORRType_Cxld */
  void OrderCanceled(const int _server_assigned_client_id_, const int _client_assigned_order_sequence_,
                     const int _server_assigned_order_sequence_, const unsigned int _security_id_, const double _price_,
                     const TradeType_t _buysell_, const int _size_remaining_, const int _client_position_,
                     const int _global_position_, const int _int_price_, const int32_t server_assigned_message_sequence,
                     const uint64_t exchange_order_id, const ttime_t time_set_by_server);

  /** \brief called by ORSMessageLiveSource when the messagetype is kORRType_CxlRejc */
  void OrderCancelRejected(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                           const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                           const double _price_, const TradeType_t t_buysell_, const int _size_remaining_,
                           const int _rejection_reason_, const int t_client_position_, const int t_global_position_,
                           const int r_int_price_, const uint64_t exchange_order_id, const ttime_t time_set_by_server);

  /** \brief called by ORSMessageLiveSource when the messagetype is kORRType_Exec */
  void OrderExecuted(const int _server_assigned_client_id_, const int _client_assigned_order_sequence_,
                     const int _server_assigned_order_sequence_, const unsigned int _security_id_, const double _price_,
                     const TradeType_t _buysell_, const int _size_remaining_, const int _size_executed_,
                     const int _client_position_, const int _global_position_, const int _int_price_,
                     const int32_t server_assigned_message_sequence, const uint64_t exchange_order_id,
                     const ttime_t time_set_by_server);

  /// \brief called by ORSMessageLiveSource when the messagetype is kORRType_Rejc
  void OrderRejected(const int _server_assigned_client_id_, const int _client_assigned_order_sequence_,
                     const unsigned int _security_id_, const double _price_, const TradeType_t _buysell_,
                     const int _size_remaining_, const int _rejection_reason_, const int _int_price_,
                     const uint64_t exchange_order_id, const ttime_t time_set_by_server);

  void OrderInternallyMatched(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                              const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                              const double _price_, const TradeType_t r_buysell_, const int _size_remaining_,
                              const int _size_executed_, const int _client_position_, const int _global_position_,
                              const int r_int_price_, const int32_t server_assigned_message_sequence,
                              const uint64_t exchange_order_id, const ttime_t time_set_by_server);
};
}
