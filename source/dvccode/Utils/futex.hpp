
#pragma once

#include <unistd.h>
#include <limits.h>
#include <sys/syscall.h>
#include <linux/futex.h>
#include <inttypes.h>
#define cpu_relax() asm volatile("pause\n" : : : "memory")
typedef unsigned int futex;
namespace HFSAT {

class Futex {
 private:
  futex ftx;

 public:
  Futex() : ftx(0) {}

  int sys_futex(void *addr1, int op, int val1, struct timespec *timeout, void *addr2, int val3) {
    return syscall(SYS_futex, addr1, op, val1, timeout, addr2, val3);
  }

  // designed to avoid the sys_call() as much as possible
  int mutex_lock() {
    unsigned int i;
    /* Try to grab lock */
    for (i = 0; i < 100; i++) {
      if ((__sync_fetch_and_or(&ftx, 1U) & 1U) == 0) return 0;
      cpu_relax();
    }
    /* Have to sleep */
    while ((__sync_lock_test_and_set(&ftx, 257U) & 1U) == 1U) sys_futex(&ftx, FUTEX_WAIT_PRIVATE, 257, NULL, NULL, 0);
    return 0;
  }
  // designed to avoid sys_call() as much as possible
  int mutex_unlock() {
    unsigned int i;
    /* Locked and not contended */
    if (ftx == 1)
      if (__sync_val_compare_and_swap(&ftx, 1U, 0) == 1U) return 0;
    /* Unlock */
    __sync_fetch_and_and(&ftx, ~1U);
    __sync_synchronize();

    /* Spin and hope someone takes the lock */
    for (i = 0; i < 200; i++) {
      if ((ftx & 1U) == 1U) return 0;

      cpu_relax();
    }
    /* We need to wake someone up */
    __sync_fetch_and_and(&ftx, ~256U);
    sys_futex(&ftx, FUTEX_WAKE_PRIVATE, 1, NULL, NULL, 0);
    return 0;
  }
};
}
