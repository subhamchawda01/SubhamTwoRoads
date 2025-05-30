#include "infracore/BasicOrderRoutingServer/multisession_settings.hpp"

#include <sstream>

namespace HFSAT {
namespace ORS {

static const char* xs[] = {
    "Password",          "SenderCompID",  "SocketConnectHost", "SocketConnectPort", "TargetCompID",
    "AccountName",       "UserID",        "TraderID",          "UserName",          "TraderUserName",
    "SenderSubID",       "SharedOrderID", "Partition",         "ORSThrottleLimit",  "NNF",
    "PasswordChangeDate", "BoxId", "TAPIP", "TAPInterface", "AlgoId"};

MultisessionSettings::MultisessionSettings(const std::string filename)
    : master_settings_(filename), is_modified_(false) {
  auto sessions = master_settings_.getValue("Multisessions", "0");
  std::replace(sessions.begin(), sessions.end(), ',', ' ');
  std::stringstream ss(sessions);
  int i = 0;
  while (ss >> i) {
    Settings* currrent_session_settings = new Settings(master_settings_);
    if (currrent_session_settings == nullptr) {
      std::cout << "Could not create Settings for Multisession Id " << i << ". Exiting." << std::endl;
      exit(1);
    }
    session_settings_.push_back(std::make_pair(i, currrent_session_settings));
    UpdateKeys(*currrent_session_settings, i);
  }
}

std::string MultisessionSettings::GetSessionKey(std::string key, int session_id) {
  std::ostringstream sstream;
  sstream << key << "-" << session_id;
  return sstream.str();
}

void MultisessionSettings::UpdateKeys(Settings& session_settings, int session_id) {
  for (auto key : xs) {
    std::string session_key = GetSessionKey(key, session_id);
    if (session_settings.has(session_key)) session_settings.setValue(key, session_settings.getValue(session_key));
  }
}

Settings* MultisessionSettings::GetSessionSettings(int session_id) {
  for (uint32_t i = 0; i < session_settings_.size(); i++) {
    if (session_settings_[i].first == session_id) {
      return session_settings_[i].second;
    }
  }

  return nullptr;
}

void MultisessionSettings::DumpSettingsToFile() {
  for (unsigned int i = 0; i < session_settings_.size(); i++) {
    int session_id = session_settings_[i].first;
    Settings& session_setting = *session_settings_[i].second;

    for (auto key : xs) {
      std::string session_key = GetSessionKey(key, session_id);  // is_modified_
      if (session_setting.has(session_key)) {
        if (!(master_settings_.getValue(session_key) == session_setting.getValue(key))) {
          master_settings_.setValue(session_key, session_setting.getValue(key));
          is_modified_ = true;
        }
      }
    }
  }

  if (is_modified_) {
    master_settings_.DumpFile();
  }
}
}
}
