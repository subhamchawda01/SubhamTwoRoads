#include "dvccode/CDef/defines.hpp"
#include "basetrade/BTUtils/ors_order_listener_thread.hpp"

namespace HFSAT {

ORSOrderListenerThread::ORSOrderListenerThread(HFSAT::DebugLogger& dbglogger, HFSAT::Watch& watch)
    : fd_to_buffer_(),
      fd_to_read_offset_(),
      sec_name_indexer_(HFSAT::SecurityNameIndexer::GetUniqueInstance()),
      order_lock_(),
      watch_(watch),
      reply_fd_(-1) {
  tcp_server_manager_ = new HFSAT::Utils::TCPServerManager(ORS_LISTENER_PORT, dbglogger);
  tcp_server_manager_->SubscribeForUpdates(this);
  queue_ = new boost::lockfree::queue<HFSAT::ORS::Order>(1000);
  plsmm_map_ = nullptr;
}

void ORSOrderListenerThread::SubscribeSMV(SecurityMarketView& smv) {
  smv.subscribe_price_type(this, HFSAT::kPriceTypeMidprice);
}

void ORSOrderListenerThread::Run() { tcp_server_manager_->run(); }

void ORSOrderListenerThread::OnMarketUpdate(const unsigned int _security_id_,
                                            const MarketUpdateInfo& _market_update_info_) {
  int i = 0;

  while (!queue_->empty()) {
    HFSAT::ORS::Order temp_order;
    queue_->pop(temp_order);

    std::cout << "Market : " << watch_.tv().tv_sec << "." << watch_.tv().tv_usec << " "
              << _market_update_info_.ToString() << std::endl;

    std::cout << "Order: " << temp_order.toString() << std::endl;

    if (plsmm_map_ != nullptr) {
      int sec_id = sec_name_indexer_.GetIdFromSecname(temp_order.symbol_);

      if (sec_id < 0) {
        std::cout << "Ignoring order as sec_id < 0 , symbol :  " << temp_order.symbol_ << std::endl;
        i++;
        continue;
      }
      HFSAT::PriceLevelSimMarketMaker* plsmm = (*plsmm_map_)[sec_id];

      plsmm->SetSecId(sec_id);

      // Hack to show send order or cancel order
      if (temp_order.is_ioc) {
        plsmm->SendOrderExch(0, temp_order.symbol_, temp_order.buysell_, temp_order.price_, temp_order.size_remaining_,
                             temp_order.int_price_, temp_order.server_assigned_order_sequence_, false, false);

      } else {
        plsmm->CancelOrderExch(0, ors_saos_to_sim_saos_[temp_order.server_assigned_order_sequence_],
                               temp_order.buysell_, temp_order.int_price_);
      }

    } else {
      std::cout << "plsmm map is null" << std::endl;
    }

    i++;
    // Allowing max 10 orders to go through at once
    if (i > 10) {
      break;
    }
  }
}

void ORSOrderListenerThread::SetSimMarketMakerMap(std::map<int, HFSAT::PriceLevelSimMarketMaker*>* plsmm_map) {
  plsmm_map_ = plsmm_map;
}

void ORSOrderListenerThread::OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                                          const MarketUpdateInfo& _market_update_info_) {
  OnMarketUpdate(_security_id_, _market_update_info_);
}

void ORSOrderListenerThread::RunLiveShmSource() {
  bool keep_me_running_ = true;
  while (keep_me_running_) {
    // has to be volatile, waiting on shared memory segment queue position
    volatile int queue_position_ = *((volatile int*)(order_shm_struct_ + EXCH_SIM_MDS_QUEUE_SIZE));

    if (shm_index_ == queue_position_) {
      continue;
    }

    memcpy(&order_, (HFSAT::ORS::Order*)(order_shm_struct_ + (shm_index_ % EXCH_SIM_MDS_QUEUE_SIZE)),
           sizeof(HFSAT::ORS::Order));

    shm_index_++;

    order_lock_.LockMutex();

    if (reply_fd_ == -1) {
      reply_fd_ = tcp_server_manager_->GetClientFDList()[0];
    }

    ors_saos_to_client_fd_[order_.server_assigned_order_sequence_] = reply_fd_;
    ProcessOrder(order_);

    order_lock_.UnlockMutex();
  }
}

