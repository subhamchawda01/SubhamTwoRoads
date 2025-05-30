// =====================================================================================
//
//       Filename:  generate_newedge_audit_log.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  11/21/2012 06:51:48 AM
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
#include "dvccode/CDef/email_utils.hpp"
#include "dvccode/Utils/multicast_receiver_socket.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/CDef/ors_messages.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

#define LOGGED_DATA_PREFIX "/NAS1/data/ORSData/"

struct TradeMsg {
  double trade_price_;
  int trade_size_;
  bool isBuy;

  std::string ToString() {
    std::ostringstream t_temp_oss_;

    t_temp_oss_ << trade_price_ << trade_size_ << isBuy;

    return t_temp_oss_.str();
  }
};

class NewEdgeAuditLogger {
 private:
  std::string shortcode_;
  int yyyymmdd_;
  unsigned int total_traded_volume_;
  std::string additional_info_;
  std::string this_exch_filename_;

  std::vector<HFSAT::GenericORSReplyStruct> generic_reply_struct_events_vec_;

  std::map<unsigned int, unsigned long> saos_to_conf_map_;
  std::map<unsigned int, unsigned long> saos_to_exec_map_;

  std::map<unsigned int, bool> filter_saos_map_;
  std::map<unsigned int, bool> filter_saos_exec_no_conf_;

  std::map<unsigned int, struct TradeMsg> saos_to_tradeinfo_map_;
  std::map<unsigned int, struct TradeMsg> exch_to_tradeinfo_map_;

  std::map<unsigned int, int> conf_orders_;
  std::map<unsigned int, int> exec_orders_;
  std::map<unsigned int, int> cxld_orders_;

  std::map<unsigned int, std::string> conf_saos_to_exch_order_id_map_;
  std::map<unsigned int, std::string> cxl_saos_to_exch_order_id_map_;
  std::map<unsigned int, std::string> exec_saos_to_exch_order_id_map_;
  std::map<unsigned int, std::string> rej_saos_to_exch_order_id_map_;
  std::map<unsigned int, std::string> crej_saos_to_exch_order_id_map_;

  std::map<unsigned int, std::string> saos_to_session_map_;

  std::map<int, int> conf_saos_size_map_;
  std::map<int, int> exec_saos_size_map_;
  std::map<int, unsigned long> filter_imatch_trades_saos_;
  std::map<int, unsigned long> exec_last_timestamp_;
  std::map<int, bool> filter_exec_after_cxld_;
  std::map<int, int> saos_to_last_size_exec_;

 public:
  NewEdgeAuditLogger(std::string _shortcode_, int _yyyymmdd_, std::string extra_info_, std::string exch_trade_file_)
      : shortcode_(_shortcode_),
        yyyymmdd_(_yyyymmdd_),
        total_traded_volume_(0),
        additional_info_(extra_info_),
        this_exch_filename_(exch_trade_file_),
        generic_reply_struct_events_vec_(),
        saos_to_conf_map_(),
        saos_to_exec_map_(),
        filter_saos_map_(),
        saos_to_tradeinfo_map_(),
        conf_orders_(),
        exec_orders_(),
        cxld_orders_(),
        conf_saos_to_exch_order_id_map_(),
        cxl_saos_to_exch_order_id_map_(),
        exec_saos_to_exch_order_id_map_(),
        rej_saos_to_exch_order_id_map_(),
        crej_saos_to_exch_order_id_map_(),
        saos_to_session_map_(),
        conf_saos_size_map_(),
        exec_saos_size_map_(),
        filter_imatch_trades_saos_(),
        exec_last_timestamp_(),
        filter_exec_after_cxld_(),
        saos_to_last_size_exec_() {}

  void SendEmailAlert(std::string alert_msg_) {}

