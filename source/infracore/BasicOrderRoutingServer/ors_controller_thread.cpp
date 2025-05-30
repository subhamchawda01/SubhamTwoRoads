#include "infracore/BasicOrderRoutingServer/ors_controller_thread.hpp"

namespace HFSAT {
namespace ORS {

ORSControllerThread* ORSControllerThread::unique_instance_ = nullptr;

ORSControllerThread::ORSControllerThread(DebugLogger& dbglogger, Settings& settings)
    : dbglogger_(dbglogger),
      client_id_(-1),
      shm_writer_(NULL),
      settings_(settings),
      heartbeat_interval_in_usecs_(999999),
      last_sent_time_() {
  timeout_.tv_sec = 0;
  timeout_.tv_usec = 3000;

  exch_source_ = HFSAT::StringToExchSource(settings.getValue("Exchange"));

  shm_writer_ = new SharedMemWriter<GenericORSRequestStruct>(exch_source_);

  client_id_ = shm_writer_->intilizeWriter();
  if (client_id_ == -1) {
    fprintf(stderr, "ORSControllerThread : Could not initialize shared mem writer.\n");
    delete (shm_writer_);
    shm_writer_ = NULL;
    exit(-1);
  } else {
    dbglogger_ << "ORS Controller SACI : " << client_id_ << "\n";
    dbglogger_.DumpCurrentBuffer();
  }

  // Specify 0 Risk Limit (Required as this thread is a type of Query for ORS)
  if (NULL != shm_writer_) {
    DataWriterIdPair<GenericORSRequestStruct> risk_request;
    risk_request.writer_id = client_id_;
    risk_request.data.orq_request_type_ = HFSAT::ORQ_RISK;
    risk_request.data.size_requested_ =
        0;  // Denotes 0 Worstcase Position Limit the the thread (which is a type of query for ORS)
    strncpy(risk_request.data.symbol_, HFSAT::kExceptionProductKey, kSecNameLen);
    shm_writer_->writeT(risk_request);
  }

  queue_ = new boost::lockfree::queue<GenericORSRequestStruct>(10);
}

ORSControllerThread::~ORSControllerThread() {
  if (shm_writer_ != NULL) delete (shm_writer_);
}

void ORSControllerThread::SetUniqueInstance(HFSAT::DebugLogger& dbglogger, HFSAT::ORS::Settings& settings) {
  if (unique_instance_ == nullptr) {
    unique_instance_ = new ORSControllerThread(dbglogger, settings);
  }
}

ORSControllerThread& ORSControllerThread::GetUniqueInstance(HFSAT::DebugLogger& dbglogger,
                                                            HFSAT::ORS::Settings& settings) {
  if (unique_instance_ == nullptr) {
    unique_instance_ = new ORSControllerThread(dbglogger, settings);
  }
  return *unique_instance_;
}

ORSControllerThread& ORSControllerThread::GetUniqueInstance() {
  if (unique_instance_ == nullptr) {
    std::cerr << "SetUniqueInstance() not called for ORSControllerThread before calling GetUniqueInstance() \n";
    exit(-1);
  }
  return *unique_instance_;
}

void ORSControllerThread::AddRequest(GenericORSRequestStruct& req) { queue_->push(req); }

void ORSControllerThread::ProcessRequestQueue() {
  while (!queue_->empty()) {
    GenericORSRequestStruct request;
    request.orq_request_type_ = HFSAT::ORQType_t::ORQ_CANCEL;
    queue_->pop(request);

    switch (request.orq_request_type_) {
      case ORQ_PROCESS_QUEUE: {
        WriteRequestToSHM(ORQ_PROCESS_QUEUE);
      } break;

      default: { break; }
    }
  }
}

void ORSControllerThread::SendFakeSendRequest() { WriteRequestToSHM(ORQ_FAKE_SEND); }

void ORSControllerThread::WriteRequestToSHM(ORQType_t type) {
  if (NULL != shm_writer_) {
    DataWriterIdPair<GenericORSRequestStruct> recovery_request;
    recovery_request.writer_id = client_id_;
    recovery_request.data.orq_request_type_ = type;  // Heartbeat request
    shm_writer_->writeT(recovery_request);
  }
}

// Sends heartbeat requests currently. Further work would be to add more kinds of request.
// Some other class would push requests to a lockfree queue and in the following thread_main we can process from that
// request queue.

void ORSControllerThread::thread_main() {
  timeval curr_time;
  gettimeofday(&curr_time, NULL);
  last_sent_time_ = curr_time.tv_sec * 1000000 + curr_time.tv_usec;

  while (true) {
    //std::cout << "ORSControllerThread::thread_main" << std::endl;
    int retval = 0;
    timeout_.tv_sec = 0;
    timeout_.tv_usec = 3000;
    retval = select(0, 0, 0, 0, &timeout_);
    if (retval == 0) {
      ProcessRequestQueue();
      SendFakeSendRequest();

      gettimeofday(&curr_time, NULL);
      uint64_t cut_int_time = curr_time.tv_sec * 1000000 + curr_time.tv_usec;
      uint64_t diff = cut_int_time - last_sent_time_;
      if (diff >= heartbeat_interval_in_usecs_) {
        WriteRequestToSHM(ORQ_HEARTBEAT);
        last_sent_time_ = cut_int_time;
      }
    }
  }
}
}
}
