// =====================================================================================
//
//       Filename:  ors_trading_control_hbt_thread.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  08/11/2015 10:04:21 AM
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

#pragma once

#include <iostream>
#include <thread>
#include <sys/time.h>
#include <atomic>

#include "dvccode/CDef/email_utils.hpp"
#include "dvccode/Utils/send_alert.hpp"
#include "dvccode/Utils/settings.hpp"
#include "dvccode/TradingInfo/network_account_interface_manager.hpp"
#include "dvccode/TradingInfo/network_account_info_manager.hpp"
#include "dvccode/CDef/control_messages.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/Utils/tcp_client_socket.hpp"
#include "dvccode/Utils/thread.hpp"
#include "dvccode/Utils/control_hub_listener.hpp"
#include "dvccode/Utils/multicast_sender_socket.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/CDef/assumptions.hpp"

#define MAX_FAILED_CONNECTION_ATTEMPT_ALLOWED 3
#define FAILED_CONNECTION_ATTEMPTS_TO_SHUTDOWN 15

namespace HFSAT {
namespace Utils {

// Need a singleton instance to ensure there is just 1 control hub thread irrespective of how many engine instances are
// forked from multisession
class ORSTradingControlHBTThread : public HFSAT::Thread {
 private:
  int32_t number_of_failed_connections_;
  bool can_we_reach_control_hub_;
  int32_t connection_status_checked_last_time_at_;
  HFSAT::DebugLogger& dbglogger_;
  std::string control_ip_;
  int32_t control_port_;
  int32_t recheck_frequency_;
  bool keep_running_;
  std::vector<HFSAT::Utils::ControlHubListener*> control_hub_connection_listeners_vec_;
  struct timeval curr_time;
  std::string this_hostname_;

  ORSTradingControlHBTThread(HFSAT::DebugLogger& dbglogger, HFSAT::ORS::Settings& settings)
      : number_of_failed_connections_(0),
        can_we_reach_control_hub_(true),
        connection_status_checked_last_time_at_(0),
        dbglogger_(dbglogger),
        control_ip_(""),
        control_port_(0),
        recheck_frequency_(0),
        keep_running_(true),
        curr_time(),
        this_hostname_("")

  {
    if (settings.getValue("Exchange") == std::string("NSE_CD") ||
        settings.getValue("Exchange") == std::string("NSE_FO") ||
        settings.getValue("Exchange") == std::string("NSE_EQ") || settings.getValue("Exchange") == std::string("ASX")) {
      if (!settings.has("ControlHubIP") || !settings.has("ControlHubPort") ||
          !settings.has("ControlHubConnectionInterval")) {
        DBGLOG_CLASS_FUNC_LINE_FATAL << "EITEHR OF THE ControlHubIP OR ControlHubPort OR ControlHubConnectionInterval "
                                        "OR ClientIDStart OR ClientIDEnd is Missing"
                                     << DBGLOG_ENDL_NOFLUSH;
        DBGLOG_DUMP;
        exit(-1);

      } else {
        control_ip_ = settings.getValue("ControlHubIP");
        control_port_ = settings.getIntValue("ControlHubPort", 0);
        recheck_frequency_ = settings.getIntValue("ControlHubConnectionInterval", 0);
      }
    }

    char hostname[64];
    hostname[63] = '\0';
    gethostname(hostname, 63);

    this_hostname_ = hostname;

    // Assume - we have all hostnames within 14 chars,
    // remove domains
    if (std::string::npos != this_hostname_.find("."))
      this_hostname_ = this_hostname_.substr(0, this_hostname_.find("."));

    if (this_hostname_.length() >= kSecNameLen) {
      DBGLOG_CLASS_FUNC_LINE_FATAL << "OUR ASUMPTION OF HOSTNAME WITHOUT DOMAIN HAVING LENGTH LESS THAN " << kSecNameLen
                                   << " DOESN'T HOLD GOOD..CAN'T CONTINUE... " << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;
      exit(-1);
    }
  }

  ORSTradingControlHBTThread(ORSTradingControlHBTThread const& disabled_copy_constructor);

