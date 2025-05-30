#include <iostream>
#include <ctime>
#include "dvccode/Profiler/cpucycle_profiler.hpp"
#include "dvccode/Utils/multicast_sender_socket.hpp"

#include <vector>

struct msg_struct {
  timeval starttime;
  char contract_[12];  ///< internal contract name
  uint32_t trd_qty_;   ///< quantity traded till now (ignore if 0)
  uint32_t level_;     ///< level >= 1 (level 0 is currently filtered at daemon)
  uint32_t size_;      ///< size at level
  double price_;       ///< price at level
  char type_;          ///< type 1 - Ask 2 - Bid , other types filtered
  char status_;        ///< ' ' is normal operation. Currently not filtered
  char action_;        ///< 1 - New, 2 - Change, 3 - Delete, 4 - Delete From , 5 - Delete through
  bool intermediate_;  ///< if this is an intermediate message .. clients should not react to intermediates
  uint32_t num_ords_;  ///<  EUREX 13.0 change, showing the number of price_size contributers ( orders ) at thislevel
  char pxsrc_[3];      ///< ???
  char flags[5];       ///< reserved ?
  //  timeval endtime;
};
typedef struct msg_struct Msgs;

int main(int argc, char** argv) {
  std::string mcast_ip_;
  int mcast_port_;

  if (argc < 4) {
    std::cerr << " Usage: " << argv[0] << " <ip> <port> <text>" << std::endl;
    exit(0);
  }

  HFSAT::CpucycleProfiler& cpucylcle_profiler_ = HFSAT::CpucycleProfiler::GetUniqueInstance(3);
  cpucylcle_profiler_.SetTag(0, "gettimeofday");
  cpucylcle_profiler_.SetTag(1, "set-some-value");
  cpucylcle_profiler_.SetTag(2, "writeN");

  Msgs test_msg_;
  timeval sample_time_;
  HFSAT::MulticastSenderSocket abc_(mcast_ip_, mcast_port_);

  for (int ii = 0; ii < 100000; ii++) {
    cpucylcle_profiler_.Start(0);
    gettimeofday(&sample_time_, NULL);  // 215 cc = 86 nanos
    cpucylcle_profiler_.End(0);
    cpucylcle_profiler_.Start(1);
    test_msg_.starttime = sample_time_;  // 70 cc = 28 nanos
    cpucylcle_profiler_.End(1);
    cpucylcle_profiler_.Start(2);
    abc_.WriteN(sizeof(Msgs), (char*)&test_msg_);  // 880 cc = 352 nanos
                                                   //        abc_.WriteN( sizeof(int), &ii);
    cpucylcle_profiler_.End(2);
  }

  const std::vector<HFSAT::CpucycleProfilerSummaryStruct> prof_summary_ = cpucylcle_profiler_.GetCpucycleSummary();
  /*  for ( unsigned int cnum_ = 0 ; cnum_ < sumstr.size ( ) ; cnum_ ++ ) {
      std::cout << " Values of counter : " << cnum_ << " : " ;
      std::cout << sumstr[cnum_].min_ << " " <<
        sumstr[cnum_].max_ << " " <<
        sumstr[cnum_].mean_ << " " <<
        sumstr[cnum_].fifty_percentile_ << " " <<
        sumstr[cnum_].sixty_percentile_ << " " <<
        sumstr[cnum_].seventy_percentile_ << " " <<
        sumstr[cnum_].eighty_percentile_ << " " <<
        sumstr[cnum_].ninety_percentile_ << " " <<
        sumstr[cnum_].ninetyfive_percentile_ << " " <<
        std::endl;
    }*/

  for (int i = 0; i < prof_summary_.size(); i++) {
    std::cout << prof_summary_[i].tag_name_ << "\n";
    std::cout << "\tMean: " << prof_summary_[i].mean_ << "\tMin: " << prof_summary_[i].min_
              << "\tMedian: " << prof_summary_[i].fifty_percentile_ << "\tMax: " << prof_summary_[i].max_
              << "\t95th: " << prof_summary_[i].ninetyfive_percentile_
              << "\tcount:" << prof_summary_[i].total_occurrence_ << "\n";
  }

  return 0;
}
