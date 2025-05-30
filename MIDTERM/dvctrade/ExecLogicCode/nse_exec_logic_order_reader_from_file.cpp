#include "dvctrade/ExecLogic/nse_exec_logic_order_reader_from_file.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

namespace NSE_SIMPLEEXEC {
SimpleNseExecLogicOrderReaderFromFile::SimpleNseExecLogicOrderReaderFromFile(HFSAT::Watch& watch_t,
                                                                             HFSAT::DebugLogger& dbglogger_t,
                                                                             bool is_live_t, std::string orders_file_t)
    : SimpleNseExecLogicOrderReader(watch_t, dbglogger_t, is_live_t),
      order_time_to_order_info_map_(),
      orders_file_(orders_file_t) {
  if (!is_live_t) {
    dbglogger_ << "LoadAllOrders in SimMode: at " << watch_t.tv() << "\n";
    LoadAllOrdersInSimMode();
  }
}

void SimpleNseExecLogicOrderReaderFromFile::LoadAllOrdersInSimMode() {
  // open file and see if there are any new orders present
  std::ifstream ordersfile_;
  ordersfile_.open(orders_file_.c_str(), std::ifstream::in);
  if (ordersfile_.is_open()) {
    const int kOrderFileLineBufferLen = 1024;
    char readline_buffer_[kOrderFileLineBufferLen];
    bzero(readline_buffer_, kOrderFileLineBufferLen);

    while (ordersfile_.good()) {
      bzero(readline_buffer_, kOrderFileLineBufferLen);
      ordersfile_.getline(readline_buffer_, kOrderFileLineBufferLen);
      HFSAT::PerishableStringTokenizer st_(readline_buffer_, kOrderFileLineBufferLen);
      const std::vector<const char*>& tokens_ = st_.GetTokens();
      if (tokens_.size() < 6)  // assuming format: timestamp Instrument lot_Size strat_id Ref_Px Entry/Exit X Y Z
        continue;
      else if (tokens_[0][0] == '#')
        continue;
      else if ((strcmp(tokens_[5], "Entry") != 0) && (strcmp(tokens_[5], "Exit") != 0) &&
               (strcmp(tokens_[5], "Rollover") != 0)) {
        DBGLOG_CLASS_FUNC_LINE_ERROR << "Incomplete order entry : " << readline_buffer_ << DBGLOG_ENDL_FLUSH;
        DBGLOG_DUMP;
      } else {
        uint64_t order_request_time = strtoul(tokens_[0], NULL, 0);
        order_request_time = order_request_time / (1000000000);
        int strat_id = atoi(tokens_[3]);
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << tokens_[0] << "_" << strat_id;  // timestamp_stratID
        RVOrderInfo_t rv_order_info;
        rv_order_info.instrument_ = std::string(tokens_[1]);
        rv_order_info.strat_id_ = strat_id;
        rv_order_info.lot_size_ = atoi(tokens_[2]);
        rv_order_info.ref_px_ = atof(tokens_[4]);
        rv_order_info.order_id_ = t_temp_oss_.str();
        order_time_to_order_info_map_.insert(std::make_pair(order_request_time, rv_order_info));
        dbglogger_ << "SimMode: this order " << strtoul(tokens_[0], NULL, 0) << " " << order_request_time << " "
                   << rv_order_info.instrument_ << '\n';
      }
    }
    dbglogger_ << "SimMode: Loaded all orders. Total Size: " << order_time_to_order_info_map_.size() << '\n';
    dbglogger_.DumpCurrentBuffer();
  }
}
void SimpleNseExecLogicOrderReaderFromFile::LookupOrdersInSimMode() {
  Range_itr current_orders_itr;
  uint64_t current_time = (uint64_t)watch_.tv().tv_sec;
  current_orders_itr = order_time_to_order_info_map_.equal_range(current_time);
  if (current_orders_itr.first == current_orders_itr.second) {
    return;  // no orders at this time
  } else {
    for (order_info_itr o_itr = current_orders_itr.first; o_itr != current_orders_itr.second;) {
      dbglogger_ << "SimMode Order Reader: Pass order to execlogic at " << watch_.tv() << " "
                 << o_itr->second.instrument_ << " " << o_itr->second.order_id_ << "\n";
      dbglogger_.DumpCurrentBuffer();
      NotifyOrderListeners(o_itr->second.instrument_, o_itr->second.order_id_, o_itr->second.lot_size_,
                           o_itr->second.ref_px_);

      order_time_to_order_info_map_.erase(o_itr++);
    }
  }
}

void SimpleNseExecLogicOrderReaderFromFile::ReadNewLiveOrdersFromFile() {
  // open file and see if there are any new orders present
  std::ifstream ordersfile_;
  ordersfile_.open(orders_file_.c_str(), std::ifstream::in);
  if (ordersfile_.is_open()) {
    const int kOrderFileLineBufferLen = 1024;
    char readline_buffer_[kOrderFileLineBufferLen];
    bzero(readline_buffer_, kOrderFileLineBufferLen);

    while (ordersfile_.good()) {
      bzero(readline_buffer_, kOrderFileLineBufferLen);
      ordersfile_.getline(readline_buffer_, kOrderFileLineBufferLen);
      HFSAT::PerishableStringTokenizer st_(readline_buffer_, kOrderFileLineBufferLen);
      const std::vector<const char*>& tokens_ = st_.GetTokens();

      if (tokens_.size() < 6)  // assuming format: timestamp Instrument lot_Size strat_id Ref_Px Entry/Exit X Y Z
        continue;
      else if (tokens_[0][0] == '#')
        continue;
      else if ((strcmp(tokens_[5], "Entry") != 0) && (strcmp(tokens_[5], "Exit") != 0) &&
               (strcmp(tokens_[5], "Rollover") != 0))
        continue;
      else {
        uint64_t order_request_time = strtoul(tokens_[0], NULL, 0);
        if (order_request_time < exec_start_time_) {
          DBGLOG_CLASS_FUNC_LINE_ERROR << "NOT PROCESSING THE ORDER REQUEST : " << readline_buffer_
                                       << " AS IT FAILS TO CLEAR TIME CHECK CONSTRAINT, EXEC TIME : "
                                       << exec_start_time_ << " ORDER TIME : " << order_request_time
                                       << DBGLOG_ENDL_FLUSH;
          DBGLOG_DUMP;
          continue;
        }

        int strat_id = atoi(tokens_[3]);
        NSEProductStratID_t product_stratId_entry;
        product_stratId_entry.instrument_ = std::string(tokens_[1]);
        product_stratId_entry.strat_id_ = strat_id;
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << tokens_[0] << "_" << strat_id;
        std::string orderID_ = t_temp_oss_.str();  // timestamp_stratID

        // Notify execLogic for first order for this inst_ or new order added
        if (instrument_to_last_processed_order_time_map_.find(product_stratId_entry) ==
                instrument_to_last_processed_order_time_map_.end() ||
            instrument_to_last_processed_order_time_map_[product_stratId_entry] < order_request_time) {
          instrument_to_last_processed_order_time_map_[product_stratId_entry] = order_request_time;
          NotifyOrderListeners(product_stratId_entry.instrument_, orderID_, atoi(tokens_[2]), atof(tokens_[4]));
        }
      }
    }
  }
}

void SimpleNseExecLogicOrderReaderFromFile::OnTimePeriodUpdate(const int num_pages_to_add_) {
  if (isLive)
    ReadNewLiveOrdersFromFile();
  else
    LookupOrdersInSimMode();
}
}
