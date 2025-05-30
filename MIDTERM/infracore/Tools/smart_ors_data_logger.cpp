// =====================================================================================
//
//       Filename:  smart_ors_data_logger.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  09/21/2016 05:23:20 AM
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
/**
 * smart_ors_data_logger.cpp
 * Purpose: Listens to ors data multicasts, logs binary/string data, sends voice alerts/email notifications for console
 activity and multiple rejects and logs internal trade executions.

 @author

 @version 1.0
*/
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <fstream>
#include <signal.h>
#include <getopt.h>
#include <map>
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CDef/file_utils.hpp"  // To create the directory
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CDef/trading_location_manager.hpp"
#include "dvccode/CDef/defines.hpp"
#include "dvccode/Utils/multicast_receiver_socket.hpp"
#include "dvccode/Utils/multicast_sender_socket.hpp"
#include "dvccode/CDef/ors_messages.hpp"
#include "dvccode/TradingInfo/network_account_interface_manager.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/ExternalData/simple_live_dispatcher.hpp"
#include "dvccode/TradingInfo/network_account_info_manager.hpp"
#include "dvccode/ExternalData/simple_external_data_live_listener.hpp"
#include "dvccode/Utils/ors_rejections_alert_thread.hpp"
#include "dvccode/Profiler/cpucycle_profiler.hpp"
#include "dvccode/Utils/send_alert.hpp"
#include "dvccode/CDef/email_utils.hpp"
#include "dvccode/Utils/settings.hpp"

#define ORS_BINARY_LOGGED_DATA_PREFIX "/spare/local/ORSBCAST/"
#define ORS_STRING_LOGGED_DATA_PREFIX "/spare/local/logs/alllogs/"
#define ORS_REJECTS_ONLY_DATA_PREFIX "/spare/local/logs/alllogs/"
#define ORS_INTERNAL_EXEC_DATA_PREFIX "/spare/local/logs/alllogs/"
#define ORS_NOTIFIER_DATA_PREFIX "/spare/local/logs/alllogs/"
#define ORS_NOTIFIER_ZABBIX_PREFIX "/spare/local/zabbix/"

#define SMART_ORS_DATA_LOGGER_SETTINGS_PREFIX "/home/pengine/prod/live_configs/"

#define EXPECTED_SETTINGS_PARAM 10
/**
 * This abstract class serves as base class for subscriber objects. It provides interface to the
 * ORSMulticastDataSubscriber class. All derived objects of this class can subscribe and receive and process ors_reply
 * structs from ORSMulticastDataSubscriber.
 */
class ORSDataListener {
 public:
  /**
   * Virtual method, called when ORSMulticastDataSubscriber recieves struct over multicast.
   * @param ors_reply_live	ORS reply struct received from multicast.
   * @param exch_name		Exchange corresponding the recieved struct.
   */
  virtual void OnORSUpdate(HFSAT::GenericORSReplyStructLive& ors_reply_live, std::string const& exch_name) = 0;
  virtual ~ORSDataListener() {}
};

/**
 * This class subscribes to the ORS multicasts, and forwards the struct to the subscribed objects.
 */
class ORSMulticastDataSubscriber : public HFSAT::SimpleExternalDataLiveListener {
 private:
  std::map<int32_t, HFSAT::MulticastReceiverSocket*>& socket_fd_to_socket_map_;
  std::map<int32_t, std::string>& socket_fd_to_exchname_map_;
  HFSAT::GenericORSReplyStructLive ors_reply_live_;
  std::vector<ORSDataListener*> ors_data_listener_vec_;

 public:
  ORSMulticastDataSubscriber(std::map<int32_t, HFSAT::MulticastReceiverSocket*>& soc_fd_to_soc_map,
                             std::map<int32_t, std::string>& soc_to_exchname_map)
      : socket_fd_to_socket_map_(soc_fd_to_soc_map),
        socket_fd_to_exchname_map_(soc_to_exchname_map),
        ors_reply_live_(),
        ors_data_listener_vec_() {}

  void AddORSDataListener(ORSDataListener* ors_data_listener) {
    HFSAT::VectorUtils::UniqueVectorAdd(ors_data_listener_vec_, ors_data_listener);
  }

  void ProcessAllEvents(int32_t socket_fd) {
    // unexpected
    if (socket_fd_to_socket_map_.end() == socket_fd_to_socket_map_.find(socket_fd)) return;

    int32_t read_length =
        socket_fd_to_socket_map_[socket_fd]->ReadN(sizeof(HFSAT::GenericORSReplyStructLive), (void*)&ors_reply_live_);
    if (read_length < (int32_t)sizeof(HFSAT::GenericORSReplyStructLive)) return;

    for (auto& itr : ors_data_listener_vec_) {
      itr->OnORSUpdate(ors_reply_live_, socket_fd_to_exchname_map_[socket_fd]);
    }
  }
};
/**
 * This class dumps string form of GenericORSReplyStructLive from multicasts to file.
 */
class ORSBroadcastLogger : public ORSDataListener {
 private:
  HFSAT::DebugLogger dbglogger_;

