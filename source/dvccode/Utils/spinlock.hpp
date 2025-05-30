/**
    \file dvccode/Utils/spinlock.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/

#ifndef BASE_UTILS_SPIN_LOCK_H
#define BASE_UTILS_SPIN_LOCK_H

#include <stdio.h>
#include <string.h>
#include <pthread.h>

namespace HFSAT {

/// Attempt to make a class to synchronize
/// two objects of subclass of class Thread
/// By current implementation, the object of class Lock has toi be created externally and
/// passed as an argument by reference to the class that will be
/// using it
class SpinLock {
 public:
  SpinLock() { pthread_spin_init(&spinlock_object_, PTHREAD_PROCESS_SHARED); }

  ~SpinLock() { pthread_spin_destroy(&spinlock_object_); }

  inline void LockMutex() { pthread_spin_lock(&spinlock_object_); }

  inline void UnlockMutex() { pthread_spin_unlock(&spinlock_object_); }

 private:
  pthread_spinlock_t spinlock_object_;
};
}

#endif  // BASE_UTILS_SPIN_LOCK_H
