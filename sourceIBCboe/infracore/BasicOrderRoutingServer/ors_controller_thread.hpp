/**
   \file BasicOrderRouting/ors_controller_thread.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */
#ifndef BASE_BASICORDERROUTINGSERVER_ORS_CONTROLLER_THREAD_H
#define BASE_BASICORDERROUTINGSERVER_ORS_CONTROLLER_THREAD_H

#include <boost/lockfree/queue.hpp>

#include "dvccode/Utils/shared_mem_writer.hpp"
#include "dvccode/Utils/settings.hpp"
#include "dvccode/Utils/thread.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CDef/ors_messages.hpp"

namespace HFSAT {
namespace ORS {
class ORSControllerThread : public HFSAT::Thread {
 private:
  static ORSControllerThread* unique_instance_;
  DebugLogger& dbglogger_;
  int client_id_;
  HFSAT::SharedMemWriter<GenericORSRequestStruct>* shm_writer_;
  GenericORSRequestStruct control_request_;
  Settings& settings_;
  uint64_t heartbeat_interval_in_usecs_;
  struct timeval timeout_;
  uint64_t last_sent_time_;
  HFSAT::ExchSource_t exch_source_;
  boost::lockfree::queue<GenericORSRequestStruct>* queue_;

  ORSControllerThread(DebugLogger& dbglogger, Settings& settings);
  ~ORSControllerThread();
  ORSControllerThread(ORSControllerThread const& disabled_copy_constructor);

 public:
  void thread_main();

  static void SetUniqueInstance(HFSAT::DebugLogger& dbglogger, HFSAT::ORS::Settings& settings);
  static ORSControllerThread& GetUniqueInstance(HFSAT::DebugLogger& dbglogger, HFSAT::ORS::Settings& settings);
  static ORSControllerThread& GetUniqueInstance();

  void AddRequest(GenericORSRequestStruct& req);
  void WriteRequestToSHM(ORQType_t type);

  void ProcessRequestQueue();
  void SendFakeSendRequest();
};
}
}

#endif  // BASE_BASICORDERROUTINGSERVER_ORS_CONTROLLER_THREAD_H
