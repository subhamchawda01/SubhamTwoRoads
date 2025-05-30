/**
    \file dvccode/ExternalData/simple_external_data_live_listener.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#pragma once

#include <sys/time.h>
#include <exanic/exanic.h>
#include <exanic/fifo_rx.h>
#include <exanic/filter.h>


namespace HFSAT {

class SimpleExternalDataLiveListener {
 public:
  virtual ~SimpleExternalDataLiveListener() {}

  /// processes all events it can read. Typically there should be some data already read in the buffer.
  virtual void ProcessAllEvents(int this_socket_fd_) = 0;
  virtual void ProcessEventsFromUDPDirectRead(char const* msg_ptr, int32_t msg_length, char seg_type,
                                              bool is_trade_exec_fd, bool is_spot_index_fd, bool is_oi_data_fd, uint32_t& udp_msg_seq_no){
    std::cout<<"SimpleExternalDataLiveListener ProcessEventsFromUDPDirectRead..." << std::endl;
    std::exit(-1);
  }
  virtual void ProcessAllEventsExanic (char *msg_ptr, ssize_t msg_length, int fd,char segment, bool is_tradeRange, bool is_spot_) {};
  virtual void CleanUp() {}
};

typedef SimpleExternalDataLiveListener* SimpleExternalDataLiveListenerPtr;
typedef std::vector<SimpleExternalDataLiveListener*> SimpleExternalDataLiveListenerPtrVec;
typedef std::vector<SimpleExternalDataLiveListener*>::iterator SimpleExternalDataLiveListenerPtrVecIter;
typedef std::vector<SimpleExternalDataLiveListener*>::const_iterator SimpleExternalDataLiveListenerPtrVecConstIter;
}
