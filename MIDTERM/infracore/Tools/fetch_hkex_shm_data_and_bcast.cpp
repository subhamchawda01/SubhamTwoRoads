// =====================================================================================
//
//       Filename:  hkex_shm_data_logger.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  02/22/2013 07:26:33 PM
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

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <getopt.h>
#include <unistd.h>

#include <iostream>
#include <fstream>
#include <map>

#include "dvccode/Utils/mds_logger.hpp"
#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/TradingInfo/network_account_interface_manager.hpp"
#include "dvccode/Utils/multicast_sender_socket.hpp"

std::string exch_ = "";

class LiveMDSLogger {
 protected:
  bool set_time;  // set time by logger. we already expect the m-casted data to have time, but we want the logger
                  // time-stamp to be set explicitly

  key_t hk_mds_shm_key_;
  int hk_mds_shmid_;

  volatile HKEX_MDS::HKEXCommonStruct *hk_mds_shm_struct_;
  struct shmid_ds hk_shmid_ds_;

  // this is meant to be a local copy to track queue position, compare with volatile shared memory counter
  int index_;

  std::string bcast_ip_;
  int bcast_port_;

 public:
  LiveMDSLogger(std::string exch_, bool set_time_, std::string _bcast_ip_, int _bcast_port_)
      : set_time(set_time_),
        hk_mds_shm_key_(SHM_KEY_HK),
        hk_mds_shmid_(-1),
        hk_mds_shm_struct_(NULL),
        index_(-1),
        bcast_ip_(_bcast_ip_),
        bcast_port_(_bcast_port_)

  {
    if ((hk_mds_shmid_ =
             shmget(hk_mds_shm_key_, (size_t)(HK_SHM_QUEUE_SIZE * (sizeof(HKEX_MDS::HKEXCommonStruct)) + sizeof(int)),
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

    if ((hk_mds_shm_struct_ = (volatile HKEX_MDS::HKEXCommonStruct *)shmat(hk_mds_shmid_, NULL, 0)) ==
        (volatile HKEX_MDS::HKEXCommonStruct *)-1) {
      perror("shmat failed");
      exit(0);
    }

    if (shmctl(hk_mds_shmid_, IPC_STAT, &hk_shmid_ds_) == -1) {
      perror("shmctl");
      exit(1);
    }

    if (hk_shmid_ds_.shm_nattch == 1) {
      memset((void *)((HKEX_MDS::HKEXCommonStruct *)hk_mds_shm_struct_), 0,
             (HK_SHM_QUEUE_SIZE * sizeof(HKEX_MDS::HKEXCommonStruct) + sizeof(int)));
    }
  }

  ~LiveMDSLogger() {
    shmdt((void *)hk_mds_shm_struct_);

    if (shmctl(hk_mds_shmid_, IPC_STAT, &hk_shmid_ds_) == -1) {
      perror("shmctl");
      exit(1);
    }

    if (hk_shmid_ds_.shm_nattch == 0) shmctl(hk_mds_shmid_, IPC_RMID, 0);  // remove shm segment if no users are
                                                                           // attached
  }

  void runLogger() {
    HKEX_MDS::HKEXCommonStruct cstr_;

    memset((void *)&cstr_, 0, sizeof(HKEX_MDS::HKEXCommonStruct));

    HFSAT::MulticastSenderSocket *mcast_sender_socket_ = new HFSAT::MulticastSenderSocket(
        bcast_ip_, bcast_port_, HFSAT::NetworkAccountInterfaceManager::instance().GetInterface(
                                    HFSAT::kExchSourceHONGKONG, HFSAT::k_MktDataLive));

    std::cout << " SHM - BCAST IP : " << bcast_ip_ << " Port : " << bcast_port_ << " Interface : "
              << HFSAT::NetworkAccountInterfaceManager::instance().GetInterface(HFSAT::kExchSourceHONGKONG,
                                                                                HFSAT::k_MktDataLive)
              << "\n";

    while (true) {
      // has to be volatile, waiting on shared memory segment queue position
      volatile int queue_position_ = *((int *)(hk_mds_shm_struct_ + HK_SHM_QUEUE_SIZE));

      // events are available only if the queue position at source is higher by 1, circular queue, will lag behind by 1
      // packet
      if (index_ == -1) {
        index_ = queue_position_;
      }

      if (index_ == queue_position_) {
        continue;
      }

      index_ = (index_ + 1) % HK_SHM_QUEUE_SIZE;

      // memcpy is only done as safegaurd from writer writing the same segment
      memcpy(&cstr_, (HKEX_MDS::HKEXCommonStruct *)(hk_mds_shm_struct_ + index_), sizeof(HKEX_MDS::HKEXCommonStruct));

      gettimeofday(&(cstr_.time_), NULL);

      mcast_sender_socket_->WriteN(sizeof(HKEX_MDS::HKEXCommonStruct), &cstr_);
    }
  }

  void closeFiles() {}
};

void *logger;

/// signal handling
void sighandler(int signum) {
  if (logger != NULL) {
    if (exch_.compare("HKEX") == 0) {
      ((LiveMDSLogger *)logger)->closeFiles();
    }
  }
  exit(0);
}

int main(int argc, char **argv) {
  std::string bcast_ip_ = "";
  int bcast_port_ = -1;

  if (argc < 3) {
    std::cerr << "Usage < exec > < bcast-ip > < bcast-port > \n";
    exit(-1);
  }

  bcast_ip_ = argv[1];
  bcast_port_ = atoi(argv[2]);

  /// set signal handler .. add other signals later
  struct sigaction sigact;
  sigact.sa_handler = sighandler;
  sigaction(SIGINT, &sigact, NULL);

  // true is for settime
  logger = (void *)(new LiveMDSLogger("HKEX", true, bcast_ip_, bcast_port_));
  ((LiveMDSLogger *)logger)->runLogger();

  return 0;
}
