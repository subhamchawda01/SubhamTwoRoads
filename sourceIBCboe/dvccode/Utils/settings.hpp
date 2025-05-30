/**
    \file BasicOrderRoutingServer/settings.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066
         India
         +91 80 4060 0717
 */

#ifndef BASE_BASICORDERROUTINGSERVER_SETTINGS_H
#define BASE_BASICORDERROUTINGSERVER_SETTINGS_H
#include <fstream>
#include <string.h>
#include <stdlib.h>
#include <string>
#include <map>
#include <iostream>

namespace HFSAT {
namespace ORS {
class Settings {
 public:
  Settings(const std::string file);
  ~Settings() { values.clear(); }

  bool has(std::string key) const;
  std::string getValue(std::string key, const char* default_value = "INVALIDKEY") const;
  std::string getValue(std::string key, const std::string& default_value) const;
  int getIntValue(const std::string& key, int default_value) const;
  uint64_t getUIntValue(const std::string& key, int default_value) const;
  void setValue(std::string key, std::string value) { values[key] = value; }
  std::string GetFileName();
  void UpdateKeyValueAndCommit(std::string _key_, std::string _updated_value_);
  void DumpFile();
  std::string ToString();

 protected:
  std::map<std::string, std::string> values;
  std::string config_file_;
};
}
}
#endif  // BASE_BASICORDERROUTINGSERVER_SETTINGS_H
