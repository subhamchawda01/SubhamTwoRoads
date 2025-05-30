#include "midterm/GeneralizedLogic/nse_exec_logic_order_reader_from_TCP.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

namespace NSE_SIMPLEEXEC {

SimpleNseExecLogicOrderReaderFromTCP::SimpleNseExecLogicOrderReaderFromTCP(
    HFSAT::Watch &watch_t, HFSAT::DebugLogger &dbglogger_t, bool is_live_t,
    std::map<std::string, SyntheticLegInfo> leg_info_t,
    NseExecLogicHelper *exec_logic_helper_t, int server_port_)
    : SimpleNseExecLogicOrderReader(watch_t, dbglogger_t, is_live_t, leg_info_t,
                                    exec_logic_helper_t, ""),
      tcp_server_manager_(NULL), is_ready_(false),
      curr_buffer_(std::move(curr_buffer_)), mkt_lock_() {
  tcp_server_manager_ =
      new HFSAT::Utils::TCPServerManager(server_port_, dbglogger_t);
  tcp_server_manager_->run();
  tcp_server_manager_->SubscribeForUpdates(this);
  dbglogger_ << "TCP io: created server manager and waiting for clients "
             << watch_t.tv() << "\n";
}

void SimpleNseExecLogicOrderReaderFromTCP::OnClientRequest(
    int32_t client_fd, char *buffer, uint32_t const &length) {
  // Put lock to avoid race condition when reading multiple threads because of
  // shared variables like cur_buffer,
  // is_ready
  mkt_lock_.LockMutex();
  DBGLOG_CLASS_FUNC_LINE_INFO << "Client received: " << client_fd
                              << " Buffer received: " << buffer
                              << " Buffer length: " << length << "\n";
  DBGLOG_DUMP;
  uint32_t i = 0;
  while (i < length) {
    // Read more into buffer till we encounter ^
    if (buffer[i] == '\0')
      break;
    while (i < length) {
      curr_buffer_ += buffer[i++];
      if (buffer[i] == '^') {
        is_ready_ = true;
        i++;
        break;
      }
    }

    // Still not ready we need to return and wait for next OnClientRequest to be
    // called
    // This is done because we might have received incomplete packets
    if (!is_ready_) {
      DBGLOG_CLASS_FUNC_LINE_ERROR
          << "Complete packet has not been received for order request:"
          << curr_buffer_ << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
      continue;
    }
    DBGLOG_CLASS_FUNC_LINE_INFO << "Client tokenize: " << client_fd << " "
                                << curr_buffer_ << "\n";
    DBGLOG_DUMP;
    std::vector<std::string> tokens_;
    HFSAT::PerishableStringTokenizer::StringSplit(curr_buffer_, '*', tokens_);
    DBGLOG_CLASS_FUNC_LINE_INFO << "Client after tokenize: " << client_fd << " "
                                << tokens_.size() << "\t";
    for (uint32_t j = 0; j < tokens_.size(); j++) {
      DBGLOG_CLASS_FUNC_LINE_INFO << tokens_[j] << " ";
    }
    DBGLOG_CLASS_FUNC_LINE_INFO << "\n\n\n";
    DBGLOG_DUMP;

    if (tokens_.size() < 6) { // assuming format:
      // timestamp|Instrument|lot_Size|strat_id|Ref_Px|Entry/Exit/RollOver^
      DBGLOG_CLASS_FUNC_LINE_ERROR << "Incorrect packet sent :" << curr_buffer_
                                   << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
      mkt_lock_.UnlockMutex();
      return;
    } else {
      uint64_t order_request_time = strtoul(tokens_[0].c_str(), NULL, 0);
      if (order_request_time < exec_start_time_) {
        DBGLOG_CLASS_FUNC_LINE_ERROR
            << "NOT PROCESSING THE ORDER REQUEST : " << curr_buffer_
            << " AS IT FAILS TO CLEAR TIME CHECK CONSTRAINT, EXEC TIME : "
            << exec_start_time_ << " ORDER TIME : " << order_request_time
            << DBGLOG_ENDL_FLUSH;
        DBGLOG_DUMP;
        mkt_lock_.UnlockMutex();
        return;
      }

      OrderType_t temp_order_type_ = OrderType_t::kEntry;
      if (strcmp(tokens_[5].c_str(), "Exit") == 0) {
        temp_order_type_ = OrderType_t::kExit;
      } else if (strcmp(tokens_[5].c_str(), "Cancel") == 0) {
        temp_order_type_ = OrderType_t::kCancel;
      } else if (strcmp(tokens_[5].c_str(), "Rollover") == 0) {
        temp_order_type_ = OrderType_t::kRollover;
      } else if (strcmp(tokens_[5].c_str(), "Modify") == 0) {
        temp_order_type_ = OrderType_t::kModify;
      }

      int strat_id = atoi(tokens_[3].c_str());

      std::string instrument_ = tokens_[1];
      std::ostringstream t_temp_oss_;
      t_temp_oss_ << tokens_[0] << "_" << strat_id;
      std::string orderID_ = t_temp_oss_.str(); // timestamp_stratID

      std::ostringstream t_temp_oss2_;
      t_temp_oss2_ << tokens_[2] << "_" << strat_id;
      std::string ordertag_ = t_temp_oss2_.str(); // tag_stratID

      // Notify execLogic for first order for this inst_ or new order added
      if (stratID_to_last_processed_order_time_map_.find(strat_id) ==
              stratID_to_last_processed_order_time_map_.end() ||
          stratID_to_last_processed_order_time_map_[strat_id] <
              order_request_time) {
        stratID_to_last_processed_order_time_map_[strat_id] =
            order_request_time;

        if (temp_order_type_ == OrderType_t::kModify) {
          NotifyModifyListeners(instrument_, ordertag_);
        } else if (temp_order_type_ == OrderType_t::kCancel) {
          NotifyCancelListeners(orderID_, ordertag_);
        } else {
          NotifyOrderListeners(instrument_, orderID_, atoi(tokens_[2].c_str()),
                               atof(tokens_[4].c_str()));
        }

        DBGLOG_CLASS_FUNC_LINE_INFO
            << "DATA NOTIFIED: " << client_fd << " " << orderID_ << " "
            << atoi(tokens_[2].c_str()) << " " << atof(tokens_[4].c_str())
            << " "
            << "\n";
        DBGLOG_DUMP;
      } else {
        DBGLOG_CLASS_FUNC_LINE_ERROR
            << "Client ignore this order: " << client_fd << " " << orderID_
            << "\n";
        // We can assume that the map is not empty because it is in if condition
        // check
        dbglogger_ << "ALERT -> Last received timestamp for strat Id: "
                   << strat_id << " is "
                   << stratID_to_last_processed_order_time_map_[strat_id]
                   << " and order request time is -> " << order_request_time
                   << '\n';

        DBGLOG_DUMP;
      }
      is_ready_ = false;
      curr_buffer_ = "";
    }
  }
  mkt_lock_.UnlockMutex();
}
}
