// =====================================================================================
//
//       Filename:  generate_retail_query_trades_report.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  03/04/2015 11:37:39 AM
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
#include <cstdlib>
#include <map>
#include <fstream>
#include <algorithm>
#include <string>
#include <cstring>
#include <set>

#include "dvccode/CDef/mds_messages.hpp"
#include "dvccode/CDef/ors_messages.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp"

#define MIN_SYMBOL_LENGTH 6

struct TradesSummary {
  double avg_bid_px;
  double avg_ask_px;
  int32_t num_of_bid_trades;
  int32_t num_of_ask_trades;
  double avg_int_match_bid_px;
  double avg_int_match_ask_px;
  int32_t num_of_int_match_bid_trades;
  int32_t num_of_int_match_ask_trades;

  TradesSummary()
      : avg_bid_px(0.0),
        avg_ask_px(0.0),
        num_of_bid_trades(0),
        num_of_ask_trades(0),
        avg_int_match_bid_px(0.0),
        avg_int_match_ask_px(0.0),
        num_of_int_match_bid_trades(0),
        num_of_int_match_ask_trades(0) {}

  std::string ToString() {
    std::ostringstream t_temp_oss;

    t_temp_oss << "AVG_BID_PX : " << avg_bid_px << " "
               << "AVG_ASK_PX : " << avg_ask_px << " "
               << "NUM_BID_TRADES : " << num_of_bid_trades << " "
               << "NUM_ASK_TRADES : " << num_of_ask_trades << " "
               << "AVG_INT_MATCH_BID_PX : " << avg_int_match_bid_px << " "
               << "AVG_INT_MATCH_ASK_PX : " << avg_int_match_ask_px << " "
               << "NUM_INT_BID_TRADES : " << num_of_int_match_bid_trades << " "
               << "NUM_INT_ASK_TRADES : " << num_of_int_match_ask_trades << "\n";

    return t_temp_oss.str();
  }
};

class DropCopyTradesFileAnalyzer {
 private:
  std::map<std::string, TradesSummary *> security_to_dc_trades_summary_;

 public:
  // Update TradesSummary Records
  void ParseTokensAndGenerateTradesSummary(const std::vector<const char *> &tokens) {
    if (security_to_dc_trades_summary_.end() == security_to_dc_trades_summary_.find(tokens[0])) {
      security_to_dc_trades_summary_[tokens[0]] = new TradesSummary();
    }

    TradesSummary *this_trade_summary = security_to_dc_trades_summary_[tokens[0]];

    // Buy
    if (atoi(tokens[1]) == 0) {
      double total_trade_value = (this_trade_summary->avg_bid_px) * (this_trade_summary->num_of_bid_trades);
      total_trade_value += (atof(tokens[3]) * atoi(tokens[2]));
      this_trade_summary->num_of_bid_trades = this_trade_summary->num_of_bid_trades + atoi(tokens[2]);

      this_trade_summary->avg_bid_px = total_trade_value / (double)(this_trade_summary->num_of_bid_trades);

    } else if (atoi(tokens[1]) == 1) {
      double total_trade_value = (this_trade_summary->avg_ask_px) * (this_trade_summary->num_of_ask_trades);
      total_trade_value += (atof(tokens[3]) * atoi(tokens[2]));
      this_trade_summary->num_of_ask_trades = this_trade_summary->num_of_ask_trades + atoi(tokens[2]);

      this_trade_summary->avg_ask_px = total_trade_value / (double)(this_trade_summary->num_of_ask_trades);

    } else {
      std::cerr << "UNEXPECTED TRADETYPE RECEVIED" << tokens[1] << std::endl;
      exit(-1);
    }
  }

