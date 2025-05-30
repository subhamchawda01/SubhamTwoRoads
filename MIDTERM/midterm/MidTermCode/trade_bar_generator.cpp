// =====================================================================================
//
//       Filename:  trade_bar_generator.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  01/05/2016 12:10:57 PM
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

#include "midterm/MidTerm/trade_bar_generator.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/CDef/nse_security_definition.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

// Set the Output struct to default values if num_trades_ == 0
void TradeBarGenerator::FillWithDefaultValues(Output &out_) {
  out_.start_time = INVALID_TIME;
  out_.close_time = INVALID_TIME;
  out_.open_price = INVALID_PRICE;
  out_.close_price = INVALID_PRICE;
  out_.low_price = INVALID_PRICE;
  out_.high_price = INVALID_PRICE;
}

// Print the completed 1-minute bar data into a string
std::string TradeBarGenerator::PrintCompletedBucket(int bucket_) {
  if (bucket_ == -1)
    return "";
  std::ostringstream t_temp_oss;
  string ready_string = "";
  string delim_ = "^";
  typedef map<string, map<int, Output>>::iterator it_type;
  for (it_type iterator = map_price_.begin(); iterator != map_price_.end();
       iterator++) {
    typedef map<int, Output>::iterator it_typeX;
    for (it_typeX itX = iterator->second.begin(); itX != iterator->second.end();
         itX++) {
      if (itX->first != bucket_)
        continue;
      if (itX->second.trades == 0)
        FillWithDefaultValues(itX->second);
      t_temp_oss << itX->first << "|" << iterator->first << "|"
                 << (itX->second).ToString() << "^";
      // dbglogger_ << itX->first << "|" << iterator->first << "|" <<
      // (itX->second).ToString() << "\n";
    }
  }
  return t_temp_oss.str();
}

void TradeBarGenerator::LogCompletedBucket(int bucket_) {
  if (bucket_ == -1)
    return;
  typedef map<string, map<int, Output>>::iterator it_type;
  for (it_type iterator = map_price_.begin(); iterator != map_price_.end();
       iterator++) {
    typedef map<int, Output>::iterator it_typeX;
    ofstream myfile;
    string outputpath = NSE_MIDTERM_BARDATA_PATH;
    vector<string> tokens_;
    // iterator->first would be like : NIFTY_FF_0_0 or NIFTY_PE_9450.000000_0 or
    // BANKNIFTY_PE_22200.000000_0_W
    HFSAT::PerishableStringTokenizer::StringSplit(iterator->first, '_',
                                                  tokens_);
    string outname = tokens_[0];

    if (tokens_[1] == "FF") {
      outputpath = NSE_MIDTERM_BARDATA_PATH;
    }

    else if (iterator->first[iterator->first.size() - 1] == 'W') {
      outputpath = NSE_MIDTERM_WEEKLY_OPTION_BARDATA_PATH;
    } else {
      outputpath = NSE_MIDTERM_OPTION_BARDATA_PATH;
    }
    myfile.open((outputpath + outname).c_str(), ios_base::app);
    for (it_typeX itX = iterator->second.begin(); itX != iterator->second.end();
         itX++) {
      if (itX->first != bucket_) {
        continue;
      }
      if (itX->second.trades == 0)
        FillWithDefaultValues(itX->second);
      // Timestamp
      myfile << itX->first << "\t";
      // Hash_String
      myfile << iterator->first << "\t";
      // First trade time
      myfile << itX->second.start_time << "\t";
      // Last Trade Time
      myfile << itX->second.close_time << "\t";
      myfile << itX->second.expiry << "\t";
      myfile << itX->second.open_price << "\t";
      myfile << itX->second.close_price << "\t";
      myfile << itX->second.low_price << "\t";
      myfile << itX->second.high_price << "\t";
      myfile << itX->second.volume << "\t";
      myfile << itX->second.trades << "\t";
      myfile << itX->second.bid_px << "\t";
      myfile << itX->second.ask_px << "\n";
      myfile.flush();
    }
  }
}

