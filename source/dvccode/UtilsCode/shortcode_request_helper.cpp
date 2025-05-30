#include "dvccode/Utils/shortcode_request_helper.hpp"

namespace HFSAT {

ShortcodeRequestHelper::ShortcodeRequestHelper(int query_id)
    : query_id_(query_id), sock_(nullptr), comb_control_request_(nullptr) {
  /// get broadcast ip/port info from network info manager
  HFSAT::NetworkAccountInfoManager network_account_info_manager;
  HFSAT::DataInfo combined_control_recv_data_info = network_account_info_manager.GetCombControlDataInfo();

  /// create broadcast socket
  sock_ = new HFSAT::MulticastSenderSocket(
      combined_control_recv_data_info.bcast_ip_, combined_control_recv_data_info.bcast_port_,
      HFSAT::NetworkAccountInterfaceManager::instance().GetInterfaceForApp(HFSAT::k_Control));

  std::cout << "IP: " << combined_control_recv_data_info.bcast_ip_
            << " Port: " << combined_control_recv_data_info.bcast_port_
            << " IFace: " << HFSAT::NetworkAccountInterfaceManager::instance().GetInterfaceForApp(HFSAT::k_Control)
            << std::endl;

  comb_control_request_ = new HFSAT::CombinedControlMessage();

  char hostname[1024];
  hostname[1023] = '\0';
  gethostname(hostname, 1023);

  strcpy(comb_control_request_->location_, hostname);
  comb_control_request_->message_code_ = HFSAT::kCmdControlMessageCodeAddRemoveShortcode;
  comb_control_request_->generic_combined_control_msg_.add_rm_shortcode_.query_id_ = query_id_;
}

void ShortcodeRequestHelper::SendMessage() {
  sock_->WriteN(sizeof(HFSAT::CombinedControlMessage), comb_control_request_);
}

void ShortcodeRequestHelper::SetVars(HFSAT::SignalType signal, std::string buffer) {
  comb_control_request_->generic_combined_control_msg_.add_rm_shortcode_.len_ = buffer.length();
  comb_control_request_->generic_combined_control_msg_.add_rm_shortcode_.signal_ = signal;
  memset(comb_control_request_->generic_combined_control_msg_.add_rm_shortcode_.buffer, 0, ADD_RM_SHC_MSG_BUFFER_LEN);
  memcpy(comb_control_request_->generic_combined_control_msg_.add_rm_shortcode_.buffer, buffer.c_str(),
         buffer.length());
}

void ShortcodeRequestHelper::AddShortcodeToListen(std::string shc) {
  shortcode_list_.push_back(shc);
  SetVars(HFSAT::kSignalAdd, shc);
  SendMessage();
}

void ShortcodeRequestHelper::AddShortcodeListToListen(std::vector<std::string> shortcode_list) {
  std::string buffer = "";

  for (auto shc : shortcode_list) {
    shortcode_list_.push_back(shc);
    if (buffer.length() + shc.length() + 5 > ADD_RM_SHC_MSG_BUFFER_LEN) {
      buffer[buffer.length() - 1] = '\0';
      SetVars(HFSAT::kSignalAdd, buffer);
      SendMessage();
      buffer = "";
    }

    buffer = buffer + shc + "|";
  }

  if (buffer.length() > 0) {
    buffer[buffer.length() - 1] = '\0';
    SetVars(HFSAT::kSignalAdd, buffer);
    SendMessage();
  }
}

void ShortcodeRequestHelper::RemoveShortcodeListToListen(std::vector<std::string> shortcode_list) {
  std::string buffer = "";

  for (auto shc : shortcode_list) {
    if (buffer.length() + shc.length() + 5 > ADD_RM_SHC_MSG_BUFFER_LEN) {
      buffer[buffer.length() - 1] = '\0';
      SetVars(HFSAT::kSignalRemove, buffer);
      SendMessage();
      buffer = "";
    }

    buffer = buffer + shc + "|";
  }

  if (buffer.length() > 0) {
    buffer[buffer.length() - 1] = '\0';
    SetVars(HFSAT::kSignalRemove, buffer);
    SendMessage();
  }
}

void ShortcodeRequestHelper::RemoveAllShortcodesToListen() { RemoveShortcodeListToListen(shortcode_list_); }
}