 public:
  /**
   * Constructor of ORSBroadcastLogger. Creates instance of HFSAT::DebugLogger() for dumping string form of ORS reply
   * structs to a file.
   * File path: ORS_STRING_LOGGED_DATA_PREFIX.
   * File name: ors_broadcast_logger_YYMMDD.log
   */
  ORSBroadcastLogger() : dbglogger_(10240, 1) {
    std::ostringstream t_temp_oss;
    t_temp_oss << ORS_STRING_LOGGED_DATA_PREFIX << "ors_broadcast_logger_" << HFSAT::DateTime::GetCurrentIsoDateLocal()
               << ".log";
    dbglogger_.OpenLogFile(t_temp_oss.str().c_str(), std::ofstream::app);
  }

  /**
   * This function dumps string form of ors_reply_live to a file.
   * @param ors_reply_live	ORS reply struct received from multicast.
   * @param exch_name		Exchange corresponding the recieved struct.
   */
  void OnORSUpdate(HFSAT::GenericORSReplyStructLive& ors_reply_live, std::string const& exch_name) {
    dbglogger_ << ors_reply_live.ToString();
    dbglogger_.CheckToFlushBuffer();

    // flush the buffer on rejects
    if (HFSAT::kORRType_Rejc == ors_reply_live.orr_type_ || HFSAT::kORRType_CxlRejc == ors_reply_live.orr_type_ ||
        HFSAT::kORRType_CxReRejc == ors_reply_live.orr_type_) {
      dbglogger_.DumpCurrentBuffer();
    }
  }
  /**
   * Closes the debug logger file handle.
   */
  void CleanUp() { dbglogger_.Close(); }
};
/**
 * This class dumps serialized GenericORSReplyStructLive from multicasts to file.
 */
class ORSBinaryLogger : public ORSDataListener {
 private:
  std::map<std::string, std::ofstream*> exch_symbol_to_ors_binfile_map_;  /// Stores Symbol to FileStream(File where
                                                                          /// symbol-wise ors binary data is dumped)
                                                                          /// mapping

  /**
   *  This functions creates and updates file handle for dumping ORS replies symbol-wise. The file path is given by
   * ORS_BINARY_LOGGED_DATA_PREFIX/exch_name/symbol_yymmdd
   * @param symbol      Symbol name for which the file needs to be created
   * @param exch_name   Exchange name for the symbol. Required for creating directory structure.
   */
  void CreateNewORSBinFile(std::string const& symbol, std::string const& exch_name) {
    std::ostringstream bin_file_str;
    bin_file_str << ORS_BINARY_LOGGED_DATA_PREFIX << exch_name << "/" << symbol << "_"
                 << HFSAT::DateTime::GetCurrentIsoDateLocal();
    std::ofstream* new_ors_bin_file = new std::ofstream();

    HFSAT::FileUtils::MkdirEnclosing(bin_file_str.str());  /// Creates the directory if not exists
    new_ors_bin_file->open(bin_file_str.str().c_str(), std::ofstream::binary | std::ofstream::app | std::ofstream::ate);
    if (!new_ors_bin_file->is_open()) {
      std::cerr << "ORS Binary Reader Failed To Open File : " << bin_file_str.str()
                << " For Writing, SysError : " << strerror(errno) << std::endl;
      exit(-1);
    }
    exch_symbol_to_ors_binfile_map_[symbol] = new_ors_bin_file;
  }

 public:
  /**
   * This function dumps serialized ors_reply_live to a file.
   * @param ors_reply_live	ORS reply struct received from multicast.
   * @param exch_name		Exchange corresponding the recieved struct.
   */
  void OnORSUpdate(HFSAT::GenericORSReplyStructLive& ors_reply_live, std::string const& exch_name) {
    ors_reply_live.time_set_by_server_ = HFSAT::GetTimeOfDay();

    std::string symbol = ors_reply_live.symbol_;
    if (std::string::npos != symbol.find(" ")) {
      std::replace(symbol.begin(), symbol.end(), ' ', '~');
    }

    if (exch_symbol_to_ors_binfile_map_.end() == exch_symbol_to_ors_binfile_map_.find(symbol)) {
      /// Create New File is symbol to dump filestream mapping is not present
      CreateNewORSBinFile(symbol, exch_name);
    }

    std::ofstream* bin_file = exch_symbol_to_ors_binfile_map_[symbol];
    bin_file->write((char*)&ors_reply_live, sizeof(HFSAT::GenericORSReplyStructLive));
    bin_file->flush();
  }

  void CleanUp() {}
};
/**
 * This class logs ORS rejects from multicasts to file in string format.
 */
class ORSRejectsOnlyLogger : public ORSDataListener {
 private:
  HFSAT::DebugLogger dbglogger_;

