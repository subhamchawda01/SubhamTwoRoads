#include "baseinfra/MarketAdapter/cme_arb_data_source.hpp"
#include "dvccode/TradingInfo/network_account_info_manager.hpp"
#include "dvccode/TradingInfo/network_account_interface_manager.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"

namespace HFSAT {

CMEArbDataSource::CMEArbDataSource(DebugLogger& dbglogger, SecurityNameIndexer& sec_name_indexer, Watch& watch,
                                   const std::vector<HFSAT::MarketOrdersView*>& sid_to_mov_ptr_map)
    : dbglogger_(dbglogger),
      watch_(watch),
      sec_name_indexer_(sec_name_indexer),
      orsreply_pkt_size_(sizeof(HFSAT::GenericORSReplyStructLive)),
      cstr_obf_pkt_size_(sizeof(CME_MDS::CMEOBFCommonStruct)),
      socket_read_size_(-1) {
  cme_book_arbitrator_ = new HFSAT::CMEMarketBookArbitrator(dbglogger, watch, sec_name_indexer, sid_to_mov_ptr_map);
  socket_fd_to_actual_socket_index_map_.resize(MAX_LIVE_SOCKETS_FD, -1);

  AddOrderLevelListener(cme_book_arbitrator_);
  AddOrderConfirmedListener(cme_book_arbitrator_);
  AddOrderExecutedListener(cme_book_arbitrator_);
}

CMEArbDataSource::~CMEArbDataSource() {}

void CMEArbDataSource::SetDataSourceSockets() {
  // v1: We will add sockets only for CME Order Feed and ORS Reply Live socket
  HFSAT::NetworkAccountInfoManager network_account_info_manager_;

  // Set CME Order Feed sockets
  HFSAT::DataInfo cme_data_info = network_account_info_manager_.GetSrcDataInfo("CMEOBF");
  HFSAT::MulticastReceiverSocket* cme_multicast_receiver_socket = new HFSAT::MulticastReceiverSocket(
      cme_data_info.bcast_ip_, cme_data_info.bcast_port_,
      HFSAT::NetworkAccountInterfaceManager::instance().GetInterface(HFSAT::kExchSourceCME, HFSAT::k_MktDataMcast));
  std::cout << "Listening to CME data at : " << cme_data_info.bcast_ip_ << " X " << cme_data_info.bcast_port_
            << std::endl;

  // Set ORS Reply live sockets
  HFSAT::DataInfo ors_data_info = network_account_info_manager_.GetDepDataInfo(HFSAT::kExchSourceCME, "ZN_0");
  HFSAT::MulticastReceiverSocket* ors_multicast_receiver_socket = new HFSAT::MulticastReceiverSocket(
      ors_data_info.bcast_ip_, ors_data_info.bcast_port_,
      HFSAT::NetworkAccountInterfaceManager::instance().GetInterfaceForApp(HFSAT::k_ORS_LS));
  std::cout << "Listening to ORS data at : " << ors_data_info.bcast_ip_ << " X " << ors_data_info.bcast_port_
            << std::endl;

  // Populate maps
  // Check If the FD -> index of the socket can be stored directly or we have to increase the capacitiy
  // <= is imporant since we are not utilising the [ 0 ] element, FD can't be zero,
  for (int32_t start_ptr = socket_fd_to_actual_socket_index_map_.size();
       start_ptr <= (std::max(cme_multicast_receiver_socket->socket_file_descriptor(),
                              ors_multicast_receiver_socket->socket_file_descriptor()));
       start_ptr++) {
    socket_fd_to_actual_socket_index_map_.push_back(-1);
  }

  // Add CME data src to map
  multicast_receiver_socket_vec_.push_back(cme_multicast_receiver_socket);
  socket_index_to_data_source_type_map_.push_back(HFSAT::CMEArbDataSourceType_t::kArbDataSrcCMEMktData);
  socket_fd_to_actual_socket_index_map_[cme_multicast_receiver_socket->socket_file_descriptor()] =
      (multicast_receiver_socket_vec_.size() - 1);

  // ADD ORS data src to map
  multicast_receiver_socket_vec_.push_back(ors_multicast_receiver_socket);
  socket_index_to_data_source_type_map_.push_back(HFSAT::CMEArbDataSourceType_t::kArbDataSrcORSReplyLive);
  socket_fd_to_actual_socket_index_map_[ors_multicast_receiver_socket->socket_file_descriptor()] =
      (multicast_receiver_socket_vec_.size() - 1);

  // Setting up the mcast sender sockets
  HFSAT::DataInfo arb_data_info = network_account_info_manager_.GetSrcDataInfo("ARBINFO");
  multicast_sender_socket_ = new HFSAT::MulticastSenderSocket(
      arb_data_info.bcast_ip_, arb_data_info.bcast_port_,
      HFSAT::NetworkAccountInterfaceManager::instance().GetInterface(HFSAT::kExchSourceCME, HFSAT::k_MktDataMcast));
  cme_book_arbitrator_->SetMultiSenderSocket(multicast_sender_socket_);
  std::cout << "Multicasting arbitrage info at : " << arb_data_info.bcast_ip_ << " X " << arb_data_info.bcast_port_
            << std::endl;
}

// Fill Up All the socket fd so the caller can add the same to it's event dispatch loop
void CMEArbDataSource::GetDataSourceSocketsFdList(std::vector<int32_t>& socket_fd_vec) {
  for (uint32_t socket_counter = 0; socket_counter < multicast_receiver_socket_vec_.size(); socket_counter++) {
    socket_fd_vec.push_back(multicast_receiver_socket_vec_[socket_counter]->socket_file_descriptor());
    multicast_receiver_socket_vec_[socket_counter]->SetNonBlocking();
  }
}

void CMEArbDataSource::AddOrderLevelListener(OrderLevelListener* listener) {
  if (listener != nullptr) {
    VectorUtils::UniqueVectorAdd(order_listener_vec_, listener);
  }
}

void CMEArbDataSource::AddOrderConfirmedListener(OrderConfirmedListener* listener) {
  if (listener != nullptr) {
    VectorUtils::UniqueVectorAdd(order_confirmed_listener_vec_, listener);
  }
}

void CMEArbDataSource::AddOrderExecutedListener(OrderExecutedListener* listener) {
  if (listener != nullptr) {
    VectorUtils::UniqueVectorAdd(order_executed_listener_vec_, listener);
  }
}

void CMEArbDataSource::NotifyOrderAdd(const uint32_t security_id, const TradeType_t buysell, const uint64_t order_id,
                                      const uint32_t priority, const double price, const uint32_t size) {
  for (auto listener : order_listener_vec_) {
    listener->OnOrderAdd(security_id, buysell, order_id, priority, price, size);
  }
}

void CMEArbDataSource::NotifyOrderModify(const uint32_t security_id, const TradeType_t buysell, const uint64_t order_id,
                                         const uint32_t priority, const double price, const uint32_t size) {
  for (auto listener : order_listener_vec_) {
    listener->OnOrderModify(security_id, buysell, order_id, priority, price, size);
  }
}

void CMEArbDataSource::NotifyOrderDelete(const uint32_t security_id, const TradeType_t buysell,
                                         const uint64_t order_id) {
  for (auto listener : order_listener_vec_) {
    listener->OnOrderDelete(security_id, buysell, order_id);
  }
}

void CMEArbDataSource::NotifyOrderExec(const uint32_t security_id, const TradeType_t buysell, const uint64_t order_id,
                                       const double exec_price, const uint32_t size_exec,
                                       const uint32_t size_remaining) {
  for (auto listener : order_listener_vec_) {
    listener->OnOrderExec(security_id, buysell, order_id, exec_price, size_exec, size_remaining);
  }
}

// Received CME Order Feed struct
// We will do respective callbacks depending upon the message
void CMEArbDataSource::ProcessCMEOBFStruct() {
  // p_time_keeper_->OnTimeReceived(cstr_obf_timestamp_);
  watch_.OnTimeReceived(cstr_obf_.time_);

  const int security_id = sec_name_indexer_.GetIdFromSecname(cstr_obf_.getContract());
  if (security_id < 0) return;

  switch (cstr_obf_.msg_) {
    case CME_MDS::CME_DELTA: {
      switch (cstr_obf_.data_.cme_dels_.action) {
        case '0': {
          NotifyOrderAdd(security_id, GetTradeType(cstr_obf_.data_.cme_dels_.type), cstr_obf_.data_.cme_dels_.order_id,
                         cstr_obf_.data_.cme_dels_.order_priority, cstr_obf_.data_.cme_dels_.price,
                         cstr_obf_.data_.cme_dels_.order_qty);

        } break;
        case '1': {
          NotifyOrderModify(security_id, GetTradeType(cstr_obf_.data_.cme_dels_.type),
                            cstr_obf_.data_.cme_dels_.order_id, cstr_obf_.data_.cme_dels_.order_priority,
                            cstr_obf_.data_.cme_dels_.price, cstr_obf_.data_.cme_dels_.order_qty);

        } break;
        case '2': {
          NotifyOrderDelete(security_id, GetTradeType(cstr_obf_.data_.cme_dels_.type),
                            cstr_obf_.data_.cme_dels_.order_id);
        } break;
        default: {
          fprintf(stderr, "Unknown Delta message type in CME OBF Processor ::flushQuoteQueue CME_DELTA  \n");
        } break;
      }

    } break;
    case CME_MDS::CME_TRADE: {
      // Legacy usage for OF.
      // Replaced by CME_EXEC in newer data feed. But kept in place to be used for older data.
      NotifyOrderExec(security_id, GetTradeType(cstr_obf_.data_.cme_trds_.agg_side_), 0,
                      cstr_obf_.data_.cme_trds_.trd_px_, cstr_obf_.data_.cme_trds_.trd_qty_,
                      cstr_obf_.data_.cme_trds_.tot_qty_ - cstr_obf_.data_.cme_trds_.trd_qty_);
    } break;
    case CME_MDS::CME_EXEC: {
      // Since exec messages don't have the following information
      // Intentionally specifying default values for:
      //  - TradeType: No info
      //  - trade_price: No info/0
      //  - Remaining trade size: No info/0
      // Listeners should use their own logic to process events for other information.
      NotifyOrderExec(security_id, kTradeTypeNoInfo, cstr_obf_.data_.cme_excs_.order_id, 0,
                      cstr_obf_.data_.cme_excs_.order_qty, 0);
    } break;
    default: { std::cerr << "Unknown msg_type in CME: " << cstr_obf_.msg_ << std::endl; } break;
  }
}

void CMEArbDataSource::ProcessORSReplyStruct() {
  // first alert Watch -- now removed
  // p_time_keeper_->OnTimeReceived(orsreply_timestamp_);
  watch_.OnTimeReceived(orsreply_.time_set_by_server_);

  const int security_id = sec_name_indexer_.GetIdFromSecname(orsreply_.symbol_);
  if (security_id < 0) return;

  switch (orsreply_.orr_type_) {
    case kORRType_None: {
    } break;
    case kORRType_Seqd: {
    } break;
    case kORRType_Conf: {
      for (size_t i = 0; i < order_confirmed_listener_vec_.size(); i++) {
        order_confirmed_listener_vec_[i]->OrderConfirmed(
            orsreply_.server_assigned_client_id_, orsreply_.client_assigned_order_sequence_,
            orsreply_.server_assigned_order_sequence_, security_id, orsreply_.price_, orsreply_.buysell_,
            orsreply_.size_remaining_, orsreply_.size_executed_, orsreply_.client_position_, orsreply_.global_position_,
            orsreply_.int_price_, orsreply_.server_assigned_message_sequence_, orsreply_.exch_assigned_sequence_,
            orsreply_.time_set_by_server_);
      }
    } break;
    case kORRType_CxlSeqd: {
    } break;
    case kORRType_Cxld: {
    } break;
    case kORRType_Exec: {
      for (size_t i = 0; i < order_executed_listener_vec_.size(); i++) {
        order_executed_listener_vec_[i]->OrderExecuted(
            orsreply_.server_assigned_client_id_, orsreply_.client_assigned_order_sequence_,
            orsreply_.server_assigned_order_sequence_, security_id, orsreply_.price_, orsreply_.buysell_,
            orsreply_.size_remaining_, orsreply_.size_executed_, orsreply_.client_position_, orsreply_.global_position_,
            orsreply_.int_price_, orsreply_.server_assigned_message_sequence_, orsreply_.exch_assigned_sequence_,
            orsreply_.time_set_by_server_);
      }
    } break;
    case kORRType_IntExec: {
    } break;
    case kORRType_Rejc: {
    } break;
    case kORRType_CxlRejc: {
    } break;
    case kORRType_CxRe: {
    } break;
    case kORRType_CxReRejc: {
    } break;
    default:
      break;
  }
}

void CMEArbDataSource::ProcessAllEvents(int32_t socket_fd_with_data) {
  int32_t socket_index = socket_fd_to_actual_socket_index_map_[socket_fd_with_data];
  HFSAT::CMEArbDataSourceType_t data_source_type = socket_index_to_data_source_type_map_[socket_index];

  while (true) {
    switch (data_source_type) {
      case HFSAT::CMEArbDataSourceType_t::kArbDataSrcCMEMktData: {
        socket_read_size_ = multicast_receiver_socket_vec_[socket_index]->ReadN(cstr_obf_pkt_size_, (void*)&cstr_obf_);
        if (cstr_obf_pkt_size_ > socket_read_size_) return;  // exhausted the read
        ProcessCMEOBFStruct();
      } break;
      case HFSAT::CMEArbDataSourceType_t::kArbDataSrcORSReplyLive: {
        socket_read_size_ = multicast_receiver_socket_vec_[socket_index]->ReadN(orsreply_pkt_size_, (void*)&orsreply_);
        if (orsreply_pkt_size_ > socket_read_size_) return;  // exhausted the read
        ProcessORSReplyStruct();
      } break;
      default: {
        DBGLOG_CLASS_FUNC_LINE_ERROR << "UNEXPECTED DATA SOURCE TYPE : " << data_source_type << " DISCARDING PACKET "
                                     << DBGLOG_ENDL_FLUSH;
        DBGLOG_DUMP;
      } break;
    }
  }
}

void CMEArbDataSource::CleanUp() {
  for (uint32_t socket_counter = 0; socket_counter < multicast_receiver_socket_vec_.size(); socket_counter++) {
    if (nullptr != multicast_receiver_socket_vec_[socket_counter]) {
      delete multicast_receiver_socket_vec_[socket_counter];
      multicast_receiver_socket_vec_[socket_counter] = nullptr;
    }
  }

  if (nullptr != multicast_sender_socket_) {
    delete multicast_sender_socket_;
    multicast_sender_socket_ = nullptr;
  }
}

TradeType_t CMEArbDataSource::GetTradeType(const char type) {
  switch (type) {
    case '0': {
      return kTradeTypeBuy;
      break;
    }
    case '1': {
      return kTradeTypeSell;
      break;
    }
    default: {
      return kTradeTypeNoInfo;
      break;
    }
  }
}
}