void TradeBarGenerator::UpdatePriceMap(std::string hash_string,
                                       int bucket_time_, double price_,
                                       double qty_, std::string exp_,
                                       double best_bid_px_,
                                       double best_ask_px_) {
  //////////////////// ADD TO MAP //////////////
  // Case when no map from a particular hash string
  if (map_price_.find(hash_string) == map_price_.end()) {
    map<int, Output> prices;
    Output metrics;
    metrics.open_price = price_;
    metrics.close_price = price_;
    metrics.low_price = price_;
    metrics.high_price = price_;
    metrics.trades = 1;
    metrics.volume = qty_;
    metrics.start_time = watch_.tv().tv_sec;
    metrics.close_time = watch_.tv().tv_sec;
    metrics.expiry = exp_;
    metrics.bid_px = best_bid_px_;
    metrics.ask_px = best_ask_px_;

    prices.insert(make_pair(bucket_time_, metrics));
    map_price_.insert(make_pair(hash_string, prices));
  }
  // Case when hash string exists in the map
  else {
    // Case when the timestamp does not exist in the smaller map
    if (map_price_[hash_string].find(bucket_time_) ==
        map_price_[hash_string].end()) {
      Output metrics;
      metrics.open_price = price_;
      metrics.close_price = price_;
      metrics.low_price = price_;
      metrics.high_price = price_;
      metrics.volume = qty_;
      metrics.trades = 1;
      metrics.start_time = watch_.tv().tv_sec;
      metrics.close_time = watch_.tv().tv_sec;
      metrics.expiry = exp_;
      metrics.bid_px = best_bid_px_;
      metrics.ask_px = best_ask_px_;

      map_price_[hash_string].insert(make_pair(bucket_time_, metrics));
    }
    // Case when everything in place. COMPLEX
    else {
      Output metrics = map_price_[hash_string][bucket_time_];
      if (watch_.tv().tv_sec < metrics.start_time) {
        metrics.start_time = watch_.tv().tv_sec;
        metrics.open_price = price_;
      }
      if (watch_.tv().tv_sec > metrics.close_time) {
        metrics.close_time = watch_.tv().tv_sec;
        metrics.close_price = price_;
        metrics.bid_px = best_bid_px_;
        metrics.ask_px = best_ask_px_;
      }
      if (price_ > metrics.high_price)
        metrics.high_price = price_;
      if (price_ < metrics.low_price)
        metrics.low_price = price_;
      metrics.volume += qty_;
      metrics.trades++;
      // No need to set expiry here
      map_price_[hash_string][bucket_time_] = metrics;
    }
  }
}