 public:
  /**
   * Constructor of ORSRejectsOnlyLogger. Creates instance of HFSAT::DebugLogger() for dumping string form of ORS
   * rejects structs to a file.
   * File path: ORS_REJECTS_ONLY_DATA_PREFIX.
   * File name: ors_rejects_logger_YYMMDD.log
   */
  ORSRejectsOnlyLogger() : dbglogger_(10240, 1) {
    std::ostringstream t_temp_oss;
    t_temp_oss << ORS_REJECTS_ONLY_DATA_PREFIX << "ors_rejects_logger_" << HFSAT::DateTime::GetCurrentIsoDateLocal()
               << ".log";
    dbglogger_.OpenLogFile(t_temp_oss.str().c_str(), std::ofstream::app);
  }
  /**
   * This function dumps string form of ors_reply_live(rejects only) to a file.
   * @param ors_reply_live  ORS reply struct received from multicast.
   * @param exch_name       Exchange corresponding the recieved struct.
   */
  void OnORSUpdate(HFSAT::GenericORSReplyStructLive& ors_reply_live, std::string const& exch_name) {
    if (HFSAT::kORRType_Rejc == ors_reply_live.orr_type_ || HFSAT::kORRType_CxlRejc == ors_reply_live.orr_type_ ||
        HFSAT::kORRType_CxReRejc == ors_reply_live.orr_type_) {
      /// Dump ors_reply_live of rejects types only.
      dbglogger_ << ors_reply_live.ToString();
      dbglogger_.DumpCurrentBuffer();
    }
  }
};
/**
 * This class logs internal execution trades matched by the ors. These trades are used by pnls.txt setup to compute
 * tagwise pnls.
 */
class ORSInternalExecLogger : public ORSDataListener {
 private:
  HFSAT::DebugLogger dbglogger_;

 public:
  /**
   * Constructor of ORSInternalExecLogger. Creates instance of HFSAT::DebugLogger() for dumping internal execution
   * trades.
   * File path: ORS_REJECTS_ONLY_DATA_PREFIX
   * File name: internal_trades.YYMMDD
   */
  ORSInternalExecLogger() : dbglogger_(10240, 1) {
    std::ostringstream t_temp_oss;
    t_temp_oss << ORS_REJECTS_ONLY_DATA_PREFIX << "internal_trades." << HFSAT::DateTime::GetCurrentIsoDateLocal();
    dbglogger_.OpenLogFile(t_temp_oss.str().c_str(), std::ofstream::app);
  }
  /**
   * This function internal execution trades to a file.
   * @param ors_reply_live  ORS reply struct received from multicast.
   * @param exch_name       Exchange corresponding the recieved struct.
   */
  void OnORSUpdate(HFSAT::GenericORSReplyStructLive& t_ors_reply, std::string const& exch_name) {
    if (HFSAT::ORRType_t::kORRType_IntExec == t_ors_reply.orr_type_) {
      /// Dump iff t_ors_reply is kORRType_IntExec
      char delim = (char)1;
      std::ostringstream temp_oss;
      temp_oss << t_ors_reply.symbol_ << delim << t_ors_reply.buysell_ << delim << t_ors_reply.size_executed_ << delim
               << t_ors_reply.price_ << delim << t_ors_reply.server_assigned_order_sequence_ << delim
               << t_ors_reply.exch_assigned_sequence_ << delim << t_ors_reply.time_set_by_server_.tv_sec << "."
               << std::setw(6) << std::setfill('0') << t_ors_reply.time_set_by_server_.tv_usec << delim
               << t_ors_reply.server_assigned_client_id_ << delim;
      dbglogger_ << temp_oss.str().c_str() << "\n";
      dbglogger_.DumpCurrentBuffer();
    }
  }
};

class ORSDataStatsComputer : public ORSDataListener {};
/**
 * This class monitors console activities and ors rejects and sends suitable voice/email alerts.
 */
class ORSRejectsAndConsoleActivityNotifier : public ORSDataListener {
 private:
  bool is_console_active_;         /// Flag to monitor console activity. If set true send alerts for console activity.
  bool is_mult_cxlrej_active_;     /// If set true send alerts for multiple cxl rejects.
  int32_t mult_cxlrej_threshold_;  /// This is the threhold value for multiple cxl rejects. Send alerts iff rejects
                                   /// count >= mult_cxlrej_threshold_.
  bool is_voice_active_;           /// If true, send voice alerts.
  bool is_zabbix_active_;          /// If true, send zabbix alerts. Currently not functional.
  bool is_mail_active_;            /// If true, send mail alerts.
  HFSAT::DebugLogger activity_notifier_;
  std::ofstream console_activity_notifier_;
  std::string rejects_notifier_filename_;
  std::ofstream rejects_notifier_file_;
  std::string multcxl_rejects_notifier_filename_;
  std::ofstream multcxl_rejects_notifier_file_;
  char this_hostname_[64];
  std::map<std::string, int> exch_sym_saos_to_conf_map_;
  std::map<std::string, int> exch_sym_saos_to_cxl_reject_map_;
  struct timeval last_reject_time_;
  HFSAT::ORSRejectionAlerts ors_rejections_alert_handler_;

