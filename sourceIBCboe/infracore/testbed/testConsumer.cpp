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

HFSAT::SharedMemReader<dummyData> smr(1, HFSAT::kExchSourceBMF);
std::vector<double> diffs_;

void sighandler(int signum) {
  printf("cleanup called\n");

  for (size_t i = 0; i < diffs_.size(); i++) {
    std::cout << diffs_[i] << "\n";
  }

  std::cout << "Packets read: " << smr.NumPacketsRead() << "\n";
  std::cout << "Prefetch instances: " << smr.NumPrefetchInstances() << "\n";

  smr.cleanUp();

  printf("cleanup up done...\n");
  exit(0);
}

int main() {
  struct sigaction sigact;
  sigact.sa_handler = sighandler;
  sigaction(SIGINT, &sigact, NULL);

  timespec _now_;
  std::pair<dummyData, int> resp;

  while (true) {
    resp = smr.readT();  // this is a blocking call
    clock_gettime(CLOCK_REALTIME, &_now_);

    diffs_.push_back((double)((_now_.tv_sec - resp.first.ts.tv_sec) * 1000000) +
                     ((double)(_now_.tv_nsec - resp.first.ts.tv_nsec) / 1000));
  }
}
