/**
    \file dvccode/Utils/sem_utils.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 162, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551

*/

#ifndef BASE_UTILS_SEM_UTILS_H_
#define BASE_UTILS_SEM_UTILS_H_

#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <pthread.h>
#include <string>
#include <iostream>
#include <errno.h>
#include <xmmintrin.h>

namespace HFSAT {

class SemUtils {
 public:
  union semun {
    int val;                    //<= value for SETVAL
    struct semid_ds *buf;       //<= buffer for IPC_STAT & IPC_SET
    unsigned short int *array;  //<= array for GETALL & SETALL
    struct seminfo *__buf;      //<= buffer for IPC_INFO
  };

  static void set_sem_value(int semid, int i, int val) {
    union semun initval;
    initval.val = val;
    if (-1 == semctl(semid, i, SETVAL, initval)) {
      perror("error in semctl SETVAL ");
      exit(1);
    }
  }

  static ipc_perm get_sem_perms(int semid, int sem_num) {
    union semun val;
    if (-1 == semctl(semid, sem_num, IPC_STAT, val)) {
      perror("error in semctl IPC_STAT");
      exit(1);
    } else {
      return val.buf->sem_perm;
    }
  }

  /**
   * for sem_num = 0
   */
  static void wait_and_lock(int semid) {
    struct sembuf op_lock[2] = {
        {0, 0, 0},        // wait until sem #0 becomes 0
        {0, 1, SEM_UNDO}  // then increment sem #0 by 1
    };

    while (-1 == semop(semid, op_lock, 2))
      ;
  }

  static void timed_wait_and_lock(int semid, struct timespec *timeout) {
    struct sembuf op_lock[2] = {
        {0, 0, 0},        // wait until sem #0 becomes 0
        {0, 1, SEM_UNDO}  // then increment sem #0 by 1
    };

    int retVal = semtimedop(semid, op_lock, 2, timeout);
    if (retVal == -1) {
      semctl(semid, 0, IPC_RMID);
      perror("IPC error: semop");
      exit(1);
    }
  }

  /**
   * for sem_num = 0
   */
  static void signal_and_unlock(int semid) {
    struct sembuf op_unlock[1] = {{0, -1, SEM_UNDO}};  // decr sem #0 by 1

    int retVal = semop(semid, op_unlock, 1);
    if (retVal == -1) {
      semctl(semid, 0, IPC_RMID);
      perror("IPC error: semop");
      exit(1);
    }
  }

  static void spin_lock(int volatile *p) {
    while (!__sync_bool_compare_and_swap(p, 0, 1)) {
      while (*p) _mm_pause();
    }
  }

  static void spin_unlock(int volatile *p) {
    asm volatile("");  // acts as a memory barrier.
    *p = 0;
  }
};
}
#endif /* BASE_UTILS_SEM_UTILS_H_ */
