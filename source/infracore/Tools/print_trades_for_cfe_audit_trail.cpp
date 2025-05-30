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
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/CDef/ttime.hpp"
#include "infracore/CFET/cmi_branch_sequence_generator.hpp"
#include <ctime>

#define LOGGED_DATA_PREFIX "/NAS1/data/ORSData/"

// This class will read from the ORSBInary Log file
class ORSBinReader {
 public:
  ORSBinReader(std::string _shortcode_, int _yyyymmdd_, std::string _ors_bin_file_, HFSAT::ttime_t _start_time_,
               HFSAT::ttime_t _end_time_, bool order)
      : shortcode_(_shortcode_),
        yyyymmdd_(_yyyymmdd_),
        ors_bin_filename_(_ors_bin_file_),
        start_time_(_start_time_),
        order(order),
        end_time_(_end_time_) {}

  /// Reader
  void processMsgRecvd() {
    std::map<int, HFSAT::ttime_t> seqd_orders_map_;
    std::map<int, HFSAT::ttime_t> conf_orders_map_;
    std::map<int, std::vector<HFSAT::GenericORSReplyStruct> > saos_req_map;
    int day, month, year;

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

        if (order) {
          if (symbol_to_expiry_date.find(reply_struct.symbol_) == symbol_to_expiry_date.end()) {
            std::cout << "Expiry date missing for " << reply_struct.symbol_ << std::endl;
            std::exit(0);
          }

          // incase of ors reject we need to remove the message from clients which caused that ors reject
          if (reply_struct.orr_type_ == HFSAT::kORRType_Rejc) {
            HFSAT::ORSRejectionReason_t rej_reason = HFSAT::ORSRejectionReason_t(reply_struct.size_executed_);
            if (rej_reason == HFSAT::kORSRejectFOKSendFailed || rej_reason == HFSAT::kORSRejectSelfTradeCheck) {
            } else if (rej_reason != HFSAT::kExchCancelReject && (rej_reason != HFSAT::kExchOrderReject || rej_reason != HFSAT::kExchDataEntryOrderReject)) {
              int remove_req = -1;
              for (int i = saos_req_map[reply_struct.server_assigned_order_sequence_].size() - 1; i >= 0; i--) {
                if (saos_req_map[reply_struct.server_assigned_order_sequence_][i].orr_type_ == HFSAT::kORRType_Seqd) {
                  remove_req = i;
                  break;
                }
              }
              if (remove_req == -1) {
                //    std::cout << "NOT POSSIBLE. SOMETHING WRONG " << reply_struct.server_assigned_order_sequence_
                //              << std::endl;
                //    std::exit(0);
              } else {
                saos_req_map[reply_struct.server_assigned_order_sequence_].erase(
                    saos_req_map[reply_struct.server_assigned_order_sequence_].begin() + remove_req);
              }
            } else {
              saos_req_map[reply_struct.server_assigned_order_sequence_].push_back(reply_struct);
            }
          } else if (reply_struct.orr_type_ == HFSAT::kORRType_CxlRejc) {
            HFSAT::CxlRejectReason_t rej_reason = HFSAT::CxlRejectReason_t(reply_struct.size_executed_);
            if (rej_reason != HFSAT::kCxlRejectReasonTooLate) {
              int remove_req = -1;
              for (int i = saos_req_map[reply_struct.server_assigned_order_sequence_].size() - 1; i >= 0; i--) {
                if (saos_req_map[reply_struct.server_assigned_order_sequence_][i].orr_type_ ==
                    HFSAT::kORRType_CxlSeqd) {
                  remove_req = i;
                  break;
                }
              }
              if (remove_req == -1) {
                //   std::cout << "NOT POSSIBLE. SOMETHING WRONG" << reply_struct.server_assigned_order_sequence_
                //             << std::endl;
                //   std::exit(0);
              } else {
                saos_req_map[reply_struct.server_assigned_order_sequence_].erase(
                    saos_req_map[reply_struct.server_assigned_order_sequence_].begin() + remove_req);
              }
            } else {
              saos_req_map[reply_struct.server_assigned_order_sequence_].push_back(reply_struct);
            }
          } else if (reply_struct.orr_type_ != HFSAT::kORRType_Exec) {
            if (saos_req_map.find(reply_struct.server_assigned_order_sequence_) == saos_req_map.end()) {
              std::vector<HFSAT::GenericORSReplyStruct> temp;
              saos_req_map[reply_struct.server_assigned_order_sequence_] = temp;
            }
            if (reply_struct.time_set_by_server_.tv_sec != 0)
              saos_req_map[reply_struct.server_assigned_order_sequence_].push_back(reply_struct);
          }
        } else {
          if (reply_struct.orr_type_ == HFSAT::kORRType_Exec) {
            if (saos_req_map.find(reply_struct.server_assigned_order_sequence_) == saos_req_map.end()) {
              std::vector<HFSAT::GenericORSReplyStruct> temp;
              saos_req_map[reply_struct.server_assigned_order_sequence_] = temp;
            }
            saos_req_map[reply_struct.server_assigned_order_sequence_].push_back(reply_struct);
          }
        }
      }

