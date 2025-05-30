#include "midterm/GeneralizedLogic/nse_exec_logic_order_reader_from_file.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/CDef/file_utils.hpp"

namespace NSE_SIMPLEEXEC {
SimpleNseExecLogicOrderReaderFromFile::SimpleNseExecLogicOrderReaderFromFile(
    HFSAT::Watch &watch_t, HFSAT::DebugLogger &dbglogger_t, bool is_live_t,
    std::string orders_file_t,
    std::string backup_file_t,
    std::map<std::string, SyntheticLegInfo> leg_info_t,
    NseExecLogicHelper *exec_logic_helper_t)
    : SimpleNseExecLogicOrderReader(watch_t, dbglogger_t, is_live_t, leg_info_t,
                                    exec_logic_helper_t, backup_file_t),
      orders_file_(orders_file_t){
  if (!is_live_t) {
    dbglogger_ << "LoadAllOrders in SimMode: at " << watch_t.tv() << "\n";
    LoadAllOrdersInSimMode();
  }
  num_backup_orders_read_ = 0;
  backup_file_read_ = false;
  watch_.subscribe_BigTimePeriod(this);
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
      HFSAT::PerishableStringTokenizer st_(readline_buffer_,
                                           kOrderFileLineBufferLen);
      const std::vector<const char *> &tokens_ = st_.GetTokens();

      if (tokens_.size() < 6) // assuming format: timestamp Instrument lot_Size
                              // strat_id Ref_Px Entry/Exit X Y Z
        continue;
      else if (tokens_[0][0] == '#')
        continue;
      else if ((strcmp(tokens_[5], "Entry") != 0) &&
               (strcmp(tokens_[5], "Exit") != 0) &&
               (strcmp(tokens_[5], "Cancel") != 0) &&
               (strcmp(tokens_[5], "Rollover") != 0) &&
               (strcmp(tokens_[5], "Modify") != 0)) {
        DBGLOG_CLASS_FUNC_LINE_ERROR
            << "Incomplete order entry : " << readline_buffer_
            << DBGLOG_ENDL_FLUSH;
        DBGLOG_DUMP;
      } else {
        OrderType_t temp_order_type_ = OrderType_t::kEntry;
        if (strcmp(tokens_[5], "Exit") == 0) {
          temp_order_type_ = OrderType_t::kExit;
        } else if (strcmp(tokens_[5], "Rollover") == 0) {
          temp_order_type_ = OrderType_t::kRollover;
        } else if (strcmp(tokens_[5], "Modify") == 0) {
          temp_order_type_ = OrderType_t::kModify;
        } else if (strcmp(tokens_[5], "Cancel") == 0) {
          temp_order_type_ = OrderType_t::kCancel;
        }

        uint64_t order_request_time = strtoul(tokens_[0], NULL, 0);
        order_request_time = order_request_time / (1000000000);
        std::string shortcode_ = std::string(tokens_[1]);
        int strat_id_ = atoi(tokens_[3]);
        double ref_px_ = atof(tokens_[4]);
        std::vector<std::string> shortcode_tokens_;
        HFSAT::PerishableStringTokenizer::StringSplit(shortcode_, '_',
                                                      shortcode_tokens_);
        std::string ticker_ = shortcode_tokens_[1];
        std::string type_ =
            shortcode_tokens_[2].substr(0, shortcode_tokens_[2].size() - 1);
        // Shortcode could be synthetic as well
        if (shortcode_.substr(0, 7) == "COMPLEX") {
          GeneralOrderInfo_t general_info;
          general_info.general_shortcode_ = shortcode_;
          std::ostringstream t_temp_oss_;
          t_temp_oss_ << tokens_[0] << "_" << strat_id_;
          std::string orderID_ = t_temp_oss_.str();
          std::ostringstream t_temp_oss2_;
          t_temp_oss2_ << tokens_[2] << "_" << strat_id_;
          std::string ordertag_ = t_temp_oss2_.str();

          general_info.order_id_ = orderID_;
          general_info.ref_px_ = ref_px_;
          general_info.strat_id_ = strat_id_;
          general_info.order_type_ = temp_order_type_;
          // For complex order, no concept of lots so we use it as an order tag
          // for modifying or cancelling orders
          general_info.order_tag_ = ordertag_;

          general_order_time_to_order_info_map_.insert(
              std::make_pair(order_request_time, general_info));
          dbglogger_ << "SimMode: Loaded general order -> "
                     << order_request_time << '\t' << shortcode_ << '\n';
        } else {
          dbglogger_
              << "ALERT -> SimMode: Shortcode from ordersfile is not handled"
              << DBGLOG_ENDL_FLUSH;
        }
      }
    }
    dbglogger_ << "SimMode: Loaded all general orders. Total Size: "
               << general_order_time_to_order_info_map_.size() << '\n';
    dbglogger_.DumpCurrentBuffer();
  }
}

