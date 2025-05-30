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

#include "dvccode/CDef/signal_msg.hpp"
#include "dvccode/CDef/mds_messages.hpp"
#include "dvccode/CDef/nse_shm_interface_defines.hpp"
#include "dvccode/CDef/bse_shm_interface_defines.hpp"
#include "dvccode/CDef/ors_reply_shm_interface_defines.hpp"
#include "dvccode/Utils/shm1_queue.hpp"
#include "dvccode/CDef/defines.hpp"

namespace HFSAT {
namespace MDSMessages {

class CombinedMDSMessagesNSEProShmBase {
 protected:
  key_t generic_mds_shm_key_;
  int generic_mds_shmid_;
  volatile NSE_MDS::NSETBTDataCommonStructProShm *generic_mds_shm_struct_;
  struct shmid_ds generic_shmid_ds_;

  HFSAT::Shm1Queue<NSE_MDS::NSETBTDataCommonStructProShm> *shm_queue_;
  unsigned int queue_size_;
  bool using_thread_safe_shm_;

 public:
  CombinedMDSMessagesNSEProShmBase()
      : generic_mds_shm_key_(NSE_MDS_SHM_KEY),
        generic_mds_shmid_(-1),
        generic_mds_shm_struct_(NULL),
        shm_queue_(nullptr),
        queue_size_(sizeof(HFSAT::Shm1Queue<NSE_MDS::NSETBTDataCommonStructProShm>)) {
    InitializeThreadSafeSHM();
  }

  //  void InitializeOldSHM() {
  //    if ((generic_mds_shmid_ =
  //             shmget(generic_mds_shm_key_,
  //                    (size_t)(NSE_MDS_QUEUE_SIZE * (sizeof(NSE_MDS::NSETBTDataCommonStructProShm)) + sizeof(int)),
  //                    IPC_CREAT | 0666)) < 0) {
  //      if (errno == EINVAL)
  //        std::cerr << "Invalid segment size specified \n";
  //
  //      else if (errno == EEXIST)
  //        std::cerr << "Segment already exists \n";
  //
  //      else if (errno == EIDRM)
  //        std::cerr << " Segment is marked for deletion \n";
  //
  //      else if (errno == ENOENT)
  //        std::cerr << " Segment Doesn't Exist \n";
  //
  //      else if (errno == EACCES)
  //        std::cerr << " Permission Denied \n";
  //
  //      else if (errno == ENOMEM)
  //        std::cerr << " Not Enough Memory To Create Shm Segment \n";
  //
  //      exit(1);
  //    }
  //
  //    if ((generic_mds_shm_struct_ = (volatile NSE_MDS::NSETBTDataCommonStructProShm *)shmat(generic_mds_shmid_, NULL,
  //    0)) ==
  //        (volatile NSE_MDS::NSETBTDataCommonStructProShm *)-1) {
  //      perror("shmat failed");
  //      exit(0);
  //    }
  //
  //    if (-1 == (shmctl(generic_mds_shmid_, IPC_STAT, &generic_shmid_ds_))) {
  //      perror("shmctl");
  //      exit(1);
  //    }
  //
  //    if (1 == generic_shmid_ds_.shm_nattch) {
  //      memset((void *)((NSE_MDS::NSETBTDataCommonStructProShm *)generic_mds_shm_struct_), 0,
  //             (NSE_MDS_QUEUE_SIZE * sizeof(NSE_MDS::NSETBTDataCommonStructProShm) + sizeof(int)));
  //    }
  //
  //    std::cout << "INITIALIZED OLD SHM" << std::endl ;
  //  }

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

    if ((shm_queue_ = (HFSAT::Shm1Queue<NSE_MDS::NSETBTDataCommonStructProShm> *)shmat(generic_mds_shmid_, NULL, 0)) ==
        (HFSAT::Shm1Queue<NSE_MDS::NSETBTDataCommonStructProShm> *)-1) {
      perror("shmat failed");
      exit(0);
    }

    if (-1 == (shmctl(generic_mds_shmid_, IPC_STAT, &generic_shmid_ds_))) {
      perror("shmctl");
      exit(1);
    }

    if (1 == generic_shmid_ds_.shm_nattch) {
      memset((void *)shm_queue_, 0, sizeof(HFSAT::Shm1Queue<NSE_MDS::NSETBTDataCommonStructProShm>));
    }
  }