 public:
  /**
   * Constructor of ORSRejectsAndConsoleActivityNotifier. Creates log files for ors rejects, console activity.
   * trades, multiple cxl rejects.
   * @param console
   * @param multcxlrej
   * @param multcxlthrs
   * @param voice_alerts
   * @param zabbix_alerts
   * @param mail_alerts
   */
  ORSRejectsAndConsoleActivityNotifier(bool console, bool multcxlrej, int32_t multcxlthrs, bool voice_alerts,
                                       bool zabbix_alerts, bool mail_alerts)
      : is_console_active_(console),
        is_mult_cxlrej_active_(multcxlrej),
        mult_cxlrej_threshold_(multcxlthrs),
        is_voice_active_(voice_alerts),
        is_zabbix_active_(zabbix_alerts),
        is_mail_active_(mail_alerts),
        activity_notifier_(10240, 1),
        console_activity_notifier_(),
        rejects_notifier_filename_(),
        rejects_notifier_file_(),
        multcxl_rejects_notifier_filename_(),
        multcxl_rejects_notifier_file_(),
        this_hostname_(),
        exch_sym_saos_to_conf_map_(),
        exch_sym_saos_to_cxl_reject_map_(),
        last_reject_time_() {
    std::ostringstream t_temp_oss;
    t_temp_oss << ORS_NOTIFIER_DATA_PREFIX << "ors_console_activity_notifier_"
               << HFSAT::DateTime::GetCurrentIsoDateLocal() << ".log";
    console_activity_notifier_.open(t_temp_oss.str().c_str(), std::ofstream::app);
    if (!console_activity_notifier_.is_open()) {
      std::cerr << "Failed To Open Console Activity File For Writing : " << t_temp_oss.str()
                << " SysError : " << strerror(errno) << std::endl;
      exit(-1);
    }

    t_temp_oss.str("");
    t_temp_oss << ORS_NOTIFIER_DATA_PREFIX << "ors_activity_notifier_" << HFSAT::DateTime::GetCurrentIsoDateLocal()
               << ".log";
    activity_notifier_.OpenLogFile(t_temp_oss.str().c_str(), std::ofstream::app);

    t_temp_oss.str("");
    t_temp_oss << ORS_NOTIFIER_DATA_PREFIX << "ors_rejects_activity_notifier_"
               << HFSAT::DateTime::GetCurrentIsoDateLocal() << ".log";
    rejects_notifier_filename_ = t_temp_oss.str();

    rejects_notifier_file_.open(rejects_notifier_filename_.c_str(), std::ofstream::app);
    if (!rejects_notifier_file_.is_open()) {
      std::cerr << "Failed To Open Rejection Notifier File For Writing : " << rejects_notifier_filename_
                << " SysError : " << strerror(errno) << std::endl;
      exit(-1);
    }

    t_temp_oss.str("");
    t_temp_oss << ORS_NOTIFIER_DATA_PREFIX << "ors_multcxl_rejects_notifier_"
               << HFSAT::DateTime::GetCurrentIsoDateLocal() << ".log";
    multcxl_rejects_notifier_filename_ = t_temp_oss.str();

    multcxl_rejects_notifier_file_.open(multcxl_rejects_notifier_filename_.c_str(), std::ofstream::app);
    if (!multcxl_rejects_notifier_file_.is_open()) {
      std::cerr << "Failed To Open MultCxlRejects Notifier File For Writing : " << multcxl_rejects_notifier_filename_
                << " SysError : " << strerror(errno) << std::endl;
      exit(-1);
    }

    memset((void*)this_hostname_, 0, sizeof(this_hostname_));
    gethostname(this_hostname_, sizeof(this_hostname_) - 1);
    ors_rejections_alert_handler_.run();
  }
  /**
   * Send alerts for console activities depending on the flags.
   */
  void ProcessConsoleActivity(HFSAT::GenericORSReplyStructLive& ors_reply_live) {
    if (!is_console_active_) return;  /// Don't log console activity. is_console_active_ not true.

    activity_notifier_ << "CONSOLE @ : " << this_hostname_ << " " << ors_reply_live.ToString() << "\n";
    activity_notifier_.DumpCurrentBuffer();  /// Dump to file
    if (HFSAT::ORRType_t::kORRType_Exec == ors_reply_live.orr_type_ ||
        HFSAT::ORRType_t::kORRType_IntExec == ors_reply_live.orr_type_) {
      /// Send alerts only on ors_execution message and internal execution.

      if (is_zabbix_active_) {
        /// Send Zabbix alerts
        console_activity_notifier_ << this_hostname_ << " " << ors_reply_live.ToString();
        console_activity_notifier_.flush();
      }

      if (is_voice_active_) {
        /// Send voice alerts
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << (ors_reply_live.buysell_ == 0 ? "Buy " : "Sell ") << ors_reply_live.size_executed_
                    << " Units of " << ors_reply_live.symbol_ << "@" << ors_reply_live.price_ << " Size Remaining "
                    << ors_reply_live.size_remaining_;
        HFSAT::SendAlert::sendAlert(std::string("INFO: ") + std::string("Console Activity Detected: ") +
                                    t_temp_oss_.str());
      }

      if (is_mail_active_) {
        /// Send email alerts
        HFSAT::Email e;
        e.setSubject("Console Activity Detected");
        e.addRecepient("nseall@tworoads.co.in");
        e.addSender("nseall@tworoads.co.in");
        string execution_type;
        if (HFSAT::ORRType_t::kORRType_Exec == ors_reply_live.orr_type_) {
          execution_type = "Exchange Execution";
        } else if (HFSAT::ORRType_t::kORRType_IntExec == ors_reply_live.orr_type_) {
          execution_type = "Internal Execution";
        }
        e.content_stream << (std::string(" Received ") + execution_type + std::string(" For : ") +
                             ors_reply_live.ToString());
        e.sendMail();
      }
    }
  }
  /**
   * Send alerts for multiple cxl alerts depending on the flags and threshold values.
   */
  void ProcessORSReplyAndCheckForMultipleCxlRejects(HFSAT::GenericORSReplyStructLive& ors_reply_live) {
    if (!is_mult_cxlrej_active_) return;

    std::ostringstream t_temp_oss;
    t_temp_oss << ors_reply_live.symbol_ << "-" << ors_reply_live.server_assigned_order_sequence_;

    if (HFSAT::kORRType_Conf == ors_reply_live.orr_type_) {
      exch_sym_saos_to_conf_map_[t_temp_oss.str()] = 1;
    }

    if (HFSAT::kORRType_CxlRejc == ors_reply_live.orr_type_) {
      if (HFSAT::kCxlRejectReasonThrottle != (HFSAT::CxlRejectReason_t)ors_reply_live.size_executed_ &&
          HFSAT::kCxlRejectReasonTAPThrottle != (HFSAT::CxlRejectReason_t)ors_reply_live.size_executed_ &&
          HFSAT::kCxlRejectReasonOrderNotConfirmed != (HFSAT::CxlRejectReason_t)ors_reply_live.size_executed_) {
        if (exch_sym_saos_to_conf_map_.end() != exch_sym_saos_to_conf_map_.find(t_temp_oss.str())) {
          if (exch_sym_saos_to_cxl_reject_map_.end() == exch_sym_saos_to_cxl_reject_map_.find(t_temp_oss.str())) {
            exch_sym_saos_to_cxl_reject_map_[t_temp_oss.str()] = 1;
          } else {
            exch_sym_saos_to_cxl_reject_map_[t_temp_oss.str()]++;

            if (exch_sym_saos_to_cxl_reject_map_[t_temp_oss.str()] >= mult_cxlrej_threshold_) {
              /// Send Alerts iff rejection count for saos greater that mult_cxlrej_threshold_
              if (is_zabbix_active_) {
                /// Send Zabbix alerts
                multcxl_rejects_notifier_file_ << this_hostname_ << " " << ors_reply_live.ToString() << std::endl;
                multcxl_rejects_notifier_file_.flush();
              }

              if (is_voice_active_) {
                /// Send Voice Alerts
                std::ostringstream rej_oss;
                rej_oss << ors_reply_live.symbol_ << " With SAOS : " << ors_reply_live.server_assigned_order_sequence_;
                HFSAT::SendAlert::sendAlert(std::string("ALERT : ") + std::string(" MultCxlRejects Detected On, ") +
                                            std::string(this_hostname_) + std::string(" For ") + rej_oss.str());
              }

              if (is_mail_active_) {
                /// Send Email Alerts
                HFSAT::Email e;
                std::ostringstream alert_msg_str_;
                alert_msg_str_ << " Multiple CxlRejects Received For : " << ors_reply_live.symbol_
                               << " Possibly Dead Order : " << ors_reply_live.server_assigned_order_sequence_
                               << " Total Cxl Rejects : " << exch_sym_saos_to_cxl_reject_map_[t_temp_oss.str()];
                e.setSubject("Possible Dead Order Detection");
                e.addRecepient("nseall@tworoads.co.in");
                e.addSender("nseall@tworoads.co.in");
                e.content_stream << alert_msg_str_.str();
                e.sendMail();
              }

              activity_notifier_ << "MultCxlRejects @ : " << this_hostname_
                                 << " With This CxlRej In Given Product And SAOS Pair " << ors_reply_live.ToString()
                                 << "\n";
              activity_notifier_.DumpCurrentBuffer();  /// Dump to logs
            }
          }
        }
      }
    }
  }

