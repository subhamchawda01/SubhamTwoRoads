#include <iostream>
#include <sstream>
#include <string>
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/Utils/common_files_path.hpp"
#include "infracore/BasicOrderRoutingServer/control_command.hpp"
#include "infracore/BasicOrderRoutingServer/control_thread.hpp"
#include "dvccode/Utils/exchange_names.hpp"  

#define NSE_SECURITIES_UNDER_BAN_FILE_PATH "/spare/local/tradeinfo/NSE_Files/SecuritiesUnderBan/"
#define NSE_SECURITIES_WITH_PHYSICAL_SETTLEMENT "/spare/local/tradeinfo/NSE_Files/SecuritiesUnderPhysicalSettlement/"
#define NSE_SECURITIES_UNDER_UNSOLICITED_SMS "/spare/local/tradeinfo/NSE_Files/SecuritiesUnderUnsolicitedSMS/"
#define IPO_LIST "/spare/local/tradeinfo/NSE_Files/"

#define EQUALITY_TOLERANCE 0.0001
#define LAKH 100000

namespace HFSAT {
namespace ORS {

ControlThread::ControlThread(DebugLogger& _dbglogger_, const int _connected_socket_file_descriptor_,
                             Settings& _settings_,
                             HFSAT::Utils::ClientLoggingSegmentInitializer* _client_logging_segment_initializer_,
                             std::string t_output_log_dir_, bool &_cancel_live_order_flag_,
                             std::map<std::string, int>& _sec_to_max_pos_map_,
                             std::map<std::string, int>& _sec_to_max_ord_sz_map_,
                             HFSAT::ORS::ControlReceiver* ors_control_receiver, int32_t tid, bool* _is_addts_running_)
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
      control_receiver_(ors_control_receiver),
      list_of_securities_under_ban_(),
      list_of_securities_under_physical_settlement_(),
      list_of_securities_with_unsolicited_sms_(),
      exchange_(m_settings.getValue("Exchange")),
      thread_id(tid),
      is_mifid_check_applicable_(HFSAT::IsMiFidCheckApplicable(HFSAT::StringToExchSource(exchange_))),
      is_addts_running_(_is_addts_running_),
      cancel_live_order_flag_(_cancel_live_order_flag_){
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

  if ("NSE_FO" == exchange_) {
    std::ostringstream t_temp_oss;
    t_temp_oss << NSE_SECURITIES_WITH_PHYSICAL_SETTLEMENT << "fo_securities_under_physical_settlement.csv";

    std::ifstream fo_ps_securities_stream;
    fo_ps_securities_stream.open(t_temp_oss.str().c_str(), std::ifstream::in);

    if (!fo_ps_securities_stream.is_open()) {
      DBGLOG_CLASS_FUNC_LINE_FATAL << "CAN'T LOCATE PHYSICAL SETTLEMENT CONTRACT FILE -> " << t_temp_oss.str()
                                   << DBGLOG_ENDL_FLUSH;
      std::exit(-1);
    }

#define MAX_LINE_READ_LENGTH 1024
    char line_buffer[MAX_LINE_READ_LENGTH];

    while (fo_ps_securities_stream.good()) {
      fo_ps_securities_stream.getline(line_buffer, MAX_LINE_READ_LENGTH);
      if (std::string(line_buffer).length() < 1) continue;

      // Only FUT0 under physical settlement
      std::string this_shortcode = std::string(line_buffer);
      list_of_securities_under_physical_settlement_.insert(this_shortcode);

      if (false == physical_settlement_products_listed_) {
        DBGLOG_CLASS_FUNC_LINE_INFO << "ALL SHORTCODES OF TICKER : " << this_shortcode
                                    << " IS UNDER PHYSICAL SETTLEMENT FOR THIS EXPIRY " << DBGLOG_ENDL_FLUSH;
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

void ControlThread::AddtradingSymbol(std::string& shortcode_temp, int max_pos, int max_order_size,std::ostringstream& report_string){
  // This is to support cases where we are dealing with orders in actual size, i.e NSE
  int32_t lotsize_factor = 1;

  if (!acceptableMaxPosParams1(shortcode_temp, max_pos, max_order_size)) {
    return;
  }

  if (false ==
      HFSAT::ORSUtils::ORSMarginManager::GetUniqueInstance(dbglogger_).AllowAddTSForShortCode(shortcode_temp)) {
    DBGLOG_CLASS_FUNC_LINE_ERROR << "MARGIN CHECKS ENABLED ON ORS BUT MARGIN NOT PROVIDED FOR SHORTCODE : "
                                 << shortcode_temp << DBGLOG_ENDL_DUMP;
    return;
  }

  if ("NSE_FO" == exchange_ || "BSE_FO" == exchange_ ) {
    std::string shortcode = shortcode_temp;
    std::string basename = shortcode.substr(shortcode.find_first_of("_") + 1);
    basename = basename.substr(0, basename.find_first_of("_"));

    if (list_of_securities_under_ban_.end() != list_of_securities_under_ban_.find(basename)) {
      DBGLOG_CLASS_FUNC_LINE_ERROR << "SHORTCODE : " << shortcode << " BASENAME : " << basename
                                   << " CANT BE TRADED TODAY AS IT IS UNDER BAN" << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
      return;
    }
    bool is_nearest_expiry = ((shortcode.find("_FUT0") != std::string::npos) ||
        			(shortcode.find("_C0") != std::string::npos) || 
        			(shortcode.find("_P0") != std::string::npos));
    // Physical settlement, Check if expiry date and contract under physical settlement cycle
    if ( is_nearest_expiry && (HFSAT::BSESecurityDefinitions::GetExpiryFromShortCode(shortcode) ==
        HFSAT::DateTime::GetCurrentIsoDateLocal() || HFSAT::BSESecurityDefinitions::GetExpiryFromShortCode(shortcode) ==
        HFSAT::DateTime::GetCurrentIsoDateLocal())) {
      
        if (list_of_securities_under_physical_settlement_.end() !=
          list_of_securities_under_physical_settlement_.find(basename)) {
        DBGLOG_CLASS_FUNC_LINE_ERROR << "SHORTCODE : " << shortcode << " BASENAME : " << basename
                                     << " CANT BE TRADED TODAY AS IT IS UNDER PHYSICAL SETTLEMENT"
                                     << DBGLOG_ENDL_FLUSH;
        DBGLOG_DUMP;
        return;
      }
    }
  }

  // Unsolicited SMS scripts
  if ("NSE_EQ" == exchange_ || "NSE_FO" == exchange_ || "BSE_EQ" == exchange_ || "BSE_FO" == exchange_) {
    std::string shortcode = shortcode_temp;
    std::string basename = shortcode.substr(shortcode.find_first_of("_") + 1);
    basename = basename.substr(0, basename.find_first_of("_"));

    if (list_of_securities_with_unsolicited_sms_.end() !=
        list_of_securities_with_unsolicited_sms_.find(shortcode)) {
      DBGLOG_CLASS_FUNC_LINE_ERROR << "SHORTCODE : " << shortcode << " BASENAME : " << basename
                                   << " CANT BE TRADED TODAY AS IT IS UNDER UNSOLICITED SMS CIRCULATION "
                                   << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
      return;
    }
  }

  bool is_shortcode_present = HFSAT::SecurityDefinitions::CheckIfContractSpecExists(
      shortcode_temp, HFSAT::DateTime::GetCurrentIsoDateLocal());

  if (!is_shortcode_present) {
    DBGLOG_CLASS_FUNC_LINE_ERROR << "Shortcode in ADDTRADINGSYMBOL : " << shortcode_temp
                                 << " not present in ContractDefinition. Returning.." << DBGLOG_ENDL_FLUSH;
    return;
  }

  const char* this_exch_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(shortcode_temp);

  // Speacial Handling For NSE & BSE
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

        return;
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
                                        "INTERNAL SYMBOL WAS INVALID, SYMBOL : "
                                     << exch_symbol_1 << " DATA SYMBOL : " << data_symbol_1
                                     << " SYMBOL2 : " << exch_symbol_2 << " DATA SYMBOL : " << data_symbol_2;
        DBGLOG_DUMP;

        return;
      }

    } else {  // Outrights

      std::string data_symbol = nse_sec_def.ConvertExchSymboltoDataSourceName(exch_symbol_str);

      if (std::string("INVALID") == data_symbol) {
        DBGLOG_CLASS_FUNC_LINE_FATAL
            << "FAILED TO ADD SYMBOL STRING AS DATASYMBOL WAS INVALID, SYMBOL : " << exch_symbol_str
            << DBGLOG_ENDL_FLUSH;
        DBGLOG_DUMP;

        return;
      }
    }

    lotsize_factor = HFSAT::SecurityDefinitions::GetContractMinOrderSize(
        shortcode_temp.c_str(), HFSAT::DateTime::GetCurrentIsoDateLocal());
  }
  else if (strncmp(exch_symbol_str.substr(0, 3).c_str(), "BSE", 3) == 0) {
    HFSAT::BSESecurityDefinitions& bse_sec_def =
        HFSAT::BSESecurityDefinitions::GetUniqueInstance(HFSAT::DateTime::GetCurrentIsoDateLocal());

    // Spreads Handling
    if (bse_sec_def.IsSpreadShortcode(shortcode_temp)) {
      std::string spread_data_source_symbol =
          bse_sec_def.ConvertExchSymboltoDataSourceName(exch_symbol_str.c_str());

      if (std::string("INVALID") == spread_data_source_symbol) {
        DBGLOG_CLASS_FUNC_LINE_FATAL
            << "FAILED TO ADD SYMBOL STRING AS SPREAD DATASYMBOL WAS INVALID, SYMBOL : " << exch_symbol_str
            << DBGLOG_ENDL_FLUSH;
        DBGLOG_DUMP;

        return;
      }

      // NSE_NIFTY_FUT_20150129_20150226
      int pos = spread_data_source_symbol.rfind('_');

      std::string data_symbol_1 = spread_data_source_symbol.substr(0, pos);
      int basename_pos = data_symbol_1.rfind('_');
      std::string data_symbol_2 =
          data_symbol_1.substr(0, basename_pos + 1) + spread_data_source_symbol.substr(pos + 1, 8);

      std::string exch_symbol_1 = bse_sec_def.ConvertDataSourceNametoExchSymbol(data_symbol_1.c_str());
      std::string exch_symbol_2 = bse_sec_def.ConvertDataSourceNametoExchSymbol(data_symbol_2.c_str());

      if (std::string("INVALID") == exch_symbol_1 || std::string("INVALID") == exch_symbol_2) {
        DBGLOG_CLASS_FUNC_LINE_FATAL << "FAILED TO ADD SYMBOL STRING AS EXCHSYMBOL FOR ONE OF THE TWO/BOTH "
                                        "INTERNAL SYMBOL WAS INVALID, SYMBOL : "
                                     << exch_symbol_1 << " DATA SYMBOL : " << data_symbol_1
                                     << " SYMBOL2 : " << exch_symbol_2 << " DATA SYMBOL : " << data_symbol_2;
        DBGLOG_DUMP;

        return;
      }

    } else {  // Outrights

      std::string data_symbol = bse_sec_def.ConvertExchSymboltoDataSourceName(exch_symbol_str);

      if (std::string("INVALID") == data_symbol) {
        DBGLOG_CLASS_FUNC_LINE_FATAL
            << "FAILED TO ADD SYMBOL STRING AS DATASYMBOL WAS INVALID, SYMBOL : " << exch_symbol_str
            << DBGLOG_ENDL_FLUSH;
        DBGLOG_DUMP;

        return;
      }
    }

    lotsize_factor = HFSAT::SecurityDefinitions::GetContractMinOrderSize(
        shortcode_temp.c_str(), HFSAT::DateTime::GetCurrentIsoDateLocal());
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
    dbglogger_ << "AddString has failed for " << shortcode_temp << " or exchsymbol "
               << HFSAT::ExchangeSymbolManager::GetExchSymbol(shortcode_temp) << " that has not been added "
               << DBGLOG_ENDL_FLUSH;
    return;
  }

  else {
    if (!AddMifidLimits(shortcode_temp, t_security_id_)) return;

    SidToShortCodeMap()[t_security_id_] = shortcode_temp;
    HFSAT::ORSUtils::SecurityDBManager::GetUniqueInstance().SetSecurityShortcode(t_security_id_, shortcode_temp);
  }

  bool use_max_exposure_ = margin_checker_.getUseMaxExposure();

  std::cout << "USE MAX EXPOSURE " << use_max_exposure_ << std::endl;

  double check_ltp = HFSAT::BSESecurityDefinitions::GetLastClose(shortcode_temp); 

/*
    if(shortcode_temp == "NSE_AMBER"){
      std::cout << "CONSIDERED IPO FOR TESTING" << std::endl;
      check_ltp=-1;
    }
*/

  if(use_max_exposure_ == true && check_ltp!=-1){
    double ltp = check_ltp; 
    long long int sec_exposure = ltp*max_pos;

    std::cout << "SHORTCODE IS " << shortcode_temp << std::endl;
    std::cout << "LTP FROM  GetLastClose  " << check_ltp << std::endl;   

    // If LTP of the product is not mentioned in bhav copy file or product is ipo
    if(ltp==-1){
      //Check if it is an ipo
      
        std::ostringstream t_temp_oss;
        t_temp_oss << IPO_LIST << "ipo_products" << ".txt";

        std::ifstream ipo_products_list;
        ipo_products_list.open(t_temp_oss.str().c_str(), std::ifstream::in);

        #define MAX_LINE_READ_LENGTH 1024
        char line_buffer[MAX_LINE_READ_LENGTH];

        while (ipo_products_list.good()) {
            ipo_products_list.getline(line_buffer, MAX_LINE_READ_LENGTH);
            if (std::string(line_buffer).length() < 1) continue;
            //std::cout << "IPO are "<< line_buffer << std::endl;

            PerishableStringTokenizer st_(line_buffer,MAX_LINE_READ_LENGTH);
            const std::vector<const char *> &ipo_content_ = st_.GetTokens();

            //std::cout << "SEGREGATED IPO CONTENT IS "  << ipo_content_[0] << " " << ipo_content_[1] << " " << ipo_content_[2] << std::endl;
            //Check if shortcode is matched further check for ltp
            if(ipo_content_[1] == shortcode_temp){

              // Check for malformated date
              bool is_num = true;
              string ipo_price = ipo_content_[2];
              for (std::string::const_iterator iter = ipo_price.begin(); iter != ipo_price.end(); iter++) {
              if (!std::isdigit(*iter)) {
                  is_num = false;
                  break;
                }
              }
            if (!is_num) {
              std::cerr << "IPO Price " << ipo_content_[2] << " is not a number. Exiting.\n";
              max_pos = 10000;
              max_order_size = 10000;

              margin_checker_.Set(t_security_id_, max_pos * lotsize_factor, max_order_size * lotsize_factor,
                      max_pos * lotsize_factor, 0);
              return;
            }
            else {
              ltp = atoi(ipo_content_[2]);
              sec_exposure = ltp*max_pos;
              std::cout << "IPO LTP IS " << ltp << std::endl;
            }  
        } 
      }
      ipo_products_list.close();

      // If not an IPO but LTP is not not present for that security
      max_pos = 10000;
      max_order_size = 10000;
      margin_checker_.Set(t_security_id_, max_pos * lotsize_factor, max_order_size * lotsize_factor,
                      max_pos * lotsize_factor, 0);
      return;
    }

    long int max_exposure_limit_ = margin_checker_.getMaxExposure();
    std::cout << "Max Exposure Limit " << margin_checker_.getMaxExposure()  << std::endl;
    std::cout << "Security Exposure Req " << sec_exposure << std::endl;


    int requested_addts = max_pos;
    if(sec_exposure>max_exposure_limit_){

        max_pos = floor(double((max_exposure_limit_)/(ltp)));
        max_order_size = floor(double((max_exposure_limit_)/(ltp)));

        if(report_string.tellp() == std::streampos(0)){

            report_string <<  "SHORTCODE" << " " << "LTP" << " " << "REQUESTED ADDTS VALUE" << " " << "SET ADDTS VALUE" << ".\n";
            report_string << "\n" << shortcode_temp << " " << ltp << " " << max_pos << " " << max_pos << ".\n";

             dbglogger_ << shortcode_temp << " " <<  " REQUESTED ADDTS VALUE " <<  requested_addts  << " SET ADDTS VALUE "  << max_pos 
                       << DBGLOG_ENDL_FLUSH;

             dbglogger_ << shortcode_temp << " " <<  " SET MAX EXPOSURE TO  " <<  sec_exposure << " TO SET ADDTS VALUE TO  "  << requested_addts
                       << DBGLOG_ENDL_FLUSH;

        }
        else{
            report_string << "\n" <<shortcode_temp << " " << to_string(ltp) << " " << to_string(max_pos) << " " << to_string(max_pos) << ".\n";

            dbglogger_ << shortcode_temp << " " <<  " REQUESTED ADDTS VALUE " <<  requested_addts <<  " SET ADDTS VALUE "  << max_pos 
                       << DBGLOG_ENDL_FLUSH;

             dbglogger_ << shortcode_temp << " " <<  " SET MAX EXPOSURE TO  " <<  sec_exposure << " TO SET ADDTS VALUE TO  "  << requested_addts
                       << DBGLOG_ENDL_FLUSH;
        }
    }
    else{
        dbglogger_ << shortcode_temp << " " <<  " REQUESTED ADDTS VALUE " <<  requested_addts <<  " SET ADDTS VALUE "  << max_pos 
                       << DBGLOG_ENDL_FLUSH;
    }
  }  
  // We make sure t_security_id_ must be >=0 else margin_checker will segfualt trying to set negative
  // index for array

  cout << "FINAL POSITIONS " << max_pos << " " << max_order_size << " " << max_pos << std::endl;
  margin_checker_.Set(t_security_id_, max_pos * lotsize_factor, max_order_size * lotsize_factor,
                      max_pos * lotsize_factor, 0);
  m_athread->KillSwitchNewForSecId(t_security_id_);
}

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
    bzero(temp_command_buffer_, COMMAND_BUFFER_LEN);
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

/*
    int len = tokens_.size();
    std::cout << "Token size " << tokens_.size() << std::endl;
    std::cout << "INPUT ARGUMENTS IS " << std::endl;
    for(int i=0;i<len;i++){
      std::cout << tokens_[i] << std::endl;
    }
*/

    if ((tokens_.size() >= 2) && (strcmp(tokens_[0], "CONTROLCOMMAND") == 0)) {
      ControlCommand_t _this_control_command_ = StringToControlCommand(tokens_[1]);

      //std::cout << "Entered control command is " << tokens_[1] << std::endl;
      //std::cout << "Enum is " << _this_control_command_ << std::endl;

      switch (_this_control_command_) {
        case kGeneral: {
          DBGLOG_CLASS_FUNC_LINE << " General Control Command : " << temp_command_buffer_ << DBGLOG_ENDL_FLUSH;
          m_athread->ProcessGeneralControlCommand(temp_command_buffer_, COMMAND_BUFFER_LEN);
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

          
          std::cout << "Inside kAddTradingSymbol " << std::endl; 
          std::cout << "is_addts_running_ " << *is_addts_running_ << std::endl;
          std::ostringstream report_string;
          
          //report_string << "";

          if(*is_addts_running_){
	          dbglogger_ << "ADDTS FOR SHORTCODE: " << tokens_[2] << " FAILED. FILE ADDTS IS RUNNING CURRENTLY"
		        << DBGLOG_ENDL_FLUSH;
	          break;
	        }
          if (tokens_.size() < 7) {
            dbglogger_ << "Improper Syntax. Expecting CONTROLCOMMAND ADDTRADINGSYMBOL <SHORTCODE> <MAXPOS> "
                          "<MAXORDERSIZE> <MAXWORSTCASEPOS> <MAXLIVEORDERS>"
                       << DBGLOG_ENDL_FLUSH;
            break;
          }
          
 	        *is_addts_running_ = true;

          std::string shortcode_temp(tokens_[2]);
          int max_pos = std::atoi(tokens_[3]);
          int max_order_size = std::atoi(tokens_[4]);

          //std::cout << shortcode_temp << " " << max_pos << " " << max_order_size << std::endl;
          AddtradingSymbol(shortcode_temp, max_pos, max_order_size,report_string);

 	        *is_addts_running_ = false;
          //EmailADDTSCapped(report_string.str());

        } break;
  
        case kLoadTradingSymbolFile:{

          std::cout << "Inside kAddTradingSymbolFile" << std::endl; 
          std::ostringstream report_string;
          //report_string << "";
      

          if(*is_addts_running_){
	     dbglogger_ << "FILE ADDTS for " << tokens_[2] << " FAILED. ANOTHER ADDTS IS RUNNING CURRENTLY"
			<< DBGLOG_ENDL_FLUSH;
 	     break;
	  }
          if(3 != tokens_.size()){
	    dbglogger_ << "INVALID SYNTAX, EXPECTING CONTROLCOMMAND LOADTRADINGSYMBOLFILE <FILENAME> ";
	    dbglogger_.DumpCurrentBuffer();
            break;
	  }
 	  *is_addts_running_ = true;
          std::ifstream addts_file_(tokens_[2], std::ifstream::in);
          if(addts_file_.is_open()){
            const int kBufferSize = 1024;
            char readline_buffer_[kBufferSize];
	    while (addts_file_.good()){
              bzero(readline_buffer_, kBufferSize);
              addts_file_.getline(readline_buffer_, kBufferSize);
              if(strlen(readline_buffer_)>0){
		dbglogger_ << readline_buffer_ << DBGLOG_ENDL_FLUSH ;
	        PerishableStringTokenizer st_(readline_buffer_, kBufferSize);
                const std::vector<const char *> &addts_content_ = st_.GetTokens();
                if (addts_content_.size() < 8 ) {
                  dbglogger_ << "Improper Syntax. Expecting FILE ENTRY <EXCH_NAME> <PROFILE> ADDTRADINGSYMBOL <SHORTCODE> <MAXPOS> "
                                "<MAXORDERSIZE> <MAXWORSTCASEPOS> <MAXLIVEORDERS>"
                             << DBGLOG_ENDL_FLUSH;
                  continue;
                }

                std::string shortcode_temp(addts_content_[3]);
                int max_pos = std::atoi(addts_content_[4]);
                int max_order_size = std::atoi(addts_content_[5]);

                AddtradingSymbol(shortcode_temp, max_pos, max_order_size,report_string);
              }
	    }
	  }
	  addts_file_.close();
          *is_addts_running_ = false;

        //EmailADDTSCapped(report_string.str());

	} break;


  case kResetExposure: {

    //std::cout << "Inside kResetExposure " << tokens_[2] << std::endl;
    long int set_exposure_ = atoi(tokens_[2]);
    std::cout << "SET EXPOSURE VALUE IS " << set_exposure_ << std::endl;
    dbglogger_ << "SET EXPOSURE VALUE IS " << set_exposure_
                            << DBGLOG_ENDL_FLUSH;
    margin_checker_.setMaxExposure(set_exposure_);

    std::cout << "Max Exposure set to " << margin_checker_.getMaxExposure() << std::endl; 

  } break;

  case kSetSecurityExposure: {
    
    //std::cout << "Inside kSetSecurityExposure" << std::endl; 
    std::ostringstream report_string;

    //report_string << "";
    std::string shortcode_temp = tokens_[2];
    long int set_exposure_ = atoi(tokens_[3]);

    //std::cout << "Security name is " << shortcode_temp << std::endl;
    //std::cout << "SET EXPOSURE VALUE IS " << set_exposure_ << std::endl;
    margin_checker_.setMaxExposure(set_exposure_);

    double ltp = HFSAT::BSESecurityDefinitions::GetLastClose(shortcode_temp);

    int max_pos = floor(double((set_exposure_)/(ltp)));
    AddtradingSymbol(shortcode_temp,max_pos,max_pos,report_string);

  } break;

  case kSecurityExposureFile:{

    //std::cout << "Setting using set exposure File " << std::endl;
    std::ostringstream report_string;
 
    report_string << "";
  
    std::ifstream addts_file_(tokens_[2], std::ifstream::in);
    if(addts_file_.is_open()){
     std::cout << "Exposure File opened " << std::endl;
      const int kBufferSize = 1024;
      char readline_buffer_[kBufferSize];
      while (addts_file_.good()){
       std::cout << "Reading Line " << std::endl;
       bzero(readline_buffer_, kBufferSize);
       addts_file_.getline(readline_buffer_, kBufferSize);
       if(strlen(readline_buffer_)>0){
         dbglogger_ << readline_buffer_ << DBGLOG_ENDL_FLUSH ;
         PerishableStringTokenizer st_(readline_buffer_, kBufferSize);
         const std::vector<const char *> &addts_content_ = st_.GetTokens();
         if (addts_content_.size() < 2 ) {
           dbglogger_ << "Improper Syntax. Expecting <SHORTCODE> <EXPOSURE_VALUE>"
                            << DBGLOG_ENDL_FLUSH;
           continue;
         }
 
         std::string shortcode_temp(addts_content_[0]);
         long int set_exposure_ = atoi(addts_content_[1]);
 
         //std::cout << "Security name is " << shortcode_temp << std::endl;
         //std::cout << "SET EXPOSURE VALUE IS " << set_exposure_ << std::endl;
         margin_checker_.setMaxExposure(set_exposure_);
 
         double ltp = HFSAT::BSESecurityDefinitions::GetLastClose(shortcode_temp);
 
         int max_pos = floor(double((set_exposure_)/(ltp)));
         AddtradingSymbol(shortcode_temp,max_pos,max_pos,report_string);
 
       }
     }
  }else{
     std::cout << "File can't be opened " << std::endl;
     dbglogger_ << "FILE CAN'T BE OPENED " 
                            << DBGLOG_ENDL_FLUSH;

 }

  //EmailADDTSCapped(report_string.str());
  } break;

  case kUseSetExposure:{

      //std::cout << "Inside kUseSetExposure "<< std::endl;
      //std::cout << "Setting using set exposure " << std::endl;

      string use_set_exposure = tokens_[2];

      if(use_set_exposure == "Y" || use_set_exposure == "y"){
        std::cout << "Setting Exposure vale to true " << std::endl;
        margin_checker_.setUseMaxExposure(true);
        dbglogger_ << "USE MAX EXPOSURE TRUE"    
                            << DBGLOG_ENDL_FLUSH;
      }
      else if(use_set_exposure == "N" || use_set_exposure == "n"){
        std::cout << "Setting Exposure vale to false " << std::endl;
        margin_checker_.setUseMaxExposure(false);
        dbglogger_ << "USE MAX EXPOSURE FALSE"    
                            << DBGLOG_ENDL_FLUSH;
      }
      else{
        dbglogger_ << "INVALID USE_SET_EXPOSURE PROVIDED " << "\n";
        dbglogger_ << "USE Y TO SET OR N TO UNSET " << "\n";
        dbglogger_.DumpCurrentBuffer();
      }

  } break;

  case kDumpORSPositions: {
          dbglogger_ << "OM\n" << order_manager_.DumpOMState() << "PM\n" << position_manager_.DumpPMState() << "\n";
          dbglogger_.DumpCurrentBuffer();

  } break;

        case kDumpAllORSPositions: {
          dbglogger_ << "OM\n" << order_manager_.DumpOMState(true) << "PM\n" << position_manager_.DumpPMState() << "\n";
          dbglogger_.DumpCurrentBuffer();

        } break;

        case kCancelLiveOrders: {
          dbglogger_ << "CANCELING LIVE ORDERS\n";
          dbglogger_.DumpCurrentBuffer();
          cancel_live_order_flag_ = true;
        } break;

        case kAddExternalPositions: {
          if (6 != tokens_.size()) {
            dbglogger_ << "INVALID SYNTAX, EXPECTING CONTROLCOMMAND ADDEXTERNALPOSITIONS <SECURITYNAME> <SACI> "
                          "<POSITION> <BUY/SELL>"
                       << DBGLOG_ENDL_FLUSH;
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
            position_manager_.AddBuyTrade(security_id, atoi(tokens_[3]), atoi(tokens_[4]));

          } else if ("SELL" == std::string(tokens_[5])) {
            position_manager_.AddSellTrade(security_id, atoi(tokens_[3]), atoi(tokens_[4]));

          } else {
            dbglogger_ << "ADDEXTERNALPOSITIONS CALLED FOR INVALID TRADE-SIDE : " << tokens_[5] << "\n";
            dbglogger_.DumpCurrentBuffer();
          }

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
            int32_t curr_ors_pnl =  HFSAT::ORSUtils::ORSPnlManager::GetUniqueInstance(dbglogger_).GetCurrentORSPnl();
            
              HFSAT::Email e;
              std::string exchange = (std::string(HFSAT::GetCurrentHostName()).find("sdv-indb-") != std::string::npos) ? "BSE" : "NSE";
              std::string subject = exchange + " PNL UPDATE REQUEST " + std::string(HFSAT::GetCurrentHostName());

              e.setSubject(subject);
              e.addRecepient("nseall@tworoads.co.in");
              e.addSender("ravi.parikh@tworoads.co.in");
              e.content_stream << "host_machine: " << HFSAT::GetCurrentHostName() << "<br/>";
              e.content_stream <<  exchange << " PNL REQUESTED VALUE : " << (double)ors_stop_pnl / LAKH << " Lakhs"  << "<br/>";
              e.content_stream << "ORS PNL CURRENT VALUE : " << (double)curr_ors_pnl / LAKH << " Lakhs"  << "<br/>";
              e.sendMail();            

          } else {
            DBGLOG_CLASS_FUNC_LINE_ERROR << "INVALID CONTROL COMMAND. USAGE : SETORSPNLCHECK <PNL_VALUE>";
            DBGLOG_DUMP;
          }

        } break;

        case kSetORSGrossMarginCheck: {
          if (3 == tokens_.size()) {
            double gross_margin = atof(tokens_[2]);
            HFSAT::ORSUtils::ORSMarginManager::GetUniqueInstance(dbglogger_).SetGrossMarginCheck(gross_margin);
            double gross_margin_used = HFSAT::ORSUtils::ORSMarginManager::GetUniqueInstance(dbglogger_).GetGrossMarginUsed();

              HFSAT::Email e;
              std::string exchange = (std::string(HFSAT::GetCurrentHostName()).find("sdv-indb-") != std::string::npos) ? "BSE" : "NSE";
              std::string subject = exchange + " GROSS MARGIN UPDATE REQUEST " + std::string(HFSAT::GetCurrentHostName());
              e.setSubject(subject);
              e.addRecepient("nseall@tworoads.co.in");
              e.addSender("ravi.parikh@tworoads.co.in");
              e.content_stream << "host_machine: " << HFSAT::GetCurrentHostName() << "<br/>";
              e.content_stream << exchange << " GROSS MARGIN REQUESTED VALUE : " << gross_margin / LAKH << " Lakhs"  << "<br/>";
              e.content_stream << "GROSS MARGIN Used : " << gross_margin_used / LAKH << " Lakhs"  << "<br/>";
              e.sendMail();
            

          } else {
            DBGLOG_CLASS_FUNC_LINE_ERROR << "INVALID CONTROL COMMAND. USAGE : SETGROSSMARGINCHECK <GROSS_MARGIN_VALUE>";
            DBGLOG_DUMP;
          }

        } break;

        case kSetORSNetMarginCheck: {
          if (3 == tokens_.size()) {
            double net_margin = atof(tokens_[2]);
            HFSAT::ORSUtils::ORSMarginManager::GetUniqueInstance(dbglogger_).SetNetMarginCheck(net_margin);
            double net_margin_used = HFSAT::ORSUtils::ORSMarginManager::GetUniqueInstance(dbglogger_).GetNetMarginUsed();

              HFSAT::Email e;
              std::string exchange = (std::string(HFSAT::GetCurrentHostName()).find("sdv-indb-") != std::string::npos) ? "BSE" : "NSE";
              std::string subject = exchange + " NET MARGIN UPDATE REQUEST " + std::string(HFSAT::GetCurrentHostName());
              e.setSubject(subject);
              e.addRecepient("nseall@tworoads.co.in");
              e.addSender("ravi.parikh@tworoads.co.in");
              e.content_stream << "host_machine: " << HFSAT::GetCurrentHostName() << "<br/>";
              e.content_stream << exchange << " NET MARGIN REQUESTED VALUE : " << net_margin / LAKH << " Lakhs" << "<br/>";
              e.content_stream << "NET MARGIN Used : " <<  net_margin_used / LAKH << " Lakhs" << "<br/>";
              e.sendMail();
            

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
	case kClearPositionManager: {
	  if (2 == tokens_.size()) {
             position_manager_.ClearPositions();
             DBGLOG_CLASS_FUNC_LINE_ERROR << "CONTROL COMMAND : CLEARPOSITIONS";
             DBGLOG_DUMP;	     

              HFSAT::Email e;
              std::string exchange = (std::string(HFSAT::GetCurrentHostName()).find("sdv-indb-") != std::string::npos) ? "BSE" : "NSE";
              std::string subject = exchange + " ORS CLEAR POSITIONS CALLED " + std::string(HFSAT::GetCurrentHostName());
              e.setSubject(subject);
               e.addRecepient(
                   "nseall@tworoads.co.in, "
                   "joseph.padiyara@tworoads-trading.co.in, uttkarsh.sarraf@tworoads.co.in, "
                   "ravi.parikh@tworoads-trading.co.in, "
                   "rakesh.kumar@tworoads.co.in, nishit.bhandari@tworoads.co.in, nsehft@tworoads.co.in, "
                   "nsemidterm@tworoads.co.in");
               e.addSender("ravi.parikh@tworoads.co.in");
               e.sendMail();
            

	  } else {
	    DBGLOG_CLASS_FUNC_LINE_ERROR << "INVALID CONTROL COMMAND. USAGE :  CLEARPOSITIONMANAGER";
	    DBGLOG_DUMP;
	  } 
	} break;
        case kenableOrsOrders:{
              dbglogger_ << "Enabling Orders For ORS.. " << "\n";
              margin_checker_.setOrsOrderCheck(true);
              dbglogger_.DumpCurrentBuffer();
              SendEmail("Alert, ORS Enabled. Allowing Orders", "Disable By /home/pengine/prod/live_execs/ors_control_exec 21240 DISABLEORSORDERS ");
         } break;
        case kdisableOrsOrders:{
              dbglogger_ << "Disabling Orders For ORS.. "<< "\n";
              margin_checker_.setOrsOrderCheck(false);
              dbglogger_.DumpCurrentBuffer();
              SendEmail("Alert, ORS Disabling. Not Allowing Orders", "Enable By /home/pengine/prod/live_execs/ors_control_exec 21240 ENABLEORSORDERS ");
              
         } break;
         case kconveystratmargin:{
          if (3 == tokens_.size()) {
            dbglogger_ << "FIRM LEVEL MARGIN "<<tokens_[2]<<"\n"; 
            m_athread->SetMarginValue(stod(tokens_[2]));
          }
         }break;
	      case kkillSwitch:{
          if (tokens_.size() == 2){
              dbglogger_ << "Kill Switch for ORS "<< "\n";
              dbglogger_ << "Cancelling all the open pending orders " << "\n";
              m_athread->KillSwitch(-1);
          } else if (tokens_.size() > 2) {
            for (unsigned int i=2;i < tokens_.size(); i++)
            {
              dbglogger_ << "Kill Switch for  "<<tokens_[i]<<"\n";
              const char* this_exch_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(tokens_[i]);
              std::string exch_symbol_str = this_exch_symbol_;
              int t_security_id_ = simple_security_symbol_indexer_.GetIdFromSecname(this_exch_symbol_);
              if (t_security_id_ < 0) {
                  dbglogger_ << "killswitch has failed for " << tokens_[i] << " or exchsymbol "
                  << HFSAT::ExchangeSymbolManager::GetExchSymbol(tokens_[i]) << " that has not been added "
                  << DBGLOG_ENDL_FLUSH;
              }else {
                m_athread->KillSwitch(t_security_id_);
              }

          }
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

void ControlThread::SendEmail(std::string subject, std::string body_){
  char hostname[128];
  hostname[127] = '\0';
  gethostname(hostname, 127);
  std::string alert_message = std::string(body_) + " " + std::string(hostname) + "\n";
  HFSAT::SendAlert::sendAlert(alert_message);
  HFSAT::Email e;
  e.setSubject(subject);
  e.addRecepient("ravi.parikh@tworoads.co.in, nseall@tworoads.co.in");
  e.addSender("ravi.parikh@tworoads.co.in");
  e.content_stream << body_ << "<br/>";
  e.sendMail();
}

void ControlThread::EmailAddSymbolFailed(std::string alert_body_) {
  char hostname[128];
  hostname[127] = '\0';
  gethostname(hostname, 127);
  std::string alert_message = std::string("ALERT: ADDTRADINGSYMBOL Warning at ") + std::string(hostname) + "\n";
  // HFSAT::SendAlert::sendAlert(alert_message); problem in case local server line down

  HFSAT::Email e;
  std::string exchange = (std::string(HFSAT::GetCurrentHostName()).find("sdv-indb-") != std::string::npos) ? "BSE" : "NSE";
  std::string subject = exchange + " ADDTRADINGSYMBOL - Warning " + std::string(HFSAT::GetCurrentHostName());
  e.addRecepient("ravi.parikh@tworoads.co.in, nseall@tworoads.co.in");
  e.addSender("subham.chawda@tworoads-trading.co.in");
  e.content_stream << alert_body_ << "<br/>";
  e.sendMail();
}

void ControlThread:: EmailADDTSCapped(std::string report_string) {
      // MAKE CHANGES HERE
    if(0 != report_string.size()){

        if (std::string::npos != HFSAT::GetCurrentHostName().find("sdv-ind-")) {
              HFSAT::Email e;
              e.setSubject("MAX ADDTS VALUE EXCEEDED MAX_EXPOSURE CAPPED NOW");
              e.addRecepient("tarun.joshi@tworoads-trading.co.in");
              e.addSender("tarun.joshi@tworoads-trading.co.in");
              e.content_stream << "host_machine: " << HFSAT::GetCurrentHostName() << "<br/>";
              e.content_stream << report_string << "<br/>";
              e.sendMail();
            }
      }
      else{
        std::cout << "No shortcode ADDTS exceeded than max_exposure " << std::endl;
      }
      return;
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

  if ((std::string("NSE_FO") == exchange_) || (std::string("BSE_FO") == exchange_)) {
    if (max_pos_ <= MAX_ACCEPTABLE_POS && max_ord_size_ <= MAX_ACCEPTABLE_ORDER_SIZE) {
      return true;
    }
  }

  // For Cash There Aren't Any Hard Limits
  if ((std::string("NSE_EQ") == exchange_) || (std::string("BSE_EQ") == exchange_)) {
    return true;
  }

  if ((std::string("NSE_CD") == exchange_) || (std::string("BSE_CD") == exchange_)) {
    if (max_pos_ <= MAX_ACCEPTABLE_POS && max_ord_size_ <= MAX_ACCEPTABLE_ORDER_SIZE) {
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
    //EmailAddSymbolFailed(this_mail_msg_.str());

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
    //EmailAddSymbolFailed(this_mail_msg_.str());

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
    //EmailAddSymbolFailed(this_mail_msg_.str());

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
