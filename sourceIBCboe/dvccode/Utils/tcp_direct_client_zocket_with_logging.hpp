// =====================================================================================
//
//       Filename:  tcp_direct_client_zocket_with_logging.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  08/13/2018 10:03:39 AM
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

#include <zf/zf.h>
#include <zf/zf_utils.h>

#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/time.h>
#include <netinet/tcp.h>
#include <vector>
#include <sstream>

#include "dvccode/Utils/sem_utils.hpp"
#include "dvccode/Utils/async_writer.hpp"
#include "dvccode/Profiler/cpucycle_profiler.hpp"
#include "dvccode/ExternalData/simple_external_data_live_listener.hpp"
#include "dvccode/Utils/runtime_profiler.hpp"

namespace HFSAT {
namespace Utils {

//  struct rx_msg {
//    struct zft_msg msg;
//    struct iovec iov[1];
//  };

class EventTimeoutListener {
 public:
  virtual ~EventTimeoutListener() {}
  virtual void OnEventsTimeout() = 0;
};

//  class TCPDirectClientZocketMuxerRX {
//
//    private :
//      struct zf_attr * attr_ ;
//      struct zf_stack * stack_ ;
//      struct zf_muxer_set * muxer_ ;
//      int max_iov_;
//      struct epoll_event ev_;
//      struct zft* zock_;
//      AsyncWriter *pReader_;
//      ChannelId readChannel_;
//      volatile int tcp_direct_mutex_;
//      struct rx_msg msg_;
//
//      TCPDirectClientZocketMuxerRX () {
//        ZF_TRY(zf_init());
//        ZF_TRY(zf_attr_alloc(&attr_));
//        ZF_TRY(zf_stack_alloc(attr_, &stack_));
//        ZF_TRY(zf_muxer_alloc(stack_, &muxer_));
//        max_iov_ = sizeof(msg_.iov) / sizeof(msg_.iov[0]);
//        msg_.msg.iovcnt = max_iov_;
//      }
//
//      TCPDirectClientZocketMuxerRX ( TCPDirectClientZocketMuxerRX const & disabled_copy_constructor ) = delete;
//
//    public :
//
//      static TCPDirectClientZocketMuxerRX & GetUniqueInstance (){
//        static TCPDirectClientZocketMuxerRX unique_instance;
//        return unique_instance ;
//      }
//
//      struct zf_attr * GetZFAttr(){
//        return attr_;
//      }
//
//      struct zf_stack * GetZFStack(){
//        return stack_;
//      }
//
//      void SetUpAuditLoggerRX( AsyncWriter * pReader, std::string logfile){
//        pReader_ = pReader;
//        if (pReader != nullptr) {
//          readChannel_ = pReader_->add(logfile + ".in");
//        }
//      }
//
//      void Lock(){
//        HFSAT::SemUtils::spin_lock(&tcp_direct_mutex_);
//      }
//
//      void Unlock(){
//        HFSAT::SemUtils::spin_unlock(&tcp_direct_mutex_);
//      }
//
//      void AddTCPClientZocketToMuxer( struct zft* zock){
//        struct epoll_event event = { .events = EPOLLIN };
//        ZF_TRY(zf_muxer_add(muxer_, zft_to_waitable(zock), &event));
//        zock_ = zock;
//      }
//
//      void ReleaseBuffer(){
//        Lock();
//        zft_zc_recv_done(zock_, &msg_.msg);
//        Unlock();
//      }
//
//      char * ReadMoreEvents(int32_t & recv_length, bool & events_available, bool & pkts_left ){
//
//        pkts_left = false ;
//        events_available = false;
//
//        Lock();
//        zf_reactor_perform(stack_);
//        zft_zc_recv(zock_, &msg_.msg, 0);
//
//        if(msg_.msg.iovcnt){
//          recv_length = msg_.msg.iov[0].iov_len ;
////          zft_zc_recv_done(zock_, &msg_.msg);
//          events_available = true;
//          pkts_left = ( msg_.msg.pkts_left == 0 ) ? false : true ;
//        }
//
//        Unlock();
//
//        if (true == events_available && pReader_ != nullptr) {
//          pReader_->log(readChannel_, (const char *)(msg_.msg.iov[0].iov_base), recv_length);
//        }
//
//        return (char *)(msg_.msg.iov[0].iov_base);
//
//      }
//
//      char * WaitForEvents( int32_t & recv_length, bool & events_available, bool & pkts_left ){
//
//        events_available = false ;
//        pkts_left = false ;
//
//        while(!zf_stack_has_pending_work(stack_));
//
//        Lock();
//        int32_t n_events = zf_muxer_wait(muxer_, &ev_, 1, 0);
//
//        if ( n_events ) {
//          zft_zc_recv(zock_, &msg_.msg, 0);
//          if ( msg_.msg.iovcnt ) {
//            recv_length = msg_.msg.iov[0].iov_len ;
////            zft_zc_recv_done(zock_, &msg_.msg);
//            events_available = true ;
//            pkts_left = ( msg_.msg.pkts_left == 0 ) ? false : true ;
//          }
//        }
//
//        Unlock();
//
//        if (true == events_available && pReader_ != nullptr) {
//          pReader_->log(readChannel_, (const char *)(msg_.msg.iov[0].iov_base), recv_length);
//        }
//
//        return (char *)(msg_.msg.iov[0].iov_base);
//      }
//
//  };
//
//  class TCPDirectClientZocketTX {
//
//    private :
//
//      TCPDirectClientZocketMuxerRX & muxer_rx_ ;
//      struct zft* zock_;
//      int32_t packet_count_ ;
//      AsyncWriter *pWriter_;
//      ChannelId writeChannel_;
//
//    public :
//
//      TCPDirectClientZocketTX ( AsyncWriter *pWriter, AsyncWriter *pReader, std::string logfile ) :
//        muxer_rx_(TCPDirectClientZocketMuxerRX::GetUniqueInstance()),
//        packet_count_(0),
//        pWriter_(pWriter) {
//
//        muxer_rx_.SetUpAuditLoggerRX(pReader, logfile);
//
//        if (pWriter_ != nullptr) {
//          writeChannel_ = pWriter_->add(logfile + ".out");
//        }
//      }
//
//      void ConnectAndAddZocketToMuxer( std::string ip, int32_t port) {
//
//        std::ostringstream t_temp_oss;
//        t_temp_oss << ip << ":" << port ;
//
//        struct addrinfo* ai;
//        if( getaddrinfo_hostport(t_temp_oss.str().c_str(), NULL, &ai) != 0 ) {
//          std::cerr << "FAILED TO LOOKUP ADDRESS : " << t_temp_oss.str() << std::endl;
//          exit(-1);
//        }
//
//        struct zft_handle* tcp_handle;
//        ZF_TRY(zft_alloc(muxer_rx_.GetZFStack(), muxer_rx_.GetZFAttr(), &tcp_handle));
//
//        //Connect
//        ZF_TRY(zft_connect(tcp_handle, ai->ai_addr, ai->ai_addrlen, &zock_));
//        while(TCP_SYN_SENT == zft_state(zock_)) zf_reactor_perform(muxer_rx_.GetZFStack());
//        ZF_TEST(TCP_ESTABLISHED == zft_state(zock_));
//
//        muxer_rx_.AddTCPClientZocketToMuxer(zock_);
//
//        std::cout << "CONNECTION ESTABLISHED : " << std::endl;
//
//      }
//
//      inline int32_t WriteN( int32_t length, void * send_buf ){
//
//        HFSAT::CpucycleProfiler::GetUniqueInstance().Start(1);
//        packet_count_++;
//        muxer_rx_.Lock ();
//        int32_t testval = zft_send_single(zock_, send_buf, length, 0);
//        if( 20 == packet_count_){
//          zf_reactor_perform(muxer_rx_.GetZFStack());
//          packet_count_ = 0;
//        }
//        muxer_rx_.Unlock();
//        HFSAT::CpucycleProfiler::GetUniqueInstance().End(1);
//
//        if(testval > 0){
//          if (pWriter_ != nullptr) {
//            pWriter_->log(writeChannel_, (const char *)send_buf, testval);
//          }
//        }
//
//        return testval;
//
//      }
//
//  };

class TCPDirectLockFreeSocket {
 private:
  struct zf_attr *attr_;
  struct zf_stack *stack_;
  struct zf_muxer_set *muxer_;
  int max_iov_;
  struct epoll_event ev_;
  struct zft *zock_;
  AsyncWriter *pReader_;
  ChannelId readChannel_;
  int32_t packet_count_;
  AsyncWriter *pWriter_;
  ChannelId writeChannel_;
  int32_t pkts_left_count_;
  bool pkts_left_;
  bool initialized_;
  bool isexchnotnse;
  struct zft_msg msg;

