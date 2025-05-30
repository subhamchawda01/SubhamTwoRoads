// =====================================================================================
//
//       Filename:  all_exchange_ors_binary_capture.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  04/25/2013 06:21:38 AM
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

#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <fstream>
#include <signal.h>
#include <getopt.h>
#include <map>

#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CDef/file_utils.hpp"  // To create the directory
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CDef/trading_location_manager.hpp"
#include "dvccode/CDef/defines.hpp"

#include "dvccode/Utils/multicast_receiver_socket.hpp"
#include "dvccode/CDef/ors_messages.hpp"
#include "dvccode/TradingInfo/network_account_interface_manager.hpp"
#include "dvccode/TradingInfo/network_account_info_manager.hpp"

#define LOGGED_DATA_PREFIX "/spare/local/ORSBCAST/"

void print_usage(const char* prg_name) {
  printf("This is the ORS Binary Data Logger \n");
  printf("Usage:%s --exch <EXCHANGE> \n", prg_name);
}

static struct option options_[] = {{"help", no_argument, 0, 'h'}, {"exch", required_argument, 0, 'a'}, {0, 0, 0, 0}};

class MultiSourceORSBinaryLogger {
 public:
  MultiSourceORSBinaryLogger(std::vector<std::string>& exchsource_string_vec_)
      : exchsource_to_socket_map_(), generic_ors_reply_struct_(), exchange_symbol_to_ors_bcast_log_map_() {
    HFSAT::NetworkAccountInfoManager network_account_info_manager_;

    for (unsigned int exch_source_counter_ = 0; exch_source_counter_ < exchsource_string_vec_.size();
         exch_source_counter_++) {
      std::string this_exchname_ = exchsource_string_vec_[exch_source_counter_];

      HFSAT::ExchSource_t this_exchsource_ = HFSAT::StringToExchSource(this_exchname_.c_str());

      std::string dummy_shortcode_ = "";

      switch (this_exchsource_) {
        case HFSAT::kExchSourceCME: {
          dummy_shortcode_ = "ZN_0";
        } break;
        case HFSAT::kExchSourceEUREX: {
          dummy_shortcode_ = "FESX_0";
        } break;
        case HFSAT::kExchSourceLIFFE: {
          dummy_shortcode_ = "LFR_0";
        } break;
        case HFSAT::kExchSourceBMF: {
          dummy_shortcode_ = "BR_DOL_0";
        } break;
        case HFSAT::kExchSourceTMX: {
          dummy_shortcode_ = "CGB_0";
        } break;
        case HFSAT::kExchSourceJPY: {
          dummy_shortcode_ = "NK_0";
        } break;
        case HFSAT::kExchSourceHONGKONG: {
          dummy_shortcode_ = "HSI_0";
        } break;

        default: { dummy_shortcode_ = "ES_0"; } break;
      }

      HFSAT::DataInfo data_info_ = network_account_info_manager_.GetDepDataInfo(this_exchsource_, dummy_shortcode_);

      HFSAT::MulticastReceiverSocket* this_exch_multicast_receiver_socket_ = new HFSAT::MulticastReceiverSocket(
          data_info_.bcast_ip_, data_info_.bcast_port_,
          HFSAT::NetworkAccountInterfaceManager::instance().GetInterfaceForApp(HFSAT::k_ORS_LS));

      exchsource_to_socket_map_[this_exchname_] = this_exch_multicast_receiver_socket_;
    }
  }