  void LoadExchangeOrderIdValues(std::string exch_filename_) {
    std::ifstream exch_order_id_file_;

    exch_order_id_file_.open(exch_filename_.c_str());

    if (!exch_order_id_file_.is_open()) {
      std::cerr << " Could Not Open Exchange Order Id file : " << exch_filename_ << "\n";

      std::ostringstream t_temp_oss_;
      t_temp_oss_ << " Could Not Open Exchange Order Id file : " << exch_filename_;

      SendEmailAlert(t_temp_oss_.str());

      exit(1);
    }

#define MAX_LINE_LENGTH 1024

    char line_buffer_[MAX_LINE_LENGTH];

    while (exch_order_id_file_.good()) {
      memset(line_buffer_, 0, MAX_LINE_LENGTH);

      exch_order_id_file_.getline(line_buffer_, MAX_LINE_LENGTH);

      HFSAT::PerishableStringTokenizer st_(line_buffer_, MAX_LINE_LENGTH);
      const std::vector<const char*>& tokens_ = st_.GetTokens();

      if (tokens_.size() != 4) {  // something wrong with line

        continue;
      }

      std::string this_order_type_ = tokens_[0];
      unsigned int this_saos_ = atoi(tokens_[1]);
      std::string this_exchange_id_ = tokens_[2];
      std::string this_session_id_ = tokens_[3];

      saos_to_session_map_[this_saos_] = this_session_id_;

      if (this_order_type_ == "CONF") {
        if (conf_saos_to_exch_order_id_map_.find(this_saos_) == conf_saos_to_exch_order_id_map_.end()) {
          conf_saos_to_exch_order_id_map_[this_saos_] = this_exchange_id_;

        } else {
          std::string this_string = conf_saos_to_exch_order_id_map_[this_saos_] + "~" + this_exchange_id_;

          conf_saos_to_exch_order_id_map_[this_saos_] = this_string;
        }

      } else if (this_order_type_ == "CXL") {
        if (cxl_saos_to_exch_order_id_map_.find(this_saos_) == cxl_saos_to_exch_order_id_map_.end()) {
          cxl_saos_to_exch_order_id_map_[this_saos_] = this_exchange_id_;

        } else {
          std::string this_string = cxl_saos_to_exch_order_id_map_[this_saos_] + "~" + this_exchange_id_;

          cxl_saos_to_exch_order_id_map_[this_saos_] = this_string;
        }

      } else if (this_order_type_ == "EXEC") {
        if (exec_saos_to_exch_order_id_map_.find(this_saos_) == exec_saos_to_exch_order_id_map_.end()) {
          exec_saos_to_exch_order_id_map_[this_saos_] = this_exchange_id_;

        } else {
          std::string this_string = exec_saos_to_exch_order_id_map_[this_saos_] + "~" + this_exchange_id_;

          exec_saos_to_exch_order_id_map_[this_saos_] = this_string;
        }

      } else if (this_order_type_ == "REJ") {
        if (rej_saos_to_exch_order_id_map_.find(this_saos_) == rej_saos_to_exch_order_id_map_.end()) {
          rej_saos_to_exch_order_id_map_[this_saos_] = this_exchange_id_;

        } else {
          std::string this_string = rej_saos_to_exch_order_id_map_[this_saos_] + "~" + this_exchange_id_;

          rej_saos_to_exch_order_id_map_[this_saos_] = this_string;
        }

      } else if (this_order_type_ == "CXLREJ") {
        if (crej_saos_to_exch_order_id_map_.find(this_saos_) == crej_saos_to_exch_order_id_map_.end()) {
          crej_saos_to_exch_order_id_map_[this_saos_] = this_exchange_id_;

        } else {
          std::string this_string = crej_saos_to_exch_order_id_map_[this_saos_] + "~" + this_exchange_id_;

          crej_saos_to_exch_order_id_map_[this_saos_] = this_string;
        }
      }
    }

    exch_order_id_file_.close();

#undef MAX_LINE_LENGTH
  }

