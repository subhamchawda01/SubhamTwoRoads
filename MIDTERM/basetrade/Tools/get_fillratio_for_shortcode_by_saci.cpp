// =====================================================================================
//
//       Filename:  get_fillratio_for_shortcode_by_query.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  12/12/2012 05:58:39 AM
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

#define LOGGED_DATA_PREFIX "/NAS1/data/ORSData/"

class ORSBinReader {
 public:
  ORSBinReader(std::string _shortcode_, int _yyyymmdd_, int _this_saci_)
      : shortcode_(_shortcode_), yyyymmdd_(_yyyymmdd_), this_saci_(_this_saci_) {}

  void GetFillRatioForSACI() {
    std::map<int, HFSAT::ttime_t> seqd_orders_map_;
    std::map<int, HFSAT::ttime_t> conf_orders_map_;

    HFSAT::ExchangeSymbolManager::SetUniqueInstance(yyyymmdd_);
    const char* t_exchange_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(shortcode_);
    std::string location_ =
        HFSAT::TradingLocationUtils::GetTradingLocationName(HFSAT::TradingLocationUtils::GetTradingLocationExch(
            HFSAT::SecurityDefinitions::GetContractExchSource(shortcode_, yyyymmdd_)));

    std::ostringstream t_temp_oss_;
    t_temp_oss_ << yyyymmdd_;
    std::string date_ = t_temp_oss_.str();

    std::stringstream ff;
    ff << LOGGED_DATA_PREFIX << location_ << "/" << date_.substr(0, 4) << "/" << date_.substr(4, 2) << "/"
       << date_.substr(6, 2) << "/" << t_exchange_symbol_ << "_" << yyyymmdd_;

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

        if (reply_struct.orr_type_ == HFSAT::kORRType_Conf) {
          int conf_size_ = reply_struct.size_remaining_;

          int saci_ = reply_struct.server_assigned_client_id_;

          if (saci_to_order_conf_flow_.find(saci_) != saci_to_order_conf_flow_.end()) {
            saci_to_order_conf_flow_[saci_] += conf_size_;

            saci_to_total_conf_count_[saci_]++;

          } else {
            saci_to_order_conf_flow_[saci_] = conf_size_;
            saci_to_total_conf_count_[saci_] = 1;
          }

        } else if (reply_struct.orr_type_ == HFSAT::kORRType_Exec) {
          int saci_ = reply_struct.server_assigned_client_id_;

          if (saci_to_client_exec_.find(saci_) != saci_to_client_exec_.end()) {
            if (saos_to_exec_size_.find(reply_struct.server_assigned_order_sequence_) != saos_to_exec_size_.end()) {
              int prev_size_ = saos_to_exec_size_[reply_struct.server_assigned_order_sequence_];

              saci_to_client_exec_[saci_] -= prev_size_;
            }

            saci_to_client_exec_[saci_] += reply_struct.size_executed_;
            saci_to_total_exec_count_[saci_]++;

          } else {
            saci_to_client_exec_[saci_] = reply_struct.size_executed_;
            saci_to_total_exec_count_[saci_] = 1;
          }

          saos_to_exec_size_[reply_struct.server_assigned_order_sequence_] = reply_struct.size_executed_;
        }
      }

      bulk_file_reader_.close();
    }

    if ((saci_to_total_conf_count_.find(this_saci_) != saci_to_total_conf_count_.end()) &&
        (saci_to_total_exec_count_.find(this_saci_) != saci_to_total_exec_count_.end())) {
      printf("%lf\n", (saci_to_client_exec_[this_saci_] / (double)(saci_to_order_conf_flow_[this_saci_]) * 100));

    }

    else if (this_saci_ == -1) {  // hack to get all fill ratio list

      std::map<int, int>::iterator saci_to_total_conf_count_itr_ = saci_to_total_conf_count_.begin();

      while (saci_to_total_conf_count_itr_ != saci_to_total_conf_count_.end()) {
        printf(
            "SACI : %d Total No Of Conf : %d Conf Size : %d Total No Of Exec : %d Exec Size : %d Fill Ratio :  %lf\n",
            saci_to_total_conf_count_itr_->first, saci_to_total_conf_count_itr_->second,
            saci_to_order_conf_flow_[saci_to_total_conf_count_itr_->first],
            saci_to_total_exec_count_[saci_to_total_conf_count_itr_->first],
            saci_to_client_exec_[saci_to_total_conf_count_itr_->first],
            (saci_to_client_exec_[saci_to_total_conf_count_itr_->first] /
             (double)(saci_to_order_conf_flow_[saci_to_total_conf_count_itr_->first])) *
                100);

        //        std::cout << " SACI : " << saci_to_total_conf_count_itr_ -> first << " Total No Of Conf : " <<
        //        saci_to_total_conf_count_itr_ -> second << " Conf Size : " << saci_to_order_conf_flow_ [
        //        saci_to_total_conf_count_itr_ -> first ] << " Total No Of Exec : " << saci_to_total_exec_count_ [
        //        saci_to_total_conf_count_itr_ -> first ] << " Exec Size : " << saci_to_client_exec_ [
        //        saci_to_total_conf_count_itr_ -> first ] << " Fill Ratio : " << ( saci_to_total_exec_count_ [
        //        saci_to_total_conf_count_itr_ -> first ] / ( double ) ( saci_to_total_conf_count_itr_ -> first ) ) <<
        //        "\n" ;

        saci_to_total_conf_count_itr_++;
      }

    } else {
      std::cout << "0.0";
    }
  }

 private:
  HFSAT::GenericORSReplyStruct generic_ors_reply_struct_;
  std::string shortcode_;
  int yyyymmdd_;
  int this_saci_;

  std::map<int, int> saci_to_total_conf_count_;
  std::map<int, int> saci_to_total_exec_count_;
  std::map<int, int> saci_to_order_conf_flow_;
  std::map<int, int> saci_to_client_exec_;
  std::map<int, int> saos_to_exec_size_;
};

int main(int argc, char** argv) {
  std::string shortcode_ = "";
  int yyyymmdd_ = 0;
  int saci_ = 0;

  if (argc < 4) {
    std::cout << "Usage : SHORTCODE YYYYMMDD SACI " << std::endl;
    exit(0);
  } else {
    shortcode_ = std::string(argv[1]);
    yyyymmdd_ = atoi(argv[2]);
    saci_ = atoi(argv[3]);
  }

  ORSBinReader this_bin_reader_(shortcode_, yyyymmdd_, saci_);

  this_bin_reader_.GetFillRatioForSACI();

  return 0;
}