  void SendGetFlatToGivenClientRange() {
    HFSAT::NetworkAccountInfoManager network_account_info_manager;
    HFSAT::DataInfo control_recv_data_info = network_account_info_manager.GetControlRecvDataInfo();
    HFSAT::MulticastSenderSocket sock(
        control_recv_data_info.bcast_ip_, control_recv_data_info.bcast_port_,
        HFSAT::NetworkAccountInterfaceManager::instance().GetInterfaceForApp(HFSAT::k_Control));

    HFSAT::GenericControlRequestStruct generic_control_request_struct;
    memset((void*)&generic_control_request_struct, 0, sizeof(HFSAT::GenericControlRequestStruct));

    // populate getflat struct
    generic_control_request_struct.control_message_.message_code_ = HFSAT::kControlMessageCodeGetFlatOnThisHost;
    generic_control_request_struct.time_set_by_frontend_ = curr_time;
    memcpy((void*)&generic_control_request_struct.symbol_, (void*)this_hostname_.c_str(), this_hostname_.length());
    generic_control_request_struct.trader_id_ = HOSTWIDE_GETFLAT_TRADER_ID;

    int32_t written_length = sock.WriteN(sizeof(HFSAT::GenericControlRequestStruct), &generic_control_request_struct);

    if (written_length < (int32_t)sizeof(HFSAT::GenericControlRequestStruct)) {
      DBGLOG_CLASS_FUNC_LINE_ERROR << "FAILED TO SEND CONTROL COMMAND TO QUERIES, "
                                   << " CONTROL IP : " << control_recv_data_info.bcast_ip_
                                   << " PORT : " << control_recv_data_info.bcast_port_ << " INTERFACE : "
                                   << HFSAT::NetworkAccountInterfaceManager::instance().GetInterfaceForApp(
                                          HFSAT::k_Control)
                                   << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;

    } else {
      DBGLOG_CLASS_FUNC_LINE_INFO << "HAVE ASKED QUERIES TO GET FLAT... " << DBGLOG_ENDL_NOFLUSH;
    }
  }

 public:
  static ORSTradingControlHBTThread& GetUniqueInstance(HFSAT::DebugLogger& dbglogger, HFSAT::ORS::Settings& settings) {
    static ORSTradingControlHBTThread unique_instance(dbglogger, settings);
    return unique_instance;
  }

  void AddControlHubListener(HFSAT::Utils::ControlHubListener* control_hub_listener) {
    HFSAT::VectorUtils::UniqueVectorAdd(control_hub_connection_listeners_vec_, control_hub_listener);
  }

  void VerifyConnectivityWithControlHub() {
    HFSAT::TCPClientSocket tcp_client_socket;
    tcp_client_socket.Connect(control_ip_.c_str(), control_port_);

    if (-1 == tcp_client_socket.socket_file_descriptor()) {
      number_of_failed_connections_++;

      DBGLOG_CLASS_FUNC_LINE_ERROR << "FAILED TO CONNECT TO CONTROL HUB AT : " << control_ip_
                                   << " X : " << control_port_ << " @ : " << curr_time.tv_sec << " FAILED_ATTEMPT -> "
                                   << number_of_failed_connections_ << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;

    } else {
      if (number_of_failed_connections_ > 0) {
        DBGLOG_CLASS_FUNC_LINE_INFO << "RESUMED CONNECTION WITH CONTROL HUB @ : " << curr_time.tv_sec
                                    << " TOTAL_FAILED_ATTEMPT -> " << number_of_failed_connections_
                                    << DBGLOG_ENDL_NOFLUSH;
      }

      number_of_failed_connections_ = 0;
      can_we_reach_control_hub_ = true;
    }

    // Let's send control message to query for getting flat and provide them 2 more cycle before ORS itself will
    // shutdown
    if (number_of_failed_connections_ >= MAX_FAILED_CONNECTION_ATTEMPT_ALLOWED &&
        number_of_failed_connections_ < FAILED_CONNECTION_ATTEMPTS_TO_SHUTDOWN) {
      can_we_reach_control_hub_ = false;

      DBGLOG_CLASS_FUNC_LINE_ERROR
          << "INITIATING QUERY GETFLAT AS FAILED CONNECTION ATTEMPTS HAVE EXCEEDED GETFLAT THRESHOLD..."
          << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;

      SendGetFlatToGivenClientRange();

    } else if (number_of_failed_connections_ >= FAILED_CONNECTION_ATTEMPTS_TO_SHUTDOWN) {
      // We've provided query a window of 2 frames to getflat, it's time to shutdown, we can put stringent limit here
      // after it works fine
      can_we_reach_control_hub_ = false;

      DBGLOG_CLASS_FUNC_LINE_FATAL << "CONNETION IS STILL DOWN... SHUTTING DOWN THE TRADING SYSTEM FOR SAFETY.. "
                                   << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;

      for (uint32_t listener_counter = 0; listener_counter < control_hub_connection_listeners_vec_.size();
           listener_counter++) {
        control_hub_connection_listeners_vec_[listener_counter]->OnControlHubDisconnectionShutdown();
      }
    }
  }

  void StopControlHBTThread() { keep_running_ = false; }

  void thread_main() {
    while (keep_running_) {
      gettimeofday(&curr_time, NULL);

      VerifyConnectivityWithControlHub();

      connection_status_checked_last_time_at_ = curr_time.tv_sec;

      // wait until we are due for next check
      sleep(recheck_frequency_);
    }
  }
};
}
}
