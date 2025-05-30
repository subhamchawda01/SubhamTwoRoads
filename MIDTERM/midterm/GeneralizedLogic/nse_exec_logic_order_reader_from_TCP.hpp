#pragma once
#include <math.h>

#include "dvccode/ExternalData/external_time_listener.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/Utils/tcp_server_manager.hpp"

#include "midterm/GeneralizedLogic/nse_simple_exec_logic.hpp"
#include "midterm/GeneralizedLogic/nse_exec_logic_order_reader.hpp"

namespace NSE_SIMPLEEXEC {
class SimpleNseExecLogicOrderReaderFromTCP
    : public SimpleNseExecLogicOrderReader,
      public HFSAT::Utils::TCPServerSocketListener {
  HFSAT::Utils::TCPServerManager *tcp_server_manager_;
  bool is_ready_;
  std::string curr_buffer_;
  HFSAT::Lock mkt_lock_;

public:
  SimpleNseExecLogicOrderReaderFromTCP(
      HFSAT::Watch &watch_t, HFSAT::DebugLogger &dbglogger_t, bool is_live_t,
      std::map<std::string, SyntheticLegInfo> leg_info_t,
      NseExecLogicHelper *exec_logic_helper_t, int server_port_);
  void Stop() { tcp_server_manager_->stop(); }
  void OnClientRequest(int32_t client_fd, char *buffer, uint32_t const &length);
};
}
