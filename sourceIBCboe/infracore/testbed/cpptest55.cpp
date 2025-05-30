#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <errno.h>
#include <ctime>

#define NUM_CLIENTS 4
#define LOOPS 10000000

using namespace std;

int random_int_ = 0;

#ifdef USE_SPINLOCK
pthread_spinlock_t spinlock;
#else
pthread_mutex_t mutex;
#endif

pid_t gettid() { return syscall(__NR_gettid); }

void *consumer(void *ptr) {
  for (unsigned int i = 0; i < LOOPS; i++) {
#ifdef USE_SPINLOCK
    pthread_spin_lock(&spinlock);
#else
    pthread_mutex_lock(&mutex);
#endif

    random_int_++;

#ifdef USE_SPINLOCK
    pthread_spin_unlock(&spinlock);
#else
    pthread_mutex_unlock(&mutex);
#endif
  }

  return NULL;
}

int main() {
  pthread_t thr[NUM_CLIENTS];
  struct timeval tv1, tv2;

#ifdef USE_SPINLOCK
  pthread_spin_init(&spinlock, 0);
#else
  pthread_mutex_init(&mutex, NULL);
#endif

  // Measuring time before starting the threads...
  gettimeofday(&tv1, NULL);

  for (unsigned int i = 0; i < NUM_CLIENTS; i++) pthread_create(&(thr[i]), NULL, consumer, NULL);

  for (unsigned int i = 0; i < NUM_CLIENTS; i++) pthread_join(thr[i], NULL);

  // Measuring time after threads finished...
  gettimeofday(&tv2, NULL);

  if (tv1.tv_usec > tv2.tv_usec) {
    tv2.tv_sec--;
    tv2.tv_usec += 1000000;
  }

  double total_usec = (((tv2.tv_sec - tv1.tv_sec) * 1000000) + (tv2.tv_usec - tv1.tv_usec));
  double unit_nsec = total_usec / (LOOPS / 1000);

  printf("Total %.1f seconds, One lock+unlock %.1f nanoseconds\n", (total_usec / 1000000), unit_nsec / NUM_CLIENTS);

#ifdef USE_SPINLOCK
  pthread_spin_destroy(&spinlock);
#else
  pthread_mutex_destroy(&mutex);
#endif

  printf("Global random_int_ = %d PLZ IGNORE\n", random_int_);

  return 0;
}