 public:
  TCPDirectLockFreeSocket(AsyncWriter *pWriter, AsyncWriter *pReader, std::string logfile, std::string interface,const char* const& exch_ = HFSAT::EXCHANGE_KEYS::kExchSourceINVALIDStr) {
    ZF_TRY(zf_init());
    ZF_TRY(zf_attr_alloc(&attr_));
    ZF_TRY(zf_attr_set_str(attr_, "interface", interface.c_str()));
    ZF_TRY(zf_stack_alloc(attr_, &stack_));
    ZF_TRY(zf_muxer_alloc(stack_, &muxer_));
    msg.iovcnt = 1;

    pReader_ = pReader;
    if (pReader != nullptr) {
      readChannel_ = pReader_->add(logfile + ".in");
    }

    pWriter_ = pWriter;
    if (pWriter_ != nullptr) {
      writeChannel_ = pWriter_->add(logfile + ".out");
    }

    pkts_left_ = false;
    isexchnotnse=true;
    if(strcmp(exch_, HFSAT::EXCHANGE_KEYS::kExchSourceNSEStr)==0){
      std::cout<<"exchange NSE in TCPDirectLockFreeSocket"<<std::endl;
      isexchnotnse=false;
    }
    initialized_ = false;
  }

  void ConnectAndAddZocketToMuxer(std::string ip, int32_t port) {
    std::ostringstream t_temp_oss;
    t_temp_oss << ip << ":" << port;

    struct addrinfo *ai;
    if (getaddrinfo_hostport(t_temp_oss.str().c_str(), NULL, &ai) != 0) {
      std::cerr << "FAILED TO LOOKUP ADDRESS : " << t_temp_oss.str() << std::endl;
      exit(-1);
    }

    struct zft_handle *tcp_handle;
    ZF_TRY(zft_alloc(stack_, attr_, &tcp_handle));

    // Connect
    ZF_TRY(zft_connect(tcp_handle, ai->ai_addr, ai->ai_addrlen, &zock_));
    while (TCP_SYN_SENT == zft_state(zock_)) zf_reactor_perform(stack_);

    int32_t state = zft_state(zock_);
    std::cout << "STATE : " << state << std::endl;
    ZF_TEST(TCP_ESTABLISHED == state);

    struct epoll_event event = {.events = EPOLLIN};
    ZF_TRY(zf_muxer_add(muxer_, zft_to_waitable(zock_), &event));

    std::cout << "CONNECTION ESTABLISHED : " << std::endl;
    initialized_ = true;
  }

