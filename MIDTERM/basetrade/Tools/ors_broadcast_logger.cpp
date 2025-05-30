#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <signal.h>
#include <getopt.h>
#include <map>
#include <iostream>

#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CDef/file_utils.hpp"  // To create the directory
#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CDef/email_utils.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/CommonTradeUtils/watch.hpp"

#include "dvccode/CommonTradeUtils/watch.hpp"

#include "dvccode/Utils/multicast_receiver_socket.hpp"
#include "dvccode/Utils/ors_rejections_alert_thread.hpp"

#include "dvccode/CDef/ors_messages.hpp"
#include "dvccode/TradingInfo/network_account_interface_manager.hpp"

void print_usage(const char* prg_name) {
  printf("This is the MDS Data Logger \n");
  printf("Usage:%s --ip <bcast_address> --port <bcast_port> \n", prg_name);
}

class ORSBroadcastLogger {
 public:
  ORSBroadcastLogger(std::string ip, int port, HFSAT::DebugLogger& t_dbglogger, HFSAT::Watch& t_watch)
      : ip_(ip), port_(port), dbglogger_(t_dbglogger), watch_(t_watch) {
    socket_ = new HFSAT::MulticastReceiverSocket(
        ip_, port_, HFSAT::NetworkAccountInterfaceManager::instance().GetInterfaceForApp(HFSAT::k_ORS_LS));
    if (socket_ == NULL) {
      std::cout << " Failed to get the Receiver socket." << std::endl;
    }
    ors_rejections_alert_handler_.run();
  }

  void SendMultiCxlRejectAlert(std::string alert_text_) {
    HFSAT::SendAlert::sendAlert(alert_text_);

    HFSAT::Email e;
    e.setSubject("Possible Dead Order Detection");
    e.addRecepient("nseall@tworoads.co.in");
    e.addSender("nseall@tworoads.co.in");
    e.content_stream << alert_text_;
    e.sendMail();
  }

