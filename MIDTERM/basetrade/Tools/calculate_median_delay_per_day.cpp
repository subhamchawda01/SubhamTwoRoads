/**
   \file Tools/volume_symbol_reconcilation.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 162, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <fstream>
#include <signal.h>
#include <getopt.h>
#include <map>
#include <algorithm>

#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CDef/file_utils.hpp"  // To create the directory
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CDef/trading_location_manager.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/ttime.hpp"
#include "dvccode/CDef/ors_defines.hpp"
#include "dvccode/CDef/security_definitions.hpp"

#include "dvccode/Utils/multicast_receiver_socket.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/CDef/ors_messages.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/CDef/ttime.hpp"
#include "baseinfra/LoggedSources/ors_message_filenamer.hpp"

// This class will read from the ORSBInary Log file
class DelayCalculator {
 public:
  DelayCalculator(std::string _shortcode_, int _yyyymmdd_, int _percentile_) :
    shortcode_(_shortcode_), yyyymmdd_(_yyyymmdd_), percentile_(_percentile_) {}

  /// Reader
  void calculate_delay() {
    std::map<int, HFSAT::ttime_t> seqd_orders_map_;
    long long total_time = 0;
    int orders = 0;
    std::vector<int> time_differences;

    HFSAT::ExchangeSymbolManager::SetUniqueInstance(yyyymmdd_);
    HFSAT::ExchangeSymbolManager::SetUniqueInstance(yyyymmdd_);
    if (strncmp(shortcode_.c_str(), "NSE_", 4) == 0) {
      HFSAT::SecurityDefinitions::GetUniqueInstance(yyyymmdd_).LoadNSESecurityDefinitions();
    }

    const char* t_exchange_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(shortcode_);
    HFSAT::TradingLocation_t location_ = HFSAT::TradingLocationUtils::GetTradingLocationExch(
        HFSAT::SecurityDefinitions::GetContractExchSource(shortcode_, yyyymmdd_));

    std::string filename_to_read = HFSAT::ORSMessageFileNamer::GetName(t_exchange_symbol_, yyyymmdd_, location_);
    std::replace(filename_to_read.begin(), filename_to_read.end(), ' ', '~');

    HFSAT::BulkFileReader bulk_file_reader_;

    if (!HFSAT::FileUtils::IsFile(filename_to_read)) {
      std::cerr << filename_to_read << " not found" << std::endl;
      exit(1);
    }

    bulk_file_reader_.open(filename_to_read.c_str());

    if (bulk_file_reader_.is_open()) {
      while (true) {
        HFSAT::GenericORSReplyStruct reply_struct;

        size_t read_length_ =
            bulk_file_reader_.read(reinterpret_cast<char*>(&reply_struct), sizeof(HFSAT::GenericORSReplyStruct));

        if (read_length_ < sizeof(HFSAT::GenericORSReplyStruct)) break;

        switch (reply_struct.orr_type_) {
          case HFSAT::kORRType_Seqd: {
            if (seqd_orders_map_.find(reply_struct.server_assigned_order_sequence_) == seqd_orders_map_.end()) {
              seqd_orders_map_[reply_struct.server_assigned_order_sequence_] = reply_struct.time_set_by_server_;
            } else {
              // std::cout << -1 << " " << shortcode_ << " " << yyyymmdd_ << " "
              //         << reply_struct.server_assigned_order_sequence_ << std::endl;
              // return;
              continue;
            }

          } break;

          case HFSAT::kORRType_Conf: {
            if (seqd_orders_map_.find(reply_struct.server_assigned_order_sequence_) != seqd_orders_map_.end()) {
              HFSAT::ttime_t difference =
                  reply_struct.time_set_by_server_ - seqd_orders_map_[reply_struct.server_assigned_order_sequence_];
              //   std::cout << difference.ToString() << std::endl;
              time_differences.push_back(difference.tv_sec * 1000000 + difference.tv_usec);
              total_time += difference.tv_sec * 1000000 + difference.tv_usec;
              orders++;
              seqd_orders_map_.erase(reply_struct.server_assigned_order_sequence_);
            } else {
              std::cerr << " Conf before Seqd for Saos : " << reply_struct.server_assigned_order_sequence_ << std::endl;
              exit(2);
            }
          } break;

          default: { } break; }
      }
    }
    if (orders == 0) {
      exit(1);
    }
    std::sort(time_differences.begin(), time_differences.end());
    // std::cout << total_time << " " << (double)orders << std::endl;
    // std::cout << total_time / (double)orders << std::endl;
    std::cout << time_differences[(time_differences.size() * percentile_) / 100] << std::endl;
  }

 private:
  HFSAT::GenericORSReplyStruct generic_ors_reply_struct_;
  std::string shortcode_;
  int yyyymmdd_;
  int percentile_;
};

int main(int argc, char** argv) {
  std::string dir = "";
  std::string shortcode_ = "";
  int yyyymmdd_ = 0;
  int percentile_ = 50;

  if (argc < 3) {
    std::cout << "Usage : SHORTCODE YYYYMMDD [percentile=50]" << std::endl;
    exit(0);
  }

  shortcode_ = std::string(argv[1]);
  yyyymmdd_ = atoi(argv[2]);

  if (argc > 3) {
    percentile_ = atoi(argv[3]);
  }

  DelayCalculator delay_calculator(shortcode_, yyyymmdd_, percentile_);
  delay_calculator.calculate_delay();

  return 0;
}