  void ProcessORSRejects(HFSAT::GenericORSReplyStructLive& ors_reply_live) {
    if (is_zabbix_active_) {
      /// Send Zabbix alerts
      rejects_notifier_file_ << this_hostname_ << " " << ors_reply_live.ToString();
    }

    if (is_voice_active_) {
      /// Send Voice Alerts
      if (HFSAT::kTAPRejectThrottleLimitReached != (HFSAT::ORSRejectionReason_t)ors_reply_live.size_executed_) {
        // Only if it's not TAP throttle
        ors_rejections_alert_handler_.addToRejectionAlertQueue(
            (HFSAT::ORSRejectionReason_t)ors_reply_live.size_executed_, ors_reply_live.symbol_);
        /// thread to send alerts
      }
    }

    activity_notifier_ << "ORS REJECTS ON : " << this_hostname_ << " " << ors_reply_live.ToString() << "\n";
    activity_notifier_.DumpCurrentBuffer();  /// Dump to logs
  }

  void OnORSUpdate(HFSAT::GenericORSReplyStructLive& ors_reply_live, std::string const& exch_name) {
    if (ors_reply_live.client_assigned_order_sequence_ >= CONSOLE_ORDERS_START_SEQUENCE) {
      /// Console Activity
      ProcessConsoleActivity(ors_reply_live);
    }

    if (HFSAT::kORRType_CxlRejc == ors_reply_live.orr_type_ || HFSAT::kORRType_Conf == ors_reply_live.orr_type_) {
      /// Multiple Cxl Rejects
      ProcessORSReplyAndCheckForMultipleCxlRejects(ors_reply_live);
    }

    if (HFSAT::kORRType_Rejc == ors_reply_live.orr_type_) {
      /// ORR Rejects
      ProcessORSRejects(ors_reply_live);
    }
  }
};