  void GenerateAuditLog() {
    // ======================================   Construct FileName =========================================== //

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

    while (filename_to_read.find(" ") != std::string::npos) {
      filename_to_read.replace(filename_to_read.find(" "), 1, "~");
    }

    // ======================================================================================================== //

    // ======================================== Process File Events =========================================== //

    HFSAT::BulkFileReader bulk_file_reader_;
    bulk_file_reader_.open(filename_to_read.c_str());

    if (!bulk_file_reader_.is_open()) {
      std::cerr << " Could Not Open File To Read : " << filename_to_read << "\n";

      if (HFSAT::FileUtils::exists(this_exch_filename_)) {
        std::ifstream this_file_;

        this_file_.open(this_exch_filename_.c_str());

        if (!this_file_.is_open()) {
          std::cerr << " Failed Opening Exch Trade File \n";
        }

        char temp_buffer[1024];
        memset(temp_buffer, 0, 1024);

        bool should_we_have_trades_ = false;

        while (this_file_.good()) {
          this_file_.getline(temp_buffer, 1024);

          std::string copy_buffer_ = temp_buffer;

          if (copy_buffer_.find(t_exchange_symbol_) != std::string::npos) {
            should_we_have_trades_ = true;
            break;
          }
        }

        if (should_we_have_trades_) {
          std::ostringstream mail_alert_;
          mail_alert_ << " Could Not Open File To Read : " << filename_to_read;

          SendEmailAlert(mail_alert_.str());
        }
      }

      exit(1);
    }

    std::cout << "\n\n\n\n\n\n";
    std::cout << "********************************************* AUDIT LOG FOR " << t_exchange_symbol_
              << " TradingDate : " << yyyymmdd_ << " ********************************************** "
              << "\n";
    std::cout << "\n\n\n";

    while (true) {
      HFSAT::GenericORSReplyStruct reply_struct;

      memset((void*)(&reply_struct), 0, sizeof(HFSAT::GenericORSReplyStruct));

      size_t read_length_ =
          bulk_file_reader_.read(reinterpret_cast<char*>(&reply_struct), sizeof(HFSAT::GenericORSReplyStruct));

      if (read_length_ < sizeof(HFSAT::GenericORSReplyStruct)) break;

      if (reply_struct.orr_type_ == HFSAT::kORRType_Seqd || HFSAT::kORRType_None == reply_struct.orr_type_ ||
          HFSAT::kORRType_ORSConf == reply_struct.orr_type_)
        continue;

      generic_reply_struct_events_vec_.push_back(reply_struct);

      if (reply_struct.orr_type_ == HFSAT::kORRType_Rejc) {
      } else {
        if (reply_struct.orr_type_ == HFSAT::kORRType_Conf) {
          saos_to_conf_map_[reply_struct.server_assigned_order_sequence_] =
              (reply_struct.time_set_by_server_.tv_sec * 1000000 + reply_struct.time_set_by_server_.tv_usec);
          conf_saos_size_map_[reply_struct.server_assigned_order_sequence_] = reply_struct.size_remaining_;
        }

        if (reply_struct.orr_type_ == HFSAT::kORRType_Exec) {
          if (conf_saos_size_map_.find(reply_struct.server_assigned_order_sequence_) == conf_saos_size_map_.end()) {
            filter_imatch_trades_saos_[reply_struct.server_assigned_order_sequence_] =
                reply_struct.time_set_by_server_.tv_sec * 1000000 + reply_struct.time_set_by_server_.tv_usec;
          }

          saos_to_exec_map_[reply_struct.server_assigned_order_sequence_] =
              (reply_struct.time_set_by_server_.tv_sec * 1000000 + reply_struct.time_set_by_server_.tv_usec);

          exec_last_timestamp_[reply_struct.server_assigned_order_sequence_] =
              reply_struct.time_set_by_server_.tv_sec * 1000000 + reply_struct.time_set_by_server_.tv_usec;

          exec_saos_size_map_[reply_struct.server_assigned_order_sequence_] = reply_struct.size_executed_;
        }

        if (reply_struct.orr_type_ == HFSAT::kORRType_Cxld) {
          if (reply_struct.size_remaining_ == conf_saos_size_map_[reply_struct.server_assigned_order_sequence_]) {
            filter_exec_after_cxld_[reply_struct.server_assigned_order_sequence_] = true;
          }

          if (exec_saos_size_map_.find(reply_struct.server_assigned_order_sequence_) != exec_saos_size_map_.end()) {
            if (exec_saos_size_map_[reply_struct.server_assigned_order_sequence_] ==
                conf_saos_size_map_[reply_struct.server_assigned_order_sequence_])
              filter_imatch_trades_saos_[reply_struct.server_assigned_order_sequence_] =
                  exec_last_timestamp_[reply_struct.server_assigned_order_sequence_];
          }

          if ((saos_to_exec_map_.find(reply_struct.server_assigned_order_sequence_) != saos_to_exec_map_.end())) {
            // partil exec hack - when orders would be internally matched later
            if (shortcode_.find("LFI") != std::string::npos) continue;
            if (shortcode_.find("LFL") != std::string::npos) continue;

            filter_saos_map_[reply_struct.server_assigned_order_sequence_] = true;
          }
        }
      }
    }

    FilterInternalMatchAccurate();
  }

