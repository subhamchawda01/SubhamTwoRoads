// =====================================================================================
//
//       Filename:  combined_mds_messages_shm_base.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  01/30/2014 12:39:20 PM
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

#include <sys/shm.h>

#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/mds_messages.hpp"
#include "dvccode/CDef/mds_shm_interface_defines.hpp"
#include "dvccode/Utils/shm1_queue.hpp"

namespace HFSAT {
namespace MDSMessages {

#define MAX_QUEUE_STATS_SIZE 25000
#define P99_MAX_QUEUE_STATS_SIZE 24750
#define LI_MAX_QUEUE_STATS_SIZE 24999

class CombinedMDSMessagesShmBase {
 protected:
  key_t generic_mds_shm_key_ors;
  key_t generic_mds_shm_key_;
  int generic_mds_shmid_;
  int generic_mds_shmid_ors;
  volatile HFSAT::MDS_MSG::GenericMDSMessage *generic_mds_shm_struct_;
  HFSAT::Shm1Queue<HFSAT::MDS_MSG::GenericMDSMessage> *ors_reply_shm_queue_;
  struct shmid_ds generic_shmid_ds_;
  struct shmid_ds generic_shmid_ds_ors;

  std::vector<int> queue_stats_;
  int queue_stats_index_;
  unsigned int ors_reply_queue_size_;
  bool is_queue_stats_enabled_;
  int64_t stats_sum_;
  uint64_t log_time_;
  int64_t max_distance_;

  HFSAT::DebugLogger stats_dbglogger_;

  HFSAT::Shm1Queue<HFSAT::MDS_MSG::GenericMDSMessage> *shm_queue_;
  unsigned int queue_size_;
  bool using_thread_safe_shm_;

 public:
  CombinedMDSMessagesShmBase()
      : generic_mds_shm_key_ors(ORS_REPLY_MDS_SHM_KEY),
        generic_mds_shm_key_(GENERIC_MDS_SHM_KEY),
        generic_mds_shmid_(-1),
        generic_mds_shmid_ors(-1),
        generic_mds_shm_struct_(NULL),
        ors_reply_shm_queue_(nullptr),
        queue_stats_(MAX_QUEUE_STATS_SIZE),
        queue_stats_index_(0),
        ors_reply_queue_size_(sizeof(HFSAT::Shm1Queue<HFSAT::MDS_MSG::GenericMDSMessage>)),
        is_queue_stats_enabled_(false),
        stats_sum_(0),
        log_time_(0),
        max_distance_(0),
        stats_dbglogger_(10240, 1),
        shm_queue_(nullptr),
        queue_size_(sizeof(HFSAT::Shm1Queue<HFSAT::MDS_MSG::GenericMDSMessage>)),
        using_thread_safe_shm_(HFSAT::UseShmforORSReply()) {
    if (using_thread_safe_shm_)
      InitializeThreadSafeSHM();
    else
      InitializeOldSHM();
    InitializeSHMORS();
  }

  void InitializeOldSHM() {
    if ((generic_mds_shmid_ =
             shmget(generic_mds_shm_key_,
                    (size_t)(GENERIC_MDS_QUEUE_SIZE * (sizeof(HFSAT::MDS_MSG::GenericMDSMessage)) + sizeof(int)),
                    IPC_CREAT | 0666)) < 0) {
      if (errno == EINVAL)
        std::cerr << "Invalid segment size specified \n";

      else if (errno == EEXIST)
        std::cerr << "Segment already exists \n";

      else if (errno == EIDRM)
        std::cerr << " Segment is marked for deletion \n";

      else if (errno == ENOENT)
        std::cerr << " Segment Doesn't Exist \n";

      else if (errno == EACCES)
        std::cerr << " Permission Denied \n";

      else if (errno == ENOMEM)
        std::cerr << " Not Enough Memory To Create Shm Segment \n";

      exit(1);
    }
    std::cout << "InitializeOldSHM: " << generic_mds_shmid_ << " KEY: " << generic_mds_shm_key_ << std::endl;
    if ((generic_mds_shm_struct_ = (volatile HFSAT::MDS_MSG::GenericMDSMessage *)shmat(generic_mds_shmid_, NULL, 0)) ==
        (volatile HFSAT::MDS_MSG::GenericMDSMessage *)-1) {
      perror("shmat failed");
      exit(0);
    }

    if (-1 == (shmctl(generic_mds_shmid_, IPC_STAT, &generic_shmid_ds_))) {
      perror("shmctl");
      exit(1);
    }

    if (1 == generic_shmid_ds_.shm_nattch) {
      memset((void *)((HFSAT::MDS_MSG::GenericMDSMessage *)generic_mds_shm_struct_), 0,
             (GENERIC_MDS_QUEUE_SIZE * sizeof(HFSAT::MDS_MSG::GenericMDSMessage) + sizeof(int)));
    }
  }

