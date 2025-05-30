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

#include "dvccode/CDef/defines.hpp"
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
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "baseinfra/LoggedSources/ors_message_filenamer.hpp"

#define LOGGED_DATA_PREFIX "/NAS1/data/ORSData/"

// This class will read from the ORSBInary Log file
class ORSBinReader {
 public:
  ORSBinReader(std::string _shortcode_, int _yyyymmdd_, bool summary_, bool plottable_stats_,
               std::string _ors_bin_file_, HFSAT::ttime_t _start_time_, HFSAT::ttime_t _end_time_)
      : shortcode_(_shortcode_),
        yyyymmdd_(_yyyymmdd_),
        seqd_conf_summary_(summary_),
        seqd_conf_plottable_stats_(plottable_stats_),
        ors_bin_filename_(_ors_bin_file_),
        start_time_(_start_time_),
        end_time_(_end_time_),
        bid_live_ords_sz_(0),
        ask_live_ords_sz_(0),
        saos_to_live_ord_map_() {}

  /// Reader
  void processMsgRecvd() {
    std::map<int, HFSAT::ttime_t> seqd_orders_map_;
    std::map<int, HFSAT::ttime_t> conf_orders_map_;

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

    if (ors_bin_filename_ != "") {
      filename_to_read = ors_bin_filename_;
    }

    bulk_file_reader_.open(filename_to_read.c_str());

    if (bulk_file_reader_.is_open()) {
      while (true) {
        HFSAT::GenericORSReplyStruct reply_struct;

        size_t read_length_ =
            bulk_file_reader_.read(reinterpret_cast<char*>(&reply_struct), sizeof(HFSAT::GenericORSReplyStruct));

        if (read_length_ < sizeof(HFSAT::GenericORSReplyStruct)) break;

        char pp[6] = {'\0'};
        sprintf(pp, "%.6f", reply_struct.price_);

        if (!(start_time_ == HFSAT::ttime_t(0, 0) || end_time_ == HFSAT::ttime_t(0, 0))) {
          if (reply_struct.time_set_by_server_ < start_time_ || reply_struct.time_set_by_server_ > end_time_) continue;
        }

        if (reply_struct.orr_type_ == HFSAT::kORRType_Conf || reply_struct.orr_type_ == HFSAT::kORRType_Exec) {
          int t_live_size_to_add_ = reply_struct.size_remaining_;
          if (saos_to_live_ord_map_.find(reply_struct.server_assigned_order_sequence_) != saos_to_live_ord_map_.end()) {
            t_live_size_to_add_ -= saos_to_live_ord_map_[reply_struct.server_assigned_order_sequence_].size_remaining_;
            if (reply_struct.size_remaining_ <= 0) {
              saos_to_live_ord_map_.erase(reply_struct.server_assigned_order_sequence_);
            }
          }

          if (reply_struct.size_remaining_ > 0) {
            saos_to_live_ord_map_[reply_struct.server_assigned_order_sequence_] = reply_struct;
          }

          if (reply_struct.buysell_ == HFSAT::kTradeTypeBuy) {
            bid_live_ords_sz_ += t_live_size_to_add_;
          } else if (reply_struct.buysell_ == HFSAT::kTradeTypeSell) {
            ask_live_ords_sz_ += t_live_size_to_add_;
          }
        } else if (reply_struct.orr_type_ == HFSAT::kORRType_Cxld) {
          if (saos_to_live_ord_map_.find(reply_struct.server_assigned_order_sequence_) != saos_to_live_ord_map_.end()) {
            if (reply_struct.buysell_ == HFSAT::kTradeTypeBuy) {
              bid_live_ords_sz_ -= saos_to_live_ord_map_[reply_struct.server_assigned_order_sequence_].size_remaining_;
            } else if (reply_struct.buysell_ == HFSAT::kTradeTypeSell) {
              ask_live_ords_sz_ -= saos_to_live_ord_map_[reply_struct.server_assigned_order_sequence_].size_remaining_;
            }
            saos_to_live_ord_map_.erase(reply_struct.server_assigned_order_sequence_);
          }
        }

        if (reply_struct.orr_type_ == HFSAT::kORRType_Rejc) {
          if (!seqd_conf_plottable_stats_ && !seqd_conf_summary_) {
            std::cout << "SYM: " << reply_struct.symbol_ << " Px: " << pp << " INTPX: " << reply_struct.int_price_
                      << " BS: " << HFSAT::GetTradeTypeChar(reply_struct.buysell_)
                      << " ST: " << reply_struct.time_set_by_server_ << " DT: " << reply_struct.client_request_time_
                      << " ORR: " << ToString(reply_struct.orr_type_)
                      << " SAOS: " << reply_struct.server_assigned_order_sequence_
                      << " CAOS: " << reply_struct.client_assigned_order_sequence_
                      << " CLTPOS: " << reply_struct.client_position_
                      << " SAMS: " << reply_struct.server_assigned_message_sequence_
                      << " SACI: " << reply_struct.server_assigned_client_id_
                      << " GBLPOS: " << reply_struct.global_position_ << " SIZE: " << reply_struct.size_remaining_
                      << " SE: " << reply_struct.size_executed_ << " Seq: " << reply_struct.exch_assigned_sequence_
                      << " BidLvSz: " << bid_live_ords_sz_ << " AskLvSz: " << ask_live_ords_sz_ << " REJECREASON: "
                      << HFSAT::ORSRejectionReasonStr(HFSAT::ORSRejectionReason_t(reply_struct.size_executed_))
                      << std::endl;
          }
          /* uncomment to see live orders when rejection came
          std::cout << "**********************LvOrdsBegin************************\n";
          for ( auto itr_=saos_to_live_ord_map_.begin(); itr_!=saos_to_live_ord_map_.end(); itr_++ )
          {
              std::cout << "SYM: "<< itr_->second.symbol_ << " Px: " << pp << " INTPX: "<< itr_->second.int_price_
                << " BS: " << HFSAT::GetTradeTypeChar ( itr_->second.buysell_ ) << " ST: "<< itr_->second .
          time_set_by_server_
                << " DT: " << itr_->second.client_request_time_ << " ORR: " << ToString ( itr_->second.orr_type_ )
                << " SAOS: " << itr_->second.server_assigned_order_sequence_
                << " CAOS: " << itr_->second.client_assigned_order_sequence_
                << " CLTPOS: "<< itr_->second.client_position_
                <<" SACI: "<< itr_->second.server_assigned_client_id_ <<" GBLPOS: "<< itr_->second.global_position_
                << " SIZE: " << itr_->second.size_remaining_ << " SE: " << itr_->second.size_executed_
                << " Seq: " << itr_->second.exch_assigned_sequence_ << std::endl;
          }
          std::cout << "**********************LvOrdsEnd************************\n";
          */
        } else {
          if (!seqd_conf_plottable_stats_ && !seqd_conf_summary_) {
            std::cout << "SYM: " << reply_struct.symbol_ << " Px: " << pp << " INTPX: " << reply_struct.int_price_
                      << " BS: " << HFSAT::GetTradeTypeChar(reply_struct.buysell_)
                      << " ST: " << reply_struct.time_set_by_server_ << " DT: " << reply_struct.client_request_time_
                      << " ORR: " << ToString(reply_struct.orr_type_)
                      << " SAOS: " << reply_struct.server_assigned_order_sequence_
                      << " CAOS: " << reply_struct.client_assigned_order_sequence_
                      << " CLTPOS: " << reply_struct.client_position_
                      << " SAMS: " << reply_struct.server_assigned_message_sequence_
                      << " SACI: " << reply_struct.server_assigned_client_id_
                      << " GBLPOS: " << reply_struct.global_position_ << " SIZE: " << reply_struct.size_remaining_
                      << " SE: " << reply_struct.size_executed_ << " Seq: " << reply_struct.exch_assigned_sequence_
                      << " BidLvSz: " << bid_live_ords_sz_ << " AskLvSz: " << ask_live_ords_sz_ << std::endl;

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
            std::cout << seqd_orders_map_itr_->second << "\t"
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
        std::cerr << " Stdev:" << HFSAT::VectorUtils::GetStdev(diff_vec_) << "  ";
        std::cerr << " MHQ:" << HFSAT::VectorUtils::GetMeanHighestQuartile(diff_vec_) << "  ";
        std::cerr << " MLQ:" << HFSAT::VectorUtils::GetMeanLowestQuartile(diff_vec_) << "  ";
        std::cerr << " 50%:" << diff_vec_[(diff_vec_.size()) * 1 / 2] << "  ";
        std::cerr << " 75%:" << diff_vec_[(diff_vec_.size()) * 3 / 4] << "  ";
        std::cerr << " 90%:" << diff_vec_[(diff_vec_.size()) * 9 / 10] << "  ";
        std::cerr << " 95%:" << diff_vec_[(diff_vec_.size()) * 95 / 100] << "  ";
        std::cerr << " 99%:" << diff_vec_[(diff_vec_.size()) * 99 / 100] << "  ";
        std::cerr << " Max:" << diff_vec_[diff_vec_.size() - 1] << "  ";
        std::cerr << " Min:" << diff_vec_[0] << "  \n";
      }
    }
  }

 private:
  HFSAT::GenericORSReplyStruct generic_ors_reply_struct_;
  std::string shortcode_;
  int yyyymmdd_;
  bool seqd_conf_summary_;
  bool seqd_conf_plottable_stats_;
  std::string ors_bin_filename_;
  HFSAT::ttime_t start_time_;
  HFSAT::ttime_t end_time_;
  int bid_live_ords_sz_;
  int ask_live_ords_sz_;
  std::map<int, HFSAT::GenericORSReplyStruct> saos_to_live_ord_map_;
};

int main(int argc, char** argv) {
  std::string dir = "";
  std::string shortcode_ = "";
  int yyyymmdd_ = 0;
  bool seqd_conf_summary_ = false;
  bool seqd_conf_plottable_stats_ = false;
  std::string ors_bin_filename_ = "";
  HFSAT::ttime_t start_time_ = HFSAT::ttime_t(0, 0);
  HFSAT::ttime_t end_time_ = HFSAT::ttime_t(0, 0);

  if (argc < 3) {
    std::cout << "Usage : SHORTCODE YYYYMMDD FILENAME " << std::endl;
    std::cout << "Usage : SHORTCODE YYYYMMDD SUMMARY FILENAME " << std::endl;
    std::cout << "Usage : SHORTCODE YYYYMMDD PLOTTABLE FILENAME " << std::endl;
    std::cout << "Usage : SHORTCODE YYYYMMDD FILENAME[INVALIDFILE] STARTTIME ENDTIME " << std::endl;
    std::cout << "Usage : SHORTCODE YYYYMMDD SUMMARY INVALIDFILE STARTTIME ENDTIME " << std::endl;
    std::cout << "Usage : SHORTCODE YYYYMMDD PLOTTABLE INVALIDFILE STARTTIME ENDTIME " << std::endl;
    exit(0);
  } else {
    shortcode_ = std::string(argv[1]);
    yyyymmdd_ = atoi(argv[2]);

    if (argc == 4) {
      if (!strcmp("SUMMARY", argv[3]))
        seqd_conf_summary_ = true;
      else if (!strcmp("PLOTTABLE", argv[3]))
        seqd_conf_plottable_stats_ = true;
      else if (!strcmp("INVALIDFILE", argv[3])) {
      } else
        ors_bin_filename_ = argv[3];

    } else if (argc == 5) {
      if (!strcmp("SUMMARY", argv[3]))
        seqd_conf_summary_ = true;
      else if (!strcmp("PLOTTABLE", argv[3]))
        seqd_conf_plottable_stats_ = true;
      else if (!strcmp("INVALIDFILE", argv[3])) {
      }

      ors_bin_filename_ = argv[4];

    } else if (argc >= 6) {
      if (!strcmp("SUMMARY", argv[3]))
        seqd_conf_summary_ = true;
      else if (!strcmp("PLOTTABLE", argv[3]))
        seqd_conf_plottable_stats_ = true;
      else if (!strcmp("INVALIDFILE", argv[3])) {
      } else {
        ors_bin_filename_ = argv[3];
      }
      start_time_ = HFSAT::ttime_t(
          HFSAT::DateTime::GetTimeFromTZHHMMSS(yyyymmdd_, HFSAT::DateTime::GetHHMMSSTime(argv[4] + 4), argv[4]), 0);
      end_time_ = HFSAT::ttime_t(
          HFSAT::DateTime::GetTimeFromTZHHMMSS(yyyymmdd_, HFSAT::DateTime::GetHHMMSSTime(argv[5] + 4), argv[5]), 0);
    }
  }

  ORSBinReader common_logger(shortcode_, yyyymmdd_, seqd_conf_summary_, seqd_conf_plottable_stats_, ors_bin_filename_,
                             start_time_, end_time_);

  common_logger.processMsgRecvd();
}
