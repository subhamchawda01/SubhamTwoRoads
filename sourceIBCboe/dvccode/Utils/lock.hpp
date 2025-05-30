/**
    \file dvccode/Utils/lock.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/

#ifndef BASE_UTILS_LOCK_H
#define BASE_UTILS_LOCK_H

#include <pthread.h>
#include <stdio.h>
#include <string.h>

namespace HFSAT {

/// Attempt to make a class to synchronize
/// two objects of subclass of class Thread
/// By current implementation, the object of class Lock has toi be created externally and
/// passed as an argument by reference to the class that will be
/// using it
class Lock {
 public:
  Lock() { pthread_mutex_init(&mutex_object_, NULL); }

  ~Lock() { pthread_mutex_destroy(&mutex_object_); }

  inline void LockMutex() { pthread_mutex_lock(&mutex_object_); }

  inline void UnlockMutex() { pthread_mutex_unlock(&mutex_object_); }

 private:
  pthread_mutex_t mutex_object_;
};
}

#endif  // BASE_UTILS_LOCK_H
