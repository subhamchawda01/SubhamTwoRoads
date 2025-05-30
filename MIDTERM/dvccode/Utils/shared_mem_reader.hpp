/**
    \file dvccode/Utils/shared_mem_reader.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 162, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551

 */

#ifndef BASE_UTILS_SHARED_MEM_READER_H_
#define BASE_UTILS_SHARED_MEM_READER_H_

#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <pthread.h>
#include <signal.h>
#include <map>
#include <string>
#include <iostream>
#include <sys/fcntl.h>
#include <vector>

#include "dvccode/Utils/saci_generator.hpp"
#include "dvccode/Utils/shared_mem_writer.hpp"
#include "dvccode/Utils/shm_queue.hpp"
#include "dvccode/Utils/thread.hpp"

#include "dvccode/CommonDataStructures/unordered_vector.hpp"

namespace HFSAT {

template <class T>
class SharedMemReader {
  class AssignWriterIdThread : public Thread {
   public:
    bool keepAlive;
    SharedMemReader* callback_handler;
    int base_writer_id_;

    inline void notifyAdd(int writer_id, int32_t const& writer_pid) {
      for (auto i = 0u; i < callback_handler->writer_add_remove_listner_vector.size(); i++) {
        callback_handler->writer_add_remove_listner_vector[i]->AddWriter(writer_id, writer_pid);
      }
    }

    void thread_main(void) {
      SemUtils::wait_and_lock(callback_handler->sem_pc_id);
      volatile HFSAT::PidWriterId* p;
      p = (volatile PidWriterId*)shmat(callback_handler->shm_pc_id, 0, 0);
      p->pid = -1;
      p->writer_id = -2;  // allow writers to request for an id
      SemUtils::signal_and_unlock(callback_handler->sem_pc_id);

      int server_assigned_client_id_mantissa_ = 1;

      int thread_id_start = 0;

      while (keepAlive) {
        SemUtils::wait_and_lock(callback_handler->sem_pc_id);
        /*critical section starts*/
        if (p->writer_id == -1 && server_assigned_client_id_mantissa_ < ORS_MAX_NUM_OF_CLIENTS) {
          server_assigned_client_id_mantissa_ =
              HFSAT::SaciIncrementGenerator::GetUniqueInstance().GetNextSaciIncrementValue();
          p->writer_id = base_writer_id_ + server_assigned_client_id_mantissa_;
          p->thread_id = thread_id_start++;
          std::pair<int, int> pr;
          pr.first = p->pid;
          pr.second = p->writer_id;
          callback_handler->pid_writer_id_vector.push_back(pr);  // associate in map the pid to the writer id
          notifyAdd(p->writer_id, pr.first);
          HFSAT::SaciIncrementGenerator::GetUniqueInstance().PersistSaciIncrementGenerator();

          // std::cerr<<"new writer found, assigned id: " <<p->writer_id <<"\n";
          // std::cerr<<" number of listners " << callback_handler->writer_add_remove_listner_vector.size()<<"\n";
        }
        SemUtils::signal_and_unlock(callback_handler->sem_pc_id);
        usleep(100);  // no need to spin, not a priority thread
      }
      // std::cout<<"exiting AssignWriterIdThread::thread_main\n";
    }
  };

  class DetectDeadWritersThread : public Thread {
    inline static bool isAlivePID(int pid) {
      // PID is not alive only if kill returns -1 && errno is ESRCH.
      // We get errno = EPERM when running the SHM client as a different user leading to errors - S.
      if (kill(pid, 0) == -1 && errno == ESRCH) {
        return false;
      }

      return true;
    }

   public:
    bool keepAlive;
    SharedMemReader* callback_handler;

    inline void notifyRemove(int writer_id, int32_t const& writer_pid) {
      for (auto i = 0u; i < callback_handler->writer_add_remove_listner_vector.size(); i++)
        callback_handler->writer_add_remove_listner_vector[i]->RemoveWriter(writer_id, writer_pid);
    }

