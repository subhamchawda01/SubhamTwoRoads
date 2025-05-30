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
#include "dvccode/CDef/bse_security_definition.hpp"
#include "infracore/BSE/BSEEngine.hpp"

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

  static void GetModifyCountFromReply(std::string ors_reply_dir_, std::string audit_dir_, std::string segment_,
                                       std::string file_date_, std::string& start_time , std::string& end_time) {

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
    std::unordered_map<uint64_t, int> saos_username;
    //std::unordered_map<int, std::ofstream*> user_list;

    HFSAT::SecurityDefinitions::GetUniqueInstance(std::stoi(file_date)).LoadBSESecurityDefinitions();

    struct stat buffer;
    if ((stat(ors_reply_dir.c_str(), &buffer) != 0) || (stat(audit_dir.c_str(), &buffer) != 0)) {
      std::cout << "Entered Directory doesn't Exists Exiting" << std::endl;
      exit(1);
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
        //user_list[user_name] = new std::ofstream();

        HFSAT::BulkFileReader reader1;
        reader1.open(entry.path().string());

        while (true) {
          char bse_msg_ptr[MAX_BSE_RESPONSE_BUFFER_SIZE];
          size_t available_len_1 = reader1.read(bse_msg_ptr, BSE_RESPONSE_HEADER_LENGTH);
          // we read the header from the file

          if (available_len_1 < BSE_RESPONSE_HEADER_LENGTH) break;
          uint32_t this_bse_message_bodylength_ = (uint32_t)(*((char *)(bse_msg_ptr)));
          uint16_t this_bse_template_id_ = *((uint16_t *)(bse_msg_ptr + 4));

          available_len_1 = reader1.read((bse_msg_ptr + BSE_RESPONSE_HEADER_LENGTH), (this_bse_message_bodylength_ - BSE_RESPONSE_HEADER_LENGTH));
          if (available_len_1 < (this_bse_message_bodylength_ - BSE_RESPONSE_HEADER_LENGTH)) break;

          const char* msg_ptr = bse_msg_ptr;

          switch (this_bse_template_id_) {
            case TID_NEW_ORDER_NR_RESPONSE: {
              HFSAT::BSE::BSENewOrderSingleShortResponse process_single_short_response_; 
              NewOrderNRResponseT* single_short_response_ = process_single_short_response_.ProcessNewOrderSingleShortResponse(msg_ptr);
              saos_username[single_short_response_->ClOrdID] = user_name;
            } break;

            default: {
            } break;
          }
        }
      }
    }

    for (const auto &entry : fs::directory_iterator(ors_reply_dir)) {

      if (endsWith(entry.path().string(), file_date + ".gz") || endsWith(entry.path().string(), file_date)) {

        int total_exec = 0;
        int total_order = 0;
        std::map<int,int> saciToPos;
      // as we are considering cash machines
        saciToPos[670101001] = 0;
        saciToPos[670101002] = 1;
        saciToPos[670101003] = 2;
        saciToPos[670101004] = 3;
        int saciToExec[4] = {0};
        int saciToOrder[4] = {0};
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


            if (next_event_.orr_type_ == HFSAT::kORRType_Exec){
              if (saos_username.find(next_event_.server_assigned_order_sequence_) != saos_username.end()) {
                  saciToExec[saciToPos[saos_username[next_event_.server_assigned_order_sequence_]]]++;
              }
            }
            else if( next_event_.orr_type_ == HFSAT::kORRType_CxRe ){
              if (saos_username.find(next_event_.server_assigned_order_sequence_) != saos_username.end()) {
                  saciToOrder[saciToPos[saos_username[next_event_.server_assigned_order_sequence_]]]++;
              }
            }
          }
          std::cout << next_event_.symbol_;
          for (int pos =0; pos < 4; pos++){
            total_order += saciToOrder[pos];
            total_exec += saciToExec[pos];
            std::cout << " " << saciToOrder[pos] << " " << saciToExec[pos];
          }
          std::cout << " " << total_order << " " << total_exec << "\n";
          bulk_file_reader_.close();
        }
        //std::cout << "Close Path file: " << entry.path().string() << std::endl;
      }
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
    std::unordered_map<uint64_t, int> saos_username;
    //std::unordered_map<int, std::ofstream*> user_list;

    HFSAT::SecurityDefinitions::GetUniqueInstance(std::stoi(file_date)).LoadBSESecurityDefinitions();

    struct stat buffer;
    if ((stat(ors_reply_dir.c_str(), &buffer) != 0) || (stat(audit_dir.c_str(), &buffer) != 0)) {
      std::cout << "Entered Directory doesn't Exists Exiting" << std::endl;
      exit(1);
    }

    std::string server_nick_name = "/home/pengine/prod/live_configs/server_latency_reporting_config.txt";

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
        //user_list[user_name] = new std::ofstream();

        HFSAT::BulkFileReader reader1;
        reader1.open(entry.path().string());

        while (true) {
          char bse_msg_ptr[MAX_BSE_RESPONSE_BUFFER_SIZE];
          size_t available_len_1 = reader1.read(bse_msg_ptr, BSE_RESPONSE_HEADER_LENGTH);
          // we read the header from the file

          if (available_len_1 < BSE_RESPONSE_HEADER_LENGTH) break;
          uint32_t this_bse_message_bodylength_ = (uint32_t)(*((char *)(bse_msg_ptr)));
          uint16_t this_bse_template_id_ = *((uint16_t *)(bse_msg_ptr + 4));

          available_len_1 = reader1.read((bse_msg_ptr + BSE_RESPONSE_HEADER_LENGTH), (this_bse_message_bodylength_ - BSE_RESPONSE_HEADER_LENGTH));
          if (available_len_1 < (this_bse_message_bodylength_ - BSE_RESPONSE_HEADER_LENGTH)) break;

          const char* msg_ptr = bse_msg_ptr;

          switch (this_bse_template_id_) {
            case TID_NEW_ORDER_NR_RESPONSE: {
              HFSAT::BSE::BSENewOrderSingleShortResponse process_single_short_response_;
              NewOrderNRResponseT* single_short_response_ = process_single_short_response_.ProcessNewOrderSingleShortResponse(msg_ptr);
              saos_username[single_short_response_->ClOrdID] = user_name;
            } break;

            default: {
            } break;
          }
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
    std::unordered_map<uint64_t, uint64_t> saos_orderid;

    HFSAT::SecurityDefinitions::GetUniqueInstance(std::stoi(file_date)).LoadBSESecurityDefinitions();

    struct stat buffer;
    if ((stat(ors_reply_dir.c_str(), &buffer) != 0) || (stat(audit_dir.c_str(), &buffer) != 0)) {
      std::cout << "Entered Directory doesn't Exists Exiting" << std::endl;
      exit(1);
    }

    std::string server_nick_name = "/home/pengine/prod/live_configs/server_latency_reporting_config.txt";

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

        HFSAT::BulkFileReader reader1;
        reader1.open(entry.path().string());

        while (true) {
          char bse_msg_ptr[MAX_BSE_RESPONSE_BUFFER_SIZE];
          size_t available_len_1 = reader1.read(bse_msg_ptr, BSE_RESPONSE_HEADER_LENGTH);
          // we read the header from the file

          if (available_len_1 < BSE_RESPONSE_HEADER_LENGTH) break;
          uint32_t this_bse_message_bodylength_ = (uint32_t)(*((char*)(bse_msg_ptr)));
          uint16_t this_bse_template_id_ = *((uint16_t *)(bse_msg_ptr + 4));

          available_len_1 = reader1.read((bse_msg_ptr + BSE_RESPONSE_HEADER_LENGTH), (this_bse_message_bodylength_ - BSE_RESPONSE_HEADER_LENGTH));
          if (available_len_1 < (this_bse_message_bodylength_ - BSE_RESPONSE_HEADER_LENGTH)) break;

          const char* msg_ptr = bse_msg_ptr;

          switch (this_bse_template_id_) {
            case TID_NEW_ORDER_NR_RESPONSE: {
              HFSAT::BSE::BSENewOrderSingleShortResponse process_single_short_response_; 
              NewOrderNRResponseT* single_short_response_ = process_single_short_response_.ProcessNewOrderSingleShortResponse(msg_ptr);
              std::string exch_symbol = to_string(single_short_response_->SecurityID);
              std::string shortcode = HFSAT::BSESecurityDefinitions::GetUniqueInstance(std::stoi(file_date)).GetShortCodeFromExchangeId(exch_symbol);
              if ( (shortcode.compare(4,symbol_.size(),symbol_)) == 0 ) {
                saos_orderid[single_short_response_->ClOrdID] = single_short_response_->OrderID;
              }
            } break;

            default: {
            } break;
          }
        }
      }
    }
    for (const auto &entry : fs::directory_iterator(ors_reply_dir)) {
      std::string file_path = entry.path().string();
      if (endsWith(file_path, file_date + ".gz") || endsWith(file_path, file_date)) {

        //std::cout << "Path file: " << entry.path().string() << std::endl;
        std::string find_symbol = "BSE_" + symbol_ + "_";
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
