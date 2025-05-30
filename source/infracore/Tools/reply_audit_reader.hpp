#pragma once
#include <dirent.h>
#include <algorithm>
#include <sstream>
#include <time.h>
#include <boost/filesystem.hpp>
#include <iterator>
#include <string>

#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp"
#include "infracore/NSET/NSEEngine.hpp"
#include "infracore/NSET/nse_tap_invitation_manager.hpp"
#include "infracore/NSET/nse_msg_handler.hpp"
#include "infracore/NSET/nse_msg_handler_cash_market.hpp"
#include "infracore/NSET/nse_msg_handler_derivatives.hpp"
#include "infracore/NSET/NSETemplates/OrderPriceChangeRequest.hpp"

namespace fs = boost::filesystem;

namespace HFSAT {
template <class T>
class ReplyAuditReader {
 public:
  static bool endsWith(const std::string &mainStr, const std::string &toMatch) {
    if (mainStr.size() >= toMatch.size() &&
        mainStr.compare(mainStr.size() - toMatch.size(), toMatch.size(), toMatch) == 0)
      return true;
    else
      return false;
  }
  static void eraseSubStr(std::string &mainStr, const std::string &toErase) {
    size_t pos = mainStr.find(toErase);
    if (pos != std::string::npos) {
      mainStr.erase(pos, toErase.length());
    }
  }

