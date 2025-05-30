/**
   \file Tools/mds_log_bcaster.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066
   India
   +91 80 4060 0717
*/
#include <iostream>
#include <stdlib.h>

#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/ttime.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"

#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/Utils/multicast_sender_socket.hpp"

void ReadMDSStructs(HFSAT::BulkFileReader& bulk_file_reader_, const HFSAT::ExchSource_t& exch, const int& st,
                    const int& et, const char* bcast_ip_, const int bcast_port_, const std::string& interface_) {
  HFSAT::MulticastSenderSocket mcast_sender_(bcast_ip_, bcast_port_, interface_);

  if (bulk_file_reader_.is_open()) {
    switch (exch) {
      case HFSAT::kExchSourceBMF: {
        NTP_MDS::NTPCommonStruct next_event_;
        while (true) {
          size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(next_event_));
          if (available_len_ < sizeof(next_event_)) break;
          HFSAT::ttime_t next_event_timestamp_ = next_event_.time_;
          if (et < next_event_timestamp_.tv_sec || next_event_timestamp_.tv_sec < st)  // time filter
            continue;
        }
      } break;
      case HFSAT::kExchSourceCME: {
        CME_MDS::CMECommonStruct next_event_;
        while (true) {
          size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(next_event_));
          if (available_len_ < sizeof(next_event_)) break;
          HFSAT::ttime_t next_event_timestamp_ = next_event_.time_;
          if (et < next_event_timestamp_.tv_sec || next_event_timestamp_.tv_sec < st)  // time filter
            continue;
        }
      } break;
      case HFSAT::kExchSourceEUREX: {
        EUREX_MDS::EUREXCommonStruct next_event_;
        while (true) {
          size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(next_event_));
          if (available_len_ < sizeof(next_event_)) break;
          HFSAT::ttime_t next_event_timestamp_ = next_event_.time_;
          if (et < next_event_timestamp_.tv_sec || next_event_timestamp_.tv_sec < st)  // time filter
            continue;
        }
      } break;
      case HFSAT::kExchSourceTMX: {
        TMX_MDS::TMXCommonStruct next_event_;
        while (true) {
          size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(next_event_));
          if (available_len_ < sizeof(next_event_)) break;
          HFSAT::ttime_t next_event_timestamp_ = next_event_.time_;
          if (et < next_event_timestamp_.tv_sec || next_event_timestamp_.tv_sec < st)  // time filter
            continue;

          gettimeofday(&next_event_.time_, NULL);
          mcast_sender_.WriteN(sizeof(next_event_), &next_event_);

          HFSAT::usleep(200000);
        }
      } break;
      default:
        std::cout << "exchange not implemented for\n";
        break;
    }
  }
}

int main(int argc, char** argv) {
  if (argc != 6 && argc != 8) {
    std::cout << " USAGE: EXEC <exchange> <file_path> <bcast_ip> <bcast_port> <interface> "
                 "<start-time-optional-unix-time-sec> <end-time-optional-unix-time-sec>"
              << std::endl;
    exit(0);
  }

  std::string exch = argv[1];
  HFSAT::ExchSource_t exch_src = HFSAT::kExchSourceInvalid;
  if (exch == "EUREX")
    exch_src = HFSAT::kExchSourceEUREX;
  else if (exch == "CME")
    exch_src = HFSAT::kExchSourceCME;
  else if (exch == "BMF")
    exch_src = HFSAT::kExchSourceBMF;
  else if (exch == "TMX")
    exch_src = HFSAT::kExchSourceTMX;

  HFSAT::BulkFileReader reader;
  reader.open(argv[2]);

  int st = 0;
  int et = 0x7fffffff;
  std::string interface = argv[5];
  if (argc == 8) {
    st = atol(argv[6]);
    et = atol(argv[7]);
  }

  ReadMDSStructs(reader, exch_src, st, et, argv[3], atoi(argv[4]), interface);
  return 0;
}