/**
 * This class multicasts GenericORSReplyStructLive from over specified IP X Port.
 */
class ORSReplyMulticaster : public ORSDataListener {
 private:
  HFSAT::MulticastSenderSocket* mcast_sender_socket_;

 public:
  bool add_listener_;
  /**
   * Constructor of ORSBroadcastLogger.
   * Creates instance of MCast Sender Socket for multicasting ORS reply.
   */
  ORSReplyMulticaster() {
    HFSAT::NetworkAccountInfoManager network_account_info_manager;
    HFSAT::DataInfo data_info =
        network_account_info_manager.GetSrcDataInfo("ORS_SLOW");  /// Get ors broadcast ip port info

    if (IsSrcDstORSDataInfoSame(data_info)) {
      add_listener_ = false;
      mcast_sender_socket_ = nullptr;
    } else {
      add_listener_ = true;
      mcast_sender_socket_ = new HFSAT::MulticastSenderSocket(
          data_info.bcast_ip_, data_info.bcast_port_,
          HFSAT::NetworkAccountInterfaceManager::instance().GetInterfaceForApp(HFSAT::k_ORSSlow));
    }
  }

  /**
   * This function muticasts ors_reply_live.
   * @param ors_reply_live	ORS reply struct received from multicast.
   * @param exch_name		Exchange corresponding the recieved struct.
   */
  void OnORSUpdate(HFSAT::GenericORSReplyStructLive& ors_reply_live, std::string const& exch_name) {
    mcast_sender_socket_->WriteN(sizeof(ors_reply_live), &ors_reply_live);
  }

  bool IsSrcDstORSDataInfoSame(HFSAT::DataInfo data_info_dst) {
    HFSAT::NetworkAccountInfoManager network_account_info_manager;
    HFSAT::TradingLocation_t curr_location_ = HFSAT::TradingLocationUtils::GetTradingLocationFromHostname();
    HFSAT::ExchSource_t current_exch_source_ = HFSAT::kExchSourceMAX;
    std::string dummy_shortcode_ = "";

    switch (curr_location_) {
      case HFSAT::kTLocCHI: {
        current_exch_source_ = HFSAT::kExchSourceCME;
        dummy_shortcode_ = "ZN_0";
      } break;
      case HFSAT::kTLocTMX: {
        current_exch_source_ = HFSAT::kExchSourceTMX;
        dummy_shortcode_ = "CGB_0";
      } break;
      case HFSAT::kTLocFR2: {
        current_exch_source_ = HFSAT::kExchSourceEUREX;
        dummy_shortcode_ = "FESX_0";
      } break;
      case HFSAT::kTLocBMF: {
        current_exch_source_ = HFSAT::kExchSourceBMF;
        dummy_shortcode_ = "BR_DOL_0";
      } break;
      case HFSAT::kTLocBSL: {
        current_exch_source_ = HFSAT::kExchSourceLIFFE;
        dummy_shortcode_ = "LFR_0";
      } break;
      case HFSAT::kTLocJPY: {
        current_exch_source_ = HFSAT::kExchSourceJPY;
        dummy_shortcode_ = "NK_0";
      } break;
      case HFSAT::kTLocHK: {
        current_exch_source_ = HFSAT::kExchSourceHONGKONG;
        dummy_shortcode_ = "HSI_0";
      } break;
      case HFSAT::kTLocCFE: {
        current_exch_source_ = HFSAT::kExchSourceCFE;
        dummy_shortcode_ = "VX_0";
      } break;
      case HFSAT::kTLocNY4: {
        current_exch_source_ = HFSAT::kExchSourceCME;
        dummy_shortcode_ = "ZN_0";
      } break;
      case HFSAT::kTLocSYD: {
        current_exch_source_ = HFSAT::kExchSourceASX;
        dummy_shortcode_ = "XT_0";
      } break;
      case HFSAT::kTLocSPR: {
        current_exch_source_ = HFSAT::kExchSourceSGX;
        dummy_shortcode_ = "SGX_NK_0";
      } break;
      case HFSAT::kTLocNSE: {
        current_exch_source_ = HFSAT::kExchSourceNSE;
        dummy_shortcode_ = "NSE_NIFTY_FUT0";
      } break;

      default: {
        current_exch_source_ = HFSAT::kExchSourceMAX;
        dummy_shortcode_ = "";
      } break;
    }

    HFSAT::DataInfo data_info_src = network_account_info_manager.GetDepDataInfo(current_exch_source_, dummy_shortcode_);

    if (data_info_src.bcast_ip_ == data_info_dst.bcast_ip_ || data_info_src.bcast_port_ == data_info_dst.bcast_port_) {
      return true;
    }

    return false;
  }

  /**
   * Closes the debug logger file handle.
   */
  void CleanUp() {
    if (nullptr != mcast_sender_socket_) {
      delete mcast_sender_socket_;
      mcast_sender_socket_ = nullptr;
    }
  }
};

