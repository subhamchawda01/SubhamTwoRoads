#pragma once

#include <string>

#include "dvccode/Utils/multicast_sender_socket.hpp"
#include "dvccode/TradingInfo/network_account_info_manager.hpp"
#include "dvccode/CombinedControlUtils/combined_control_messages.hpp"
#include "dvccode/TradingInfo/network_account_interface_manager.hpp"
#include "dvccode/CDef/trading_location_manager.hpp"

namespace HFSAT {

class ShortcodeRequestHelper {
 private:
  int query_id_;
  HFSAT::MulticastSenderSocket* sock_;
  HFSAT::CombinedControlMessage* comb_control_request_;
  char hostname[1024];
  std::vector<std::string> shortcode_list_;

  void AddShortcodeToListen(std::string);

 public:
  ShortcodeRequestHelper(int query_id);
  ~ShortcodeRequestHelper() {}

  void SendMessage();

  void SetVars(HFSAT::SignalType signal, std::string buffer);

  void AddShortcodeListToListen(std::vector<std::string> shortcode_list);
  void RemoveShortcodeListToListen(std::vector<std::string> shortcode_list);
  void RemoveAllShortcodesToListen();
};
}
