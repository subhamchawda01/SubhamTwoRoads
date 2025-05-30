/**
 \file ORSUtils/broadcast_strat_cmds.hpp

 \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
 Address:
 Suite 217, Level 2, Prestige Omega,
 No 104, EPIP Zone, Whitefield,
 Bangalore - 560066, India
 +91 80 4060 0717
 */

#pragma once

#include <deque>
#include <boost/lockfree/queue.hpp>

#include "infracore/BasicOrderRoutingServer/order_manager.hpp"
#include "dvccode/TradingInfo/network_account_info_manager.hpp"
#include "dvccode/TradingInfo/network_account_interface_manager.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CDef/ors_messages.hpp"
#include "dvccode/Utils/multicast_sender_socket.hpp"
#include "infracore/BasicOrderRoutingServer/position_manager.hpp"
#include "dvccode/CommonDataStructures/simple_security_symbol_indexer.hpp"
#include "dvccode/Utils/ors_reply_shm_interface.hpp"
#include "dvccode/CommonDataStructures/lockfree_simple_mempool.hpp"
#include "dvccode/Utils/rdtscp_timer.hpp"

namespace HFSAT {

class BroadcastStratCmds {
 private:
  DebugLogger& dbglogger_;
  HFSAT::NetworkAccountInfoManager network_account_info_manager_;
  HFSAT::DataInfo control_recv_data_info_;
  HFSAT::MulticastSenderSocket sock_;
  HFSAT::GenericControlRequestStruct* gcrs_;
 public:
  BroadcastStratCmds(DebugLogger& dbglogger);
  ~BroadcastStratCmds();
  void sendStratCmd(ControlMessageCode_t msg_code_);
};
}
