/**
  \file Tools/get_data_verification_stats.cpp

  \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
  Address:
   Suite No 162, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551

*/

//@brief compares the data across two locations and generated the diff

#include <iostream>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include <iomanip>
#include <fstream>
#include "dvccode/CDef/email_utils.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CDef/trading_location_manager.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp"

#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

#include "baseinfra/LoggedSources/cme_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/eurex_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/tmx_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/ntp_logged_message_filenamer.hpp"
#include "baseinfra/MarketAdapter/book_init_utils.hpp"
#include "baseinfra/LoggedSources/eobi_price_feed_logged_message_filenamer.hpp"

#define TIME_PERIOD_LIMIT 300
#define PACKET_DROP_LIMIT_PERC 1
#define SIZE_LIMIT_PERC 5
#define CHUNK_DROP_LIMIT 10
#define DIFF_LIMIT_PERC 0.5

void ParseCommandLineParams(const int argc, const char **argv, std::string &shortcode_, int &input_date_,
                            std::string &primary_location_, std::string &alternate_location_,
                            int &begin_secs_from_midnight_, int &end_secs_from_midnight_) {
  // expect :
  // 1. $0 shortcode date_YYYYMMDD [ start_tm HHMM ] [ end_tm HHMM ]
  if (argc < 5) {
    std::cerr << "Usage: " << argv[0]
              << " shortcode input_date_YYYYMMDD pri_loc sec_loc [ start_tm_utc_hhmm ] [ end_tm_utc_hhmm ]"
              << std::endl;
    exit(0);
  } else {
    shortcode_ = argv[1];
    input_date_ = atoi(argv[2]);
    primary_location_ = argv[3];
    alternate_location_ = argv[4];

    if (argc > 5) {
      begin_secs_from_midnight_ = (atoi(argv[5]) / 100) * 60 * 60 + (atoi(argv[3]) % 100) * 60;
    }
    if (argc > 6) {
      end_secs_from_midnight_ = (atoi(argv[6]) / 100) * 60 * 60 + (atoi(argv[4]) % 100) * 60;
    }
  }
}

void printCMETrade(std::string &location_, CME_MDS::CMECommonStruct &cme_trd_msg_, bool isDiff = false) {
  char diff_indicator_ = isDiff ? '*' : '+';

  std::cout << location_ << " " << diff_indicator_ << "TRADE : " << cme_trd_msg_.time_.tv_sec << "."
            << cme_trd_msg_.time_.tv_usec << "  "
            << "Contract:" << cme_trd_msg_.data_.cme_trds_.contract_ << "  "
            << "Agg_Side:" << cme_trd_msg_.data_.cme_trds_.agg_side_ << "  "
            << "Trade_Size:" << cme_trd_msg_.data_.cme_trds_.trd_qty_ << "  "
            << "Tot_Qty:" << cme_trd_msg_.data_.cme_trds_.tot_qty_ << "  "
            << "Trade_Px:" << cme_trd_msg_.data_.cme_trds_.trd_px_ << "  "
            << "InstSeqNum:" << cme_trd_msg_.data_.cme_trds_.seqno_ << std::endl;
}

void printCMEDelta(std::string &location_, CME_MDS::CMECommonStruct &cme_dels_msg_, bool isDiff = false) {
  char diff_indicator_ = isDiff ? '*' : '+';

  std::string action_ = "";
  std::string type_ = "";

  switch (cme_dels_msg_.data_.cme_dels_.action_) {
    case '0':
      action_ = "New";
      break;
    case '1':
      action_ = "Change";
      break;
    case '2':
      action_ = "Delete";
      break;
    case '3':
      action_ = "Delete Thru";
      break;
    case '4':
      action_ = "Delete From";
      break;
    default:
      action_ = "Undefined";
      break;
  }

  switch (cme_dels_msg_.data_.cme_dels_.type_) {
    case '0':
      type_ = "Bid";
      break;
    case '1':
      type_ = "Offer";
      break;
    default:
      type_ = "Undefined";
      break;
  }

  std::cout << location_ << " " << diff_indicator_ << "DELTA : " << cme_dels_msg_.time_.tv_sec << "."
            << cme_dels_msg_.time_.tv_usec << "  "
            << "Contract:" << cme_dels_msg_.data_.cme_dels_.contract_ << "  "
            << "Level:" << cme_dels_msg_.data_.cme_dels_.level_ << "  "
            << "Size:" << cme_dels_msg_.data_.cme_dels_.size_ << "  "
            << "Num_Ords:" << cme_dels_msg_.data_.cme_dels_.num_ords_ << "  "
            << "Price:" << cme_dels_msg_.data_.cme_dels_.price_ << "  "
            << "InstSeqNum:" << cme_dels_msg_.data_.cme_dels_.seqno_ << "  "
            << "Type:" << type_ << "  "
            << "Status:" << cme_dels_msg_.data_.cme_dels_.status_ << "  "
            << "Action:" << action_ << "  "
            << "Intermediate:" << (cme_dels_msg_.data_.cme_dels_.intermediate_ ? "Y" : "N") << std::endl;
}

// can use overloaded == with CMEStruct
bool isCMETradeEqual(CME_MDS::CMECommonStruct &msg_1_, CME_MDS::CMECommonStruct &msg_2_) {
  if (strcmp(msg_1_.data_.cme_trds_.contract_, msg_2_.data_.cme_trds_.contract_)) return false;
  if (msg_1_.data_.cme_trds_.trd_qty_ != msg_2_.data_.cme_trds_.trd_qty_) return false;
  if (msg_1_.data_.cme_trds_.tot_qty_ != msg_2_.data_.cme_trds_.tot_qty_) return false;
  if (msg_1_.data_.cme_trds_.trd_px_ != msg_2_.data_.cme_trds_.trd_px_) return false;
  if (msg_1_.data_.cme_trds_.agg_side_ != msg_2_.data_.cme_trds_.agg_side_) return false;

  return true;
}

bool isCMEDeltaEqual(CME_MDS::CMECommonStruct &msg_1_, CME_MDS::CMECommonStruct &msg_2_) {
  if (strcmp(msg_1_.data_.cme_dels_.contract_, msg_2_.data_.cme_dels_.contract_)) return false;
  if (msg_1_.data_.cme_dels_.level_ != msg_2_.data_.cme_dels_.level_) return false;
  if (msg_1_.data_.cme_dels_.size_ != msg_2_.data_.cme_dels_.size_) return false;
  if (msg_1_.data_.cme_dels_.num_ords_ != msg_2_.data_.cme_dels_.num_ords_) return false;
  if (msg_1_.data_.cme_dels_.price_ != msg_2_.data_.cme_dels_.price_) return false;
  if (msg_1_.data_.cme_dels_.type_ != msg_2_.data_.cme_dels_.type_) return false;
  if (msg_1_.data_.cme_dels_.status_ != msg_2_.data_.cme_dels_.status_) return false;
  if (msg_1_.data_.cme_dels_.action_ != msg_2_.data_.cme_dels_.action_) return false;
  //  if( msg_1_.data_.cme_dels_.intermediate_ != msg_2_.data_.cme_dels_.intermediate_ )   return false ;

  return true;
}