void SimpleNseExecLogicOrderReaderFromFile::HandleGeneralizedOrdersInSim() {
  general_Range_itr general_current_orders_itr;
  uint64_t general_current_time = (uint64_t)watch_.tv().tv_sec;
  general_current_orders_itr =
      general_order_time_to_order_info_map_.equal_range(general_current_time);
  if (general_current_orders_itr.first != general_current_orders_itr.second) {
    for (general_order_info_itr o_itr = general_current_orders_itr.first;
         o_itr != general_current_orders_itr.second;) {
      if (o_itr->second.order_type_ == OrderType_t::kModify) {
        dbglogger_ << "SimMode Order Reader: Pass general modify order to "
                      "execlogic at "
                   << watch_.tv() << " " << o_itr->second.general_shortcode_
                   << " for tag -> " << o_itr->second.order_id_ << "\n";
        dbglogger_.DumpCurrentBuffer();
        NotifyModifyListeners(o_itr->second.general_shortcode_,
                              o_itr->second.order_tag_);
      } else if (o_itr->second.order_type_ == OrderType_t::kCancel) {
        dbglogger_ << "SimMode Order Reader: Pass cancel order to "
                      "execlogic at "
                   << watch_.tv() << " " << o_itr->second.general_shortcode_
                   << " with ID -> " << o_itr->second.order_id_
                   << " for tag -> " << o_itr->second.order_tag_ << "\n";
        dbglogger_.DumpCurrentBuffer();
        NotifyCancelListeners(o_itr->second.order_id_,
                              o_itr->second.order_tag_);
      } else {
        dbglogger_ << "SimMode Order Reader: Pass general complex order to "
                      "execlogic at "
                   << watch_.tv() << " " << o_itr->second.general_shortcode_
                   << " with ID -> " << o_itr->second.order_id_ << "\n";
        dbglogger_.DumpCurrentBuffer();
        NotifyOrderListeners(o_itr->second.general_shortcode_,
                             o_itr->second.order_id_, 0, o_itr->second.ref_px_);
      }
      general_order_time_to_order_info_map_.erase(o_itr++);
    }
  }
}

