/**
    \file dvccode/Utils/shared_mem_writer.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551

 */

#ifndef BASE_UTILS_SHARED_MEM_WRITER_H_
#define BASE_UTILS_SHARED_MEM_WRITER_H_

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
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>

#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/Utils/shm_queue.hpp"
#include "dvccode/Utils/sem_utils.hpp"
#include "dvccode/CDef/defines.hpp"

namespace HFSAT {

#define SHM_KEY ((key_t)0x10)         // key for shared memory
#define SHM_KEY_ID_GEN ((key_t)0x30)  // key for producer id generation
#define SEM_KEY ((key_t)0x20)         // key for semaphore
#define SEM_KEY_ID_GEN ((key_t)0x40)  // key for semaphore

struct PidWriterId {
  int pid;
  int writer_id;
  int thread_id;
  // For possible future use
  int flag_1;
  char buffer[32];
};

class ShmWriterAddRemoveListner {
 public:
  ShmWriterAddRemoveListner() {}

  virtual ~ShmWriterAddRemoveListner() {}
  virtual void AddWriter(int writer_id, int32_t const &writer_pid) = 0;
  virtual void RemoveWriter(int writer_id, int32_t const &writer_pid) = 0;
};

template <class T>
class DataWriterIdPair {
 public:
  int writer_id;
  T data;
};

template <class T>
class SharedMemWriter {
 protected:
  int shmid;
  int semid;
  int sem_pc_id;
  int shm_pc_id;
  int writer_id;
  int thread_id;

  HFSAT::ExchSource_t exchange_;

  int ORS_SHM_KEY;
  int ORS_SHM_KEY_ID_GEN;
  int ORS_SEM_KEY;
  int ORS_SEM_KEY_ID_GEN;

  ShmQueue<DataWriterIdPair<T> > *data_queue;

  void setWriterId() {
    bool isIdRequested = false;
    volatile PidWriterId *p = (volatile PidWriterId *)shmat(shm_pc_id, 0, 0);

    int numTries = 50000;  // some arbitrary value; typically combined with usleep(10) inside the loop this
    int micro_sec_sleep_time =
        10;  // means that the producer will try to acquire an id for numTries * microSecSleepTime after which it fails
    while (numTries--) {
      SemUtils::wait_and_lock(sem_pc_id);

      if (isIdRequested) {
        if (p->writer_id != -1) {
          writer_id = p->writer_id;
          thread_id = p->thread_id;
          p->writer_id = -2;  // other producers can now write their requests
          SemUtils::signal_and_unlock(sem_pc_id);
          shmdt((const void *)p);  // necessary! detach memory
          return;
        }
      } else {
        if (p->writer_id == -2)  // write the id if and only if this field is set to -2
        {
          p->pid = getpid();
          p->writer_id =
              -1;  // other producers will not write on this. consumer will now that it has to assign a new value
          isIdRequested = true;
          // std::cerr<<"id requested\n";
        }
      }
      SemUtils::signal_and_unlock(sem_pc_id);
      usleep(micro_sec_sleep_time);
    }
    // not able to initialize writer_id.. detach it (cleanup code)
    SemUtils::wait_and_lock(sem_pc_id);
    p->pid = -1;
    p->writer_id = -2;
    SemUtils::signal_and_unlock(sem_pc_id);
    shmdt((const void *)p);
  }