  void FilterInternalMatchAccurate() {
    unsigned int total_conf_ = 0;
    unsigned int total_cxld_ = 0;
    unsigned int total_rej_ = 0;
    unsigned int total_exec_ = 0;

    unsigned int total_trades_ = 0;

    int last_global_exec_position_ = -1000;

    for (unsigned int event_counter_ = 0; event_counter_ < generic_reply_struct_events_vec_.size(); event_counter_++) {
      bool loopback = false;

      HFSAT::GenericORSReplyStruct reply_struct = generic_reply_struct_events_vec_[event_counter_];

      switch (generic_reply_struct_events_vec_[event_counter_].orr_type_) {
        case HFSAT::kORRType_Conf: {
          total_conf_++;

        } break;

        case HFSAT::kORRType_Cxld: {
          total_cxld_++;
        } break;
        case HFSAT::kORRType_Exec: {
          if (filter_exec_after_cxld_.find(reply_struct.server_assigned_order_sequence_) !=
              filter_exec_after_cxld_.end()) {
            loopback = true;
            break;
          }

          if (filter_imatch_trades_saos_.find(reply_struct.server_assigned_order_sequence_) !=
              filter_imatch_trades_saos_.end()) {
            unsigned long this_time_ =
                reply_struct.time_set_by_server_.tv_sec * 1000000 + reply_struct.time_set_by_server_.tv_usec;

            if (filter_imatch_trades_saos_[reply_struct.server_assigned_order_sequence_] == this_time_) {
              loopback = true;
              break;
            }
          }

          int size_exec_ = 0;

          if (!reply_struct.size_executed_) {
            loopback = true;
            break;
          }

          if (saos_to_last_size_exec_.find(reply_struct.server_assigned_order_sequence_) !=
              saos_to_last_size_exec_.end()) {
            size_exec_ =
                reply_struct.size_executed_ - saos_to_last_size_exec_[reply_struct.server_assigned_order_sequence_];

          } else {
            size_exec_ = reply_struct.size_executed_;
          }

          if (!size_exec_ || size_exec_ < 0) {
            loopback = true;
            break;
          }

          if (last_global_exec_position_ == -100)
            last_global_exec_position_ = reply_struct.global_position_;
          else if (reply_struct.global_position_ == last_global_exec_position_) {
            loopback = true;
            break;
          }

          if (conf_saos_size_map_[reply_struct.server_assigned_order_sequence_] < size_exec_) {
            size_exec_ = conf_saos_size_map_[reply_struct.server_assigned_order_sequence_];
          }

          total_exec_++;

          total_trades_ += size_exec_;

          saos_to_last_size_exec_[reply_struct.server_assigned_order_sequence_] = reply_struct.size_executed_;

          struct TradeMsg this_trade_msg_;

          this_trade_msg_.trade_price_ = reply_struct.price_;
          this_trade_msg_.trade_size_ = size_exec_;
          this_trade_msg_.isBuy = (reply_struct.buysell_ == HFSAT::kTradeTypeBuy) ? true : false;

          saos_to_tradeinfo_map_[reply_struct.server_assigned_order_sequence_] = this_trade_msg_;

        } break;

        case HFSAT::kORRType_Rejc: {
          total_rej_++;
        } break;
        case HFSAT::kORRType_CxRe: {
          total_rej_++;
        } break;
        default: { } break; }

      if (loopback) continue;

      char pp[6] = {'\0'};
      sprintf(pp, "%.6f", reply_struct.price_);

      std::string this_exchange_order_id_ = "";
      bool found_exchange_order_id_ = false;

      std::string this_session_id_ = "";

      if ((reply_struct.orr_type_ == HFSAT::kORRType_Rejc || HFSAT::kORRType_CxlRejc == reply_struct.orr_type_) &&
          (HFSAT::kExchOrderReject == (HFSAT::ORSRejectionReason_t(reply_struct.size_executed_)))) {
        switch (reply_struct.orr_type_) {
          case HFSAT::kORRType_Rejc: {
            if (rej_saos_to_exch_order_id_map_.find(reply_struct.server_assigned_order_sequence_) !=
                rej_saos_to_exch_order_id_map_.end()) {
              found_exchange_order_id_ = true;

              std::string this_temp_string_ =
                  rej_saos_to_exch_order_id_map_[reply_struct.server_assigned_order_sequence_];

              this_exchange_order_id_ = this_temp_string_.substr(0, this_temp_string_.find_first_of("~"));

              this_temp_string_ = this_temp_string_.substr(this_temp_string_.find_first_of("~") + 1);

              rej_saos_to_exch_order_id_map_[reply_struct.server_assigned_order_sequence_] = this_temp_string_;
            }

          } break;

          case HFSAT::kORRType_CxRe: {
            if (crej_saos_to_exch_order_id_map_.find(reply_struct.server_assigned_order_sequence_) !=
                crej_saos_to_exch_order_id_map_.end()) {
              found_exchange_order_id_ = true;

              std::string this_temp_string_ =
                  crej_saos_to_exch_order_id_map_[reply_struct.server_assigned_order_sequence_];

              this_exchange_order_id_ = this_temp_string_.substr(0, this_temp_string_.find_first_of("~"));

              this_temp_string_ = this_temp_string_.substr(this_temp_string_.find_first_of("~") + 1);

              crej_saos_to_exch_order_id_map_[reply_struct.server_assigned_order_sequence_] = this_temp_string_;
            }

          } break;

          default: { } break; }

        if (!found_exchange_order_id_) {
          // put something to alert in mail
        }

        std::cout << additional_info_
                  << " SESSION_ID : " << saos_to_session_map_[reply_struct.server_assigned_order_sequence_]
                  << " Transaction Date : " << yyyymmdd_ << " SYM : " << reply_struct.symbol_ << " Price :" << pp
                  << " BS: " << HFSAT::GetTradeTypeChar(reply_struct.buysell_)
                  << " TIMESET : " << reply_struct.time_set_by_server_
                  << " ORRTYPE : " << ToString(reply_struct.orr_type_) << " EXCH ORDER ID : " << this_exchange_order_id_
                  << " SAOS : " << reply_struct.server_assigned_order_sequence_
                  << " GBLPOS: " << reply_struct.global_position_ << " REJECREASON: "
                  << HFSAT::ORSRejectionReasonStr(HFSAT::ORSRejectionReason_t(reply_struct.size_executed_))
                  << std::endl;

      } else {
        switch (reply_struct.orr_type_) {
          case HFSAT::kORRType_Conf: {
            if (conf_saos_to_exch_order_id_map_.find(reply_struct.server_assigned_order_sequence_) !=
                conf_saos_to_exch_order_id_map_.end()) {
              found_exchange_order_id_ = true;

              std::string this_temp_string_ =
                  conf_saos_to_exch_order_id_map_[reply_struct.server_assigned_order_sequence_];

              this_exchange_order_id_ = this_temp_string_.substr(0, this_temp_string_.find_first_of("~"));

              this_temp_string_ = this_temp_string_.substr(this_temp_string_.find_first_of("~") + 1);

              conf_saos_to_exch_order_id_map_[reply_struct.server_assigned_order_sequence_] = this_temp_string_;
            }

          } break;

          case HFSAT::kORRType_Cxld: {
            if (cxl_saos_to_exch_order_id_map_.find(reply_struct.server_assigned_order_sequence_) !=
                cxl_saos_to_exch_order_id_map_.end()) {
              found_exchange_order_id_ = true;

              std::string this_temp_string_ =
                  cxl_saos_to_exch_order_id_map_[reply_struct.server_assigned_order_sequence_];

              this_exchange_order_id_ = this_temp_string_.substr(0, this_temp_string_.find_first_of("~"));

              this_temp_string_ = this_temp_string_.substr(this_temp_string_.find_first_of("~") + 1);

              cxl_saos_to_exch_order_id_map_[reply_struct.server_assigned_order_sequence_] = this_temp_string_;
            }

          } break;

          case HFSAT::kORRType_Exec: {
            if (exec_saos_to_exch_order_id_map_.find(reply_struct.server_assigned_order_sequence_) !=
                exec_saos_to_exch_order_id_map_.end()) {
              found_exchange_order_id_ = true;

              std::string this_temp_string_ =
                  exec_saos_to_exch_order_id_map_[reply_struct.server_assigned_order_sequence_];

              this_exchange_order_id_ = this_temp_string_.substr(0, this_temp_string_.find_first_of("~"));

              this_temp_string_ = this_temp_string_.substr(this_temp_string_.find_first_of("~") + 1);

              exec_saos_to_exch_order_id_map_[reply_struct.server_assigned_order_sequence_] = this_temp_string_;
            }

          } break;

          default: { } break; }

        if (!found_exchange_order_id_) {
          // put something to alert in mail
        }

        std::cout << additional_info_
                  << " SESSION_ID : " << saos_to_session_map_[reply_struct.server_assigned_order_sequence_]
                  << " Transaction Date : " << yyyymmdd_ << " SYM : " << reply_struct.symbol_ << " Price :" << pp
                  << " BS: " << HFSAT::GetTradeTypeChar(reply_struct.buysell_)
                  << " TIMESET : " << reply_struct.time_set_by_server_
                  << " ORRTYPE : " << ToString(reply_struct.orr_type_) << " EXCH ORDER ID : " << this_exchange_order_id_
                  << " SAOS : " << reply_struct.server_assigned_order_sequence_
                  << " SIZE: " << reply_struct.size_remaining_ << " SIZE_EXEC: " << reply_struct.size_executed_
                  << std::endl;
      }
    }

    total_traded_volume_ = total_trades_;

    std::cout << "\n\n\n";
    std::cout << " Total Trades : " << total_trades_ << "\n";
    std::cout << " AUDIT LOG MSG SUMMARY : Total Conf : " << total_conf_ << " Total Cxld : " << total_cxld_
              << " Total Rejects : " << total_rej_ << " Total Exec : " << total_exec_ << "\n";
    std::cout << "*****************************************************************************************************"
                 "****************************************************************\n";
  }