  void LogAuditIn(char* buffer, int32_t buffer_length){
    pReader_->log(readChannel_, (const char *) buffer, buffer_length);
  }
  void LogAuditIn(const char* buffer1, int32_t buffer_length1,const char* buffer2, int32_t buffer_length2 ){
    pReader_->log(readChannel_, (const char *) buffer1, buffer_length1,(const char *) buffer2, buffer_length2);
  }

  int32_t PacketLeft() { return pkts_left_count_; }

  char *ReadLockFreeNPendingEvents(int32_t &recv_length, bool &events_available) {
    pkts_left_count_ = 0;
    pkts_left_ = false;
    events_available = false;

    zf_reactor_perform(stack_);
    zft_zc_recv(zock_, &msg, 0);

    if (msg.iovcnt) {
      recv_length = msg.iov[0].iov_len;
      events_available = true;
      pkts_left_ = (msg.pkts_left == 0) ? false : true;
      pkts_left_count_ = msg.pkts_left;
      zft_zc_recv_done(zock_, &msg);
    }
    if(isexchnotnse){
      if (true == events_available) {
        pReader_->log(readChannel_, (const char *)(msg.iov[0].iov_base), recv_length);
      }
    }
    return (char *)(msg.iov[0].iov_base);
  }

  char *ReadLockFreeN(int32_t &recv_length, bool &events_available) {
    if (true == pkts_left_) return ReadLockFreeNPendingEvents(recv_length, events_available);

    events_available = false;
    pkts_left_ = false;
    pkts_left_count_ = 0;

    // stack doesn't have new events generated
    if (false == zf_stack_has_pending_work(stack_)) return nullptr;

    // if stack is ready but zocket doesn't have events, return
    if (0 == zf_muxer_wait(muxer_, &ev_, 1, 0)) return nullptr;

    zft_zc_recv(zock_, &msg, 0);
    if (msg.iovcnt) {
      recv_length = msg.iov[0].iov_len;
      events_available = true;
      pkts_left_ = (msg.pkts_left == 0) ? false : true;
      pkts_left_count_ = msg.pkts_left;
      zft_zc_recv_done(zock_, &msg);
    }
    if(isexchnotnse){
      if (true == events_available) {
        pReader_->log(readChannel_, (const char *)(msg.iov[0].iov_base), recv_length);
      }
    }
    return (char *)(msg.iov[0].iov_base);
  }

