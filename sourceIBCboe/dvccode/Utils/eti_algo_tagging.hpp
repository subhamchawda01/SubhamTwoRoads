// =====================================================================================
//
//       Filename:  eti_algo_tagging.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  02/18/2014 05:37:01 PM
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

#pragma once

#include <set>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <map>
#include <iostream>
#include <unordered_map>

#include "dvccode/CDef/email_utils.hpp"
#include "dvccode/CDef/assumptions.hpp"
#include "dvccode/Utils/send_alert.hpp"
#include "dvccode/Utils/lock.hpp"

#define DEF_ETI_STRATEGY_MD5SUM_ALGOCODE_FILENAME "/spare/local/files/EUREX/eti_md5sum_strategy_algocode_database.db"
#define DEF_ICE_STRATEGY_MD5SUM_ALGOCODE_FILENAME "/spare/local/files/ICE/ice_md5sum_strategy_algocode_database.db"
#define INITIAL_SIZE_OF_CID_TO_ALGO_MAPPING_VECTOR 64

namespace HFSAT {
namespace Utils {

struct AlgoInfo {
  std::string md5sum_;
  std::string strategy_name_;
};

class ETIAlgoTagging {
 private:
  std::map<std::string, uint16_t> md5sum_strategy_name_to_base_algo_code_map_;
  std::map<std::string, std::string> md5sum_strategy_name_to_desc_map_;
  std::string ors_control_ip_;
  int ors_control_port_;
  HFSAT::TCPClientSocket tcp_client_socket_;

  std::vector<int32_t> client_id_to_algo_mapping_;
  std::map<int32_t, std::string> client_id_to_md5sum_;
  std::map<int32_t, std::string> client_id_to_strategy_name_;

  std::set<uint32_t> unique_algo_set_;

  HFSAT::Lock mutex_lock_;

 private:
  // Will send an email to dvctech
  void EmailETIAlgoTaggingAlert(std::string _alert_body_, bool _is_alert_ = true) {
    // also send an alert
    char hostname[128];
    hostname[127] = '\0';
    gethostname(hostname, 127);

    if (_is_alert_) {
      std::string alert_message = std::string("ALERT") + _alert_body_;
      HFSAT::SendAlert::sendAlert(alert_message);
    }

    HFSAT::Email e;

    if (_is_alert_) {
      e.setSubject("ETIAlgoTagginFailure");

    } else {
      e.setSubject("ETIAlgoTagginInfo");
    }

    e.addRecepient("ravi.parikh@tworoads.co.in, nseall@tworoads.co.in");
    e.addSender("ravi.parikh@tworoads.co.in");
    e.content_stream << "host_machine: " << hostname << "<br/>";
    e.content_stream << _alert_body_ << "<br/>";
    e.sendMail();
  }

  void LoadMD5SUMStrategyNamesToCodeDatabase(const std::string& exchange) {
    mutex_lock_.LockMutex();

    std::ifstream md5sum_strategy_names_to_code_database_stream_;
    if (exchange == "ICE") {
      md5sum_strategy_names_to_code_database_stream_.open(DEF_ICE_STRATEGY_MD5SUM_ALGOCODE_FILENAME, std::ios::in);
    } else {
      md5sum_strategy_names_to_code_database_stream_.open(DEF_ETI_STRATEGY_MD5SUM_ALGOCODE_FILENAME, std::ios::in);
    }

    if (!md5sum_strategy_names_to_code_database_stream_.is_open()) {
      std::cerr << " Failed To Open Strategy-Algo Mapping Database : " << DEF_ETI_STRATEGY_MD5SUM_ALGOCODE_FILENAME
                << " With : " << strerror(errno) << "\n";
      EmailETIAlgoTaggingAlert(std::string(" Failed To Open Strategy-Algo Mapping Database : ") +
                               std::string(DEF_ETI_STRATEGY_MD5SUM_ALGOCODE_FILENAME));

      mutex_lock_.UnlockMutex();
      exit(1);
    }

#define MAX_LINE_SIZE 1024

    char buffer[MAX_LINE_SIZE];
    std::string line_buffer_str_ = "";

    memset((void*)(buffer), 0, MAX_LINE_SIZE);

    while (md5sum_strategy_names_to_code_database_stream_.good()) {
      md5sum_strategy_names_to_code_database_stream_.getline(buffer, MAX_LINE_SIZE);
      line_buffer_str_ = buffer;

      // Skip over comments
      if (line_buffer_str_.find("#") != std::string::npos) continue;

      HFSAT::PerishableStringTokenizer st_(buffer, MAX_LINE_SIZE);
      const std::vector<const char*>& tokens_ = st_.GetTokens();

      if (tokens_.size() >= 4) {
        std::string md5sum_ = tokens_[0];
        std::string strategy_name_ = tokens_[1];
        uint32_t algo_base_code_ = atoi(tokens_[2]);
        std::string desc_ = tokens_[3];

        std::string md5sum_strategy_name = md5sum_ + strategy_name_;

        md5sum_strategy_name_to_base_algo_code_map_[md5sum_strategy_name] = algo_base_code_;
        md5sum_strategy_name_to_desc_map_[md5sum_strategy_name] = desc_;
      }
    }

    md5sum_strategy_names_to_code_database_stream_.close();

#undef MAX_LINE_SIZE

    mutex_lock_.UnlockMutex();
  }

