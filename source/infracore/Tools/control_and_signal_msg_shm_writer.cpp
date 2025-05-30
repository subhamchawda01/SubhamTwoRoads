// =====================================================================================
// 
//       Filename:  control_and_signal_msg_shm_writer.cpp
// 
//    Description:  
// 
//        Version:  1.0
//        Created:  04/20/2023 07:31:25 AM
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


#include <cstdlib>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "dvccode/CDef/assumptions.hpp"
#include "dvccode/CDef/signal_msg.hpp"
#include "dvccode/CDef/control_messages.hpp"
#include "dvccode/Utils/shm1_queue.hpp"
#include "dvccode/TradingInfo/network_account_info_manager.hpp"
#include "dvccode/TradingInfo/network_account_interface_manager.hpp"
#include "dvccode/Utils/multicast_receiver_socket.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/ExternalData/simple_live_dispatcher.hpp"

class ControlMsgCapture : public HFSAT::SimpleExternalDataLiveListener{
 private:
  HFSAT::MulticastReceiverSocket *control_multicast_receiver_socket_;
  HFSAT::GenericControlRequestStruct control_request_;
  key_t key;
  int shmid;

  volatile HFSAT::GenericControlRequestStruct *mds_shm_queue_, *mds_shm_queue_pointer_;

  struct shmid_ds shm_ds;
  int count;
  int last_write_seq_num_;
  volatile int *shm_queue_index_;

  HFSAT::Shm1Queue<HFSAT::GenericControlRequestStruct> *shm_queue_;
  unsigned int queue_size_;

  int32_t ConsumersAttached() {
    int32_t shmctl_ret_val = shmctl(shmid, IPC_STAT, &shm_ds);
    return (shmctl_ret_val < 0 ? 0 : shm_ds.shm_nattch);
  }

  void InitializeThreadSafeSHM() {
    if ((shmid = shmget(key, queue_size_, IPC_CREAT | 0666)) < 0) {
      std::cerr << "Sizeof Of the Segment " << queue_size_ << " Key Value : " << key << "\n";
      std::cerr << "Failed to get segment : " << strerror(errno) << "\n";

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

    if ((shm_queue_ = (HFSAT::Shm1Queue<HFSAT::GenericControlRequestStruct> *)shmat(shmid, NULL, 0)) ==
        (HFSAT::Shm1Queue<HFSAT::GenericControlRequestStruct> *)-1) {
      perror("shmat");
      exit(0);
    }

    if (shmctl(shmid, IPC_STAT, &shm_ds) == -1) {
      perror("shmctl");
      exit(1);
    }

    if (ConsumersAttached() == 1) {
      memset((void *)shm_queue_, 0, queue_size_);
      shm_queue_->Reset();
    }
  }
 public:
  ControlMsgCapture()
      : control_multicast_receiver_socket_(NULL),
        control_request_(),
        key(CONTROL_MDS_SHM_KEY),
        shmid(-1),
        shm_queue_(nullptr),
        queue_size_(sizeof(HFSAT::Shm1Queue<HFSAT::GenericControlRequestStruct>)) {

          std::cout << " QUEUE SIZE : " << queue_size_ << std::endl;

    HFSAT::NetworkAccountInfoManager network_account_info_manager;
    HFSAT::DataInfo control_recv_data_info = network_account_info_manager.GetControlRecvUDPDirectDataInfo();

    control_multicast_receiver_socket_ = new HFSAT::MulticastReceiverSocket(
        control_recv_data_info.bcast_ip_, control_recv_data_info.bcast_port_,
        HFSAT::NetworkAccountInterfaceManager::instance().GetInterfaceForApp(HFSAT::k_Control));

    std::cout << "LISTENING ON : " << control_recv_data_info.bcast_ip_ << " " << control_recv_data_info.bcast_port_
              << " " << HFSAT::NetworkAccountInterfaceManager::instance().GetInterfaceForApp(HFSAT::k_Control)
              << std::endl;

    InitializeThreadSafeSHM();
  }

  inline void ProcessAllEvents(int32_t socket_fd) {
    int32_t read_size = control_multicast_receiver_socket_->ReadN(sizeof(HFSAT::GenericControlRequestStruct), &control_request_);
    std::cout << " Read Size : " << read_size << std::endl;
    if(read_size != sizeof(HFSAT::GenericControlRequestStruct)){
      std::cout << "CAN'T PROCESS MSG OF SIZE :" << read_size
                << " EXPECTED SIZE : " << sizeof(HFSAT::GenericControlRequestStruct) << std::endl;
    }else{
      std::cout << "RECEIVED MSG SIZE : " << read_size << " " << control_request_.ToString() << std::endl;
      shm_queue_->PushLockFree(control_request_);
    }
  }

  HFSAT::MulticastReceiverSocket *GetMcastReceiverSocket(){
    return control_multicast_receiver_socket_;
  }

};


class SignalMsgCapture : public HFSAT::SimpleExternalDataLiveListener{
 private:
  HFSAT::MulticastReceiverSocket *signal_multicast_receiver_socket_;
  HFSAT::IVCurveData signal_request_;
  key_t key;
  int shmid;