      if (order) {
        for (auto const& reply_struct_vector : saos_req_map) {
          for (int i = 0; i < reply_struct_vector.second.size(); i++) {
            HFSAT::GenericORSReplyStruct reply_struct = reply_struct_vector.second[i];
            char pp[6] = {'\0'};
            sprintf(pp, "%.6f", reply_struct.price_);

            time_t now = reply_struct.time_set_by_server_.tv_sec - 5 * 60 * 60;  // converting to est
            tm* ltm = gmtime(&now);
            int msecs = reply_struct.time_set_by_server_.tv_usec / 1000;

            // ORDER_ENTRY_DATE -> The order which caused this event to occur
            if (reply_struct.orr_type_ == HFSAT::kORRType_Cxld) {
              for (int j = i; j >= 0; j--)
                if (reply_struct_vector.second[j].orr_type_ == HFSAT::kORRType_CxlSeqd) {
                  now = reply_struct_vector.second[j].time_set_by_server_.tv_sec - 5 * 60 * 60;
                  ltm = gmtime(&now);
                  msecs = reply_struct_vector.second[j].time_set_by_server_.tv_usec / 1000;
                  break;
                }
            }

            else if (reply_struct.orr_type_ == HFSAT::kORRType_Conf) {
              for (int j = i; j >= 0; j--)
                if (reply_struct_vector.second[j].orr_type_ == HFSAT::kORRType_Seqd) {
                  now = reply_struct_vector.second[j].time_set_by_server_.tv_sec - 5 * 60 * 60;
                  ltm = gmtime(&now);
                  msecs = reply_struct_vector.second[j].time_set_by_server_.tv_usec / 1000;
                  break;
                }
            }

            year = 1900 + ltm->tm_year;
            month = 1 + ltm->tm_mon;
            day = ltm->tm_mday;
            std::cout << year;
            if (month < 10) std::cout << "0";
            std::cout << month;
            if (day < 10) std::cout << "0";
            std::cout << day << ", ";

            // ORDER_ENTRY_TIME
            if (ltm->tm_hour < 10) std::cout << "0";
            std::cout << ltm->tm_hour << ":";
            if (ltm->tm_min < 10) std::cout << "0";
            std::cout << ltm->tm_min << ":";
            if (ltm->tm_sec < 10) std::cout << "0";
            std::cout << ltm->tm_sec << ":";
            if (msecs < 100) std::cout << "0";
            if (msecs < 10) std::cout << "0";
            std::cout << msecs << ", ";

            // PRODUCT_TYPE
            if (reply_struct.symbol_[4] == '_')
              std::cout << "S, ";  // SPREAD -> STRATEGY
            else
              std::cout << "F, ";  // FUTURE

            // RECORD_TYPE
            if (reply_struct.orr_type_ == HFSAT::kORRType_Seqd || reply_struct.orr_type_ == HFSAT::kORRType_Conf ||
                reply_struct.orr_type_ == HFSAT::kORRType_Rejc)
              std::cout << "O, ";

            else
              std::cout << "C, ";

            // EVENT_TYPE
            if (reply_struct.orr_type_ == HFSAT::kORRType_Seqd)
              std::cout << "W, ";  // WAITING
            else if (reply_struct.orr_type_ == HFSAT::kORRType_Conf)
              std::cout << "N, ";  // NEW
            else if (reply_struct.orr_type_ == HFSAT::kORRType_CxlSeqd)
              std::cout << "R, ";  // CANCEL_REQUEST
            else if (reply_struct.orr_type_ == HFSAT::kORRType_Cxld)
              std::cout << "C, ";  // CANCELLED
            else if (reply_struct.orr_type_ == HFSAT::kORRType_CxlRejc)
              std::cout << "T, ";  // TOO LATE TO CANCEL

            // EVENT_DATE
            now = reply_struct.time_set_by_server_.tv_sec - 5 * 60 * 60;  // converting to est
            ltm = gmtime(&now);
            year = 1900 + ltm->tm_year;
            month = 1 + ltm->tm_mon;
            day = ltm->tm_mday;
            std::cout << year;
            if (month < 10) std::cout << "0";
            std::cout << month;
            if (day < 10) std::cout << "0";
            std::cout << day << ", ";

            // EVENT_TIME
            if (ltm->tm_hour < 10) std::cout << "0";
            std::cout << ltm->tm_hour << ":";
            if (ltm->tm_min < 10) std::cout << "0";
            std::cout << ltm->tm_min << ":";
            if (ltm->tm_sec < 10) std::cout << "0";
            std::cout << ltm->tm_sec << ":";
            msecs = reply_struct.time_set_by_server_.tv_usec / 1000;
            if (msecs < 100) std::cout << "0";
            if (msecs < 10) std::cout << "0";
            std::cout << msecs << ", ";

            // CMPLX_SMPL_INDICATOR
            if (reply_struct.symbol_[4] == '_')
              std::cout << "1, ";  // SPREAD -> COMPLEX
            else
              std::cout << "0, ";  // FUTURE -> SIMPLE

            // ORDER_ID
            std::cout << ", ";

            // MASTER_ORDER_ID
            std::cout << ", ";

            // FIRM_BRANCH_CODE
            char branch_code_[10];
            memset(branch_code_, '\0', 10);

            uint16_t branch_sequence;
            HFSAT::CFET::BranchSequenceGenerator& branch_seqence_generator_ =
                HFSAT::CFET::BranchSequenceGenerator::GetUniqueInstance();
            for (int k = 1; k < reply_struct.server_assigned_order_sequence_; k += 100) {
              branch_seqence_generator_.GetNextBranchSequenceAndCodeFromSaos(k, branch_code_, branch_sequence);
            }
            branch_seqence_generator_.GetNextBranchSequenceAndCodeFromSaos(reply_struct.server_assigned_order_sequence_,
                                                                           branch_code_, branch_sequence);
            std::cout << branch_code_ << ", ";

            // BRANCH_SEQ_NUMBER
            std::cout << branch_sequence << ", ";

            // TRADER_ACRONYM
            std::cout << ", ";

            // TRADER_ROLE_CODE
            std::cout << ", ";

            // USER_ID
            std::cout << "DV01, ";

            // ACCOUNT_CODE
            std::cout << "DVC15038200, ";

            // ACCOUNT_ORIGIN
            std::cout << "O, ";

            // EXECUTING_FIRM_CODE
            std::cout << "762, ";

            // CMTA_FIRM_CODE
            std::cout << "DV01, ";

            // CORES_FIRM_CODE
            std::cout << ", ";

            // EXCHANGE
            std::cout << "CBOE, ";

            // ORIG_ORDER_QUANTITY
            std::cout << reply_struct_vector.second[0].size_remaining_ << ", ";

            // BUY_SELL_CODE
            std::cout << HFSAT::GetTradeTypeChar(reply_struct.buysell_) << ", ";

            // ORDER_PRICE
            std::cout << pp << ", ";

            // CABINET_INDICATOR
            std::cout << ", ";

            // UNDERLYING_SYMBOL
            std::cout << ", ";

            // SYMBOL
            std::cout << reply_struct.symbol_ << ", ";

            // PUT_CALL_CODE
            std::cout << ", ";

            // EXPIRATION_DATE
            std::cout << symbol_to_expiry_date[reply_struct.symbol_] << ", ";

            // EXERCISE_PRICE
            std::cout << ", ";

            // TIME_IN_FORCE_INDICATOR
            std::cout << "D, ";

            // OPEN_CLOSE_INDICATOR
            std::cout << ", ";

            // TRADE_QUANTITY
            std::cout << ", ";

            // TRADE_ID
            std::cout << ", ";

            // BUST_QUANTITY
            std::cout << ", ";

            // CANCELLED_QUANTITY
            if (reply_struct.orr_type_ == HFSAT::kORRType_Cxld) {
              for (int j = i; j >= 0; j--)
                if (reply_struct_vector.second[j].orr_type_ == HFSAT::kORRType_CxlSeqd) {
                  std::cout << reply_struct_vector.second[j].size_remaining_;
                  break;
                }
            }

            std::cout << ", ";

            // REQUESTED_CANCEL_QUANTITY
            if (reply_struct.orr_type_ == HFSAT::kORRType_CxlSeqd) std::cout << reply_struct.size_remaining_;

            std::cout << ", ";

            // REMAINING_QUANTITY
            std::cout << ", ";

            // MPID
            std::cout << ", ";

            // CREATE_REDEEM_TYPE
            std::cout << ", ";

            // AGGREGATE_UNIT
            std::cout << ", ";

            // EXER_ASGN
            std::cout << ", ";

            // CANCEL_ORDER_ID
            std::cout << ", ";

            // REPLACE_ORDER_ID
            std::cout << ", ";

            // CRD_NUMBER
            std::cout << ", ";

            // OPT_DATA
            std::cout << ", ";

            // CONTINGENCY_TYPE
            std::cout << ", ";

            // LOCATE
            std::cout << ", ";

            // TRANS_DRCTN
            if (reply_struct.orr_type_ == HFSAT::kORRType_Conf || reply_struct.orr_type_ == HFSAT::kORRType_CxlRejc ||
                reply_struct.orr_type_ == HFSAT::kORRType_Cxld || reply_struct.orr_type_ == HFSAT::kORRType_Rejc)
              std::cout << "FROM, ";
            else
              std::cout << "TO, ";

            // TRANS_SITE
            if (reply_struct.orr_type_ == HFSAT::kORRType_Conf || reply_struct.orr_type_ == HFSAT::kORRType_CxlRejc ||
                reply_struct.orr_type_ == HFSAT::kORRType_Cxld || reply_struct.orr_type_ == HFSAT::kORRType_Rejc)
              std::cout << "EXCH, ";
            else
              std::cout << "CUST, ";

            // ORDER_TYPE_VAL
            std::cout << ", ";

            // TIED_TO
            std::cout << ", ";

            // EXEC_VENUE
            std::cout << ", ";

            // TRADE_PRICE
            std::cout << ", ";

            std::cout << std::endl;
          }
        }
      }

