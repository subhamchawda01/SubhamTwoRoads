/**
   \file BasicOrderRoutingServer/settings.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066
   India
   +91 80 4060 0717
 */

#include <sstream>
#include "dvccode/Utils/settings.hpp"
namespace HFSAT {
namespace ORS {
Settings::Settings(const std::string file) : config_file_(file) {
  values.clear();
  /// read key value pairs of settings from file
  std::ifstream cfile;
  cfile.open(file.c_str(), std::ofstream::in);
  if (!cfile.is_open()) {
    fprintf(stderr, "Cannot open file '%s' for reading \n", file.c_str());
    exit(-1);
  }
  char line[1024];
  while (!cfile.eof()) {
    memset(line, 0, sizeof(line));
    cfile.getline(line, sizeof(line));
    //      if ( strlen( line ) == 0 || index( line, '#' ) != NULL )
    if (strlen(line) == 0)  // ETI session have # in password
      continue;
    char* key = strtok(line, "\t\n ");
    char* value = strtok(NULL, "\t\n ");
    if (key && value) values[key] = value;
  }
  cfile.close();
}

bool Settings::has(std::string key) const { return (values.find(key) != values.end()); }

int Settings::getIntValue(const std::string& key, int default_value) const {
  std::map<std::string, std::string>::const_iterator cit_ = values.find(key);
  return (cit_ == values.end()) ? default_value : atoi(cit_->second.c_str());
}

uint64_t Settings::getUIntValue(const std::string& key, int default_value) const {
  std::map<std::string, std::string>::const_iterator cit_ = values.find(key);
  return (cit_ == values.end()) ? default_value : std::stoull(cit_->second.c_str());
}

std::string Settings::getValue(std::string key, const std::string& default_value) const {
  auto v = values.find(key);
  return v == values.end() ? default_value : v->second;
}
std::string Settings::getValue(std::string key, const char* default_value) const {
  std::map<std::string, std::string>::const_iterator cit_ = values.find(key);
  if (cit_ == values.end()) {
    return default_value;
  } else {
    return cit_->second;
  }
}

std::string Settings::GetFileName() { return config_file_; }

void Settings::UpdateKeyValueAndCommit(std::string _key_, std::string _updated_value_) {
  if (values.find(_key_) == values.end()) {
    std::cerr << " Key : " << _key_ << " Doesn't Exist In The Map, Failed To Upload The New Value : " << _updated_value_
              << "\n";
    return;
  }

  values[_key_] = _updated_value_;

  DumpFile();
}

void Settings::DumpFile() {
  std::ofstream config_file_stream;
  config_file_stream.open(config_file_.c_str(), std::ios::out);

  if (!config_file_stream.is_open()) {
    std::cerr << " Failed To Open The Config File For Writing : " << config_file_ << "\n";
    return;
  }

  for (std::map<std::string, std::string>::const_iterator cit = values.begin(); cit != values.end(); cit++) {
    config_file_stream << cit->first << " " << cit->second << "\n";
  }

  config_file_stream.close();
}

std::string Settings::ToString() {
  std::ostringstream t_temp_oss;
  t_temp_oss << "===================== ORS Config File =====================\n";

  for (auto& itr : values) {
    t_temp_oss << itr.first << "->" << itr.second << "\n";
  }

  t_temp_oss << "===========================================================\n";

  return t_temp_oss.str();
}
}
}