  volatile uint64_t GetQueuePos() { return shm_queue_->writer_pos(); }

  virtual ~CombinedMDSMessagesNSEProShmBase() {}

  inline virtual void RunLiveShmSource(bool _keep_in_loop_ = true) = 0;
};

class CombinedMDSMessagesBSEShmBase {
 protected:
  key_t generic_mds_shm_key_;
  int generic_mds_shmid_;
  volatile EOBI_MDS::EOBICommonStruct *bse_generic_mds_shm_struct_;
  struct shmid_ds generic_shmid_ds_;

  HFSAT::Shm1Queue<EOBI_MDS::EOBICommonStruct> *bse_shm_queue_;
  unsigned int bse_queue_size_;
  bool using_thread_safe_shm_;

 public:
  CombinedMDSMessagesBSEShmBase()
      : generic_mds_shm_key_(BSE_MDS_SHM_KEY),
        generic_mds_shmid_(-1),
        bse_generic_mds_shm_struct_(NULL),
        bse_shm_queue_(nullptr),
        bse_queue_size_(sizeof(HFSAT::Shm1Queue<EOBI_MDS::EOBICommonStruct>)) {
    InitializeThreadSafeSHM();
  }

  //  void InitializeOldSHM() {
  //    if ((generic_mds_shmid_ =
  //             shmget(generic_mds_shm_key_,
  //                    (size_t)(BSE_MDS_QUEUE_SIZE * (sizeof(EOBI_MDS::EOBICommonStruct)) + sizeof(int)),
  //                    IPC_CREAT | 0666)) < 0) {
  //      if (errno == EINVAL)
  //        std::cerr << "Invalid segment size specified \n";
  //
  //      else if (errno == EEXIST)
  //        std::cerr << "Segment already exists \n";
  //
  //      else if (errno == EIDRM)
  //        std::cerr << " Segment is marked for deletion \n";
  //
  //      else if (errno == ENOENT)
  //        std::cerr << " Segment Doesn't Exist \n";
  //
  //      else if (errno == EACCES)
  //        std::cerr << " Permission Denied \n";
  //
  //      else if (errno == ENOMEM)
  //        std::cerr << " Not Enough Memory To Create Shm Segment \n";
  //
  //      exit(1);
  //    }
  //
  //    if ((bse_generic_mds_shm_struct_ = (volatile EOBI_MDS::EOBICommonStruct *)shmat(generic_mds_shmid_, NULL,
  //    0)) ==
  //        (volatile EOBI_MDS::EOBICommonStruct *)-1) {
  //      perror("shmat failed");
  //      exit(0);
  //    }
  //
  //    if (-1 == (shmctl(generic_mds_shmid_, IPC_STAT, &generic_shmid_ds_))) {
  //      perror("shmctl");
  //      exit(1);
  //    }
  //
  //    if (1 == generic_shmid_ds_.shm_nattch) {
  //      memset((void *)((EOBI_MDS::EOBICommonStruct *)bse_generic_mds_shm_struct_), 0,
  //             (BSE_MDS_QUEUE_SIZE * sizeof(EOBI_MDS::EOBICommonStruct) + sizeof(int)));
  //    }
  //
  //    std::cout << "INITIALIZED OLD SHM" << std::endl ;
  //  }

  void InitializeThreadSafeSHM() {
    if ((generic_mds_shmid_ = shmget(generic_mds_shm_key_, bse_queue_size_, IPC_CREAT | 0666)) < 0) {
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

    if ((bse_shm_queue_ = (HFSAT::Shm1Queue<EOBI_MDS::EOBICommonStruct> *)shmat(generic_mds_shmid_, NULL, 0)) ==
        (HFSAT::Shm1Queue<EOBI_MDS::EOBICommonStruct> *)-1) {
      perror("shmat failed");
      exit(0);
    }

    if (-1 == (shmctl(generic_mds_shmid_, IPC_STAT, &generic_shmid_ds_))) {
      perror("shmctl");
      exit(1);
    }

    if (1 == generic_shmid_ds_.shm_nattch) {
      memset((void *)bse_shm_queue_, 0, sizeof(HFSAT::Shm1Queue<EOBI_MDS::EOBICommonStruct>));
    }
  }

  volatile uint64_t GetQueuePosBSE() { return bse_shm_queue_->writer_pos(); }

  virtual ~CombinedMDSMessagesBSEShmBase() {}

  inline virtual void RunLiveShmSourceBSE(bool _keep_in_loop_ = true) = 0;
};

class CombinedMDSMessagesORSProShmBase {
 protected:
  key_t generic_mds_shm_key_;
  int generic_mds_shmid_;
  volatile HFSAT::GenericORSReplyStructLiveProShm *ors_reply_mds_shm_struct_;
  struct shmid_ds generic_shmid_ds_;