      else {
        for (auto const& reply_struct_vector : saos_req_map) {
          for (int i = 0; i < reply_struct_vector.second.size(); i++) {
            HFSAT::GenericORSReplyStruct reply_struct = reply_struct_vector.second[i];
            char pp[6] = {'\0'};
            sprintf(pp, "%.6f", reply_struct.price_);

            char branch_code_[10];
            memset(branch_code_, '\0', 10);

            uint16_t branch_sequence;
            HFSAT::CFET::BranchSequenceGenerator& branch_seqence_generator_ =
                HFSAT::CFET::BranchSequenceGenerator::GetUniqueInstance();
            for (int k = 1; k < reply_struct.server_assigned_order_sequence_; k += 100) {
              branch_seqence_generator_.GetNextBranchSequenceAndCodeFromSaos(k, branch_code_, branch_sequence);
            }
            branch_seqence_generator_.GetNextBranchSequenceAndCodeFromSaos(reply_struct.server_assigned_order_sequence_,
                                                                           branch_code_, branch_sequence);

            std::string branch_seq_key = std::string(branch_code_);
            branch_seq_key = branch_seq_key + std::to_string(branch_sequence);

            time_t now = reply_struct.time_set_by_server_.tv_sec - 5 * 60 * 60;  // converting to est
            tm* ltm = gmtime(&now);
            int msecs = reply_struct.time_set_by_server_.tv_usec / 1000;

            // TRADE_DATE
            year = 1900 + ltm->tm_year;
            month = 1 + ltm->tm_mon;
            day = ltm->tm_mday;
            std::cout << year;
            if (month < 10) std::cout << "0";
            std::cout << month;
            if (day < 10) std::cout << "0";
            std::cout << day << ", ";

            // TRADE_TIME
            if (ltm->tm_hour < 10) std::cout << "0";
            std::cout << ltm->tm_hour << ":";
            if (ltm->tm_min < 10) std::cout << "0";
            std::cout << ltm->tm_min << ":";
            if (ltm->tm_sec < 10) std::cout << "0";
            std::cout << ltm->tm_sec << ":";
            if (msecs < 100) std::cout << "0";
            if (msecs < 10) std::cout << "0";
            std::cout << msecs << ", ";

            // PRODUCT_TYPE
            if (reply_struct.symbol_[4] == '_')
              std::cout << "S, ";  // SPREAD -> STRATEGY
            else
              std::cout << "F, ";  // FUTURE

            // TRADE_TYPE
            std::cout << "REG, ";

            // TRADE_ID
            if (branchseq_to_tradeid.find(branch_seq_key) != branchseq_to_tradeid.end() &&
                branchseq_to_tradeid[branch_seq_key].size() != 0) {
              std::cout << branchseq_to_tradeid[branch_seq_key][0];
              branchseq_to_tradeid[branch_seq_key].erase(branchseq_to_tradeid[branch_seq_key].begin());
            } else {
              std::cout << "***" << branch_seq_key << "***";
              std::cout << "SIZE : " << branchseq_to_tradeid[branch_seq_key].size() << " "
                        << (branchseq_to_tradeid.find(branch_seq_key) != branchseq_to_tradeid.end());
            }
            std::cout << ", ";

            // ACCOUNT_CODE
            std::cout << "DVC, ";

            // ACCOUNT_ORIGIN
            std::cout << "O, ";

            // ORDER_ID
            if (branchseq_to_orderid.find(branch_seq_key) != branchseq_to_orderid.end() &&
                branchseq_to_orderid[branch_seq_key].size() != 0) {
              std::cout << branchseq_to_orderid[branch_seq_key][0];
              branchseq_to_orderid[branch_seq_key].erase(branchseq_to_orderid[branch_seq_key].begin());
            } else {
              std::cout << "SIZE : " << branchseq_to_tradeid[branch_seq_key].size() << " "
                        << (branchseq_to_tradeid.find(branch_seq_key) != branchseq_to_tradeid.end());
            }

            std::cout << ", ";

            // ORDER_ENTRY_DATE assuming no open positions at eod
            year = 1900 + ltm->tm_year;
            month = 1 + ltm->tm_mon;
            day = ltm->tm_mday;
            std::cout << year;
            if (month < 10) std::cout << "0";
            std::cout << month;
            if (day < 10) std::cout << "0";
            std::cout << day << ", ";

            // UNDERLYING_SYMBOL
            std::cout << ", ";

            // SYMBOL
            std::cout << reply_struct.symbol_ << ", ";

            // EXPIRATION_DATE
            std::cout << symbol_to_expiry_date[reply_struct.symbol_] << ", ";

            // EXERCISE_PRICE
            std::cout << ", ";

            // PUT_CALL_CODE
            std::cout << ", ";

            // ORIG_ORDER_PRICE
            std::cout << ", ";

            // ORIG_ORDER_QUANTITY
            std::cout << ", ";

            // TRADE_PRICE
            std::cout << pp << ", ";

            // TRADE_QUANTITY
            std::cout << reply_struct.size_executed_ << ", ";

            // TRADER_ACRONYM
            std::cout << ", ";

            // USER_ID
            std::cout << "DV01, ";

            // FIRM_ID
            std::cout << ", ";

            // CLEAR_FIRM_CODE
            std::cout << "762, ";

            // CLEAR_FIRM_ACRONYM
            std::cout << ", ";

            // CMTA_CODE
            std::cout << ", ";

            // OPEN_CLOSE_INDICATOR
            std::cout << ", ";

            // USER_ROLE_CODE
            std::cout << ", ";

            // CORRES_FIRM_CODE
            std::cout << ", ";

            // BUY_SELL_CODE
            std::cout << HFSAT::GetTradeTypeChar(reply_struct.buysell_) << ", ";

            // CREATE_REDEEM_TYPE
            std::cout << ", ";

            // EXER_ASGN_TYPE
            std::cout << ", ";

            // CRD_NUMBER
            std::cout << ", ";

            // AGGREGATION_UNIT
            std::cout << ", ";

            // TRANS_DRCTN
            std::cout << "FROM, ";

            // TRANS_SITE
            std::cout << "EXCH, ";

            // EXEC_VENUE
            std::cout << ", ";

            std::cout << std::endl;
          }
        }
      }
    }
    // print map
  }

  std::map<std::string, int> symbol_to_expiry_date;
  std::map<std::string, std::vector<long> > branchseq_to_tradeid;
  std::map<std::string, std::vector<long> > branchseq_to_orderid;

 private:
  HFSAT::GenericORSReplyStruct generic_ors_reply_struct_;
  std::string shortcode_;
  int yyyymmdd_;
  std::string ors_bin_filename_;
  HFSAT::ttime_t start_time_;
  HFSAT::ttime_t end_time_;
  bool order;
};