  inline void WriteLockFreeNWarmOnly(int32_t length, void *send_buf) {
    if (false == initialized_) return;
    zft_send_single_warm(zock_, send_buf, length);
  }

  inline int32_t WriteLockFreeN(int32_t length, void *send_buf) {
    packet_count_++;
    int32_t testval = zft_send_single(zock_, send_buf, length, 0);
    if (20 == packet_count_) {
      zf_reactor_perform(stack_);
      packet_count_ = 0;
    }
    //Disabling audit out
    /*
    if (testval > 0) {
      if (pWriter_ != nullptr) {
        pWriter_->log(writeChannel_, (const char *)send_buf, testval);
      }
    }
    */
    return testval;
  }

  void Close(){
      int free_socket = zft_shutdown_tx (zock_);
      std::cout << "SOCKET CLOSED WITH VAL " << free_socket << std::endl;
  }

};

struct UDPSocket {
  struct zfur *udp_direct_zocket_;
  bool packets_left_;
  uint32_t socket_fd_to_last_seen_seq_;
  char segment_type_;
  bool is_trade_exec_fd_;
  bool is_spot_idx_fd_;
  bool is_oi_data_fd_;
  HFSAT::SimpleExternalDataLiveListener *listener_;
  struct zf_waitable *zfwait_;
  std::string ip_;
};

class UDPDirectMultipleZocket {
 private:
  struct zf_attr *attr_;
  struct zf_stack *stack_;
  struct zf_muxer_set *muxer_;
  int max_iov_;
  struct epoll_event ev_[64];
  struct UDPSocket udp_sockets_[64];
  int32_t last_allocated_udp_socket_counter_;
  HFSAT::RuntimeProfiler &runtime_profiler_;
  EventTimeoutListener *event_timeout_listener_;
  struct zfur_msg msg;

