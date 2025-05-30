/**
    \file dvccode/Utils/recovery_manager_config_parser.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/

#pragma once

#include <iostream>
#include <cstdlib>
#include <fstream>
#include <signal.h>
#include <unistd.h>
#include <string>
#include <vector>
#include <stdio.h>

#include "dvccode/CDef/defines.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

namespace HFSAT {

class RecoveryManagerConfigParser {
 public:
  RecoveryManagerConfigParser() : client_port_(RECOVERY_HOST_PORT), is_initialised_(false) {}

  ~RecoveryManagerConfigParser() {}

  void Initialise( std::string exchange_file = "") {
    if (is_initialised_) return;

    is_initialised_ = true;

    char hostname[128];
    hostname[127] = '\0';
    gethostname(hostname, 127);
    std::string config_file_name(hostname);

    if( exchange_file.length() > 0 ){
      config_file_name += ( "_" + exchange_file ) ; //Optional arg
    }

    config_file_name += "_recovery_manager.cfg";

    config_file_path_ = PROD_CONFIGS_DIR + config_file_name;

    std::ifstream file_handler;

    file_handler.open(config_file_path_.c_str(), std::ofstream::in);

    if (!file_handler.is_open()) {
      fprintf(stderr, "Cannot open file %s for reading reference data\n", config_file_path_.c_str());
      exit(-1);
    }

    char line[1024];

    while (!file_handler.eof()) {
      memset(line, 0, sizeof(line));

      file_handler.getline(line, sizeof(line));

      if (strlen(line) == 0 || line[0] == '#')  /// comments etc
        continue;

      std::vector<char*> tokens;
      HFSAT::PerishableStringTokenizer::NonConstStringTokenizer(line, " ", tokens);

      if (tokens.size() < 2) continue;

      std::string argument_name = tokens[0];
      if (argument_name == "RECOVERY_IP") {
        if (tokens.size() < 3) continue;
        std::string tmp_exch_name = tokens[1];
        ValidateExchangeName(tmp_exch_name);
        recovery_ip_[tmp_exch_name] = tokens[2];
      }else if ( argument_name == "RECOVERY") {
        if (tokens.size() < 4) continue;
        std::string tmp_exch_name = tokens[1];
        ValidateExchangeName(tmp_exch_name);
        recovery_ip_[tmp_exch_name] = tokens[2];
        recovery_port_[tmp_exch_name] = atoi(tokens[3]);
      } else if (argument_name == "EXCHANGE") {
        std::string tmp_exch_name = tokens[1];
        ValidateExchangeName(tmp_exch_name);
        exchange_sources_.push_back(tmp_exch_name);
      } else if (argument_name == "CLIENT_PORT") {
        client_port_ = atoi(tokens[1]);
      }
    }
    file_handler.close();
  }

  void ValidateExchangeName(std::string t_exchange) {
    if (HFSAT::exchange_name_str_to_int_mapping.find(t_exchange) == HFSAT::exchange_name_str_to_int_mapping.end()) {
      std::cerr << "EXCHANGE SOURCE <" << t_exchange
                << "> NOT DEFINED IN CDEF/DEFINES exchange_name_str_to_int_mapping MAP..." << std::endl;
      exit(-1);
    }
    return;
  }

  std::string GetRecoveryHostIP(int32_t t_int_exchange) {
    std::string str_exchange = "INVALID";
    for (auto it : (HFSAT::exchange_name_str_to_int_mapping)) {
      if (it.second == t_int_exchange) {
        str_exchange = it.first;
        break;
      }
    }

    if (str_exchange == "INVALID") {
      fprintf(stderr, "EXCHANGE SOURCE <%d> NOT DEFINED IN CDEF/DEFINES exchange_name_str_to_int_mapping MAP...\n",
              t_int_exchange);
      exit(-1);
    }

    if (recovery_ip_.find(str_exchange) == recovery_ip_.end()) {
      fprintf(stderr, "RecoveryHostIP for <%s> is not present in %s\n", str_exchange.c_str(),
              config_file_path_.c_str());
      exit(-1);
    }
    return recovery_ip_[str_exchange];
  }

  int GetRecoveryHostClientPort() { return client_port_; }

  int GetRecoveryHostClientPort(int32_t t_int_exchange) { 
    std::string str_exchange = "INVALID";
    for (auto it : (HFSAT::exchange_name_str_to_int_mapping)) {
      if (it.second == t_int_exchange) {
        str_exchange = it.first;
        break;
      }
    }

    if (str_exchange == "INVALID") {
      fprintf(stderr, "EXCHANGE SOURCE <%d> NOT DEFINED IN CDEF/DEFINES exchange_name_str_to_int_mapping MAP...\n",
              t_int_exchange);
      exit(-1);
    }

    if (recovery_port_.find(str_exchange) == recovery_port_.end()) {
      fprintf(stderr, "RecoveryHostIP for <%s> is not present in %s\n", str_exchange.c_str(),
              config_file_path_.c_str());
      return client_port_ ; 
    }
    return recovery_port_[str_exchange];
  }

  std::vector<std::string> GetAllExchangeSources() { return exchange_sources_; }

 private:
  int client_port_;
  bool is_initialised_;
  std::map<std::string, std::string> recovery_ip_;
  std::map<std::string, int32_t> recovery_port_;
  std::vector<std::string> exchange_sources_;
  std::string config_file_path_;
};
}