void TradeBarGenerator::OnRawTradePrint(
    unsigned int const security_id,
    HFSAT::TradePrintInfo const &trade_print_info,
    HFSAT::MarketUpdateInfo const &market_update_info) {
  // NIFTY67345
  std::string symbol = sec_name_indexer_.GetSecurityNameFromId(security_id);
  // NSE_NIFTY_FUT_20151029
  std::string internal_symbol =
      HFSAT::NSESecurityDefinitions::ConvertExchSymboltoDataSourceNameGeneric(
          symbol);
  // NSE_NIFTY_FUT0, for options: NSE_NIFTY_<C/P><0/1/2>_<A/O/I><0-10>
  std::string shortcode = sec_name_indexer_.GetShortcodeFromId(security_id);

  if (INVALID_INTERNAL_SYMBOL == internal_symbol) {
    DBGLOG_CLASS_FUNC_LINE_ERROR
        << "RECEVIED INVALID INTERNAL SYMBOL FOR : " << symbol
        << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    return;
  }

  // Get expiry number and append that to get the hash string
  vector<string> tokens_from_shortcode_;
  HFSAT::PerishableStringTokenizer::StringSplit(shortcode, '_',
                                                tokens_from_shortcode_);
  string temp = tokens_from_shortcode_[2];
  // Like FUT0 or C0 or P0
  string expiry_id = temp.substr(temp.size() - 1, 1);
  int exp_num = atoi(expiry_id.c_str());

  std::string hash_string;
  std::string other_hash_string = "";

  bool is_option_ = (tokens_from_shortcode_.size() >= 4) ? true : false;
  string exp_;
  // Case when FUTURE
  if (!is_option_) {
    hash_string = tokens_from_shortcode_[1] + "_FF_0_" + expiry_id;
    exp_ = to_string(
        HFSAT::NSESecurityDefinitions::GetExpiryFromShortCode(shortcode));
  }
  // Case when OPTION
  else {
    string option_type;
    if (tokens_from_shortcode_[2][0] == 'C') {
      option_type = "CE";
    } else if (tokens_from_shortcode_[2][0] == 'P') {
      option_type = "PE";
    } else {
      DBGLOG_CLASS_FUNC_LINE_ERROR
          << "ALERT : Received an invalid option code : "
          << " for symbol: " << symbol << " ... Not processing further"
          << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
      return;
    }
    string strike_ = to_string(
        HFSAT::NSESecurityDefinitions::GetStrikePriceFromShortCodeGeneric(
            shortcode));
    bool is_weekly_option =
        HFSAT::NSESecurityDefinitions::IsWeeklyOption(shortcode);

    hash_string = tokens_from_shortcode_[1] + "_" + option_type + "_" +
                  strike_ + "_" + expiry_id;

    // Get ATM option for underlying
    string ticker_ = tokens_from_shortcode_[1];
    string atm_shortcode_ = "NSE_" + ticker_ + "_C" + expiry_id + "_A";

    if (is_weekly_option) {
      hash_string += "_W";
      atm_shortcode_ += "_W";
    }

    // Ignore weekly option if greater than W_1
    if (is_weekly_option && exp_num > 1)
      return;

    exp_ = to_string(
        HFSAT::NSESecurityDefinitions::GetExpiryFromShortCode(atm_shortcode_));

    // Special handling for BANKNIFTY Weekly Options
    if (ticker_ == "BANKNIFTY" || ticker_ == "NIFTY" || ticker_ == "FINNIFTY" || ticker_ == "MIDCPNIFTY") {
      if (is_pre_monthly_expiry_week) {
        // Near expiry has to be 0_W, Next has to be 0
        // Ignore shortcodes of _W which are not nearest
        if (is_weekly_option && exp_num > 0)
          return;
        // Record OPT_0 as next weekly option
        if (!is_weekly_option && exp_num == 0)
          other_hash_string = tokens_from_shortcode_[1] + "_" + option_type +
                              "_" + strike_ + "_1_W";
      }
      if (is_monthly_expiry_week) {
        // Near expiry has to be 0, Next has to be 0_W
        // Ignore shortcodes of _W which are not nearest
        if (is_weekly_option && exp_num > 0)
          return;
        // Record OPT_0 as nearest weekly option also
        if (!is_weekly_option && exp_num == 0)
          other_hash_string = hash_string + "_W";
        // Change 0_W to next expiry(1_W)
        if (is_weekly_option && exp_num == 0)
          hash_string = tokens_from_shortcode_[1] + "_" + option_type + "_" +
                        strike_ + "_1_W";
      }
    }
  }

#if DBG_LEVEL_SEVERE
#endif

  int bucket_time_ =
      watch_.tv().tv_sec - (watch_.tv().tv_sec % LOGPERIOD_LISTENER_SIZE);

  if (bucket_time_ != bucket_) {
    for (auto each_shc_BA : shc_2_BA) {
      if (map_price_.find(each_shc_BA.first) == map_price_.end() ||
          map_price_[each_shc_BA.first].find(bucket_) ==
              map_price_[each_shc_BA.first].end()) {
        map<int, Output> prices;
        Output metrics;
        metrics.trades = 0;
        metrics.volume = 0;
        metrics.expiry = each_shc_BA.second.Expiry;
        metrics.bid_px = each_shc_BA.second.Bid_Px;
        metrics.ask_px = each_shc_BA.second.Ask_Px;
        prices.insert(make_pair(bucket_, metrics));
        map_price_.insert(make_pair(each_shc_BA.first, prices));
      }
    }

    switch (operating_mode_) {
    case Mode::kNSELoggerMode: // Simply Logs data
    {
      dbglogger_ << "Logging...\n";
      dbglogger_.DumpCurrentBuffer();
      LogCompletedBucket(bucket_);
      map_price_.clear();
    } break;
    case Mode::kNSEServerMode: // Acts As Server In Live To Provide Data
    {
      // dbglogger_ << "----SENDING MINUTE BAR-DATA TO STRATS----\n";
      std::string bucket_string_data = PrintCompletedBucket(bucket_);
      // dbglogger_ << "------------------DONE-------------------\n";
      int32_t length_of_data =
          bucket_string_data.length() + sizeof(int32_t) + sizeof('|');
      std::ostringstream t_temp_oss;
      t_temp_oss << length_of_data << '|' << bucket_string_data;

      // dbglogger_ << "LENGTH OF DATA : " << length_of_data << "\n";

      // A stringent check here, Won't send data if buffer is more than expected
      // size, or empty

      int32_t start_pos = 0;

      if (length_of_data <= (int32_t)(sizeof(int32_t) + sizeof('|'))){
          break;
      }
      //std::cout<< t_temp_oss.str();
      while(start_pos < length_of_data){
    	  //int32_t length_of_data_being_sent = min(NSE_MIDTERM_DATA_BUFFER_LENGTH, length_of_data - start_pos);
          int32_t length_of_data_being_sent = NSE_MIDTERM_DATA_BUFFER_LENGTH < length_of_data - start_pos ? NSE_MIDTERM_DATA_BUFFER_LENGTH : length_of_data - start_pos;
    	  memset((void *)packet_buffer_, 0, NSE_MIDTERM_DATA_BUFFER_LENGTH);
    	  memcpy((void *)packet_buffer_, (void *)(t_temp_oss.str().substr
    	   	  (start_pos, length_of_data_being_sent)).c_str(), length_of_data_being_sent);
    	  //std::string packet_buffer_ = (void *)(t_temp_oss.str().substr(start_pos, length_of_data_being_sent));
          //std::cout<< packet_buffer_;
    	  tcp_server_manager_->RespondToAllClients(packet_buffer_, length_of_data_being_sent);
    	  start_pos += NSE_MIDTERM_DATA_BUFFER_LENGTH;
      }


      /*
      if (length_of_data > NSE_MIDTERM_DATA_BUFFER_LENGTH ||
          length_of_data <= (int32_t)(sizeof(int32_t) + sizeof('|')))
        break;

      memset((void *)packet_buffer_, 0, NSE_MIDTERM_DATA_BUFFER_LENGTH);
      memcpy((void *)packet_buffer_, (void *)t_temp_oss.str().c_str(),
             length_of_data);

      // Notify Clients About Data
      tcp_server_manager_->RespondToAllClients(packet_buffer_, length_of_data);
      */
      map_price_.clear();
    } break;
    case Mode::kNSEOfflineMode: // Offline Processing For Historical Data
    {
    } break;
    case Mode::kNSEHybridMode: // Logs As Well As Sends Data to clients, Used
                               // For Initial Stage Deployment
      {
        std::string bucket_string_data = PrintCompletedBucket(bucket_);
        int32_t length_of_data =
            bucket_string_data.length() + sizeof(int32_t) + sizeof('|');
        std::ostringstream t_temp_oss;
        t_temp_oss << length_of_data << '|' << bucket_string_data;

        // dbglogger_ << "LENGTH OF DATA : " << length_of_data << "\n";

        // A stringent check here, Won't send data if buffer is more than
        // expected
        // size, or empty
        if (length_of_data > NSE_MIDTERM_DATA_BUFFER_LENGTH ||
            length_of_data <= (int32_t)(sizeof(int32_t) + sizeof('|')))
          break;

        memset((void *)packet_buffer_, 0, NSE_MIDTERM_DATA_BUFFER_LENGTH);
        memcpy((void *)packet_buffer_, (void *)t_temp_oss.str().c_str(),
               length_of_data);

        // Notify Clients About Data
        tcp_server_manager_->RespondToAllClients(packet_buffer_,
                                                 length_of_data);

        dbglogger_ << bucket_string_data << "\n";
      }
      break;
    default: {
      dbglogger_ << "SHOULD NEVER REACH HERE, CONSTRUCTION OF THE OBJECT "
                    "SHOULD HAVE FAILED\n";
      dbglogger_.DumpCurrentBuffer();
      exit(-1);
    } break;
    }

    bucket_ = bucket_time_;
  }

  double price_ = trade_print_info.trade_price_;
  double qty_ = trade_print_info.size_traded_;
  double best_bid_px_ = market_update_info.bestbid_price_;
  double best_ask_px_ = market_update_info.bestask_price_;
  if (price_ <= 0) {
    dbglogger_ << "ALERT : Received a price of " << price_
               << " with quantity : " << qty_ << " at time : " << bucket_time_
               << "\n";
    dbglogger_.DumpCurrentBuffer();
    return;
  }

  BA_struct ba;
  ba.Bid_Px = best_bid_px_;
  ba.Ask_Px = best_ask_px_;
  ba.Expiry = exp_;
  shc_2_BA[hash_string] = ba;
  UpdatePriceMap(hash_string, bucket_time_, price_, qty_, exp_, best_bid_px_,
                 best_ask_px_);
  if (other_hash_string != "" &&  other_hash_string != hash_string) {
    shc_2_BA[other_hash_string] = ba;
    UpdatePriceMap(other_hash_string, bucket_time_, price_, qty_, exp_,
                   best_bid_px_, best_ask_px_);
  }
}

