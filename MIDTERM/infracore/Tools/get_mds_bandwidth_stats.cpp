// =====================================================================================
//
//       Filename:  get_mds_bandwidth_stats.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  08/07/2012 06:13:37 PM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
//
//        Address:  Suite No 162, Evoma, #14, Bhattarhalli,
//                  Old Madras Road, Near Garden City College,
//                  KR Puram, Bangalore 560049, India
//          Phone:  +91 80 4190 3551
//
// =====================================================================================

#include <iostream>
#include <stdlib.h>
#include <algorithm>

#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/ttime.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp"

std::vector<unsigned long long> g_sorted_time_events;
unsigned long long g_total_events_encountered = 0;

template <class T>
// gets all the timestamps for this day into a global vector
class MDSLogReader {
 public:
  static void LoadMDSEvents(HFSAT::BulkFileReader& bulk_file_reader) {
    T next_event;

    if (bulk_file_reader.is_open()) {
      while (true) {
        size_t available_len = bulk_file_reader.read(&next_event, sizeof(T));

        if (available_len < sizeof(next_event)) {
          break;
        }
        // in case of NSEDotexStruct, use source_time field instead of time
        /*unsigned int hhmm = HFSAT::DateTime::GetUTCHHMMFromTime(next_event.time_.tv_sec);
        if (hhmm < 400 || hhmm >= 1000) {  // get all packets in non NSE hours
          g_total_events_encountered++;
          g_sorted_time_events.push_back(next_event.time_.tv_sec * 1000000 + next_event.time_.tv_usec);
        }*/
        g_total_events_encountered++;
        g_sorted_time_events.push_back(next_event.time_.tv_sec * 1000000 + next_event.time_.tv_usec);
      }

      bulk_file_reader.close();
    }
  }
};

void ComputeBandwidthStats(int bandwidth_time_interval, int raw_size, int low_size) {
  std::sort(g_sorted_time_events.begin(), g_sorted_time_events.end());

  unsigned long long start_time = 0;
  int total_event_over_time_interval = 1;

  int struct_size = low_size == -1 ? raw_size : low_size;

  std::vector<unsigned long> time_interval_bandwidth_vec;

  for (auto event_time : g_sorted_time_events) {
    if (start_time == 0) {
      start_time = event_time - (event_time % bandwidth_time_interval);
    } else if ((int)(event_time - start_time) >= bandwidth_time_interval) {
      do {
        time_interval_bandwidth_vec.push_back(total_event_over_time_interval * struct_size);
        start_time += bandwidth_time_interval;
        total_event_over_time_interval = 0;
      } while ((int)(event_time - start_time) >= bandwidth_time_interval);

      total_event_over_time_interval++;
    } else {
      total_event_over_time_interval++;
    }
  }

  unsigned long total_bandwidth = 0;
  if (time_interval_bandwidth_vec.size() <= 0) {
    std::cout << std::endl;
    return;
  }
  std::sort(time_interval_bandwidth_vec.begin(), time_interval_bandwidth_vec.end());

  for (unsigned long bandwidth_events = 0; bandwidth_events < time_interval_bandwidth_vec.size(); bandwidth_events++) {
    total_bandwidth += time_interval_bandwidth_vec[bandwidth_events];
  }
  std::cout << "Filename,Mean,"
            << "Median,"
            << "75,"
            << "90,"
            << "95,"
            << "99" << std::endl;  // all numbers are in kbps

  std::cout << (double)(((total_bandwidth * 1000000) / (time_interval_bandwidth_vec.size() * bandwidth_time_interval)) *
                        8 / (double)1024)
            << ",";
  std::cout << (double)((((double)time_interval_bandwidth_vec[time_interval_bandwidth_vec.size() / 2] * 1000000) /
                         (bandwidth_time_interval)) *
                        8 / (double)1024)
            << ",";
  std::cout << (double)((((double)time_interval_bandwidth_vec[time_interval_bandwidth_vec.size() * 3 / 4] * 1000000) /
                         (bandwidth_time_interval)) *
                        8 / (double)1024)
            << ",";
  std::cout << (double)((((double)time_interval_bandwidth_vec[time_interval_bandwidth_vec.size() * 9 / 10] * 1000000) /
                         (bandwidth_time_interval)) *
                        8 / (double)1024)
            << ",";
  std::cout << (double)((((double)time_interval_bandwidth_vec[time_interval_bandwidth_vec.size() * 95 / 100] *
                          1000000) /
                         (bandwidth_time_interval)) *
                        8 / (double)1024)
            << ",";
  std::cout << (double)((((double)time_interval_bandwidth_vec[time_interval_bandwidth_vec.size() * 99 / 100] *
                          1000000) /
                         (bandwidth_time_interval)) *
                        8 / (double)1024)
            << std::endl;
}