 public:
  UDPDirectMultipleZocket(std::string _interface_ = "DUMMY") : runtime_profiler_(HFSAT::RuntimeProfiler::GetUniqueInstance()) {

    last_allocated_udp_socket_counter_ = 0 ;

    ZF_TRY(zf_init());
    ZF_TRY(zf_attr_alloc(&attr_));

    if( std::string::npos == _interface_.find("DUMMY")){
      ZF_TRY(zf_attr_set_str(attr_, "interface", _interface_.c_str()));
    }

    std::cout << "ZOCKET CREATED ON INTERFACE : " << _interface_ << std::endl;

    int32_t value = zf_stack_alloc(attr_, &stack_);
    std::cout << "STACK VALUE : " << value << std::endl;
    ZF_TRY(value);
    ZF_TRY(zf_muxer_alloc(stack_, &muxer_));

    std::cout << " MUXER CREATED : " << _interface_ << " " << muxer_ << std::endl ;

    msg.iovcnt = 1;
  }

  UDPDirectMultipleZocket(UDPDirectMultipleZocket const &disabled_copy_constructor) = delete;

 public:
  static UDPDirectMultipleZocket &GetUniqueInstance() {
    static UDPDirectMultipleZocket unique_instance;
    return unique_instance;
  }

  void CloseRecoverySocket() {
    for ( int index_ = 0 ; index_ < last_allocated_udp_socket_counter_ ; ++index_ ) {
      if (udp_sockets_[last_allocated_udp_socket_counter_].is_trade_exec_fd_ == true) {
        int close_recovery_socket = zf_muxer_del(udp_sockets_[last_allocated_udp_socket_counter_].zfwait_);
        if (close_recovery_socket < 0) {
          std::cerr << "FAILED TO CLOSE RECOVERY SOCKET: " << udp_sockets_[last_allocated_udp_socket_counter_].ip_ << " ERROR_CODE: " << close_recovery_socket << std::endl; 
        } else {
          std::cout << "RECOVERY SOCKET CLOSED: " << udp_sockets_[last_allocated_udp_socket_counter_].ip_ << std::endl;
        }
      }
    }
  }

  void CreateSocketAndAddToMuxer(std::string ip, int32_t port, HFSAT::SimpleExternalDataLiveListener *listener,
                                 char seg_type, bool is_trade_exec_fd, bool is_spot_idx_fd, bool is_oi_data_fd) {
//    std::cout << "CREATE ZOCKET COUNTER :" << last_allocated_udp_socket_counter_ << " " << ip << " " << port << std::endl ;
    if (last_allocated_udp_socket_counter_ > 63) {
      std::cout << "CAN'T ALLOCATE ANY MORE UDP ZOCKETS : " << std::endl;
      std::exit(-1);
    }

    std::ostringstream t_temp_oss;
    t_temp_oss << ip << ":" << port;

    struct addrinfo *ai;
    if (getaddrinfo_hostport(t_temp_oss.str().c_str(), NULL, &ai) != 0) {
      std::cerr << "FAILED TO LOOKUP ADDRESS : " << t_temp_oss.str() << std::endl;
      exit(-1);
    }

    //        struct zfur *ur = udp_sockets_[last_allocated_udp_socket_counter_].udp_direct_zocket_;
    ZF_TRY(zfur_alloc(&(udp_sockets_[last_allocated_udp_socket_counter_].udp_direct_zocket_), stack_, attr_));
    struct zfur *ur = udp_sockets_[last_allocated_udp_socket_counter_].udp_direct_zocket_;

    ZF_TRY(zfur_addr_bind(ur, ai->ai_addr, ai->ai_addrlen, NULL, 0, 0));

    struct epoll_event event;
    event.events = EPOLLIN;

    event.data.ptr = &(udp_sockets_[last_allocated_udp_socket_counter_]);

    udp_sockets_[last_allocated_udp_socket_counter_].listener_ = listener;
    udp_sockets_[last_allocated_udp_socket_counter_].socket_fd_to_last_seen_seq_ = UINT_MAX;
    udp_sockets_[last_allocated_udp_socket_counter_].segment_type_ = seg_type;
    udp_sockets_[last_allocated_udp_socket_counter_].is_trade_exec_fd_ = is_trade_exec_fd;
    udp_sockets_[last_allocated_udp_socket_counter_].is_oi_data_fd_ = is_oi_data_fd;
    udp_sockets_[last_allocated_udp_socket_counter_].is_spot_idx_fd_ = is_spot_idx_fd;

    struct zf_waitable *zfw = zfur_to_waitable(ur);
    ZF_TRY(zf_muxer_add(muxer_, zfw, &event));
    udp_sockets_[last_allocated_udp_socket_counter_].zfwait_ = zfw;
    udp_sockets_[last_allocated_udp_socket_counter_].ip_ = ip;

    last_allocated_udp_socket_counter_++;

    std::cout << "SOCKET CREATED : " << ip << " " << port << std::endl;
  }

