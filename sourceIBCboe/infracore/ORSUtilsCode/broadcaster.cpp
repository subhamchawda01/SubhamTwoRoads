/**
 \file ORSUtils/broadcaster.cpp

 \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
 Address:
 Suite 217, Level 2, Prestige Omega,
 No 104, EPIP Zone, Whitefield,
 Bangalore - 560066, India
 +91 80 4060 0717
 */

#include "dvccode/CDef/email_utils.hpp"
#include "dvccode/Utils/send_alert.hpp"
#include "infracore/ORSUtils/broadcaster.hpp"
#include "dvccode/TradingInfo/network_account_interface_manager.hpp"

namespace HFSAT {

Broadcaster::Broadcaster(DebugLogger& t_dbglogger_, std::string bcast_ip, int bcast_port, int32_t base_writer_id)
    : dbglogger_(t_dbglogger_),
      base_writer_id_(base_writer_id +
                      1),  // since we are allocating client id starting with +1 and incrementing by 1 then onwards
      multicast_sender_socket_(bcast_ip, bcast_port,
                               NetworkAccountInterfaceManager::instance().GetInterfaceForApp(k_ORS)),
      position_manager_(HFSAT::ORS::PositionManager::GetUniqueInstance()),
      simple_security_symbol_indexer_(HFSAT::SimpleSecuritySymbolIndexer::GetUniqueInstance()),
      order_manager_(HFSAT::ORS::OrderManager::GetUniqueInstance()),
      mds_shm_interface_(HFSAT::Utils::ORSReplyShmInterface::GetUniqueInstance()),
      use_shm_for_ors_reply_(true),
      mutex_(0),
      clock_source_(HFSAT::ClockSource::GetUniqueInstance()),
      using_simulated_clocksource_(clock_source_.AreWeUsingSimulatedClockSource()) {
  queue_ = new boost::lockfree::queue<GenericORSReplyStructLiveProShm>(100);
  mds_shm_interface_.Initialize();
}

Broadcaster::~Broadcaster() {}

void Broadcaster::thread_main() {
  // We want the replies to reach as fast as it can to the awaiting clients,
  // moving ahead this thread will evolve into something which will be doing some more tasks than just sending messages
  // to the clients
  setName("BroadcastManager");
  AllocateCPUOrExit();

  while (1) {
    while (!queue_->empty()) {
      GenericORSReplyStructLiveProShm reply_struct;
      queue_->pop(reply_struct);
      multicast_sender_socket_.WriteN(sizeof(reply_struct), &reply_struct);
    }
  }
}

void Broadcaster::Push(GenericORSReplyStructLiveProShm& t_ors_reply_) {
  if (use_shm_for_ors_reply_) {
    mds_shm_interface_.WriteGenericStructLockFree(&t_ors_reply_);
  }

  queue_->push(t_ors_reply_);
}
}
