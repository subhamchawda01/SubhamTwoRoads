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

#define LOGGED_DATA_PREFIX "/spare/local/logs/tradelogs/ORSBCAST/"

// This class will read from the ORSBInary Log file
class ORSBinReader {
 public:
  ORSBinReader(std::string _shortcode_, int _yyyymmdd_, bool summary_, bool plottable_stats_)
      : shortcode_(_shortcode_),
        yyyymmdd_(_yyyymmdd_),
        seqd_conf_summary_(summary_),
        seqd_conf_plottable_stats_(plottable_stats_) {}

  /// Reader
  void processMsgRecvd() {
    std::map<int, HFSAT::ttime_t> seqd_orders_map_;
    std::map<int, HFSAT::ttime_t> conf_orders_map_;

    HFSAT::ExchangeSymbolManager::SetUniqueInstance(yyyymmdd_);
    const char* t_exchange_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(shortcode_);

    std::string this_exch_source_ =
        HFSAT::ExchSourceStringForm(HFSAT::SecurityDefinitions::GetContractExchSource(shortcode_, yyyymmdd_));

    std::ostringstream t_temp_oss_;
    t_temp_oss_ << yyyymmdd_;
    std::string date_ = t_temp_oss_.str();

    std::stringstream ff;
    ff << LOGGED_DATA_PREFIX << this_exch_source_ << "/" << t_exchange_symbol_ << "_" << yyyymmdd_;

    std::string filename_to_read = ff.str();

    while (filename_to_read.find(" ") != std::string::npos) {  // Liffe naming issues
      filename_to_read.replace(filename_to_read.find(" "), 1, "~");
    }

    HFSAT::BulkFileReader bulk_file_reader_;
    bulk_file_reader_.open(filename_to_read.c_str());

    if (bulk_file_reader_.is_open()) {
      while (true) {
        HFSAT::GenericORSReplyStruct reply_struct;

        size_t read_length_ =
            bulk_file_reader_.read(reinterpret_cast<char*>(&reply_struct), sizeof(HFSAT::GenericORSReplyStruct));

        if (read_length_ < sizeof(HFSAT::GenericORSReplyStruct)) break;

        char pp[6] = {'\0'};
        sprintf(pp, "%.6f", reply_struct.price_);

        if (reply_struct.orr_type_ == HFSAT::kORRType_Rejc) {
          if (!seqd_conf_plottable_stats_ && !seqd_conf_summary_) {
            std::cout << "SYM : " << reply_struct.symbol_ << " Price :" << pp << " INTPX: " << reply_struct.int_price_
                      << " BS: " << HFSAT::GetTradeTypeChar(reply_struct.buysell_)
                      << " TIMESET : " << reply_struct.time_set_by_server_
                      << " ORRTYPE : " << ToString(reply_struct.orr_type_)
                      << " SAOS : " << reply_struct.server_assigned_order_sequence_
                      << " CLTPOS: " << reply_struct.client_position_
                      << " SACI: " << reply_struct.server_assigned_client_id_
                      << " GBLPOS: " << reply_struct.global_position_ << " REJECREASON: "
                      << HFSAT::ORSRejectionReasonStr(HFSAT::ORSRejectionReason_t(reply_struct.size_executed_))
                      << std::endl;
          }
        } else {
          if (!seqd_conf_plottable_stats_ && !seqd_conf_summary_) {
            std::cout << "SYM : " << reply_struct.symbol_ << " Price :" << pp << " INTPX: " << reply_struct.int_price_
                      << " BS: " << HFSAT::GetTradeTypeChar(reply_struct.buysell_)
                      << " TIMESET : " << reply_struct.time_set_by_server_
                      << " ORRTYPE : " << ToString(reply_struct.orr_type_)
                      << " SAOS : " << reply_struct.server_assigned_order_sequence_
                      << " CLTPOS: " << reply_struct.client_position_
                      << " SACI: " << reply_struct.server_assigned_client_id_
                      << " GBLPOS: " << reply_struct.global_position_ << " SIZE: " << reply_struct.size_remaining_
                      << " SIZE_EXEC: " << reply_struct.size_executed_ << std::endl;

          } else {
            switch (reply_struct.orr_type_) {
              case HFSAT::kORRType_Seqd: {
                if (seqd_orders_map_.find(reply_struct.server_assigned_order_sequence_) == seqd_orders_map_.end()) {
                  seqd_orders_map_[reply_struct.server_assigned_order_sequence_] = reply_struct.time_set_by_server_;
                }

              } break;

              case HFSAT::kORRType_Conf: {
                if (conf_orders_map_.find(reply_struct.server_assigned_order_sequence_) == conf_orders_map_.end()) {
                  conf_orders_map_[reply_struct.server_assigned_order_sequence_] = reply_struct.time_set_by_server_;
                }
              } break;

              default: { } break; }
          }
        }
      }
    }

    if (seqd_conf_plottable_stats_ || seqd_conf_summary_) {
      std::vector<HFSAT::ttime_t> time_diff_;
      std::map<int, HFSAT::ttime_t>::iterator seqd_orders_map_itr_ = seqd_orders_map_.begin();

      while (seqd_orders_map_itr_ != seqd_orders_map_.end()) {
        if (conf_orders_map_.find(seqd_orders_map_itr_->first) == conf_orders_map_.end()) {
          // std::cerr << " SAOS NOT FOUND " << (seqd_orders_map_itr_ -> first) << "\n" ;
          seqd_orders_map_itr_++;
          continue;

        } else {
          if (conf_orders_map_[seqd_orders_map_itr_->first] < seqd_orders_map_itr_->second) {
            // invalid conf saos before seqd
            seqd_orders_map_itr_++;
            continue;
          }

          time_diff_.push_back((conf_orders_map_[seqd_orders_map_itr_->first]) - (seqd_orders_map_itr_->second));

          if (seqd_conf_plottable_stats_)
            std::cout << (seqd_orders_map_itr_->first) << "\t" << seqd_orders_map_itr_->second << "\t"
                      << (((((conf_orders_map_[seqd_orders_map_itr_->first]) - (seqd_orders_map_itr_->second)).tv_sec *
                                1000000 +
                            ((conf_orders_map_[seqd_orders_map_itr_->first]) - (seqd_orders_map_itr_->second))
                                .tv_usec)) /
                          1000.0) << "\n";
        }

        seqd_orders_map_itr_++;
      }

      if (seqd_conf_summary_) {
        unsigned long total_time_ = 0;

        std::vector<unsigned long> diff_vec_;

        for (auto i = 0u; i < time_diff_.size(); i++) {
          total_time_ += time_diff_[i].tv_sec * 1000000 + time_diff_[i].tv_usec;
          diff_vec_.push_back(time_diff_[i].tv_sec * 1000000 + time_diff_[i].tv_usec);
        }

        std::sort(diff_vec_.begin(), diff_vec_.end());

        std::cerr << " Shortcode :" << shortcode_ << "  ";
        std::cerr << " Date:" << yyyymmdd_ << "  ";
        std::cerr << " Sample Size:" << time_diff_.size() << "  ";
        std::cerr << " Mean:" << (double)total_time_ / time_diff_.size() << "  ";
        std::cerr << " 50%:" << diff_vec_[(diff_vec_.size()) * 1 / 2] << "  ";
        std::cerr << " 75%:" << diff_vec_[(diff_vec_.size()) * 3 / 4] << "  ";
        std::cerr << " 90%:" << diff_vec_[(diff_vec_.size()) * 9 / 10] << "  ";
        std::cerr << " 95%:" << diff_vec_[(diff_vec_.size()) * 95 / 100] << "  ";
        std::cerr << " 99%:" << diff_vec_[(diff_vec_.size()) * 99 / 100] << "  ";
        std::cerr << " Max:" << diff_vec_[diff_vec_.size() - 1] << "  \n";
      }
    }
  }