void ORSOrderListenerThread::OnClientRequest(int32_t client_fd, char* buffer, uint32_t const& length) {
  if (fd_to_read_offset_.find(client_fd) == fd_to_read_offset_.end()) {
    fd_to_read_offset_[client_fd] = 0;
    fd_to_buffer_[client_fd] = (char*)malloc(MAX_TCP_LEN);
  }

  memcpy(fd_to_buffer_[client_fd] + fd_to_read_offset_[client_fd], buffer, length);
  fd_to_read_offset_[client_fd] =
      ProcessORSInput(fd_to_buffer_[client_fd], fd_to_read_offset_[client_fd] + length, client_fd);
}

int ORSOrderListenerThread::ProcessORSInput(char const* simulator_reply, int32_t const packet_length,
                                            int32_t client_fd) {
  int32_t legnth_to_be_processed = packet_length;
  char const* msg_ptr = simulator_reply;

  while (legnth_to_be_processed > 0) {
    if ((uint32_t)legnth_to_be_processed < sizeof(HFSAT::ORS::Order)) {
      memmove((void*)simulator_reply, (void*)msg_ptr, legnth_to_be_processed);
      return legnth_to_be_processed;
    }

    order_lock_.LockMutex();

    // ProcessSingle Packet
    memcpy((void*)&order_, (void*)msg_ptr, sizeof(HFSAT::ORS::Order));
    msg_ptr += sizeof(HFSAT::ORS::Order);
    legnth_to_be_processed -= sizeof(HFSAT::ORS::Order);
    ors_saos_to_client_fd_[order_.server_assigned_order_sequence_] = client_fd;
    ProcessOrder(order_);

    order_lock_.UnlockMutex();
  }

  return 0;
}

void ORSOrderListenerThread::ProcessOrder(HFSAT::ORS::Order const& order) { queue_->push(order_); }

/**
 *
 * @param t_server_assigned_client_id_
 * @param _client_assigned_order_sequence_
 * @param _server_assigned_order_sequence_
 * @param _security_id_
 * @param r_buysell_
 * @param r_int_price_
 * @param server_assigned_message_sequence
 * @param exchange_order_id
 * @param time_set_by_server
 */
void ORSOrderListenerThread::OrderNotFound(const int t_server_assigned_client_id_,
                                           const int _client_assigned_order_sequence_,
                                           const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                                           const TradeType_t r_buysell_, const int r_int_price_,
                                           const int32_t server_assigned_message_sequence,
                                           const uint64_t exchange_order_id, const ttime_t time_set_by_server) {}

/**
 *
 * @param t_server_assigned_client_id_
 * @param _client_assigned_order_sequence_
 * @param _server_assigned_order_sequence_
 * @param _security_id_
 * @param _price_
 * @param r_buysell_
 * @param _size_remaining_
 * @param _size_executed_
 * @param _client_position_
 * @param _global_position_
 * @param r_int_price_
 * @param server_assigned_message_sequence
 * @param exchange_order_id
 * @param time_set_by_server
 */
void ORSOrderListenerThread::OrderSequenced(
    const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
    const int _server_assigned_order_sequence_, const unsigned int _security_id_, const double _price_,
    const TradeType_t r_buysell_, const int _size_remaining_, const int _size_executed_, const int _client_position_,
    const int _global_position_, const int r_int_price_, const int32_t server_assigned_message_sequence,
    const uint64_t exchange_order_id, const ttime_t time_set_by_server) {
  ors_saos_to_sim_saos_[_client_assigned_order_sequence_] = _server_assigned_order_sequence_;
  std::cout << "OrderSeqd" << std::endl;
}

