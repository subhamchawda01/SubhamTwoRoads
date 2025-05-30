#include <iostream>

#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/Utils/common_files_path.hpp"
#include "infracore/BasicOrderRoutingServer/control_command.hpp"
#include "infracore/BasicOrderRoutingServer/control_thread.hpp"
#include "dvccode/Utils/exchange_names.hpp"

#define NSE_SECURITIES_UNDER_BAN_FILE_PATH "/spare/local/tradeinfo/NSE_Files/SecuritiesUnderBan/"
#define NSE_SECURITIES_WITH_PHYSICAL_SETTLEMENT "/spare/local/tradeinfo/NSE_Files/SecuritiesUnderPhysicalSettlement/"
#define NSE_SECURITIES_UNDER_UNSOLICITED_SMS "/spare/local/tradeinfo/NSE_Files/SecuritiesUnderUnsolicitedSMS/"
#define EQUALITY_TOLERANCE 0.0001

namespace HFSAT {
namespace ORS {

ControlThread::ControlThread(DebugLogger& _dbglogger_, const int _connected_socket_file_descriptor_,
                             Settings& _settings_,
                             HFSAT::Utils::ClientLoggingSegmentInitializer* _client_logging_segment_initializer_,
                             std::string t_output_log_dir_, std::map<std::string, int>& _sec_to_max_pos_map_,
                             std::map<std::string, int>& _sec_to_max_ord_sz_map_,
                             HFSAT::ORS::ControlReceiver* ors_control_receiver, int32_t tid)
    : dbglogger_(_dbglogger_),
      simple_security_symbol_indexer_(HFSAT::SimpleSecuritySymbolIndexer::GetUniqueInstance()),
      margin_checker_(
          MarginChecker::GetUniqueInstance(_dbglogger_, HFSAT::StringToExchSource(_settings_.getValue("Exchange")))),
      connected_socket_file_descriptor_(_connected_socket_file_descriptor_),
      m_athread(AccountManager::GetNewAccountThread(_dbglogger_, OrderManager::GetUniqueInstance(), _settings_,
                                                    _client_logging_segment_initializer_, t_output_log_dir_)),
      m_settings(_settings_),
      client_logging_segment_initializer_(_client_logging_segment_initializer_),
      sec_to_max_pos_map_(_sec_to_max_pos_map_),
      sec_to_max_ord_sz_map_(_sec_to_max_ord_sz_map_),
      shc_to_mifid_limits_(),
      keep_control_threads_running_(1),
      position_manager_(PositionManager::GetUniqueInstance()),
      order_manager_(OrderManager::GetUniqueInstance()),
      eti_algo_tagger_(HFSAT::Utils::ETIAlgoTagging::GetUniqueInstance(_settings_.getValue("Exchange"))),
      control_receiver_(ors_control_receiver),
      list_of_securities_under_ban_(),
      list_of_securities_under_physical_settlement_(),
      list_of_securities_with_unsolicited_sms_(),
      exchange_(m_settings.getValue("Exchange")),
      thread_id(tid),
      is_mifid_check_applicable_(HFSAT::IsMiFidCheckApplicable(HFSAT::StringToExchSource(exchange_))) {
  if (is_mifid_check_applicable_) {
    LoadMifidLimitsFile();
  }
  LoadNSEFOSecuritiesUnderBan();
  LoadNSEFOSecuritiesUnderPhysicalSettlement();
  LoadNSESecuritiesUnderUnsolicitedSMSList();
}

bool ControlThread::banned_products_listed_ = false;
bool ControlThread::physical_settlement_products_listed_ = false;
bool ControlThread::unsolicited_sms_products_listed_ = false;

void ControlThread::LoadNSEFOSecuritiesUnderBan() {
  list_of_securities_under_ban_.clear();

  if ("NSE_FO" == exchange_) {
    std::ostringstream t_temp_oss;
    t_temp_oss << NSE_SECURITIES_UNDER_BAN_FILE_PATH << "fo_secban_" << HFSAT::DateTime::GetCurrentIsoDateLocal()
               << ".csv";

    std::ifstream fo_banned_securities_stream;
    fo_banned_securities_stream.open(t_temp_oss.str().c_str(), std::ifstream::in);

#define MAX_LINE_READ_LENGTH 1024
    char line_buffer[MAX_LINE_READ_LENGTH];

    while (fo_banned_securities_stream.good()) {
      fo_banned_securities_stream.getline(line_buffer, MAX_LINE_READ_LENGTH);
      if (std::string(line_buffer).length() < 1) continue;
      list_of_securities_under_ban_.insert(line_buffer);

      if (false == banned_products_listed_) {
        DBGLOG_CLASS_FUNC_LINE_INFO << "SECURITY : " << line_buffer << " IS UNDER BAN FOR TODAY" << DBGLOG_ENDL_FLUSH;
        DBGLOG_DUMP;
      }
    }

    fo_banned_securities_stream.close();

#undef MAX_LINE_READ_LENGTH
  }

  banned_products_listed_ = true;
}

void ControlThread::LoadNSEFOSecuritiesUnderPhysicalSettlement() {
  list_of_securities_under_physical_settlement_.clear();

  if ("NSE_FO" == exchange_){
    std::ostringstream t_temp_oss;
    t_temp_oss << NSE_SECURITIES_WITH_PHYSICAL_SETTLEMENT << "fo_securities_under_physical_settlement.csv";

    std::ifstream fo_ps_securities_stream;
    fo_ps_securities_stream.open(t_temp_oss.str().c_str(), std::ifstream::in);

    if( !fo_ps_securities_stream.is_open() ){
      DBGLOG_CLASS_FUNC_LINE_FATAL << "CAN'T LOCATE PHYSICAL SETTLEMENT CONTRACT FILE -> " << t_temp_oss.str() << DBGLOG_ENDL_FLUSH;
      std::exit(-1);
    }

#define MAX_LINE_READ_LENGTH 1024
    char line_buffer[MAX_LINE_READ_LENGTH];

    while (fo_ps_securities_stream.good()) {
      fo_ps_securities_stream.getline(line_buffer, MAX_LINE_READ_LENGTH);
      if (std::string(line_buffer).length() < 1) continue;

      //Only FUT0 under physical settlement
      std::string this_shortcode = std::string(line_buffer) ;
      list_of_securities_under_physical_settlement_.insert(this_shortcode);

      if (false == physical_settlement_products_listed_ ) {
        DBGLOG_CLASS_FUNC_LINE_INFO << "SECURITY : " << this_shortcode << " IS UNDER PHYSICAL SETTLEMENT FOR THIS EXPIRY " << DBGLOG_ENDL_FLUSH;
        DBGLOG_DUMP;
      }
    }

    fo_ps_securities_stream.close();

#undef MAX_LINE_READ_LENGTH
  }

  physical_settlement_products_listed_ = true;
}

void ControlThread::LoadNSESecuritiesUnderUnsolicitedSMSList() {
  list_of_securities_with_unsolicited_sms_.clear();

  if ("NSE_FO" == exchange_ || "NSE_EQ" == exchange_) {
    std::ostringstream t_temp_oss;
    t_temp_oss << NSE_SECURITIES_UNDER_UNSOLICITED_SMS << "nse_sec_unsolicited_sms_.csv";

    std::ifstream banned_securities_stream;
    banned_securities_stream.open(t_temp_oss.str().c_str(), std::ifstream::in);

#define MAX_LINE_READ_LENGTH 1024
    char line_buffer[MAX_LINE_READ_LENGTH];

    while (banned_securities_stream.good()) {
      banned_securities_stream.getline(line_buffer, MAX_LINE_READ_LENGTH);
      if (std::string(line_buffer).length() < 1) continue;
      list_of_securities_with_unsolicited_sms_.insert(line_buffer);

      if (false == unsolicited_sms_products_listed_) {
        DBGLOG_CLASS_FUNC_LINE_INFO << "SECURITY : " << line_buffer << " IS UNDER BAN FOR TODAY" << DBGLOG_ENDL_FLUSH;
        DBGLOG_DUMP;
      }
    }

    banned_securities_stream.close();

#undef MAX_LINE_READ_LENGTH
  }

  unsolicited_sms_products_listed_ = true;
}


void ControlThread::LoadMifidLimitsFile() {
  std::ifstream mifid_file;

  std::ostringstream t_temp_oss;
  t_temp_oss << PROD_CONFIGS_DIR << GetCurrentHostName() << "_mifid_limits.cfg";
  std::string file_path = t_temp_oss.str();

  mifid_file.open(file_path.c_str(), std::ifstream::in);
  if (!mifid_file.is_open()) {
    dbglogger_ << "Cannot open Mifid limits file : " << file_path << ". No addts command would work without this. \n";
    dbglogger_.DumpCurrentBuffer();
    return;
  }

  char line[1024];
  while (!mifid_file.eof()) {
    bzero(line, 1024);
    mifid_file.getline(line, 1024);
    if (strlen(line) == 0 || strstr(line, "#") != NULL) continue;
    HFSAT::PerishableStringTokenizer st_(line, 1024);
    const std::vector<const char*>& tokens_ = st_.GetTokens();
    if (tokens_.size() != 6) {
      dbglogger_ << "Ignoring malformatted line " << line << "\n";
      dbglogger_.DumpCurrentBuffer();
      continue;
    }
    std::string shc = tokens_[0];

    if (!HFSAT::SecurityDefinitions::CheckIfContractSpecExists(shc, HFSAT::DateTime::GetCurrentIsoDateLocal())) {
      dbglogger_ << "SHC in input mifid file doesn't exist in security definitions : " << shc << "\n";
      dbglogger_.DumpCurrentBuffer();
      continue;
    }

    MiFidLimits limit;
    limit.otr_min_vol = atoi(tokens_[1]);
    limit.otr_min_cnt = atoi(tokens_[2]);
    limit.otr_limit_vol = atoi(tokens_[3]);
    limit.otr_limit_cnt = atoi(tokens_[4]);
    limit.msg_count = atoi(tokens_[5]);

    shc_to_mifid_limits_[shc] = limit;
  }
  mifid_file.close();
}

void ControlThread::sendBaseId() {
  int server_assigned_client_id_base_ = -1;
  if (m_settings.has("Client_Base")) server_assigned_client_id_base_ = atoi(m_settings.getValue("Client_Base").c_str());

  if (margin_server_fd_ != -1) {
    int num_written =
        write(margin_server_fd_, &server_assigned_client_id_base_, sizeof(server_assigned_client_id_base_));
    if (num_written != sizeof(server_assigned_client_id_base_)) {
      dbglogger_ << "Error Sending to margin_server_fd_ ( " << margin_server_fd_
                 << " ) server_assigned_client_id_base_ " << server_assigned_client_id_base_;
      dbglogger_.CheckToFlushBuffer();
    } else {
      dbglogger_ << "Sent to margin_server_fd_ ( " << margin_server_fd_ << " ) server_assigned_client_id_base_ "
                 << server_assigned_client_id_base_;
      dbglogger_.CheckToFlushBuffer();
    }
  }
}

void ControlThread::Close() {
  if (connected_socket_file_descriptor_ != -1) {
    shutdown(connected_socket_file_descriptor_, SHUT_RDWR);
    close(connected_socket_file_descriptor_);
    connected_socket_file_descriptor_ = -1;
  }
  control_receiver_->RemoveThread(thread_id);
}

void ControlThread::StopControlThread() { __sync_bool_compare_and_swap(&keep_control_threads_running_, 1, 0); }

void ControlThread::thread_main()  // Short lived thread no affinity needed.
{
#define COMMAND_BUFFER_LEN 1024
  char command_buffer_[COMMAND_BUFFER_LEN];
  char temp_command_buffer_[COMMAND_BUFFER_LEN];

  // select related vars
  fd_set rfd;
  int maxfdplus1 = connected_socket_file_descriptor_ + 1;

  while (keep_control_threads_running_) {
    bzero(command_buffer_, COMMAND_BUFFER_LEN);
    FD_ZERO(&rfd);
    FD_SET(maxfdplus1 - 1, &rfd);

    struct timespec timeout_pselect_;
    timeout_pselect_.tv_sec = 2;
    timeout_pselect_.tv_nsec = 0;
    int retval = pselect(maxfdplus1, &rfd, NULL, NULL, &timeout_pselect_, NULL);
    if (retval == 0) {
      continue;
    }
    if (retval == -1) {
      if (dbglogger_.CheckLoggingLevel(ORS_ERROR)) {
        dbglogger_ << " Control Client disconnects .. returning.\n ";
        dbglogger_.CheckToFlushBuffer();
      }
      // Error at pselect (network issue) => close socket and return
      StopControlThread();
      break;
    }

    int read_len_ = read(connected_socket_file_descriptor_, command_buffer_, COMMAND_BUFFER_LEN);

    // Lack of this check can cause some serious CPU hogging, when
    // the other end of the TCP connection is snapped shut by ors_control_exec.
    if (read_len_ <= 0) {
      // Almost always means that the other end has closed the connection.
      // Exit gracefully.

      if (dbglogger_.CheckLoggingLevel(ORS_ERROR)) {
        // dbglogger_ << " Control Client disconnects .. returning.\n ";
        // dbglogger_.CheckToFlushBuffer ( );
      }
      // Read returned no data/error (due to socket close) => close socket and return
      StopControlThread();
      break;
    }

    if (command_buffer_[0] != '\0') {
      dbglogger_.snprintf(COMMAND_BUFFER_LEN, command_buffer_);
      dbglogger_ << " : ThreadID: " << thread_id;
      dbglogger_ << '\n';
      dbglogger_.CheckToFlushBuffer();
    }

    strcpy(temp_command_buffer_, command_buffer_);
    PerishableStringTokenizer st_(command_buffer_, COMMAND_BUFFER_LEN);
    const std::vector<const char*>& tokens_ = st_.GetTokens();
    // expect lines of the form :
    // "CONTROLCOMMAND" "START"
    // "CONTROLCOMMAND" "LOGIN"
    if ((tokens_.size() >= 2) && (strcmp(tokens_[0], "CONTROLCOMMAND") == 0)) {
      ControlCommand_t _this_control_command_ = StringToControlCommand(tokens_[1]);
      switch (_this_control_command_) {
        case kGeneral: {
          DBGLOG_CLASS_FUNC_LINE << " General Control Command : " << temp_command_buffer_ << DBGLOG_ENDL_FLUSH;
          m_athread->ProcessGeneralControlCommand(temp_command_buffer_, COMMAND_BUFFER_LEN);
          break;
        }
        case kAdjustSymbol: {
          m_athread->AdjustSymbol(string(tokens_[2]).c_str());
          break;
        }
        case kChangeSpreadRatio: {
          /// CONTROLCOMMAND CHANGESPREADRATIO SPREAD RATIO
          if (tokens_.size() != 4) {
            dbglogger_ << "Improper Syntax. Expecting  CONTROLCOMMAND CHANGESPREADRATIO SPREAD RATIO"
                       << DBGLOG_ENDL_FLUSH;
            break;
          }
          m_athread->ChangeSpreadRatio(string(tokens_[2]).c_str(), string(tokens_[3]).c_str());
          break;
        }
        case kReloadMarginFile: {
          LoadNSEFOSecuritiesUnderBan();
          LoadNSEFOSecuritiesUnderPhysicalSettlement();
          if (control_receiver_ != NULL) {
            control_receiver_->LoadMarginFile();
          } else {
            dbglogger_ << "NULL control_receiver in control_thread. Not refreshing margin file\n";
            dbglogger_.CheckToFlushBuffer();
          }
          break;
        }
        case kReloadRolloverFile: {
          /// CONTROLCOMMAND RELOADROLLOVERFILE SHORTCODE
          if (tokens_.size() != 3) {
            dbglogger_ << "Improper Syntax. Expecting CONTROLCOMMAND RELOADROLLOVERFILE <SHORTCODE>"
                       << DBGLOG_ENDL_FLUSH;
            break;
          }
          ExchangeSymbolManager::ReloadRolloverFile(tokens_[2]);
          break;
        }
        case kControlStop: {
          m_athread->DisConnect();
        } break;

        case kControlStart: {
          m_athread->Connect();
        } break;
        case kControlStartWithoutPlayback: {
          m_athread->Connect();
          m_athread->getEngine()->SetPlaybackMode(false);
        } break;

        case kControlLogin: {
          m_athread->Login();
        } break;

        case kControlLogout: {
          m_athread->Logout();
        } break;

        case kAddTradingSymbol: {
          /// CONTROLCOMMAND ADDTRADINGSYMBOL SHORTCODE MAXPOS MAXORDERSIZE MAXWORSTCASEPOS MAXLIVEORDERS
          if (tokens_.size() < 7) {
            dbglogger_ << "Improper Syntax. Expecting CONTROLCOMMAND ADDTRADINGSYMBOL <SHORTCODE> <MAXPOS> "
                          "<MAXORDERSIZE> <MAXWORSTCASEPOS> <MAXLIVEORDERS>" << DBGLOG_ENDL_FLUSH;
            break;
          }

          std::string shortcode_temp(tokens_[2]);
          int max_pos = std::atoi(tokens_[3]);
          int max_order_size = std::atoi(tokens_[4]);

          // This is to support cases where we are dealing with orders in actual size, i.e NSE
          int32_t lotsize_factor = 1;

          if (!acceptableMaxPosParams1(shortcode_temp, max_pos, max_order_size)) {
            break;
          }

          if (false ==
              HFSAT::ORSUtils::ORSMarginManager::GetUniqueInstance(dbglogger_).AllowAddTSForShortCode(shortcode_temp)) {
            DBGLOG_CLASS_FUNC_LINE_ERROR << "MARGIN CHECKS ENABLED ON ORS BUT MARGIN NOT PROVIDED FOR SHORTCODE : "
                                         << shortcode_temp << DBGLOG_ENDL_DUMP;
            break;
          }

          if ("NSE_FO" == exchange_) {
            std::string shortcode = tokens_[2];
            std::string basename = shortcode.substr(shortcode.find_first_of("_") + 1);
            basename = basename.substr(0, basename.find_first_of("_"));

            if (list_of_securities_under_ban_.end() != list_of_securities_under_ban_.find(basename)) {
              DBGLOG_CLASS_FUNC_LINE_ERROR << "SHORTCODE : " << shortcode << " BASENAME : " << basename
                                           << " CANT BE TRADED TODAY AS IT IS UNDER BAN" << DBGLOG_ENDL_FLUSH;
              DBGLOG_DUMP;
              break;
            }
	    bool is_nearest_expiry_contract = ((shortcode.find("_FUT0") != std::string::npos) ||
					       (shortcode.find("_C0") != std::string::npos) ||
                                               (shortcode.find("_P0") != std::string::npos));
            //Physical settlement, Check if expiry date and contract under physical settlement cycle
            if(is_nearest_expiry_contract && HFSAT::NSESecurityDefinitions::GetExpiryFromShortCode(shortcode) == HFSAT::DateTime::GetCurrentIsoDateLocal()){
              if( list_of_securities_under_physical_settlement_.end() != list_of_securities_under_physical_settlement_.find(basename)){
                DBGLOG_CLASS_FUNC_LINE_ERROR << "SHORTCODE : " << shortcode << " BASENAME : " << basename
                                             << " CANT BE TRADED TODAY AS IT IS UNDER PHYSICAL SETTLEMENT" << DBGLOG_ENDL_FLUSH;
                DBGLOG_DUMP;
                break;
              }
            }
          }

          //Unsolicited SMS scripts
          if( "NSE_EQ" == exchange_ || "NSE_FO" == exchange_ ){

            std::string shortcode = tokens_[2];
            std::string basename = shortcode.substr(shortcode.find_first_of("_") + 1);
            basename = basename.substr(0, basename.find_first_of("_"));

            if( list_of_securities_with_unsolicited_sms_.end() != list_of_securities_with_unsolicited_sms_.find(shortcode) ){
              DBGLOG_CLASS_FUNC_LINE_ERROR << "SHORTCODE : " << shortcode << " BASENAME : " << basename
                                           << " CANT BE TRADED TODAY AS IT IS UNDER UNSOLICITED SMS CIRCULATION " << DBGLOG_ENDL_FLUSH;
              DBGLOG_DUMP;
              break;
            }

          }

          bool is_shortcode_present = HFSAT::SecurityDefinitions::CheckIfContractSpecExists(
              shortcode_temp, HFSAT::DateTime::GetCurrentIsoDateLocal());

          if (!is_shortcode_present) {
            DBGLOG_CLASS_FUNC_LINE_ERROR << "Shortcode in ADDTRADINGSYMBOL : " << shortcode_temp
                                         << " not present in ContractDefinition. Returning.." << DBGLOG_ENDL_FLUSH;
            break;
          }

          const char* this_exch_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(tokens_[2]);

          // Speacial Handling For NSE
          std::string exch_symbol_str = this_exch_symbol_;
          if (strncmp(exch_symbol_str.substr(0, 3).c_str(), "NSE", 3) == 0) {
            HFSAT::NSESecurityDefinitions& nse_sec_def =
                HFSAT::NSESecurityDefinitions::GetUniqueInstance(HFSAT::DateTime::GetCurrentIsoDateLocal());

            // Spreads Handling
            if (nse_sec_def.IsSpreadShortcode(shortcode_temp)) {
              std::string spread_data_source_symbol =
                  nse_sec_def.ConvertExchSymboltoDataSourceName(exch_symbol_str.c_str());

              if (std::string("INVALID") == spread_data_source_symbol) {
                DBGLOG_CLASS_FUNC_LINE_FATAL
                    << "FAILED TO ADD SYMBOL STRING AS SPREAD DATASYMBOL WAS INVALID, SYMBOL : " << exch_symbol_str
                    << DBGLOG_ENDL_FLUSH;
                DBGLOG_DUMP;

                break;
              }

              // NSE_NIFTY_FUT_20150129_20150226
              int pos = spread_data_source_symbol.rfind('_');

              std::string data_symbol_1 = spread_data_source_symbol.substr(0, pos);
              int basename_pos = data_symbol_1.rfind('_');
              std::string data_symbol_2 =
                  data_symbol_1.substr(0, basename_pos + 1) + spread_data_source_symbol.substr(pos + 1, 8);

              std::string exch_symbol_1 = nse_sec_def.ConvertDataSourceNametoExchSymbol(data_symbol_1.c_str());
              std::string exch_symbol_2 = nse_sec_def.ConvertDataSourceNametoExchSymbol(data_symbol_2.c_str());

              if (std::string("INVALID") == exch_symbol_1 || std::string("INVALID") == exch_symbol_2) {
                DBGLOG_CLASS_FUNC_LINE_FATAL << "FAILED TO ADD SYMBOL STRING AS EXCHSYMBOL FOR ONE OF THE TWO/BOTH "
                                                "INTERNAL SYMBOL WAS INVALID, SYMBOL : " << exch_symbol_1
                                             << " DATA SYMBOL : " << data_symbol_1 << " SYMBOL2 : " << exch_symbol_2
                                             << " DATA SYMBOL : " << data_symbol_2;
                DBGLOG_DUMP;

                break;
              }

            } else {  // Outrights

              std::string data_symbol = nse_sec_def.ConvertExchSymboltoDataSourceName(exch_symbol_str);

              if (std::string("INVALID") == data_symbol) {
                DBGLOG_CLASS_FUNC_LINE_FATAL
                    << "FAILED TO ADD SYMBOL STRING AS DATASYMBOL WAS INVALID, SYMBOL : " << exch_symbol_str
                    << DBGLOG_ENDL_FLUSH;
                DBGLOG_DUMP;
                exit(-1);
                break;
              }
            }

            lotsize_factor = HFSAT::SecurityDefinitions::GetContractMinOrderSize(
                tokens_[2], HFSAT::DateTime::GetCurrentIsoDateLocal());
          }

          int t_security_id_ = simple_security_symbol_indexer_.GetIdFromSecname(this_exch_symbol_);
          if (t_security_id_ < 0) {
            dbglogger_ << "Calling AddString With RiskParams As, MaxPosition : " << max_pos
                       << " MaxOrderSize : " << max_order_size << "\n";
            dbglogger_.DumpCurrentBuffer();

            if (simple_security_symbol_indexer_.GetNumSecurityId() < DEF_MAX_SEC_ID) {
              simple_security_symbol_indexer_.AddString(this_exch_symbol_);
              t_security_id_ = simple_security_symbol_indexer_.GetIdFromSecname(this_exch_symbol_);

              if (t_security_id_ < 0) {
                dbglogger_ << " Failed to set limits \n";  // nothing to do here, next check will take care of this.
              }
            } else {
              dbglogger_ << "Not adding symbol : " << this_exch_symbol_
                         << ". NumSecurities exceeding DEF_MAX_SEC_ID : " << DEF_MAX_SEC_ID << DBGLOG_ENDL_FLUSH;
            }
          }

          if (t_security_id_ < 0) {
            dbglogger_ << "AddString has failed for " << tokens_[2] << " or exchsymbol "
                       << HFSAT::ExchangeSymbolManager::GetExchSymbol(tokens_[2]) << " that has not been added "
                       << DBGLOG_ENDL_FLUSH;
            break;
          }

          else {
            if (!AddMifidLimits(tokens_[2], t_security_id_)) break;

            SidToShortCodeMap()[t_security_id_] = tokens_[2];
            HFSAT::ORSUtils::SecurityDBManager::GetUniqueInstance().SetSecurityShortcode(t_security_id_, tokens_[2]);
          }

          // We make sure t_security_id_ must be >=0 else margin_checker will segfualt trying to set negative
          // index for array
          margin_checker_.Set(t_security_id_, max_pos * lotsize_factor, max_order_size * lotsize_factor,
                              max_pos * lotsize_factor, 0);

        } break;

        case kControlDisableNewOrders: {
          dbglogger_ << "CONTROLCOMMAND DisableNewOrders" << DBGLOG_ENDL_FLUSH;
          m_athread->getEngine()->disableNewOrders();
        } break;

        case kControlEnableNewOrders: {
          dbglogger_ << "CONTROLCOMMAND EnableNewOrders" << DBGLOG_ENDL_FLUSH;
          m_athread->getEngine()->enableNewOrders();
        } break;

        case kUpdateSettings: {
          /// syntax CONTROLCOMMAND UPDATESETTINGS KEY VALUE
          if (tokens_.size() != 4) {
            dbglogger_ << "Improper Syntax for CONTROLCOMMAND UPDATESETTINGS \n";
            dbglogger_.CheckToFlushBuffer();
            break;
          }
          m_settings.setValue(tokens_[2], tokens_[3]);
        } break;
#if USING_STC
        case kDisableSelfTradeCheck: {
          /// syntax CONTROLCOMMAND DISABLESELFTRADECHECK
          if (tokens_.size() != 2) {
            dbglogger_ << "Improper Syntax for CONTROLCOMMAND DISABLESELFTRADECHECK \n";
            dbglogger_.CheckToFlushBuffer();
            break;
          }
          m_athread->DisableSelfTradeCheck();

        } break;

        case kEnableSelfTradeCheck: {
          /// syntax CONTROLCOMMAND ENABLESELFTRADECHECK
          if (tokens_.size() != 2) {
            dbglogger_ << "Improper Syntax for CONTROLCOMMAND ENABLESELFTRADECHECK \n";
            dbglogger_.CheckToFlushBuffer();
            break;
          }
          m_athread->EnableSelfTradeCheck();

        } break;
#endif
        case kSendOrder: {
          /// syntax CONTROLCOMMAND SENDORDER SYMBOL SEQNUM SIDE SIZE PX
          if (tokens_.size() != 7) {
            dbglogger_ << "Improper Syntax for CONTROLCOMMAND SENDORDER \n";
            dbglogger_.CheckToFlushBuffer();
            break;
          }
          Order ord;
          memset(&ord, 0, sizeof(ord));
          strncpy(ord.symbol_, tokens_[2], kSecNameLen);
          ord.server_assigned_order_sequence_ = atoi(tokens_[3]);
          ord.buysell_ = (tokens_[4][0] == 'B' ? kTradeTypeBuy : kTradeTypeSell);
          ord.size_remaining_ = atoi(tokens_[5]);
          ord.price_ = atof(tokens_[6]);

          const int _security_id_ = simple_security_symbol_indexer_.GetIdFromChar16(ord.symbol_);

          // Need to set the security id for the order -- .
          dbglogger_ << "\n\n\tSymbol : " << ord.symbol_ << " has Security ID: " << _security_id_ << "\n";
          dbglogger_.DumpCurrentBuffer();
          ord.security_id_ = _security_id_;

          HFSAT::ORS::Order* my_order = new HFSAT::ORS::Order();  // TODO_OPT should be using MemPool
          memcpy((void*)my_order, (void*)(&ord), sizeof(HFSAT::ORS::Order));

          m_athread->SendTrade(my_order);
        } break;
        // #ifdef ONLY_FOR_AUTOCERTPLUS
        //                 case kSendTIFOrder:
        //                   {
        // 	 	    ///syntax CONTROLCOMMAND SENDTIFORDER SYMBOL SEQNUM SIDE SIZE PX TIF
        //                     if ( tokens_.size( ) != 8 ) {
        //		      dbglogger_ << "Improper Syntax for CONTROLCOMMAND SENDTIFORDER \n";
        //		      dbglogger_.CheckToFlushBuffer ( );
        //                       break;
        //                     }
        //                     Order ord;
        //                     memset( &ord, 0, sizeof( ord ) );
        // 		    strncpy( ord.symbol_, tokens_[ 2 ], 5 );
        //                     ord.server_assigned_order_sequence_ = atoi( tokens_[ 3 ] );
        //                     ord.buysell_ = ( tokens_[ 4 ][ 0 ] == 'B'? kTradeTypeBuy : kTradeTypeSell );
        //                     ord.size_remaining_ = atoi( tokens_[ 5 ] );
        //                     ord.price_ = atof( tokens_[ 6 ] );
        // 		    std::string tif_str_ = tokens_[7] ; // can be DAY / GTC / GTD / FAK
        // 		    if ( tif_str_.compare ( "DAY" ) == 0 )
        // 		      {
        // 			m_athread -> SendTrade( &ord );
        // 		      }
        // 		    else
        // 		      {
        // 			m_athread -> SendTIFTrade ( &ord, tif_str_ );
        // 		      }
        //                   }
        //                   break;
        // #endif
        case kCancelOrder: {
          /// syntax CONTROLCOMMAND CANCELORDER ORD_ID
          if (tokens_.size() != 3) {
            dbglogger_ << "Improper Syntax for CONTROLCOMMAND TMXCANCELORDER \n";
            dbglogger_.CheckToFlushBuffer();
            break;
          }
          m_athread->Cancel(atoi(tokens_[2]));
        } break;
        case kModifyOrder: {
          /// syntax CONTROLCOMMAND MODIFYORDER SEQNO SYMBOL SIDE SIZE PX
          if (tokens_.size() != 7) {
            dbglogger_ << "Improper Syntax for CONTROLCOMMAND TMXMODIFYORDER \n";
            dbglogger_.CheckToFlushBuffer();
            break;
          }

          dbglogger_ << "\n\nkModifyOrder IN control_thread.cpp\n\n";
          dbglogger_.CheckToFlushBuffer();

          Order* p_this_order_ = OrderManager::GetUniqueInstance().GetOrderByOrderSequence(atoi(tokens_[2]));

          Order ord;
          bzero(&ord, sizeof(ord));

          memcpy(
              ord.exch_assigned_order_sequence_, p_this_order_->exch_assigned_order_sequence_,
              EXCH_ASSIGNED_ORDER_SEQUENCE_LEN);  // since without this we were sending 37=NULL after removing ordIdmap

          ord.server_assigned_order_sequence_ = atoi(tokens_[2]);
          strncpy(ord.symbol_, tokens_[3], kSecNameLen);
          ord.buysell_ = (tokens_[4][0] == 'B' ? kTradeTypeBuy : kTradeTypeSell);
          ord.size_remaining_ = atoi(tokens_[5]);
          ord.price_ = atof(tokens_[6]);
          ord.security_id_ = p_this_order_->security_id_;  // Else it might be ZERO by default and will take different
                                                           // preset CancelReplace Order structure from map &hence wrong
                                                           // symbol, etc
          m_athread->CxlReplace(&ord);
        } break;

        case kAddLoggingLevel: {
          // syntax CONTROLCOMMAND ADDLOGGINGLEVEL LEVELNAME
          if (tokens_.size() != 3) {
            dbglogger_ << "Improper Syntax for CONTROLCOMMAND ADDLOGGINGLEVEL LEVELNAME/LEVELNUMBER \n";
            dbglogger_.CheckToFlushBuffer();
            break;
          }

          int log_level_ = DebugLogger::TextToLogLevel(tokens_[2]);
          dbglogger_.AddLogLevel(log_level_);
        } break;

        case kSetSenderId: {
          // syntax CONTROLCOMMAND SETSENDERID SENDER_ID
          if (tokens_.size() != 3) {
            dbglogger_ << "Improper Syntax for CONTROLCOMMAND SETSENDERID SENDER_ID_ \n";
            dbglogger_.CheckToFlushBuffer();
            break;
          }

          m_athread->getEngine()->setSenderSubId(tokens_[2]);
          fprintf(stdout, "Setting SenderSubId to : %s\n", tokens_[2]);
        } break;

        case kSetClientInfo: {  // syntax CONTROLCOMMAND SETCLIENTINFO SHORTCODE CLIENT_INFO
          if (tokens_.size() != 4) {
            dbglogger_ << "Improper Syntax for CONTROLCOMMAND SETCLIENTINFO SHORTCODE CLIENT_INFO \n";
            dbglogger_.CheckToFlushBuffer();
            break;
          }

          const char* this_exch_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(tokens_[2]);
          unsigned int t_security_id_ = simple_security_symbol_indexer_.GetIdFromSecname(this_exch_symbol_);

          m_athread->getEngine()->SetClientInfo(t_security_id_, tokens_[3]);
          fprintf(stdout, "Setting ClientInfo for %s to %s\n", tokens_[2], tokens_[3]);
        } break;

        case kDumpEngineLoggingInfo: {  // syntax CONTROLCOMMAND SETCLIENTINFO SHORTCODE CLIENT_INFO
          if (tokens_.size() != 2) {
            dbglogger_ << "Improper Syntax for CONTROLCOMMAND DumpEngineLoggingInfo \n";
            dbglogger_.CheckToFlushBuffer();
            break;
          }

          m_athread->getEngine()->DumpEngineLogginInfo();
        } break;

        case kSimulateFundsReject: {
          if (tokens_.size() != 3) {
            dbglogger_ << "Improper Syntax for CONTROLCOMMAND REJECTDUETOFUNDS SAOS \n";
            dbglogger_.CheckToFlushBuffer();
            break;
          }
          int saos_ = atoi(tokens_[2]);
          m_athread->OnRejectDueToFunds(saos_);
        } break;

        // ------------------------------------------------------------------------------------------
        // ALL THIS WAS USED BY THE OLD TMX ENGINE.
        // VERIFY IF WE STILL NEED THIS WITH THE NEW TMX ENGINE ON THE PRODUCTION NETWORK.
        // -- .
        // ------------------------------------------------------------------------------------------

        // ///put additional margin control commands here
        // ///TMX Conformance stuff
        // case kTMXForceDeSync:
        //   {
        //     m_settings.setValue( "TMXForceDeSync", "yes" );
        //   }
        //   break;
        // case kTMXSync:
        //   {
        //     m_settings.setValue( "TMXForceDeSync", "no" );
        //   }
        //   break;
        // case kTMXAccountType:
        //   {
        //     //syntax CONTROLCOMMAND TMXACCOUNTTYPE TYPE
        //     TMXEngine* t_engine = static_cast<TMXEngine* >( m_athread -> getEngine( ) );
        //     if ( tokens_.size( ) != 3 )
        //       {
        // 	dbglogger_ << "Improper Syntax for CONTROLCOMMAND TMXACCOUNTTYPE \n";
        // 	dbglogger_.CheckToFlushBuffer ( );
        // 	break;
        //       }
        //     //                    t_engine -> SetAccountType( tokens_[ 2 ][ 0 ] );
        //   }
        //   break;
        // case kTMXClearingTradeType:
        //   {
        //     //syntax CONTROLCOMMAND TMXCLEARINGTRADETYPE TYPE
        //     TMXEngine* t_engine = static_cast<TMXEngine* >( m_athread -> getEngine( ) );
        //     if ( tokens_.size( ) != 3 )
        //       {
        //       dbglogger_ << "Improper Syntax for CONTROLCOMMAND TMXCLEARINGTRADETYPE \n";
        //       dbglogger_.CheckToFlushBuffer ( );
        //       break;
        //     }
        //     //                    t_engine -> SetClearingTradeType( tokens_[ 2 ][ 0 ] );
        //   }
        //   break;
        // case kTMXModifyOrderMode:
        //   {
        //     //syntax CONTROLCOMMAND TMXMODIFYORDERMODE TYPE
        //     TMXEngine* t_engine = static_cast<TMXEngine* >( m_athread -> getEngine( ) );
        //     if ( tokens_.size( ) != 3 )
        //       {
        //       dbglogger_ << "Improper Syntax for CONTROLCOMMAND TMXMODIFYORDERMODE \n";
        //       dbglogger_.CheckToFlushBuffer ( );
        //       break;
        //     }
        //     //                    t_engine -> SetModOrderMode( tokens_[ 2 ][ 0 ] );
        //   }
        //   break;
        // case kTMXSendRFQ:
        //   {
        //     ///syntax CONTROLCOMMAND TMXSENDRFQ INST SIZE
        //     TMXEngine* t_engine = static_cast<TMXEngine* >( m_athread -> getEngine( ) );
        //     if ( tokens_.size( ) != 4 )
        //       {
        //       dbglogger_ << "Improper Syntax for CONTROLCOMMAND TMXSENDRFQ \n";
        //       dbglogger_.CheckToFlushBuffer ( );
        //       break;
        //     }
        //     //                    t_engine -> SendRFQ( tokens_[ 2 ], atoi( tokens_[ 3 ] ) );
        //   }
        //   break;

        /// LOPR related stuff .. permanent
        case kTMXRegisterAccount: {
        } break;
        case kTMXDeleteAccount: {
        } break;
        case kTMXPositionEntry: {
        } break;
        case kTMXPositionDelete: {
        } break;
        case kTMXPositionDelimiter: {
        } break;

        case kAddPriceLimit: {
          /// CONTROLCOMMAND ADDPRICELIMIT SHORTCODE LOWERLIMIT UPPERLIMIT
          if (tokens_.size() != 5) {
            dbglogger_
                << "Improper Syntax. Expecting CONTROLCOMMAND ADDPRICELIMIT <SHORTCODE> <LOWERLIMIT> <UPPERLIMIT>"
                << DBGLOG_ENDL_FLUSH;
            break;
          }

          std::string shortcode_temp(tokens_[2]);
          std::string lower_limit_temp(tokens_[3]);
          std::string upper_limit_temp(tokens_[4]);

          if ((atof(lower_limit_temp.c_str()) <= 0 || atof(upper_limit_temp.c_str()) <= 0 ||
               atof(lower_limit_temp.c_str()) >= 100000 || atof(upper_limit_temp.c_str()) >= 100000)) {
            dbglogger_ << "Requested Price Limits Can't be Set, Fails Sanity Checks " << DBGLOG_ENDL_FLUSH;
            break;
          }

          const char* this_exch_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(tokens_[2]);

          int t_security_id_ = simple_security_symbol_indexer_.GetIdFromSecname(this_exch_symbol_);

          if (t_security_id_ < 0) {
            dbglogger_ << " ORS doens't know of requested security, first provide the ADDTRADINGSYMBOL"
                       << DBGLOG_ENDL_FLUSH;
            break;
          }
          // FIXME: Add something here

          dbglogger_ << " LowerPriceLimit : " << lower_limit_temp << " UpperPriceLimit : " << upper_limit_temp
                     << DBGLOG_ENDL_FLUSH;

        } break;

        case kDumpORSPositions: {
          dbglogger_ << "OM\n" << order_manager_.DumpOMState() << "PM\n" << position_manager_.DumpPMState() << "\n";
          dbglogger_.DumpCurrentBuffer();

        } break;

        case kTagETIAlgo: {
          if (tokens_.size() != 6) {
            dbglogger_
                << "Improper Syntax. Expecting CONTROLCOMMAND TagETIAlgo <SACI> <MD5SUM> <STRATEGY_NAME><exchange_name>"
                << DBGLOG_ENDL_FLUSH;
            break;
          }

          // Make Sure We have Up-to-Date Database EveryTime Client Connects
          eti_algo_tagger_.ETIAlgoTaggingReloadDatabase(tokens_[5]);

          if (!eti_algo_tagger_.AddClientInfo(atoi(tokens_[2]), tokens_[3], tokens_[4])) {
            dbglogger_ << "Failed to Add Client : " << tokens_[2] << " " << tokens_[3] << " " << tokens_[4] << " "
                       << tokens_[5] << "\n";
            dbglogger_.DumpCurrentBuffer();
            break;
          }

        } break;

        case kAddExternalPositions: {
          if (6 != tokens_.size()) {
            dbglogger_ << "INVALID SYNTAX, EXPECTING CONTROLCOMMAND ADDEXTERNALPOSITIONS <SECURITYNAME> <SACI> "
                          "<POSITION> <BUY/SELL>" << DBGLOG_ENDL_FLUSH;
            dbglogger_.DumpCurrentBuffer();

            break;
          }

          // Replace all occurrences of ~ with space (required mostly for LFIs: as their symbol contains spaces)
          std::string sec_symbol = tokens_[2];
          std::replace(sec_symbol.begin(), sec_symbol.end(), '~', ' ');

          int32_t security_id = simple_security_symbol_indexer_.GetIdFromSecname(sec_symbol.c_str());

          dbglogger_ << " SECURITY ID : " << security_id << " TOKEN : " << tokens_[2] << " "
                     << simple_security_symbol_indexer_.GetIdFromSecname(tokens_[2]) << "\n";
          dbglogger_.DumpCurrentBuffer();

          if (security_id < 0) {
            dbglogger_ << "ADDEXTERNALPOSITIONS CALLED FOR UNKNOWN SECURITY : " << tokens_[2]
                       << " SACI : " << tokens_[3] << " POSITION : " << tokens_[4] << " BUY/SELL : " << tokens_[5]
                       << DBGLOG_ENDL_FLUSH;
            dbglogger_.DumpCurrentBuffer();
            break;
          }

          if ("BUY" == std::string(tokens_[5])) {
            position_manager_.AddBuyTrade(security_id, atoi(tokens_[3]), atoi(tokens_[4]), false);

          } else if ("SELL" == std::string(tokens_[5])) {
            position_manager_.AddSellTrade(security_id, atoi(tokens_[3]), atoi(tokens_[4]), false);

          } else {
            dbglogger_ << "ADDEXTERNALPOSITIONS CALLED FOR INVALID TRADE-SIDE : " << tokens_[5] << "\n";
            dbglogger_.DumpCurrentBuffer();
          }

        } break;

        case kAddCombinedTradingSymbol: {
          if (tokens_.size() < 9) {
            dbglogger_ << "Improper Syntax. Expecting <CONTROLCOMMAND> <ADDCOMBINEDTRADINGSYMBOL> <COMBINED_SHORTCODE> "
                          "<MAX_POSITION> <MAX_ORDER_SIZE> <MAX_WORSTCASE> <MAX_LIVE_SIZE> "
                          "<AT_LEAST_ONE_CONSTITUENT_IN_LIST> <CONSTITUENT_WEIGHT_NOT_LESS_THAN_1> \n";
            dbglogger_.DumpCurrentBuffer();
            break;
          }

          std::string shortcode_temp(tokens_[2]);
          int max_pos = std::atoi(tokens_[3]);
          int max_order_size = std::atoi(tokens_[4]);

          if (!acceptableMaxPosParams1(shortcode_temp, max_pos, max_order_size)) {
            break;
          }

          bool is_shortcode_present = HFSAT::SecurityDefinitions::CheckIfContractSpecExists(
              shortcode_temp, HFSAT::DateTime::GetCurrentIsoDateLocal());

          if (!is_shortcode_present) {
            DBGLOG_CLASS_FUNC_LINE_ERROR << "Shortcode in ADDCOMBINEDTRADINGSYMBOL : " << shortcode_temp
                                         << " not present in ContractDefinition. Returning.." << DBGLOG_ENDL_FLUSH;
            break;
          }

          for (uint32_t component_counter = 7; tokens_.size() > component_counter; component_counter += 2) {
            std::string component_shortcode(tokens_[component_counter]);

            bool is_component_shortcode_present = HFSAT::SecurityDefinitions::CheckIfContractSpecExists(
                component_shortcode, HFSAT::DateTime::GetCurrentIsoDateLocal());

            if (!is_component_shortcode_present) {
              DBGLOG_CLASS_FUNC_LINE_ERROR << "Shortcode in ADDCOMBINEDTRADINGSYMBOL : " << component_shortcode
                                           << " not present in ContractDefinition. Returning.." << DBGLOG_ENDL_FLUSH;
              break;
            }
          }

          const char* this_exch_symbol_ = ExchangeSymbolManager::GetExchSymbol(tokens_[2]);

          int t_combined_security_id_ = simple_security_symbol_indexer_.GetIdFromSecname(this_exch_symbol_);

          if (t_combined_security_id_ < 0) {
            dbglogger_ << "Calling AddString For : " << this_exch_symbol_
                       << " With Combined RiskParams As, MaxPosition : " << max_pos
                       << " MaxOrderSize : " << max_order_size << "\n";
            dbglogger_.DumpCurrentBuffer();
            if (simple_security_symbol_indexer_.GetNumSecurityId() < DEF_MAX_SEC_ID) {
              simple_security_symbol_indexer_.AddString(this_exch_symbol_);
              t_combined_security_id_ = simple_security_symbol_indexer_.GetIdFromSecname(this_exch_symbol_);

              if (t_combined_security_id_ < 0) {
                dbglogger_ << " Failed to set limits \n";  // nothing to do here, next check will take care of this.
              }
            } else {
              dbglogger_ << "Not adding symbol : " << this_exch_symbol_
                         << ". NumSecurities exceeding DEF_MAX_SEC_ID : " << DEF_MAX_SEC_ID << DBGLOG_ENDL_FLUSH;
            }
          }

          if (t_combined_security_id_ < 0) {
            dbglogger_ << "AddString has failed for " << tokens_[2] << " or exchsymbol "
                       << HFSAT::ExchangeSymbolManager::GetExchSymbol(tokens_[2]) << " that has not been added "
                       << DBGLOG_ENDL_FLUSH;
            break;
          }

          else {
            SidToShortCodeMap()[t_combined_security_id_] = tokens_[2];
          }

          // We make sure t_combined_security_id_ must be >=0 else margin_checker will segfualt trying to set negative
          // index for array
          margin_checker_.Set(t_combined_security_id_, max_pos, max_order_size, max_pos, 0);

          for (uint32_t component_counter = 7; tokens_.size() > component_counter; component_counter += 2) {
            const char* this_exch_symbol_ = ExchangeSymbolManager::GetExchSymbol(tokens_[component_counter]);

            int t_security_id_ = simple_security_symbol_indexer_.GetIdFromSecname(this_exch_symbol_);

            if (t_security_id_ < 0) {
              dbglogger_ << "Calling AddString : " << this_exch_symbol_
                         << " With Combined RiskParams As, MaxPosition : " << max_pos
                         << " MaxOrderSize : " << max_order_size << "\n";
              dbglogger_.DumpCurrentBuffer();
              simple_security_symbol_indexer_.AddString(this_exch_symbol_);
              t_security_id_ = simple_security_symbol_indexer_.GetIdFromSecname(this_exch_symbol_);

              if (t_security_id_ < 0) {
                dbglogger_ << " Failed to set limits \n";  // nothing to do here, next check will take care of this.
              }
            }

            if (t_security_id_ < 0) {
              dbglogger_ << "AddString has failed for " << tokens_[2] << " or exchsymbol "
                         << ExchangeSymbolManager::GetExchSymbol(tokens_[2]) << " that has not been added "
                         << DBGLOG_ENDL_FLUSH;
              break;
            }

            else {
              if (!AddMifidLimits(tokens_[component_counter], t_security_id_)) break;

              SidToShortCodeMap()[t_security_id_] = tokens_[2];
              HFSAT::ORSUtils::SecurityDBManager::GetUniqueInstance().SetSecurityShortcode(t_security_id_, tokens_[2]);
            }

            double security_weight = atof(tokens_[component_counter + 1]);
            // TODO, Assuming Only Sreads Have SP_
            bool is_this_security_spread =
                std::string::npos == ((std::string(tokens_[component_counter])).find("SP_")) ? false : true;

            position_manager_.AddSecurityToCombinedPool(t_security_id_, t_combined_security_id_, security_weight,
                                                        is_this_security_spread);
          }

        } break;

        case kSendCrossTrade: {
          if (tokens_.size() < 8) {
            dbglogger_ << "Improper Syntax. Expecting <CONTROLCOMMAND> <SENDCROSSTRADE> <SYMBOL> <ORDER_QTY> <PRICE> "
                          "<ACCOUNT> <BROKER_TAG> <COUNTER_PARTY>\n";
            dbglogger_.DumpCurrentBuffer();
            break;
          }

          m_athread->getEngine()->SendCrossTrade(tokens_[2], atoi(tokens_[3]), atof(tokens_[4]), tokens_[5], tokens_[6],
                                                 tokens_[7]);

        } break;

        case kRecoverFundRejects: {
          if (tokens_.size() < 3) {
            dbglogger_ << "Improper Syntax. Expecting <CONTROLCOMMAND> <RECOVERFUNDREJECTS> <SYMBOL>\n";
            dbglogger_.DumpCurrentBuffer();
            break;
          }

          // Replace all occurrences of ~ with space (required mostly for LFIs: as their symbol contains spaces)
          std::string sec_symbol = tokens_[2];
          std::replace(sec_symbol.begin(), sec_symbol.end(), '~', ' ');

          int32_t security_id = simple_security_symbol_indexer_.GetIdFromSecname(sec_symbol.c_str());

          m_athread->ForceRecoverFundRejects(security_id);

        } break;

        // TODO - Not Supporting Right Now
        case kUpdateNSEConfigFiles: {
          if (tokens_.size() != 2) {
            DBGLOG_CLASS_FUNC_LINE_ERROR
                << "INVALID CONTROLCMD SYNTAX, EXPECTING : <CONTROLCOMMAND> <UPDATENSECONFIGFILES>"
                << DBGLOG_ENDL_NOFLUSH;
            DBGLOG_DUMP;
          }

          //          nse_config_based_checks_.ReloadAllConfigBasedChecksFiles();

        } break;

        case kSetPasswordOSEUser: {  // syntax CONTROLCOMMAND SETCLIENTINFO SHORTCODE CLIENT_INFO
          if (tokens_.size() != 4) {
            dbglogger_ << "Improper Syntax for CONTROLCOMMAND SETPASSWORD USER NEWPASSWORD \n";
            dbglogger_.CheckToFlushBuffer();
            break;
          }

          m_athread->getEngine()->SetOSEUserPassword(atoi(tokens_[2]), tokens_[3]);
        } break;

        case kSendExplicitOrder: {
          /// syntax CONTROLCOMMAND SENDORDER SYMBOL SEQNUM SIDE SIZE PX
          if (tokens_.size() != 7) {
            dbglogger_ << "Improper Syntax for CONTROLCOMMAND SENDEXPLICITORDER "
                          "SERIES SIZE PRICE BID/ASK SAOS\n";
            dbglogger_.CheckToFlushBuffer();
            break;
          }

          std::string bidask_ = tokens_[5];

          char temp_ = (bidask_ == "BUY") ? 'B' : 'S';

          dbglogger_ << " Send ExplitiOrder \n";
          dbglogger_.DumpCurrentBuffer();

          m_athread->getEngine()->SendOrderExplicit(tokens_[2], atoi(tokens_[3]), atoi(tokens_[4]), temp_,
                                                    atoi(tokens_[5]));

        } break;

        case kCxlExplicitOrder: {
          /// syntax CONTROLCOMMAND SENDORDER SYMBOL SEQNUM SIDE SIZE PX
          if (tokens_.size() != 6) {
            dbglogger_ << "Improper Syntax for CONTROLCOMMAND CXLEXPLICITORDER "
                          "SERIES ORDERNUM BID/ASK SAOS\n";
            dbglogger_.CheckToFlushBuffer();
            break;
          }

          uint64_t order_num_ = 0;
          order_num_ = strtoul(tokens_[3], NULL, 0);

          dbglogger_ << " OrderNumber : " << order_num_ << "\n";
          dbglogger_.DumpCurrentBuffer();

          std::string bidask_ = tokens_[4];
          char temp_ = (bidask_ == "BUY") ? 'B' : 'S';

          dbglogger_ << " Send ExplitiCxlOrder \n";
          dbglogger_.DumpCurrentBuffer();

          m_athread->getEngine()->CancelExplicitOrder(order_num_, tokens_[2], temp_, atoi(tokens_[5]));

        } break;

        case kQueryClearingInfo: {  // syntax CONTROLCOMMAND QUERYCLRINFO
          if (tokens_.size() != 2) {
            dbglogger_ << "Improper Syntax for CONTROLCOMMAND QUERYCLRINFO \n";
            dbglogger_.CheckToFlushBuffer();
            break;
          }

          m_athread->getEngine()->QueryClrInfo();
        } break;

        case kQueryOrderBook: {  // syntax CONTROLCOMMAND QUERYCLRINFO
          if (tokens_.size() != 3) {
            dbglogger_ << "Improper Syntax for CONTROLCOMMAND QUERYORDERBOOK SYMBOL\n";
            dbglogger_.CheckToFlushBuffer();
            break;
          }

          m_athread->getEngine()->QueryOrderBook(tokens_[2]);
        } break;

        case kQueryInactiveOrderBook: {  // syntax CONTROLCOMMAND QUERYCLRINFO
          if (tokens_.size() != 3) {
            dbglogger_ << "Improper Syntax for CONTROLCOMMAND QUERYINACTIVEORDERBOOK SYMBOL\n";
            dbglogger_.CheckToFlushBuffer();
            break;
          }

          m_athread->getEngine()->QueryInactiveOrderBook(tokens_[2]);
        } break;

        case kQueryMissingTrade: {  // syntax CONTROLCOMMAND QUERYCLRINFO
          if (tokens_.size() != 6) {
            dbglogger_ << "Improper Syntax for CONTROLCOMMAND QUERYORDERBOOK SYMBOL START_SEQ END_SEQ DATE \n";
            dbglogger_.CheckToFlushBuffer();
            break;
          }

          m_athread->getEngine()->QueryMissingTrade(atoi(tokens_[3]), atoi(tokens_[4]), tokens_[5], tokens_[2]);
        } break;

        case kQueryMissingBroadcast: {  // syntax CONTROLCOMMAND QUERYCLRINFO
          if (tokens_.size() != 6) {
            dbglogger_ << "Improper Syntax for CONTROLCOMMAND QUERYORDERBOOK SYMBOL START_SEQ END_SEQ DATE \n";
            dbglogger_.CheckToFlushBuffer();
            break;
          }

          m_athread->getEngine()->QueryMissingBroadcast(atoi(tokens_[3]), atoi(tokens_[4]), tokens_[5], tokens_[2]);
        } break;

        case kExplicitCancel: {  // syntax CONTROLCOMMAND QUERYCLRINFO
          if (tokens_.size() != 5) {
            dbglogger_ << "Improper Syntax for CONTROLCOMMAND EXPLICITCXL SYMBOL BUYSELL ORDERNUM \n";
            dbglogger_.CheckToFlushBuffer();
            break;
          }

          uint64_t order_num_ = 0;

          order_num_ = strtoul(tokens_[4], NULL, 0);

          dbglogger_ << " OrderNumber : " << order_num_ << "\n";
          dbglogger_.DumpCurrentBuffer();

          std::string bdask_ = tokens_[3];

          char bid_ask_ = (bdask_ == "BUY") ? 'B' : 'S';

          std::string symbol_ = tokens_[2];

          m_athread->getEngine()->CancelOrderExplicit(order_num_, bid_ask_, symbol_);
        } break;

        case kExplicitCanceNormal: {  // syntax CONTROLCOMMAND QUERYCLRINFO
          if (tokens_.size() != 5) {
            dbglogger_ << "Improper Syntax for CONTROLCOMMAND EXPLICITCXLNORMAL SYMBOL BUYSELL ORDERNUM \n";
            dbglogger_.CheckToFlushBuffer();
            break;
          }

          uint64_t order_num_ = 0;

          order_num_ = strtoul(tokens_[4], NULL, 0);

          dbglogger_ << " OrderNumber : " << order_num_ << "\n";
          dbglogger_.DumpCurrentBuffer();

          std::string bdask_ = tokens_[3];

          char bid_ask_ = (bdask_ == "BUY") ? 'B' : 'S';

          std::string symbol_ = tokens_[2];

          m_athread->getEngine()->CancelOrderExplicitNormal(order_num_, bid_ask_, symbol_);
        } break;

        case kSetExchOrderType: {  // syntax CONTROLCOMMAND SETCLIENTINFO SHORTCODE
                                   // CLIENT_INFO
          if (tokens_.size() != 3) {
            dbglogger_ << "Improper Syntax for CONTROLCOMMAND SETEXCHORDERTYPE "
                          "EXCH_ORD_TYPE_N_ \n";
            dbglogger_.CheckToFlushBuffer();
            break;
          }

          m_athread->getEngine()->SetExchOrderType(atoi(tokens_[2]));
        } break;

        case kSetOrderValidity: {  // syntax CONTROLCOMMAND SETCLIENTINFO SHORTCODE
                                   // CLIENT_INFO
          if (tokens_.size() != 3) {
            dbglogger_ << "Improper Syntax for CONTROLCOMMAND SETVALIDITY VALIDITY \n";
            dbglogger_.CheckToFlushBuffer();
            break;
          }

          m_athread->getEngine()->SetOrderValidity(atoi(tokens_[2]));
        } break;

        case kQueryMarketStatus: {  // syntax CONTROLCOMMAND QUERYCLRINFO
          if (tokens_.size() != 2) {
            dbglogger_ << "Improper Syntax for CONTROLCOMMAND QUERYMARKETSTATUS\n";
            dbglogger_.CheckToFlushBuffer();
            break;
          }

          m_athread->getEngine()->QueryMarketStatus();
        } break;

        case kQueryInstrumentClass: {  // syntax CONTROLCOMMAND QUERYCLRINFO
          if (tokens_.size() != 2) {
            dbglogger_ << "Improper Syntax for CONTROLCOMMAND QUERYINSTRUMENTCLASS \n";
            dbglogger_.CheckToFlushBuffer();
            break;
          }

          m_athread->getEngine()->QueryInstrumentClass();
        } break;

        case kQuerySeriesRealTime: {  // syntax CONTROLCOMMAND QUERYCLRINFO
          if (tokens_.size() != 2) {
            dbglogger_ << "Improper Syntax for CONTROLCOMMAND QUERYSERIESREALTIME\n";
            dbglogger_.CheckToFlushBuffer();
            break;
          }

          m_athread->getEngine()->QuerySeriesRealTime();
        } break;

        case kQueryComboSeriesRealTime: {  // syntax CONTROLCOMMAND QUERYCLRINFO
          if (tokens_.size() != 2) {
            dbglogger_ << "Improper Syntax for CONTROLCOMMAND "
                          "QUERYCOMBOSERIESREALTIME\n";
            dbglogger_.CheckToFlushBuffer();
            break;
          }

          m_athread->getEngine()->QueryComboSeriesRealTime();
        } break;

        case kDumpLastTradedPrice: {
          margin_checker_.DumpLastTradedPrices();
        } break;

        case kSetPriceLimitCheck: {
          /// CONTROLCOMMAND SETPRICELIMITCHECK 0/1
          if (tokens_.size() < 3 || !IsNumber(tokens_[2])) {
            dbglogger_ << "Improper Syntax. Expecting CONTROLCOMMAND SETPRICELIMITCHECK 0/1" << DBGLOG_ENDL_FLUSH;
            break;
          }

          int input = std::atoi(tokens_[2]);
          if (!(input == 0 || input == 1)) {
            dbglogger_ << "Improper Syntax. Expecting CONTROLCOMMAND SETPRICELIMITCHECK 0/1" << DBGLOG_ENDL_FLUSH;
            break;
          }
          bool is_enabled = (!!input);

          dbglogger_ << "PriceLimitCheck Status : " << is_enabled << "\n";
          dbglogger_.DumpCurrentBuffer();

          margin_checker_.SetPriceCheckMode(is_enabled);

        } break;

        case kSetAllowedPriceTickLimit: {
          /// CONTROLCOMMAND SETALLOWEDPRICETICKLIMIT <NUM_TICKS>
          if (tokens_.size() < 3 || !IsNumber(tokens_[2])) {
            dbglogger_ << "Improper Syntax. Expecting CONTROLCOMMAND SETALLOWEDPRICETICKLIMIT <NUM_TICKS>"
                       << DBGLOG_ENDL_FLUSH;
            break;
          }

          int input = std::atoi(tokens_[2]);
          if (input < 1 || input > 1000) {
            dbglogger_
                << "Improper Syntax. Expecting CONTROLCOMMAND SETALLOWEDPRICETICKLIMIT <NUM_TICKS (Between 1 and 100)>"
                << DBGLOG_ENDL_FLUSH;
            break;
          }

          dbglogger_ << "AllowedPriceTickLimit : " << input << "\n";
          dbglogger_.DumpCurrentBuffer();

          margin_checker_.SetAllowedPriceTickLimit(input);
        } break;
        case kDumpSACIPositions: {
          std::string out_filename;
          if (tokens_.size() == 4) {
            std::string saci_cfg_file(tokens_[3]);
            out_filename = tokens_[2];
            DisplaySACIPositions(out_filename, saci_cfg_file);
          } else if (tokens_.size() == 3) {
            out_filename = tokens_[2];
            DisplaySACIPositions(out_filename);
          } else {
            dbglogger_ << "Invalid command. Usage - control_exec control_port DUMPSACIPOSITIONS Outputfile SACIConfig";
            dbglogger_.DumpCurrentBuffer();
            break;
          }

        } break;
        case kDumpSecurityPositions: {
          if (tokens_.size() == 3) {
            std::string out_filename(tokens_[2]);
            DisplaySecurityPositions(out_filename);
          } else {
            dbglogger_ << "Invalid command. Usage - control_exec control_port DUMPSACIPOSITIONS Outputfile SACIConfig";
            dbglogger_.DumpCurrentBuffer();
            break;
          }
        } break;

        case kSetORSPnlCheck: {
          if (3 == tokens_.size()) {
            int32_t ors_stop_pnl = atoi(tokens_[2]);
            HFSAT::ORSUtils::ORSPnlManager::GetUniqueInstance(dbglogger_).SetORSStopPnl(ors_stop_pnl);

            if (std::string::npos != HFSAT::GetCurrentHostName().find("sdv-ind-")) {
              HFSAT::Email e;
              e.setSubject("NSE PNL UPDATE REQUEST");
              e.addRecepient(
                "nseall@tworoads.co.in, "
				"joseph.padiyara@tworoads-trading.co.in, uttkarsh.sarraf@tworoads.co.in, ravi.parikh@tworoads-trading.co.in, "
				"rakesh.kumar@tworoads.co.in, nishit.bhandari@tworoads.co.in, nsehft@tworoads.co.in, nsemidterm@tworoads.co.in");
              e.addSender("nseall@tworoads.co.in");
              e.content_stream << "host_machine: " << HFSAT::GetCurrentHostName() << "<br/>";
              e.content_stream << "PNL REQUESTED VALUE : " << ors_stop_pnl << "<br/>";
              e.sendMail();
            }

          } else {
            DBGLOG_CLASS_FUNC_LINE_ERROR << "INVALID CONTROL COMMAND. USAGE : SETORSPNLCHECK <PNL_VALUE>";
            DBGLOG_DUMP;
          }

        } break;

        case kSetORSGrossMarginCheck: {
          if (3 == tokens_.size()) {
            double gross_margin = atof(tokens_[2]);
            HFSAT::ORSUtils::ORSMarginManager::GetUniqueInstance(dbglogger_).SetGrossMarginCheck(gross_margin);

            if (std::string::npos != HFSAT::GetCurrentHostName().find("sdv-ind-")) {
              HFSAT::Email e;
              e.setSubject("NSE GROSS MARGIN UPDATE REQUEST");
              e.addRecepient(
                "nseall@tworoads.co.in, "
				"joseph.padiyara@tworoads-trading.co.in, uttkarsh.sarraf@tworoads.co.in, ravi.parikh@tworoads-trading.co.in, "
			  	"rakesh.kumar@tworoads.co.in, nishit.bhandari@tworoads.co.in, nsehft@tworoads.co.in, nsemidterm@tworoads.co.in");
              e.addSender("nseall@tworoads.co.in");
              e.content_stream << "host_machine: " << HFSAT::GetCurrentHostName() << "<br/>";
              e.content_stream << "GROSS MARGIN REQUESTED VALUE : " << gross_margin << "<br/>";
              e.sendMail();
            }

          } else {
            DBGLOG_CLASS_FUNC_LINE_ERROR << "INVALID CONTROL COMMAND. USAGE : SETGROSSMARGINCHECK <GROSS_MARGIN_VALUE>";
            DBGLOG_DUMP;
          }

        } break;

        case kSetORSNetMarginCheck: {
          if (3 == tokens_.size()) {
            double net_margin = atof(tokens_[2]);
            HFSAT::ORSUtils::ORSMarginManager::GetUniqueInstance(dbglogger_).SetNetMarginCheck(net_margin);
          } else {
            DBGLOG_CLASS_FUNC_LINE_ERROR << "INVALID CONTROL COMMAND. USAGE : SETNETMARGINCHECK <NET_MARGIN_VALUE>";
            DBGLOG_DUMP;
          }

        } break;

        case kUpdateMarginFactor: {
          if (3 == tokens_.size()) {
            double margin_fac = atof(tokens_[2]);
            HFSAT::ORSUtils::ORSMarginManager::GetUniqueInstance(dbglogger_).UpdateMarginFactor(margin_fac);
          } else {
            DBGLOG_CLASS_FUNC_LINE_ERROR << "INVALID CONTROL COMMAND. USAGE : UPDATEMARGINFACTOR <MARGIN_FACTOR_VALUE>";
            DBGLOG_DUMP;
          }

        } break;

        case kUpdateSecurityMarginValue: {
          if (4 == tokens_.size()) {
            std::string shc = tokens_[2];
            double margin = atof(tokens_[3]);
            HFSAT::ORSUtils::ORSMarginManager::GetUniqueInstance(dbglogger_)
                .UpdateSecurityMarginForShortCode(shc, margin);
          } else {
            DBGLOG_CLASS_FUNC_LINE_ERROR
                << "INVALID CONTROL COMMAND. USAGE : UPDATESECURITYMARGIN  <SHORTCODE> <MARGIN_VALUE>";
            DBGLOG_DUMP;
          }

        } break;

        case kReloadSecurityMarginFile: {
          if (2 == tokens_.size()) {
            HFSAT::ORSUtils::ORSMarginManager::GetUniqueInstance(dbglogger_).ReloadMarginValues();
          } else {
            DBGLOG_CLASS_FUNC_LINE_ERROR << "INVALID CONTROL COMMAND. USAGE : RELOADSECURITYMARGINFILE";
            DBGLOG_DUMP;
          }

        } break;

        case kDumpORSPnlMarginStatus: {
          if (2 == tokens_.size()) {
            HFSAT::ORSUtils::ORSMarginManager::GetUniqueInstance(dbglogger_).DumpMarginStatus();
            HFSAT::ORSUtils::ORSPnlManager::GetUniqueInstance(dbglogger_).DumpORSPnlStatus();
          } else {
            DBGLOG_CLASS_FUNC_LINE_ERROR << "INVALID CONTROL COMMAND. USAGE : DUMPORSPNLMARGINSTATUS";
            DBGLOG_DUMP;
          }

        } break;

        case kControlInvalid:
        default: {
          dbglogger_ << " Invalid Control Message received " << tokens_[1] << " .. hence ignoring." << '\n';
          dbglogger_.CheckToFlushBuffer();
        } break;
      }
    }
  }
  // Close socket before exiting thread
  Close();
}

void ControlThread::EmailAddSymbolFailed(std::string alert_body_) {
  char hostname[128];
  hostname[127] = '\0';
  gethostname(hostname, 127);
  std::string alert_message = std::string("ALERT: ADDTRADINGSYMBOL Warning at ") + std::string(hostname) + "\n";
  HFSAT::SendAlert::sendAlert(alert_message);

  HFSAT::Email e;

  e.setSubject("ADDTRADINGSYMBOL - Warning");
  e.addRecepient("nseall@tworoads.co.in");
  e.addSender("nseall@tworoads.co.in");
  e.content_stream << alert_body_ << "<br/>";
  e.sendMail();
}

int ControlThread::GetKthPercentOf(int num, int k) {
  double num_double = (double)num;
  double factor = (double)k / 100;
  num_double = num_double * factor;
  double num_floor = floor(num_double);
  return (fabs(num_double - num_floor) < EQUALITY_TOLERANCE) ? (int(num_floor) - 1) : (int)num_floor;
}

void ControlThread::DisplaySACIPositions(std::string& out_filename, const std::string& saci_cfg_filename) {
  std::ofstream outfile;
  std::ifstream saci_file;
  std::string output_file_path = std::string(HFSAT::FILEPATH::kSACIPositionFolder) + out_filename;

  outfile.open(output_file_path);
  if (saci_cfg_filename.empty()) {
    outfile << position_manager_.DumpSACIPosition() << "\n";
    outfile.flush();
    outfile.close();
    return;
  }
  saci_file.open(saci_cfg_filename.c_str(), std::ifstream::in);
  if (!saci_file.is_open()) {
    dbglogger_ << "Cannot open file " << saci_cfg_filename << "\n";
    dbglogger_.DumpCurrentBuffer();
    return;
  }
  char line[1024];
  while (!saci_file.eof()) {
    bzero(line, 1024);
    saci_file.getline(line, 1024);
    if (strlen(line) == 0 || strstr(line, "#") != NULL) continue;
    HFSAT::PerishableStringTokenizer st_(line, 1024);
    const std::vector<const char*>& tokens_ = st_.GetTokens();
    if (tokens_.size() != 1) {
      dbglogger_ << "Ignoring malformatted line " << line << "\n";
      dbglogger_.DumpCurrentBuffer();
      continue;
    }
    int saci(atoi(tokens_[0]));
    if (position_manager_.GetClientPosition(saci) != 0) {
      outfile << "SACI: " << tokens_[0] << " Position: " << position_manager_.GetClientPosition(saci) << "\n";
      outfile.flush();
    }
  }
  outfile.close();
  saci_file.close();
}

void ControlThread::DisplaySecurityPositions(std::string& out_filename) {
  std::ofstream outfile;
  std::string output_file_path = std::string(HFSAT::FILEPATH::kSACIPositionFolder) + out_filename;
  outfile.open(output_file_path);
  if (outfile.is_open()) {
    outfile << position_manager_.DumpSecurityPositions();
    outfile << "\n\n";
    outfile.flush();
    outfile.close();
  } else {
    dbglogger_ << "Error opening the file " << output_file_path << "\n";
    dbglogger_.DumpCurrentBuffer();
  }
}
// MFGlobal checks on the risk parameters.
bool ControlThread::acceptableMaxPosParams1(std::string& shortcode_, int& max_pos_, int& max_ord_size_) {
  // This is the max position value assigned by the clearers.
  int clearer_max_pos_ = GetKthPercentOf(sec_to_max_pos_map_[shortcode_], 90);
  int clearer_max_order_size_ = sec_to_max_ord_sz_map_[shortcode_];

  if (std::string("NSE_FO") == exchange_) {
    if (max_pos_ <= NSE_MAX_ACCEPTABLE_POS && max_ord_size_ <= NSE_MAX_ACCEPTABLE_ORDER_SIZE) {
      return true;
    }
  }

  // For Cash There Aren't Any Hard Limits
  if (std::string("NSE_EQ") == exchange_) {
    return true;
  }

  if (std::string("NSE_CD") == exchange_) {
    if (max_pos_ <= NSE_MAX_ACCEPTABLE_POS && max_ord_size_ <= NSE_MAX_ACCEPTABLE_ORDER_SIZE) {
      return true;
    }
  }

  if (clearer_max_pos_ < max_pos_) {
    dbglogger_ << "Requested max position value : " << max_pos_
               << " exceeds clearer max_pos limits : " << clearer_max_pos_ << " for " << shortcode_
               << ". Resetting max pos to " << clearer_max_pos_ << ".\n";
    dbglogger_.CheckToFlushBuffer();

    std::ostringstream this_mail_msg_;
    this_mail_msg_ << "Requested max position value : " << max_pos_ << " exceeds clearer limits : " << clearer_max_pos_
                   << " for " << shortcode_ << ". Adding max pos limit " << clearer_max_pos_ << ".\n";
    EmailAddSymbolFailed(this_mail_msg_.str());

    max_pos_ = clearer_max_pos_;
  }

  if (clearer_max_pos_ < max_ord_size_) {
    dbglogger_ << "Requested max order size value : " << max_ord_size_
               << " exceeds clearer max_pos limits : " << clearer_max_pos_ << " for " << shortcode_
               << ". Resetting maxpos to " << clearer_max_pos_ << ".\n";
    dbglogger_.CheckToFlushBuffer();

    std::ostringstream this_mail_msg_;
    this_mail_msg_ << "Requested max order size value : " << max_ord_size_
                   << " exceeds clearer limits : " << clearer_max_pos_ << " for " << shortcode_
                   << ". Adding max order size limit " << clearer_max_pos_ << ".\n";
    EmailAddSymbolFailed(this_mail_msg_.str());

    max_ord_size_ = clearer_max_pos_;
  }

  if (clearer_max_order_size_ < max_ord_size_) {
    dbglogger_ << "Requested order size value : " << max_ord_size_
               << " exceeds clearer max_order_size limits : " << clearer_max_order_size_ << " for " << shortcode_
               << ". Resetting max order size to " << clearer_max_order_size_ << ".\n";
    dbglogger_.CheckToFlushBuffer();

    std::ostringstream this_mail_msg_;
    this_mail_msg_ << "Requested order size value : " << max_ord_size_
                   << " exceeds clearer limits : " << clearer_max_order_size_ << " for " << shortcode_
                   << ". Adding max order size " << clearer_max_order_size_ << ".\n";
    EmailAddSymbolFailed(this_mail_msg_.str());

    max_ord_size_ = clearer_max_order_size_;
  }

  return true;  // All checks passed or limits modified to max allowed limits
}

bool ControlThread::acceptableMaxPosParams(std::string shortcode_, std::string max_pos_, std::string max_ord_size_,
                                           std::string max_worst_case_size_, std::string max_live_ord_) {
  // This is the max position value assigned by the clearers.
  int clearer_max_pos_ = GetKthPercentOf(sec_to_max_pos_map_[shortcode_], 90);

  int clearer_max_order_size_ = sec_to_max_ord_sz_map_[shortcode_];

  if (clearer_max_pos_ < atoi(max_pos_.c_str())) {
    dbglogger_ << "Requested max position value : " << atoi(max_pos_.c_str())
               << " exceeds clearer limits : " << clearer_max_pos_ << " for " << shortcode_ << "\n";
    dbglogger_.CheckToFlushBuffer();
    return false;
  }

  if (clearer_max_pos_ < atoi(max_ord_size_.c_str())) {
    dbglogger_ << "Requested max order size value : " << atoi(max_ord_size_.c_str())
               << " exceeds clearer limits : " << clearer_max_pos_ << " for " << shortcode_ << "\n";
    dbglogger_.CheckToFlushBuffer();
    return false;
  }

  if (clearer_max_pos_ < atoi(max_worst_case_size_.c_str())) {
    dbglogger_ << "Requested max worst case size value : " << atoi(max_worst_case_size_.c_str())
               << " exceeds clearer limits : " << (clearer_max_pos_) << " for " << shortcode_ << "\n";
    dbglogger_.CheckToFlushBuffer();
    return false;
  }

  if (5 * clearer_max_pos_ < atoi(max_live_ord_.c_str())) {
    dbglogger_ << "Requested max live orders value : " << atoi(max_live_ord_.c_str())
               << " exceeds clearer limits : " << (5 * clearer_max_pos_) << " for " << shortcode_ << "\n";
    dbglogger_.CheckToFlushBuffer();
    return false;
  }

  if (clearer_max_order_size_ < atoi(max_ord_size_.c_str())) {
    dbglogger_ << "Requested order size value : " << atoi(max_ord_size_.c_str())
               << " exceeds clearer limits : " << clearer_max_order_size_ << " for " << shortcode_ << "\n";
    dbglogger_.CheckToFlushBuffer();
    return false;
  }

  return true;  // All checks passed.
}

bool ControlThread::AddMifidLimits(std::string shortcode, int sec_id) {
  if (is_mifid_check_applicable_) {
    if (shc_to_mifid_limits_.find(shortcode) != shc_to_mifid_limits_.end()) {
      MiFidLimits limit = shc_to_mifid_limits_[shortcode];

      margin_checker_.SetMifidLimits(sec_id, limit.otr_min_vol, limit.otr_min_cnt, limit.otr_limit_vol,
                                     limit.otr_limit_cnt, limit.msg_count);
      dbglogger_ << "Setting mifid limits for : " << shortcode << " ID: " << sec_id << " Limits: " << limit.otr_min_vol
                 << " : " << limit.otr_min_cnt << " : " << limit.otr_limit_vol << " : " << limit.otr_limit_cnt << " : "
                 << limit.msg_count << "\n";
      dbglogger_.DumpCurrentBuffer();
      return true;

    } else {
      dbglogger_ << "MiFid limits not present  for shortcode : " << shortcode << ". Won't be adding limits \n";
      dbglogger_.DumpCurrentBuffer();
      return false;
    }
  }
  return true;
}

bool ControlThread::IsNumber(const char* str) {
  for (unsigned int i = 0; i < strlen(str); i++) {
    if (!(str[i] >= '0' && str[i] <= '9')) {
      return false;
    }
  }
  return true;
}
}
}