 private:
  HFSAT::GenericORSReplyStruct generic_ors_reply_struct_;
  std::string shortcode_;
  int yyyymmdd_;
  bool seqd_conf_summary_;
  bool seqd_conf_plottable_stats_;
};

int main(int argc, char** argv) {
  std::string dir = "";
  std::string shortcode_ = "";
  int yyyymmdd_ = 0;
  bool seqd_conf_summary_ = false;
  bool seqd_conf_plottable_stats_ = false;

  if (argc < 3) {
    std::cout << "Usage : SHORTCODE YYYYMMDD" << std::endl;
    std::cout << "Usage : SHORTCODE YYYYMMDD SUMMARY " << std::endl;
    std::cout << "Usage : SHORTCODE YYYYMMDD PLOTTABLE " << std::endl;
    exit(0);
  } else {
    shortcode_ = std::string(argv[1]);
    yyyymmdd_ = atoi(argv[2]);

    if (argc >= 4) {
      if (!strcmp("SUMMARY", argv[3]))
        seqd_conf_summary_ = true;
      else if (!strcmp("PLOTTABLE", argv[3]))
        seqd_conf_plottable_stats_ = true;
    }
  }

  ORSBinReader common_logger(shortcode_, yyyymmdd_, seqd_conf_summary_, seqd_conf_plottable_stats_);

  common_logger.processMsgRecvd();
}