  void FilterInternalMatch() {
    std::map<unsigned int, unsigned long>::iterator saos_exec_itr_ = saos_to_exec_map_.begin();

    while (saos_exec_itr_ != saos_to_exec_map_.end()) {
      unsigned long this_time_diff_ = 0;
      (void)this_time_diff_;

      unsigned int this_saos_ = saos_exec_itr_->first;

      if (saos_to_conf_map_.find(this_saos_) == saos_to_conf_map_.end()) {
        filter_saos_exec_no_conf_[this_saos_] = true;

      } else {
        this_time_diff_ = (saos_exec_itr_->second) - saos_to_conf_map_[this_saos_];
      }

      saos_exec_itr_++;
    }

    unsigned int total_conf_ = 0;
    unsigned int total_cxld_ = 0;
    unsigned int total_rej_ = 0;
    unsigned int total_exec_ = 0;

    unsigned int total_trades_ = 0;

    for (unsigned int event_counter_ = 0; event_counter_ < generic_reply_struct_events_vec_.size(); event_counter_++) {
      if (filter_saos_exec_no_conf_.find(
              generic_reply_struct_events_vec_[event_counter_].server_assigned_order_sequence_) !=
          filter_saos_exec_no_conf_.end()) {
        continue;
      }

      if ((generic_reply_struct_events_vec_[event_counter_].orr_type_ == HFSAT::kORRType_Exec) &&
          (filter_saos_map_.find(generic_reply_struct_events_vec_[event_counter_].server_assigned_order_sequence_) !=
           filter_saos_map_.end())) {
        continue;
      }

      HFSAT::GenericORSReplyStruct reply_struct = generic_reply_struct_events_vec_[event_counter_];

      bool is_already_seen_ = false;

      switch (generic_reply_struct_events_vec_[event_counter_].orr_type_) {
        case HFSAT::kORRType_Conf: {
          if (conf_orders_.find(reply_struct.server_assigned_order_sequence_) != conf_orders_.end()) {
            is_already_seen_ = true;
            break;
          }

          total_conf_++;

          conf_orders_[reply_struct.server_assigned_order_sequence_] = reply_struct.size_remaining_;

        } break;

        case HFSAT::kORRType_Cxld: {
          total_cxld_++;
        } break;
        case HFSAT::kORRType_Exec: {
          if (exec_orders_.find(reply_struct.server_assigned_order_sequence_) != conf_orders_.end()) {
            if (exec_orders_[reply_struct.server_assigned_order_sequence_] >=
                conf_orders_[reply_struct.server_assigned_order_sequence_]) {
              is_already_seen_ = true;
              break;
            }
          }

          if (exec_orders_.find(reply_struct.server_assigned_order_sequence_) == exec_orders_.end()) {
            total_trades_ += reply_struct.size_executed_;

          } else {
            total_trades_ -= exec_orders_[reply_struct.server_assigned_order_sequence_];
            total_trades_ += reply_struct.size_executed_;
          }

          exec_orders_[reply_struct.server_assigned_order_sequence_] = reply_struct.size_executed_;

          total_exec_++;

          struct TradeMsg this_trade_msg_;

          this_trade_msg_.trade_price_ = reply_struct.price_;
          this_trade_msg_.trade_size_ = reply_struct.size_executed_;
          this_trade_msg_.isBuy = (reply_struct.buysell_ == HFSAT::kTradeTypeBuy) ? true : false;

          saos_to_tradeinfo_map_[reply_struct.server_assigned_order_sequence_] = this_trade_msg_;

        } break;

        case HFSAT::kORRType_Rejc: {
          total_rej_++;
        } break;
        case HFSAT::kORRType_CxRe: {
          total_rej_++;
        } break;
        default: { } break; }

      if (is_already_seen_) continue;

      char pp[6] = {'\0'};
      sprintf(pp, "%.6f", reply_struct.price_);

      std::string this_exchange_order_id_ = "";
      bool found_exchange_order_id_ = false;

      if ((reply_struct.orr_type_ == HFSAT::kORRType_Rejc || HFSAT::kORRType_CxlRejc == reply_struct.orr_type_) &&
          (HFSAT::kExchOrderReject == (HFSAT::ORSRejectionReason_t(reply_struct.size_executed_)))) {
        switch (reply_struct.orr_type_) {
          case HFSAT::kORRType_Rejc: {
            if (rej_saos_to_exch_order_id_map_.find(reply_struct.server_assigned_order_sequence_) !=
                rej_saos_to_exch_order_id_map_.end()) {
              found_exchange_order_id_ = true;

              this_exchange_order_id_ = rej_saos_to_exch_order_id_map_[reply_struct.server_assigned_order_sequence_];
            }

          } break;

          case HFSAT::kORRType_CxRe: {
            if (crej_saos_to_exch_order_id_map_.find(reply_struct.server_assigned_order_sequence_) !=
                crej_saos_to_exch_order_id_map_.end()) {
              found_exchange_order_id_ = true;

              this_exchange_order_id_ = crej_saos_to_exch_order_id_map_[reply_struct.server_assigned_order_sequence_];
            }

          } break;

          default: { } break; }

        if (!found_exchange_order_id_) {
          // put something to alert in mail
        }

        std::cout << additional_info_ << " Transaction Date : " << yyyymmdd_ << " SYM : " << reply_struct.symbol_
                  << " Price :" << pp << " BS: " << HFSAT::GetTradeTypeChar(reply_struct.buysell_)
                  << " TIMESET : " << reply_struct.time_set_by_server_
                  << " ORRTYPE : " << ToString(reply_struct.orr_type_) << " EXCH ORDER ID : " << this_exchange_order_id_
                  << " SAOS : " << reply_struct.server_assigned_order_sequence_
                  << " GBLPOS: " << reply_struct.global_position_ << " REJECREASON: "
                  << HFSAT::ORSRejectionReasonStr(HFSAT::ORSRejectionReason_t(reply_struct.size_executed_))
                  << std::endl;

      } else {
        switch (reply_struct.orr_type_) {
          case HFSAT::kORRType_Conf: {
            if (conf_saos_to_exch_order_id_map_.find(reply_struct.server_assigned_order_sequence_) !=
                conf_saos_to_exch_order_id_map_.end()) {
              found_exchange_order_id_ = true;

              this_exchange_order_id_ = conf_saos_to_exch_order_id_map_[reply_struct.server_assigned_order_sequence_];
            }

          } break;

          case HFSAT::kORRType_Cxld: {
            if (cxl_saos_to_exch_order_id_map_.find(reply_struct.server_assigned_order_sequence_) !=
                cxl_saos_to_exch_order_id_map_.end()) {
              found_exchange_order_id_ = true;

              this_exchange_order_id_ = cxl_saos_to_exch_order_id_map_[reply_struct.server_assigned_order_sequence_];
            }

          } break;

          case HFSAT::kORRType_Exec: {
            if (exec_saos_to_exch_order_id_map_.find(reply_struct.server_assigned_order_sequence_) !=
                exec_saos_to_exch_order_id_map_.end()) {
              found_exchange_order_id_ = true;

              this_exchange_order_id_ = exec_saos_to_exch_order_id_map_[reply_struct.server_assigned_order_sequence_];
            }

          } break;

          default: { } break; }

        if (!found_exchange_order_id_) {
          // put something to alert in mail
        }

        std::cout << additional_info_ << " Transaction Date : " << yyyymmdd_ << " SYM : " << reply_struct.symbol_
                  << " Price :" << pp << " BS: " << HFSAT::GetTradeTypeChar(reply_struct.buysell_)
                  << " TIMESET : " << reply_struct.time_set_by_server_
                  << " ORRTYPE : " << ToString(reply_struct.orr_type_) << " EXCH ORDER ID : " << this_exchange_order_id_
                  << " SAOS : " << reply_struct.server_assigned_order_sequence_
                  << " SIZE: " << reply_struct.size_remaining_ << " SIZE_EXEC: " << reply_struct.size_executed_
                  << std::endl;
      }
    }

    total_traded_volume_ = total_trades_;

    std::cout << "\n\n\n";
    std::cout << " Total Trades : " << total_trades_ << "\n";
    std::cout << " AUDIT LOG MSG SUMMARY : Total Conf : " << total_conf_ << " Total Cxld : " << total_cxld_
              << " Total Rejects : " << total_rej_ << " Total Exec : " << total_exec_ << "\n";
    std::cout << "*****************************************************************************************************"
                 "****************************************************************\n";
  }