  HFSAT::Shm1Queue<HFSAT::GenericORSReplyStructLiveProShm> *ors_reply_shm_queue_;
  unsigned int ors_reply_queue_size_;
  bool using_thread_safe_shm_;

 public:
  CombinedMDSMessagesORSProShmBase()
      : generic_mds_shm_key_(ORS_REPLY_MDS_SHM_KEY),
        generic_mds_shmid_(-1),
        ors_reply_mds_shm_struct_(NULL),
        ors_reply_shm_queue_(nullptr),
        ors_reply_queue_size_(sizeof(HFSAT::Shm1Queue<HFSAT::GenericORSReplyStructLiveProShm>)) {
    InitializeThreadSafeSHM();
  }

  //  void InitializeOldSHM() {
  //    if ((generic_mds_shmid_ =
  //             shmget(generic_mds_shm_key_,
  //                    (size_t)(ORS_REPLY_MDS_QUEUE_SIZE * (sizeof(HFSAT::GenericORSReplyStructLiveProShm)) +
  //                    sizeof(int)),
  //                    IPC_CREAT | 0666)) < 0) {
  //      if (errno == EINVAL)
  //        std::cerr << "Invalid segment size specified \n";
  //
  //      else if (errno == EEXIST)
  //        std::cerr << "Segment already exists \n";
  //
  //      else if (errno == EIDRM)
  //        std::cerr << " Segment is marked for deletion \n";
  //
  //      else if (errno == ENOENT)
  //        std::cerr << " Segment Doesn't Exist \n";
  //
  //      else if (errno == EACCES)
  //        std::cerr << " Permission Denied \n";
  //
  //      else if (errno == ENOMEM)
  //        std::cerr << " Not Enough Memory To Create Shm Segment \n";
  //
  //      exit(1);
  //    }
  //
  //    if ((ors_reply_mds_shm_struct_ = (volatile HFSAT::GenericORSReplyStructLiveProShm *)shmat(generic_mds_shmid_,
  //    NULL, 0)) ==
  //        (volatile HFSAT::GenericORSReplyStructLiveProShm *)-1) {
  //      perror("shmat failed");
  //      exit(0);
  //    }
  //
  //    if (-1 == (shmctl(generic_mds_shmid_, IPC_STAT, &generic_shmid_ds_))) {
  //      perror("shmctl");
  //      exit(1);
  //    }
  //
  //    if (1 == generic_shmid_ds_.shm_nattch) {
  //      memset((void *)((HFSAT::GenericORSReplyStructLiveProShm *)ors_reply_mds_shm_struct_), 0,
  //             (ORS_REPLY_MDS_QUEUE_SIZE * sizeof(HFSAT::GenericORSReplyStructLiveProShm) + sizeof(int)));
  //    }
  //  }

  void InitializeThreadSafeSHM() {
    if ((generic_mds_shmid_ = shmget(generic_mds_shm_key_, ors_reply_queue_size_, IPC_CREAT | 0666)) < 0) {
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

    if ((ors_reply_shm_queue_ = (HFSAT::Shm1Queue<HFSAT::GenericORSReplyStructLiveProShm> *)shmat(
             generic_mds_shmid_, NULL, 0)) == (HFSAT::Shm1Queue<HFSAT::GenericORSReplyStructLiveProShm> *)-1) {
      perror("shmat failed");
      exit(0);
    }

    if (-1 == (shmctl(generic_mds_shmid_, IPC_STAT, &generic_shmid_ds_))) {
      perror("shmctl");
      exit(1);
    }

    if (1 == generic_shmid_ds_.shm_nattch) {
      memset((void *)ors_reply_shm_queue_, 0, sizeof(HFSAT::Shm1Queue<HFSAT::GenericORSReplyStructLiveProShm>));
    }
  }

  volatile uint64_t GetORSReplyQueuePos() { return ors_reply_shm_queue_->writer_pos(); }

  virtual ~CombinedMDSMessagesORSProShmBase() {}

  //  inline virtual void RunLiveShmSource(bool _keep_in_loop_ = true) = 0;
};

class CombinedMDSMessagesControlProShmBase {
 protected:
  key_t control_mds_shm_key_;
  int control_mds_shmid_;
  volatile HFSAT::GenericControlRequestStruct *control_mds_shm_struct_;
  struct shmid_ds control_shmid_ds_;

