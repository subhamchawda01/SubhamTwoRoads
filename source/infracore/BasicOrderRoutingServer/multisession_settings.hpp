#ifndef _MULTISESSION_SETTINGS_HPP
#define _MULTISESSION_SETTINGS_HPP

#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <stdlib.h>
#include <string>
#include <vector>

#include "dvccode/Utils/settings.hpp"

namespace HFSAT {
namespace ORS {

class MultisessionSettings {
 public:
  MultisessionSettings(const std::string filename);
  ~MultisessionSettings() {}

  Settings* GetSessionSettings(int session_id);
  void DumpSettingsToFile();

 private:
  static void UpdateKeys(Settings& session_settings, int session_id);
  static std::string GetSessionKey(std::string key, int session_id);

  Settings master_settings_;
  std::vector<std::pair<int, Settings*> > session_settings_;
  bool is_modified_;
};
}
}

#endif