  void AddEventTimeoutNotifyListener(EventTimeoutListener *listener) { event_timeout_listener_ = listener; }

  inline void ReadAndDispatchEvents(UDPSocket *udp_socket) {
    do {
      zfur_zc_recv(udp_socket->udp_direct_zocket_, &msg, 0);
      runtime_profiler_.Start(HFSAT::ProfilerType::kCOMBINEDSHMWRITER);
      (udp_socket->listener_)
          ->ProcessEventsFromUDPDirectRead((char *)msg.iov[0].iov_base, msg.iov[0].iov_len, udp_socket->segment_type_,
                                           udp_socket->is_trade_exec_fd_, udp_socket->is_spot_idx_fd_, udp_socket->is_oi_data_fd_, udp_socket->socket_fd_to_last_seen_seq_);
      zfur_zc_recv_done(udp_socket->udp_direct_zocket_, &msg);
    } while (msg.dgrams_left);
  }

  inline void NotifyTimeOut() { event_timeout_listener_->OnEventsTimeout(); }

  void RunLiveDispatcherWithTimeOut(int64_t timeout) {
    int64_t wait_timeout = timeout;

    while (true) {
      int32_t no_of_events = zf_muxer_wait(muxer_, ev_, 64, wait_timeout);

      if (0 == no_of_events) {
        NotifyTimeOut();
        continue;
      }

      for (int32_t ev_counter = 0; ev_counter < no_of_events; ev_counter++) {
        ReadAndDispatchEvents((UDPSocket *)((ev_[ev_counter].data).ptr));
      }
    }
  }

  void RunLiveDispatcher() {
    while (true) {
      int32_t no_of_events = zf_muxer_wait(muxer_, ev_, 64, -1);

      for (int32_t ev_counter = 0; ev_counter < no_of_events; ev_counter++) {
        ReadAndDispatchEvents((UDPSocket *)((ev_[ev_counter].data).ptr));
      }
    }
  }

  void ProcessEventsFromZocket(){

//    std::cout << "ProcessEventsFromZocket Start : " << muxer_ << std::endl ;

//    std::cout << "Going For Muxer Wait : "<< std::endl ;
    int32_t no_of_events = zf_muxer_wait(muxer_, ev_, 1, 0);
//    std::cout << "ProcessEventsFromZocket Events : " << no_of_events << std::endl ;

    for (int32_t ev_counter = 0; ev_counter < no_of_events; ev_counter++) {
//      std::cout << "Process Event : " << ev_counter << std::endl ;
      ReadAndDispatchEvents((UDPSocket *)((ev_[ev_counter].data).ptr));
//      std::cout << "Process Event End " << std::endl ;
    }
  }

  void RunLiveDispatcherOEBU() {
      int32_t no_of_events = zf_muxer_wait(muxer_, ev_, 64, -1);

      for (int32_t ev_counter = 0; ev_counter < no_of_events; ev_counter++) {
        ReadAndDispatchEvents((UDPSocket *)((ev_[ev_counter].data).ptr));
      }
  }

};
}
}
