#include "dvctrade/ExecLogic/nse_exec_logic_order_reader_from_TCP.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

namespace NSE_SIMPLEEXEC {

SimpleNseExecLogicOrderReaderFromTCP::SimpleNseExecLogicOrderReaderFromTCP(HFSAT::Watch &watch_t,
                                                                           HFSAT::DebugLogger &dbglogger_t,
                                                                           bool is_live_t)
    : SimpleNseExecLogicOrderReader(watch_t, dbglogger_t, is_live_t),
      tcp_server_manager_(NULL),
      is_ready_(false),
      curr_buffer_(std::move(curr_buffer_)),
      mkt_lock_() {
  tcp_server_manager_ = new HFSAT::Utils::TCPServerManager(NSE_ORDER_SEND_PORT, dbglogger_t);
  tcp_server_manager_->run();
  tcp_server_manager_->SubscribeForUpdates(this);
  dbglogger_ << "TCP io: created server manager and waiting for clients " << watch_t.tv() << "\n";
}

void SimpleNseExecLogicOrderReaderFromTCP::OnClientRequest(int32_t client_fd, char *buffer, uint32_t const &length) {
  // Put lock to avoid race condition when reading multiple threads because of shared variables like cur_buffer,
  // is_ready
  mkt_lock_.LockMutex();
  DBGLOG_CLASS_FUNC_LINE_INFO << "Client received: " << client_fd << " Buffer received: " << buffer
                              << " Buffer length: " << length << "\n";
  DBGLOG_DUMP;
  int i = 0;
  while (true) {
    // Read more into buffer till we encounter ^
    if (buffer[i] == '\0') break;
    while (true) {
      curr_buffer_ += buffer[i++];
      if (buffer[i] == '^') {
        is_ready_ = true;
        i++;
        break;
      }
    }

    // Still not ready we need to return and wait for next OnClientRequest to be called
    // This is done because we might have received incomplete packets
    if (!is_ready_) {
      DBGLOG_CLASS_FUNC_LINE_ERROR << "Complete packet has not been received for order request:" << curr_buffer_
                                   << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
      continue;
    }
    DBGLOG_CLASS_FUNC_LINE_INFO << "Client tokenize: " << client_fd << " " << curr_buffer_ << "\n";
    DBGLOG_DUMP;
    std::vector<std::string> tokens_;
    HFSAT::PerishableStringTokenizer::StringSplit(curr_buffer_, '|', tokens_);
    DBGLOG_CLASS_FUNC_LINE_INFO << "Client after tokenize: " << client_fd << " " << tokens_.size() << "\t";
    for (uint32_t j = 0; j < tokens_.size(); j++) {
      DBGLOG_CLASS_FUNC_LINE_INFO << tokens_[j] << " ";
    }
    DBGLOG_CLASS_FUNC_LINE_INFO << "\n";
    DBGLOG_DUMP;

    if (tokens_.size() < 6) {  // assuming format: timestamp|Instrument|lot_Size|strat_id|Ref_Px|Entry/Exit/RollOver^
      DBGLOG_CLASS_FUNC_LINE_ERROR << "Incorrect packet sent :" << curr_buffer_ << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
      continue;
    } else if (tokens_[0][0] == '#')
      continue;
    else {
      uint64_t order_request_time = strtoul(tokens_[0].c_str(), NULL, 0);
      if (order_request_time < exec_start_time_) {
        DBGLOG_CLASS_FUNC_LINE_ERROR << "NOT PROCESSING THE ORDER REQUEST : " << curr_buffer_
                                     << " AS IT FAILS TO CLEAR TIME CHECK CONSTRAINT, EXEC TIME : " << exec_start_time_
                                     << " ORDER TIME : " << order_request_time << DBGLOG_ENDL_FLUSH;
        DBGLOG_DUMP;
        continue;
      }
      int strat_id = atoi(tokens_[3].c_str());
      NSEProductStratID_t product_stratId_entry;
      product_stratId_entry.instrument_ = tokens_[1];
      product_stratId_entry.strat_id_ = strat_id;
      std::ostringstream t_temp_oss_;
      t_temp_oss_ << tokens_[0] << "_" << strat_id;
      std::string orderID_ = t_temp_oss_.str();  // timestamp_stratID
      // Notify execLogic for first order for this inst_ or new order added
      if (instrument_to_last_processed_order_time_map_.find(product_stratId_entry) ==
              instrument_to_last_processed_order_time_map_.end() ||
          (instrument_to_last_processed_order_time_map_[product_stratId_entry]) / pow(10.0, 9.0) <=
              order_request_time / pow(10.0, 9.0)) {
        instrument_to_last_processed_order_time_map_[product_stratId_entry] = order_request_time;
        NotifyOrderListeners(product_stratId_entry.instrument_, orderID_, atoi(tokens_[2].c_str()),
                             atof(tokens_[4].c_str()));

        DBGLOG_CLASS_FUNC_LINE_INFO << "DATA NOTIFIED: " << client_fd << " " << orderID_ << " "
                                    << atoi(tokens_[2].c_str()) << " " << atof(tokens_[4].c_str()) << " "
                                    << "\n";
        DBGLOG_DUMP;
      } else {
        DBGLOG_CLASS_FUNC_LINE_ERROR << "Client ignore this order: " << client_fd << " " << orderID_ << "\n";
        DBGLOG_DUMP;
      }
      is_ready_ = false;
      curr_buffer_ = "";
    }
  }
  mkt_lock_.UnlockMutex();
}
}