  void ReconcileTradesSaos(std::string exch_trades_filename_) {
    std::ifstream exchange_trades_file_;

    exchange_trades_file_.open(exch_trades_filename_.c_str());

    if (!exchange_trades_file_.is_open()) {
      std::cerr << " Coudl Not Opne Exchange Trades File : " << exch_trades_filename_ << "\n";
    }

#define MAX_LINE_LENGTH 1024

    char line_buffer_[MAX_LINE_LENGTH];

    bool send_alert_ = false;

    std::ostringstream t_temp_oss_;

    t_temp_oss_ << " Shortcode : " << shortcode_ << " Date : " << yyyymmdd_ << "</br>";

    unsigned int total_exchange_volume_ = 0;

    while (exchange_trades_file_.good()) {
      memset(line_buffer_, 0, MAX_LINE_LENGTH);
      exchange_trades_file_.getline(line_buffer_, MAX_LINE_LENGTH);

      HFSAT::PerishableStringTokenizer st_(line_buffer_, MAX_LINE_LENGTH);
      const std::vector<const char*>& tokens_ = st_.GetTokens();

      if (tokens_.size() != 4) continue;

      unsigned int exch_saos_ = atoi(tokens_[0]);
      double exch_price_ = atof(tokens_[1]);
      int size_ = atoi(tokens_[2]);
      bool isBuy = (atoi(tokens_[3]) == 0) ? true : false;

      total_exchange_volume_ += size_;

      if (saos_to_tradeinfo_map_.find(exch_saos_) == saos_to_tradeinfo_map_.end()) {
        send_alert_ = true;
        t_temp_oss_ << " No Matching Trade For SAOS : " << exch_saos_ << " Price : " << exch_price_
                    << " Size : " << size_ << " Side : " << isBuy << "</br>";
        continue;
      }

      struct TradeMsg our_trade_info_ = saos_to_tradeinfo_map_[exch_saos_];

      std::string our_string_trade_info_ = our_trade_info_.ToString();

      std::ostringstream exch_string_trade_info_;
      exch_string_trade_info_ << exch_price_ << size_ << isBuy;

      // dont want to compare float with precision here
      if (exch_string_trade_info_.str() != our_string_trade_info_) {
        t_temp_oss_ << " Discrepancy Matching Exch Trade : " << exch_saos_ << " Price : " << exch_price_
                    << " Size : " << size_ << " Side : " << isBuy << " Our Price : " << our_trade_info_.trade_price_
                    << " Size : " << our_trade_info_.trade_size_ << " Side : " << our_trade_info_.isBuy << "</br>";
      }
    }

    exchange_trades_file_.close();

#undef MAX_LINE_LENGTH

    if (total_traded_volume_ != total_exchange_volume_) {
      send_alert_ = true;
      t_temp_oss_ << " Our Trade Volume : " << total_traded_volume_
                  << " Differs From Exchange : " << total_exchange_volume_ << "\n";
    }

    if (send_alert_) {
      std::cerr << t_temp_oss_.str() << "\n";
      SendEmailAlert(t_temp_oss_.str());
    }
  }
};

int main(int argc, char** argv) {
  std::string shortcode_ = "";
  int yyyymmdd_ = 0;

  if (argc < 5) {
    std::cout << "Usage : SHORTCODE YYYYMMDD EXCHSEQFILE EXCHTRADESFILE EXTRAINFO " << std::endl;
    exit(1);
  }

  shortcode_ = argv[1];
  yyyymmdd_ = atoi(argv[2]);
  std::string exchange_order_id_file_ = argv[3];
  std::string exchange_trades_file_ = argv[4];

  std::ostringstream t_temp_oss_;

  for (int i = 5; i < argc; i++) {
    t_temp_oss_ << argv[i] << " ";
  }

  std::string additional_info_ = t_temp_oss_.str();

  NewEdgeAuditLogger newedge_audit_logger_(shortcode_, yyyymmdd_, additional_info_, exchange_trades_file_);

  newedge_audit_logger_.LoadExchangeOrderIdValues(exchange_order_id_file_);
  newedge_audit_logger_.GenerateAuditLog();
  newedge_audit_logger_.ReconcileTradesSaos(exchange_trades_file_);

  return 0;
}
