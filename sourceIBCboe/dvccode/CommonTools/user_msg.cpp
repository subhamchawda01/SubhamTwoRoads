#include <time.h>
#include <iostream>
#include <getopt.h>

#include "dvccode/Utils/multicast_sender_socket.hpp"
#include "dvccode/TradingInfo/network_account_info_manager.hpp"
#include "dvccode/CDef/control_messages.hpp"
#include "dvccode/TradingInfo/network_account_interface_manager.hpp"

#include "dvccode/CDef/ttime.hpp"
#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CDef/math_utils.hpp"

static struct option umsg_options_[] = {
    // please maintain the order
    {"getflat", no_argument, 0, 'a'},
    {"start", no_argument, 0, 'b'},
    {"setstarttime", required_argument, 0, 'c'},
    {"setendtime", required_argument, 0, 'd'},
    // {"freeze",     no_argument,      0, 'c' },
    // {"unfreeze",   no_argument,      0, 'd' },
    {"setunitsize", required_argument, 0, 'e'},
    {"setmaxpos", required_argument, 0, 'f'},
    {"setworstpos", required_argument, 0, 'g'},
    {"help", no_argument, 0, 'h'},
    {"enableimprove", no_argument, 0, 'i'},
    {"disableagg", no_argument, 0, 'j'},
    {"enableagg", no_argument, 0, 'k'},
    {"showparams", no_argument, 0, 'l'},
    {"traderid", required_argument, 0, 'm'},
    {"disableimprove", no_argument, 0, 'n'},
    {"showindicators", no_argument, 0, 'o'},
    {"cleansumszmaps", no_argument, 0, 'p'},
    {"setopentradeloss", required_argument, 0, 'q'},
    {"agggetflat", no_argument, 0, 'r'},
    {"seteco", required_argument, 0, 's'},
    {"setmaxintspreadtoplace", required_argument, 0, 't'},
    {"setmaxintleveldifftoplace", required_argument, 0, 'u'},
    {"setglobalmaxloss", required_argument, 0, 'v'},
    {"setmaxpnl", required_argument, 0, 'w'},
    {"setshorttermglobalmaxloss", required_argument, 0, 'x'},
    {"setexplicitmaxlongpos", required_argument, 0, 'y'},
    {"setexplicitworstlongpos", required_argument, 0, 'z'},
    {"shortcode", required_argument, 0, 'A'},
    {"setmaxglobalrisk", required_argument, 0, 'B'},
    {"dumpposition", no_argument, 0, 'C'},
    {"addposition", required_argument, 0, 'D'},
    // {"enablesmarteco",        no_argument,0, 'A' },
    // {"disablesmarteco",       no_argument,0, 'B' },
    // {"enablepricemanager",    no_argument,0, 'C' },
    // {"disablepricemanager",   no_argument,0, 'D' },
    {"enablemarketmanager", no_argument, 0, 'E'},
    {"disablemarketmanager", no_argument, 0, 'F'},
    {"disableselfordercheck", no_argument, 0, 'G'},
    {"enableselfordercheck", no_argument, 0, 'H'},
    // {"dumpnonselfsmv",   no_argument,0,       'I' },
    // {"setmaxunitratio",  required_argument,0, 'J' },
    // {"setworstunitratio",required_argument,0, 'K' },
    {"forceallindicatorready", no_argument, 0, 'I'},
    {"setmaxdrawdown", required_argument, 0, 'K'},
    {"enableaggcooloff", no_argument, 0, 'L'},
    {"disableaggcooloff", no_argument, 0, 'M'},
    {"enablenonstandardcheck", no_argument, 0, 'N'},
    {"disablenonstandardcheck", no_argument, 0, 'O'},
    {"forceindicatorready", required_argument, 0, 'P'},
    {"enablelogging", no_argument, 0, 'Q'},
    {"disablelogging", no_argument, 0, 'R'},
    {"setmaxloss", required_argument, 0, 'S'},
    {"showorders", no_argument, 0, 'T'},
    {"setmaxglobalpos", required_argument, 0, 'U'},
    {"cancelandfreeze", no_argument, 0, 'V'},
    {"enablezerologgingmode", no_argument, 0, 'W'},
    {"disablezerologgingmode", no_argument, 0, 'X'},
    {"setmaxsecuritypos", required_argument, 0, 'Y'},
    {"setbreakopentradeloss", required_argument, 0, 'Z'},
    {"refreshecoevents", no_argument, 0, '9'},
    {"showecoevents", no_argument, 0, '8'},
    {"reloadafestimates", no_argument, 0, '7'},
    {"disablefreezeonrejects", no_argument, 0, '5'},
    {"enablefreezeonrejects", no_argument, 0, '6'},
    {"enableefficientsquareoff",required_argument,0,'4'},
    {"reloadspan", no_argument, 0, '2'},
    {"geteodpositions",no_argument,0,'3'},
    {"unfreezedataentryrejectdisable",no_argument,0,'1'},
    {0, 0, 0, 0}};