  void InitializeSHMORS() {
    std::cout << "InitializeSHMORS: " << generic_mds_shmid_ors << " KEY: " << generic_mds_shm_key_ors << " SIZE: " << ors_reply_queue_size_ << std::endl;
    if ((generic_mds_shmid_ors = shmget(generic_mds_shm_key_ors, ors_reply_queue_size_, IPC_CREAT | 0666)) < 0) {
      if (errno == EINVAL)
        std::cerr << "Invalid segment size specified \n";

      else if (errno == EEXIST)
        std::cerr << "Segment already exists \n";

      else if (errno == EIDRM)
        std::cerr << " Segment is marked for deletion \n";

      else if (errno == ENOENT)
        std::cerr << " Segment Doesn't Exist \n";

      else if (errno == EACCES)
        std::cerr << " Permission Denied \n";

      else if (errno == ENOMEM)
        std::cerr << " Not Enough Memory To Create Shm Segment \n";

      exit(1);
    }
    std::cout << "InitializeSHMORS: " << generic_mds_shmid_ors << " KEY: " << generic_mds_shm_key_ors << std::endl;


    if ((ors_reply_shm_queue_ = (HFSAT::Shm1Queue<HFSAT::MDS_MSG::GenericMDSMessage> *)shmat(
             generic_mds_shmid_ors, NULL, 0)) == (HFSAT::Shm1Queue<HFSAT::MDS_MSG::GenericMDSMessage> *)-1) {
      perror("shmat failed");
      exit(0);
    }

    if (-1 == (shmctl(generic_mds_shmid_ors, IPC_STAT, &generic_shmid_ds_ors))) {
      perror("shmctl");
      exit(1);
    }

    if (1 == generic_shmid_ds_ors.shm_nattch) {
      memset((void *)ors_reply_shm_queue_, 0, sizeof(HFSAT::Shm1Queue<HFSAT::MDS_MSG::GenericMDSMessage>));
    }
  }

  void InitializeThreadSafeSHM() {
    if ((generic_mds_shmid_ = shmget(generic_mds_shm_key_, queue_size_, IPC_CREAT | 0666)) < 0) {
      if (errno == EINVAL)
        std::cerr << "Invalid segment size specified \n";

      else if (errno == EEXIST)
        std::cerr << "Segment already exists \n";

      else if (errno == EIDRM)
        std::cerr << " Segment is marked for deletion \n";

      else if (errno == ENOENT)
        std::cerr << " Segment Doesn't Exist \n";

      else if (errno == EACCES)
        std::cerr << " Permission Denied \n";

      else if (errno == ENOMEM)
        std::cerr << " Not Enough Memory To Create Shm Segment \n";

      exit(1);
    }

    if ((shm_queue_ = (HFSAT::Shm1Queue<HFSAT::MDS_MSG::GenericMDSMessage> *)shmat(generic_mds_shmid_, NULL, 0)) ==
        (HFSAT::Shm1Queue<HFSAT::MDS_MSG::GenericMDSMessage> *)-1) {
      perror("shmat failed");
      exit(0);
    }

    if (-1 == (shmctl(generic_mds_shmid_, IPC_STAT, &generic_shmid_ds_))) {
      perror("shmctl");
      exit(1);
    }

    if (1 == generic_shmid_ds_.shm_nattch) {
      memset((void *)shm_queue_, 0, sizeof(HFSAT::Shm1Queue<HFSAT::MDS_MSG::GenericMDSMessage>));
    }
  }

  volatile uint64_t GetORSReplyQueuePos() { return ors_reply_shm_queue_->writer_pos(); }

  volatile uint64_t GetQueuePos() {
    if (!using_thread_safe_shm_) {
      return *((int *)(generic_mds_shm_struct_ + GENERIC_MDS_QUEUE_SIZE));
    } else {
      return shm_queue_->writer_pos();
    }
  }

  virtual ~CombinedMDSMessagesShmBase() {
    stats_dbglogger_.DumpCurrentBuffer();
    stats_dbglogger_.Close();
  }

  inline virtual void RunLiveShmSource(bool _keep_in_loop_ = true) = 0;

  void DumpStats(std::vector<int> &stats, int length) {
    if (!is_queue_stats_enabled_) {
      return;
    }
    if (stats.empty()) {
      return;
    }
    std::sort(stats.begin(), stats.end());

    // double mean = stats_sum_ / ((double)length);
    double median = stats[length >> 1];

    // double percentile_99 = stats[uint32_t(0.99 * length)];

    stats_dbglogger_ << " Median: " << median << " 99P:" << stats[P99_MAX_QUEUE_STATS_SIZE]
                     << " Max: " << stats[LI_MAX_QUEUE_STATS_SIZE] << "\n";
    // KP: I am not sure why we have to dump now
    // stats_dbglogger_.DumpCurrentBuffer();
    stats_dbglogger_.CheckToFlushBuffer();
  }

  void EnableQueueStats(std::string logfilepath) {
    if (!is_queue_stats_enabled_) {
      is_queue_stats_enabled_ = true;
      stats_dbglogger_.OpenLogFile(logfilepath.c_str(), std::ios::out | std::ios::app);
    }
  }

  void ProcessQueueStats(volatile int queue_position, int index, const timeval &time) {
    if (is_queue_stats_enabled_) {
      if (queue_stats_index_ == MAX_QUEUE_STATS_SIZE) {
        stats_dbglogger_ << " Time: " << log_time_ << " #:" << max_distance_ << "\n";
        stats_dbglogger_.CheckToFlushBuffer();
        queue_stats_index_ = 0;
        max_distance_ = 0;
      }

      int t_distance_ =
          (queue_position >= index) ? (queue_position - index) : (GENERIC_MDS_QUEUE_SIZE - (index - queue_position));
      if (t_distance_ > max_distance_) {
        max_distance_ = t_distance_;
        log_time_ = (((uint64_t)time.tv_sec) * 1000000 + (uint64_t)time.tv_usec);
      }
      queue_stats_index_++;
    }
  }
};
}
}  // namespace HFSAT