void TradeBarGenerator::OnMarketUpdate(
    unsigned int const security_id,
    HFSAT::MarketUpdateInfo const &market_update_info) {
  // Update Watch time
  last_data_received_time = watch_.tv().tv_sec;
  // NIFTY67345
  std::string symbol = sec_name_indexer_.GetSecurityNameFromId(security_id);
  // NSE_NIFTY_FUT_20151029
  std::string internal_symbol =
      HFSAT::NSESecurityDefinitions::ConvertExchSymboltoDataSourceNameGeneric(
          symbol);
  // NSE_NIFTY_FUT0, for options: NSE_NIFTY_<C/P><0/1/2>_<A/O/I><0-10>
  std::string shortcode = sec_name_indexer_.GetShortcodeFromId(security_id);

  if (INVALID_INTERNAL_SYMBOL == internal_symbol) {
    DBGLOG_CLASS_FUNC_LINE_ERROR
        << "RECEVIED INVALID INTERNAL SYMBOL FOR : " << symbol
        << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    return;
  }

  // Get expiry number and append that to get the hash string
  vector<string> tokens_from_shortcode_;
  HFSAT::PerishableStringTokenizer::StringSplit(shortcode, '_',
                                                tokens_from_shortcode_);
  string temp = tokens_from_shortcode_[2];
  // Like FUT0 or C0 or P0
  string expiry_id = temp.substr(temp.size() - 1, 1);
  int exp_num = atoi(expiry_id.c_str());
  string hash_string;
  string other_hash_string = "";
  bool is_option_ = (tokens_from_shortcode_.size() >= 4) ? true : false;
  string exp_;
  // Case when FUTURE
  if (!is_option_) {
    hash_string = tokens_from_shortcode_[1] + "_FF_0_" + expiry_id;
    exp_ = to_string(
        HFSAT::NSESecurityDefinitions::GetExpiryFromShortCode(shortcode));
  }
  // Case when OPTION
  else {
    string option_type;

    if (tokens_from_shortcode_[2][0] == 'C') {
      option_type = "CE";
    } else if (tokens_from_shortcode_[2][0] == 'P') {
      option_type = "PE";
    } else {
      DBGLOG_CLASS_FUNC_LINE_ERROR
          << "ALERT : Received an invalid option code : "
          << " for symbol: " << symbol << " ... Not processing further"
          << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
      return;
    }
    string strike_ = to_string(
        HFSAT::NSESecurityDefinitions::GetStrikePriceFromShortCodeGeneric(
            shortcode));
    bool is_weekly_option =
        HFSAT::NSESecurityDefinitions::IsWeeklyOption(shortcode);

    hash_string = tokens_from_shortcode_[1] + "_" + option_type + "_" +
                  strike_ + "_" + expiry_id;

    // Get ATM option for underlying
    string ticker_ = tokens_from_shortcode_[1];
    string atm_shortcode_ = "NSE_" + ticker_ + "_C" + expiry_id + "_A";

    if (is_weekly_option) {
      hash_string += "_W";
      atm_shortcode_ += "_W";
    }
    // Ignore weekly option if greater than W_1
    if (is_weekly_option && exp_num > 1)
      return;

    exp_ = to_string(
        HFSAT::NSESecurityDefinitions::GetExpiryFromShortCode(atm_shortcode_));
    // Special handling for BANKNIFTY Weekly Options
    if (ticker_ == "BANKNIFTY" || ticker_ == "NIFTY" || ticker_ == "FINNIFTY" || ticker_ == "MIDCPNIFTY") {
      if (is_pre_monthly_expiry_week) {
        // Near expiry has to be 0_W, Next has to be 0
        // Ignore shortcodes of _W which are not nearest
        if (is_weekly_option && exp_num > 0)
          return;
        // Record OPT_0 as next weekly option
        if (!is_weekly_option && exp_num == 0)
          other_hash_string = tokens_from_shortcode_[1] + "_" + option_type +
                              "_" + strike_ + "_1_W";
      }
      if (is_monthly_expiry_week) {
        // Near expiry has to be 0, Next has to be 0_W
        // Ignore shortcodes of _W which are not nearest
        if (is_weekly_option && exp_num > 0)
          return;
        // Record OPT_0 as nearest weekly option also
        if (!is_weekly_option && exp_num == 0)
          other_hash_string = hash_string + "_W";
        // Change 0_W to next expiry(1_W)
        if (is_weekly_option && exp_num == 0)
          hash_string = tokens_from_shortcode_[1] + "_" + option_type + "_" +
                        strike_ + "_1_W";
      }
    }
  }
  double best_bid_px_ = market_update_info.bestbid_price_;
  double best_ask_px_ = market_update_info.bestask_price_;
  // DBGLOG_CLASS_FUNC_LINE_ERROR << market_update_info.ToString() << DBGLOG_ENDL_FLUSH;
  if (best_bid_px_ <= 0 || best_ask_px_ <= 0) {
    // we could have single side of the book here .. 
    return;
    /*  DBGLOG_CLASS_FUNC_LINE_ERROR
        << "ALERT: Obtained invalid bid/ask price for " << symbol
        << " , hence not processing further... " << shortcode << " " << hash_string << " " <<  exp_<< " BID "<< 
        best_bid_px_ << " " << best_ask_px_ << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    return; */
  }
  BA_struct ba;
  ba.Bid_Px = best_bid_px_;
  ba.Ask_Px = best_ask_px_;
  ba.Expiry = exp_;
  shc_2_BA[hash_string] = ba;
  if (other_hash_string != "")
    shc_2_BA[other_hash_string] = ba;
}

int TradeBarGenerator::GetLastDataReceivedTime() {
  return last_data_received_time;
}
