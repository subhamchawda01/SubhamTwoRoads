
// =====================================================================================
//
//       Filename:  nse_bardata_recovery_manager.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  09/21/2015 02:28:29 AM
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
//
//
// =====================================================================================

#include <iostream>
#include <cstdlib>
#include <fstream>
#include <signal.h>
#include <unistd.h>
#include <cstring>
#include <string>

#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/ExternalData/simple_live_dispatcher.hpp"
#include "dvccode/Utils/tcp_server_manager.hpp"
#include "dvccode/Utils/tcp_server_socket_listener.hpp"
#include "dvccode/Utils/load_our_defined_products.hpp"
#include "dvccode/CDef/defines.hpp"
#include "dvccode/Utils/recovery_manager_config_parser.hpp"

#include "infracore/NSEMD/nse_tbt_raw_md_handler.hpp"

#define DEBUG_REC_MGR 0

class BaseBardataHandlerInterface {
 public:
  virtual bool ProcessBardataRequest(char *in_buffer, int32_t length, char *out_buffer) = 0;
  virtual ~BaseBardataHandlerInterface() {}
};

template <class T>
class BardataHandlerInterface : public BaseBardataHandlerInterface {
 private:
  T *raw_md_handler_ptr_;

 public:
  BardataHandlerInterface(T *t_raw_md_handler_ptr) : raw_md_handler_ptr_(t_raw_md_handler_ptr) {}

  bool ProcessBardataRequest(char *in_buffer, int32_t length, char *out_buffer) {
#if DEBUG_REC_MGR
    std::cout << " BardataHandlerInterface::ProcessBardataRequest " << length << std::endl;
#endif

    return raw_md_handler_ptr_->ProcessBardataRequest(in_buffer, length, out_buffer);
  }
};

class BardataRecoveryInterface : public HFSAT::Utils::TCPServerSocketListener {
 private:
  HFSAT::Utils::TCPServerManager &tcp_server_manager_;
  std::map<std::string, BaseBardataHandlerInterface *> exch_src_to_handler_interface_;
  HFSAT::DebugLogger &dbglogger_;

 public:
  BardataRecoveryInterface(HFSAT::Utils::TCPServerManager &tcp_server_manager, HFSAT::DebugLogger &dbglogger)
      : tcp_server_manager_(tcp_server_manager), dbglogger_(dbglogger) {
    tcp_server_manager_.SubscribeForUpdates(this);
  }

  ~BardataRecoveryInterface() {}

  void AddExchangeSource(std::string exchange_src, BaseBardataHandlerInterface *bardata_handler_interface_ptr_) {
    exch_src_to_handler_interface_[exchange_src] = bardata_handler_interface_ptr_;
  }

  void ParseMetaData(char *&in_buffer, std::string &exchange_name) {
    // extract exchange name
    int32_t int_exchange_src = *((int32_t *)(in_buffer));
    exchange_name = "INVALID";

    for (auto it : (HFSAT::exchange_name_str_to_int_mapping)) {
      if (it.second == int_exchange_src) {
        exchange_name = it.first;
        break;
      }
    }

    dbglogger_ << "Received bardata recovery request for exchange source : " << exchange_name << '\n';
    dbglogger_.DumpCurrentBuffer();

    in_buffer += BARDATA_RECOVERY_REQUEST_EXCHANGE_LENGTH;

    return;
  }

  void OnClientRequest(int32_t client_fd, char *in_buffer, uint32_t const &length) {
#if DEBUG_REC_MGR
    std::cout << "BardataRecoveryInterface::OnClientRequest: received length: " << length
              << " exch symbol: " << *(in_buffer + 20) << std::endl;
#endif
    if (length != BARDATA_RECOVERY_REQUEST_MESSAGE_LENGTH) return;

    std::string exchange_name;

    ParseMetaData(in_buffer, exchange_name);

    char *out_buffer = new char[MAX_RECOVERY_PACKET_SIZE];

    if (exch_src_to_handler_interface_.find(exchange_name) == exch_src_to_handler_interface_.end()) {
      tcp_server_manager_.RespondFailureToClient(client_fd);
      dbglogger_ << "Not listening to this exchange source/Exchange Source not present in CDef/Defines map..." << '\n';
      dbglogger_.DumpCurrentBuffer();
      return;
    }

    if (false == exch_src_to_handler_interface_[exchange_name]->ProcessBardataRequest(in_buffer, length, out_buffer)) {
      tcp_server_manager_.RespondFailureToClient(client_fd);
      return;
    }

    int32_t *packet_length_ptr = (int32_t *)((char *)out_buffer);
    int32_t packet_length = *packet_length_ptr;
    packet_length += sizeof(int32_t);

    tcp_server_manager_.RespondToClient(client_fd, out_buffer, packet_length);
    delete out_buffer;
  }
};