// Mode O for orders and T for trades.
// EXPIRY_CONFIG contains expiry dates of products. Each line in this file should have space seperated Symbol and its
// expiry data.
// TRADE_CONFIG contains map between branch_seq and (Trade_high, Trade_low, Order_high, Order_low). All of them in same
// order seperated by space.
// TRADE_CONFIG can be generated from drop_copy logs
int main(int argc, char** argv) {
  std::string dir = "";
  std::string shortcode_ = "";
  std::string expiry_config_fname = "";
  std::string trade_config_fname = "";

  int yyyymmdd_ = 0;
  bool order = false;
  std::string ors_bin_filename_ = "";
  HFSAT::ttime_t start_time_ = HFSAT::ttime_t(0, 0);
  HFSAT::ttime_t end_time_ = HFSAT::ttime_t(0, 0);

  if (argc != 6 && argc != 7) {
    std::cout << "Usage : EXEC SHORTCODE YYYYMMDD O/T(ORDERS/TRADES) EXPIRY_CONFIG TRADE_CONFIG [ORS_BINARY_FILENAME]"
              << std::endl;
    exit(0);
  } else {
    shortcode_ = std::string(argv[1]);
    yyyymmdd_ = atoi(argv[2]);
    if (argv[3][0] == 'O') order = true;
    expiry_config_fname = std::string(argv[4]);
    trade_config_fname = std::string(argv[5]);
  }
  if (argc == 7) ors_bin_filename_ = argv[6];

  ORSBinReader common_logger(shortcode_, yyyymmdd_, ors_bin_filename_, start_time_, end_time_, order);

  std::string symbol;
  int expiry_date;

  std::ifstream f;
  f.open(expiry_config_fname);

  while (f >> symbol) {
    f >> expiry_date;
    common_logger.symbol_to_expiry_date[symbol] = expiry_date;
  }
  f.close();

  f.open(trade_config_fname);
  std::string branch_seq;
  long trade_high;
  long trade_low;
  long order_high;
  long order_low;

  while (f >> branch_seq) {
    f >> trade_high >> trade_low >> order_high >> order_low;
    //  std::cout << " " << branch_seq << " " << trade_high << " " << trade_low << " " << order_high << " " << order_low
    //          << std::endl;
    long tradeid = 4294967296 * trade_high + trade_low;
    long orderid = 4294967296 * order_high + order_low;

    if (common_logger.branchseq_to_orderid.find(branch_seq) == common_logger.branchseq_to_orderid.end()) {
      std::vector<long> temp;
      common_logger.branchseq_to_orderid[branch_seq] = temp;
    }

    if (common_logger.branchseq_to_tradeid.find(branch_seq) == common_logger.branchseq_to_tradeid.end()) {
      std::vector<long> temp;
      common_logger.branchseq_to_tradeid[branch_seq] = temp;
    }

    common_logger.branchseq_to_orderid[branch_seq].push_back(orderid);
    common_logger.branchseq_to_tradeid[branch_seq].push_back(tradeid);
  }
  f.close();

  common_logger.processMsgRecvd();
}
