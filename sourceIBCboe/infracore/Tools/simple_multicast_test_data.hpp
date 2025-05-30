
#pragma once
#include <string>
#include <sstream>
#define MC_TEST_PACKET_LEN 38

#pragma pack(push, 1)
struct TestOrderDatagram {
  uint64_t seq_;
  HFSAT::ttime_t time_;
  char reserve_data[MC_TEST_PACKET_LEN - 16];

  TestOrderDatagram() : seq_(-1), time_(HFSAT::GetTimeOfDay()) {}
  void PrepareNextPacket() {
    seq_++;
    time_ = HFSAT::GetTimeOfDay();
  }
  std::string ToString() {
    std::stringstream ss;
    ss << " seq: " << seq_;
    ss << " TS: " << time_;
    std::string str = ss.str();
    return str;
  }
};
#pragma pack(pop)