  HFSAT::Shm1Queue<HFSAT::GenericControlRequestStruct> *control_shm_queue_;
  unsigned int control_shm_queue_size_;
  bool using_thread_safe_shm_;

 public:
  CombinedMDSMessagesControlProShmBase()
      : control_mds_shm_key_(CONTROL_MDS_SHM_KEY),
        control_mds_shmid_(-1),
        control_mds_shm_struct_(NULL),
        control_shm_queue_(nullptr),
        control_shm_queue_size_(sizeof(HFSAT::Shm1Queue<HFSAT::GenericControlRequestStruct>)) {
    InitializeThreadSafeSHM();
  }

  void InitializeThreadSafeSHM() {
    if ((control_mds_shmid_ = shmget(control_mds_shm_key_, control_shm_queue_size_, IPC_CREAT | 0666)) < 0) {
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

    if ((control_shm_queue_ = (HFSAT::Shm1Queue<HFSAT::GenericControlRequestStruct> *)shmat(
             control_mds_shmid_, NULL, 0)) == (HFSAT::Shm1Queue<HFSAT::GenericControlRequestStruct> *)-1) {
      perror("shmat failed");
      exit(0);
    }

    if (-1 == (shmctl(control_mds_shmid_, IPC_STAT, &control_shmid_ds_))) {
      perror("shmctl");
      exit(1);
    }

    if (1 == control_shmid_ds_.shm_nattch) {
      memset((void *)control_shm_queue_, 0, sizeof(HFSAT::Shm1Queue<HFSAT::GenericControlRequestStruct>));
    }
  }

  volatile uint64_t GetControlMsgQueuePos() { return control_shm_queue_->writer_pos(); }

  virtual ~CombinedMDSMessagesControlProShmBase() {}
};

class CombinedMDSMessagesSignalProShmBase {
 protected:
  key_t signal_mds_shm_key_;
  int signal_mds_shmid_;
  volatile HFSAT::IVCurveData *signal_mds_shm_struct_;
  struct shmid_ds signal_shmid_ds_;

  HFSAT::Shm1Queue<HFSAT::IVCurveData> *signal_shm_queue_;
  unsigned int signal_shm_queue_size_;
  bool using_thread_safe_shm_;

 public:
  CombinedMDSMessagesSignalProShmBase()
      : signal_mds_shm_key_(SIGNAL_MDS_SHM_KEY),
        signal_mds_shmid_(-1),
        signal_mds_shm_struct_(NULL),
        signal_shm_queue_(nullptr),
        signal_shm_queue_size_(sizeof(HFSAT::Shm1Queue<HFSAT::IVCurveData>)) {
    InitializeThreadSafeSHM();
  }

  void InitializeThreadSafeSHM() {
    if ((signal_mds_shmid_ = shmget(signal_mds_shm_key_, signal_shm_queue_size_, IPC_CREAT | 0666)) < 0) {
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

    if ((signal_shm_queue_ = (HFSAT::Shm1Queue<HFSAT::IVCurveData> *)shmat(
             signal_mds_shmid_, NULL, 0)) == (HFSAT::Shm1Queue<HFSAT::IVCurveData> *)-1) {
      perror("shmat failed");
      exit(0);
    }

    if (-1 == (shmctl(signal_mds_shmid_, IPC_STAT, &signal_shmid_ds_))) {
      perror("shmctl");
      exit(1);
    }

    if (1 == signal_shmid_ds_.shm_nattch) {
      memset((void *)signal_shm_queue_, 0, sizeof(HFSAT::Shm1Queue<HFSAT::IVCurveData>));
    }
  }

  volatile uint64_t GetSignalMsgQueuePos() { return signal_shm_queue_->writer_pos(); }

  virtual ~CombinedMDSMessagesSignalProShmBase() {}
};


}
}
