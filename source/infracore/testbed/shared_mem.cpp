#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>

#define STACK_LIMIT 5  // maximum element size for stack.
#define NUM_ITER 100   // # of cycle for processes.

#define SHM_KEY ((key_t)0x103)  // key for shared memory
#define SEM_KEY ((key_t)0x203)  // key for semaphore

union semun {
  int val;
  struct semid_ds* buf;
  unsigned short* array;
};

void set_sem_value(int semid, int i, int val) {
  union semun initval;
  initval.val = val;
  semctl(semid, i, SETVAL, initval);
}

struct sembuf op_lock[2] = {
    0, 0, 0,        // wait until sem #0 becomes 0
    0, 1, SEM_UNDO  // then increment sem #0 by 1
};

struct sembuf op_unlock[1] = {
    0, -1, SEM_UNDO  // decr sem #0 by 1
};

typedef struct {
  int top;
  int home[STACK_LIMIT];
} Stack;

// necessary function declarions.
int isFull(Stack*);
int isEmpty(Stack*);
int push(Stack*, int);
int pop(Stack*);
int generate_number(int, int);

void wait_and_lock(int semid) { semop(semid, op_lock, 2); }
void signal_and_unlock(int semid) { semop(semid, op_unlock, 1); }

static __inline__ unsigned long GetCpucycleCount(void) {
  unsigned a, d;
  asm volatile("rdtsc" : "=a"(a), "=d"(d));
  return ((unsigned long)a) | (((unsigned long)d) << 32);
}

int main() {
  Stack* sp;
  int shmid, semid, pid;

  // cleanup
  shmget(shmid, 0, 0);
  if (shmid != -1) shmctl(shmid, IPC_RMID, 0);  // remove the shared memory area
  semget(semid, 0, 0);
  if (semid != -1) semctl(semid, IPC_RMID, 0);  // remove the semaphore.

  unsigned long start, end;
  shmid = shmget(SHM_KEY, sizeof(Stack), IPC_CREAT | 0600);
  sp = (Stack*)shmat(shmid, 0, 0);
  sp->top = -1;

  semid = semget(SEM_KEY, 1, 07777 | IPC_CREAT);
  set_sem_value(semid, 0, 0);  // set initial value of semaphore

  pid = fork();  // create producer process
  if (pid == 0) {
    // producer
    // printf("\nProducer Started");

    unsigned long a[100];
    for (int i = 0; i < NUM_ITER; i++) {
      while (isFull(sp)) {
      }

      start = GetCpucycleCount();
      wait_and_lock(semid);

      {  // critical region
        int produced = generate_number(11, 41);
        // printf("\nProduced: %d",produced);
        push(sp, produced);
      }
      signal_and_unlock(semid);
      end = GetCpucycleCount();
      a[i] = end - start;

      usleep(1);
    }

    // printf("\nProducer Exited %d\n", ((end-start)/NUM_ITER));
    sleep(1);
    for (int j = 0; j < 100; j++) {
      printf("proTime: %ld\n", a[j]);
    }
  } else {
    pid = fork();  // create consumer process
    if (pid == 0) {
      unsigned long a[100];

      // printf("\nConsumer Started");
      for (int i = 0; i < NUM_ITER; i++) {
        while (isEmpty(sp)) {
        }

        start = GetCpucycleCount();
        wait_and_lock(semid);

        {  // critical region
          pop(sp);
          // printf("\nConsumed: %d", pop(sp));
        }

        signal_and_unlock(semid);
        end = GetCpucycleCount();
        a[i] = end - start;

        usleep(1);
      }

      // printf("\nConsumer Exited %d\n", ((end-start)/NUM_ITER));
      sleep(3);
      for (int j = 0; j < 100; j++) {
        printf("conTime: %ld\n", a[j]);
      }
    } else { /*
            printf("\nBuffer shower Started");

            for(int i=0; i<NUM_ITER; i++ )
              { //this part of the code, shows contents of stack
                if( !isEmpty(sp) ) //if there is any info in stack, show.
                  {
                    wait_and_lock( semid );
                    printf("\nContents of Buffer: [");
                    for(int index =0; index<= sp->top; index++ )
                      printf(" %d",sp->home[ index ]);
                    printf(" ]");
                    signal_and_unlock( semid );

                  }
                //sleep( generate_number(1,2) );
              }
            //printf("\nBuffer shower Exited\n");
        */
    }
    wait();
  }
  wait();
  sleep(1);
  shmdt(sp);                   // detach the shared memory area.
  shmctl(shmid, IPC_RMID, 0);  // remove the shared memory area
  semctl(semid, IPC_RMID, 0);  // remove the semaphore.
}

inline int isFull(Stack* sp) {
  if (sp->top == STACK_LIMIT - 1) return 1;
  return 0;
}
inline int isEmpty(Stack* sp) {
  if (sp->top == -1) return 1;
  return 0;
}
inline int push(Stack* sp, int value) {
  if (isFull(sp)) return -1;
  sp->home[++sp->top] = value;
  return 0;
}
inline int pop(Stack* sp) {
  if (isEmpty(sp)) return -1;
  return sp->home[sp->top--];
}

inline int generate_number(int lower, int upper) { return (rand() % (upper - lower)) + lower; }