  DropCopyTradesFileAnalyzer(const char *_dropcopy_trades_filename_) {
    std::ifstream dropcopy_trades_file_stream;
    dropcopy_trades_file_stream.open(_dropcopy_trades_filename_, std::ifstream::in);

    if (!dropcopy_trades_file_stream.is_open()) {
      std::cerr << "Failed to open the dropcopy file to analyze the trades summary" << _dropcopy_trades_filename_
                << std::endl;
      exit(-1);
    }

    char trades_buffer[1024];
    std::string trades_buffer_str = "";

    while (dropcopy_trades_file_stream.good()) {
      dropcopy_trades_file_stream.getline(trades_buffer, 1024);
      trades_buffer_str = trades_buffer;

      if (trades_buffer_str.length() <= 0) continue;

      // replace all delim characters in the string with spaces so token generator can work on that
      std::replace(trades_buffer_str.begin(), trades_buffer_str.end(), '\001', ' ');
      memcpy((void *)trades_buffer, (void *)trades_buffer_str.c_str(), trades_buffer_str.length());

      HFSAT::PerishableStringTokenizer st(trades_buffer, 1024);
      const std::vector<const char *> &tokens = st.GetTokens();

      // We expect exact 5 tokens
      if (tokens.size() != 5) continue;

      ParseTokensAndGenerateTradesSummary(tokens);
    }

    dropcopy_trades_file_stream.close();

    for (auto itr : security_to_dc_trades_summary_) {
      if (NULL == security_to_dc_trades_summary_[itr.first]) {
        std::cerr << " NULL ORDER BUG \n";
      }

      //        std::cout << " SECURITY : " << itr.first << " SUMMARY : " << ( security_to_dc_trades_summary_ [
      //        itr.first ] ) -> ToString () << "\n" ;
    }
  }

  std::map<std::string, TradesSummary *> GetDropCopyTradesData() { return security_to_dc_trades_summary_; }

  void CleanUp() {
    for (auto itr : security_to_dc_trades_summary_) {
      if (NULL != itr.second) {
        delete itr.second;
        itr.second = NULL;
      }
    }
  }

  ~DropCopyTradesFileAnalyzer() { CleanUp(); }
};

class ORSDataAnalyzer {
 private:
  std::map<int32_t, int32_t> saos_to_execution_size_;
  std::map<std::string, TradesSummary *> security_to_ors_trades_summary_;

 public:
  void AnalyzedORSEventsForTradesSummary(const HFSAT::GenericORSReplyStruct &_ors_reply_event_,
                                         const std::set<int32_t> &_list_of_saci_) {
    if (HFSAT::kORRType_Exec != _ors_reply_event_.orr_type_ && HFSAT::kORRType_IntExec != _ors_reply_event_.orr_type_)
      return;
    if (_list_of_saci_.end() == _list_of_saci_.find(_ors_reply_event_.server_assigned_client_id_)) return;

    int32_t current_size_executed = 0;

    if (saos_to_execution_size_.end() ==
        saos_to_execution_size_.find(_ors_reply_event_.server_assigned_order_sequence_)) {
      current_size_executed = _ors_reply_event_.size_executed_;

    } else {
      current_size_executed =
          _ors_reply_event_.size_executed_ - saos_to_execution_size_[_ors_reply_event_.server_assigned_order_sequence_];
    }

    if (0 == current_size_executed) {
      std::cerr << "Unexpected Size Executed Found : " << current_size_executed
                << " SAOS : " << _ors_reply_event_.server_assigned_order_sequence_ << std::endl;
      exit(-1);
    }

    // Update last exec size
    saos_to_execution_size_[_ors_reply_event_.server_assigned_order_sequence_] = _ors_reply_event_.size_executed_;

    if (security_to_ors_trades_summary_.end() == security_to_ors_trades_summary_.find(_ors_reply_event_.symbol_)) {
      security_to_ors_trades_summary_[_ors_reply_event_.symbol_] = new TradesSummary();
    }

    TradesSummary *this_trade_summary = security_to_ors_trades_summary_[_ors_reply_event_.symbol_];

    if (HFSAT::kORRType_Exec == _ors_reply_event_.orr_type_) {
      // Buy
      if (HFSAT::kTradeTypeBuy == _ors_reply_event_.buysell_) {
        double total_trade_value = (this_trade_summary->avg_bid_px) * (this_trade_summary->num_of_bid_trades);
        total_trade_value += (_ors_reply_event_.price_ * current_size_executed);
        this_trade_summary->num_of_bid_trades = this_trade_summary->num_of_bid_trades + current_size_executed;

        this_trade_summary->avg_bid_px = total_trade_value / (double)(this_trade_summary->num_of_bid_trades);

      } else if (HFSAT::kTradeTypeSell == _ors_reply_event_.buysell_) {
        double total_trade_value = (this_trade_summary->avg_ask_px) * (this_trade_summary->num_of_ask_trades);
        total_trade_value += (_ors_reply_event_.price_ * current_size_executed);
        this_trade_summary->num_of_ask_trades = this_trade_summary->num_of_ask_trades + current_size_executed;

        this_trade_summary->avg_ask_px = total_trade_value / (double)(this_trade_summary->num_of_ask_trades);

      } else {
        std::cerr << "UNEXPECTED TRADETYPE RECEVIED" << (int32_t)_ors_reply_event_.buysell_ << std::endl;
        exit(-1);
      }

    } else {
      // Buy
      if (HFSAT::kTradeTypeBuy == _ors_reply_event_.buysell_) {
        double total_trade_value =
            (this_trade_summary->avg_int_match_bid_px) * (this_trade_summary->num_of_int_match_bid_trades);
        total_trade_value += (_ors_reply_event_.price_ * current_size_executed);
        this_trade_summary->num_of_int_match_bid_trades =
            this_trade_summary->num_of_int_match_bid_trades + current_size_executed;

        this_trade_summary->avg_int_match_bid_px =
            total_trade_value / (double)(this_trade_summary->num_of_int_match_bid_trades);

      } else if (HFSAT::kTradeTypeSell == _ors_reply_event_.buysell_) {
        double total_trade_value =
            (this_trade_summary->avg_int_match_ask_px) * (this_trade_summary->num_of_int_match_ask_trades);
        total_trade_value += (_ors_reply_event_.price_ * current_size_executed);
        this_trade_summary->num_of_int_match_ask_trades =
            this_trade_summary->num_of_int_match_ask_trades + current_size_executed;

        this_trade_summary->avg_int_match_ask_px =
            total_trade_value / (double)(this_trade_summary->num_of_int_match_ask_trades);

      } else {
        std::cerr << "UNEXPECTED TRADETYPE RECEVIED" << (int32_t)_ors_reply_event_.buysell_ << std::endl;
        exit(-1);
      }
    }
  }