bool isEurexMsgEqual(EUREX_MDS::EUREXCommonStruct &msg_1_, EUREX_MDS::EUREXCommonStruct &msg_2_) {
  if (msg_1_.msg_ != msg_2_.msg_) return false;

  if (msg_1_.msg_ == EUREX_MDS::EUREX_DELTA) {
    if (strcmp(msg_1_.data_.eurex_dels_.contract_, msg_2_.data_.eurex_dels_.contract_)) return false;
    if (msg_1_.data_.eurex_dels_.trd_qty_ != msg_2_.data_.eurex_dels_.trd_qty_) return false;
    if (msg_1_.data_.eurex_dels_.level_ != msg_2_.data_.eurex_dels_.level_) return false;
    if (msg_1_.data_.eurex_dels_.size_ != msg_2_.data_.eurex_dels_.size_) return false;
    if (msg_1_.data_.eurex_dels_.num_ords_ != msg_2_.data_.eurex_dels_.num_ords_) return false;
    if (msg_1_.data_.eurex_dels_.price_ != msg_2_.data_.eurex_dels_.price_) return false;
    if (msg_1_.data_.eurex_dels_.type_ != msg_2_.data_.eurex_dels_.type_) return false;
    if (msg_1_.data_.eurex_dels_.status_ != msg_2_.data_.eurex_dels_.status_) return false;
    if (msg_1_.data_.eurex_dels_.action_ != msg_2_.data_.eurex_dels_.action_) return false;
    if (msg_1_.data_.eurex_dels_.intermediate_ != msg_2_.data_.eurex_dels_.intermediate_) return false;

  } else if (msg_1_.msg_ == EUREX_MDS::EUREX_TRADE) {
    if (strcmp(msg_1_.data_.eurex_trds_.contract_, msg_2_.data_.eurex_trds_.contract_)) return false;
    if (msg_1_.data_.eurex_trds_.agg_side_ != msg_2_.data_.eurex_trds_.agg_side_) return false;
    if (msg_1_.data_.eurex_trds_.trd_qty_ != msg_2_.data_.eurex_trds_.trd_qty_) return false;
    if (msg_1_.data_.eurex_trds_.tot_qty_ != msg_2_.data_.eurex_trds_.tot_qty_) return false;
    if (msg_1_.data_.eurex_trds_.num_buy_ords_ != msg_2_.data_.eurex_trds_.num_buy_ords_) return false;
    if (msg_1_.data_.eurex_trds_.num_sell_ords_ != msg_2_.data_.eurex_trds_.num_sell_ords_) return false;
    if (msg_1_.data_.eurex_trds_.trd_px_ != msg_2_.data_.eurex_trds_.trd_px_) return false;
  }

  return true;
}

void printEUREXDelta(std::string location_, EUREX_MDS::EUREXCommonStruct &eurex_dels_msg_, bool isDiff = false) {
  char diff_indicator_ = isDiff ? '*' : '+';

  std::string action_ = "";
  std::string type_ = "";

  switch (eurex_dels_msg_.data_.eurex_dels_.action_) {
    case '1':
      action_ = "NEW";
      break;
    case '2':
      action_ = "CHANGE";
      break;
    case '3':
      action_ = "DELETE";
      break;
    case '4':
      action_ = "DEL_FRM";
      break;
    case '5':
      action_ = "DEL_THRU";
      break;
    default:
      action_ = "Undefined";
      break;
  }

  switch (eurex_dels_msg_.data_.eurex_dels_.type_) {
    case '1':
      type_ = "ASK";
      break;
    case '2':
      type_ = "BID";
      break;
    default:
      type_ = "---";
      break;
  }

  std::cout << location_ << " " << diff_indicator_ << "DELTA : " << eurex_dels_msg_.time_.tv_sec << "."
            << eurex_dels_msg_.time_.tv_usec << "  "
            << "Contract:" << eurex_dels_msg_.data_.eurex_dels_.contract_ << "  "
            << "Tot_Qty:" << eurex_dels_msg_.data_.eurex_dels_.trd_qty_ << "  "
            << "Level:" << eurex_dels_msg_.data_.eurex_dels_.level_ << "  "
            << "Size:" << eurex_dels_msg_.data_.eurex_dels_.size_ << "  "
            << "Num_Ords:" << eurex_dels_msg_.data_.eurex_dels_.num_ords_ << "  "
            << "Price:" << eurex_dels_msg_.data_.eurex_dels_.price_ << "  "
            << "Type:" << type_ << "  "
            << "Status:" << eurex_dels_msg_.data_.eurex_dels_.status_ << "  "
            << "Action:" << action_ << "  "
            << "Intermediate:" << (eurex_dels_msg_.data_.eurex_dels_.intermediate_ ? "Y" : "N") << std::endl;
}

void printEUREXTrade(std::string location_, EUREX_MDS::EUREXCommonStruct &eurex_trds_msg_, bool isDiff = false) {
  char diff_indicator_ = isDiff ? '*' : '+';

  std::cout << location_ << " " << diff_indicator_ << "TRADE : " << eurex_trds_msg_.time_.tv_sec << "."
            << eurex_trds_msg_.time_.tv_usec << "  "
            << "Contract:" << eurex_trds_msg_.data_.eurex_trds_.contract_ << "  "
            << "Agg_Side:" << eurex_trds_msg_.data_.eurex_trds_.agg_side_ << "  "
            << "Trade_Size:" << eurex_trds_msg_.data_.eurex_trds_.trd_qty_ << "  "
            << "Tot_Qty:" << eurex_trds_msg_.data_.eurex_trds_.tot_qty_ << "  "
            << "Num_Buy_Sell:" << eurex_trds_msg_.data_.eurex_trds_.num_buy_ords_ << "  "
            << eurex_trds_msg_.data_.eurex_trds_.num_sell_ords_ << "  "
            << "Trade_Px:" << eurex_trds_msg_.data_.eurex_trds_.trd_px_ << std::endl;
}

int get_msec_diff_between_two_msg(EUREX_MDS::EUREXCommonStruct &msg_1_, EUREX_MDS::EUREXCommonStruct &msg_2_) {
  //  std::cout << " Msg 1 : " << msg_1_.time_.tv_sec << "." << msg_1_.time_.tv_usec << " Msg 2 :" <<
  //  msg_2_.time_.tv_sec << "." << msg_2_.time_.tv_usec << "\n";
  if (msg_1_.time_.tv_sec == msg_2_.time_.tv_sec) return ((msg_1_.time_.tv_usec - msg_2_.time_.tv_usec) / 1000);

  if (msg_1_.time_.tv_sec > msg_2_.time_.tv_sec) {
    if (msg_1_.time_.tv_usec < msg_2_.time_.tv_usec)
      return (((msg_1_.time_.tv_sec - msg_2_.time_.tv_sec) * 1000000 - (msg_1_.time_.tv_usec - msg_2_.time_.tv_usec)) /
              1000);
    else
      return (((msg_1_.time_.tv_sec - msg_2_.time_.tv_sec) * 1000000 + (msg_1_.time_.tv_usec - msg_2_.time_.tv_usec)) /
              1000);
  }

  return -1;  // not handling the case now
}