  void ProcessORSData() {
    fd_set readfd_;

    FD_ZERO(&readfd_);

    int max_descriptor_ = 0;

    std::map<std::string, HFSAT::MulticastReceiverSocket*>::iterator itr_;

    itr_ = exchsource_to_socket_map_.begin();

    while (itr_ != exchsource_to_socket_map_.end()) {
      max_descriptor_ = (max_descriptor_ < ((itr_->second)->socket_file_descriptor()))
                            ? ((itr_->second)->socket_file_descriptor())
                            : max_descriptor_;

      itr_++;
    }

    max_descriptor_ += 1;

    while (true) {
      itr_ = exchsource_to_socket_map_.begin();

      while (itr_ != exchsource_to_socket_map_.end()) {
        FD_SET(((itr_->second)->socket_file_descriptor()), &readfd_);

        itr_++;
      }

      int select_returned_value_ = select(max_descriptor_, &readfd_, NULL, NULL, NULL);

      if (select_returned_value_ <= 0) {
        std::cerr << " Select Returned Error : " << select_returned_value_ << "\n";
        exit(-1);
      }

      int read_bytes_ = 0;

      itr_ = exchsource_to_socket_map_.begin();

      while (itr_ != exchsource_to_socket_map_.end()) {
        if (FD_ISSET(((itr_->second)->socket_file_descriptor()), &readfd_)) {
          read_bytes_ = ((HFSAT::MulticastReceiverSocket*)(itr_->second))
                            ->ReadN(sizeof(HFSAT::GenericORSReplyStruct), (void*)&generic_ors_reply_struct_);

          if (read_bytes_ < (int)(sizeof(HFSAT::GenericORSReplyStruct))) {
            std::cerr << " Read Error , Number Of Bytes Read : " << read_bytes_
                      << " Expected : " << sizeof(HFSAT::GenericORSReplyStruct) << "\n";
            exit(-1);

          } else {
            std::string symbol_got(generic_ors_reply_struct_.symbol_);

            if ((itr_->first) == "LIFFE") {
              std::replace(symbol_got.begin(), symbol_got.end(), ' ', '~');
            }

            if (exchange_symbol_to_ors_bcast_log_map_.find(symbol_got) == exchange_symbol_to_ors_bcast_log_map_.end()) {
              AddToMap((itr_->first), symbol_got);
            }

            std::ofstream* file_streamer = exchange_symbol_to_ors_bcast_log_map_[symbol_got];
            file_streamer->write((char*)(&generic_ors_reply_struct_), read_bytes_);
            file_streamer->flush();
          }
        }

        itr_++;
      }
    }
  }

  void AddToMap(std::string exchange_name_, std::string exchange_symbol) {
    time_t m_time_t;
    time(&m_time_t);
    struct tm m_tm;
    localtime_r(&m_time_t, &m_tm);
    unsigned int yyyymmdd = (1900 + m_tm.tm_year) * 10000 + (1 + m_tm.tm_mon) * 100 + m_tm.tm_mday;
    char date_str[9] = {'\0'};
    sprintf(date_str, "%d", yyyymmdd);
    std::string filename = LOGGED_DATA_PREFIX + exchange_name_ + "/" + exchange_symbol + "_" + date_str;
    std::cout << "Total Filename :" << filename << std::endl;
    std::ofstream* new_file = new std::ofstream();

    HFSAT::FileUtils::MkdirEnclosing(filename);

    new_file->open(filename.c_str(), std::ofstream::binary | std::ofstream::app | std::ofstream::ate);
    if (!new_file->is_open()) {
      fprintf(stderr, "ors_bcast_binary could not open file %s for writing \n", filename.c_str());
      exit(-1);
    }
    exchange_symbol_to_ors_bcast_log_map_[exchange_symbol] = new_file;
  }

 private:
  std::map<std::string, HFSAT::MulticastReceiverSocket*> exchsource_to_socket_map_;
  HFSAT::GenericORSReplyStruct generic_ors_reply_struct_;
  std::map<std::string, std::ofstream*> exchange_symbol_to_ors_bcast_log_map_;
};

void sighandler(int signum) { exit(0); }

int main(int argc, char** argv) {
  //-------------- START OPTIONS--------------
  int c;
  int hflag = 0;

  std::string exch_ = "";

  /// parse input options
  while (1) {
    int option_index = 0;
    c = getopt_long(argc, argv, "", options_, &option_index);
    if (c == -1) break;
    switch (c) {
      case 'h':
        hflag = 1;
        break;

      case 'a':
        exch_ = optarg;
        break;

      case '?':
        if (optopt != 'h') {
          fprintf(stderr, "Option %c requires an argument .. will exit \n", optopt);
          print_usage(argv[0]);
          exit(-1);
        }
        break;

      default:
        fprintf(stderr, "Weird option specified .. no handling yet \n");
        break;
    }
  }

  if (hflag || exch_.empty()) {
    print_usage(argv[0]);
    exit(-1);
  }

  std::string exchange_name = "CME";

  exchange_name = exch_;

  std::vector<std::string> exchange_list_;

  if (exchange_name == "ALL") {
    exchange_list_.push_back("CME");
    exchange_list_.push_back("EUREX");
    exchange_list_.push_back("LIFFE");
    exchange_list_.push_back("BMF");
    exchange_list_.push_back("TMX");
    exchange_list_.push_back("HONGKONG");
    exchange_list_.push_back("OSE");

  } else {
    std::cerr << " For Exch Argument only ALL expected \n";
    exit(-1);
  }

  MultiSourceORSBinaryLogger multisource_ors_bin_logger_(exchange_list_);

  // Start getting data
  multisource_ors_bin_logger_.ProcessORSData();

  struct sigaction sigact;
  sigact.sa_handler = sighandler;
  sigaction(SIGINT, &sigact, NULL);
}