/**
 *  \brief called by ORSMessageLiveSource when the messagetype is kORRType_Conf
 * @param _server_assigned_client_id_
 * @param _client_assigned_order_sequence_
 * @param _server_assigned_order_sequence_
 * @param _security_id_
 * @param _price_
 * @param _buysell_
 * @param _size_remaining_
 * @param _size_executed_
 * @param _client_position_
 * @param _global_position_
 * @param _int_price_
 * @param server_assigned_message_sequence
 * @param exchange_order_id
 * @param time_set_by_server
 */
void ORSOrderListenerThread::OrderConfirmed(
    const int _server_assigned_client_id_, const int _client_assigned_order_sequence_,
    const int _server_assigned_order_sequence_, const unsigned int _security_id_, const double _price_,
    const TradeType_t _buysell_, const int _size_remaining_, const int _size_executed_, const int _client_position_,
    const int _global_position_, const int _int_price_, const int32_t server_assigned_message_sequence,
    const uint64_t exchange_order_id, const ttime_t time_set_by_server) {
  std::cout << "OrderConf" << std::endl;

  memcpy(response_.symbol, sec_name_indexer_.GetSecurityNameFromId(_security_id_), kSecNameLen);
  response_.event = kORRType_Conf;
  response_.buysell = _buysell_;
  response_.price = _price_;
  response_.saos = _client_assigned_order_sequence_;
  response_.size_remaining = _size_remaining_;
  response_.size_executed = 0;

  tcp_server_manager_->RespondToClient(ors_saos_to_client_fd_[_client_assigned_order_sequence_], (char*)&response_,
                                       sizeof(response_));
}

void ORSOrderListenerThread::OrderORSConfirmed(const int t_server_assigned_client_id_,
                                               const int _client_assigned_order_sequence_,
                                               const int _server_assigned_order_sequence_,
                                               const unsigned int _security_id_, const double _price_,
                                               const TradeType_t r_buysell_, const int _size_remaining_,
                                               const int _size_executed_, const int r_int_price_,
                                               const int32_t server_assigned_message_sequence,
                                               const uint64_t exchange_order_id, const ttime_t time_set_by_server) {}

/** \brief called by ORSMessageLiveSource when the messagetype is kORRType_CxRe */
void ORSOrderListenerThread::OrderConfCxlReplaced(
    const int _server_assigned_client_id_, const int _client_assigned_order_sequence_,
    const int _server_assigned_order_sequence_, const unsigned int _security_id_, const double _price_,
    const TradeType_t _buysell_, const int _size_remaining_, const int _size_executed_, const int _client_position_,
    const int _global_position_, const int _int_price_, const int32_t server_assigned_message_sequence,
    const uint64_t exchange_order_id, const ttime_t time_set_by_server) {}

void ORSOrderListenerThread::OrderCxlSequenced(const int t_server_assigned_client_id_,
                                               const int _client_assigned_order_sequence_,
                                               const int _server_assigned_order_sequence_,
                                               const unsigned int _security_id_, const double _price_,
                                               const TradeType_t r_buysell_, const int _size_remaining_,
                                               const int _client_position_, const int _global_position_,
                                               const int r_int_price_, const int32_t server_assigned_message_sequence,
                                               const uint64_t exchange_order_id, const ttime_t time_set_by_server) {}

/** \brief called by ORSMessageLiveSource when the messagetype is kORRType_Cxld */
void ORSOrderListenerThread::OrderCanceled(const int _server_assigned_client_id_,
                                           const int _client_assigned_order_sequence_,
                                           const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                                           const double _price_, const TradeType_t _buysell_,
                                           const int _size_remaining_, const int _client_position_,
                                           const int _global_position_, const int _int_price_,
                                           const int32_t server_assigned_message_sequence,
                                           const uint64_t exchange_order_id, const ttime_t time_set_by_server) {
  std::cout << "OrderCxld" << std::endl;

  memcpy(response_.symbol, sec_name_indexer_.GetSecurityNameFromId(_security_id_), kSecNameLen);
  response_.event = kORRType_Cxld;
  response_.buysell = _buysell_;
  response_.price = _price_;
  response_.saos = _client_assigned_order_sequence_;
  response_.size_remaining = 0;
  response_.size_executed = 0;

  tcp_server_manager_->RespondToClient(ors_saos_to_client_fd_[_client_assigned_order_sequence_], (char*)&response_,
                                       sizeof(response_));
}