void SimpleNseExecLogicOrderReaderFromFile::LookupOrdersInSimMode() {
  if (general_order_time_to_order_info_map_.size() != 0) {
    HandleGeneralizedOrdersInSim();
  }
}

  void SimpleNseExecLogicOrderReaderFromFile::ReadLiveOrdersFromFile(std::string fname, bool time_checks) {
  // open file and see if there are any new orders present
  std::ifstream ordersfile_;
  ordersfile_.open(fname.c_str(), std::ifstream::in);
  if (ordersfile_.is_open()) {
    const int kOrderFileLineBufferLen = 2048;
    char readline_buffer_[kOrderFileLineBufferLen];
    bzero(readline_buffer_, kOrderFileLineBufferLen);

    while (ordersfile_.good()) {
      bzero(readline_buffer_, kOrderFileLineBufferLen);
      ordersfile_.getline(readline_buffer_, kOrderFileLineBufferLen);
      HFSAT::PerishableStringTokenizer st_(readline_buffer_,
                                           kOrderFileLineBufferLen);
      const std::vector<const char *> &tokens_ = st_.GetTokens();
      if (tokens_.size() < 6) {// assuming format: timestamp Instrument lot_Size
                              // strat_id Ref_Px Entry/Exit X Y Z
        DBGLOG_CLASS_FUNC_LINE_ERROR
            << "TOKEN LESS THAN 6, Incomplete order entry : " << readline_buffer_
            << DBGLOG_ENDL_FLUSH;
        DBGLOG_DUMP;
        continue;
      }
      else if (tokens_[0][0] == '#')
        continue;
      else if ((strcmp(tokens_[5], "Entry") != 0) &&
               (strcmp(tokens_[5], "Exit") != 0) &&
               (strcmp(tokens_[5], "Cancel") != 0) &&
               (strcmp(tokens_[5], "Rollover") != 0) &&
               (strcmp(tokens_[5], "Modify") != 0) && 
               (strcmp(tokens_[5], "ForceCancel") != 0)){
        DBGLOG_CLASS_FUNC_LINE_ERROR << "INVALID ORDER ENTRY FOR : " << readline_buffer_
            << DBGLOG_ENDL_FLUSH;
        DBGLOG_DUMP;
        continue;
      }
      else {
        uint64_t order_request_time = strtoul(tokens_[0], NULL, 0);
        if (time_checks && order_request_time < exec_start_time_) {
          DBGLOG_CLASS_FUNC_LINE_ERROR
              << "NOT PROCESSING THE ORDER REQUEST : " << readline_buffer_
              << " AS IT FAILS TO CLEAR TIME CHECK CONSTRAINT, EXEC TIME : "
              << exec_start_time_ << " ORDER TIME : " << order_request_time << "Order Id : "<< tokens_[0]
              << DBGLOG_ENDL_FLUSH;
          DBGLOG_DUMP;
          continue;
        }

        OrderType_t temp_order_type_ = OrderType_t::kEntry;
        if (strcmp(tokens_[5], "Exit") == 0) {
          temp_order_type_ = OrderType_t::kExit;
        } else if (strcmp(tokens_[5], "Cancel") == 0) {
          temp_order_type_ = OrderType_t::kCancel;
        } else if (strcmp(tokens_[5], "Rollover") == 0) {
          temp_order_type_ = OrderType_t::kRollover;
        } else if (strcmp(tokens_[5], "Modify") == 0) {
          temp_order_type_ = OrderType_t::kModify;
        } else if (strcmp(tokens_[5], "ForceCancel") == 0) {
          temp_order_type_ = OrderType_t::kForceCancel;
        }


        int strat_id = atoi(tokens_[3]);
        std::string instrument_ = std::string(tokens_[1]);

        std::ostringstream t_temp_oss_;
        t_temp_oss_ << tokens_[0] << "_" << strat_id;
        std::string orderID_ = t_temp_oss_.str(); // timestamp_stratID

        std::ostringstream t_temp_oss2_;
        t_temp_oss2_ << tokens_[2] << "_" << strat_id;
        std::string ordertag_ = t_temp_oss2_.str(); // tag_stratID

        // Notify execLogic for first order for this inst_ or new order added
        if (!time_checks || (stratID_to_last_processed_order_time_map_.find(strat_id) ==
                stratID_to_last_processed_order_time_map_.end() ||
            stratID_to_last_processed_order_time_map_[strat_id] <
			     order_request_time)) {
	  if (time_checks) {
	    stratID_to_last_processed_order_time_map_[strat_id] =
              order_request_time;
	  } else {
	    num_backup_orders_read_ ++;
	    if (num_backup_orders_read_ > LARGE_NUM_BACKUP_ORDS) {
	      DBGLOG_CLASS_FUNC_LINE_ERROR << "Order not processed from an abundance of caution since too many backup orders have been detected" << DBGLOG_ENDL_FLUSH;
              break;
	    }
            //TODO -- remove logging after tests
            dbglogger_ << "Processing order " << num_backup_orders_read_ << ' ' << orderID_ << " at " 
		       << watch_.tv() << DBGLOG_ENDL_FLUSH;
	  }
	 
          // tokens_[2] is position for simple exec logic, and used as tag_id_
          // for generalized
          if (temp_order_type_ == OrderType_t::kModify) {
            NotifyModifyListeners(instrument_, ordertag_);
          } else if (temp_order_type_ == OrderType_t::kCancel) {
            NotifyCancelListeners(orderID_, ordertag_);
          } else if (temp_order_type_ == OrderType_t::kForceCancel){
              dbglogger_ << "TRYING TO FORCE CANCEL ORDER -> " << instrument_ << " ORDER " << orderID_ << " ORDERTAG: " << ordertag_ << "\n";
              DBGLOG_DUMP;
              NotifyForceCancelListeners(instrument_, orderID_, ordertag_);
              dbglogger_ << "DONE FORCE CANCEL " << "\n";
          } else {
            std::cout << "INSTRUMENT "<< instrument_ <<std::endl;
            std::vector<std::string> tokens_shortcode;
            std::vector<std::string> shortcodes_; 
            HFSAT::PerishableStringTokenizer::StringSplit(instrument_, '%',
                                                  tokens_shortcode);
            HFSAT::PerishableStringTokenizer::StringSplit(tokens_shortcode[2], '|', shortcodes_);
            int num_sec_ = std::atoi(tokens_shortcode[1].c_str());
            std::cout<< "Num of sec" << num_sec_ <<std::endl;
            if (shortcodes_.size() != (std::vector<std::string>::size_type)num_sec_)  {
                dbglogger_ << "ERROR -> Number of securities mismatch..." << orderID_  << "\n";
                std::cout<< "ERROR -> Number of securities mismatch... " << orderID_ <<"\n";
                std::string subject_email_string_ =" ERROR -> Number of securities mismatch... " + orderID_;
                HFSAT::Email email_;
                char hostname[128];
                hostname[127] = '\0';
                gethostname(hostname, 127);
                email_.setSubject(subject_email_string_);
                email_.addRecepient("raghunandan.sharma@tworoads-trading.co.in");
                email_.addSender("raghunandan.sharma@tworoads-trading.co.in");
                email_.content_stream << "host_machine: " << hostname << "<br/>";
                email_.sendMail();
                return ;
            }

            NotifyOrderListeners(instrument_, orderID_, atoi(tokens_[2]),
                                 atof(tokens_[4]));
          }
        }
        else {
//        	DBGLOG_CLASS_FUNC_LINE_ERROR
//        	              << "Order not processed because order time less than last order processed for this strat. ORDER TIME : " << order_request_time << "Order Id : "<< tokens_[0]
//        	              << DBGLOG_ENDL_FLUSH;

        }
      }
    }
  }
}

void SimpleNseExecLogicOrderReaderFromFile::OnTimePeriodUpdate(
    const int num_pages_to_add_) {
  if (isLive) {
    ReadLiveOrdersFromFile(orders_file_);
    //Call backup orders processing slightly after open to avoid
    //issues with books not being ready etc
    if (watch_.msecs_from_midnight() > BACKUP_PROCESSING_MSECS_BEGIN &&
	!backup_file_read_ && 
        HFSAT::FileUtils::ExistsAndReadable(bkp_file_)) {
      backup_file_read_ = true;
      dbglogger_ << "Calling ReadLiveOrders for backup file at " << watch_.tv() << DBGLOG_ENDL_FLUSH; 
      ReadLiveOrdersFromFile(bkp_file_, false);
      std::string bkp_file_renamed_ = bkp_file_ + ".processed";
      std::rename(bkp_file_.c_str(), bkp_file_renamed_.c_str());
    }
  }
  else
    LookupOrdersInSimMode();
}
}