  ETIAlgoTagging(const ETIAlgoTagging& disabled_copy_constructor_);

  ETIAlgoTagging(const std::string& exchange)
      : md5sum_strategy_name_to_base_algo_code_map_(),
        md5sum_strategy_name_to_desc_map_(),
        ors_control_ip_("127.0.0.1"),
        ors_control_port_(-1),
        tcp_client_socket_(),
        client_id_to_algo_mapping_(INITIAL_SIZE_OF_CID_TO_ALGO_MAPPING_VECTOR, -1) {
    LoadMD5SUMStrategyNamesToCodeDatabase(exchange);
  }

  /// Used to get server_assigned_client_id_mantissa_ ( which is a small number and hence an array index ) by
  /// subtracting from server_assigned_client_id_.
  /// Equivalent to just ( & 0x0000ffff )
  inline int SACItoKey(int _server_assigned_client_id_) const {
    return (_server_assigned_client_id_ & SACI_MANTISSA_HEX);
  }

 public:
  static ETIAlgoTagging& GetUniqueInstance(const std::string& exchange) {
    static ETIAlgoTagging unique_instance_(exchange);
    return unique_instance_;
  }

  void SetORSControlNetworkConfiguration(const std::string& _ors_control_ip_, const int32_t& _ors_control_port_) {
    ors_control_ip_ = _ors_control_ip_;
    ors_control_port_ = _ors_control_port_;
  }

  // explicit way to reload database
  void ETIAlgoTaggingReloadDatabase(const std::string& exch_) { LoadMD5SUMStrategyNamesToCodeDatabase(exch_); }

  bool AddClientInfo(const int32_t& _client_id_, const std::string& _md5sum_, const std::string& _strategy_name_) {
    mutex_lock_.LockMutex();

    const int key_obtained_from_saci = SACItoKey(_client_id_);
    if ((key_obtained_from_saci < (int)client_id_to_algo_mapping_.size()) &&
        (client_id_to_algo_mapping_[key_obtained_from_saci] != -1)) {
      // Client Already Added
      // Notify Alert

      std::ostringstream t_temp_oss_;
      t_temp_oss_ << " The Client ID : " << _client_id_
                  << " Has Already Been Added With Algo As : " << client_id_to_algo_mapping_[key_obtained_from_saci]
                  << " MD5SUM : " << client_id_to_md5sum_[_client_id_] << " "
                  << client_id_to_strategy_name_[_client_id_] << "\n";

      std::cerr << t_temp_oss_.str();
      EmailETIAlgoTaggingAlert(t_temp_oss_.str());

      mutex_lock_.UnlockMutex();
      return false;
    }

    std::string client_md5sum_strategy_name = std::string(_md5sum_) + std::string(_strategy_name_);

    if (md5sum_strategy_name_to_base_algo_code_map_.find(client_md5sum_strategy_name) ==
        md5sum_strategy_name_to_base_algo_code_map_.end()) {
      std::ostringstream t_temp_oss_;
      t_temp_oss_ << " SACI : " << _client_id_ << " StrategyType : " << _strategy_name_ << " MD5SUM " << _md5sum_
                  << " Combination Doesn't Match With Any Defined Algo Code \n";
      EmailETIAlgoTaggingAlert(t_temp_oss_.str());

      mutex_lock_.UnlockMutex();
      return false;
    }

    uint32_t unique_regulatory_id_ = md5sum_strategy_name_to_base_algo_code_map_[client_md5sum_strategy_name];

    while ((int)client_id_to_algo_mapping_.size() <= key_obtained_from_saci) {
      client_id_to_algo_mapping_.push_back(-1);
    }
    client_id_to_algo_mapping_[key_obtained_from_saci] = unique_regulatory_id_;

    std::ostringstream t_temp_oss_;
    t_temp_oss_ << " SACI : " << _client_id_ << " StrategyType : " << _strategy_name_ << " MD5SUM : " << _md5sum_
                << " Exec Update Information : " << md5sum_strategy_name_to_desc_map_[_md5sum_]
                << " Regulatory ID : " << unique_regulatory_id_ << "\n";
    //        EmailETIAlgoTaggingAlert ( t_temp_oss_.str (), false ) ;

    mutex_lock_.UnlockMutex();
    return true;
  }

