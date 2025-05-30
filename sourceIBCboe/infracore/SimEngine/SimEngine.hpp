// =====================================================================================
//
//       Filename:  SimEngine.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  04/14/2017 01:05:36 PM
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

#include <cstdlib>
#include <string>
#include <map>
#include <ctime>
#include <fstream>
#include <memory>
#include <iostream>
#include <unordered_map>

#include "dvccode/CDef/basic_ors_defines.hpp"
#include "infracore/BasicOrderRoutingServer/engine_listener.hpp"
#include "dvccode/Utils/settings.hpp"
#include "infracore/BasicOrderRoutingServer/base_engine.hpp"
#include "dvccode/CommonDataStructures/simple_security_symbol_indexer.hpp"
#include "dvccode/CDef/order.hpp"
#include "dvccode/Profiler/cpucycle_profiler.hpp"
#include "dvccode/Utils/exch_sim_shm_interface.hpp"

namespace HFSAT {
namespace SimEngine {

class SIMEngine : public HFSAT::ORS::BaseEngine, public HFSAT::SimpleSecuritySymbolIndexerListener {
 private:
  bool engine_running_;
  HFSAT::TCPClientSocket tcp_client_socket_;
  char reply_bufer_[512];
  int32_t read_offset_;
  HFSAT::ORS::ExchSimResponseStruct simulator_order_response_;
  std::string simulator_ip_;
  int32_t simulator_port_;
  std::unordered_map<int, int> saos_to_size_executed_;
  HFSAT::Utils::ExchSimShmInterface &exch_sim_shm_interface_;
  int sock_fd_;

 public:
  int32_t id_;
  SIMEngine(HFSAT::ORS::Settings &settings, HFSAT::DebugLogger &logger, std::string output_log_dir, int32_t engine_id,
            AsyncWriter *pWriter, AsyncWriter *pReader);

  void SendOrder(HFSAT::ORS::Order *order) override;
  void SendOrder(std::vector<HFSAT::ORS::Order*> multi_leg_order_ptr_vec_) override {}
  void SendSpreadOrder(HFSAT::ORS::Order *order1_, HFSAT::ORS::Order *order2_) override;
  void SendThreeLegOrder(HFSAT::ORS::Order *order1_, HFSAT::ORS::Order *order2_, HFSAT::ORS::Order *order3_) override;
  void SendTwoLegOrder(HFSAT::ORS::Order *order1_, HFSAT::ORS::Order *order2_) override;
  void CancelOrder(HFSAT::ORS::Order *order) override;
  void ModifyOrder(HFSAT::ORS::Order *order, HFSAT::ORS::Order *orig_order) override;

  void Connect();
  void DisConnect();
  void Login();
  void Logout();
  void sendHeartbeat();
  void ProcessSinglePacket(HFSAT::ORS::ExchSimResponseStruct const &simulator_order_response);
  int32_t ProcessSimulatorReply(char const *simulator_reply, int32_t const packet_length);

  void thread_main();
  int32_t onInputAvailable(int32_t socket);
  std::vector<int> init();
  void CheckToSendHeartbeat();
  void ProcessFakeSend(HFSAT::ORS::Order *ord, ORQType_t type) override;

  void OnAddString(unsigned int t_num_security_id_);
  void RequestORSPNL() override {};
  void RequestORSOpenPositions() override {};
};
}
}
