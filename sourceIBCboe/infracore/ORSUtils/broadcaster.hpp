/**
 \file ORSUtils/broadcaster.hpp

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
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CDef/ors_messages.hpp"
#include "dvccode/Utils/multicast_sender_socket.hpp"
#include "dvccode/Utils/thread.hpp"
#include "infracore/BasicOrderRoutingServer/position_manager.hpp"
#include "dvccode/CommonDataStructures/simple_security_symbol_indexer.hpp"
#include "dvccode/Utils/ors_reply_shm_interface.hpp"
#include "dvccode/CommonDataStructures/lockfree_simple_mempool.hpp"
#include "dvccode/Utils/rdtscp_timer.hpp"

namespace HFSAT {

class Broadcaster : public Thread {
 private:
  DebugLogger& dbglogger_;
  int32_t base_writer_id_;
  boost::lockfree::queue<GenericORSReplyStructLiveProShm>* queue_;
  HFSAT::MulticastSenderSocket multicast_sender_socket_;
  HFSAT::ORS::PositionManager& position_manager_;
  HFSAT::SimpleSecuritySymbolIndexer& simple_security_symbol_indexer_;
  HFSAT::ORS::OrderManager& order_manager_;
  HFSAT::Utils::ORSReplyShmInterface& mds_shm_interface_;
  bool use_shm_for_ors_reply_;
  volatile int mutex_;
  HFSAT::ClockSource& clock_source_;
  bool using_simulated_clocksource_;

 public:
  Broadcaster(DebugLogger& dbglogger, std::string bcast_ip, int bcast_port, int32_t base_writer_id);
  ~Broadcaster();

  void thread_main();

  void Push(GenericORSReplyStructLiveProShm& ors_reply);
};
}