  void LoadSHMKeys(const HFSAT::ExchSource_t &_exchange_) {
    std::string _this_exchange_ = HFSAT::ExchSourceStringForm(_exchange_);
    std::cout << "LoadSHMKeys" << _this_exchange_ << std::endl;
    std::ifstream ors_shm_key_config_file_;

    ors_shm_key_config_file_.open(ORS_SHM_KEY_CONFIG_FILE);

    if (!ors_shm_key_config_file_.is_open()) {
      std::cerr << "Fatal Error, Can't Read ORS SHM KEY FILE : " << ORS_SHM_KEY_CONFIG_FILE
                << " Can't Continue Exiting \n";
      exit(1);
    }

    char line_buffer_[1024];
    std::string line_read_;

    while (ors_shm_key_config_file_.good()) {
      memset(line_buffer_, 0, sizeof(line_buffer_));

      ors_shm_key_config_file_.getline(line_buffer_, sizeof(line_buffer_));

      line_read_ = line_buffer_;

      if (line_read_.find("#") != std::string::npos) {
        continue;  // comments
      }

      HFSAT::PerishableStringTokenizer st_(line_buffer_, sizeof(line_buffer_));
      const std::vector<const char *> &tokens_ = st_.GetTokens();

      if (tokens_.size() > 0) {
        std::string exch = tokens_[0];
        if (exch != _this_exchange_) continue;

        ORS_SHM_KEY = atoi(tokens_[1]);
        ORS_SHM_KEY_ID_GEN = atoi(tokens_[2]);
        ORS_SEM_KEY = atoi(tokens_[3]);
        ORS_SEM_KEY_ID_GEN = atoi(tokens_[4]);
      }
    }

    ors_shm_key_config_file_.close();
    std::cout << "LoadSHMKeys Completed" << std::endl;
  }

 public:
  SharedMemWriter(const HFSAT::ExchSource_t &_exchange_) {
    ORS_SHM_KEY = SHM_KEY;
    ORS_SHM_KEY_ID_GEN = SHM_KEY_ID_GEN;
    ORS_SEM_KEY = SEM_KEY;
    ORS_SEM_KEY_ID_GEN = SEM_KEY_ID_GEN;

    if (_exchange_ == HFSAT::kExchSourceRTS || _exchange_ == HFSAT::kExchSourceMICEX ||
        _exchange_ == HFSAT::kExchSourceMICEX_CR || _exchange_ == HFSAT::kExchSourceMICEX_EQ ||
        HFSAT::kExchSourceBMFEQ == _exchange_ || HFSAT::kExchSourceNSE_FO == _exchange_ ||
        HFSAT::kExchSourceNSE_CD == _exchange_ || HFSAT::kExchSourceNSE_EQ == _exchange_ || HFSAT::kExchSourceNSE_IDX == _exchange_ ) {
      LoadSHMKeys(_exchange_);
    }

    shmid = shmget(ORS_SHM_KEY, sizeof(ShmQueue<DataWriterIdPair<T> >), 0);
    semid = semget(ORS_SEM_KEY, 1, 0);  // semaphore
    shm_pc_id = shmget(ORS_SHM_KEY_ID_GEN, sizeof(PidWriterId), 0);
    sem_pc_id = semget(ORS_SEM_KEY_ID_GEN, 1, 0);  // semaphore

    writer_id = -1;
    data_queue = NULL;
  }

  virtual ~SharedMemWriter() {
    if (data_queue != NULL) {
      // std::cerr<<"shared mem region detached\n";
      shmdt((const void *)data_queue);
      data_queue = NULL;
    }
  }

  void writeT(DataWriterIdPair<T> &data) {
    data_queue->push(data);  // necessary to check success if we have more than 1 writers
  }

  /**
   * return -1 if can't initialize writer or should we exit(ERR_CODE) ?
   */
  int intilizeWriter() {
    if (semid == -1 || shmid == -1 || sem_pc_id == -1 || shm_pc_id == -1) {
      std::cerr << "Can't initialize, shared memory are not allocated. Start Reader first...\n";
      return -1;
    }

    setWriterId();
    if (writer_id != -1) {
      data_queue = (ShmQueue<DataWriterIdPair<T> > *)shmat(shmid, 0, 0);
      HFSAT::set_thr_id(thread_id);
      data_queue->SetNumClients(thread_id);
      return writer_id;
    }
    std::cerr << "can't assign writerId\n";
    return -1;
  }
};
}
#endif /* BASE_UTILS_SHARED_MEM_WRITER_H_ */
