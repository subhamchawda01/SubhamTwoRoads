// =====================================================================================
//
//       Filename:  SimEngine.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  04/14/2017 01:10:04 PM
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

#include "dvccode/Utils/query_t2t.hpp"
#include "infracore/SimEngine/SimEngine.hpp"
#include <errno.h>

namespace HFSAT {
namespace SimEngine {

SIMEngine::SIMEngine(HFSAT::ORS::Settings &settings, HFSAT::DebugLogger &logger, std::string output_log_dir,
                     int32_t engine_id, AsyncWriter *pWriter, AsyncWriter *pReader)
    : BaseEngine(settings, logger),
      engine_running_(false),
      tcp_client_socket_(),
      reply_bufer_(),
      read_offset_(0),
      simulator_order_response_(),
      simulator_ip_("INVALID"),
      simulator_port_(-1),
      saos_to_size_executed_(),
      exch_sim_shm_interface_(HFSAT::Utils::ExchSimShmInterface::GetUniqueInstance()),
      sock_fd_(-1),
      id_(engine_id) {
  if (settings.has("SIMULATOR_IP") && settings.has("SIMULATOR_PORT")) {
    simulator_ip_ = settings.getValue("SIMULATOR_IP");
    simulator_port_ = settings.getIntValue("SIMULATOR_PORT", 0);
  }

  if (simulator_ip_ == "INVALID" || simulator_port_ == -1) {
    DBGLOG_CLASS_FUNC_LINE_FATAL << "CAN'T INITIALIZE SIM ENGINE WITH IP AND PORT : " << simulator_ip_ << " "
                                 << simulator_port_ << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    std::exit(-1);
  }

  if (simulator_ip_.find("10.23.") == std::string::npos) {
    DBGLOG_CLASS_FUNC_LINE_FATAL << "THIS DOESN'T SEEM TO BE OUR INTERNAL NETWORK : " << simulator_ip_ << " "
                                 << simulator_port_ << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    std::exit(-1);
  }

  memset((void *)reply_bufer_, 0, sizeof(reply_bufer_));
}

void SIMEngine::Connect() {
  tcp_client_socket_.Connect(simulator_ip_.c_str(), simulator_port_);

  if (-1 == tcp_client_socket_.socket_file_descriptor()) {
    DBGLOG_CLASS_FUNC_LINE_FATAL << "FAILED TO CONNECT TO THE SIMULATOR : " << simulator_ip_.c_str() << " X "
                                 << simulator_port_ << " Error : " << strerror(errno) << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    std::exit(-1);
  }

  sock_fd_ = tcp_client_socket_.socket_file_descriptor();

  DBGLOG_CLASS_FUNC_LINE_INFO << "SIM ENGINE CONNECTED TO SIMULATOR AT : " << simulator_ip_ << " X " << simulator_port_
                              << " FD: " << sock_fd_ << DBGLOG_ENDL_FLUSH;
  DBGLOG_DUMP;

  exch_sim_shm_interface_.Initialize();

  engine_running_ = true;

  p_engine_listener_->OnConnect(true);

//  IsOnloadMsgWarmSuppoted(tcp_client_socket_.socket_file_descriptor());

  Login();
}

void SIMEngine::CheckToSendHeartbeat() {}

void SIMEngine::ProcessFakeSend(HFSAT::ORS::Order *ord, ORQType_t type) {
  static int temp_sim = 1;
  if (temp_sim < 20) {
    DBGLOG_CLASS_FUNC_LINE << "Fake send " << DBGLOG_ENDL_FLUSH;
    temp_sim++;
  }
}

void SIMEngine::Login() { p_engine_listener_->OnLogin(true); }

void SIMEngine::SendOrder(HFSAT::ORS::Order *order) {
  HFSAT::ORS::Order new_order;
  memcpy(&new_order, order, sizeof(HFSAT::ORS::Order));

  new_order.is_ioc = true;
//  DBGLOG_CLASS_FUNC_LINE << "EngineID: " << id_ << " " << new_order.toString() << DBGLOG_ENDL_FLUSH;

  exch_sim_shm_interface_.WriteOrderToExchSim(&new_order);

  HFSAT::QueryT2T::GetUniqueInstance().End(order->server_assigned_client_id_);
}

void SIMEngine::CancelOrder(HFSAT::ORS::Order *order) {

  HFSAT::ORS::Order cxl_order;
  memcpy(&cxl_order, order, sizeof(HFSAT::ORS::Order));
  cxl_order.is_ioc = false;

//  DBGLOG_CLASS_FUNC_LINE << "EngineID: " << id_ << " " << cxl_order.toString() << DBGLOG_ENDL_FLUSH;
  exch_sim_shm_interface_.WriteOrderToExchSim(&cxl_order);

  HFSAT::QueryT2T::GetUniqueInstance().End(order->server_assigned_client_id_);
}

void SIMEngine::ModifyOrder(HFSAT::ORS::Order *order, ORS::Order *orig_order) {
  exch_sim_shm_interface_.WriteOrderToExchSim(order);

  HFSAT::QueryT2T::GetUniqueInstance().End(order->server_assigned_client_id_);
}

void SIMEngine::DisConnect() { tcp_client_socket_.Close(); }

void SIMEngine::Logout() { p_engine_listener_->OnLogout(); }

void SIMEngine::sendHeartbeat() {
  // DO Nothing
}

void SIMEngine::ProcessSinglePacket(HFSAT::ORS::ExchSimResponseStruct const &simulator_order_response) {
  //  DBGLOG_CLASS_FUNC_LINE << "ExchSimResponse : " << simulator_order_response.ToString() << DBGLOG_ENDL_FLUSH;

  ORRType_t type = simulator_order_response.event;

  switch (type) {
    case kORRType_Conf: {
      p_engine_listener_->OnOrderConf(simulator_order_response.saos, "", simulator_order_response.price,
                                      simulator_order_response.size_remaining);

    } break;

    case kORRType_Rejc: {
      p_engine_listener_->OnReject(simulator_order_response.saos);
    } break;

    case kORRType_Cxld: {
      p_engine_listener_->OnOrderCxl(simulator_order_response.saos);
    } break;

    case kORRType_CxlRejc: {
      p_engine_listener_->OnCxlReject(simulator_order_response.saos, kCxlRejectReasonTooLate);
    } break;

    case kORRType_Exec: {
      int saos = simulator_order_response.saos;
      if (simulator_order_response.size_executed > 0) {
        // We get total executed size from the exchange simulator, so we need to compute incremental size_executed
        int old_size_executed = 0;
        if (saos_to_size_executed_.find(saos) != saos_to_size_executed_.end()) {
          old_size_executed = saos_to_size_executed_[saos];
        }

        int size_exec = simulator_order_response.size_executed - old_size_executed;

        if (size_exec > 0) {
          p_engine_listener_->OnOrderExec(simulator_order_response.saos, simulator_order_response.symbol,
                                          simulator_order_response.buysell, simulator_order_response.price, size_exec,
                                          simulator_order_response.size_remaining);
        }

        saos_to_size_executed_[saos] = simulator_order_response.size_executed;
      }
    } break;

    default: {
      DBGLOG_CLASS_FUNC_LINE << "Event not handled : " << HFSAT::ToString(type) << DBGLOG_ENDL_FLUSH;
      break;
    }
  }
}

int32_t SIMEngine::ProcessSimulatorReply(char const *simulator_reply, int32_t const packet_length) {
  int32_t legnth_to_be_processed = packet_length;
  char const *msg_ptr = simulator_reply;

  while (legnth_to_be_processed > 0) {
    if ((uint32_t)legnth_to_be_processed < sizeof(HFSAT::ORS::ExchSimResponseStruct)) {
      memcpy((void *)reply_bufer_, (void *)msg_ptr, legnth_to_be_processed);
      return legnth_to_be_processed;
    }

    // ProcessSingle Packet
    memcpy((void *)&simulator_order_response_, (void *)msg_ptr, sizeof(HFSAT::ORS::ExchSimResponseStruct));
    msg_ptr += sizeof(HFSAT::ORS::ExchSimResponseStruct);
    legnth_to_be_processed -= sizeof(HFSAT::ORS::ExchSimResponseStruct);

    ProcessSinglePacket(simulator_order_response_);
  }

  return 0;
}

void SIMEngine::thread_main() {
  if (engine_id_ >= 0) {
    DBGLOG_CLASS_FUNC_LINE
        << "Not expected to be called. Running in MultiSession mode. Returning from thread main. EngineID: " << id_
        << DBGLOG_ENDL_FLUSH;
    return;
  }
  memset((void *)reply_bufer_, 0, sizeof(reply_bufer_));

  setName("SIMEngineThread");
  AllocateCPUOrExit();

  while (engine_running_) {
    int32_t retval = tcp_client_socket_.ReadN(512 - read_offset_, reply_bufer_ + read_offset_);

    if (retval <= 0) {
      DBGLOG_CLASS_FUNC_LINE_FATAL << "READ FROM SIMULATOR HAS FAILED, READ : " << retval
                                   << " SYSTEM ERROR : " << strerror(errno) << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
      engine_running_ = false;
    }

    read_offset_ = ProcessSimulatorReply(reply_bufer_, read_offset_ + retval);
  }
}

int32_t SIMEngine::onInputAvailable(int32_t _socket_) {
  if (!engine_running_) {
    DBGLOG_CLASS_FUNC_LINE_ERROR << "SOCKET READY WHILE MULTISESSION ENGINE IS NOT RUNNING WITH ID : " << id_
                                 << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;

    return HFSAT::ORS::kSessionNotRunning;
  }

  while (engine_running_) {
    int32_t read_length = tcp_client_socket_.ReadN(512 - read_offset_, reply_bufer_ + read_offset_);

    if (read_length > 0) {
      read_offset_ = ProcessSimulatorReply(reply_bufer_, read_offset_ + read_length);
      if (0 == read_offset_) return 0;
    } else if (read_length < 0) {
      DBGLOG_CLASS_FUNC_LINE_FATAL << "READ RETURNED WITH AN ERROR : " << strerror(errno) << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;

      engine_running_ = false;

      return HFSAT::ORS::kSessionTerminated;

    } else {
      DBGLOG_CLASS_FUNC_LINE_FATAL << "SOCKET CLOSED ON PEER SIDE : " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;

      engine_running_ = false;
    }
  }

  DBGLOG_CLASS_FUNC_LINE_FATAL << "Session Closed" << DBGLOG_ENDL_FLUSH;

  return HFSAT::ORS::kSessionClosed;
}

std::vector<int> SIMEngine::init() {
  vector<int> ss;
  ss.push_back(tcp_client_socket_.socket_file_descriptor());
  return ss;
}

void SIMEngine::OnAddString(uint32_t sec_id) {
  // DO Nothing
}
}
}
