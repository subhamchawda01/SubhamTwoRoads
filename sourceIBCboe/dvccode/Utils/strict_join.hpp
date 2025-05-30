#pragma once
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unistd.h>
#include "dvccode/CDef/defines.hpp"
#define WHICH_ADDR_TO_JOIN "which_addr_to_join"

class SocketJoinType {
  // const std::string interface_ip_;
 public:
  static bool IsStrictJoin() {
    char hostname[128];
    hostname[127] = '\0';
    gethostname(hostname, 127);
    bool addr_join_type = false;

    std::stringstream ss;
    ss << PROD_CONFIGS_DIR << WHICH_ADDR_TO_JOIN << "_" << hostname;
    std::string addr_to_join_config_filename = ss.str();

    std::ifstream addr_to_join_config_file;
    addr_to_join_config_file.open(addr_to_join_config_filename.c_str());
    if (addr_to_join_config_file.is_open()) {
      if (addr_to_join_config_file.good()) {
        char buffer[1024];
        addr_to_join_config_file.getline(buffer, 1024);
        if (strcmp(buffer, "STRICT") == 0) {
          addr_join_type = true;
          std::cout << "Found strict join file:" << addr_to_join_config_filename << std::endl;
        }
        addr_to_join_config_file.close();
      }
    } else {
      // In this case: if the file is not present, addr_join_=false (by default value).
    }
    return addr_join_type;
  }

  static std::string& GetStrictJoinIp() {
    static std::string interface_ip_ = "127.0.0.1";
    return interface_ip_;
  }
};