  void processMsgRecvd() {
    while (true) {
      int num_bytes = socket_->ReadN(sizeof(HFSAT::GenericORSReplyStruct), (void*)(&orsreply_));
      if (num_bytes >= (int)sizeof(HFSAT::GenericORSReplyStruct)) {
        watch_.OnTimeReceived(orsreply_.time_set_by_server_);

        switch (orsreply_.orr_type_) {
          case HFSAT::kORRType_None: {
            DBGLOG_TIME_CLASS_FUNC << "kORRType_None    SAOS : " << orsreply_.server_assigned_order_sequence_
                                   << " SYM:  " << orsreply_.symbol_
                                   << " CAOS: " << orsreply_.client_assigned_order_sequence_
                                   << " bs: " << (orsreply_.buysell_ == HFSAT::kTradeTypeBuy ? "BUY" : "SELL")
                                   //		    << " sz: " << orsreply_.size_requested_
                                   << " px: " << orsreply_.price_ << " intpx: " << orsreply_.int_price_
                                   << DBGLOG_ENDL_FLUSH;
          } break;
          case HFSAT::kORRType_Seqd: {
            DBGLOG_TIME_CLASS_FUNC << "kORRType_Seqd    SAOS : " << orsreply_.server_assigned_order_sequence_
                                   << " SYM:  " << orsreply_.symbol_
                                   << " CAOS: " << orsreply_.client_assigned_order_sequence_
                                   << " bs: " << (orsreply_.buysell_ == HFSAT::kTradeTypeBuy ? "BUY" : "SELL")
                                   // << " sz: " << orsreply_.size_requested_
                                   << " px: " << orsreply_.price_ << " intpx: " << orsreply_.int_price_
                                   << DBGLOG_ENDL_FLUSH;
          }

          break;

          case HFSAT::kORRType_Conf: {
            DBGLOG_TIME_CLASS_FUNC << "kORRType_Conf    SAOS : " << orsreply_.server_assigned_order_sequence_
                                   << " SYM:  " << orsreply_.symbol_
                                   << " CAOS: " << orsreply_.client_assigned_order_sequence_
                                   << " bs: " << (orsreply_.buysell_ == HFSAT::kTradeTypeBuy ? "BUY" : "SELL")
                                   //  << " sz: " << orsreply_.size_requested_
                                   << " px: " << orsreply_.price_ << " intpx: " << orsreply_.int_price_
                                   << " client_position_ : " << orsreply_.client_position_
                                   << " global_position_ : " << orsreply_.global_position_ << DBGLOG_ENDL_FLUSH;
          }

          break;
          case HFSAT::kORRType_ORSConf: {
            DBGLOG_TIME_CLASS_FUNC << "kORRType_ORSConf SAOS : " << orsreply_.server_assigned_order_sequence_
                                   << " SYM:  " << orsreply_.symbol_
                                   << " CAOS: " << orsreply_.client_assigned_order_sequence_
                                   << " bs: " << (orsreply_.buysell_ == HFSAT::kTradeTypeBuy ? "BUY" : "SELL")
                                   //  << " sz: " << orsreply_.size_requested_
                                   << " px: " << orsreply_.price_ << " intpx: " << orsreply_.int_price_
                                   << DBGLOG_ENDL_FLUSH;
          }

          break;
          case HFSAT::kORRType_CxlRejc: {
            DBGLOG_TIME_CLASS_FUNC << "kORRType_CxlRejc SAOS : " << orsreply_.server_assigned_order_sequence_
                                   << " SYM:  " << orsreply_.symbol_
                                   << " CAOS: " << orsreply_.client_assigned_order_sequence_
                                   << " bs: " << (orsreply_.buysell_ == HFSAT::kTradeTypeBuy ? "BUY" : "SELL")
                                   //<< " sz: " << orsreply_.size_requested_
                                   << " px: " << orsreply_.price_ << " intpx: " << orsreply_.int_price_
                                   << " rejR: " << orsreply_.size_executed_ << DBGLOG_ENDL_FLUSH;

            std::ostringstream t_temp_oss_;

            t_temp_oss_ << orsreply_.symbol_ << "-" << orsreply_.server_assigned_order_sequence_;

            if (exch_sym_saos_to_cxl_reject_map_.find(t_temp_oss_.str()) != exch_sym_saos_to_cxl_reject_map_.end()) {
              std::ostringstream alert_msg_str_;

              alert_msg_str_ << " Multiple CxlRejects Received For : " << orsreply_.symbol_
                             << " Possibly Dead Order : " << orsreply_.server_assigned_order_sequence_
                             << " Total Cxl Rejects : " << exch_sym_saos_to_cxl_reject_map_[t_temp_oss_.str()];

              SendMultiCxlRejectAlert(alert_msg_str_.str());

              exch_sym_saos_to_cxl_reject_map_[t_temp_oss_.str()]++;

            } else {
              exch_sym_saos_to_cxl_reject_map_[t_temp_oss_.str()] = 1;
            }

          }

          break;
          case HFSAT::kORRType_Cxld: {
            DBGLOG_TIME_CLASS_FUNC << "kORRType_Cxld    SAOS : " << orsreply_.server_assigned_order_sequence_
                                   << " SYM:  " << orsreply_.symbol_
                                   << " CAOS: " << orsreply_.client_assigned_order_sequence_
                                   << " bs: " << (orsreply_.buysell_ == HFSAT::kTradeTypeBuy ? "BUY" : "SELL")
                                   //  << " sz: " << orsreply_.size_requested_
                                   << " px: " << orsreply_.price_ << " intpx: " << orsreply_.int_price_
                                   << " client_position_ : " << orsreply_.client_position_
                                   << " global_position_ : " << orsreply_.global_position_ << DBGLOG_ENDL_FLUSH;
          }

          break;
          case HFSAT::kORRType_Exec: {
            DBGLOG_TIME_CLASS_FUNC << "kORRType_Exec    SAOS : " << orsreply_.server_assigned_order_sequence_
                                   << " SYM:  " << orsreply_.symbol_
                                   << " CAOS: " << orsreply_.client_assigned_order_sequence_
                                   << " bs: " << (orsreply_.buysell_ == HFSAT::kTradeTypeBuy ? "BUY" : "SELL")
                                   //<< " sz: " << orsreply_.size_requested_
                                   << " px: " << orsreply_.price_ << " intpx: " << orsreply_.int_price_
                                   << " executed: " << orsreply_.size_executed_
                                   << " remaining: " << orsreply_.size_remaining_
                                   << " client_position_ : " << orsreply_.client_position_
                                   << " global_position_ : " << orsreply_.global_position_ << DBGLOG_ENDL_FLUSH;
          } break;

          case HFSAT::kORRType_Rejc: {
            DBGLOG_TIME_CLASS_FUNC << "kORRType_Rejc    SAOS : " << orsreply_.server_assigned_order_sequence_
                                   << " SYM:  " << orsreply_.symbol_
                                   << " CAOS: " << orsreply_.client_assigned_order_sequence_
                                   << " bs: " << (orsreply_.buysell_ == HFSAT::kTradeTypeBuy ? "BUY" : "SELL")
                                   //<< " sz: " << orsreply_.size_requested_
                                   << " px: " << orsreply_.price_ << " intpx: " << orsreply_.int_price_
                                   << " executed: " << orsreply_.size_executed_
                                   << " RejReason: " << HFSAT::ORSRejectionReasonStr(
                                                            (HFSAT::ORSRejectionReason_t)orsreply_.size_executed_)
                                   << " remaining: " << orsreply_.size_remaining_ << DBGLOG_ENDL_FLUSH;

            // thread to send alerts
            ors_rejections_alert_handler_.addToRejectionAlertQueue(
                (HFSAT::ORSRejectionReason_t)orsreply_.size_executed_, orsreply_.symbol_);
          } break;

          case HFSAT::kORRType_IntExec: {
            DBGLOG_TIME_CLASS_FUNC << "kORRType_IntExec    SAOS : " << orsreply_.server_assigned_order_sequence_
                                   << " SYM:  " << orsreply_.symbol_
                                   << " CAOS: " << orsreply_.client_assigned_order_sequence_
                                   << " bs: " << (orsreply_.buysell_ == HFSAT::kTradeTypeBuy ? "BUY" : "SELL")
                                   //<< " sz: " << orsreply_.size_requested_
                                   << " px: " << orsreply_.price_ << " intpx: " << orsreply_.int_price_
                                   << " executed: " << orsreply_.size_executed_
                                   << " remaining: " << orsreply_.size_remaining_
                                   << " client_position_ : " << orsreply_.client_position_
                                   << " global_position_ : " << orsreply_.global_position_ << DBGLOG_ENDL_FLUSH;
          } break;

          case HFSAT::kORRType_Rejc:
          default:
            break;
        }
      }
    }
  }