    void thread_main(void) {
      while (keepAlive) {
        SemUtils::wait_and_lock(callback_handler->sem_pc_id);  // can be optimized as we can wait on a different
                                                               // semaphore, but since this is a low priority task, we
                                                               // probably don't care
        UnOrderedVec<std::pair<int, int> >& vec = callback_handler->pid_writer_id_vector;
        for (auto i = 0u; i < vec.size(); i++) {
          // using PID is not a concern as we are checking quite frequently and the possibility that a different
          // process has come up with the same processId is quite rare
          if (!isAlivePID(vec.at(i).first)) {
            // printf ("PID %d | writer id %d not alive, calling clean up methods\n", vec.at ( i ).first, vec.at ( i
            // ).second );
            notifyRemove(vec.at(i).second, vec.at(i).first);
            vec.remove_and_delete(i);
          }
        }
        SemUtils::signal_and_unlock(callback_handler->sem_pc_id);
        sleep(1);  // no need to spin, not a priority thread. sleeping for 1 second
      }
      // std::cout<<"exiting DetectDeadWritersThread::thread_main\n";
    }
  };

  friend class DetectDeadWritersThread;
  friend class AssignWriterIdThread;

 protected:
  int shmid;
  int semid;
  int sem_pc_id;
  int shm_pc_id;

  UnOrderedVec<std::pair<int, int> > pid_writer_id_vector;
  ShmQueue<DataWriterIdPair<T> >* dataQ;
  std::vector<ShmWriterAddRemoveListner*> writer_add_remove_listner_vector;

  AssignWriterIdThread assign_writer_id_thread;
  DetectDeadWritersThread detect_dead_writers_thread;
  bool keepReaderRunning;

  HFSAT::ExchSource_t exchange_;

  int ORS_SHM_KEY;
  int ORS_SHM_KEY_ID_GEN;
  int ORS_SEM_KEY;
  int ORS_SEM_KEY_ID_GEN;

  DataWriterIdPair<T>* data_arr_;
  int data_index_;
  int num_packets_left_;
  int num_packets_read_;
  int num_prefetch_instances_;

  void clearAllMem() {
    if (dataQ != NULL) shmdt((const void*)dataQ);  // detach the shared memory area.
    if (shmid != -1) {
      // printf ( "...removed shmid\n" );
      shmctl(shmid, IPC_RMID, 0);
    }  // remove the shared memory area
    if ((semid = semget(ORS_SEM_KEY, 1, 0)) != -1) {
      // printf ( "...removed semid\n" );
      semctl(semid, IPC_RMID, 0);
    }  // remove the semaphore.
    if ((shm_pc_id = shmget(ORS_SHM_KEY_ID_GEN, 0, 0)) != -1) {
      // printf ( "...removed shm_pc_id\n" );
      shmctl(shm_pc_id, IPC_RMID, 0);
    }  // remove the shared memory area
    if ((sem_pc_id = semget(ORS_SEM_KEY_ID_GEN, 1, 0)) != -1) {
      // printf ( "...removed sem_pc_id\n" );
      semctl(sem_pc_id, IPC_RMID, 0);
    }  // remove the semaphore.
  }

  void LoadSHMKeys(const HFSAT::ExchSource_t& _exchange_) {
    std::string _this_exchange_ = HFSAT::ExchSourceStringForm(_exchange_);

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
      const std::vector<const char*>& tokens_ = st_.GetTokens();

      if (tokens_.size() > 0) {
        std::cout << " Exchange : " << tokens_[0] << " Keys : " << tokens_[1] << " " << tokens_[2] << " " << tokens_[3]
                  << " " << tokens_[4] << " " << tokens_[5] << "\n";

        std::string exch = tokens_[0];

        if (exch != _this_exchange_) continue;

        ORS_SHM_KEY = atoi(tokens_[1]);
        ORS_SHM_KEY_ID_GEN = atoi(tokens_[2]);
        ORS_SEM_KEY = atoi(tokens_[3]);
        ORS_SEM_KEY_ID_GEN = atoi(tokens_[4]);
      }
    }

    ors_shm_key_config_file_.close();
  }

