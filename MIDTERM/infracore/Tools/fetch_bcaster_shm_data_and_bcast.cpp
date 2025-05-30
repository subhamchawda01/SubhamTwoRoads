#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>

#include <iostream>
#include <fstream>
#include <map>

#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/ors_messages.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/TradingInfo/network_account_interface_manager.hpp"
#include "dvccode/Utils/multicast_sender_socket.hpp"

volatile HFSAT::GenericORSReplyStruct *ORSReply_queue;
struct shmid_ds shm_ds;
int shmid = -1;

void sighandler(int signum) {
  shmdt((void *)ORSReply_queue);
  if (shmctl(shmid, IPC_STAT, &shm_ds) == -1) {
    perror("shmctl");
    exit(1);
  }
  if (shm_ds.shm_nattch == 0) shmctl(shmid, IPC_RMID, 0);  // remove shm segment if no users are attached

  exit(0);
}

int main(int argc, char *argv[]) {
  if (argc < 3) {
    std::cout << "Usage ./exe <bcast_ip> <bcast_port>" << std::endl;
    exit(1);
  }
  int index_ = -1, bcast_port_ = atoi(argv[2]);
  std::string bcast_ip_ = argv[1];

  struct sigaction sigact;
  sigact.sa_handler = sighandler;
  sigaction(SIGINT, &sigact, NULL);

  if ((shmid = shmget(SHM_KEY_BCASTER, (size_t)(4000 * (sizeof(HFSAT::GenericORSReplyStruct)) + sizeof(int)),
                      IPC_CREAT | 0666)) < 0) {
    std::cout << "Size of segment = " << 4000 * sizeof(HFSAT::GenericORSReplyStruct)
              << " bytes key = " << SHM_KEY_BCASTER << std::endl;

    printf("Failed to shmget error = %s\n", strerror(errno));
    if (errno == EINVAL)
      printf("Invalid segment size specified\n");
    else if (errno == EEXIST)
      printf("Segment exists, cannot create it\n");
    else if (errno == EIDRM)
      printf("Segment is marked for deletion or was removed\n");
    else if (errno == ENOENT)
      printf("Segment does not exist\n");
    else if (errno == EACCES)
      printf("Permission denied\n");
    else if (errno == ENOMEM)
      printf("Not enough memory to create segment\n");

    exit(1);
  }

  if ((ORSReply_queue = (volatile HFSAT::GenericORSReplyStruct *)shmat(shmid, NULL, 0)) ==
      (volatile HFSAT::GenericORSReplyStruct *)-1) {
    perror("shmat failed");
    exit(0);
  }

  if (shmctl(shmid, IPC_STAT, &shm_ds) == -1) {
    perror("shmctl");
    exit(1);
  }

  if (shm_ds.shm_nattch == 1) {
    memset((void *)ORSReply_queue, 0, (4000 * (sizeof(HFSAT::GenericORSReplyStruct)) + sizeof(int)));
  }

  HFSAT::GenericORSReplyStruct reply_msg;
  memset((void *)&reply_msg, 0, sizeof(HFSAT::GenericORSReplyStruct));
  HFSAT::MulticastSenderSocket *mcast_sender_socket_ = new HFSAT::MulticastSenderSocket(
      bcast_ip_, bcast_port_, HFSAT::NetworkAccountInterfaceManager::instance().GetInterfaceForApp(HFSAT::k_ORS));
  while (true) {
    volatile int queue_position_ = *((int *)(ORSReply_queue + 4000));
    if (index_ == -1) {
      index_ = queue_position_;
    }

    if (index_ == queue_position_) {
      continue;
    }

    index_ = (index_ + 1) % 4000;
    memcpy(&reply_msg, (HFSAT::GenericORSReplyStruct *)(ORSReply_queue + index_), sizeof(HFSAT::GenericORSReplyStruct));
    mcast_sender_socket_->WriteN(sizeof(HFSAT::GenericORSReplyStruct), &reply_msg);
  }
}