void print_usage(const char* prg_name) {
  printf("This is the user_msg exec \n");
  printf("Usage: %s --option <optional_val> [ --option <optional_val> ... ]\n", prg_name);
  printf("Valid options are:\n");
  printf("--help\t\t\t Displays usage_menu \n");
  printf("--getflat --traderid id\t\tSend Getflat message to id\n");
  printf("--start --traderid id\t\tSend Start message to id\n");
  // printf( "--freeze --traderid id\t\tSend Freeze message to id\n" );
  // printf( "--unfreeze --traderid id\t\tSend Unfreeze message to id\n" );
  printf("--setunitsize size --traderid id\tSend SetUnitTradeSize message to id\n");
  printf("--setmaxpos size --traderid id\tSend SetMaxPos message to id\n");
  printf("--setworstpos size --traderid id\t Send SetWorstCasePosition message to id\n");
  printf("--disableimprove --traderid id\tSend DisableImprove message to id\n");
  printf("--enableimprove --traderid id\tSend EnableImprove message to id\n");
  printf("--disableagg --traderid id\t\tSend DisableAggressive message to id\n");
  printf("--enableagg --traderid id\t\tSend EnableAggressive message to id\n");
  printf("--showparams --traderid id\t\tSend ShowParams message to id \n");
  printf("--showindicators --traderid id\t\tSend ShowIndicators message to id \n");
  printf("--cleansumszmaps --traderid id\t\tSend CleanSumSizeMaps message to id \n");
  printf("--seteco ECOVAL,END_TIME --traderid id\t\tSend SetEconomicSeverity message to id \n");
  // printf( "--enablesmarteco --traderid id\t\tSend EnableSmartEco message to id \n" );
  // printf( "--disablesmarteco --traderid id\t\tSend DisableSmartEco message to id \n" );
  // printf( "--enablepricemanager --traderid id\t\tSend EnablePriceManager message to id \n" );
  // printf( "--disablepricemanager --traderid id\t\tSend DisablePriceManager message to id \n" );
  printf("--enablemarketmanager --traderid id\t\tSend EnableMarketManager message to id \n");
  printf("--disablemarketmanager --traderid id\t\tSend DisableMarketManager message to id \n");
  printf("--disableselfordercheck --traderid id\t\tSend DisableSelfOrderCheck message to id \n");
  printf("--enableselfordercheck --traderid id\t\tSend EnableSelfOrderCheck message to id \n");
  // printf( "--dumpnonselfsmv --traderid id\t\tSend DumpNonSelfSMV message to id \n" );
  // printf( "--setmaxunitratio ratio --traderid id\tSend SetMaxUnitRatio message to id\n" );
  // printf( "--setworstunitratio ratio --traderid id\t Send SetWorstCaseUnitRatio message to id\n" );
  printf("--enableaggcooloff --traderid id\t Send EnableAggCooloff message to id\n");
  printf("--disableaggcooloff --traderid id\t Send DisableAggCooloff message to id\n");
  printf("--enablenonstandardcheck --traderid id\t Send EnableNonStandardCheck message to id\n");
  printf("--disablenonstandardcheck --traderid id\t Send DisableNonStandardCheck message to id\n");
  printf("--forceindicatorready indicatorindex --traderid id\t Send ForceIndicatorReady message to id\n");
  printf("--forceallindicatorready --traderid id\t Send ForceAllIndicatorReady message to id\n");
  printf("--enablelogging --traderid id\t Send EnableLogging message to id\n");
  printf("--disablelogging --traderid id\t Send DisableLogging message to id\n");
  printf("--setmaxloss absoluteloss --traderid id\t Send SetMaxLoss message to id\n");
  printf("--setmaxdrawdown maxabsdd --traderid id\t Send SetMaxDrawDown message to id\n");
  printf("--showorders --traderid id\t Send ShowOrders message to id\n");
  printf("--setmaxglobalpos size --traderid id\tSend SetMaxGlobalPos message to id\n");
  printf("--cancelandfreeze --traderid id\t\tSend CancelAllFreezeTrading message to id\n");
  printf("--enablezerologgingmode --traderid id\t Send Enable Zero Logging Mode to id\n");
  printf("--disablezerologgingmode --traderid id\t Send Enable Zero Logging Mode to id\n");
  printf("--setmaxsecuritypos size --traderid id\tSend SetMaxSecurityPos message to id\n");
  printf("--setstarttime  --traderid id\tSend SetStarttime in UTC message to id\n");
  printf("--setendtime  --traderid id\tSend SetEndTime in UTC message to id\n");
  printf("--setopentradeloss  --traderid id\tSend SetOpenTradeLoss message to id\n");
  printf("--agggetflat  --traderid id\tSend AggGetflat message to id\n");
  printf("--setmaxintspreadtoplace  --traderid id\tSend SetMaxIntSpreadToPlace message to id\n");
  printf("--setmaxintleveldifftoplace  --traderid id\tSend SetMaxIntLevelDiffToPlace message to id\n");
  printf("--setglobalmaxloss  --traderid id\tSend SetGlobalMaxLoss message to id\n");
  printf("--setmaxpnl  --traderid id\tSend SetMaxPnl message to id\n");
  printf("--setshorttermglobalmaxloss --traderid id\tSend SetShortTermGlobalMaxLoss message to id\n");
  printf("--setexplicitmaxlongpos size --traderid id\tSend SetExplicitMaxLongPosition message to id\n");
  printf("--setexplicitworstlongpos size --traderid id \tSend SetExplicitWorstLongPosition message to id\n");
  printf("--setbreakopentradeloss msecs --traderid id \tSend SetBreakOpenTradeLoss message to id\n");
  printf("--setmaxglobalrisk risk --traderid id \tSend SetMaxGlobalRisk message to id\n");
  printf(
      "--refreshecoevents  --traderid id\tRefresh economic events (if FXStreet web page got updated while trading)\n");
  printf("--showecoevents  --traderid id\tShow present economic events (loaded by the query)\n");
  printf("--reloadafestimates  --traderid id\tReload Estimate values for Alphaflash Query (loaded by the query)\n");
  printf("--addposition position --traderid id\t Add positions to tradeinit\n");
  printf("--disablefreezeonrejects --traderid id\t Disable rejects based auto freeze system\n");
  printf("--enablefreezeonrejects --traderid id\t Enable rejects based auto freeze system\n");
  printf("--enableefficientsquareoff --traderid id\t\tSend EnableEfficientSquareoff message to id \n");
  printf("--geteodpositions --traderid id\t\tSend EODPositions and Algos to id \n");
}