int main(int argc, char *argv[]) {
  signal(SIGPIPE, SIG_IGN);

  HFSAT::SimpleLiveDispatcher simple_live_dispatcher;
  HFSAT::DebugLogger dbglogger(10240);
  std::ostringstream t_temp_oss;
  //  HFSAT::SecurityNameIndexer &sec_name_indexer_ = HFSAT::SecurityNameIndexer::GetUniqueInstance();
  HFSAT::RecoveryManagerConfigParser recovery_manager_config_parser_;
  std::string this_exchange = "NSE_BARDATA";

  // NSE_EQ_BARDATA OR NSE_FO_BARDATA
  if (argc > 1) {
    this_exchange = argv[1];
  }

  recovery_manager_config_parser_.Initialise(this_exchange);

  int client_recovery_port = recovery_manager_config_parser_.GetRecoveryHostClientPort();
  std::vector<std::string> exchange_sources_to_listen = recovery_manager_config_parser_.GetAllExchangeSources();
  std::set<HFSAT::ExchSource_t> exch_source_processing;

  t_temp_oss << "/spare/local/MDSlogs/new_bardata_recovery_manager_" << HFSAT::DateTime::GetCurrentIsoDateLocalAsString()
             << "_" << client_recovery_port << ".log";

  dbglogger.OpenLogFile(t_temp_oss.str().c_str(), std::ofstream::app);
  HFSAT::Utils::TCPServerManager tcp_server_manager(client_recovery_port, dbglogger);

  tcp_server_manager.run();

  dbglogger << "STARTING BARDATA_RECOVERY_MANAGER TCP SERVER AT PORT: " << client_recovery_port << "\n";

  BardataRecoveryInterface recovery_interface(tcp_server_manager, dbglogger);

  dbglogger << "LISTENING TO EXCHANGE(S):";
  for (auto exch_ : exchange_sources_to_listen) {
    dbglogger << " " << exch_;
    if (exch_ == "NSE" || "NSE_FO" == exch_ || "NSE_EQ" == exch_ || "NSE_CD" == exch_) {
      HFSAT::NSEMD::NSETBTRawMDHandler *nse_tbt_raw_handler = &(HFSAT::NSEMD::NSETBTRawMDHandler::GetUniqueInstance(
          dbglogger, simple_live_dispatcher, nullptr, HFSAT::kBardataRecoveryHost, exch_));

      BardataHandlerInterface<HFSAT::NSEMD::NSETBTRawMDHandler> *nse_tbt_raw_md_handler_interface_ptr =
          new BardataHandlerInterface<HFSAT::NSEMD::NSETBTRawMDHandler>(nse_tbt_raw_handler);

      recovery_interface.AddExchangeSource(exch_, nse_tbt_raw_md_handler_interface_ptr);
      dbglogger << "EXCH: " << exch_ << "\n";

    } else {
      dbglogger << "No handling for <" << exch_ << "> added in recovery manager..." << '\n';
      dbglogger.DumpCurrentBuffer();
      exit(-1);
    }
  }

  dbglogger << "\n";
  dbglogger.DumpCurrentBuffer();

  // This is required for filtering, and while filtering this call is must
  HFSAT::Utils::LoadOurDefinedProducts::GetUniqueInstance(exch_source_processing).AddExchanges(exch_source_processing);

  simple_live_dispatcher.RunLive();

  return EXIT_SUCCESS;
}  // ----------  end of function main  ----------
