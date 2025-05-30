/**
   \file ExternalDataCode/simple_live_dispatcher.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#include <stdio.h>
#include <algorithm>
#include <errno.h>
#include <cstring>
#include <string.h>
#include <sys/epoll.h>
#include "dvccode/ExternalData/simple_live_dispatcher.hpp"
#include "dvccode/Utils/runtime_profiler.hpp"
#define MAXEVENTS 1024

namespace HFSAT {

SimpleLiveDispatcher::SimpleLiveDispatcher(int timeout)
    : simple_external_data_live_listener_array_(NULL),
      socket_fd_array_(NULL),
      simple_external_data_live_listener_array_size_(0),
      simple_external_data_live_listener_array_capacity_(16),
      max_socket_file_descriptor_plus1_(0),
      temp_arr_size(0),
      temp_arr_capacity_(16),
      temp_add_delete_info_is_add(NULL),
      temp_simple_external_data_live_listener_array_(NULL),
      temp_socket_fd_array_(NULL),
      primary_socket_listeners_(),
      timeout_(timeout) {
  primary_socket_listeners_.resize(MAXEVENTS, NULL);

  for (int32_t cnt = 0; cnt < MAXEVENTS; cnt++) {
    primary_socket_listeners_.push_back(NULL);
  }

  simple_external_data_live_listener_array_ = (SimpleExternalDataLiveListenerPtr*)calloc(
      simple_external_data_live_listener_array_capacity_, sizeof(SimpleExternalDataLiveListenerPtr));
  socket_fd_array_ = (int*)calloc(simple_external_data_live_listener_array_capacity_, sizeof(int));

  for (auto i = 0u; i < simple_external_data_live_listener_array_capacity_; i++) {
    simple_external_data_live_listener_array_[i] = NULL;
    socket_fd_array_[i] = 0;
  }

  temp_add_delete_info_is_add = (bool*)calloc(temp_arr_capacity_, sizeof(bool));
  temp_simple_external_data_live_listener_array_ =
      (SimpleExternalDataLiveListenerPtr*)calloc(temp_arr_capacity_, sizeof(SimpleExternalDataLiveListenerPtr));
  temp_socket_fd_array_ = (int*)calloc(temp_arr_capacity_, sizeof(int));
}

SimpleLiveDispatcher::~SimpleLiveDispatcher() {
  if (simple_external_data_live_listener_array_ != NULL) {
    free(simple_external_data_live_listener_array_);
    simple_external_data_live_listener_array_ = NULL;
  }
  if (socket_fd_array_ != NULL) {
    free(socket_fd_array_);
    socket_fd_array_ = NULL;
  }
  if (temp_add_delete_info_is_add != NULL) {
    free(temp_add_delete_info_is_add);
    temp_add_delete_info_is_add = NULL;
  }
  if (temp_simple_external_data_live_listener_array_ != NULL) {
    free(temp_simple_external_data_live_listener_array_);
    temp_simple_external_data_live_listener_array_ = NULL;
  }
  if (temp_socket_fd_array_ != NULL) {
    free(temp_socket_fd_array_);
    temp_socket_fd_array_ = NULL;
  }
}

/// In livetrading, takes the socket file descriptiors and selects on them
/// and for the ones that returns true on FD_ISSET
/// calls them to process the data they have.

void SimpleLiveDispatcher::RunLive() {
  int epollfd = -1;
  struct epoll_event event_array[MAXEVENTS];
  struct epoll_event events[MAXEVENTS];

  int es;

  int sec_epollfd = -1;
  struct epoll_event sec_event_array[MAXEVENTS];
  struct epoll_event sec_events[MAXEVENTS];

  // Fixed valgrind error for uninitialized memory location
  for (int counter_ = 0; counter_ < MAXEVENTS; counter_++) {
    memset(&event_array[counter_], 0, sizeof(epoll_event));
    memset(&sec_event_array[counter_], 0, sizeof(epoll_event));
  }

  int sec_es;

  epollfd = epoll_create(MAXEVENTS);
  sec_epollfd = epoll_create(MAXEVENTS);

  if (epollfd == -1 || sec_epollfd == -1) {
    std::cerr << "epoll creation failed\n";
    return;
  }

  FlushTempBuffers();

  for (int32_t primary_fd_counter = 0; primary_fd_counter < MAXEVENTS; primary_fd_counter++) {
    if (NULL == primary_socket_listeners_[primary_fd_counter]) {
      continue;
    }

    event_array[primary_fd_counter].data.fd = primary_fd_counter;

    std::cout << " Adding FD : " << primary_fd_counter << "\n";
    (std::cout).flush();

    // associate Level Triggered epoll event to the fd
    event_array[primary_fd_counter].events = EPOLLIN;

    es = epoll_ctl(epollfd, EPOLL_CTL_ADD, primary_fd_counter, &(event_array[primary_fd_counter]));

    if (es == -1) {
      std::cerr << " Error Adding Epoll Event \n";
      exit(-1);
    }
  }

  for (unsigned int fd_counter = 0; fd_counter < simple_external_data_live_listener_array_size_; fd_counter++) {
    std::cout << " Adding sec FD : " << socket_fd_array_[fd_counter] << "\n";
    (std::cout).flush();

    sec_event_array[fd_counter].data.fd = socket_fd_array_[fd_counter];

    // associate Level Triggered epoll event to the fd
    sec_event_array[fd_counter].events = EPOLLIN;

    sec_es = epoll_ctl(sec_epollfd, EPOLL_CTL_ADD, socket_fd_array_[fd_counter], &(sec_event_array[fd_counter]));

    if (sec_es == -1) {
      std::cerr << " Error Adding Epoll Event \n";
      exit(-1);
    }
  }

  keep_me_running_ = true;

  while (keep_me_running_) {
    int number_of_descriptors_with_data = epoll_wait(epollfd, events, MAXEVENTS, timeout_);  // 0 timeout is essential

    // Primary
    if (number_of_descriptors_with_data > 0) {
      for (unsigned int data_fd_counter = 0; data_fd_counter < (unsigned int)number_of_descriptors_with_data;
           data_fd_counter++) {
        // ProfilerTimeInfo time_info;
        // RuntimeProfiler::GetUniqueInstance(ProfilerType::kCOMBINEDSHMWRITER).Start(time_info);
        // HFSAT::CpucycleProfiler::GetUniqueInstance().Start(4);
        // HFSAT::CpucycleProfiler::GetUniqueInstance().Start(1);
        primary_socket_listeners_[events[data_fd_counter].data.fd]->ProcessAllEvents(events[data_fd_counter].data.fd);
      }
      continue;
    } else if (number_of_descriptors_with_data == -1) {
      std::cerr << " Epoll Returned An Error : " << strerror(errno) << "\n";
      FlushTempBuffers();
    } else {
      // Send dummy packets if timeout was enabled
      if (timeout_ != 0) {
        for (unsigned int listener_counter = 0; listener_counter < MAXEVENTS; listener_counter++) {
          // ProfilerTimeInfo time_info;
          // RuntimeProfiler::GetUniqueInstance(ProfilerType::kCOMBINEDSHMWRITER).Start(time_info);
          if (NULL != primary_socket_listeners_[listener_counter]) {
            primary_socket_listeners_[listener_counter]->ProcessAllEvents(-1);
          }
        }
      }

      // Seconday

      int sec_number_of_descriptors_with_data =
          epoll_wait(sec_epollfd, sec_events, MAXEVENTS, 0);  // 0 timeout is essential

      // Primary
      if (sec_number_of_descriptors_with_data > 0) {
        for (unsigned int sec_data_fd_counter = 0;
             sec_data_fd_counter < (unsigned int)sec_number_of_descriptors_with_data; sec_data_fd_counter++) {
          for (unsigned int sec_listener_counter = 0;
               sec_listener_counter < simple_external_data_live_listener_array_size_; sec_listener_counter++) {
            if (sec_events[sec_data_fd_counter].data.fd == socket_fd_array_[sec_listener_counter]) {
              // ProfilerTimeInfo time_info;
              // RuntimeProfiler::GetUniqueInstance(ProfilerType::kCOMBINEDSHMWRITER).Start(time_info);
              // HFSAT::CpucycleProfiler::GetUniqueInstance().Start(4);
              // HFSAT::CpucycleProfiler::GetUniqueInstance().Start(1);
              simple_external_data_live_listener_array_[sec_listener_counter]->ProcessAllEvents(
                  socket_fd_array_[sec_listener_counter]);
            }
          }
        }

      } else if (sec_number_of_descriptors_with_data == -1) {
        std::cerr << " EPOLL Wait Returned Error : " << strerror(errno) << "\n";
      }

      if (temp_arr_size > 0) {  // Recovery Sockets Are not considered Primary even being local sockets

        FlushTempBuffers();

        for (unsigned int fd_counter = 0; fd_counter < simple_external_data_live_listener_array_size_; fd_counter++) {
          std::cerr << " Adding New Fds \n";
          sec_event_array[fd_counter].data.fd = socket_fd_array_[fd_counter];

          // associate Level Triggered epoll event to the fd
          sec_event_array[fd_counter].events = EPOLLIN;

          sec_es = epoll_ctl(sec_epollfd, EPOLL_CTL_ADD, socket_fd_array_[fd_counter], &(sec_event_array[fd_counter]));

          if (sec_es == -1) {
            if (errno == EEXIST) continue;  // Already Added

            if (errno == ENOENT) {
              std::cerr << "ENOENT error for FD: " << sec_event_array[fd_counter].data.fd
                        << " . This can come for TCP sockets while using VMA or onload" << std::endl;
              continue;
            }  // This can come for TCP sockets

            std::cerr << " Error Adding Epoll Event: " << errno << " ( " << strerror(errno) << " )\n";
            exit(-1);
          }
        }
      }
    }

    if (!shm_sockets_.empty()) {
      for (unsigned int shm_counter_ = 0; shm_counter_ < shm_sockets_.size(); shm_counter_++) {
        shm_listeners_[shm_counter_]->ProcessAllEvents(shm_sockets_[shm_counter_]);
      }
    }
  }
}

/// call to add a < source, sockfd > that dispatcher will need to work with
void SimpleLiveDispatcher::AddSimpleExternalDataLiveListenerSocket(SimpleExternalDataLiveListener* _to_add_,
                                                                   int _new_socket_fd_, bool _is_local_data_socket_) {
  if (_is_local_data_socket_) {  // You Always Assume that we are only looking for static socket from primary data
                                 // source, no dynamic recovery streams etc
    if (_new_socket_fd_ > MAXEVENTS) {
      std::cerr << "Can't Add System Sockets With FD : " << _new_socket_fd_ << " higher than : " << MAXEVENTS
                << std::endl;
      std::exit(-1);
    }
    primary_socket_listeners_[_new_socket_fd_] = _to_add_;
  } else {
    if (temp_arr_size == temp_arr_capacity_) DoubleTempBufferSize();
    temp_add_delete_info_is_add[temp_arr_size] = true;
    temp_simple_external_data_live_listener_array_[temp_arr_size] = _to_add_;
    temp_socket_fd_array_[temp_arr_size++] = _new_socket_fd_;
  }
}

#if USE_SHM_FOR_LIVESOURCES

void SimpleLiveDispatcher::AddSimpleExternalDataLiveListenerShmSocket(SimpleExternalDataLiveListener* _to_add_,
                                                                      int _new_socket_fd_) {
  shm_sockets_.push_back(_new_socket_fd_);
  shm_listeners_.push_back(_to_add_);
}

#endif

void SimpleLiveDispatcher::RemoveSimpleExternalDataLiveListenerSocket(
    HFSAT::SimpleExternalDataLiveListener* _to_remove_, int _new_socket_fd_, bool _is_local_data_socket_) {
  if (_is_local_data_socket_) {  // You Always Assume that we are only looking for static socket from primary data
                                 // source, no dynamic recovery streams etc

    primary_socket_listeners_[_new_socket_fd_] = NULL;

  } else {
    if (temp_arr_size == temp_arr_capacity_) DoubleTempBufferSize();
    temp_add_delete_info_is_add[temp_arr_size] = false;
    temp_simple_external_data_live_listener_array_[temp_arr_size] = _to_remove_;
    temp_socket_fd_array_[temp_arr_size++] = _new_socket_fd_;
  }
}

void SimpleLiveDispatcher::FlushTempBuffers() {
  for (auto i = 0u; i < temp_arr_size; i++) {
    HFSAT::SimpleExternalDataLiveListener* _new_listener_ = temp_simple_external_data_live_listener_array_[i];

    int _new_socket_fd_ = temp_socket_fd_array_[i];

    if ((_new_listener_ == NULL) || (_new_socket_fd_ < 0)) continue;

    unsigned int index = 0;
    for (; index < simple_external_data_live_listener_array_size_; index++) {
      if (simple_external_data_live_listener_array_[index] == _new_listener_ &&
          socket_fd_array_[index] == _new_socket_fd_)
        break;
    }

    if (temp_add_delete_info_is_add[i] == true && index < simple_external_data_live_listener_array_size_)
      continue;  // Nothing to add
    if (temp_add_delete_info_is_add[i] == true) {
      if (simple_external_data_live_listener_array_size_ == simple_external_data_live_listener_array_capacity_) {
        DoubleEDLLSize();
      }
      simple_external_data_live_listener_array_[simple_external_data_live_listener_array_size_] = _new_listener_;
      socket_fd_array_[simple_external_data_live_listener_array_size_] = _new_socket_fd_;

      simple_external_data_live_listener_array_size_++;

      continue;
    }

    if (index < simple_external_data_live_listener_array_size_) {
      // swap with last entry, we don't care if it itself is the last entry, remove a check, possibly more optimal
      simple_external_data_live_listener_array_size_--;
      simple_external_data_live_listener_array_[index] =
          simple_external_data_live_listener_array_[simple_external_data_live_listener_array_size_];
      socket_fd_array_[index] = socket_fd_array_[simple_external_data_live_listener_array_size_];

      continue;
    }
  }
  temp_arr_size = 0;  // reset temporary buffer counter
}

void SimpleLiveDispatcher::DoubleEDLLSize() {
  doubleCapacityOfArray(simple_external_data_live_listener_array_, simple_external_data_live_listener_array_capacity_);
  doubleCapacityOfArray(socket_fd_array_, simple_external_data_live_listener_array_capacity_);
  simple_external_data_live_listener_array_capacity_ *= 2;
}

void SimpleLiveDispatcher::DoubleTempBufferSize() {
  doubleCapacityOfArray(temp_simple_external_data_live_listener_array_, temp_arr_capacity_);
  doubleCapacityOfArray(temp_socket_fd_array_, temp_arr_capacity_);
  doubleCapacityOfArray(temp_add_delete_info_is_add, temp_arr_capacity_);
  temp_arr_capacity_ *= 2;
}

template <class T>
inline void SimpleLiveDispatcher::doubleCapacityOfArray(T*& arr, const unsigned int origCapacity) {
  T* new_arr = (T*)calloc(origCapacity * 2, sizeof(T));
  memcpy(new_arr, arr, origCapacity * sizeof(T));
  free(arr);
  arr = new_arr;
}
}