  volatile HFSAT::IVCurveData *mds_shm_queue_, *mds_shm_queue_pointer_;

  struct shmid_ds shm_ds;
  int count;
  int last_write_seq_num_;
  volatile int *shm_queue_index_;

  HFSAT::Shm1Queue<HFSAT::IVCurveData> *shm_queue_;
  unsigned int queue_size_;

  int32_t ConsumersAttached() {
    int32_t shmctl_ret_val = shmctl(shmid, IPC_STAT, &shm_ds);
    return (shmctl_ret_val < 0 ? 0 : shm_ds.shm_nattch);
  }

  void InitializeThreadSafeSHM() {
    if ((shmid = shmget(key, queue_size_, IPC_CREAT | 0666)) < 0) {
      std::cerr << "Sizeof Of the Segment " << queue_size_ << " Key Value : " << key << "\n";
      std::cerr << "Failed to get segment : " << strerror(errno) << "\n";

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

    if ((shm_queue_ = (HFSAT::Shm1Queue<HFSAT::IVCurveData> *)shmat(shmid, NULL, 0)) ==
        (HFSAT::Shm1Queue<HFSAT::IVCurveData> *)-1) {
      perror("shmat");
      exit(0);
    }

    if (shmctl(shmid, IPC_STAT, &shm_ds) == -1) {
      perror("shmctl");
      exit(1);
    }

    if (ConsumersAttached() == 1) {
      memset((void *)shm_queue_, 0, queue_size_);
      shm_queue_->Reset();
    }
  }
 public:
  SignalMsgCapture()
      : signal_multicast_receiver_socket_(NULL),
        signal_request_(),
        key(SIGNAL_MDS_SHM_KEY),
        shmid(-1),
        shm_queue_(nullptr),
        queue_size_(sizeof(HFSAT::Shm1Queue<HFSAT::IVCurveData>)) {

          std::cout << " QUEUE SIZE : " << queue_size_ << std::endl;

    HFSAT::NetworkAccountInfoManager network_account_info_manager;
    HFSAT::DataInfo signal_recv_data_info = network_account_info_manager.GetSignalRecvUDPDirectDataInfo();

    signal_multicast_receiver_socket_ = new HFSAT::MulticastReceiverSocket(
        signal_recv_data_info.bcast_ip_, signal_recv_data_info.bcast_port_,
        HFSAT::NetworkAccountInterfaceManager::instance().GetInterfaceForApp(HFSAT::k_Control));

    std::cout << "LISTENING ON : " << signal_recv_data_info.bcast_ip_ << " " << signal_recv_data_info.bcast_port_
              << " " << HFSAT::NetworkAccountInterfaceManager::instance().GetInterfaceForApp(HFSAT::k_Control)
              << std::endl;

    InitializeThreadSafeSHM();
  }

  HFSAT::MulticastReceiverSocket *GetMcastReceiverSocket(){
    return signal_multicast_receiver_socket_;
  }

  inline void ProcessAllEvents(int32_t socket_fd) {
    int32_t read_size = signal_multicast_receiver_socket_->ReadN(sizeof(HFSAT::IVCurveData), &signal_request_);
    if(read_size != sizeof(HFSAT::IVCurveData)){
      std::cout << "CAN'T PROCESS MSG OF SIZE :" << read_size
                << " EXPECTED SIZE : " << sizeof(HFSAT::IVCurveData) << std::endl;
    }else{
      std::cout << "RECEIVED MSG SIZE : " << read_size << " " << signal_request_.ToString() << std::endl;
      shm_queue_->PushLockFree(signal_request_);
    }
  }

};

int main(int argc, char *argv[]) {

  ControlMsgCapture control_msg_capture;
  SignalMsgCapture signal_msg_capture;

  HFSAT::SimpleLiveDispatcher simple_live_dispatcher;
  simple_live_dispatcher.AddSimpleExternalDataLiveListenerSocket(&control_msg_capture, control_msg_capture.GetMcastReceiverSocket()->socket_file_descriptor(), true);
  simple_live_dispatcher.AddSimpleExternalDataLiveListenerSocket(&signal_msg_capture, signal_msg_capture.GetMcastReceiverSocket()->socket_file_descriptor(), true);

  std::cout << " Socket FD : " << control_msg_capture.GetMcastReceiverSocket()->socket_file_descriptor() << " " << signal_msg_capture.GetMcastReceiverSocket()->socket_file_descriptor() << std::endl;

  simple_live_dispatcher.RunLive();
}

