#pragma once

#include "dvccode/Utils/tinyexpr.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include <string>
#include <regex>
#include <sstream>

class Parser {
  Parser();
  ~Parser();

 public:
  static void ParseConfig(const std::string& _config_filename_, std::map<std::string, std::string>& _key_val_map_,
                          unsigned int val_token_number_ = 1, unsigned int date_ = 0) {
    std::ifstream ifs_(_config_filename_.c_str(), std::ifstream::in);
    if (ifs_.is_open()) {
      const int kBufferLength = 1024;
      char buffer_[kBufferLength];
      while (ifs_.good()) {
        bzero(buffer_, kBufferLength);
        ifs_.getline(buffer_, kBufferLength);
        std::string t_val_;
        HFSAT::PerishableStringTokenizer::TrimString(buffer_, t_val_);

        std::vector<char*> tokens_;
        HFSAT::PerishableStringTokenizer::NonConstStringTokenizer(buffer_, " =", tokens_);
        if (tokens_.empty() || tokens_[0][0] == '#' || tokens_.size() < (val_token_number_ + 1)) {
          // std::cerr << " Cant handle inappropriate line " << std::endl;
          continue;
        } else {
          if (date_ != 0 && date_ < 20190116 &&
              std::string(tokens_[val_token_number_]).find("IDFCFIRSTB") != std::string::npos) {
            std::string dummy = std::string(tokens_[val_token_number_]);
            dummy.replace(dummy.find("IDFCFIRSTB"), 10, "IDFCBANK");
            strcpy(tokens_[val_token_number_], dummy.c_str());
            // std::cout << " Adding to map " << tokens_[0] << " === " << tokens_[1]<< std::endl;
          }
          _key_val_map_[tokens_[0]] = tokens_[val_token_number_];
        }
      }
      ifs_.close();
    } else {
      std::cerr << "Could not open " << _config_filename_ << " for reading " << std::endl;
    }
  }

  static double GetDouble(std::map<std::string, std::string>* key_val_map, std::string key, double default_val, uint32_t dep_one_lot_size_val=0, uint32_t indep_one_lot_size_val=0) {
    std::map<std::string, std::string>::iterator iter = key_val_map->find(key);
    if (iter != key_val_map->end()) {

      //contains regex for dep
      if((iter->second).find(_DEP_ONE_LOT_SIZE_) != std::string::npos){

        std::string value = std::string(iter->second);

        std::ostringstream t_oss;
        t_oss << dep_one_lot_size_val;

        std::string ip_regex = std::regex_replace(value,std::regex(_DEP_ONE_LOT_SIZE_),t_oss.str().c_str());
        return te_interp(ip_regex.c_str(), 0);

      }else if((iter->second).find(_INDEP_ONE_LOT_SIZE_) != std::string::npos){

        std::string value = std::string(iter->second);

        std::ostringstream t_oss;
        t_oss << indep_one_lot_size_val;

        std::string ip_regex = std::regex_replace(value,std::regex(_INDEP_ONE_LOT_SIZE_),t_oss.str().c_str());
        return te_interp(ip_regex.c_str(), 0);

      }else {
        return std::stod(iter->second);
      }
    } else {
      return default_val;
    }
  }

  static int GetInt(std::map<std::string, std::string>* key_val_map, std::string key, int default_val, uint32_t dep_one_lot_size_val=0, uint32_t indep_one_lot_size_val=0) {
    std::map<std::string, std::string>::iterator iter = key_val_map->find(key);
    if (iter != key_val_map->end()) {

      //contains regex for dep
      if((iter->second).find(_DEP_ONE_LOT_SIZE_) != std::string::npos){

        std::string value = std::string(iter->second);

        std::ostringstream t_oss;
        t_oss << dep_one_lot_size_val;

        std::string ip_regex = std::regex_replace(value,std::regex(_DEP_ONE_LOT_SIZE_),t_oss.str().c_str());
        return te_interp(ip_regex.c_str(), 0);

      }else if((iter->second).find(_INDEP_ONE_LOT_SIZE_) != std::string::npos){

        std::string value = std::string(iter->second);

        std::ostringstream t_oss;
        t_oss << indep_one_lot_size_val;

        std::string ip_regex = std::regex_replace(value,std::regex(_INDEP_ONE_LOT_SIZE_),t_oss.str().c_str());
        return te_interp(ip_regex.c_str(), 0);

      } else {
        return std::stoi(iter->second);
      }
    } else {
      return default_val;
    }
  }

  static bool GetBool(std::map<std::string, std::string>* key_val_map, std::string key, bool default_val) {
    std::map<std::string, std::string>::iterator iter = key_val_map->find(key);
    if (iter != key_val_map->end()) {
      if (iter->second == "true" || iter->second == "True" || iter->second == "TRUE" || iter->second == "1")
        return true;
      else
        return false;
    } else {
      return default_val;
    }
  }

  static std::string GetString(std::map<std::string, std::string>* key_val_map, std::string key,
                               std::string default_val) {
    std::map<std::string, std::string>::iterator iter = key_val_map->find(key);
    if (iter != key_val_map->end()) {
      return iter->second;
    } else {
      return default_val;
    }
  }
};