 public:
  SharedMemReader<T>(int base_writer_id, const HFSAT::ExchSource_t& _exchange_)
      : pid_writer_id_vector(),
        dataQ(NULL),
        writer_add_remove_listner_vector(),
        keepReaderRunning(true),
        data_index_(0),
        num_packets_left_(0),
        num_packets_read_(0),
        num_prefetch_instances_(0) {
    data_arr_ = (DataWriterIdPair<T>*)malloc(sizeof(DataWriterIdPair<T>) * 100);

    ORS_SHM_KEY = SHM_KEY;
    ORS_SHM_KEY_ID_GEN = SHM_KEY_ID_GEN;
    ORS_SEM_KEY = SEM_KEY;
    ORS_SEM_KEY_ID_GEN = SEM_KEY_ID_GEN;

    if (_exchange_ == HFSAT::kExchSourceRTS || _exchange_ == HFSAT::kExchSourceMICEX ||
        _exchange_ == HFSAT::kExchSourceMICEX_CR || _exchange_ == HFSAT::kExchSourceMICEX_EQ ||
        HFSAT::kExchSourceBMFEQ == _exchange_ || HFSAT::kExchSourceNSE_FO == _exchange_ ||
        HFSAT::kExchSourceNSE_CD == _exchange_ || HFSAT::kExchSourceNSE_EQ == _exchange_) {
      LoadSHMKeys(_exchange_);
    }

    std::cout << "Loaded ORS SHM KEY : " << ORS_SHM_KEY << " " << ORS_SHM_KEY_ID_GEN << " " << ORS_SEM_KEY << " "
              << ORS_SEM_KEY_ID_GEN << "\n";

    shmid = shmget(ORS_SHM_KEY, 0, 0);

    if (shmid != -1) {
      // std::cerr<< "shared memory already created. Probably another instance of reader is running. Will free and
      // recreate new memory space\n";
      clearAllMem();
    }

    shmid = shmget(ORS_SHM_KEY, sizeof(ShmQueue<DataWriterIdPair<T> >), IPC_CREAT | 0666);  // create shared memory area
    if (shmid == -1) {
      fprintf(stderr, "SHM creation error shmget returned -1 for %d : %s \n", ORS_SHM_KEY, strerror(errno));
      exit(-1);
    }
    std::cout << "REQUEST SharedMemReader::ORSSHMID: " << shmid << " KEY: " << ORS_SHM_KEY << std::endl;
    semid = semget(ORS_SEM_KEY, 1, 07777 | IPC_CREAT);
    if (semid == -1) {
      perror("SEM creation error\n");
    }
    // semctl(semid, 0 , )
    shm_pc_id = shmget(ORS_SHM_KEY_ID_GEN, sizeof(PidWriterId), IPC_CREAT | 0666);
    sem_pc_id = semget(ORS_SEM_KEY_ID_GEN, 1, 07777 | IPC_CREAT);

    dataQ = (ShmQueue<DataWriterIdPair<T> >*)shmat(shmid, 0, 0);
    if (dataQ == (void*)-1) {
      fprintf(stderr, "shmat error for shmid %d : %s\n", shmid, strerror(errno));
      exit(-1);
    }

    dataQ->reset();

    assign_writer_id_thread.base_writer_id_ = base_writer_id;
    assign_writer_id_thread.callback_handler = this;
    assign_writer_id_thread.keepAlive = true;
    assign_writer_id_thread.run();
    detect_dead_writers_thread.callback_handler = this;
    detect_dead_writers_thread.keepAlive = true;
    detect_dead_writers_thread.run();
  }

  // TODO: we can directly return pointer of shared memory as it won't be overwritten unless we call readT() again.
  void readT(DataWriterIdPair<T>& dest) { dataQ->pop(dest); }

  void interruptRead() { keepReaderRunning = false; }

  void cleanUp() {
    // reader ends
    assign_writer_id_thread.keepAlive = false;
    detect_dead_writers_thread.keepAlive = false;

    assign_writer_id_thread.stop();
    detect_dead_writers_thread.stop();

    clearAllMem();
  }

  void addShmWriterAddRemoveListner(ShmWriterAddRemoveListner* listner_) {
    writer_add_remove_listner_vector.push_back(listner_);
  }
};
}

#endif /*BASE_UTILS_SHARED_MEM_READER_H_*/