  static void ReadOrsReplyAndAuditFile(std::string ors_reply_dir_, std::string audit_dir_, std::string segment_,
                                       std::string file_date_, std::string server_, std::string& start_time , std::string& end_time) {

    std::string ors_reply_dir = ors_reply_dir_;
    std::string audit_dir = audit_dir_;
    std::string file_date = file_date_;

    int hour, minutes;
    struct tm tm_start_time, tm_end_time;
    hour = std::stoi(start_time.substr(4,2));
    minutes = std::stoi(start_time.substr(6,2));
    tm_start_time.tm_hour = hour;
    tm_start_time.tm_min = minutes;

    hour = std::stoi(end_time.substr(4,2));
    minutes = std::stoi(end_time.substr(6,2));
    tm_end_time.tm_hour = hour;
    tm_end_time.tm_min = minutes;

    std::unordered_map<std::string, std::string> ref;
    std::unordered_map<int32_t, int> saos_username;
    std::unordered_map<int, std::ofstream*> user_list;

    HFSAT::NSE::NseMsgHandler* nse_msgs_handler_;
    if (segment_ == "NSE_EQ") {
      nse_msgs_handler_ = new HFSAT::NSE::NseMsgHandlerCashMarket();
    } else {
      nse_msgs_handler_ = new HFSAT::NSE::NseMsgHandlerDerivatives();
    }

    HFSAT::SecurityDefinitions::GetUniqueInstance(std::stoi(file_date)).LoadNSESecurityDefinitions();

    struct stat buffer;
    if ((stat(ors_reply_dir.c_str(), &buffer) != 0) || (stat(audit_dir.c_str(), &buffer) != 0)) {
      std::cout << "Entered Directory doesn't Exists Exiting" << std::endl;
      exit(1);
    }

    std::string server_nick_name = "/home/pengine/prod/live_configs/server_mapping.txt";

    // Reading file data line by line and mapping the server name into server code
    std::ifstream file(server_nick_name);

    if (!file.is_open()) {
      std::cout << "Config File for mapping server doesn't Exists " << server_nick_name << std::endl;
      exit(1);
    }


    std::string str;
    while (std::getline(file, str)) {
      std::stringstream ss(str);
      std::string word;

      ss >> word;
      std::string short_name = word;

      ss >> word;
      std::string server_code = word;

      ref[short_name] = server_code;
    }

    std::string server_number = ref[server_];
    if (server_number == "") {
      std::cout << "SERVER " << server_ << " DOESN't Exist in Conf" << server_nick_name << std::endl;
      return;
    }

    for (const auto &entry : fs::directory_iterator(audit_dir)) {
      //        Checking for the desired file with desired date in the dir_path

      if (endsWith(entry.path().string(), file_date + ".in.gz") || endsWith(entry.path().string(), file_date + ".in")) {
        // Printing the file name of the zipped file to be opened

        //std::cout << "Path file: " << entry.path().string() << std::endl;
        std::string file_path = entry.path().string();
        std::vector<std::string> tokens_;
        std::stringstream ss(file_path);
        std::string item;
        while (std::getline(ss, item, '.')) {
          tokens_.push_back(item);
        }
        int user_name = std::stoi(tokens_[1]);
        user_list[user_name] = new std::ofstream();

        HFSAT::BulkFileReader reader1, reader2;
        reader1.open(entry.path().string());
        reader2.open(entry.path().string());
        HFSAT::NSE::ProcessedPacketHeader* processed_packet_header;
        HFSAT::NSE::ProcessedResponseHeader* processed_response_header;

        while (true) {
          char nse_msg_ptr[MAX_NSE_RESPONSE_BUFFER_SIZE];
          size_t available_len_1 = reader1.read(nse_msg_ptr, NSE_PACKET_RESPONSE_LENGTH);
          // we read the header from the file

          processed_packet_header = nse_msgs_handler_->packet_response_.ProcessPakcet(nse_msg_ptr);
          if (available_len_1 < NSE_PACKET_RESPONSE_LENGTH) break;
          available_len_1 = reader1.read(nse_msg_ptr, NSE_RESPONSE_MESSAGE_HEADER_LENGTH);
          processed_response_header = nse_msgs_handler_->response_header_.ProcessHeader(nse_msg_ptr);
          if (available_len_1 < NSE_RESPONSE_MESSAGE_HEADER_LENGTH) break;

          reader2.read(nse_msg_ptr, processed_packet_header->packet_length);
          const char* msg_ptr = nse_msg_ptr;
          msg_ptr += NSE_PACKET_RESPONSE_LENGTH;
          msg_ptr += NSE_RESPONSE_MESSAGE_HEADER_LENGTH;

          switch (processed_response_header->transaction_code) {
            case ORDER_CONFIRMATION_TR: {
              HFSAT::NSE::ProcessedOrderResponse* processed_order_response_;
              msg_ptr -= NSE_RESPONSE_MESSAGE_HEADER_LENGTH;
              processed_order_response_ = nse_msgs_handler_->order_response_->ProcessOrderResponse(msg_ptr);
              saos_username[processed_order_response_->saos] = user_name;
              msg_ptr += NSE_RESPONSE_MESSAGE_HEADER_LENGTH;
            } break;
            default: {
            } break;
          }
          size_t msg_len =
              (processed_packet_header->packet_length - NSE_RESPONSE_MESSAGE_HEADER_LENGTH - NSE_PACKET_RESPONSE_LENGTH);
          reader1.read(nse_msg_ptr, msg_len);
        }
      }
    }
    for (const auto &entry : fs::directory_iterator(ors_reply_dir)) {

      if (endsWith(entry.path().string(), file_date + ".gz") || endsWith(entry.path().string(), file_date)) {

        //std::cout << "Path file: " << entry.path().string() << std::endl;
        HFSAT::BulkFileReader bulk_file_reader_;
        bulk_file_reader_.open(entry.path().string());
        T next_event_;
        if (bulk_file_reader_.is_open()) {
          while (true) {
            size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(T));
            if (available_len_ < sizeof(next_event_)) break;
            int client_id = next_event_.server_assigned_client_id_ >> 16;

            if (client_id == stoi(server_number)) {
              time_t time = (time_t)next_event_.client_request_time_.tv_sec ;
              struct tm tm_curr = *localtime(&time);

              if ( tm_curr.tm_hour < tm_start_time.tm_hour ) continue ;
              else if ( (tm_curr.tm_hour == tm_start_time.tm_hour) && (tm_curr.tm_min < tm_start_time.tm_min) ) continue;

              if ( tm_curr.tm_hour > tm_end_time.tm_hour ) continue;
              else if ( (tm_curr.tm_hour == tm_end_time.tm_hour) && (tm_curr.tm_min > tm_end_time.tm_min) ) continue;

              //std::cout << next_event_.server_assigned_order_sequence_ << "_" << user_list[next_event_.server_assigned_order_sequence_] << std::endl;
              if (saos_username.find(next_event_.server_assigned_order_sequence_) != saos_username.end()) {
                //std::cout << "present" << std::endl;
                //*(user_list[next_event_.server_assigned_order_sequence_]) << next_event_.ToString(); //crash
                std::cout << saos_username[next_event_.server_assigned_order_sequence_] << " " << next_event_.ToString();
              }
            }
          }
          bulk_file_reader_.close();
        }
        //std::cout << "Close Path file: " << entry.path().string() << std::endl;
      }
    }
  }