  ORSDataAnalyzer(const char *_ors_data_filename_, const std::set<int32_t> &_list_of_saci_) {
    HFSAT::BulkFileReader *bulk_file_reader = new HFSAT::BulkFileReader();
    bulk_file_reader->open(_ors_data_filename_);

    if (!bulk_file_reader->is_open()) {
      std::cerr << "Failed to open the ors data file to analyzing trades : " << _ors_data_filename_ << std::endl;
      exit(-1);
    }

    HFSAT::MDS_MSG::GenericMDSMessage generic_mds_message_event;
    HFSAT::GenericORSReplyStruct ors_reply_event;

    while (true) {
      auto read_length = bulk_file_reader->read(&generic_mds_message_event, sizeof(HFSAT::MDS_MSG::GenericMDSMessage));

      // No more data to read
      if (read_length < sizeof(HFSAT::MDS_MSG::GenericMDSMessage)) break;

      memcpy((void *)&ors_reply_event, (void *)&generic_mds_message_event.generic_data_.ors_reply_data_,
             sizeof(HFSAT::GenericORSReplyStruct));

      AnalyzedORSEventsForTradesSummary(ors_reply_event, _list_of_saci_);
    }
  }

  void GenerateTradesReport(std::map<std::string, TradesSummary *> _dc_security_to_tradesdata_map_) {
    for (auto ors_itr : security_to_ors_trades_summary_) {
      // Some Security Exists in ORS for which there is no mapping in DC
      if (_dc_security_to_tradesdata_map_.end() == _dc_security_to_tradesdata_map_.find(ors_itr.first)) {
        std::cerr
            << "SECURITY : " << ors_itr.first
            << " TRADED FROM ORS FOR MENTIONED SACI AS RETAIL BUT DOESN'T HAVE CORROSPONDING RETAIL TRADES IN DC FILES"
            << std::endl;

      } else {
        TradesSummary *ors_trades_summary = ors_itr.second;
        TradesSummary *dc_trades_summary = _dc_security_to_tradesdata_map_[ors_itr.first];

        if (dc_trades_summary->num_of_bid_trades !=
            (ors_trades_summary->num_of_ask_trades + ors_trades_summary->num_of_int_match_ask_trades)) {
          //            std::cerr << "TRADE DISCREPANCY FOR : " << ors_itr.first << " DC BID TRADES : " <<
          //            dc_trades_summary -> num_of_bid_trades << " ORS ASK TRADES MARKET : " << ors_trades_summary ->
          //            num_of_ask_trades << " ORS ASK TRADES INT MATCH : " << ors_trades_summary ->
          //            num_of_int_match_ask_trades << std::endl ;

        } else if (dc_trades_summary->num_of_ask_trades !=
                   (ors_trades_summary->num_of_bid_trades + ors_trades_summary->num_of_int_match_bid_trades)) {
          //            std::cerr << "TRADE DISCREPANCY FOR : " << ors_itr.first << " DC ASK TRADES : " <<
          //            dc_trades_summary -> num_of_ask_trades << " ORS BID TRADES MARKET : " << ors_trades_summary ->
          //            num_of_bid_trades << " ORS BID TRADES INT MATCH : " << ors_trades_summary ->
          //            num_of_int_match_bid_trades << std::endl ;
        }

        if (0 != dc_trades_summary->num_of_bid_trades)
          std::cout << ors_itr.first << '\001' << 0 << '\001'
                    << std::min(dc_trades_summary->num_of_bid_trades, ors_trades_summary->num_of_ask_trades) << '\001'
                    << dc_trades_summary->avg_bid_px << '\001' << "99999\n";
        if (0 != ors_trades_summary->num_of_ask_trades)
          std::cout << ors_itr.first << '\001' << 1 << '\001' << ors_trades_summary->num_of_ask_trades << '\001'
                    << ors_trades_summary->avg_ask_px << '\001' << "99999\n";
        if (0 != dc_trades_summary->num_of_ask_trades)
          std::cout << ors_itr.first << '\001' << 1 << '\001'
                    << std::min(dc_trades_summary->num_of_ask_trades, ors_trades_summary->num_of_bid_trades) << '\001'
                    << dc_trades_summary->avg_ask_px << '\001' << "99999\n";
        if (0 != ors_trades_summary->num_of_bid_trades)
          std::cout << ors_itr.first << '\001' << 0 << '\001' << ors_trades_summary->num_of_bid_trades << '\001'
                    << ors_trades_summary->avg_bid_px << '\001' << "99999\n";

        int32_t min_bid_trades = std::min(dc_trades_summary->num_of_bid_trades - ors_trades_summary->num_of_ask_trades,
                                          ors_trades_summary->num_of_int_match_ask_trades);
        int32_t min_ask_trades = std::min(dc_trades_summary->num_of_ask_trades - ors_trades_summary->num_of_bid_trades,
                                          ors_trades_summary->num_of_int_match_bid_trades);

        //          std::cout << " Min Bid : " << min_bid_trades << " " << min_ask_trades << " " << dc_trades_summary ->
        //          num_of_bid_trades << " " << dc_trades_summary -> num_of_ask_trades << " " << dc_trades_summary ->
        //          avg_bid_px << " " << dc_trades_summary -> avg_ask_px << " " << ors_trades_summary ->
        //          avg_int_match_bid_px << " " << ors_trades_summary -> avg_int_match_ask_px << "\n" ;

        // Unrealized
        if (0 != min_ask_trades)
          std::cerr << ors_itr.first << '\001' << 1 << '\001' << min_ask_trades << '\001'
                    << dc_trades_summary->avg_ask_px << '\001' << "99999\n";
        if (0 != min_ask_trades)
          std::cerr << ors_itr.first << '\001' << 0 << '\001' << min_ask_trades << '\001'
                    << ors_trades_summary->avg_int_match_bid_px << '\001' << "99999\n";
        if (0 != min_bid_trades)
          std::cerr << ors_itr.first << '\001' << 1 << '\001' << min_bid_trades << '\001'
                    << ors_trades_summary->avg_int_match_ask_px << '\001' << "99999\n";
        if (0 != min_bid_trades)
          std::cerr << ors_itr.first << '\001' << 0 << '\001' << min_bid_trades << '\001'
                    << dc_trades_summary->avg_bid_px << '\001' << "99999\n";
      }
    }
  }

  void CleanUp() {
    for (auto itr : security_to_ors_trades_summary_) {
      if (NULL != itr.second) {
        delete itr.second;
        itr.second = NULL;
      }
    }
  }

  ~ORSDataAnalyzer() {}
};

int main(int argc, char *argv[]) {
  if (argc < 4) {
    std::cerr << "Usage : <exec> <dropcopy-trades-file> <raw-ors-data-file> <list-of-saci>" << std::endl;
    exit(-1);
  }

  DropCopyTradesFileAnalyzer dropcopy_trades_file_analyzer(argv[1]);

  std::set<int32_t> list_of_retail_saci_vec;

  for (int32_t arg_counter = 3; arg_counter < argc; arg_counter++) {
    list_of_retail_saci_vec.insert(atoi(argv[arg_counter]));
  }

  ORSDataAnalyzer ors_data_analyzer(argv[2], list_of_retail_saci_vec);

  ors_data_analyzer.GenerateTradesReport(dropcopy_trades_file_analyzer.GetDropCopyTradesData());

  return EXIT_SUCCESS;
}  // ----------  end of function main  ----------
