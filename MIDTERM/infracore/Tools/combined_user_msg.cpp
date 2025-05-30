#include <time.h>
#include <iostream>
#include <getopt.h>
#include <string>

#include "dvccode/Utils/multicast_sender_socket.hpp"
#include "dvccode/TradingInfo/network_account_info_manager.hpp"
#include "dvccode/CombinedControlUtils/combined_control_messages.hpp"
#include "dvccode/TradingInfo/network_account_interface_manager.hpp"
#include "dvccode/CDef/trading_location_manager.hpp"

#include "dvccode/CDef/ttime.hpp"
#include "dvccode/CDef/file_utils.hpp"

static struct option umsg_options_[] = {{"help", no_argument, 0, 'h'},
                                        {"set_tolerance", required_argument, 0, 'a'},
                                        {"exchange", required_argument, 0, 'b'},
                                        {"dump_live_orders", no_argument, 0, 'c'},
                                        {"traderid", required_argument, 0, 'd'},
                                        {"only_on_location", required_argument, 0, 'e'},
                                        {"dump_mds_files", no_argument, 0, 'f'},
                                        {"only_ors_files", required_argument, 0, 'g'},
                                        {"add_rm_shortcode", required_argument, 0, 'i'},
                                        {"shortcode", required_argument, 0, 'j'},
                                        {0, 0, 0, 0}};

void print_usage(const char* prg_name) {
  printf("This is the combined user_msg exec \n");
  printf("Usage: %s --option <optional_val> [ --option <optional_val> ... ]\n", prg_name);
  printf("Valid options are:\n");
  printf("--help\t Displays usage_menu \n");
  printf(
      "--set_tolerance <tolerance> --exchange <exchnage> [ --only_on_location <0/1> ] \tSets Alert Tolerance of an "
      "Exchange \n");
  printf(
      "--dump_live_orders --traderid <traderid> \tDirects Combined Writer to Dump Live Orders for given TraderID \n");
  printf(
      "--dump_mds_files [ --only_ors_files <0/1> ] \tDirects Combined Writer to Dump Mds Files. By default only dumps "
      "ors_mds files. \n");
  printf(
      "--add_rm_shortcode <1/2/3> --shortcode <shc>  \t Add/remove shortcode to listen data for. 1 - Add, 2- "
      "Remove 3-Show.\n");
}

int main(int argc, char** argv) {
  int c;
  int tolerance_ = -1;
  int trader_id_ = -1;
  bool only_on_location_ = false;
  bool only_ors_files_ = true;
  HFSAT::SignalType signal = HFSAT::kSignalInvalid;
  std::string exchange_ = "";
  char buffer[ADD_RM_SHC_MSG_BUFFER_LEN];

  memset(buffer, 0, ADD_RM_SHC_MSG_BUFFER_LEN);

  HFSAT::CombinedControlMessageCode_t msg_code_ = HFSAT::kCmdControlMessageCodeMax;
  /// parse input options
  while (1) {
    int option_index = 0;
    c = getopt_long(argc, argv, "", umsg_options_, &option_index);
    if (c == -1) break;
    switch (c) {
      case 'h':
        print_usage(argv[0]);
        exit(0);

      case 'a':
        msg_code_ = HFSAT::kCmdControlMessageCodeChangeTolerance;
        tolerance_ = atoi(optarg);
        break;

      case 'b':
        exchange_ = std::string(optarg);
        break;

      case 'c':
        msg_code_ = HFSAT::kCmdControlMessageCodeDumpLiveOrders;
        break;

      case 'd':
        trader_id_ = atoi(optarg);
        break;

      case 'e':
        only_on_location_ = atoi(optarg) ? true : false;
        break;

      case 'f':
        msg_code_ = HFSAT::kCmdControlMessageCodeDumpMdsFiles;
        break;

      case 'g':
        only_ors_files_ = atoi(optarg) ? true : false;
        break;

      case 'i':
        msg_code_ = HFSAT::kCmdControlMessageCodeAddRemoveShortcode;
        signal = static_cast<HFSAT::SignalType>(atoi(optarg));
        break;

      case 'j':
        memcpy(buffer, optarg, ADD_RM_SHC_MSG_BUFFER_LEN);
        break;

      case '?':
        print_usage(argv[0]);
        exit(0);
        break;
    }
  }

  if (msg_code_ == HFSAT::kCmdControlMessageCodeMax) {
    printf("User must choose valid option to send to CombinedWriter \n");
    print_usage(argv[0]);
    exit(0);
  }

  /// get broadcast ip/port info from network info manager
  HFSAT::NetworkAccountInfoManager network_account_info_manager_;
  HFSAT::DataInfo combined_control_recv_data_info_ = network_account_info_manager_.GetCombControlDataInfo();

  /// create broadcast socket
  HFSAT::MulticastSenderSocket sock_(
      combined_control_recv_data_info_.bcast_ip_, combined_control_recv_data_info_.bcast_port_,
      HFSAT::NetworkAccountInterfaceManager::instance().GetInterfaceForApp(HFSAT::k_Control));

  /// create struct; populate it and send it along
  HFSAT::CombinedControlMessage* comb_control_request_ = new HFSAT::CombinedControlMessage();

  char hostname[1024];
  hostname[1023] = '\0';
  gethostname(hostname, 1023);

  strcpy(comb_control_request_->location_, hostname);
  comb_control_request_->message_code_ = msg_code_;

  switch (msg_code_) {
    case HFSAT::kCmdControlMessageCodeChangeTolerance: {
      HFSAT::ExchSource_t exch_source = HFSAT::StringToExchSource(exchange_);

      if (exchange_ == "NTP")  // NTP converts to kExchSourceBMF, but we want is as kExchSourceNTP
        exch_source = HFSAT::kExchSourceNTP;

      comb_control_request_->generic_combined_control_msg_.tolerance_msg_.exch_src_ = exch_source;
      comb_control_request_->generic_combined_control_msg_.tolerance_msg_.tolerance_ = tolerance_;
      comb_control_request_->generic_combined_control_msg_.tolerance_msg_.on_location_only_ = only_on_location_;
    } break;

    case HFSAT::kCmdControlMessageCodeDumpLiveOrders: {
      comb_control_request_->generic_combined_control_msg_.dump_live_orders_msg_.trader_id_ = trader_id_;
    } break;

    case HFSAT::kCmdControlMessageCodeDumpMdsFiles: {
      comb_control_request_->generic_combined_control_msg_.dump_mds_files_.dump_only_ors_files_ = only_ors_files_;
    } break;

    case HFSAT::kCmdControlMessageCodeAddRemoveShortcode: {
      comb_control_request_->generic_combined_control_msg_.add_rm_shortcode_.query_id_ = trader_id_;
      comb_control_request_->generic_combined_control_msg_.add_rm_shortcode_.len_ = strlen(buffer);
      comb_control_request_->generic_combined_control_msg_.add_rm_shortcode_.signal_ = signal;
      memcpy(comb_control_request_->generic_combined_control_msg_.add_rm_shortcode_.buffer, buffer,
             ADD_RM_SHC_MSG_BUFFER_LEN);
    } break;

    default:
      break;
  }

  std::cout << " IP: " << combined_control_recv_data_info_.bcast_ip_
            << " Port: " << combined_control_recv_data_info_.bcast_port_
            << " Iface: " << HFSAT::NetworkAccountInterfaceManager::instance().GetInterfaceForApp(HFSAT::k_Control)
            << " Message: " << comb_control_request_->ToString() << "\n";

  sock_.WriteN(sizeof(HFSAT::CombinedControlMessage), comb_control_request_);
}