long get_usec_time_diff_between_two_timestamp(struct timeval &time1, struct timeval &time2) {
  if (time1.tv_sec == time2.tv_sec) return (time1.tv_usec - time2.tv_usec);

  if (time1.tv_sec > time2.tv_usec) {
    if (time1.tv_usec < time2.tv_usec)
      return ((time1.tv_sec - time2.tv_sec) * 1000000 - (time1.tv_usec - time2.tv_usec));
    else
      return ((time1.tv_sec - time2.tv_sec) * 1000000 + (time1.tv_usec - time2.tv_usec));
  }
  return -1;
}

void sendEmailNotification(std::string email_body_) {
  HFSAT::Email e;
  e.setSubject("Subject: Data Verification Stats");
  e.addRecepient("nseall@tworoads.co.in");
  e.addSender("nseall@tworoads.co.in");
  e.content_stream << email_body_;

  e.sendMail();
}

int main(int argc, char **argv) {
  std::string shortcode_ = "";
  int input_date_ = 20110101;
  int begin_secs_from_midnight_ = 0;
  int end_secs_from_midnight_ = 24 * 60 * 60;
  std::string primary_location_ = "";
  std::string alternate_location_ = "";
  bool drop_detected_at_primary_ = false;
  bool drop_detected_at_alternate_ = false;

  bool found_diff_ = false;

  std::vector<std::string> list_of_shortcodes_;
  std::vector<std::string> list_of_alternate_;
  bool all_shortcode_switch_ = false;

  //  int msec_diff_between_locations_ = 0 ;
  // stats params
  unsigned long total_dels_packets_at_primary_ = 0;
  unsigned long total_dels_packets_at_alternate_ = 0;
  unsigned int total_trds_packets_at_primary_ = 0;
  unsigned int total_trds_packets_at_alternate_ = 0;
  unsigned int total_chunk_drops_at_primary_ = 0;
  unsigned int total_chunk_drops_at_alternate_ = 0;
  unsigned int total_differences_ = 0;
  double total_bytes_at_primary_ = 0;
  double total_bytes_at_alternate_ = 0;

  std::vector<struct timeval> chunk_drop_start_time_at_primary_;
  std::vector<struct timeval> chunk_drop_end_time_at_primary_;
  std::vector<struct timeval> chunk_drop_start_time_at_alternate_;
  std::vector<struct timeval> chunk_drop_end_time_at_alternate_;
  std::vector<int> total_packets_lost_in_chunk_at_primary_;
  std::vector<int> total_packets_lost_in_chunk_at_alternate_;
  int packets_drop_in_chunk_at_primary_ = 0;
  int packets_drop_in_chunk_at_alternate_ = 0;

  long max_usec_difference_between_two_packets_at_primary_ = 0;
  long max_usec_difference_between_two_packets_at_alternate_ = 0;
  struct timeval packet_received_last_time_at_primary_;
  struct timeval packet_received_last_time_at_alternate_;
  struct timeval packet_received_start_time_at_primary_;
  struct timeval packet_received_end_time_at_primary_;
  struct timeval packet_received_start_time_at_alternate_;
  struct timeval packet_received_end_time_at_alternate_;

  packet_received_last_time_at_primary_.tv_sec = packet_received_last_time_at_primary_.tv_usec = 0;
  packet_received_last_time_at_alternate_.tv_sec = packet_received_last_time_at_alternate_.tv_usec = 0;
  packet_received_start_time_at_primary_.tv_sec = packet_received_start_time_at_primary_.tv_usec = 0;
  packet_received_end_time_at_primary_.tv_sec = packet_received_end_time_at_primary_.tv_usec = 0;
  packet_received_start_time_at_alternate_.tv_sec = packet_received_start_time_at_alternate_.tv_usec = 0;
  packet_received_end_time_at_alternate_.tv_sec = packet_received_end_time_at_alternate_.tv_usec = 0;

  // to have some reference where the drops are at beginning or at the end
  uint32_t start_seq_at_primary_ = 0;
  uint32_t end_seq_at_primary_ = 0;
  uint32_t start_seq_at_alternate_ = 0;
  uint32_t end_seq_at_alternate_ = 0;

  ParseCommandLineParams(argc, (const char **)argv, shortcode_, input_date_, primary_location_, alternate_location_,
                         begin_secs_from_midnight_, end_secs_from_midnight_);

  std::ostringstream t_temp_oss_;
  t_temp_oss_ << "<head><center>" << input_date_ << "</center></head><br/>";
  std::string email_text_ =
      t_temp_oss_.str() + " SHORTCODE LOCATION SIZE PACKETS CHUNK_DRP MAX_SILENT_PERIOD TOTAL_DIFF <br/><br/>";

  HFSAT::ExchangeSymbolManager::SetUniqueInstance(input_date_);

  if (shortcode_ == "ALL_SEC") {
    std::ifstream all_shortcode_file_;
    all_shortcode_file_.open("/spare/local/files/mkt_sec.txt", std::ios::in);

    if (!all_shortcode_file_.is_open()) {
      std::cerr << " File : /spare/local/files/mkt_sec.txt doesn't exist " << std::endl;
      exit(-1);
    }

    char line_buffer_[1024];
    std::string line_read_ = "";

    while (all_shortcode_file_.good()) {
      memset(line_buffer_, 0, sizeof(line_buffer_));
      all_shortcode_file_.getline(line_buffer_, sizeof(line_buffer_));
      line_read_ = line_buffer_;

      if (line_read_.find("#") != std::string::npos) continue;  // comments etc.

      HFSAT::PerishableStringTokenizer st_(line_buffer_, sizeof(line_buffer_));
      const std::vector<const char *> &tokens_ = st_.GetTokens();

      if (tokens_.size() == 1) {  // only expecting shortcodes

        list_of_shortcodes_.push_back(tokens_[0]);
      }
    }

    all_shortcode_file_.close();

  } else {
    list_of_shortcodes_.push_back(shortcode_);
  }

  if (alternate_location_ == "ALL_ALTR") {
    list_of_alternate_.push_back("TOR");
    list_of_alternate_.push_back("BRZ");
    all_shortcode_switch_ = true;
  } else
    list_of_alternate_.push_back(alternate_location_);

  // iterate over shortcode list
  for (unsigned int shortcode_counter_ = 0; shortcode_counter_ < list_of_shortcodes_.size(); shortcode_counter_++) {
    shortcode_ = list_of_shortcodes_[shortcode_counter_];

    const char *t_exchange_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(shortcode_);

    if (all_shortcode_switch_ && list_of_alternate_.size() > 2) list_of_alternate_.pop_back();

    if (all_shortcode_switch_ &&
        HFSAT::SecurityDefinitions::GetContractExchSource(shortcode_, input_date_) == HFSAT::kExchSourceCME) {
      list_of_alternate_.push_back("FR2");
      primary_location_ = "CHI";
    }
    if (all_shortcode_switch_ &&
        HFSAT::SecurityDefinitions::GetContractExchSource(shortcode_, input_date_) == HFSAT::kExchSourceEUREX) {
      list_of_alternate_.push_back("CHI");
      primary_location_ = "FR2";
    }

    for (unsigned int alternate_location_counter_ = 0; alternate_location_counter_ < list_of_alternate_.size();
         alternate_location_counter_++) {
      // reset all params TODO put all this params in a struct
      drop_detected_at_primary_ = drop_detected_at_alternate_ = false;
      //  msec_diff_between_locations_ = 0;
      total_dels_packets_at_primary_ = total_dels_packets_at_alternate_ = total_trds_packets_at_primary_ =
          total_trds_packets_at_alternate_ = 0;
      total_chunk_drops_at_primary_ = total_chunk_drops_at_alternate_ = total_differences_ = total_bytes_at_primary_ =
          total_bytes_at_alternate_ = 0;
      chunk_drop_start_time_at_primary_.clear();
      chunk_drop_end_time_at_primary_.clear();
      chunk_drop_start_time_at_alternate_.clear();
      chunk_drop_end_time_at_alternate_.clear();
      total_packets_lost_in_chunk_at_primary_.clear();
      total_packets_lost_in_chunk_at_alternate_.clear();
      packets_drop_in_chunk_at_primary_ = packets_drop_in_chunk_at_alternate_ =
          max_usec_difference_between_two_packets_at_primary_ = max_usec_difference_between_two_packets_at_alternate_ =
              0;
      packet_received_last_time_at_primary_.tv_sec = packet_received_last_time_at_primary_.tv_usec = 0;
      packet_received_last_time_at_alternate_.tv_sec = packet_received_last_time_at_alternate_.tv_usec = 0;
      packet_received_start_time_at_primary_.tv_sec = packet_received_start_time_at_primary_.tv_usec = 0;
      packet_received_end_time_at_primary_.tv_sec = packet_received_end_time_at_primary_.tv_usec = 0;
      packet_received_start_time_at_alternate_.tv_sec = packet_received_start_time_at_alternate_.tv_usec = 0;
      packet_received_end_time_at_alternate_.tv_sec = packet_received_end_time_at_alternate_.tv_usec = 0;
      start_seq_at_primary_ = end_seq_at_primary_ = start_seq_at_alternate_ = end_seq_at_alternate_ = 0;

      alternate_location_ = list_of_alternate_[alternate_location_counter_];

      HFSAT::TradingLocation_t primary_location_file_read_ =
          HFSAT::TradingLocationUtils::GetTradingLocationFromLOC_NAME(primary_location_);
      HFSAT::TradingLocation_t alternate_location_file_read_ =
          HFSAT::TradingLocationUtils::GetTradingLocationFromLOC_NAME(alternate_location_);

      HFSAT::BulkFileReader bulk_file_reader_primary_;
      HFSAT::BulkFileReader bulk_file_reader_alternate_;

      //  msec_diff_between_locations_ = HFSAT::TradingLocationUtils::GetMSecsBetweenTradingLocations(
      //  primary_location_file_read_, alternate_location_file_read_ ) ;

      switch (HFSAT::SecurityDefinitions::GetContractExchSource(shortcode_, input_date_)) {
        // TODO add support for Other Exchanges
        case HFSAT::kExchSourceCME: {
          std::string t_cme_filename_primary_ =
              HFSAT::CMELoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_, primary_location_file_read_);
          std::string t_cme_filename_alternate_ =
              HFSAT::CMELoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_, alternate_location_file_read_);

          std::cerr << " Primary : " << t_cme_filename_primary_ << "\n";
          std::cerr << " Secondary : " << t_cme_filename_alternate_ << "\n";

          if (t_cme_filename_primary_ == t_cme_filename_alternate_) {
            std::cerr << " Couldn't find data for given instrument at one location(or alternate location)."
                      << std::endl;
            continue;
          }

          bulk_file_reader_primary_.open(t_cme_filename_primary_);
          bulk_file_reader_alternate_.open(t_cme_filename_alternate_);

          if (!bulk_file_reader_primary_.is_open() || !bulk_file_reader_alternate_.is_open()) {
            std::cerr << "Could not open primary/secondary file\n";
            exit(-1);
          }

          size_t primary_available_len_ = -1;
          size_t alternate_available_len_ = -1;
          CME_MDS::CMECommonStruct primary_next_event_;
          CME_MDS::CMECommonStruct alternate_next_event_;
          uint32_t primary_instseq_no_ = -1;
          uint32_t alternate_instseq_no_ = -1;

          bool is_drop_in_chunk_at_primary_ = false;
          bool is_drop_in_chunk_at_alternate_ = false;

          while (true) {
            if (!drop_detected_at_primary_) {
              primary_available_len_ =
                  bulk_file_reader_primary_.read(&primary_next_event_, sizeof(CME_MDS::CMECommonStruct));

              if (primary_available_len_ >= sizeof(CME_MDS::CMECommonStruct)) {  // data not found in file

                if (packet_received_last_time_at_primary_.tv_sec == 0) {
                  packet_received_last_time_at_primary_ = primary_next_event_.time_;
                } else if (max_usec_difference_between_two_packets_at_primary_ <
                           get_usec_time_diff_between_two_timestamp(primary_next_event_.time_,
                                                                    packet_received_last_time_at_primary_)) {
                  max_usec_difference_between_two_packets_at_primary_ = get_usec_time_diff_between_two_timestamp(
                      primary_next_event_.time_, packet_received_last_time_at_primary_);
                  packet_received_start_time_at_primary_ = packet_received_last_time_at_primary_;
                  packet_received_end_time_at_primary_ = primary_next_event_.time_;
                }

                packet_received_last_time_at_primary_ = primary_next_event_.time_;

                if (primary_next_event_.msg_ == CME_MDS::CME_TRADE) {
                  primary_instseq_no_ = primary_next_event_.data_.cme_trds_.seqno_;

                  if (start_seq_at_primary_ == 0) start_seq_at_primary_ = primary_instseq_no_;
                  total_trds_packets_at_primary_++;

                } else if (primary_next_event_.msg_ == CME_MDS::CME_DELTA) {
                  primary_instseq_no_ = primary_next_event_.data_.cme_dels_.seqno_;

                  if (start_seq_at_primary_ == 0) start_seq_at_primary_ = primary_instseq_no_;
                  total_dels_packets_at_primary_++;
                }
              }
            }

            if (!drop_detected_at_alternate_) {
              alternate_available_len_ =
                  bulk_file_reader_alternate_.read(&alternate_next_event_, sizeof(CME_MDS::CMECommonStruct));

              if (alternate_available_len_ >= sizeof(CME_MDS::CMECommonStruct)) {  // data not found in file

                if (packet_received_last_time_at_alternate_.tv_sec == 0) {
                  packet_received_last_time_at_alternate_ = alternate_next_event_.time_;
                } else if (max_usec_difference_between_two_packets_at_alternate_ <
                           get_usec_time_diff_between_two_timestamp(alternate_next_event_.time_,
                                                                    packet_received_last_time_at_alternate_)) {
                  max_usec_difference_between_two_packets_at_alternate_ = get_usec_time_diff_between_two_timestamp(
                      alternate_next_event_.time_, packet_received_last_time_at_alternate_);
                  packet_received_start_time_at_alternate_ = packet_received_last_time_at_alternate_;
                  packet_received_end_time_at_alternate_ = alternate_next_event_.time_;
                }

                packet_received_last_time_at_alternate_ = alternate_next_event_.time_;

                if (alternate_next_event_.msg_ == CME_MDS::CME_TRADE) {
                  alternate_instseq_no_ = alternate_next_event_.data_.cme_trds_.seqno_;

                  if (start_seq_at_alternate_ == 0) start_seq_at_alternate_ = alternate_instseq_no_;
                  total_trds_packets_at_alternate_++;

                } else if (alternate_next_event_.msg_ == CME_MDS::CME_DELTA) {
                  alternate_instseq_no_ = alternate_next_event_.data_.cme_dels_.seqno_;

                  if (start_seq_at_alternate_ == 0) start_seq_at_alternate_ = alternate_instseq_no_;
                  total_dels_packets_at_alternate_++;
                }
              }
            }

            if (primary_available_len_ != sizeof(CME_MDS::CMECommonStruct)) {
              drop_detected_at_primary_ = true;  // EOF

              if (end_seq_at_primary_ == 0) end_seq_at_primary_ = primary_instseq_no_;  // last read sequence number
              primary_instseq_no_ = -1;  // rollover uint32(should be large enough then drops) so we always read from
                                         // the location which has some data
            }
            if (alternate_available_len_ != sizeof(CME_MDS::CMECommonStruct)) {
              drop_detected_at_alternate_ = true;  // EOF

              if (end_seq_at_alternate_ == 0)
                end_seq_at_alternate_ = alternate_instseq_no_;  // last read sequence number
              alternate_instseq_no_ = -1;  // rollover uint32(should be large enough then drops) so we always read from
                                           // the location which has some data
            }

            // no data left in any file
            if (primary_available_len_ != sizeof(CME_MDS::CMECommonStruct) &&
                alternate_available_len_ != sizeof(CME_MDS::CMECommonStruct)) {
              if (is_drop_in_chunk_at_primary_) {
                chunk_drop_end_time_at_primary_.push_back(alternate_next_event_.time_);
                total_packets_lost_in_chunk_at_primary_.push_back(packets_drop_in_chunk_at_primary_);
                is_drop_in_chunk_at_primary_ = false;
              }

              if (is_drop_in_chunk_at_alternate_) {
                chunk_drop_end_time_at_alternate_.push_back(primary_next_event_.time_);
                total_packets_lost_in_chunk_at_alternate_.push_back(packets_drop_in_chunk_at_alternate_);
                is_drop_in_chunk_at_alternate_ = false;
              }

              break;
            }

            if (primary_instseq_no_ < alternate_instseq_no_) {
              if (!is_drop_in_chunk_at_alternate_) {
                std::cout << "@@ New Drop @@" << std::endl;
                chunk_drop_start_time_at_alternate_.push_back(primary_next_event_.time_);
                total_chunk_drops_at_alternate_++;
              }

              if (is_drop_in_chunk_at_primary_) {
                chunk_drop_end_time_at_primary_.push_back(alternate_next_event_.time_);
                total_packets_lost_in_chunk_at_primary_.push_back(packets_drop_in_chunk_at_primary_);
              }

              if (primary_next_event_.msg_ == CME_MDS::CME_TRADE) {
                printCMETrade(primary_location_, primary_next_event_);

              } else if (primary_next_event_.msg_ == CME_MDS::CME_DELTA) {
                printCMEDelta(primary_location_, primary_next_event_);
              }

              packets_drop_in_chunk_at_primary_ = 0;
              packets_drop_in_chunk_at_alternate_++;
              drop_detected_at_alternate_ = true;
              drop_detected_at_primary_ = false;
              is_drop_in_chunk_at_primary_ = false;
              is_drop_in_chunk_at_alternate_ = true;

            } else if (alternate_instseq_no_ < primary_instseq_no_) {
              if (!is_drop_in_chunk_at_primary_) {
                std::cout << "@@ New Drop @@" << std::endl;
                chunk_drop_start_time_at_primary_.push_back(alternate_next_event_.time_);
                total_chunk_drops_at_primary_++;
              }

              if (is_drop_in_chunk_at_alternate_) {
                chunk_drop_end_time_at_alternate_.push_back(primary_next_event_.time_);
                total_packets_lost_in_chunk_at_alternate_.push_back(packets_drop_in_chunk_at_alternate_);
              }

              if (alternate_next_event_.msg_ == CME_MDS::CME_TRADE) {
                printCMETrade(alternate_location_, alternate_next_event_);

              } else if (alternate_next_event_.msg_ == CME_MDS::CME_DELTA) {
                printCMEDelta(alternate_location_, alternate_next_event_);
              }

              packets_drop_in_chunk_at_primary_++;
              packets_drop_in_chunk_at_alternate_ = 0;
              drop_detected_at_primary_ = true;
              drop_detected_at_alternate_ = false;
              is_drop_in_chunk_at_primary_ = true;
              is_drop_in_chunk_at_alternate_ = false;

            } else {
              if (primary_next_event_.msg_ != alternate_next_event_.msg_) {
                std::cerr << " Got Different Type Of Msg [TRADE vs DELTA] At Same Seq No : " << primary_instseq_no_
                          << std::endl;

              } else {
                if (primary_next_event_.msg_ == CME_MDS::CME_TRADE &&
                    !isCMETradeEqual(primary_next_event_, alternate_next_event_)) {
                  total_differences_++;

                  std::cout << "\n";
                  printCMETrade(primary_location_, primary_next_event_, true);
                  printCMETrade(alternate_location_, alternate_next_event_, true);
                  std::cout << "\n";

                } else if (primary_next_event_.msg_ == CME_MDS::CME_DELTA &&
                           !isCMEDeltaEqual(primary_next_event_, alternate_next_event_)) {
                  total_differences_++;

                  std::cout << "\n";
                  printCMEDelta(primary_location_, primary_next_event_, true);
                  printCMEDelta(alternate_location_, alternate_next_event_, true);
                  std::cout << "\n";
                }
              }

              if (is_drop_in_chunk_at_primary_) {
                chunk_drop_end_time_at_primary_.push_back(alternate_next_event_.time_);
                total_packets_lost_in_chunk_at_primary_.push_back(packets_drop_in_chunk_at_primary_);
              }

              if (is_drop_in_chunk_at_alternate_) {
                chunk_drop_end_time_at_alternate_.push_back(primary_next_event_.time_);
                total_packets_lost_in_chunk_at_alternate_.push_back(packets_drop_in_chunk_at_alternate_);
              }

              packets_drop_in_chunk_at_alternate_ = packets_drop_in_chunk_at_primary_ = 0;
              drop_detected_at_primary_ = drop_detected_at_alternate_ = false;
              is_drop_in_chunk_at_primary_ = is_drop_in_chunk_at_alternate_ = false;
            }
          }

          bulk_file_reader_primary_.close();
          bulk_file_reader_alternate_.close();
          total_bytes_at_primary_ =
              (total_dels_packets_at_primary_ + total_trds_packets_at_primary_) * sizeof(CME_MDS::CMECommonStruct);
          total_bytes_at_alternate_ =
              (total_dels_packets_at_alternate_ + total_trds_packets_at_alternate_) * sizeof(CME_MDS::CMECommonStruct);
        } break;

        case HFSAT::kExchSourceEUREX: {
          std::string t_eurex_filename_primary_ = "";

          if (HFSAT::UseEOBIData(primary_location_file_read_, input_date_, shortcode_)) {
            t_eurex_filename_primary_ = HFSAT::EOBIPriceFeedLoggedMessageFileNamer::GetName(
                t_exchange_symbol_, input_date_, primary_location_file_read_);
          } else {
            t_eurex_filename_primary_ = HFSAT::EUREXLoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_,
                                                                                    primary_location_file_read_);
          }

          std::string t_eurex_filename_alternate_ = HFSAT::EUREXLoggedMessageFileNamer::GetName(
              t_exchange_symbol_, input_date_, alternate_location_file_read_);

          std::cerr << " Primary : " << t_eurex_filename_primary_ << "\n";
          std::cerr << " Secondary : " << t_eurex_filename_alternate_ << "\n";

          if (t_eurex_filename_primary_ == t_eurex_filename_alternate_) {
            std::cerr << " Couldn't find data for given instrument at one location(or alternate location)."
                      << std::endl;
            exit(-1);
          }

          bulk_file_reader_primary_.open(t_eurex_filename_primary_);
          bulk_file_reader_alternate_.open(t_eurex_filename_alternate_);

          if (!bulk_file_reader_primary_.is_open() || !bulk_file_reader_alternate_.is_open()) {
            std::cerr << "Could not open primary/secondary file\n";
            exit(-1);
          }

          size_t primary_available_len_ = -1;
          size_t alternate_available_len_ = -1;
          EUREX_MDS::EUREXCommonStruct primary_next_event_;
          EUREX_MDS::EUREXCommonStruct alternate_next_event_;
          bool reached_eod_at_primary_ = false;
          bool reached_eod_at_alternate_ = false;

          bool is_drop_in_chunk_at_primary_ = false;
          bool is_drop_in_chunk_at_alternate_ = false;

          while (true) {
            if (!drop_detected_at_primary_) {
              primary_available_len_ =
                  bulk_file_reader_primary_.read(&primary_next_event_, sizeof(EUREX_MDS::EUREXCommonStruct));

              if (primary_available_len_ >= sizeof(EUREX_MDS::EUREXCommonStruct)) {
                if (primary_next_event_.msg_ == EUREX_MDS::EUREX_DELTA) total_dels_packets_at_primary_++;
                if (primary_next_event_.msg_ == EUREX_MDS::EUREX_TRADE) total_trds_packets_at_primary_++;

                if (packet_received_last_time_at_primary_.tv_sec == 0) {
                  packet_received_last_time_at_primary_ = primary_next_event_.time_;
                } else if (max_usec_difference_between_two_packets_at_primary_ <
                           get_usec_time_diff_between_two_timestamp(primary_next_event_.time_,
                                                                    packet_received_last_time_at_primary_)) {
                  max_usec_difference_between_two_packets_at_primary_ = get_usec_time_diff_between_two_timestamp(
                      primary_next_event_.time_, packet_received_last_time_at_primary_);
                  packet_received_start_time_at_primary_ = packet_received_last_time_at_primary_;
                  packet_received_end_time_at_primary_ = primary_next_event_.time_;
                }

                packet_received_last_time_at_primary_ = primary_next_event_.time_;

              } else {
                drop_detected_at_primary_ = true;
                reached_eod_at_primary_ = true;
              }
            }

            if (!drop_detected_at_alternate_) {
              alternate_available_len_ =
                  bulk_file_reader_alternate_.read(&alternate_next_event_, sizeof(EUREX_MDS::EUREXCommonStruct));

              if (alternate_available_len_ >= sizeof(EUREX_MDS::EUREXCommonStruct)) {
                if (alternate_next_event_.msg_ == EUREX_MDS::EUREX_DELTA) total_dels_packets_at_alternate_++;
                if (alternate_next_event_.msg_ == EUREX_MDS::EUREX_TRADE) total_trds_packets_at_alternate_++;

                if (packet_received_last_time_at_alternate_.tv_sec == 0) {
                  packet_received_last_time_at_alternate_ = alternate_next_event_.time_;
                } else if (max_usec_difference_between_two_packets_at_alternate_ <
                           get_usec_time_diff_between_two_timestamp(alternate_next_event_.time_,
                                                                    packet_received_last_time_at_alternate_)) {
                  max_usec_difference_between_two_packets_at_alternate_ = get_usec_time_diff_between_two_timestamp(
                      alternate_next_event_.time_, packet_received_last_time_at_alternate_);
                  packet_received_start_time_at_alternate_ = packet_received_last_time_at_alternate_;
                  packet_received_end_time_at_alternate_ = alternate_next_event_.time_;
                }

                packet_received_last_time_at_alternate_ = alternate_next_event_.time_;

              } else {
                drop_detected_at_alternate_ = true;
                reached_eod_at_alternate_ = true;
              }
            }

            if (reached_eod_at_primary_ && reached_eod_at_alternate_) break;

            if (reached_eod_at_primary_) {
              if (is_drop_in_chunk_at_primary_) {
                chunk_drop_end_time_at_primary_.push_back(alternate_next_event_.time_);
                total_packets_lost_in_chunk_at_primary_.push_back(packets_drop_in_chunk_at_primary_);
                is_drop_in_chunk_at_primary_ = false;
              }

              if (alternate_next_event_.msg_ == EUREX_MDS::EUREX_DELTA)
                printEUREXDelta(alternate_location_, alternate_next_event_);

              else if (alternate_next_event_.msg_ == EUREX_MDS::EUREX_TRADE)
                printEUREXTrade(alternate_location_, alternate_next_event_);

              drop_detected_at_alternate_ = false;

            } else if (reached_eod_at_alternate_) {
              if (is_drop_in_chunk_at_alternate_) {
                chunk_drop_end_time_at_alternate_.push_back(primary_next_event_.time_);
                total_packets_lost_in_chunk_at_alternate_.push_back(packets_drop_in_chunk_at_alternate_);
                is_drop_in_chunk_at_alternate_ = false;
              }

              if (primary_next_event_.msg_ == EUREX_MDS::EUREX_DELTA)
                printEUREXDelta(primary_location_, primary_next_event_);

              else if (primary_next_event_.msg_ == EUREX_MDS::EUREX_TRADE)
                printEUREXTrade(primary_location_, primary_next_event_);

              drop_detected_at_primary_ = false;

            } else {
              if (get_msec_diff_between_two_msg(alternate_next_event_, primary_next_event_) == -1) {
                if (!is_drop_in_chunk_at_primary_) {
                  std::cout << "@@ New Drop @@" << std::endl;
                  chunk_drop_start_time_at_primary_.push_back(alternate_next_event_.time_);
                  total_chunk_drops_at_primary_++;
                }

                if (alternate_next_event_.msg_ == EUREX_MDS::EUREX_DELTA)
                  printEUREXDelta(alternate_location_, alternate_next_event_);
                if (alternate_next_event_.msg_ == EUREX_MDS::EUREX_TRADE)
                  printEUREXTrade(alternate_location_, alternate_next_event_);

                packets_drop_in_chunk_at_alternate_ = 0;
                packets_drop_in_chunk_at_primary_++;
                drop_detected_at_primary_ = true;
                drop_detected_at_alternate_ = false;
                is_drop_in_chunk_at_alternate_ = false;
                is_drop_in_chunk_at_primary_ = true;

              } else {
                if (is_drop_in_chunk_at_primary_) {
                  chunk_drop_end_time_at_primary_.push_back(alternate_next_event_.time_);
                  total_packets_lost_in_chunk_at_primary_.push_back(packets_drop_in_chunk_at_primary_);
                }
                // TODO for eurex the difference is taken as packet drop
                drop_detected_at_primary_ = drop_detected_at_alternate_ = false;

                if (isEurexMsgEqual(primary_next_event_, alternate_next_event_)) {
                  if (is_drop_in_chunk_at_alternate_) {
                    chunk_drop_end_time_at_alternate_.push_back(primary_next_event_.time_);
                    total_packets_lost_in_chunk_at_alternate_.push_back(packets_drop_in_chunk_at_alternate_);
                  }

                  packets_drop_in_chunk_at_alternate_ = packets_drop_in_chunk_at_primary_ = 0;
                  is_drop_in_chunk_at_primary_ = false;
                  is_drop_in_chunk_at_alternate_ = false;
                  continue;
                }

                if (!is_drop_in_chunk_at_alternate_) {
                  std::cout << "@@ New Drop @@" << std::endl;
                  chunk_drop_start_time_at_alternate_.push_back(primary_next_event_.time_);
                  total_chunk_drops_at_alternate_++;
                }

                if (primary_next_event_.msg_ == EUREX_MDS::EUREX_DELTA)
                  printEUREXDelta(primary_location_, primary_next_event_);
                if (primary_next_event_.msg_ == EUREX_MDS::EUREX_TRADE)
                  printEUREXTrade(primary_location_, primary_next_event_);

                packets_drop_in_chunk_at_alternate_++;
                packets_drop_in_chunk_at_primary_ = 0;
                drop_detected_at_alternate_ = true;
                drop_detected_at_primary_ = false;

                is_drop_in_chunk_at_alternate_ = true;
                is_drop_in_chunk_at_primary_ = false;
              }
            }
          }

          bulk_file_reader_primary_.close();
          bulk_file_reader_alternate_.close();
          total_bytes_at_primary_ =
              (total_dels_packets_at_primary_ + total_trds_packets_at_primary_) * sizeof(EUREX_MDS::EUREXCommonStruct);
          total_bytes_at_alternate_ = (total_dels_packets_at_alternate_ + total_trds_packets_at_alternate_) *
                                      sizeof(EUREX_MDS::EUREXCommonStruct);

        } break;

        case HFSAT::kExchSourceTMX: {
          std::string t_tmx_filename_primary_ =
              HFSAT::TMXLoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_, primary_location_file_read_);

          std::cerr << " Primary : " << t_tmx_filename_primary_ << "\n";

          bulk_file_reader_primary_.open(t_tmx_filename_primary_);

          if (!bulk_file_reader_primary_.is_open()) {
            std::cerr << "Could not open primary file\n";
            exit(-1);
          }

          size_t primary_available_len_ = -1;
          TMX_MDS::TMXCommonStruct primary_next_event_;

          while (true) {
            primary_available_len_ =
                bulk_file_reader_primary_.read(&primary_next_event_, sizeof(TMX_MDS::TMXCommonStruct));

            if (primary_available_len_ >= sizeof(TMX_MDS::TMXCommonStruct)) {
              if (primary_next_event_.msg_ == TMX_MDS::TMX_QUOTE) total_dels_packets_at_primary_++;  // book and quote
              if (primary_next_event_.msg_ == TMX_MDS::TMX_BOOK) total_dels_packets_at_primary_++;
              if (primary_next_event_.msg_ == TMX_MDS::TMX_TRADE) total_trds_packets_at_primary_++;

              if (packet_received_last_time_at_primary_.tv_sec == 0) {
                packet_received_last_time_at_primary_ = primary_next_event_.time_;
              } else if (max_usec_difference_between_two_packets_at_primary_ <
                         get_usec_time_diff_between_two_timestamp(primary_next_event_.time_,
                                                                  packet_received_last_time_at_primary_)) {
                max_usec_difference_between_two_packets_at_primary_ = get_usec_time_diff_between_two_timestamp(
                    primary_next_event_.time_, packet_received_last_time_at_primary_);
                packet_received_start_time_at_primary_ = packet_received_last_time_at_primary_;
                packet_received_end_time_at_primary_ = primary_next_event_.time_;
              }

              packet_received_last_time_at_primary_ = primary_next_event_.time_;

            } else
              break;
          }

          total_bytes_at_primary_ =
              (total_dels_packets_at_primary_ + total_trds_packets_at_primary_) * sizeof(TMX_MDS::TMXCommonStruct);
          bulk_file_reader_primary_.close();
        } break;

        case HFSAT::kExchSourceBMF:
        case HFSAT::kExchSourceNTP: {
          std::string t_tmx_filename_primary_ =
              HFSAT::NTPLoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_, primary_location_file_read_);

          std::cerr << " Primary : " << t_tmx_filename_primary_ << "\n";

          bulk_file_reader_primary_.open(t_tmx_filename_primary_);

          if (!bulk_file_reader_primary_.is_open()) {
            std::cerr << "Could not open primary file\n";
            exit(-1);
          }

          size_t primary_available_len_ = -1;
          NTP_MDS::NTPCommonStruct primary_next_event_;

          while (true) {
            primary_available_len_ =
                bulk_file_reader_primary_.read(&primary_next_event_, sizeof(NTP_MDS::NTPCommonStruct));

            if (primary_available_len_ >= sizeof(NTP_MDS::NTPCommonStruct)) {
              if (primary_next_event_.msg_ == NTP_MDS::NTP_DELTA) total_dels_packets_at_primary_++;  // book and quote
              if (primary_next_event_.msg_ == NTP_MDS::NTP_TRADE) total_trds_packets_at_primary_++;

              if (packet_received_last_time_at_primary_.tv_sec == 0) {
                packet_received_last_time_at_primary_ = primary_next_event_.time_;
              } else if (max_usec_difference_between_two_packets_at_primary_ <
                         get_usec_time_diff_between_two_timestamp(primary_next_event_.time_,
                                                                  packet_received_last_time_at_primary_)) {
                max_usec_difference_between_two_packets_at_primary_ = get_usec_time_diff_between_two_timestamp(
                    primary_next_event_.time_, packet_received_last_time_at_primary_);
                packet_received_start_time_at_primary_ = packet_received_last_time_at_primary_;
                packet_received_end_time_at_primary_ = primary_next_event_.time_;
              }

              packet_received_last_time_at_primary_ = primary_next_event_.time_;

            } else
              break;
          }

          total_bytes_at_primary_ =
              (total_dels_packets_at_primary_ + total_trds_packets_at_primary_) * sizeof(NTP_MDS::NTPCommonStruct);
          bulk_file_reader_primary_.close();
        } break;

        default: { } break; }

      found_diff_ = false;

      std::ostringstream t_temp_mail_body_;

      HFSAT::ExchSource_t exchange_source_ = HFSAT::SecurityDefinitions::GetContractExchSource(shortcode_, input_date_);

      if (exchange_source_ != HFSAT::kExchSourceBMF && exchange_source_ != HFSAT::kExchSourceNTP &&
          exchange_source_ != HFSAT::kExchSourceTMX) {
        // check to see if any threashold were crossed
        if (((packet_received_end_time_at_primary_.tv_sec - packet_received_start_time_at_primary_.tv_sec) >
                 TIME_PERIOD_LIMIT ||
             (packet_received_end_time_at_alternate_.tv_sec - packet_received_start_time_at_alternate_.tv_sec) >
                 TIME_PERIOD_LIMIT) ||
            (((total_dels_packets_at_primary_ + total_trds_packets_at_primary_) -
              (total_dels_packets_at_alternate_ + total_trds_packets_at_alternate_)) /
                 (total_dels_packets_at_primary_ + total_trds_packets_at_primary_) * 100 >
             PACKET_DROP_LIMIT_PERC) ||
            ((total_bytes_at_primary_ - total_bytes_at_alternate_) / total_bytes_at_primary_ * 100 > SIZE_LIMIT_PERC) ||
            (total_chunk_drops_at_primary_ > CHUNK_DROP_LIMIT || total_chunk_drops_at_alternate_ > CHUNK_DROP_LIMIT) ||
            (total_differences_ / (total_dels_packets_at_primary_ + total_trds_packets_at_primary_) * 100 >
             DIFF_LIMIT_PERC))

        {
          found_diff_ = true;
          t_temp_mail_body_
              << "<b>" << shortcode_ << " " << primary_location_ << ":" << alternate_location_ << "</b>"
              << " " << (total_bytes_at_primary_) / 1024 / 1024 << ":" << (total_bytes_at_alternate_) / 1024 / 1024
              << " " << (total_dels_packets_at_primary_ + total_trds_packets_at_primary_) << ":"
              << (total_dels_packets_at_alternate_ + total_trds_packets_at_alternate_) << " "
              << total_chunk_drops_at_primary_ << ":" << total_chunk_drops_at_alternate_ << " "
              << (packet_received_end_time_at_primary_.tv_sec - packet_received_start_time_at_primary_.tv_sec) << ":"
              << (packet_received_end_time_at_alternate_.tv_sec - packet_received_start_time_at_alternate_.tv_sec)
              << " " << total_differences_ << "<br/>";
        }

      } else {
        if ((packet_received_end_time_at_primary_.tv_sec - packet_received_start_time_at_primary_.tv_sec) >
            TIME_PERIOD_LIMIT) {
          found_diff_ = true;

          t_temp_mail_body_ << "<b>" << shortcode_ << " " << primary_location_ << "</b>"
                            << " " << (total_bytes_at_primary_) / 1024 / 1024 << " "
                            << (total_dels_packets_at_primary_ + total_trds_packets_at_primary_) << " "
                            << (packet_received_end_time_at_primary_.tv_sec -
                                packet_received_start_time_at_primary_.tv_sec) << "<br/>";
        }
      }

      email_text_ += t_temp_mail_body_.str();

      std::cerr << "\n\n\n";
      std::cerr << "=======================Data Comparison Summary=======================\n\n";
      std::cerr << " Total Bytes At " << primary_location_ << " : " << (total_bytes_at_primary_) / 1024 / 1024
                << " Mbytes \n";
      std::cerr << " Total Bytes At " << alternate_location_ << " : " << (total_bytes_at_alternate_) / 1024 / 1024
                << " Mbytes \n\n";
      std::cerr << " Total Packets At " << primary_location_ << " Delta : " << total_dels_packets_at_primary_
                << " Trade : " << total_trds_packets_at_primary_
                << " Total : " << (total_dels_packets_at_primary_ + total_trds_packets_at_primary_) << std::endl;
      std::cerr << " Total Packets At " << alternate_location_ << " Delta : " << total_dels_packets_at_alternate_
                << " Trade : " << total_trds_packets_at_alternate_
                << " Total : " << (total_dels_packets_at_alternate_ + total_trds_packets_at_alternate_) << std::endl;
      std::cerr << "\n";
      std::cerr << " Total Chunk Drop At " << primary_location_ << " : " << total_chunk_drops_at_primary_ << std::endl;
      std::cerr << " Total Chunk Drop At " << alternate_location_ << " : " << total_chunk_drops_at_alternate_ << "\n\n";
      std::cerr << " Max Silent Period At " << primary_location_
                << " , Start : " << packet_received_start_time_at_primary_.tv_sec << "."
                << packet_received_start_time_at_primary_.tv_usec
                << " End : " << packet_received_end_time_at_primary_.tv_sec << "."
                << packet_received_end_time_at_primary_.tv_usec << "\n";
      std::cerr << " Max Silent Period At " << alternate_location_
                << " , Start : " << packet_received_start_time_at_alternate_.tv_sec << "."
                << packet_received_start_time_at_alternate_.tv_usec
                << " End : " << packet_received_end_time_at_alternate_.tv_sec << "."
                << packet_received_end_time_at_alternate_.tv_usec << "\n\n";
      std::cerr << " Total Differences Apart from Drops : " << total_differences_ << "\n\n";
      std::cerr << " Sequence Range At " << primary_location_ << " Start : " << start_seq_at_primary_
                << " End : " << end_seq_at_primary_ << "\n";
      std::cerr << " Sequence Range At " << alternate_location_ << " Start : " << start_seq_at_alternate_
                << " End : " << end_seq_at_alternate_ << "\n";

      std::cerr << "\n\n\n";
      std::cerr << "=======================Detailed Drop At " << alternate_location_ << "=======================\n\n";

      for (auto i = 0u; i < total_packets_lost_in_chunk_at_alternate_.size(); i++) {
        std::cerr << " Drop " << std::setw(3) << std::setfill('0') << i << " : "
                  << chunk_drop_start_time_at_alternate_[i].tv_sec << "." << std::setw(6) << std::setfill('0')
                  << chunk_drop_start_time_at_alternate_[i].tv_usec;

        std::cerr << " to " << chunk_drop_end_time_at_alternate_[i].tv_sec << "." << std::setw(6) << std::setfill('0')
                  << chunk_drop_end_time_at_alternate_[i].tv_usec;

        std::cerr << " lost " << total_packets_lost_in_chunk_at_alternate_[i] << std::endl;
      }

      std::cerr << "\n\n\n";
      std::cerr << "=======================Detailed Drop At " << primary_location_ << "=======================\n\n";

      for (auto i = 0u; i < total_packets_lost_in_chunk_at_primary_.size(); i++) {
        std::cerr << " Drop " << std::setw(3) << std::setfill('0') << i << " : "
                  << chunk_drop_start_time_at_primary_[i].tv_sec << "." << std::setw(6) << std::setfill('0')
                  << chunk_drop_start_time_at_primary_[i].tv_usec;

        std::cerr << " to " << chunk_drop_end_time_at_primary_[i].tv_sec << "." << std::setw(6) << std::setfill('0')
                  << chunk_drop_end_time_at_primary_[i].tv_usec;

        std::cerr << " lost " << total_packets_lost_in_chunk_at_primary_[i] << std::endl;
      }

      std::cerr << "\n\n\n\n";
    }

    if (found_diff_) email_text_ += "<br/>";
  }

  sendEmailNotification(email_text_);

  return 0;
}