  static void ReadOrsReplyAndAuditFileAll(std::string ors_reply_dir_, std::string segment_, std::string file_date_,
                                          std::string& start_time , std::string& end_time, std::vector<std::string> audit_file_dirs_vec) {

    std::string ors_reply_dir = ors_reply_dir_;
    std::string file_date = file_date_;

    int hour, minutes;
    struct tm tm_start_time, tm_end_time;
    hour = std::stoi(start_time.substr(4,2));
    minutes = std::stoi(start_time.substr(6,2));
    tm_start_time.tm_hour = hour;
    tm_start_time.tm_min = minutes;

    hour = std::stoi(end_time.substr(4,2));
    minutes = std::stoi(end_time.substr(6,2));
    tm_end_time.tm_hour = hour;
    tm_end_time.tm_min = minutes;

    std::unordered_map<std::string, int> shortcodesaos_username;

    HFSAT::NSE::NseMsgHandler* nse_msgs_handler_;
    if (segment_ == "NSE_EQ") {
      nse_msgs_handler_ = new HFSAT::NSE::NseMsgHandlerCashMarket();
    } else {
      nse_msgs_handler_ = new HFSAT::NSE::NseMsgHandlerDerivatives();
    }

    HFSAT::SecurityDefinitions::GetUniqueInstance(std::stoi(file_date)).LoadNSESecurityDefinitions();

    for (unsigned int dir_path = 0; dir_path < audit_file_dirs_vec.size(); dir_path++) {
      for (const auto &entry : fs::directory_iterator(audit_file_dirs_vec[dir_path])) {
      //        Checking for the desired file with desired date in the dir_path

        if (endsWith(entry.path().string(), file_date + ".in.gz") || endsWith(entry.path().string(), file_date + ".in")) {
          // Printing the file name of the zipped file to be opened

          std::string file_path = entry.path().string();
          std::vector<std::string> audit_fields_;
          std::stringstream ss(file_path);
          std::string item;
          while (std::getline(ss, item, '.')) {
            audit_fields_.push_back(item);
          }
          int user_name = std::stoi(audit_fields_[1]);

          HFSAT::BulkFileReader reader1, reader2;
          reader1.open(entry.path().string());
          reader2.open(entry.path().string());
          HFSAT::NSE::ProcessedPacketHeader* processed_packet_header;
          HFSAT::NSE::ProcessedResponseHeader* processed_response_header;

          while (true) {
            char nse_msg_ptr[MAX_NSE_RESPONSE_BUFFER_SIZE];
            size_t available_len_1 = reader1.read(nse_msg_ptr, NSE_PACKET_RESPONSE_LENGTH);
            // we read the header from the file

            processed_packet_header = nse_msgs_handler_->packet_response_.ProcessPakcet(nse_msg_ptr);
            if (available_len_1 < NSE_PACKET_RESPONSE_LENGTH) break;
            available_len_1 = reader1.read(nse_msg_ptr, NSE_RESPONSE_MESSAGE_HEADER_LENGTH);
            processed_response_header = nse_msgs_handler_->response_header_.ProcessHeader(nse_msg_ptr);
            if (available_len_1 < NSE_RESPONSE_MESSAGE_HEADER_LENGTH) break;

            reader2.read(nse_msg_ptr, processed_packet_header->packet_length);
            const char* msg_ptr = nse_msg_ptr;
            msg_ptr += NSE_PACKET_RESPONSE_LENGTH;
            msg_ptr += NSE_RESPONSE_MESSAGE_HEADER_LENGTH;

            switch (processed_response_header->transaction_code) {
              case ORDER_CONFIRMATION_TR: {
                HFSAT::NSE::ProcessedOrderResponse* processed_order_response_;
                msg_ptr -= NSE_RESPONSE_MESSAGE_HEADER_LENGTH;
                processed_order_response_ = nse_msgs_handler_->order_response_->ProcessOrderResponse(msg_ptr);
                std::string symbol;
                HFSAT::PerishableStringTokenizer::TrimString(processed_order_response_->symbol, symbol, ' ');
                std::string map_key = "NSE_" + symbol + "_" + std::to_string(processed_order_response_->saos);
                shortcodesaos_username[map_key] = user_name;
                msg_ptr += NSE_RESPONSE_MESSAGE_HEADER_LENGTH;
              } break;
              default: {
              } break;
            }
            size_t msg_len =
                (processed_packet_header->packet_length - NSE_RESPONSE_MESSAGE_HEADER_LENGTH - NSE_PACKET_RESPONSE_LENGTH);
            reader1.read(nse_msg_ptr, msg_len);
          }
        }
      }
    }
    for (const auto &entry : fs::directory_iterator(ors_reply_dir)) {
      std::set<std::string> product_user_set;
      std::unordered_map<std::string,int> total_modify_map;
      std::unordered_map<std::string,int> total_modifyof_map;
      std::unordered_map<std::string,int> total_order_map;
      std::unordered_map<std::string,int> total_exec_map;
      std::unordered_map<std::string,double> total_order_price_map;
      std::unordered_map<std::string,double> total_exec_price_map;
      std::unordered_map<std::string,double> last_trade_map;
      std::unordered_map <std::string, int> saci_saos_sizeexecuted_map;

      if (endsWith(entry.path().string(), file_date + ".gz") || endsWith(entry.path().string(), file_date)) {

        //std::cout << "Path file: " << entry.path().string() << std::endl;
        HFSAT::BulkFileReader bulk_file_reader_;
        bulk_file_reader_.open(entry.path().string());
        T next_event_;
        if (bulk_file_reader_.is_open()) {
          while (true) {
            size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(T));
            if (available_len_ < sizeof(next_event_)) break;

            time_t time = (time_t)next_event_.client_request_time_.tv_sec ;
            struct tm tm_curr = *localtime(&time);

            if ( tm_curr.tm_hour < tm_start_time.tm_hour ) continue ;
            else if ( (tm_curr.tm_hour == tm_start_time.tm_hour) && (tm_curr.tm_min < tm_start_time.tm_min) ) continue;

            if ( tm_curr.tm_hour > tm_end_time.tm_hour ) continue;
            else if ( (tm_curr.tm_hour == tm_end_time.tm_hour) && (tm_curr.tm_min > tm_end_time.tm_min) ) continue;

            std::string find_key = std::string(next_event_.symbol_) + "_" + std::to_string(next_event_.server_assigned_order_sequence_);
            if (shortcodesaos_username.find(find_key) != shortcodesaos_username.end()) {
              if (next_event_.orr_type_ == HFSAT::kORRType_Exec){
                std::string shortcodeuser_key = std::string(next_event_.symbol_) + "_" + std::to_string(shortcodesaos_username[find_key]);
                product_user_set.insert(shortcodeuser_key);
                std::string sacisaos_key = std::to_string(next_event_.server_assigned_client_id_) + "_" + std::to_string(next_event_.server_assigned_order_sequence_);
                int temp_size = next_event_.size_executed_ - saci_saos_sizeexecuted_map[sacisaos_key];
                total_exec_price_map[shortcodeuser_key] += next_event_.price_ * temp_size;
                saci_saos_sizeexecuted_map[sacisaos_key] = next_event_.size_executed_;
                total_exec_map[shortcodeuser_key]++;
                last_trade_map[shortcodeuser_key] = next_event_.price_;
              }
              else if(next_event_.orr_type_ == HFSAT::kORRType_Conf ||next_event_.orr_type_ == HFSAT::kORRType_CxRe || next_event_.orr_type_ ==  HFSAT::kORRType_Cxld){
                std::string shortcodeuser_key = std::string(next_event_.symbol_) + "_" + std::to_string(shortcodesaos_username[find_key]);
                product_user_set.insert(shortcodeuser_key);
                total_order_map[shortcodeuser_key]++;
                total_order_price_map[shortcodeuser_key] += next_event_.price_ * next_event_.size_remaining_;
                if(next_event_.orr_type_ == HFSAT::kORRType_CxRe) {
                  total_modify_map[shortcodeuser_key]++;
                  if(next_event_.order_flags_ == 1)
                    total_modifyof_map[shortcodeuser_key]++;
                }
                last_trade_map[shortcodeuser_key] = next_event_.price_;
              }
            }
          }
        }
        bulk_file_reader_.close();
      }
      for (auto set_val : product_user_set) {
        std::vector<char*> tokens_;
        char readline_buffer_copy_[1024];
        memset(readline_buffer_copy_, 0, sizeof(readline_buffer_copy_));
        strcpy(readline_buffer_copy_, set_val.c_str());
        HFSAT::PerishableStringTokenizer::NonConstStringTokenizer(readline_buffer_copy_, "_", tokens_);
        std::cout << tokens_[0] << "_" << tokens_[1] << " " << total_modify_map[set_val] << " " << total_modifyof_map[set_val] << " " << total_order_map[set_val] << " "
                  << total_exec_map[set_val] << std::fixed << std::setprecision(2) << " " << total_order_price_map[set_val] << " " << total_exec_price_map[set_val] << " " 
                  << last_trade_map[set_val] << " " << tokens_[2] << std::endl;
      }
    }
  }

  static void ReadOrderIdSaciFromReplyAndAuditFile(std::string ors_reply_dir_, std::string audit_dir_, std::string segment_, std::string file_date_,
                                                   std::string server_, std::string symbol_, std::string& start_time , std::string& end_time) {

    std::string ors_reply_dir = ors_reply_dir_;
    std::string audit_dir = audit_dir_;
    std::string file_date = file_date_;

    int hour, minutes;
    struct tm tm_start_time, tm_end_time;
    hour = std::stoi(start_time.substr(4,2));
    minutes = std::stoi(start_time.substr(6,2));
    tm_start_time.tm_hour = hour;
    tm_start_time.tm_min = minutes;

    hour = std::stoi(end_time.substr(4,2));
    minutes = std::stoi(end_time.substr(6,2));
    tm_end_time.tm_hour = hour;
    tm_end_time.tm_min = minutes;

    std::unordered_map<std::string, std::string> ref;
    std::unordered_map<int32_t, int64_t> saos_orderid;

    HFSAT::NSE::NseMsgHandler* nse_msgs_handler_;
    if (segment_ == "NSE_EQ") {
      nse_msgs_handler_ = new HFSAT::NSE::NseMsgHandlerCashMarket();
    } else {
      nse_msgs_handler_ = new HFSAT::NSE::NseMsgHandlerDerivatives();
    }

    HFSAT::SecurityDefinitions::GetUniqueInstance(std::stoi(file_date)).LoadNSESecurityDefinitions();

    struct stat buffer;
    if ((stat(ors_reply_dir.c_str(), &buffer) != 0) || (stat(audit_dir.c_str(), &buffer) != 0)) {
      std::cout << "Entered Directory doesn't Exists Exiting" << std::endl;
      exit(1);
    }

    std::string server_nick_name = "/home/pengine/prod/live_configs/server_mapping.txt";

    // Reading file data line by line and mapping the server name into server code
    std::ifstream file(server_nick_name);

    if (!file.is_open()) {
      std::cout << "Config File for mapping server doesn't Exists " << server_nick_name << std::endl;
      exit(1);
    }


    std::string str;
    while (std::getline(file, str)) {
      std::stringstream ss(str);
      std::string word;

      ss >> word;
      std::string short_name = word;

      ss >> word;
      std::string server_code = word;

      ref[short_name] = server_code;
    }

    std::string server_number = ref[server_];
    if (server_number == "") {
      std::cout << "SERVER " << server_ << " DOESN't Exist in Conf " << server_nick_name << std::endl;
      return;
    }

    for (const auto &entry : fs::directory_iterator(audit_dir)) {
      //        Checking for the desired file with desired date in the dir_path

      if (endsWith(entry.path().string(), file_date + ".in.gz") || endsWith(entry.path().string(), file_date + ".in")) {
        // Printing the file name of the zipped file to be opened

        //std::cout << "Path file: " << entry.path().string() << std::endl;

        HFSAT::BulkFileReader reader1, reader2;
        reader1.open(entry.path().string());
        reader2.open(entry.path().string());
        HFSAT::NSE::ProcessedPacketHeader* processed_packet_header;
        HFSAT::NSE::ProcessedResponseHeader* processed_response_header;

        while (true) {
          char nse_msg_ptr[MAX_NSE_RESPONSE_BUFFER_SIZE];
          size_t available_len_1 = reader1.read(nse_msg_ptr, NSE_PACKET_RESPONSE_LENGTH);
          // we read the header from the file

          processed_packet_header = nse_msgs_handler_->packet_response_.ProcessPakcet(nse_msg_ptr);
          if (available_len_1 < NSE_PACKET_RESPONSE_LENGTH) break;
          available_len_1 = reader1.read(nse_msg_ptr, NSE_RESPONSE_MESSAGE_HEADER_LENGTH);
          processed_response_header = nse_msgs_handler_->response_header_.ProcessHeader(nse_msg_ptr);
          if (available_len_1 < NSE_RESPONSE_MESSAGE_HEADER_LENGTH) break;

          reader2.read(nse_msg_ptr, processed_packet_header->packet_length);
          const char* msg_ptr = nse_msg_ptr;
          msg_ptr += NSE_PACKET_RESPONSE_LENGTH;
          msg_ptr += NSE_RESPONSE_MESSAGE_HEADER_LENGTH;

          switch (processed_response_header->transaction_code) {
            case ORDER_CONFIRMATION_TR: {
              HFSAT::NSE::ProcessedOrderResponse* processed_order_response_;
              msg_ptr -= NSE_RESPONSE_MESSAGE_HEADER_LENGTH;
              processed_order_response_ = nse_msgs_handler_->order_response_->ProcessOrderResponse(msg_ptr);
              std::string symbol(processed_order_response_->symbol);
              if ( (symbol.compare(0,symbol_.size(),symbol_)) == 0 ) {
                saos_orderid[processed_order_response_->saos] = processed_order_response_->order_number;
              }
              msg_ptr += NSE_RESPONSE_MESSAGE_HEADER_LENGTH;
            } break;
            default: {
            } break;
          }
          size_t msg_len =
              (processed_packet_header->packet_length - NSE_RESPONSE_MESSAGE_HEADER_LENGTH - NSE_PACKET_RESPONSE_LENGTH);
          reader1.read(nse_msg_ptr, msg_len);
        }
      }
    }
    for (const auto &entry : fs::directory_iterator(ors_reply_dir)) {
      std::string file_path = entry.path().string();
      if (endsWith(file_path, file_date + ".gz") || endsWith(file_path, file_date)) {

        //std::cout << "Path file: " << entry.path().string() << std::endl;
        std::string find_symbol = "NSE_" + symbol_ + "_";
        if ( file_path.find(find_symbol) == std::string::npos ) continue;
        HFSAT::BulkFileReader bulk_file_reader_;
        bulk_file_reader_.open(entry.path().string());
        T next_event_;
        if (bulk_file_reader_.is_open()) {
          while (true) {
            size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(T));
            if (available_len_ < sizeof(next_event_)) break;
            int client_id = next_event_.server_assigned_client_id_ >> 16;

            if (client_id == stoi(server_number)) {
              time_t time = (time_t)next_event_.client_request_time_.tv_sec ;
              struct tm tm_curr = *localtime(&time);

              if ( tm_curr.tm_hour < tm_start_time.tm_hour ) continue ;
              else if ( (tm_curr.tm_hour == tm_start_time.tm_hour) && (tm_curr.tm_min < tm_start_time.tm_min) ) continue;

              if ( tm_curr.tm_hour > tm_end_time.tm_hour ) continue;
              else if ( (tm_curr.tm_hour == tm_end_time.tm_hour) && (tm_curr.tm_min > tm_end_time.tm_min) ) continue;

              if (saos_orderid.find(next_event_.server_assigned_order_sequence_) != saos_orderid.end()) {
                //std::cout << "present" << std::endl;
                std::cout << "date: " << file_date_ << " server: " << server_ 
                          << " symbol: " << next_event_.symbol_  
                          << " orderid: " << saos_orderid[next_event_.server_assigned_order_sequence_]
                          << " saci: " << next_event_.server_assigned_client_id_ << std::endl;
              }
            }
          }
          bulk_file_reader_.close();
        }
        //std::cout << "Close Path file: " << entry.path().string() << std::endl;
      }
    }
  }
  // End of class ReplyAuditReader
};

// End of namespace
}  // namespace HFSAT
