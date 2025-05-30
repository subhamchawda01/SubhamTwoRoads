/**
    \file dvccode/ExternalData/simple_live_dispatcher.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#ifndef BASE_EXTERNALDATA_SIMPLE_LIVE_DISPATCHER_HPP
#define BASE_EXTERNALDATA_SIMPLE_LIVE_DISPATCHER_HPP

#include <sys/select.h>
#include <vector>
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/ExternalData/simple_external_data_live_listener.hpp"
#include "dvccode/Profiler/cpucycle_profiler.hpp"

namespace HFSAT {

/// Livetrading, takes the socket file descriptors and selects on them
/// and for the ones that returns true on FD_ISSET
/// calls them to process the data they have.
/// Main difference : no time based stuff .
/// Also here please be careful in adding the more important SimpleExternalDataLiveListeners first
class SimpleLiveDispatcher {
 protected:
  SimpleExternalDataLiveListenerPtr* simple_external_data_live_listener_array_;
  int* socket_fd_array_;
  unsigned int simple_external_data_live_listener_array_size_;
  unsigned int simple_external_data_live_listener_array_capacity_;
  int max_socket_file_descriptor_plus1_;

  unsigned int temp_arr_size;
  unsigned int temp_arr_capacity_;
  bool* temp_add_delete_info_is_add;  // true for listener add, false for listener remove
  SimpleExternalDataLiveListenerPtr* temp_simple_external_data_live_listener_array_;
  int* temp_socket_fd_array_;

#if USE_SHM_FOR_LIVESOURCES

  std::vector<int> shm_sockets_;
  std::vector<SimpleExternalDataLiveListener*> shm_listeners_;

#endif

  std::vector<SimpleExternalDataLiveListener*> primary_socket_listeners_;

  bool keep_me_running_;
  int timeout_;

 public:
  /// Contructor
  SimpleLiveDispatcher(int timeout = 0);

  ~SimpleLiveDispatcher();

  /// call to add a < source, sockfd > that dispatcher will need to work with
  void AddSimpleExternalDataLiveListenerSocket(SimpleExternalDataLiveListener* _new_listener_, int socket_fd_,
                                               bool _is_local_data_socket_ = false);

#if USE_SHM_FOR_LIVESOURCES

  void AddSimpleExternalDataLiveListenerShmSocket(SimpleExternalDataLiveListener* _new_listener_,
                                                  int socket_fd_);  // no notion of primary socket here for shm writer

#endif
  // Remove a socket
  void RemoveSimpleExternalDataLiveListenerSocket(SimpleExternalDataLiveListener* _to_remove_, int socket_fd_,
                                                  bool _is_local_data_socket_ = false);

  /// called from main after all the sources have been added
  void RunLive();

  void CleanUp() { keep_me_running_ = false; }

 private:
  void FlushTempBuffers();

  void DeleteSources();

  template <class T>
  inline void doubleCapacityOfArray(T*& arr, const unsigned int origCapacity);
  void DoubleEDLLSize();
  void DoubleTempBufferSize();
};
}
#endif  // BASE_EXTERNALDATA_SIMPLE_LIVE_DISPATCHER_HPP