/** \brief called by ORSMessageLiveSource when the messagetype is kORRType_CxlRejc */
void ORSOrderListenerThread::OrderCancelRejected(const int t_server_assigned_client_id_,
                                                 const int _client_assigned_order_sequence_,
                                                 const int _server_assigned_order_sequence_,
                                                 const unsigned int _security_id_, const double _price_,
                                                 const TradeType_t t_buysell_, const int _size_remaining_,
                                                 const int _rejection_reason_, const int t_client_position_,
                                                 const int t_global_position_, const int r_int_price_,
                                                 const uint64_t exchange_order_id, const ttime_t time_set_by_server) {
  std::cout << "OrderCxlRej" << std::endl;

  memcpy(response_.symbol, sec_name_indexer_.GetSecurityNameFromId(_security_id_), kSecNameLen);
  response_.event = kORRType_CxlRejc;
  response_.buysell = t_buysell_;
  response_.price = _price_;
  response_.saos = _client_assigned_order_sequence_;
  response_.size_remaining = 0;
  response_.size_executed = 0;

  tcp_server_manager_->RespondToClient(ors_saos_to_client_fd_[_client_assigned_order_sequence_], (char*)&response_,
                                       sizeof(response_));
}

/** \brief called by ORSMessageLiveSource when the messagetype is kORRType_Exec */
void ORSOrderListenerThread::OrderExecuted(const int _server_assigned_client_id_,
                                           const int _client_assigned_order_sequence_,
                                           const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                                           const double _price_, const TradeType_t _buysell_,
                                           const int _size_remaining_, const int _size_executed_,
                                           const int _client_position_, const int _global_position_,
                                           const int _int_price_, const int32_t server_assigned_message_sequence,
                                           const uint64_t exchange_order_id, const ttime_t time_set_by_server) {
  std::cout << "OrderExecuted" << std::endl;

  memcpy(response_.symbol, sec_name_indexer_.GetSecurityNameFromId(_security_id_), kSecNameLen);
  response_.event = kORRType_Exec;
  response_.buysell = _buysell_;
  response_.price = _price_;
  response_.saos = _client_assigned_order_sequence_;
  response_.size_remaining = _size_remaining_;
  response_.size_executed = _size_executed_;

  std::cout << response_.ToString() << std::endl;
  tcp_server_manager_->RespondToClient(ors_saos_to_client_fd_[_client_assigned_order_sequence_], (char*)&response_,
                                       sizeof(response_));
}

/// \brief called by ORSMessageLiveSource when the messagetype is kORRType_Rejc
void ORSOrderListenerThread::ORSOrderListenerThread::OrderRejected(
    const int _server_assigned_client_id_, const int _client_assigned_order_sequence_, const unsigned int _security_id_,
    const double _price_, const TradeType_t _buysell_, const int _size_remaining_, const int _rejection_reason_,
    const int _int_price_, const uint64_t exchange_order_id, const ttime_t time_set_by_server) {}

void ORSOrderListenerThread::OrderInternallyMatched(
    const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
    const int _server_assigned_order_sequence_, const unsigned int _security_id_, const double _price_,
    const TradeType_t r_buysell_, const int _size_remaining_, const int _size_executed_, const int _client_position_,
    const int _global_position_, const int r_int_price_, const int32_t server_assigned_message_sequence,
    const uint64_t exchange_order_id, const ttime_t time_set_by_server) {}
}