int main(int argc, char* argv[]) {
  /***
   * Flags for setting what information to log. These flags are set using the host specific
   * smart_ors_data_logger_settings.config file
   */
  bool are_we_logging_ors_binary_data = false;
  bool are_we_logging_ors_string_data = false;
  bool are_we_logigng_ors_rejects_only_data = false;
  bool are_we_sending_console_activity_notifs = false;
  bool are_we_sending_multiple_cxlrejects_notifs = false;
  bool are_we_using_zabbix_alerts_for_ors_rejects = false;
  bool are_we_using_mail_alerts_for_ors_rejects = false;
  bool are_we_using_voice_alerts_for_ors_rejects = false;
  bool are_we_logging_internal_exec_data = false;
  bool are_we_multicasting_ors_reply_data = false;
  int32_t cxl_rejects_threshold_for_same_saos = 3;
  /// Threshold value for multiple cxl rejects. Sends alerts iff rejects count >= this value.
  bool is_any_mode_active = false;
  /// Used to verify if the ors_logger is monitoring/logging any of the above. Exit if false.

  HFSAT::CpucycleProfiler::SetUniqueInstance(10);  /// This line is should be added before GetUniqueInstance
  char hostname[128];
  hostname[127] = '\0';
  gethostname(hostname, 127);

  std::ostringstream t_temp_oss;
  /// File containing values for setting monitoring flags. This file tells logger what to monitor.
  t_temp_oss << SMART_ORS_DATA_LOGGER_SETTINGS_PREFIX << hostname << "_smart_ors_data_logger_settings.cfg";
  std::cout << "Reading settings from config file: " << t_temp_oss.str() << std::endl;

  HFSAT::ORS::Settings settings(t_temp_oss.str());

  /// Setting the monitoring flag values.
  are_we_logging_ors_binary_data =
      settings.has("LOG_ORS_BINARY_DATA") && (settings.getIntValue("LOG_ORS_BINARY_DATA", 0) == 1);

  are_we_logging_ors_string_data =
      settings.has("LOG_ORS_STRING_DATA") && (settings.getIntValue("LOG_ORS_STRING_DATA", 0) == 1);

  are_we_logigng_ors_rejects_only_data =
      settings.has("LOG_ORS_REJECTS_ONLY_DATA") && (settings.getIntValue("LOG_ORS_REJECTS_ONLY_DATA", 0) == 1);

  are_we_sending_console_activity_notifs =
      settings.has("SEND_CONSOLE_ACTIVITY_NOTIF") && (settings.getIntValue("SEND_CONSOLE_ACTIVITY_NOTIF", 0) == 1);

  are_we_sending_multiple_cxlrejects_notifs =
      settings.has("SEND_MULTIPLE_CXLREJECTS_NOTIF") && (settings.getIntValue("SEND_MULTIPLE_CXLREJECTS_NOTIF", 0) > 0);

  are_we_using_voice_alerts_for_ors_rejects = settings.has("USE_VOICE_ALERTS_FOR_ORS_REJECTS") &&
                                              (settings.getIntValue("USE_VOICE_ALERTS_FOR_ORS_REJECTS", 0) == 1);

  are_we_using_zabbix_alerts_for_ors_rejects = settings.has("USE_ZABBIX_ALERTS_FOR_ORS_REJECTS") &&
                                               (settings.getIntValue("USE_ZABBIX_ALERTS_FOR_ORS_REJECTS", 0) == 1);

  are_we_using_mail_alerts_for_ors_rejects = settings.has("USE_MAIL_ALERTS_FOR_ORS_REJECTS") &&
                                             (settings.getIntValue("USE_MAIL_ALERTS_FOR_ORS_REJECTS", 0) == 1);

  are_we_logging_internal_exec_data =
      settings.has("LOG_INTERNAL_EXEC_DATA") && (settings.getIntValue("LOG_INTERNAL_EXEC_DATA", 0) == 1);

  are_we_multicasting_ors_reply_data =
      settings.has("MULTICAST_ORS_REPLY_DATA") && (settings.getIntValue("MULTICAST_ORS_REPLY_DATA", 0) == 1);

  if (true == are_we_sending_multiple_cxlrejects_notifs) {
    cxl_rejects_threshold_for_same_saos = std::max(3, settings.getIntValue("SEND_MULTIPLE_CXLREJECTS_NOTIF", 0));
  }

  is_any_mode_active = (are_we_logging_ors_binary_data | are_we_logging_ors_string_data |
                        are_we_logigng_ors_rejects_only_data | are_we_sending_console_activity_notifs |
                        are_we_sending_multiple_cxlrejects_notifs | are_we_using_voice_alerts_for_ors_rejects |
                        are_we_using_zabbix_alerts_for_ors_rejects | are_we_using_mail_alerts_for_ors_rejects |
                        are_we_logging_internal_exec_data | are_we_multicasting_ors_reply_data);
  if (!is_any_mode_active) {
    /// Run smart_ors_data_logger iff we are monitoring atleast one thing.
    std::cerr << "ORSSmartDataLogger Asked To Run For Nothing, Please Verify Config !!" << std::endl;
    exit(-1);
  }

#define MAX_LINE_BUFFER_SIZE 1024

  t_temp_oss.str("");
  /// This file provides list of keys(Exchanges) for fetching the ORS Broadcast IP Port
  t_temp_oss << SMART_ORS_DATA_LOGGER_SETTINGS_PREFIX << hostname << "_smart_ors_data_logger_config.cfg";

  std::vector<HFSAT::ExchSource_t> exchange_list_for_ors_data_vec;

  std::ifstream ors_logger_config_stream;
  ors_logger_config_stream.open(t_temp_oss.str().c_str());
  if (!ors_logger_config_stream.is_open()) {
    std::cerr << "Unable To Open SmartORSDataLogger Config File : " << t_temp_oss.str() << std::endl;
    exit(-1);
  }

  char line_buffer[MAX_LINE_BUFFER_SIZE];
  while (ors_logger_config_stream.good()) {
    ors_logger_config_stream.getline(line_buffer, MAX_LINE_BUFFER_SIZE);
    if (std::string::npos != std::string(line_buffer).find("#")) continue;

    if (HFSAT::kExchSourceInvalid == HFSAT::StringToExchSource(line_buffer)) continue;
    exchange_list_for_ors_data_vec.push_back(HFSAT::StringToExchSource(line_buffer));
  }

  ors_logger_config_stream.close();

  if (exchange_list_for_ors_data_vec.size() == 0) {
    std::cerr << "No Sources To Run The Exec... " << std::endl;
    exit(-1);
  }

  HFSAT::SimpleLiveDispatcher sld;
  HFSAT::NetworkAccountInfoManager network_account_info_manager;
  std::map<int32_t, HFSAT::MulticastReceiverSocket*> socket_fd_to_socket_map;
  std::map<int32_t, std::string> socket_fd_to_exchname_map;

  for (auto& itr : exchange_list_for_ors_data_vec) {
    std::string exch_name = HFSAT::ExchSourceStringForm(itr);
    if (std::string("INVALID") == exch_name) continue;
    HFSAT::DataInfo data_info =
        network_account_info_manager.GetDepDataInfo(itr, "dummy_0");  /// Get ors broadcast ip port info
    HFSAT::MulticastReceiverSocket* mcast_receiver_socket = new HFSAT::MulticastReceiverSocket(
        data_info.bcast_ip_, data_info.bcast_port_,
        HFSAT::NetworkAccountInterfaceManager::instance().GetInterfaceForApp(HFSAT::k_ORS_LS));

    socket_fd_to_socket_map[mcast_receiver_socket->socket_file_descriptor()] = mcast_receiver_socket;
    socket_fd_to_exchname_map[mcast_receiver_socket->socket_file_descriptor()] = exch_name;
  }

  ORSMulticastDataSubscriber ors_multicast_data_subscriber(socket_fd_to_socket_map, socket_fd_to_exchname_map);

  for (auto& itr : socket_fd_to_socket_map) {
    sld.AddSimpleExternalDataLiveListenerSocket(&ors_multicast_data_subscriber, itr.first, true);
  }
  /// Depending on the monitoring flags add objects to Data Listner list
  if (true == are_we_logging_ors_binary_data) {
    /// If logging ors binary data
    ORSDataListener* ors_bin_data_logger = new ORSBinaryLogger();
    ors_multicast_data_subscriber.AddORSDataListener(ors_bin_data_logger);
  }

  if (true == are_we_logging_ors_string_data) {
    /// if logging ors string data
    ORSDataListener* ors_string_data_logger = new ORSBroadcastLogger();
    ors_multicast_data_subscriber.AddORSDataListener(ors_string_data_logger);
  }

  if (true == are_we_logigng_ors_rejects_only_data) {
    /// If logging ors rejects data
    ORSDataListener* ors_rejects_only_logger = new ORSRejectsOnlyLogger();
    ors_multicast_data_subscriber.AddORSDataListener(ors_rejects_only_logger);
  }

  if (true == are_we_logging_internal_exec_data) {
    /// If logging internal execution data
    ORSDataListener* ors_internal_exec_logger = new ORSInternalExecLogger();
    ors_multicast_data_subscriber.AddORSDataListener(ors_internal_exec_logger);
  }

  if (true == are_we_sending_console_activity_notifs || true == are_we_sending_multiple_cxlrejects_notifs) {
    /// If logging console activity or multiple cxl_rejects
    ORSDataListener* ors_notifier = new ORSRejectsAndConsoleActivityNotifier(
        are_we_sending_console_activity_notifs, are_we_sending_multiple_cxlrejects_notifs,
        cxl_rejects_threshold_for_same_saos, are_we_using_voice_alerts_for_ors_rejects,
        are_we_using_zabbix_alerts_for_ors_rejects, are_we_using_mail_alerts_for_ors_rejects);

    ors_multicast_data_subscriber.AddORSDataListener(ors_notifier);
  }

  if (true == are_we_multicasting_ors_reply_data) {
    // multicast ORS reply
    ORSReplyMulticaster* ors_reply_multicaster = new ORSReplyMulticaster();
    if (ors_reply_multicaster->add_listener_) {
      ors_multicast_data_subscriber.AddORSDataListener(ors_reply_multicaster);
    }
  }

  sld.RunLive();

#undef MAX_LINE_BUFFER_SIZE

  return EXIT_SUCCESS;
}  // ----------  end of function main  ----------