int main(int argc, char** argv) {
  int c;
  int unit_size_ = 1;
  int max_pos_ = 0;
  double max_unit_ratio_ = 0.0;
  int worst_pos_ = 0;
  int worst_unit_ratio_ = 0;
  double severity_to_getflat_on_ = 1.00;
  int seteco_end_time_ = 2359;
  int indicator_index_ = -1;
  int max_security_pos_ = -1;
  int abs_max_loss_ = 0;
  int abs_max_dd_ = 0;
  int max_global_pos_ = -1;
  int utc_start_time_hhmm_ = -1;
  int utc_end_time_hhmm_ = -1;
  int abs_open_trade_loss_ = -1;
  int max_int_spread_to_place_ = 1;
  int max_int_level_diff_to_place_ = 1;
  int global_max_loss_ = 0;
  int max_pnl_ = 0;
  int shortterm_max_loss_ = 0;
  int explicit_max_long_position_ = 0;
  int explicit_worst_case_long_position_ = 0;
  int break_msecs_on_max_opentrade_loss_ = 0;
  int max_global_risk_ = 0;
  int position_offset_ = 0;
  int trader_id_ = -1;
  double eff_sqoff_multiplier_ = 1;
  std::string shortcode_;
  std::string symbol_ = "";
  HFSAT::ControlMessageCode_t msg_code_ = HFSAT::kControlMessageCodeMax;
  std::string delimiter_ = ",";
  std::string arg_;
  std::string set_eco_number_;
  std::size_t start_idx_ = 0U;
  std::size_t end_idx_ = 0U;
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
        msg_code_ = HFSAT::kControlMessageCodeGetflat;
        break;

      case 'b':
        msg_code_ = HFSAT::kControlMessageCodeStartTrading;
        break;

      // case 'c':
      //   msg_code_ = HFSAT::kControlMessageCodeFreezeTrading;
      //   break;

      // case 'd':
      //   msg_code_ = HFSAT::kControlMessageCodeUnFreezeTrading;
      //   break;

      case 'e':
        msg_code_ = HFSAT::kControlMessageCodeSetUnitTradeSize;
        unit_size_ = atoi(optarg);
        break;

      case 'f':
        msg_code_ = HFSAT::kControlMessageCodeSetMaxPosition;
        max_pos_ = atoi(optarg);
        break;

      case 'g':
        msg_code_ = HFSAT::kControlMessageCodeSetWorstCasePosition;
        worst_pos_ = atoi(optarg);
        break;

      case 'n':
        msg_code_ = HFSAT::kControlMessageCodeDisableImprove;
        break;

      case 'i':
        msg_code_ = HFSAT::kControlMessageCodeEnableImprove;
        break;

      case 'j':
        msg_code_ = HFSAT::kControlMessageCodeDisableAggressive;
        break;

      case 'k':
        msg_code_ = HFSAT::kControlMessageCodeEnableAggressive;
        break;

      case 'l':
        msg_code_ = HFSAT::kControlMessageCodeShowParams;
        break;

      case 'm':
        trader_id_ = atoi(optarg);
        break;

      case 'o':
        msg_code_ = HFSAT::kControlMessageCodeShowIndicators;
        break;

      case 'p':
        msg_code_ = HFSAT::kControlMessageCodeCleanSumSizeMaps;
        break;

      case 's':
        msg_code_ = HFSAT::kControlMessageCodeSetEcoSeverity;
        arg_ = std::string(optarg);
        end_idx_ = arg_.find(delimiter_);
        if (end_idx_ != std::string::npos) {
          set_eco_number_ = arg_.substr(start_idx_, end_idx_);
          severity_to_getflat_on_ = atof(set_eco_number_.c_str());
          start_idx_ = end_idx_ + delimiter_.length();
          end_idx_ = arg_.find(delimiter_, start_idx_);
          set_eco_number_ = arg_.substr(start_idx_, end_idx_ - start_idx_);
          seteco_end_time_ = atoi(set_eco_number_.c_str());
        } else {
          severity_to_getflat_on_ = atof(optarg);
        }
        break;

      case 'A':
        shortcode_ = optarg;
        break;

      case 'B':
        msg_code_ = HFSAT::kControlMessageCodeSetMaxGlobalRisk;
        max_global_risk_ = atof(optarg);
        break;

      case 'C':
        msg_code_ = HFSAT::kControlMessageCodeDumpPositions;
        break;

      case 'D':
        msg_code_ = HFSAT::kControlMessageCodeAddPosition;
        position_offset_ = atoi(optarg);
        break;

      // case 'A':
      //   msg_code_ = HFSAT::kControlMessageEnableSmartEco;
      //   break;

      // case 'B':
      //   msg_code_ = HFSAT::kControlMessageDisableSmartEco;
      //   break;

      // case 'C':
      //   msg_code_ = HFSAT::kControlMessageEnablePriceManager;
      //   break;

      // case 'D':
      //   msg_code_ = HFSAT::kControlMessageDisablePriceManager;
      //   break;

      case 'E':
        msg_code_ = HFSAT::kControlMessageEnableMarketManager;
        break;

      case 'F':
        msg_code_ = HFSAT::kControlMessageDisableMarketManager;
        break;

      case 'G':
        msg_code_ = HFSAT::kControlMessageDisableSelfOrderCheck;
        break;

      case 'H':
        msg_code_ = HFSAT::kControlMessageEnableSelfOrderCheck;
        break;

      case 'I':
        msg_code_ = HFSAT::kControlMessageCodeForceAllIndicatorReady;
        break;

      // case 'I':
      //   msg_code_ = HFSAT::kControlMessageDumpNonSelfSMV;
      //   break;

      // case 'J':
      //   msg_code_ = HFSAT::kControlMessageCodeSetMaxUnitRatio;
      //   max_unit_ratio_ = atof ( optarg );
      //   break;

      // case 'K':
      //   msg_code_ = HFSAT::kControlMessageCodeSetWorstCaseUnitRatio;
      //   worst_unit_ratio_ = atoi( optarg );
      //   break;
      case 'K':
        msg_code_ = HFSAT::kControlMessageCodeSetMaxDrawDown;
        abs_max_dd_ = abs(atoi(optarg));
        break;

      case 'L':
        msg_code_ = HFSAT::kControlMessageCodeEnableAggCooloff;
        break;

      case 'M':
        msg_code_ = HFSAT::kControlMessageCodeDisableAggCooloff;
        break;

      case 'N':
        msg_code_ = HFSAT::kControlMessageCodeEnableNonStandardCheck;
        break;

      case 'O':
        msg_code_ = HFSAT::kControlMessageCodeDisableNonStandardCheck;
        break;

      case 'P':
        msg_code_ = HFSAT::kControlMessageCodeForceIndicatorReady;
        indicator_index_ = atoi(optarg);
        break;

      case 'Q':
        msg_code_ = HFSAT::kControlMessageCodeEnableLogging;
        break;

      case 'R':
        msg_code_ = HFSAT::kControlMessageCodeDisableLogging;
        break;

      case 'S':
        msg_code_ = HFSAT::kControlMessageCodeSetMaxLoss;
        abs_max_loss_ = abs(atoi(optarg));
        break;

      case 'T':
        msg_code_ = HFSAT::kControlMessageCodeShowOrders;
        break;

      case 'U':
        msg_code_ = HFSAT::kControlMessageCodeSetMaxGlobalPosition;
        max_global_pos_ = atoi(optarg);
        break;

      case 'V':
        msg_code_ = HFSAT::kControlMessageCodeCancelAllFreezeTrading;
        break;

      case 'W':
        msg_code_ = HFSAT::kControlMessageCodeEnableZeroLoggingMode;
        break;

      case 'X':
        msg_code_ = HFSAT::kControlMessageCodeDisableZeroLoggingMode;
        break;

      case 'Y':
        msg_code_ = HFSAT::kControlMessageCodeSetMaxSecurityPosition;
        max_security_pos_ = atoi(optarg);
        break;

      case 'Z':
        msg_code_ = HFSAT::kControlMessageCodeSetBreakMsecsOpenTradeLoss;
        break_msecs_on_max_opentrade_loss_ = atoi(optarg);
        break;

      case 'c':
        msg_code_ = HFSAT::kControlMessageCodeSetStartTime;
        utc_start_time_hhmm_ = atoi(optarg);
        break;

      case 'd':
        msg_code_ = HFSAT::kControlMessageCodeSetEndTime;
        utc_end_time_hhmm_ = atoi(optarg);
        break;

      case 'q':
        msg_code_ = HFSAT::kControlMessageCodeSetOpenTradeLoss;
        abs_open_trade_loss_ = abs(atoi(optarg));
        break;

      case 'r':
        msg_code_ = HFSAT::kControlMessageCodeAggGetflat;
        break;

      case 't':
        msg_code_ = HFSAT::kControlMessageCodeSetMaxIntSpreadToPlace;
        max_int_spread_to_place_ = atoi(optarg);
        break;

      case 'u':
        msg_code_ = HFSAT::kControlMessageCodeSetMaxIntLevelDiffToPlace;
        max_int_level_diff_to_place_ = atoi(optarg);
        break;

      case 'v':
        msg_code_ = HFSAT::kControlMessageCodeSetGlobalMaxLoss;
        global_max_loss_ = atoi(optarg);
        break;

      case 'w':
        msg_code_ = HFSAT::kControlMessageCodeSetMaxPnl;
        max_pnl_ = atoi(optarg);
        break;

      case 'x':
        msg_code_ = HFSAT::kControlMessageCodeSetShortTermGlobalMaxLoss;
        shortterm_max_loss_ = atoi(optarg);
        break;

      case 'y':
        msg_code_ = HFSAT::kControlMessageCodeSetExplicitMaxLongPosition;
        explicit_max_long_position_ = atoi(optarg);
        break;

      case 'z':
        msg_code_ = HFSAT::kControlMessageCodeSetExplicitWorstLongPosition;
        explicit_worst_case_long_position_ = atoi(optarg);
        break;

      case '9':
        msg_code_ = HFSAT::kControlMessageReloadEconomicEvents;
        break;

      case '8':
        msg_code_ = HFSAT::kControlMessageShowEconomicEvents;
        break;

      case '7':
        msg_code_ = HFSAT::kControlMessageReloadAfEstimates;
        break;

      case '5':
        msg_code_ = HFSAT::kControlMessageDisableFreezeOnRejects;
        break;

      case '6':
        msg_code_ = HFSAT::kControlMessageEnableFreezeOnRejects;
        break;
      case '1':
        msg_code_ = HFSAT::kControlMessageUnfreezeDataEntryReject;
        break;
      case '4':
        msg_code_ = HFSAT::kControlMessageEnableEfficientSquareoff;
        eff_sqoff_multiplier_ = atof(optarg);
        break;
      case '3':
        msg_code_ = HFSAT::kControlMessageGetEODPositions;
        break;
      case '2':
        msg_code_ = HFSAT::kControlMessageReloadSpanFile;
        break;

      case '?':
        print_usage(argv[0]);
        exit(0);
        break;
    }
  }

  /// check to see trader_id and msg_code set properly
  if (trader_id_ == -1) {
    printf("User must specify destination trader_id \n");
    exit(0);
  }

  if (msg_code_ == HFSAT::kControlMessageCodeMax) {
    printf("User must choose valid option to send to traderid \n");
    print_usage(argv[0]);
  }

  /// get broadcast ip/port info from network info manager
  HFSAT::NetworkAccountInfoManager network_account_info_manager_;
  HFSAT::DataInfo control_recv_data_info_ = network_account_info_manager_.GetControlRecvDataInfo();

  /// create broadcast socket
  HFSAT::MulticastSenderSocket sock_(
      control_recv_data_info_.bcast_ip_, control_recv_data_info_.bcast_port_,
      HFSAT::NetworkAccountInterfaceManager::instance().GetInterfaceForApp(HFSAT::k_Control));

 std::cout << " Control : " << control_recv_data_info_.bcast_ip_ << " " << control_recv_data_info_.bcast_port_ << " " << HFSAT::NetworkAccountInterfaceManager::instance().GetInterfaceForApp(HFSAT::k_Control) << std::endl;

  /// create struct; populate it and send it along
  HFSAT::GenericControlRequestStruct* gcrs_ =
      (HFSAT::GenericControlRequestStruct*)calloc(sizeof(HFSAT::GenericControlRequestStruct), 1);

  gcrs_->time_set_by_frontend_ = HFSAT::GetTimeOfDay();

  gcrs_->trader_id_ = trader_id_;

  size_t shc_cpy_length_ = sizeof((gcrs_->control_message_).strval_1_);
  if (!(shortcode_.empty()) && shc_cpy_length_ > shortcode_.size()) {
    shc_cpy_length_ = shortcode_.size();
  }

  (gcrs_->control_message_).message_code_ = msg_code_;
  if (msg_code_ == HFSAT::kControlMessageCodeGetflat) {
    bzero((gcrs_->control_message_).strval_1_, 20);
    if (!(shortcode_.empty())) {
      strncpy((gcrs_->control_message_).strval_1_, shortcode_.c_str(), shc_cpy_length_);
    }

  } else if (msg_code_ == HFSAT::kControlMessageCodeAggGetflat) {
    bzero((gcrs_->control_message_).strval_1_, 20);
    if (!(shortcode_.empty())) {
      strncpy((gcrs_->control_message_).strval_1_, shortcode_.c_str(), shc_cpy_length_);
    }
  } else if (msg_code_ == HFSAT::kControlMessageCodeStartTrading) {
    bzero((gcrs_->control_message_).strval_1_, 20);
    if (!(shortcode_.empty())) {
      strncpy((gcrs_->control_message_).strval_1_, shortcode_.c_str(), shc_cpy_length_);
    }
  } else if (msg_code_ == HFSAT::kControlMessageCodeSetUnitTradeSize) {
    (gcrs_->control_message_).intval_1_ = unit_size_;
    bzero((gcrs_->control_message_).strval_1_, 20);
    if (!(shortcode_.empty())) {
      strncpy((gcrs_->control_message_).strval_1_, shortcode_.c_str(), shc_cpy_length_);
    }
  } else if (msg_code_ == HFSAT::kControlMessageCodeSetMaxPosition) {
    (gcrs_->control_message_).intval_1_ = max_pos_;
    bzero((gcrs_->control_message_).strval_1_, 20);
    if (!(shortcode_.empty())) {
      strncpy((gcrs_->control_message_).strval_1_, shortcode_.c_str(), shc_cpy_length_);
    }

    //      strncpy ( ( gcrs_ -> control_message_ ).strval_1_, shortcode_, sizeof ( ( gcrs_ -> control_message_
    //      ).strval_1_ ) );
  } else if (msg_code_ == HFSAT::kControlMessageCodeAddPosition) {
    (gcrs_->control_message_).intval_1_ = position_offset_;
    bzero((gcrs_->control_message_).strval_1_, 20);
    if (!(shortcode_.empty())) {
      strncpy((gcrs_->control_message_).strval_1_, shortcode_.c_str(), shc_cpy_length_);
    }
  } else if (msg_code_ == HFSAT::kControlMessageCodeSetMaxUnitRatio) {
    (gcrs_->control_message_).doubleval_1_ = max_unit_ratio_;
    bzero((gcrs_->control_message_).strval_1_, 20);
    if (!(shortcode_.empty())) {
      strncpy((gcrs_->control_message_).strval_1_, shortcode_.c_str(), shc_cpy_length_);
    }

    //      strncpy ( ( gcrs_ -> control_message_ ).strval_1_, shortcode_, sizeof ( ( gcrs_ -> control_message_
    //      ).strval_1_ ) );
  } else if (msg_code_ == HFSAT::kControlMessageCodeSetWorstCasePosition) {
    (gcrs_->control_message_).intval_1_ = worst_pos_;
    bzero((gcrs_->control_message_).strval_1_, 20);
    if (!(shortcode_.empty())) {
      strncpy((gcrs_->control_message_).strval_1_, shortcode_.c_str(), shc_cpy_length_);
    }

    //      strncpy ( ( gcrs_ -> control_message_ ).strval_1_, shortcode_, sizeof ( ( gcrs_ -> control_message_
    //      ).strval_1_ ) );
  } else if (msg_code_ == HFSAT::kControlMessageCodeSetWorstCaseUnitRatio) {
    (gcrs_->control_message_).intval_1_ = worst_unit_ratio_;
    bzero((gcrs_->control_message_).strval_1_, 20);
    if (!(shortcode_.empty())) {
      strncpy((gcrs_->control_message_).strval_1_, shortcode_.c_str(), shc_cpy_length_);
    }

    //      strncpy ( ( gcrs_ -> control_message_ ).strval_1_, shortcode_, sizeof ( ( gcrs_ -> control_message_
    //      ).strval_1_ ) );
  } else if (msg_code_ == HFSAT::kControlMessageCodeSetEcoSeverity) {
    (gcrs_->control_message_).doubleval_1_ = severity_to_getflat_on_;
    (gcrs_->control_message_).intval_1_ = seteco_end_time_ * 100;  // HHMM to HHMMSS
  } else if (msg_code_ == HFSAT::kControlMessageCodeForceIndicatorReady) {
    (gcrs_->control_message_).intval_1_ = indicator_index_;
    bzero((gcrs_->control_message_).strval_1_, 20);
    if (!(shortcode_.empty())) {
      strncpy((gcrs_->control_message_).strval_1_, shortcode_.c_str(), shc_cpy_length_);
    }

    //      strncpy ( ( gcrs_ -> control_message_ ).strval_1_, shortcode_, sizeof ( ( gcrs_ -> control_message_
    //      ).strval_1_ ) );
  } else if (msg_code_ == HFSAT::kControlMessageCodeForceAllIndicatorReady) {
    bzero((gcrs_->control_message_).strval_1_, 20);
    if (!(shortcode_.empty())) {
      strncpy((gcrs_->control_message_).strval_1_, shortcode_.c_str(), shc_cpy_length_);
    }
  } else if (msg_code_ == HFSAT::kControlMessageCodeSetMaxSecurityPosition) {
    (gcrs_->control_message_).intval_1_ = max_security_pos_;
    bzero((gcrs_->control_message_).strval_1_, 20);
    if (!(shortcode_.empty())) {
      strncpy((gcrs_->control_message_).strval_1_, shortcode_.c_str(), shc_cpy_length_);
    }
  } else if (msg_code_ == HFSAT::kControlMessageCodeSetMaxLoss) {
    (gcrs_->control_message_).intval_1_ = abs_max_loss_;

    bzero((gcrs_->control_message_).strval_1_, 20);
    if (!(shortcode_.empty())) {
      strncpy((gcrs_->control_message_).strval_1_, shortcode_.c_str(), shc_cpy_length_);
    }

  } else if (msg_code_ == HFSAT::kControlMessageCodeSetMaxDrawDown) {
    (gcrs_->control_message_).intval_1_ = abs_max_dd_;

    bzero((gcrs_->control_message_).strval_1_, 20);
    if (!(shortcode_.empty())) {
      strncpy((gcrs_->control_message_).strval_1_, shortcode_.c_str(), shc_cpy_length_);
    }

  } else if (msg_code_ == HFSAT::kControlMessageCodeSetMaxGlobalPosition) {
    (gcrs_->control_message_).intval_1_ = max_global_pos_;
  } else if (msg_code_ == HFSAT::kControlMessageCodeSetMaxGlobalRisk) {
    (gcrs_->control_message_).intval_1_ = max_global_risk_;
    bzero((gcrs_->control_message_).strval_1_, 20);
    if (!(shortcode_.empty())) {
      strncpy((gcrs_->control_message_).strval_1_, shortcode_.c_str(), shc_cpy_length_);
    }

    //          strncpy ( ( gcrs_ -> control_message_ ).strval_1_, shortcode_, sizeof ( ( gcrs_ -> control_message_
    //          ).strval_1_ ) );
  } else if (msg_code_ == HFSAT::kControlMessageCodeSetStartTime) {
    (gcrs_->control_message_).intval_1_ = utc_start_time_hhmm_;
    // TODO not sure if we want to change start-end time for a particular shc in structured trading
  } else if (msg_code_ == HFSAT::kControlMessageCodeSetEndTime) {
    (gcrs_->control_message_).intval_1_ = utc_end_time_hhmm_;
  } else if (msg_code_ == HFSAT::kControlMessageCodeSetOpenTradeLoss) {
    (gcrs_->control_message_).intval_1_ = abs_open_trade_loss_;
  } else if (msg_code_ == HFSAT::kControlMessageCodeSetMaxIntSpreadToPlace) {
    (gcrs_->control_message_).intval_1_ = max_int_spread_to_place_;
    bzero((gcrs_->control_message_).strval_1_, 20);
    if (!(shortcode_.empty())) {
      strncpy((gcrs_->control_message_).strval_1_, shortcode_.c_str(), shc_cpy_length_);
    }

    //      strncpy ( ( gcrs_ -> control_message_ ).strval_1_, shortcode_, sizeof ( ( gcrs_ -> control_message_
    //      ).strval_1_ ) );
  } else if (msg_code_ == HFSAT::kControlMessageCodeSetMaxIntLevelDiffToPlace) {
    (gcrs_->control_message_).intval_1_ = max_int_level_diff_to_place_;
    bzero((gcrs_->control_message_).strval_1_, 20);
    if (!(shortcode_.empty())) {
      strncpy((gcrs_->control_message_).strval_1_, shortcode_.c_str(), shc_cpy_length_);
    }

    //      strncpy ( ( gcrs_ -> control_message_ ).strval_1_, shortcode_, sizeof ( ( gcrs_ -> control_message_
    //      ).strval_1_ ) );
  } else if (msg_code_ == HFSAT::kControlMessageCodeSetGlobalMaxLoss) {
    (gcrs_->control_message_).intval_1_ = global_max_loss_;
  } else if (msg_code_ == HFSAT::kControlMessageCodeSetMaxPnl) {
    (gcrs_->control_message_).intval_1_ = max_pnl_;
  } else if (msg_code_ == HFSAT::kControlMessageCodeSetShortTermGlobalMaxLoss) {
    (gcrs_->control_message_).intval_1_ = shortterm_max_loss_;
  } else if (msg_code_ == HFSAT::kControlMessageCodeSetExplicitMaxLongPosition) {
    (gcrs_->control_message_).intval_1_ = explicit_max_long_position_;
    bzero((gcrs_->control_message_).strval_1_, 20);
    if (!(shortcode_.empty())) {
      strncpy((gcrs_->control_message_).strval_1_, shortcode_.c_str(), shc_cpy_length_);
    }

  } else if (msg_code_ == HFSAT::kControlMessageCodeSetExplicitWorstLongPosition) {
    (gcrs_->control_message_).intval_1_ = explicit_worst_case_long_position_;
    bzero((gcrs_->control_message_).strval_1_, 20);
    if (!(shortcode_.empty())) {
      strncpy((gcrs_->control_message_).strval_1_, shortcode_.c_str(), shc_cpy_length_);
    }

  } else if (msg_code_ == HFSAT::kControlMessageCodeSetBreakMsecsOpenTradeLoss) {
    (gcrs_->control_message_).intval_1_ = break_msecs_on_max_opentrade_loss_;
  } else if (msg_code_ == HFSAT::kControlMessageEnableEfficientSquareoff) {
    sprintf((gcrs_->control_message_).strval_1_,"%2.5f",eff_sqoff_multiplier_);
    //(gcrs_->control_message_).doubleval_1_ = eff_sqoff_multiplier_;
    //std::cout << "Eff sqoff values " << (gcrs_->control_message_).strval_1_ << " " <<eff_sqoff_multiplier_ << std::endl;
  } else if (msg_code_ == HFSAT::kControlMessageReloadSpanFile) {
    std::cout << "kControlMessageReloadSpanFile\n";
    // bzero((gcrs_->control_message_).strval_1_, 20);
    // if (!(shortcode_.empty())) {
    //   strncpy((gcrs_->control_message_).strval_1_, shortcode_.c_str(), shc_cpy_length_);
    // }
  }

  sock_.WriteN(sizeof(HFSAT::GenericControlRequestStruct), gcrs_);
}
