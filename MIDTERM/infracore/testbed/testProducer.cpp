#include <iostream>

#include <time.h>

#include "dvccode/Utils/shared_mem_writer.hpp"
#include "dvccode/Utils/shared_mem_reader.hpp"

struct dummyData {
  int a;
  int b;
  timespec ts;
  char c[16];

  std::string ToString() { return "asdf\n"; }
};

class ProducerThread : public HFSAT::Thread {
 private:
  HFSAT::SharedMemWriter<dummyData>* writer_;
  int writer_id_;
  int num_packets_to_write_;
  dummyData d1;

 public:
  ProducerThread(int t_num_packets_to_write_) {
    writer_ = new HFSAT::SharedMemWriter<dummyData>(HFSAT::kExchSourceBMF);
    writer_id_ = writer_->intilizeWriter();
    num_packets_to_write_ = t_num_packets_to_write_;
  }

  void thread_main() {
    for (int i = 0; i < num_packets_to_write_; i++) {
      clock_gettime(CLOCK_REALTIME, &d1.ts);
      writer_->writeT(d1);
    }
    std::cout << "Producer " << writer_id_ << " wrote " << num_packets_to_write_ << " packets.\n";
  }
};

int main(int argc, char** argv) {
  if (argc < 3) {
    std::cerr << "invalid args.\n";
    exit(1);
  }

  int num_threads_ = atoi(argv[1]);
  int num_packets_ = atoi(argv[2]);
  std::vector<ProducerThread*> threads_;

  for (int i = 0; i < num_threads_; i++) {
    ProducerThread* producer_ = new ProducerThread(num_packets_);
    threads_.push_back(producer_);
  }

  sleep(1);

  for (int i = 0; i < num_threads_; i++) {
    threads_[i]->run();
  }

  sleep(2);
  //  HFSAT::SharedMemWriter<dummyData> sm ( HFSAT::kExchSourceBMF );
  //  int writerId =  sm.intilizeWriter ( );
  //
  //  if(-1 == writerId)
  //    {
  //      //error
  //      exit ( 0 );
  //    }
  //
  //
  //  dummyData d1;
  //  d1.a = writerId;
  //  d1.b = writerId;
  //  for(int i = 0; i < 15; i++)
  //    d1.c[i] = 'a' + (i + writerId)%26;
  //  d1.c[15] ='\0';
  //
  //  for ( int i = 0 ; i < atoi ( argv [ 1 ] ); i++ )
  //    {
  //      clock_gettime ( CLOCK_REALTIME, &d1.ts );
  //      sm.writeT(d1);
  //    }
  //
  //  printf("data written\n");

  return 0;
}