int main(int argc, char** argv) {
  if (argc < 4) {
    std::cout << " USAGE: EXEC <exchange> <time-interval-usec> <files-list> " << std::endl;
    exit(0);
  }

  std::string exch = argv[1];
  int interval_usecs = atoi(argv[2]);

  int raw_size = -1;
  int low_bw_size = -1;

  for (int file_counter = 3; file_counter < argc; file_counter++) {
    HFSAT::BulkFileReader reader;
    reader.open(argv[file_counter]);
    std::cout << std::string(argv[file_counter]) << ",";

    if (exch == "EUREX") {
      MDSLogReader<EUREX_MDS::EUREXCommonStruct>::LoadMDSEvents(reader);
      raw_size = sizeof(EUREX_MDS::EUREXCommonStruct);
      low_bw_size = sizeof(EUREX_MDS::EUREXLSCommonStruct);
    } else if (exch == "CME") {
      MDSLogReader<CME_MDS::CMECommonStruct>::LoadMDSEvents(reader);
      raw_size = sizeof(CME_MDS::CMECommonStruct);
      low_bw_size = sizeof(CME_MDS::CMELSCommonStruct);
    } else if (exch == "BMF") {
      MDSLogReader<BMF_MDS::BMFCommonStruct>::LoadMDSEvents(reader);
      raw_size = sizeof(BMF_MDS::BMFCommonStruct);
    } else if (exch == "NTP") {
      MDSLogReader<NTP_MDS::NTPCommonStruct>::LoadMDSEvents(reader);
      raw_size = sizeof(NTP_MDS::NTPCommonStruct);
    } else if (exch == "TMX") {
      MDSLogReader<TMX_MDS::TMXCommonStruct>::LoadMDSEvents(reader);
      raw_size = sizeof(TMX_MDS::TMXCommonStruct);
    } else if (exch == "LIFFE") {
      MDSLogReader<LIFFE_MDS::LIFFECommonStruct>::LoadMDSEvents(reader);
      raw_size = sizeof(LIFFE_MDS::LIFFECommonStruct);
    } else if (exch == "ASX") {
      MDSLogReader<ASX_MDS::ASXPFCommonStruct>::LoadMDSEvents(reader);
      raw_size = sizeof(ASX_MDS::ASXPFCommonStruct);
    } else if (exch == "OSE") {
      MDSLogReader<OSE_MDS::OSEPriceFeedCommonStruct>::LoadMDSEvents(reader);
      raw_size = sizeof(OSE_MDS::OSEPriceFeedCommonStruct);
    } else if (exch == "HKEX") {
      MDSLogReader<HKEX_MDS::HKEXCommonStruct>::LoadMDSEvents(reader);
      raw_size = sizeof(HKEX_MDS::HKEXCommonStruct);
    } else if (exch == "HKOMD") {
      MDSLogReader<HKOMD_MDS::HKOMDPFCommonStruct>::LoadMDSEvents(reader);
      raw_size = sizeof(HKOMD_MDS::HKOMDPFCommonStruct);
    } else if (exch == "RTS") {
      MDSLogReader<RTS_MDS::RTSCommonStruct>::LoadMDSEvents(reader);
      raw_size = sizeof(RTS_MDS::RTSCommonStruct);
    } else if (exch == "MICEX") {
      MDSLogReader<MICEX_MDS::MICEXCommonStruct>::LoadMDSEvents(reader);
      raw_size = sizeof(MICEX_MDS::MICEXCommonStruct);
    } else if (exch == "SGX_PF") {
      MDSLogReader<SGX_MDS::SGXPFCommonStruct>::LoadMDSEvents(reader);
      raw_size = sizeof(SGX_MDS::SGXPFCommonStruct);
    }
    /*else if (exch == "SGX_ITCH") { //To uncomment this please change timeval field to time_ in SGXItchOrder struct
      MDSLogReader<SGX_ITCH_MDS::SGXItchOrder>::LoadMDSEvents(reader);
      raw_size = sizeof(SGX_ITCH_MDS::SGXItchOrder);
    } */
    else if (exch == "OSE_ITCH_PF") {
      MDSLogReader<OSE_ITCH_MDS::OSEPFCommonStruct>::LoadMDSEvents(reader);
      raw_size = sizeof(OSE_ITCH_MDS::OSEPFCommonStruct);
    } else if (exch == "OSE_PF") {
      MDSLogReader<OSE_MDS::OSEPriceFeedCommonStruct>::LoadMDSEvents(reader);
      raw_size = sizeof(OSE_MDS::OSEPriceFeedCommonStruct);
    }
    /*else if (exch == "NSE") { //for NSE time is named source_time
      MDSLogReader<NSE_MDS::NSEDotexOfflineCommonStruct>::LoadMDSEvents(reader);
      raw_size = sizeof(NSE_MDS::NSEDotexOfflineCommonStruct);
    } */ else {
      std::cerr << "Invalid exchange type" << std::endl;
      return 0;
    }
  }
  if (g_sorted_time_events.size() > 0)
    ComputeBandwidthStats(interval_usecs, raw_size, low_bw_size);
  else
    std::cout << std::endl;
  return 0;
}