  void TagAlgo(const int& _client_id_, const std::string& _md5sum_, const std::string& _strategy_name_,
               const std::string& exch_) {
    HFSAT::TCPClientSocket tcp_client_socket_;
    tcp_client_socket_.Connect(ors_control_ip_, ors_control_port_);

    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "CONTROLCOMMAND"
                << " "
                << "TagETIAlgo"
                << " " << _client_id_ << " " << _md5sum_ << " " << _strategy_name_ << " " << exch_;

    int write_length_ = tcp_client_socket_.WriteN((t_temp_oss_.str()).length(), (void*)(t_temp_oss_.str().c_str()));

    if (write_length_ < (int)((t_temp_oss_.str()).length())) {
      std::ostringstream t_temp_oss_;
      t_temp_oss_ << " Failed To Tag ETI Algo, Control Write Failed, Expected to Write : "
                  << (t_temp_oss_.str()).length() << " Written : " << write_length_ << " Error : " << strerror(errno)
                  << "\n";
      std::cerr << t_temp_oss_.str();
      EmailETIAlgoTaggingAlert(t_temp_oss_.str());
      exit(-1);
    }
    tcp_client_socket_.Close();
  }

  bool ShouldAllowThisClientToTrade(const int& _client_id_) {
    const int key_obtained_from_saci = SACItoKey(_client_id_);
    if ((key_obtained_from_saci >= (int)client_id_to_algo_mapping_.size()) ||
        (client_id_to_algo_mapping_[key_obtained_from_saci] == -1)) {
      std::ostringstream t_temp_oss_;
      t_temp_oss_ << "CLIENT : " << _client_id_ << " NOT ALLOWED TO TRADE "
                  << " Client ID : " << _client_id_;
      EmailETIAlgoTaggingAlert(t_temp_oss_.str());

      return false;
    }

    return true;
  }

  const uint32_t GetComplianceIdForClient(const int& _client_id_) {
    const int key_obtained_from_saci = SACItoKey(_client_id_);
    if ((key_obtained_from_saci >= (int)client_id_to_algo_mapping_.size()) ||
        (client_id_to_algo_mapping_[key_obtained_from_saci] == -1)) {
      std::ostringstream t_temp_oss_;
      t_temp_oss_ << "UNABLE TO TAG ALGO FOR CLIENT ID : " << _client_id_;
      EmailETIAlgoTaggingAlert(t_temp_oss_.str());

      return 0;
    }
    return client_id_to_algo_mapping_[key_obtained_from_saci];
  }

  // Only used by fake send
  const uint32_t GetComplianceIdForClientFake(const int& key_obtained_from_saci) {
    if ((key_obtained_from_saci >= (int)client_id_to_algo_mapping_.size()) ||
        (client_id_to_algo_mapping_[key_obtained_from_saci] == -1)) {
      return 0;
    }
    return client_id_to_algo_mapping_[key_obtained_from_saci];
  }
};
}
}
