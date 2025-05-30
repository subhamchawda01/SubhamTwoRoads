#include <iostream>

#include <signal.h>
#include <time.h>

#include "dvccode/Utils/shared_mem_writer.hpp"
#include "dvccode/Utils/shared_mem_reader.hpp"

struct dummyData {
  int a;
  int b;
  timespec ts;
  char c[16];

  void print() { std::cout << a << " " << b << " " << c << "\n"; }
};

HFSAT::SharedMemReader<dummyData> smr(1, HFSAT::kExchSourceBMFEQ);
std::vector<double> diffs_;

void sighandler(int signum) {
  printf("cleanup called\n");
  std::cout<<"No of products Build " << diffs_.size() << std::endl;
//  for (size_t i = 0; i < diffs_.size(); i++) {
//    std::cout << diffs_[i] << "\n";
//  }

  smr.cleanUp();

  printf("cleanup up done...\n");
  exit(0);
}

int main() {
  struct sigaction sigact;
  sigact.sa_handler = sighandler;
  sigaction(SIGINT, &sigact, NULL);
  
  timespec _now;
  HFSAT:: DataWriterIdPair<dummyData> resp;
  while (true) {
    bool success = smr.readTNonBlocking(resp);  // this is a blocking call
    if (success == true) {
	 clock_gettime(CLOCK_REALTIME, &_now);

    diffs_.push_back((double)((_now.tv_sec - resp.data.ts.tv_sec) * 1000000) +
                     ((double)(_now.tv_nsec - resp.data.ts.tv_nsec) / 1000));

  }
  }
  return 0;
}

