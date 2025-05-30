#include <stdio.h>
#include <string.h>
#include <vector>
#include <map>
#include <iomanip>
#include <inttypes.h>
#include <iostream>
#include <time.h>
#include <ratio>
#include <chrono>

#include "dvccode/Utils/shared_mem_writer.hpp"
#include "dvccode/Utils/shared_mem_reader.hpp"

typedef unsigned long cyclecount_t;

static __inline__ cyclecount_t GetCpucycleCount(void) {
    unsigned int lo, hi;
    asm volatile("rdtscp"
                 : "=a"(lo), "=d"(hi) /* outputs */
                 : "a"(0)             /* inputs */
                 : "%ebx", "%ecx");   /* clobbers*/
    return ((cyclecount_t)lo) | (((cyclecount_t)hi) << 32);
}

static __inline__ cyclecount_t Diff(cyclecount_t t1, cyclecount_t t0) { return (t1 - t0); }

struct dummyData {
  int a;
  int b;
  timespec ts;
  char c[16];

  std::string ToString() { return "asdf\n"; }
};

using namespace std::chrono;

int main(int argc, char** argv) {
 
  HFSAT::SharedMemWriter<dummyData>* writer_ = new HFSAT::SharedMemWriter<dummyData>(HFSAT::kExchSourceBMFEQ);
  int writer_id_ = writer_->intilizeWriter();
  if ( -1 == writer_id_) {
   // error
    std::cout<<"Error in Setting writer ID" << std::endl;
    return 0;
  }
  std::cout<<"WriterId "<< writer_id_ << std::endl;
  dummyData d1;
  HFSAT:: DataWriterIdPair<dummyData> send_order_request_;
  send_order_request_.writer_id = writer_id_;
  d1.a = writer_id_;
  d1.b = writer_id_;
  int num_packets_ = 10000;
  int t = 10;
  double sum = 0;
  cyclecount_t total_c=0,c1,c2,tc;
   
  while(t--){
  
  high_resolution_clock::time_point t1 = high_resolution_clock::now();
  c1=GetCpucycleCount();
  for (int i = 0; i < num_packets_; i++) {
      clock_gettime(CLOCK_REALTIME, &d1.ts);
      send_order_request_.data = d1;
      writer_->writeT(send_order_request_);
  }
  c2=GetCpucycleCount();
  //std::cout<<c2<<std::endl;
  high_resolution_clock::time_point t2 = high_resolution_clock::now();
  tc = Diff(c2,c1);
  total_c += tc;
  duration<double, std::milli> time_span = t2 - t1;
  sum += time_span.count();
  std::cout << "Time_in_Milli: " << time_span.count() << " CPUCYLES: " << tc << "\n";
  }
  std::cout << "TotalTime: " << sum <<" CPUCYLES: " << total_c << std::endl;
  sleep(2);
  return 0;
}
