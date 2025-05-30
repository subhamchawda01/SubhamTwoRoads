/**
    \file dvccode/Utils/scope_lock.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/

#ifndef BASE_UTILS_SCOPELOCK_H
#define BASE_UTILS_SCOPELOCK_H

#include <pthread.h>
#include <stdio.h>
#include <string.h>

namespace HFSAT {

/// Attempt to make a class to synchronize
/// two objects of subclass of class Thread
class ScopeLock {
 public:
  ScopeLock(pthread_mutex_t& mutex_object_) {
    pthread_mutex_init(&mutex_object_, NULL);
    pthread_mutex_lock(&mutex_object_);
  }

  ~ScopeLock() {
    pthread_mutex_unlock(&mutex_object_);
    pthread_mutex_destroy(&mutex_object_);
  }

 private:
};
}

#endif  // BASE_UTILS_SCOPELOCK_H