 private:
  HFSAT::MulticastReceiverSocket* socket_;
  std::string ip_;
  int port_;
  HFSAT::GenericORSReplyStruct orsreply_;
  HFSAT::ORSRejectionAlerts ors_rejections_alert_handler_;
  HFSAT::DebugLogger& dbglogger_;
  HFSAT::Watch& watch_;

  std::map<std::string, int> exch_sym_saos_to_cxl_reject_map_;
};

HFSAT::DebugLogger dbglogger_(10240, 1);

void termination_handler(int signum) {
  dbglogger_.Close();
  exit(0);
}

int main(int argc, char** argv) {
  // signal handling, Interrupts and seg faults
  signal(SIGINT, termination_handler);

  if (argc < 3) {
    std::cerr << "usage: " << argv[0] << " <ip> <port> \n";
    exit(0);
  }

  int tradingdate_ = HFSAT::DateTime::GetCurrentIsoDateLocal();

  {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "/spare/local/logs/alllogs/ors_listener_log_" << atoi(argv[2]);
    std::string logfilename_ = t_temp_oss_.str();
    dbglogger_.OpenLogFile(logfilename_.c_str(), std::ofstream::app);
  }
  // moved out to global scope to call close or clean exit in signal handler
  int global_port = atoi(argv[2]);
  std::string global_ip = std::string(argv[1]);
  dbglogger_ << "Opening.. "
             << " IP : " << global_ip << " PORT: " << global_port << "\n";
  dbglogger_.DumpCurrentBuffer();

  HFSAT::Watch watch_(dbglogger_, tradingdate_);

  ORSBroadcastLogger common_logger(global_ip, global_port, dbglogger_, watch_);
  common_logger.processMsgRecvd();
}
