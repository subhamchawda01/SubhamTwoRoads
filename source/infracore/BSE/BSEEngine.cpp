// =====================================================================================
//
//       Filename:  BSEEngine.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  11/06/2012 09:14:46 AM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
//
//        Address:  Suite No 162, Evoma, #14, Bhattarhalli,
//                  Old Madras Road, Near Garden City College,
//                  KR Puram, Bangalore 560049, India
//          Phone:  +91 80 4190 3551
//
// =====================================================================================

#include <regex>
#include "dvccode/CDef/security_definitions.hpp"
#include "infracore/BSE/BSEEngine.hpp"


#define BSE_REFERENCE_DATA_FILE "/spare/local/files/EUREX/eobi-prod-codes.txt"
#define BSE_DUMMY_ORDER_PRICE_VALUE 100.00
#define BSE_DUMMY_ORDER_SIZE_VALUE 2
#define GET_FLAG_MASK 0xC0000000
#define BSE_DEBUG_INFO 0

namespace HFSAT {
namespace BSE {

BSEEngine::BSEEngine(HFSAT::ORS::Settings &settings, HFSAT::DebugLogger &dbglogger, std::string output_log_dir,
                     int32_t engine_id, AsyncWriter *pWriter, AsyncWriter *pReader)
    : BaseEngine(settings, dbglogger),
      dbglogger_(dbglogger),
      container_(),
      keep_engine_running_(false),
      keep_trying_different_gateways_(true),
      is_connected_(false),
      is_logged_in_(false),
      is_mkt_order_(false),
      next_seq_num_(2),
      dummy_order_(),
      heartbeat_interval_(0),
      is_read_socket_open_(false),
      is_secure_box_registered_(false),
      bse_connection_gateway_socket_(nullptr),
      open_ssl_crypto_(),
      bse_gateway_ip_list_(),
      bse_gateway_ip_("10.255.255.6"),
      bse_gateway_port_(12908),
      tcp_direct_read_buffer_(NULL),
      tcp_direct_read_length_(-1),
      was_tcp_direct_read_successful_(false),
      tap_gateway_(),
      tap_interface_(),
      tap_ip_(),
      username_(-1),
      session_id_(-1),
      sender_location_id_(-1),
      seqno_to_saos_ordertype_(),
      saos_to_symbol_(),
      bse_pre_computed_message_lengths_(),
      bse_connection_gateway_request_(),
      bse_session_registration_request_(),
      bse_session_registration_response_(),
      bse_session_logon_request_(),
      bse_session_logout_request_(),
      bse_user_logon_request_(),
      bse_user_logout_request_(),
      bse_new_order_single_short_(),
      bse_new_order_single_(),
      bse_cancel_order_single_(),
      bse_modify_order_single_short_(),
      bse_sec_def_(HFSAT::BSESecurityDefinitions::GetUniqueInstance(HFSAT::DateTime::GetCurrentIsoDateLocal())),
      bse_daily_token_symbol_handler_(
          HFSAT::Utils::BSEDailyTokenSymbolHandler::GetUniqueInstance(HFSAT::DateTime::GetCurrentIsoDateLocal())),
      ref_data_loader(HFSAT::Utils::BSERefDataLoader::GetUniqueInstance(HFSAT::DateTime::GetCurrentIsoDateLocal())),
      setting_(settings),
      bse_segment_type_('I'),
      bse_session_logon_request_primary_ip_address_(""),
      bse_session_logon_request_primary_port_(-1),
      bse_session_logon_request_secondary_ip_address_(""),
      bse_session_logon_request_secondary_port_(-1),
      send_seq_num_lock_(),
      read_offset_(-1),
      order_manager_(HFSAT::ORS::OrderManager::GetUniqueInstance()),
      fast_px_convertor_vec_(DEF_MAX_SEC_ID, NULL),
      last_send_time_(),
      is_alert_batch_cancellation(true),
      id_(engine_id),
      is_risk_reduction_set(false),
      tcp_direct_client_zocket_(pWriter, pReader,
                                output_log_dir + "/audit." + settings.getValue("UserName") + "." + HFSAT::DateTime::GetCurrentIsoDateLocalAsString(),
                                settings.getValue("TAPInterface"),HFSAT::EXCHANGE_KEYS::kExchSourceNSEStr) 
{
  if (!settings.has("ExchangeGatewayIP") || !settings.has("ExchangeGatewayPort") || !settings.has("UserName") ||
      !settings.has("SessionID") || !settings.has("Password") || !settings.has("SessionPassword") ||
      !settings.has("PasswordChangeDate") || !settings.has("SessionPasswordChangeDate") ||
      !settings.has("HeartbeatIntervalInSeconds") || !settings.has("AlgoId") || !settings.has("LocationId") ||
      !settings.has("ClientCode")) {
    dbglogger_ << "Config file doesnot have ExchangeGatewayIP, ExchangeGatewayPort, UserID, Password, SessionID, HeartbeatIntervalInSeconds,"
                  "Password, SessionPassword, PasswordChangeDate, SessionPasswordChangeDate, AlgoId, LocationId, ClientCode\n";
    dbglogger_.CheckToFlushBuffer();
    dbglogger_.Close();
    exit(EXIT_FAILURE);
  }

  if (std::string("BSE_FO") != std::string(settings.getValue("Exchange")) &&
      std::string("BSE_CD") != std::string(settings.getValue("Exchange")) &&
      std::string("BSE_EQ") != std::string(settings.getValue("Exchange"))) {
    DBGLOG_CLASS_FUNC_LINE_FATAL << "EXCHANGE HAS TO BE EITHER BSE_CD , BSE_FO or BSE_EQ, WHAT WE HAVE IN CONFIG IS : "
                                 << settings.getValue("Exchange") << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;

    exit(-1);
  }

  if (std::string(settings.getValue("Exchange")) == std::string("BSE_FO")) {
    bse_segment_type_ = BSE_FO_SEGMENT_MARKING;
  } else if (std::string(settings.getValue("Exchange")) == std::string("BSE_CD")) {
    bse_segment_type_ = BSE_CD_SEGMENT_MARKING;
  } else if (std::string(settings.getValue("Exchange")) == std::string("BSE_EQ")) {
    bse_segment_type_ = BSE_EQ_SEGMENT_MARKING;
  }

  send_write_length = bse_new_order_single_short_.getBSENewOrderSingleShortMsgLength();
  send_mkt_write_length = bse_new_order_single_.getBSENewOrderSingleMsgLength();
  cancel_write_length = bse_cancel_order_single_.getBSECancelOrderSingleMsgLength();
  modify_write_length = bse_modify_order_single_short_.getBSEModifyOrderSingleShortMsgLength();
  
  memset(bse_msg_char_buf_, 0, MAX_BSE_RESPONSE_BUFFER_SIZE);
  memset(user_password_, 0, LEN_PASSWORD);
  memset(session_password_, 0, LEN_PASSWORD);
  memset(client_code_, 0, LEN_FREE_TEXT1);
  memset(cp_code_, 0, LEN_CP_CODE);
#if BSE_DEBUG_INFO
  dbglogger_ << " AlgoId : " << algo_id_ << "\n";
#endif

  // get the main gateway socket connection details
  bse_gateway_ip_ = settings.getValue("ExchangeGatewayIP");
  bse_gateway_port_ = settings.getIntValue("ExchangeGatewayPort", -1);

  // Get the valid list of gateway IPs in a string vector
  // IP list  to be input as IP1~IP2~IP3
  std::string ip_list = settings.getValue("SocketConnectHost");
  std::replace(ip_list.begin(), ip_list.end(), '~', ' ');
  HFSAT::PerishableStringTokenizer ips(const_cast<char *>((ip_list).c_str()), 1024);
  const std::vector<const char *> ip_vec = ips.GetTokens();
  for (auto ip : ip_vec) {
    bse_gateway_ip_list_.push_back(ip);
  }

  tap_gateway_ = settings.getValue("TAPGateway");
  tap_interface_ = settings.getValue("TAPInterface");
  tap_ip_ = settings.getValue("TAPIP");

  username_ = settings.getUIntValue("UserName", 0);
  session_id_ = settings.getUIntValue("SessionID", 0);
  sender_location_id_ = settings.getUIntValue("LocationId", 0);
  strcpy(user_password_, settings.getValue("Password").c_str());
  strcpy(session_password_, settings.getValue("SessionPassword").c_str());
  heartbeat_interval_ = settings.getIntValue("HeartbeatIntervalInSeconds", 30) * 1000; // milliseconds
  algo_id_ = settings.getValue("AlgoId");
  strcpy(client_code_, settings.getValue("ClientCode").c_str());
//  strcpy(cp_code_, settings.getValue("CPCode").c_str());
  last_user_pw_change_date = settings.getIntValue("PasswordChangeDate", -1);

  // fill up the connection gateway fields
  bse_connection_gateway_request_.setBSEConnectionGatewayRequestStaticFields(session_id_, session_password_,
                                                                             LEN_PASSWORD);

  // fill up the bse session logon config fields
  bse_session_logon_request_.setBSESessionLogonStaticFields(heartbeat_interval_, session_id_, session_password_,
                                                            LEN_PASSWORD);

  bse_session_registration_request_.setBSESessionLogonStaticFields(session_id_);
  heartbeat_interval_ = (heartbeat_interval_ / 1000);

  // fill up the bse user logon config fields
  bse_user_logon_request_.setBSEUserLogonStaticFields(username_, user_password_, LEN_PASSWORD);

#if BSE_DEBUG_INFO
  dbglogger_ << " UserName : " << username_ << " Password : " << user_password_ << "\n"
             << " SessionID : " << session_id_ << " Password : " << session_password_ << "\n"
             << " LocationId : " << sender_location_id_ << "\n"
             << " AlgoId : " << algo_id_ << "\n"
             << " AlgoId : " << settings.getValue("AlgoId") << "\n"
             << " AlgoId : " << settings.getValue("AlgoId").c_str() << "\n"
             << " ClientCode : " << client_code_ << "\n"
             << " CPCode : " << cp_code_ << "\n";
#endif

  // fill up the bse user logout config fields
  bse_user_logout_request_.setBSEUserLogoutStaticFields(username_);

  // fill up the username for new order request
  bse_new_order_single_short_.setBSENewOrderSingleShortStaticFields(username_, sender_location_id_, 
                                                                    algo_id_.c_str(), client_code_, cp_code_);

  // fill up the username for new market order request
  bse_new_order_single_.setBSENewOrderSingleStaticFields(username_, sender_location_id_, 
                                                         algo_id_.c_str(), client_code_, cp_code_);
  
  // fill up the username and session id for cancel request
  bse_cancel_order_single_.setBSECancelOrderSingleStaticFields(session_id_, username_, algo_id_.c_str());
  
  // fill up the username for modify request
  bse_modify_order_single_short_.setBSEModifyOrderSingleShortStaticFields(username_, sender_location_id_, 
                                                                          algo_id_.c_str(), client_code_, cp_code_);

  read_offset_ = 0;
 
  std::string openssl_certificate_file="";
  if (std::string(settings.getValue("Exchange")) == std::string("BSE_FO")){
    openssl_certificate_file="/spare/local/tradeinfo/BSE_Files/ssl_certificates/fo_certificate_file";
  }else if (std::string("BSE_EQ") == std::string(settings.getValue("Exchange"))){
    openssl_certificate_file="/spare/local/tradeinfo/BSE_Files/ssl_certificates/cm_certificate_file";
  }

  bse_connection_gateway_socket_ = new HFSAT::Utils::OpenSSLTLSClientSocket(openssl_certificate_file);
/*
#define BSE_REF_DATA_BUFFER_LENGTH_ 1024

  std::string bse_reference_data_filename_ = BSE_REFERENCE_DATA_FILE;

  std::ifstream bse_referencedata_file_;

  bse_referencedata_file_.open(bse_reference_data_filename_.c_str());

  if (!bse_referencedata_file_.is_open()) {
    dbglogger_ << " Fatal Error : Could Not Load Reference Data, Expecting At : " << bse_reference_data_filename_
               << "\n";
    dbglogger_.DumpCurrentBuffer();
  }

  char bse_ref_data_buffer_[BSE_REF_DATA_BUFFER_LENGTH_];

  while (bse_referencedata_file_.good()) {
    memset(bse_ref_data_buffer_, 0, BSE_REF_DATA_BUFFER_LENGTH_);

    bse_referencedata_file_.getline(bse_ref_data_buffer_, BSE_REF_DATA_BUFFER_LENGTH_);

    std::string this_ref_file_line_ = bse_ref_data_buffer_;

    if (this_ref_file_line_.find("#") != std::string::npos) continue;  // comments etc

    HFSAT::PerishableStringTokenizer st_(bse_ref_data_buffer_, sizeof(bse_ref_data_buffer_));
    const std::vector<const char *> &tokens_ = st_.GetTokens();
    if (tokens_.size() < 4) {
      continue;
    }

    std::string this_exchange_symbol_ = tokens_[1];
    int this_market_segment_id_ = atoi(tokens_[2]);
    int this_security_id_ = atoi(tokens_[0]);

    dbglogger_ << " Exchange Symbol : " << this_exchange_symbol_ << " Segment Id : " << this_market_segment_id_
               << " Security Id : " << this_security_id_ << "\n";

    ReferenceData this_reference_data_;

    this_reference_data_.market_segment_id_ = this_market_segment_id_;
    this_reference_data_.security_id_ = this_security_id_;

    exchange_symbol_to_reference_data_map_[this_exchange_symbol_] = this_reference_data_;

    security_id_to_exchange_symbol_map_[this_security_id_] = this_exchange_symbol_;
  }

  dbglogger_.DumpCurrentBuffer();

#undef BSE_REF_DATA_BUFFER_LENGTH_

  HFSAT::ShortcodeContractSpecificationMap &contract_specification_map =
      HFSAT::SecurityDefinitions::GetUniqueInstance(HFSAT::DateTime::GetCurrentIsoDateLocal())
          .contract_specification_map_;
  HFSAT::ShortcodeContractSpecificationMapCIter_t contract_specification_map_itr = contract_specification_map.begin();
  std::ostringstream t_temp_oss;
  bool shouldExit_all_refCheck = false;
  bool shortcode_alert_ = false;

  for (; contract_specification_map_itr != contract_specification_map.end(); contract_specification_map_itr++) {
    if (HFSAT::kExchSourceEUREX != (contract_specification_map_itr->second).exch_source_) continue;

    string shortcode = contract_specification_map_itr->first;
    int shortcode_length = shortcode.length();

    if (shortcode[shortcode_length - 3] == 'Y' && shortcode[shortcode_length - 2] == '_') continue;

    const char *secdef_exchange_symbol =
        HFSAT::ExchangeSymbolManager::GetExchSymbol(contract_specification_map_itr->first);

    if (exchange_symbol_to_reference_data_map_.find(secdef_exchange_symbol) ==
        exchange_symbol_to_reference_data_map_.end()) {
      shortcode_alert_ = true;
      bool shouldExit = ExitOnRefError(shortcode_alert_, contract_specification_map_itr->first);
      if (shouldExit) shouldExit_all_refCheck = true;  // if needs exit even for one failed shc, do exit
      if (shortcode_alert_) {
        t_temp_oss << "FAILED TO RETRIEVE REFERENCE DATA FOR : " << secdef_exchange_symbol
                   << " FROM : " << BSE_REFERENCE_DATA_FILE << " IsExiting :" << shouldExit << "\n";
      }
    }
  }
  if (shortcode_alert_)
    if (shouldExit_all_refCheck) exit(-1);

  for (unsigned int saos_entry_counter = 0; saos_entry_counter < MAX_SAOS_TO_METADATA_ENTRIES_LIMIT;
       saos_entry_counter++) {
    SaosPriceStruct *new_saos_metadata = new SaosPriceStruct();
    new_saos_metadata->Initialize();
    saos_to_metadata_.push_back(new_saos_metadata);
  }

  HFSAT::SimpleSecuritySymbolIndexer::GetUniqueInstance().AddSSSIListener(this);
*/
}

void BSEEngine::CleanUp() {
  // Ensure the SAOS and BranchCode/Sequence Are Always in sync
  HFSAT::ORS::SequenceGenerator::GetUniqueInstance().persist_seq_num();
} 

BSEEngine::~BSEEngine() {
  //keep_engine_running_ = false;

  // Deallocate fast_px_convertor_vec_
  for (int secid = 0; secid < DEF_MAX_SEC_ID; secid++) {
    if (fast_px_convertor_vec_[secid] != NULL) {
      delete fast_px_convertor_vec_[secid];
      fast_px_convertor_vec_[secid] = NULL;
    }
  }
}

void EmailForDisconnectWithoutLogout() {
  // also send an alert
  char hostname[128];
  hostname[127] = '\0';
  gethostname(hostname, 127);
  HFSAT::Email e;

  e.setSubject("Error: BSE Disconnected from exchange without sending logout : " + std::string(hostname));
  e.addRecepient("ravi.parikh@tworoads.co.in, nseall@tworoads.co.in");
  e.addSender("ravi.parikh@tworoads.co.in");
  e.content_stream << "host_machine: " << hostname << "<br/>";

  e.sendMail();
}

void BSEEngine::Connect() {

  //dbglogger_ << "SENDING GATEWAY REQUEST BSEEngine::Connect\n";
  struct timeval login_time;
  if (is_connected_) return;
  for (auto ip : bse_gateway_ip_list_) {
    bse_gateway_ip_ = ip;
    is_connected_ = true;  // making this true before Connect() call to avoid race condition from reply thread

    system("route");

    std::string route_cmd = "sudo ip route del " + bse_gateway_ip_ + "/32";
    std::cout << " INITIATING ROUTING CHANGES...." << route_cmd << std::endl;

    system(route_cmd.c_str());

    route_cmd = "sudo ip route add " + bse_gateway_ip_ + "/32 via " + tap_gateway_ + " dev " + tap_interface_ +
                " src " + tap_ip_;
    system(route_cmd.c_str());

    std::cout << " UPDATED ROUTING TABLE NOW... " << route_cmd << std::endl;

    system("route");

    bse_connection_gateway_socket_->Connect(bse_gateway_ip_, bse_gateway_port_);
    gettimeofday(&login_time, NULL);
    DBGLOG_CLASS_FUNC_LINE_INFO << "Trying IP: " << bse_gateway_ip_ << ":" << bse_gateway_port_
                                << " @ :" << login_time.tv_sec << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;

    GatewayRequest();

    // TODO
    //    Login();
    //    WaitForLoginSuccess();
    // Waiting after Login success to give a chance to exchange to disconnect so that we can retry at different IP
    //    sleep(3);

    // If we are still connected, then this gateway should be fine
    // is_connected_ would be set to false by reply thread if we got disconnected
    if (is_connected_) {
      DBGLOG_CLASS_FUNC_LINE_INFO << " The IP: " << ip << " Worked" << DBGLOG_ENDL_FLUSH;
      keep_trying_different_gateways_ = false;
      break;
    }
  }

  if (!is_connected_) {
    DBGLOG_CLASS_FUNC_LINE_INFO << " None of the IPs worked. Exiting." << DBGLOG_ENDL_FLUSH;
    keep_trying_different_gateways_ = false;
    exit(1);
  }
  p_engine_listener_->OnConnect(true);

}

/*
bool BSEEngine::ExitOnRefError(bool &shortcode_alert_, std::string shc_) {
  std::string allowe_ref_error_file_name_ =
      HFSAT::FileUtils::AppendHome(std::string(BASESYSINFODIR) + "AllowedRefError/allow_ref_error.txt");

  std::ifstream allowed_ref_error_file_;

  bool ret_val_ = true;

  allowed_ref_error_file_.open(allowe_ref_error_file_name_.c_str());

  if (!allowed_ref_error_file_.is_open()) {
    dbglogger_ << " Fatal Error : Could Not Load Allowed Ref Error File, Expecting At : " << allowe_ref_error_file_name_
               << "\n";
    dbglogger_.DumpCurrentBuffer();
  }

  char allowed_ref_data_buffer_[1024];

  while (allowed_ref_error_file_.good()) {
    memset(allowed_ref_data_buffer_, 0, 1024);

    allowed_ref_error_file_.getline(allowed_ref_data_buffer_, 1024);

    std::string this_ref_file_line_ = allowed_ref_data_buffer_;

    if (this_ref_file_line_.find("#") != std::string::npos) continue;  // comments etc

    HFSAT::PerishableStringTokenizer st_(allowed_ref_data_buffer_, sizeof(allowed_ref_data_buffer_));
    const std::vector<const char *> &tokens_ = st_.GetTokens();

    if (tokens_.size() != 2) {  // strict check - not using <

      continue;
    }

    std::string engine_name_ = tokens_[0];
    int ignore_ref_error_ = atoi(tokens_[1]);

    if (engine_name_ == "BSE" && ignore_ref_error_ == 1) {
      ret_val_ = false;
    }

    if (engine_name_ == shc_ && ignore_ref_error_ == 1) {
      shortcode_alert_ = false;
    }
  }
  return ret_val_;
}
*/


void BSEEngine::OnAddString(uint32_t num_sid) {
  SimpleSecuritySymbolIndexer &indexer = SimpleSecuritySymbolIndexer::GetUniqueInstance();
  std::string exchange_symbol = indexer.GetSecuritySymbolFromId(num_sid - 1);

  // Get data symbol. For cash market, data symbol is shortcode itself
  std::string data_symbol = bse_sec_def_.ConvertExchSymboltoDataSourceName(exchange_symbol);
  if (std::string("INVALID") == data_symbol) {
    DBGLOG_CLASS_FUNC_LINE_FATAL << "FAILED TO ADD SYMBOL STRING AS DATASYMBOL WAS INVALID, SYMBOL : "
                                 << exchange_symbol << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;

    // Should never have reached here
    exit(-1);
  }
  DBGLOG_CLASS_FUNC_LINE_INFO << "SecID: " << num_sid << " ExchSym: " << exchange_symbol << " DataSym: " << data_symbol << DBGLOG_ENDL_FLUSH;

  if (bse_sec_def_.IsSpreadExchSymbol(exchange_symbol)) {  // spread symbol added

    std::string spread_data_source_symbol = data_symbol;

    // NSE_NIFTY_FUT_20150129_20150226
    int pos = spread_data_source_symbol.rfind('_');

    // NSE_NIFTY_FUT_20150129
    std::string data_symbol_1 = spread_data_source_symbol.substr(0, pos);

    int basename_pos = data_symbol_1.rfind('_');
    // NSE_NIFTY_FUT_ + 20150226 = NSE_NIFTY_FUT_20150226
    std::string data_symbol_2 =
        data_symbol_1.substr(0, basename_pos + 1) + spread_data_source_symbol.substr(pos + 1, 8);

    if (std::string("INVALID") == data_symbol_1 || std::string("INVALID") == data_symbol_2) {
      DBGLOG_CLASS_FUNC_LINE_FATAL << "FAILED TO ADD SYMBOL STRING AS DATASYMBOL FOR ONE OF THE TWO/BOTH INTERNAL "
                                      "SYMBOL WAS INVALID DATA SYMBOL1 : "
                                   << data_symbol_1 << " DATA SYMBOL2 : " << data_symbol_2;
      DBGLOG_DUMP;

      // Should never have reached here
      exit(-1);
    }

    int32_t token_1 =
        bse_daily_token_symbol_handler_.GetTokenFromInternalSymbol(data_symbol_1.c_str(), bse_segment_type_);
    int32_t token_2 =
        bse_daily_token_symbol_handler_.GetTokenFromInternalSymbol(data_symbol_2.c_str(), bse_segment_type_);

    std::cout << "RefDataLeg1: " << ref_data_loader.GetBSERefData(bse_segment_type_)[token_1].ToString() << "\n";
    std::cout << "RefDataLeg2: " << ref_data_loader.GetBSERefData(bse_segment_type_)[token_2].ToString() << "\n";

    char *instrument_1 = ref_data_loader.GetBSERefData(bse_segment_type_)[token_1].instrument;
    char *instrument_2 = ref_data_loader.GetBSERefData(bse_segment_type_)[token_2].instrument;

    char symbol_1[11];
    memset(symbol_1, ' ', sizeof(symbol_1));
    memcpy(symbol_1, ref_data_loader.GetBSERefData(bse_segment_type_)[token_1].symbol,
           strlen(ref_data_loader.GetBSERefData(bse_segment_type_)[token_1].symbol));
    symbol_1[10] = 0;

    char symbol_2[11];
    memset(symbol_2, ' ', sizeof(symbol_2));
    memcpy(symbol_2, ref_data_loader.GetBSERefData(bse_segment_type_)[token_2].symbol,
           strlen(ref_data_loader.GetBSERefData(bse_segment_type_)[token_2].symbol));
    symbol_2[10] = 0;

    char *option_type_1 = ref_data_loader.GetBSERefData(bse_segment_type_)[token_1].option_type;
    char *option_type_2 = ref_data_loader.GetBSERefData(bse_segment_type_)[token_2].option_type;

    int32_t strike_price_1 =
        (int32_t)(ref_data_loader.GetBSERefData(bse_segment_type_)[token_1].strike_price); //add * price_multiplier_ subham
    int32_t strike_price_2 =
        (int32_t)(ref_data_loader.GetBSERefData(bse_segment_type_)[token_2].strike_price); //add * price_multiplier_ subham

    int32_t expiry_1 = ref_data_loader.GetBSERefData(bse_segment_type_)[token_1].expiry;
    int32_t expiry_2 = ref_data_loader.GetBSERefData(bse_segment_type_)[token_2].expiry;

    if (strcmp(option_type_1, "XX") == 0) {
      strike_price_1 = -1;
      strike_price_2 = -1;
      std::cout << "strike changed to -1 \n";
    }

    std::cout << "\nSymbol1: " << symbol_1 << " symbol2: " << symbol_2 << "\nInstrument: " << instrument_1
              << " symbol2: " << instrument_2 << "\nexpiry: " << expiry_1 << " 2: " << expiry_2
              << "\nstrike: " << strike_price_1 << " 2: " << strike_price_2 << "\noption_type: " << option_type_1 << " "
              << option_type_2 << "\ntoken: " << token_1 << " 2: " << token_2 << "\n";
#ifdef USE_NSE_SPREAD_ORDER
    // container_.spread_inst_desc_[num_sid - 1] = new SpreadInstrumentDesc();
    // container_.spread_inst_desc_[num_sid - 1]->SetSpreadInstrumentDesc(token_1, instrument_1, symbol_1, expiry_1,
    //   strike_price_1, option_type_1, token_2,
    //        instrument_2, symbol_2, expiry_2, strike_price_2, option_type_2);
    // DBGLOG_CLASS_FUNC_LINE_FATAL << container_.spread_inst_desc_[num_sid - 1]->ToString() << DBGLOG_ENDL_FLUSH;
    //container_.spread_inst_desc_[num_sid - 1].SetSpreadInstrumentDesc(
    //    token_1, instrument_1, symbol_1, expiry_1, strike_price_1, option_type_1, token_2, instrument_2, symbol_2,
    //    expiry_2, strike_price_2, option_type_2);
#endif
  } else {
    int32_t token = bse_daily_token_symbol_handler_.GetTokenFromInternalSymbol(data_symbol.c_str(), bse_segment_type_);
    char *instrument = ref_data_loader.GetBSERefData(bse_segment_type_)[token].instrument;

    char symbol[11];
    memset(symbol, ' ', sizeof(symbol));
    memcpy(symbol, ref_data_loader.GetBSERefData(bse_segment_type_)[token].symbol,
           strlen(ref_data_loader.GetBSERefData(bse_segment_type_)[token].symbol));
    symbol[10] = 0;

    char *option_type = ref_data_loader.GetBSERefData(bse_segment_type_)[token].option_type;
    int32_t strike_price =
        (int32_t)(std::round(ref_data_loader.GetBSERefData(bse_segment_type_)[token].strike_price)); //add * price_multiplier_ subham
    int32_t expiry = ref_data_loader.GetBSERefData(bse_segment_type_)[token].expiry;

    if (strcmp(option_type, "XX") == 0) {
      strike_price = -1;
      std::cout << "strike changed to -1 \n";
    }

    // For Cash Market, we require symbol and series info only as part of instrument descriptor.
    // We are using option_type field as series info (2 bytes each).
    if (HFSAT::BSESecurityDefinitions::IsEquity(data_symbol)) {  // data symbol is shc for cash segment
      strcpy(option_type, "EQ");
      std::string eq_symbol = exchange_symbol.substr(4);
      strncpy(symbol, eq_symbol.c_str(), eq_symbol.length());  // do not append '/0'
    }
    int32_t product_id_ = HFSAT::BSESecurityDefinitions::GetProductIdFromExchangeId(token);
    std::cout << "Token: " << token << " ExchSym: " << exchange_symbol
              << " RefData: " << ref_data_loader.GetBSERefData(bse_segment_type_)[token].ToString()
              << " Symbol: " << symbol << " Instrument: " << instrument << " Expiry: " << expiry
              << " Strike: " << strike_price << " Option_type: " << option_type << " ProductId: " 
              << product_id_ << "\n" << std::endl;
    container_.inst_desc_[num_sid - 1].SetInstrumentDesc(token, product_id_);
    //secid_to_secinfocashmarket_[num_sid - 1]->SetSecurityInfo(symbol, option_type);
    // Add to FastPxConverter map
    HFSAT::SecurityDefinitions &security_definitions_ =
        HFSAT::SecurityDefinitions::GetUniqueInstance(HFSAT::DateTime::GetCurrentIsoDateLocal());
    HFSAT::ShortcodeContractSpecificationMap &this_contract_specification_map_ =
        security_definitions_.contract_specification_map_;
    HFSAT::ExchangeSymbolManager::SetUniqueInstance(HFSAT::DateTime::GetCurrentIsoDateLocal());

    for (auto itr_ = this_contract_specification_map_.begin(); itr_ != this_contract_specification_map_.end(); itr_++) {
      std::string shortcode_ = (itr_->first);
      if (itr_->second.exch_source_ != HFSAT::kExchSourceBSE) {
        continue;
      }
      std::string this_symbol = bse_sec_def_.GetExchSymbolBSE(shortcode_);
      if (exchange_symbol == this_symbol) {
        // This is the corresponding security
        HFSAT::ContractSpecification &contract_spec_ = (itr_->second);
        if (fast_px_convertor_vec_[num_sid - 1] == NULL) {
          fast_px_convertor_vec_[num_sid - 1] = new HFSAT::FastPriceConvertor(contract_spec_.min_price_increment_);
          DBGLOG_CLASS_FUNC_LINE_INFO << " FAST PRICE MAP ADDED FOR SYMBOL : " << exchange_symbol
                                      << " MIN PRICE : " << contract_spec_.min_price_increment_ << DBGLOG_ENDL_FLUSH;
          DBGLOG_DUMP;
        }
        break;
      }
    }
  }

}

//=======================================================    ADMIN REQUESTS
//=============================================================//


/*
inline void BSEEngine::ProcessLockFreeTCPDirectRead() {
  if (keep_engine_running_) {
   struct timeval test_time;
   gettimeofday(&test_time,NULL);
   DBGLOG_CLASS_FUNC_LINE_INFO << "BSEEngine::ProcessLockFreeTCPDirectRead ReadN In: " << test_time.tv_sec << "." << test_time.tv_usec << DBGLOG_ENDL_DUMP ;
   int retval = bse_session_socket_->ReadN(MAX_BSE_RESPONSE_BUFFER_SIZE - read_offset_, bse_msg_char_buf_ + read_offset_);
   gettimeofday(&test_time,NULL);
   DBGLOG_CLASS_FUNC_LINE_INFO << "BSEEngine::ProcessLockFreeTCPDirectRead ReadN Out: " << test_time.tv_sec << "." << test_time.tv_usec << DBGLOG_ENDL_DUMP ;
   // DBGLOG_CLASS_FUNC_LINE_INFO << "bse_session_socket_->ReadN " << retval << DBGLOG_ENDL_DUMP;

    // reply from session socket, call processExchMSg
    if (retval > 0) {
      // Process Whatever left-over plus new read
      gettimeofday(&test_time,NULL);
      DBGLOG_CLASS_FUNC_LINE_INFO << "BSEEngine::ProcessLockFreeTCPDirectRead processBSEExchangeMsg IN: " 
                                  << test_time.tv_sec << "." << test_time.tv_usec << DBGLOG_ENDL_DUMP ;
      read_offset_ = processBSEExchangeMsg(bse_msg_char_buf_, read_offset_ + retval);
      gettimeofday(&test_time,NULL);
      DBGLOG_CLASS_FUNC_LINE_INFO << "BSEEngine::ProcessLockFreeTCPDirectRead processBSEExchangeMsg OUT: " 
                                  << test_time.tv_sec << "." << test_time.tv_usec << DBGLOG_ENDL_DUMP ;
      return ;  // Must return at this point for tcp direct
    } else if (retval < 0) {
      DBGLOG_CLASS_FUNC_LINE_FATAL << "READ RETURNED WITH AN ERROR : " << strerror(errno) << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;

      CleanUp();
      is_connected_ = false;
      is_logged_in_ = false;
      keep_engine_running_ = false;

      return ;

    } else {
      struct timeval disconnect_time;
      gettimeofday(&disconnect_time, NULL);

      DBGLOG_CLASS_FUNC_LINE_FATAL << "SOCKET CLOSED ON PEER SIDE @ : " << disconnect_time.tv_sec << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;

      is_connected_ = false;

      DBGLOG_CLASS_FUNC_LINE_INFO << "KeepTryingDifferentGateways: " << keep_trying_different_gateways_
                                  << " , Setting is_connected to false: " << is_connected_ << DBGLOG_ENDL_FLUSH;
      // Will not exit the thread until we are trying different gateways
      if (keep_trying_different_gateways_) {
        DBGLOG_CLASS_FUNC_LINE_INFO << "Not exiting now as we have to try other gateways, is_logged_in : "
                                    << is_logged_in_ << DBGLOG_ENDL_FLUSH;

        // This will give back the control to multisession engine so that it can call OnInputAvailable again.
        return ;
      }

      CleanUp();

      keep_engine_running_ = false;
      if (is_logged_in_) {
        // Sending a mail, we got disconnected and we are still logged in
        //EmailForDisconnectWithoutLogout();
        return ;
      }

      is_logged_in_ = false;
    }
    DBGLOG_CLASS_FUNC_LINE_FATAL << "Session Closed" << DBGLOG_ENDL_FLUSH;
  }

  return ;

}
*/

/// @brief 
inline void BSEEngine::ProcessLockFreeTCPDirectRead() {

  //dbglogger_ << "BSEEngine::ProcessLockFreeTCPDirectRead\n";
  //  struct timeval tv;
  //  gettimeofday(&tv,NULL);
  //  std::cout << "CURRENT TIME IS : " << ( tv.tv_sec * 1000000 + tv.tv_usec ) << std::endl ;

  if(!is_read_socket_open_) return;
  struct timeval test_time;
   // DBGLOG_CLASS_FUNC_LINE_INFO << "bse_session_socket_->ReadN " << retval << DBGLOG_ENDL_DUMP;
  HFSAT::ORS::InstrumentDescBSE *inst_desc = &container_.inst_desc_[0];
  //SecurityInfoCashMarket *sec_info = secid_to_secinfocashmarket_[0];
  if (inst_desc->GetToken() == 0) return;
  bse_new_order_single_short_.setBSENewOrderSingleShortDynamicFields(&dummy_order_, 0, 0);

  gettimeofday(&test_time,NULL);
  //DBGLOG_CLASS_FUNC_LINE_INFO << "BSEEngine::ProcessLockFreeTCPDirectRead ReadN In: " << test_time.tv_sec << "." << test_time.tv_usec << DBGLOG_ENDL_DUMP ;
  tcp_direct_client_zocket_.WriteLockFreeNWarmOnly(send_write_length,(void *)(&bse_new_order_single_short_));

  tcp_direct_read_buffer_ =
      tcp_direct_client_zocket_.ReadLockFreeN(tcp_direct_read_length_, was_tcp_direct_read_successful_);
  gettimeofday(&test_time,NULL);
  //DBGLOG_CLASS_FUNC_LINE_INFO << "BSEEngine::ProcessLockFreeTCPDirectRead ReadN Out: " << test_time.tv_sec << "." << test_time.tv_usec << DBGLOG_ENDL_DUMP ;

  if (true == was_tcp_direct_read_successful_) {
    onInputAvailable(0, tcp_direct_read_buffer_, tcp_direct_read_length_);
  }
}

inline int32_t BSEEngine::onInputAvailable(int32_t _socket_, char *buffer, int32_t length) {

  if (keep_engine_running_) {
    int32_t read_length = length;
    //        nse_session_socket_->ReadN(MAX_NSE_RESPONSE_BUFFER_SIZE - read_offset_, nse_msg_buffer_ + read_offset_);

    memcpy((char *)bse_msg_char_buf_ + read_offset_, buffer, length);

    //#if NSE_DEBUG_INFO
    //    DBGLOG_CLASS_FUNC_LINE_INFO << "onInpAvail: " << read_length << " " << _socket_ << " "
    //                                << nse_session_socket_->socket_file_descriptor() << DBGLOG_ENDL_FLUSH;
    //    DBGLOG_CLASS_FUNC_LINE_INFO << "SOCKET READ RETURNED : " << read_length << DBGLOG_ENDL_FLUSH;
    //#endif

    if (read_length > 0) {
    //  read_offset_ = processBSEExchangeMsg(bse_msg_char_buf_, read_offset_ + read_length);
      read_offset_ = ProcessEncryptedExchangeResponse(bse_msg_char_buf_, read_offset_ + read_length);
      //read_offset_ = ProcessExchangeResponse(nse_msg_buffer_, read_offset_ + read_length);
      return 0;  // Must return at this point for tcp direct
    } else if (read_length < 0) {
      DBGLOG_CLASS_FUNC_LINE_FATAL << "READ RETURNED WITH AN ERROR : " << strerror(errno) << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;

      CleanUp();
      is_connected_ = false;
      is_logged_in_ = false;
      keep_engine_running_ = false;

      return HFSAT::ORS::kSessionTerminated;

    } else {
      struct timeval disconnect_time;
      gettimeofday(&disconnect_time, NULL);

      DBGLOG_CLASS_FUNC_LINE_FATAL << "SOCKET CLOSED ON PEER SIDE @ : " << disconnect_time.tv_sec << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;

      is_connected_ = false;

      DBGLOG_CLASS_FUNC_LINE_INFO << "KeepTryingDifferentGateways: " << keep_trying_different_gateways_
                                  << " , Setting is_connected to false: " << is_connected_ << DBGLOG_ENDL_FLUSH;

      // Will not exit the thread until we are trying different gateways
      if (keep_trying_different_gateways_) {
        DBGLOG_CLASS_FUNC_LINE_INFO << "Not exiting now as we have to try other gateways, is_logged_in : "
                                    << is_logged_in_ << DBGLOG_ENDL_FLUSH;

        // This will give back the control to multisession engine so that it can call OnInputAvailable again.
        return 0;
      }

      CleanUp();

      keep_engine_running_ = false;
      if (is_logged_in_) {
        // Sending a mail, we got disconnected and we are still logged in
        EmailForDisconnectWithoutLogout();
        return HFSAT::ORS::kSessionTerminated;
      }

      is_logged_in_ = false;
    }
  }

  DBGLOG_CLASS_FUNC_LINE_FATAL << "Session Closed" << DBGLOG_ENDL_FLUSH;

  return HFSAT::ORS::kSessionClosed;
}


void BSEEngine::GatewayRequest() {
  //DBGLOG_CLASS_FUNC_LINE_INFO << "BSEEngine::GatewayRequest" << "\n";
  if (is_logged_in_) return;

  if (!is_connected_) {
    DBGLOG_CLASS_FUNC_LINE_FATAL << "LOGIN CALLED WITHOUT BEING CONNECTED" << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    exit(-1);
  }

  int write_len_ = bse_connection_gateway_request_.getBSEConnectionGatewayRequestMsgLength();
  DBGLOG_CLASS_FUNC_LINE_INFO << "GatewayRequest write_len " << write_len_ << DBGLOG_ENDL_DUMP;
  dbglogger_ << "GatewayRequest BodyLen: " << bse_connection_gateway_request_.bse_connection_gateway_request_.MessageHeaderIn.BodyLen << " " <<
                "TemplateID: " << bse_connection_gateway_request_.bse_connection_gateway_request_.MessageHeaderIn.TemplateID << " " <<
                "MsgSeqNum: " << bse_connection_gateway_request_.bse_connection_gateway_request_.RequestHeader.MsgSeqNum << " " <<
                "Password: " << bse_connection_gateway_request_.bse_connection_gateway_request_.Password << " " <<
                "PartyIDSessionID: " << bse_connection_gateway_request_.bse_connection_gateway_request_.PartyIDSessionID << " " <<
                "DefaultCstmApplVerID:" << bse_connection_gateway_request_.bse_connection_gateway_request_.DefaultCstmApplVerID << " " <<
                "GatewayRequestT:" << sizeof(GatewayRequestT) << " " << "\n";
  dbglogger_.DumpCurrentBuffer();

  last_send_time_ = time(NULL);
  int32_t write_length = bse_connection_gateway_socket_->WriteN(write_len_, (void *)(&bse_connection_gateway_request_));

  if (write_length < write_len_) {
    DBGLOG_CLASS_FUNC_LINE_FATAL << "FAILD TO WRITE COMPLETE MESSAGE TO EXCHANGE SOCKET" << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    exit(-1);
  }

  char buffer[1024];
  memset((void *)buffer, 0, 1024);

  int retval = bse_connection_gateway_socket_->ReadN(1024, buffer);

  processBSEExchangeMsg(buffer,retval);

}


void BSEEngine::SecondarySessionLogin() {
  DBGLOG_CLASS_FUNC_LINE_INFO << "SecondarySessionLogin IN: " << "is_logged_in_: " << is_logged_in_ << " is_connected_: " << is_connected_ << DBGLOG_ENDL_DUMP;
  if (is_logged_in_) return;

  if (!is_connected_) {
    DBGLOG_CLASS_FUNC_LINE_FATAL << "LOGIN CALLED WITHOUT BEING CONNECTED" << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    exit(-1);
  }

  std::cerr << " Secondary Login IP : " << bse_session_logon_request_secondary_ip_address_
            << " Port : " << bse_session_logon_request_secondary_port_ << "\n";

  if ("0" == bse_session_logon_request_secondary_ip_address_ || 0 == bse_session_logon_request_secondary_port_) {
    DBGLOG_CLASS_FUNC_LINE_FATAL << "COULDN'T GET IP AND PORT FROM EXCHANGE FOR LOGIN" << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    exit(-1);
  }

  std::cout << " CONNECTING TO : " << bse_session_logon_request_secondary_ip_address_ << " " << bse_session_logon_request_secondary_port_ << std::endl;

  system("route");

  std::cout << " INITIATING ROUTING CHNAGES...." << std::endl;

  std::string route_cmd = "sudo ip route del " + bse_session_logon_request_secondary_ip_address_ + "/32";
  system(route_cmd.c_str());

  route_cmd =
      "sudo ip route add " + bse_session_logon_request_secondary_ip_address_ + "/32 via " + tap_gateway_ + " dev " + tap_interface_ + " src " + tap_ip_;
  system(route_cmd.c_str());

  std::cout << " UPDATED ROUTING TABLE NOW... " << std::endl;

  system("route");

/*
  // Since the Older Socket Was cleaned up
  bse_session_socket_ =
      new TcpClientSocketWithLogging(true, async_sender, async_reciever,
                                     output_log_dir_ + "/audit." + HFSAT::DateTime::GetCurrentIsoDateLocalAsString() +
                                     "." + m_settings.getValue("UserID"));

  bse_session_socket_->Connect(bse_session_logon_request_secondary_ip_address_.c_str(),
                               bse_session_logon_request_secondary_port_);
*/
  tcp_direct_client_zocket_.ConnectAndAddZocketToMuxer(bse_session_logon_request_secondary_ip_address_,
                                                       bse_session_logon_request_secondary_port_);

  int write_len_ = bse_session_logon_request_.getBSESessionLogonMsgLength();

  last_send_time_ = time(NULL);
  if (tcp_direct_client_zocket_.WriteLockFreeN(write_len_, (void *)(&bse_session_logon_request_)) < write_len_) {
    DBGLOG_CLASS_FUNC_LINE_FATAL << "FAILD TO WRITE COMPLETE SECONDARYSESSIONLOGON MESSAGE TO EXCHANGE SOCKET" << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    exit(-1);
  } else {
    keep_engine_running_ = true;
  }

}


void BSEEngine::SessionRegistrationRequest(){

  //dbglogger_ << "BSEEngine::SessionRegistrationRequest\n";
  
  if (is_logged_in_) return;

  if (!is_connected_) {
    DBGLOG_CLASS_FUNC_LINE_FATAL << "SessionRegistrationRequest CALLED WITHOUT BEING CONNECTED" << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    exit(-1);
  }

  std::cerr << " Primary Login IP : " << bse_session_logon_request_primary_ip_address_
            << " Port : " << bse_session_logon_request_primary_port_ << "\n";

  if ("0" == bse_session_logon_request_primary_ip_address_ || 0 == bse_session_logon_request_primary_port_) {
    DBGLOG_CLASS_FUNC_LINE_FATAL << "COULDN'T GET IP AND PORT FROM EXCHANGE FOR LOGIN" << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    exit(-1);
  }

  std::cout << " CONNECTING TO : " << bse_session_logon_request_primary_ip_address_ << " " << bse_session_logon_request_primary_port_ << std::endl;

  system("route");

  std::cout << " INITIATING ROUTING CHNAGES...." << std::endl;

  std::string route_cmd = "sudo ip route del " + bse_session_logon_request_primary_ip_address_ + "/32";
  system(route_cmd.c_str());

  route_cmd =
      "sudo ip route add " + bse_session_logon_request_primary_ip_address_ + "/32 via " + tap_gateway_ + " dev " + tap_interface_ + " src " + tap_ip_;
  system(route_cmd.c_str());

  std::cout << " UPDATED ROUTING TABLE NOW... " << std::endl;

  system("route");
  
  bse_connection_gateway_socket_->DisConnect(); 

  tcp_direct_client_zocket_.ConnectAndAddZocketToMuxer(bse_session_logon_request_primary_ip_address_,
                                                       bse_session_logon_request_primary_port_);

  last_send_time_ = time(NULL);


  int32_t write_len_ = bse_session_registration_request_.getBSESessionLogonMsgLength();
  //dbglogger_ << "LENGTH OF bse_session_registration_request_ IS " << write_len_ << "\n";

  if (tcp_direct_client_zocket_.WriteLockFreeN(write_len_, (void *)(&bse_session_registration_request_)) < write_len_) {
    DBGLOG_CLASS_FUNC_LINE_FATAL << "FAILED TO WRITE COMPLETE SESSIONLOGON MESSAGE TO EXCHANGE SOCKET" << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
  }

  DBGLOG_CLASS_FUNC_LINE_INFO << "WROTE : " << write_len_ << DBGLOG_ENDL_FLUSH;
  DBGLOG_DUMP;

  keep_engine_running_ = true;
  is_read_socket_open_ = true;
}

void BSEEngine::SessionLogin() {

  
  DBGLOG_CLASS_FUNC_LINE_INFO << "SessionLogin IN: " << "is_logged_in_: " << is_logged_in_ << " is_connected_: " << is_connected_ << DBGLOG_ENDL_DUMP;
  if (is_logged_in_) return;

  if (!is_connected_) {
    DBGLOG_CLASS_FUNC_LINE_FATAL << "LOGIN CALLED WITHOUT BEING CONNECTED" << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    exit(-1);
  }

  std::cerr << " Primary Login IP : " << bse_session_logon_request_primary_ip_address_
            << " Port : " << bse_session_logon_request_primary_port_ << "\n";

  if ("0" == bse_session_logon_request_primary_ip_address_ || 0 == bse_session_logon_request_primary_port_) {
    DBGLOG_CLASS_FUNC_LINE_FATAL << "COULDN'T GET IP AND PORT FROM EXCHANGE FOR LOGIN" << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    exit(-1);
  }

  std::cout << " CONNECTING TO : " << bse_session_logon_request_primary_ip_address_ << " " << bse_session_logon_request_primary_port_ << std::endl;

  /*
  system("route");

  std::cout << " INITIATING ROUTING CHNAGES...." << std::endl;

  std::string route_cmd = "sudo ip route del " + bse_session_logon_request_primary_ip_address_ + "/32";
  system(route_cmd.c_str());

  route_cmd =
      "sudo ip route add " + bse_session_logon_request_primary_ip_address_ + "/32 via " + tap_gateway_ + " dev " + tap_interface_ + " src " + tap_ip_;
  system(route_cmd.c_str());

  std::cout << " UPDATED ROUTING TABLE NOW... " << std::endl;

  system("route");

  bse_connection_gateway_socket_->DisConnect();
  

  bse_session_socket_->Connect(bse_session_logon_request_primary_ip_address_.c_str(),
                               bse_session_logon_request_primary_port_);

  tcp_direct_client_zocket_.ConnectAndAddZocketToMuxer(bse_session_logon_request_primary_ip_address_,
                                                       bse_session_logon_request_primary_port_);
  */

  int write_len_ = bse_session_logon_request_.getBSESessionLogonMsgLength();
  next_seq_num_ = 2;
  bse_session_logon_request_.bse_session_logon_request_.RequestHeader.MsgSeqNum = 2;

  DBGLOG_CLASS_FUNC_LINE_INFO << "SessionLogin write_len " << write_len_ << DBGLOG_ENDL_DUMP;
  dbglogger_ << "SessionLogin BodyLen: " << bse_session_logon_request_.bse_session_logon_request_.MessageHeaderIn.BodyLen << " " <<
                "TemplateID: " << bse_session_logon_request_.bse_session_logon_request_.MessageHeaderIn.TemplateID << " " <<
                "MsgSeqNum: " << bse_session_logon_request_.bse_session_logon_request_.RequestHeader.MsgSeqNum << " " <<
                "HeartBtInt: " << bse_session_logon_request_.bse_session_logon_request_.HeartBtInt << " " <<
                "PartyIDSessionID: " << bse_session_logon_request_.bse_session_logon_request_.PartyIDSessionID << " " <<
                "DefaultCstmApplVerID: " << bse_session_logon_request_.bse_session_logon_request_.DefaultCstmApplVerID << " " <<
                "Password: " << bse_session_logon_request_.bse_session_logon_request_.Password << " " << "\n";
/*
                "ApplUsageOrders: " << bse_session_logon_request_.bse_session_logon_request_.ApplUsageOrders << " " << 
                "ApplUsageQuotes: " << bse_session_logon_request_.bse_session_logon_request_.ApplUsageQuotes << " " << 
                "OrderRoutingIndicator: " << bse_session_logon_request_.bse_session_logon_request_.OrderRoutingIndicator << " " <<
                "FIXEngineName: " << bse_session_logon_request_.bse_session_logon_request_.FIXEngineName << " " <<
                "FIXEngineVersion: " << bse_session_logon_request_.bse_session_logon_request_.FIXEngineVersion << " " << 
                "FIXEngineVendor: " << bse_session_logon_request_.bse_session_logon_request_.FIXEngineVendor << " " <<
                "ApplicationSystemName: " << bse_session_logon_request_.bse_session_logon_request_.ApplicationSystemName << " " <<
                "ApplicationSystemVersion: " << bse_session_logon_request_.bse_session_logon_request_.ApplicationSystemVersion << " " << 
                "ApplicationSystemVendor: " << bse_session_logon_request_.bse_session_logon_request_.ApplicationSystemVendor << " " << 
                "LogonRequestT: " << sizeof(LogonRequestT) << " " << "\n";
*/
  dbglogger_.DumpCurrentBuffer();

  bool primary_connection_failed_ = false;

  char send_buffer[1024];
  memset((void*)send_buffer,0,1024); 
  memcpy((void*)send_buffer,(void*)&bse_session_logon_request_.bse_session_logon_request_.MessageHeaderIn, 16);
  open_ssl_crypto_.aes_encrypt((unsigned char*)(((char*)&bse_session_logon_request_.bse_session_logon_request_)+16), bse_session_logon_request_.getBSESessionLogonMsgLength()-16);

  //std::cout << " ENCRYPTED LENGTH : " << open_ssl_crypto_.encrypt_len << " INPUT LENGTH : " << bse_session_logon_request_.getBSESessionLogonMsgLength()-16 << std::endl;
  memcpy((void*)(send_buffer+16), (void*)open_ssl_crypto_.encrypt_text, open_ssl_crypto_.encrypt_len);

  last_send_time_ = time(NULL);
  if (tcp_direct_client_zocket_.WriteLockFreeN(write_len_, (void*)send_buffer) < write_len_) {
    DBGLOG_CLASS_FUNC_LINE_FATAL << "FAILD TO WRITE COMPLETE SESSIONLOGON MESSAGE TO EXCHANGE SOCKET" << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;

    //bse_session_socket_->Close();

    primary_connection_failed_ = true;

  } else {
    keep_engine_running_ = true;
  }


  if (primary_connection_failed_) {
    SecondarySessionLogin();  // Couldn't connect to the Primary Session Gateway, Trying for secondary
  }
}

void BSEEngine::Login() {

  if (is_logged_in_) return;

  if (!is_connected_) {
    DBGLOG_CLASS_FUNC_LINE_FATAL << "LOGIN CALLED WITHOUT BEING CONNECTED" << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    exit(-1);
  }

  int write_len_ = bse_user_logon_request_.getBSEUserLogonMsgLength();

//  send_seq_num_lock_.LockMutex();
  
  bse_user_logon_request_.setBSEUserLogonMessageSequence(next_seq_num_);

  next_seq_num_++;
  bse_user_logon_request_.bse_user_logon_request_.RequestHeader.MsgSeqNum = next_seq_num_;
  next_seq_num_++;

  char send_buffer[1024];
  memset((void*)send_buffer,0,1024); 
  memcpy((void*)send_buffer,&bse_user_logon_request_.bse_user_logon_request_.MessageHeaderIn, 16);

  open_ssl_crypto_.aes_encrypt((unsigned char*)&bse_user_logon_request_.bse_user_logon_request_+16, bse_user_logon_request_.getBSEUserLogonMsgLength()-16);

  memcpy((void*)(send_buffer+16), (void*)open_ssl_crypto_.encrypt_text, open_ssl_crypto_.encrypt_len);

  last_send_time_ = time(NULL);
  if (tcp_direct_client_zocket_.WriteLockFreeN(16+open_ssl_crypto_.encrypt_len, (void*)send_buffer) < write_len_) {
    DBGLOG_CLASS_FUNC_LINE_FATAL << "FAILD TO WRITE COMPLETE USERLOGON MESSAGE TO EXCHANGE SOCKET " << write_len_ << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    exit(-1);
  }

  dbglogger_ << "UserLogon BodyLen: " << bse_user_logon_request_.bse_user_logon_request_.MessageHeaderIn.BodyLen << " " <<
                "TemplateID: " << bse_user_logon_request_.bse_user_logon_request_.MessageHeaderIn.TemplateID << " " <<
                "MsgSeqNum: " << bse_user_logon_request_.bse_user_logon_request_.RequestHeader.MsgSeqNum << " " <<
                "Username: " << bse_user_logon_request_.bse_user_logon_request_.Username << " " <<
                "Password: " << bse_user_logon_request_.bse_user_logon_request_.Password << " " <<
                "UserLoginRequestT: " << sizeof(UserLoginRequestT) << " " << "\n";
  dbglogger_.DumpCurrentBuffer();

//  send_seq_num_lock_.UnlockMutex();

  //IsOnloadMsgWarmSuppoted(bse_session_socket_->socket_file_descriptor());

  //DBGLOG_CLASS_FUNC_LINE_INFO << "Onload Msg Warm Supported?: " << is_msg_warm_supported_ << DBGLOG_ENDL_FLUSH;
}

void BSEEngine::SessionLogout() {

  if (!is_logged_in_) {
    DBGLOG_CLASS_FUNC_LINE_ERROR << "DISCONNECT CALLED WITHOUT BEING LOGGED IN" << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
  }
  if (!is_connected_) {
    DBGLOG_CLASS_FUNC_LINE_ERROR << "DISCONNECT CALLED WITHOUT BEING CONNECTED" << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
  }

  int write_len_ = bse_session_logout_request_.getBSESessionLogoutRequestMsgLength();

//  send_seq_num_lock_.LockMutex();

  bse_session_logout_request_.setBSESessionLogoutMessageSequence(next_seq_num_);

  next_seq_num_++;

  char send_buffer[1024];
  memset((void*)send_buffer,0,1024); 
  memcpy((void*)send_buffer,&bse_session_logout_request_.bse_session_logout_request_.MessageHeaderIn, 16);

  open_ssl_crypto_.aes_encrypt((unsigned char*)&bse_session_logout_request_.bse_session_logout_request_+16, bse_session_logout_request_.getBSESessionLogoutRequestMsgLength()-16);

  memcpy((void*)(send_buffer+16), (void*)open_ssl_crypto_.encrypt_text, open_ssl_crypto_.encrypt_len);

  if (tcp_direct_client_zocket_.WriteLockFreeN(16+open_ssl_crypto_.encrypt_len, (void*)send_buffer) < write_len_) {
    DBGLOG_CLASS_FUNC_LINE_FATAL << "FAILD TO WRITE COMPLETE SESSIONLOGOUT MESSAGE TO EXCHANGE SOCKET " << write_len_ << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    exit(-1);
  }

  dbglogger_ << "SessionLogout BodyLen: " << bse_session_logout_request_.bse_session_logout_request_.MessageHeaderIn.BodyLen << " " <<
                "TemplateID: " << bse_session_logout_request_.bse_session_logout_request_.MessageHeaderIn.TemplateID << " " <<
                "MsgSeqNum: " << bse_session_logout_request_.bse_session_logout_request_.RequestHeader.MsgSeqNum << " " <<
                "LogoutRequestT : " << sizeof(LogoutRequestT) << " " << "\n";
  dbglogger_.DumpCurrentBuffer();

  char buffer[1024];
  memset((void *)buffer, 0, 1024);

  //int retval = bse_connection_gateway_socket_->ReadN(1024, buffer);

  //processBSEExchangeMsg(buffer,retval);
  //ProcessEncryptedExchangeResponse(buffer,retval);

//  send_seq_num_lock_.UnlockMutex();

}

void BSEEngine::Logout() {

  if (!is_logged_in_) {
    DBGLOG_CLASS_FUNC_LINE_ERROR << "DISCONNECT CALLED WITHOUT BEING LOGGED IN" << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
  }
  if (!is_connected_) {
    DBGLOG_CLASS_FUNC_LINE_ERROR << "DISCONNECT CALLED WITHOUT BEING CONNECTED" << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
  }

  int write_len_ = bse_user_logout_request_.getBSEUserLogoutMsgLength();

//  send_seq_num_lock_.LockMutex();

  bse_user_logout_request_.setBSEUserLogoutMessageSeqeunce(next_seq_num_);

  next_seq_num_++;

  char send_buffer[1024];
  memset((void*)send_buffer,0,1024); 
  memcpy((void*)send_buffer,&bse_user_logout_request_.bse_user_logout_request_.MessageHeaderIn, 16);

  open_ssl_crypto_.aes_encrypt((unsigned char*)&bse_user_logout_request_.bse_user_logout_request_+16, bse_user_logout_request_.getBSEUserLogoutMsgLength()-16);

  memcpy((void*)(send_buffer+16), (void*)open_ssl_crypto_.encrypt_text, open_ssl_crypto_.encrypt_len);

  if (tcp_direct_client_zocket_.WriteLockFreeN(16+open_ssl_crypto_.encrypt_len, (void*)send_buffer) < write_len_) {
    DBGLOG_CLASS_FUNC_LINE_FATAL << "FAILD TO WRITE COMPLETE USERLOGOUT MESSAGE TO EXCHANGE SOCKET " << write_len_ << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    exit(-1);
  }

  dbglogger_ << "UserLogout BodyLen: " << bse_user_logout_request_.bse_user_logout_request_.MessageHeaderIn.BodyLen << " " <<
                "TemplateID: " << bse_user_logout_request_.bse_user_logout_request_.MessageHeaderIn.TemplateID << " " <<
                "MsgSeqNum: " << bse_user_logout_request_.bse_user_logout_request_.RequestHeader.MsgSeqNum << " " <<
                "Username: " << bse_user_logout_request_.bse_user_logout_request_.Username << " " <<
                "UserLogoutRequestT: " << sizeof(UserLogoutRequestT) << " " << "\n";
  dbglogger_.DumpCurrentBuffer();

  char buffer[1024];
  memset((void *)buffer, 0, 1024);

  //dbglogger_ << "READING FROM USER LOGOUT FROM BSE EXCHANGE\n";
  //int retval = bse_connection_gateway_socket_->ReadN(1024, buffer);
  //dbglogger_ << "READING FROM USER LOGOUT FROM BSE EXCHANGE COMPLETED\n";

  //processBSEExchangeMsg(buffer,retval);
  //ProcessEncryptedExchangeResponse(buffer,retval);

//  send_seq_num_lock_.UnlockMutex();
}

void BSEEngine::DisConnect() {

  if (!is_logged_in_) {
    DBGLOG_CLASS_FUNC_LINE_ERROR << "DISCONNECT CALLED WITHOUT BEING LOGGED IN" << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
  }

  if (!is_connected_) {
    DBGLOG_CLASS_FUNC_LINE_ERROR << "DISCONNECT CALLED WITHOUT BEING CONNECTED" << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
  }
  Logout();

}

void BSEEngine::OnControlHubDisconnectionShutdown() {
  DBGLOG_CLASS_FUNC_LINE_FATAL << "RECEIVED SHUTDOWN REQUEST AS CONTROL HUB COULDN'T BE REACHED...."
                               << DBGLOG_ENDL_FLUSH;
  DBGLOG_CLASS_FUNC_LINE_ERROR << "INITIATING BLOCKING OF FURTHER ORDERS THROUGH ORS...." << DBGLOG_ENDL_FLUSH;

  sleep(1);

  DBGLOG_CLASS_FUNC_LINE_ERROR << "CANCELLING EXISTING ORDERS.... " << DBGLOG_ENDL_FLUSH;
  p_engine_listener_->CancelAllPendingOrders();

  DBGLOG_CLASS_FUNC_LINE_ERROR << "SENDING LOGOUT..., INFORMING LISTENERS..." << DBGLOG_ENDL_FLUSH;
  Logout();

  DBGLOG_CLASS_FUNC_LINE_ERROR << "CLEANING UP SOCKETS... " << DBGLOG_ENDL_FLUSH;

  DBGLOG_CLASS_FUNC_LINE_ERROR << "DONE WITH CLEAN UP... BRINGING SYSTEM DOWN... " << DBGLOG_ENDL_FLUSH;
}

void BSEEngine::CheckToSendHeartbeat() {
  if (!is_logged_in_) {
    return;
  }

  time_t cptime = time(NULL);
  unsigned int diff = cptime - lastMsgSentAtThisTime();
  if (diff >= (unsigned int) heartbeat_interval_ - 1) {
#if BSE_DEBUG_INFO
    DBGLOG_CLASS_FUNC_LINE_INFO << "sendHeartbeat: " << diff << DBGLOG_ENDL_DUMP;
#endif
    sendHeartbeat();
  }

//  if (true == is_pre_open_) {
//    is_pre_open_ = ValidatePreOpenMarketTimings();
//  }
}

void BSEEngine::sendHeartbeat() {
#if BSE_DEBUG_INFO
  DBGLOG_CLASS_FUNC_LINE_INFO << "sendHeartbeat: " << " logged: " << is_logged_in_ << " connected: " << is_connected_ << DBGLOG_ENDL_DUMP;
#endif
  if (!is_logged_in_ || !is_connected_) return;

  int write_len_ = bse_pre_computed_message_lengths_.HeartbeatT_MSG_LENGTH_;

  char send_buffer[1024];
  memset((void*)send_buffer,0,1024); 
  memcpy((void*)send_buffer,&bse_heartbeat_request_.bse_heartbeat_request_.MessageHeaderIn, 16);

  open_ssl_crypto_.aes_encrypt((unsigned char*)&bse_heartbeat_request_.bse_heartbeat_request_+16, bse_pre_computed_message_lengths_.HeartbeatT_MSG_LENGTH_-16);

  memcpy((void*)(send_buffer+16), (void*)open_ssl_crypto_.encrypt_text, open_ssl_crypto_.encrypt_len);

  last_send_time_ = time(NULL);
  if (tcp_direct_client_zocket_.WriteLockFreeN(16+open_ssl_crypto_.encrypt_len, (void*)send_buffer) < write_len_) {
    DBGLOG_CLASS_FUNC_LINE_FATAL << "FAILED TO WRITE COMPLETE HEARTBEAT MESSAGE TO EXCHANGE SOCKET" << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;

    if (is_logged_in_ && is_connected_) {  // Check if we have not issued a graceful logout or disconnect request

      exit(1);
    }
  }

  //num_unacked_heartbeats_ = 0;
}

/*
void BSEEngine::ProcessFakeSend(HFSAT::ORS::Order *ord, ORQType_t type) {
  if (!is_logged_in_) {
    return;
  }

  if (type == ORQ_SEND) {
    BSE::BSENewOrderSingleShort *new_ord = container_.new_order[ord->security_id_];

    if (new_ord == NULL) return;

    int fake_saci_key = 3;

    uint32_t compliance_id = bse_algo_tagger_.GetComplianceIdForClientFake(fake_saci_key);
    new_ord->setBSENewOrderSingleShortComplianceId(compliance_id);

    // just set some values to bring in cache
    new_ord->setBSENewOrderSingleShortMessageSequnece(0);

    int write_len = bse_pre_computed_message_lengths_.NewOrderSingleShortRequestT_MSG_LENGTH_;

    if (is_msg_warm_supported_) {
      send(bse_session_socket_->socket_file_descriptor(), &new_ord->bse_new_order_single_short_, write_len,
           ONLOAD_MSG_WARM);
    }
  }

  else if (type == ORQ_CANCEL) {
    BSE::BSECancelOrderSingle *cancel_ord = container_.cancel_order[ord->security_id_];

    if (cancel_ord == NULL) return;

    uint32_t compliance_id = bse_algo_tagger_.GetComplianceIdForClient(ord->server_assigned_client_id_);
    cancel_ord->setBSECancelOrderSingleComplianceId(compliance_id);

    cancel_ord->setBSECancelOrderSingleMessageSequence(0);

    int write_len = bse_pre_computed_message_lengths_.DeleteOrderSingleRequestT_MSG_LENGTH_;

    if (is_msg_warm_supported_) {
      send(bse_session_socket_->socket_file_descriptor(), &cancel_ord->bse_cancel_order_single_request_, write_len,
           ONLOAD_MSG_WARM);
    }
  }
}
*/

void BSEEngine::SendOrder(HFSAT::ORS::Order *rp_order_, const int32_t& prod_token_) {

#if BSE_DEBUG_INFO
  uint8_t buy_sell_ = (rp_order_->buysell_ == HFSAT::kTradeTypeBuy) ? 0 : 1;
  DBGLOG_CLASS_FUNC_LINE_INFO << "NEW ORDER REQUEST::\n" << rp_order_->toString() << DBGLOG_ENDL_FLUSH;
  DBGLOG_CLASS_FUNC_LINE << "Token: " << prod_token_ << " BuySell: " << buy_sell_ << " Price: " << rp_order_->price_ 
                         << " Size: " << rp_order_->size_remaining_
                         << " Size Disclosed: " << rp_order_->size_disclosed_
                         << " Saos: " << rp_order_->server_assigned_order_sequence_
                         << DBGLOG_ENDL_FLUSH;
#endif
//  bse_new_order_single_short_.setBSENewOrderSingleShortDynamicFields(rp_order_->price_, rp_order_->server_assigned_order_sequence_,
//                                                                     rp_order_->size_remaining_, rp_order_->buysell_,
//                                                                     next_seq_num_, prod_token_);

  bse_new_order_single_short_.setBSENewOrderSingleShortDynamicFields(rp_order_, next_seq_num_, prod_token_);

  if(is_risk_reduction_set){
    //Although this should already be set IOC because in enable and disable IOC it must be set(precaution)
    bse_new_order_single_short_.setBSENewOrderSingleShortTimeInforce(ENUM_TIMEINFORCE_IOC);
  	rp_order_->is_ioc = true;
    DBGLOG_CLASS_FUNC_LINE_FATAL << "Only IOC Order Sent " << rp_order_->toStringShort();
 	  DBGLOG_DUMP;
  }

  char send_buffer[1024];
  memset((void*)send_buffer,0,1024); 
  memcpy((void*)send_buffer,&bse_new_order_single_short_.bse_new_order_single_short_request_.MessageHeaderIn, 16);

  open_ssl_crypto_.aes_encrypt((unsigned char*)&bse_new_order_single_short_.bse_new_order_single_short_request_+16, (int32_t)send_write_length-16);

  memcpy((void*)(send_buffer+16), (void*)open_ssl_crypto_.encrypt_text, open_ssl_crypto_.encrypt_len);


  if (tcp_direct_client_zocket_.WriteLockFreeN(16+open_ssl_crypto_.encrypt_len, (void*)send_buffer) < (int32_t)send_write_length) {
    DBGLOG_CLASS_FUNC_LINE_FATAL << "FAILD TO WRITE COMPLETE SENTORDER MESSAGE TO EXCHANGE SOCKET" << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    exit(1);  // no point continuing
  }

  rp_order_->ors_end_cycle_count_ = HFSAT::GetCpucycleCountForTimeTick();
  rp_order_->message_sequence_number_ = next_seq_num_;
  rp_order_->send_modify_cancel = 'S';
  seqno_to_saos_ordertype_[next_seq_num_] = make_pair(rp_order_->server_assigned_order_sequence_, 'S');
  saos_to_symbol_[rp_order_->server_assigned_order_sequence_] = std::string(rp_order_->symbol_);
#if BSE_DEBUG_INFO
  DBGLOG_CLASS_FUNC_LINE_INFO << "BSEEngine::SendOrder " << rp_order_->message_sequence_number_ << " "
                                                         << rp_order_->send_modify_cancel << DBGLOG_ENDL_DUMP;
#endif
  next_seq_num_++;

}


void BSEEngine::SendMktOrder(HFSAT::ORS::Order *rp_order_, const int32_t& prod_token_, const int32_t& prod_id_) {

//  bse_new_order_single_.setBSENewOrderSingleDynamicFields(rp_order_->server_assigned_order_sequence_,
//                                                          rp_order_->size_remaining_, rp_order_->buysell_,
//                                                          next_seq_num_, prod_id_, prod_token_);
  bse_new_order_single_.setBSENewOrderSingleDynamicFields(rp_order_, next_seq_num_, prod_id_, prod_token_);

  char send_buffer[1024];
  memset((void*)send_buffer,0,1024); 
  memcpy((void*)send_buffer,&bse_new_order_single_.bse_new_order_single_request_.MessageHeaderIn, 16);

  open_ssl_crypto_.aes_encrypt((unsigned char*)&bse_new_order_single_.bse_new_order_single_request_+16, (int32_t)send_mkt_write_length-16);

  memcpy((void*)(send_buffer+16), (void*)open_ssl_crypto_.encrypt_text, open_ssl_crypto_.encrypt_len);

  if (tcp_direct_client_zocket_.WriteLockFreeN(16+open_ssl_crypto_.encrypt_len, (void*)send_buffer) < (int32_t)send_mkt_write_length) {
    DBGLOG_CLASS_FUNC_LINE_FATAL << "FAILD TO WRITE COMPLETE SENTMKTORDER MESSAGE TO EXCHANGE SOCKET" << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    exit(1);  // no point continuing
  }

  rp_order_->ors_end_cycle_count_ = HFSAT::GetCpucycleCountForTimeTick();
  rp_order_->message_sequence_number_ = next_seq_num_;
  rp_order_->send_modify_cancel = 'S';
  seqno_to_saos_ordertype_[next_seq_num_] = make_pair(rp_order_->server_assigned_order_sequence_, 'S');

#if BSE_DEBUG_INFO
  DBGLOG_CLASS_FUNC_LINE_INFO << "BSEEngine::SendMktOrder " << rp_order_->message_sequence_number_ << " "
                                                         << rp_order_->send_modify_cancel << DBGLOG_ENDL_DUMP;
#endif
  next_seq_num_++;


  uint8_t buy_sell_ = (rp_order_->buysell_ == HFSAT::kTradeTypeBuy) ? 0 : 1;
  DBGLOG_CLASS_FUNC_LINE_INFO << "MKTORDER REQUEST: " << rp_order_->message_sequence_number_
                              << " Token: " << prod_token_ << " BuySell: " << buy_sell_ << " Price: " << rp_order_->price_ 
                              << " Size: " << rp_order_->size_remaining_
                              << " Size Disclosed: " << rp_order_->size_disclosed_
                              << " Saos: " << rp_order_->server_assigned_order_sequence_
                              << DBGLOG_ENDL_FLUSH;
}

void BSEEngine::CancelOrder(HFSAT::ORS::Order *rp_order_, const int32_t& prod_token_, const int32_t& prod_id_) {

#if BSE_DEBUG_INFO
  uint8_t buy_sell_ = (rp_order_->buysell_ == HFSAT::kTradeTypeBuy) ? 0 : 1;
  DBGLOG_CLASS_FUNC_LINE_INFO << "CANCEL ORDER REQUEST::\n" << rp_order_->toString() << DBGLOG_ENDL_FLUSH;
  DBGLOG_CLASS_FUNC_LINE << "Token: " << prod_token_ << " BuySell: " << buy_sell_ << " Price: " << rp_order_->price_ << " Size: " << rp_order_->size_remaining_
                         << " Size Disclosed: " << rp_order_->size_disclosed_
                         << " Saos: " << rp_order_->server_assigned_order_sequence_
                         << " OrderNum: " << (int64_t)rp_order_->exch_assigned_seq_ << " Entry: " << rp_order_->entry_dt_
                         << " LastModified: " << rp_order_->last_mod_dt_  
                         << DBGLOG_ENDL_FLUSH;
#endif
/*
  bse_cancel_order_single_.setBSECancelOrderSingleDynamicFields(next_seq_num_, rp_order_->exch_assigned_seq_,
                                                                rp_order_->server_assigned_order_sequence_,
                                                                rp_order_->server_assigned_order_sequence_, rp_order_->last_activity_reference_,
                                                                prod_id_, prod_token_);
*/
  bse_cancel_order_single_.setBSECancelOrderSingleDynamicFields(rp_order_, next_seq_num_,
                                                                prod_id_, prod_token_);

  char send_buffer[1024];
  memset((void*)send_buffer,0,1024); 
  memcpy((void*)send_buffer,&bse_cancel_order_single_.bse_cancel_order_single_request_.MessageHeaderIn, 16);

  open_ssl_crypto_.aes_encrypt((unsigned char*)&bse_cancel_order_single_.bse_cancel_order_single_request_+16, (int32_t)cancel_write_length-16);

  memcpy((void*)(send_buffer+16), (void*)open_ssl_crypto_.encrypt_text, open_ssl_crypto_.encrypt_len);

  if (tcp_direct_client_zocket_.WriteLockFreeN(16+open_ssl_crypto_.encrypt_len, (void*)send_buffer) < (int32_t)cancel_write_length) {
    DBGLOG_CLASS_FUNC_LINE_FATAL << "FAILD TO WRITE COMPLETE CANCELORDER MESSAGE TO EXCHANGE SOCKET" << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    exit(1);  // no point continuing
  }

  rp_order_->ors_end_cycle_count_ = HFSAT::GetCpucycleCountForTimeTick();
  rp_order_->message_sequence_number_ = next_seq_num_;
  rp_order_->send_modify_cancel = 'C';
  seqno_to_saos_ordertype_[next_seq_num_] = make_pair(rp_order_->server_assigned_order_sequence_, 'C');
#if BSE_DEBUG_INFO
  DBGLOG_CLASS_FUNC_LINE_INFO << "BSEEngine::CancelOrder " << rp_order_->message_sequence_number_ << " "
                                                         << rp_order_->send_modify_cancel << DBGLOG_ENDL_DUMP;
#endif

  next_seq_num_++;

}

void BSEEngine::ModifyOrder(HFSAT::ORS::Order* rp_order_, const int32_t& prod_token_, HFSAT::ORS::Order* rp_order2_) {

#if BSE_DEBUG_INFO
  uint8_t buy_sell_ = (rp_order_->buysell_ == HFSAT::kTradeTypeBuy) ? 0 : 1;
  DBGLOG_CLASS_FUNC_LINE_INFO << "BSEEngine::ModifyOrder " << "SeqNo: " << next_seq_num_ << " " << "OrderId: " << rp_order_->exch_assigned_seq_ << " "
                                                           << "SAOS: " << rp_order_->server_assigned_order_sequence_ << " "
                                                           << "OrigSAOS: " << rp_order_->server_assigned_order_sequence_ << " " 
                                                           << "Price: " << rp_order_->price_ << " "
                                                           << "Time: " << rp_order_->last_activity_reference_ << " " 
                                                           << "Size: " << rp_order_->size_remaining_ << " "
                                                           << "SecId: " << prod_token_ << " " 
                                                           << "Buy/Sell: " << buy_sell_ << DBGLOG_ENDL_DUMP;
#endif
/*
  bse_modify_order_single_short_.setBSEModifyOrderSingleShortDynamicFields(
       next_seq_num_, rp_order_->exch_assigned_seq_, rp_order_->server_assigned_order_sequence_, rp_order_->server_assigned_order_sequence_,
       rp_order_->price_, rp_order_->last_activity_reference_, rp_order_->size_remaining_,
       prod_token_, rp_order_->buysell_);
*/
  bse_modify_order_single_short_.setBSEModifyOrderSingleShortDynamicFields( rp_order_, next_seq_num_, prod_token_);

  char send_buffer[1024];
  memset((void*)send_buffer,0,1024); 
  memcpy((void*)send_buffer,&bse_modify_order_single_short_.bse_modify_order_single_short_request_.MessageHeaderIn, 16);

  open_ssl_crypto_.aes_encrypt((unsigned char*)&bse_modify_order_single_short_.bse_modify_order_single_short_request_+16, (int32_t)modify_write_length-16);

  memcpy((void*)(send_buffer+16), (void*)open_ssl_crypto_.encrypt_text, open_ssl_crypto_.encrypt_len);

  if (tcp_direct_client_zocket_.WriteLockFreeN(16+open_ssl_crypto_.encrypt_len, (void*)send_buffer) < modify_write_length) {
    DBGLOG_CLASS_FUNC_LINE_FATAL << "FAILD TO WRITE COMPLETE MODIFYORDER MESSAGE TO EXCHANGE SOCKET" << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    exit(1);  // no point continuing
  }

  rp_order2_->ors_end_cycle_count_ = HFSAT::GetCpucycleCountForTimeTick();
  rp_order_->message_sequence_number_ = next_seq_num_;
  rp_order2_->message_sequence_number_ = next_seq_num_;
  rp_order_->send_modify_cancel = 'M';
  rp_order2_->send_modify_cancel = 'M';
  seqno_to_saos_ordertype_[next_seq_num_] = make_pair(rp_order_->server_assigned_order_sequence_, 'M');
#if BSE_DEBUG_INFO
  DBGLOG_CLASS_FUNC_LINE_INFO << "BSEEngine::ModifyOrder " << rp_order_->message_sequence_number_ << " "
                                                         << rp_order_->send_modify_cancel << DBGLOG_ENDL_DUMP;
#endif

  next_seq_num_++;

}

void BSEEngine::thread_main() {}

int BSEEngine::processBSEExchangeMsg(char *bse_msg_char_buf, const int &this_msg_length_) {

  
  int length_to_be_processed_ = this_msg_length_;

  const char *exch_msg_buffer_ = bse_msg_char_buf;

  // process multiple messages till the length to be processed is >0, reduce as we process the messages
  while (length_to_be_processed_ > 0) {
    // Broken Header - Don't even know how much more data is to be read
    if ((uint32_t)length_to_be_processed_ < sizeof(uint32_t)) {
      dbglogger_ << " Incomplete Header, requires : " << sizeof(uint32_t)
                 << " and we have : " << length_to_be_processed_ << " Initial Length : " << this_msg_length_
                 << " go for socket read \n";
      dbglogger_.DumpCurrentBuffer();

      memmove((void *)(bse_msg_char_buf), (void *)(exch_msg_buffer_), length_to_be_processed_);
      return length_to_be_processed_;
    }

    // At least we have header now

    // TODO for most messages pre-computed length can be used, but this is safer and not any worse
    uint32_t this_bse_message_bodylength_ = *((uint32_t *)(exch_msg_buffer_));

    // Incomplete Message, requires more socket read
    if ((uint32_t)length_to_be_processed_ < this_bse_message_bodylength_) {
      dbglogger_ << " Incomplete Message, requires : " << this_bse_message_bodylength_
                 << " and we have : " << length_to_be_processed_ << " Initial Length : " << this_msg_length_
                 << " go for socket read \n";
      dbglogger_.DumpCurrentBuffer();

      // memset can be done here, but if there isn't any issue with partial handling it's not necessary since we know
      // length to process
      memmove((void *)(bse_msg_char_buf), (void *)(exch_msg_buffer_), length_to_be_processed_);
      return length_to_be_processed_;  // left over bytes
    }

    uint16_t this_bse_template_id_ = *((uint16_t *)(exch_msg_buffer_ + 4));  // first 4 bytes are for body length

    // replies are in Network order, match with the pre-computed network bytes for template id

    switch (this_bse_template_id_) {
      case TID_GW_ORDER_ACKNOWLEDGEMENT: {
        //processBSEOrderConfirmationResponse(exch_msg_buffer_);
        length_to_be_processed_ -= this_bse_message_bodylength_;
        exch_msg_buffer_ += this_bse_message_bodylength_;

      } break;

      case TID_MODIFY_ORDER_NR_RESPONSE: {
        processBSEModifyOrderNRResponse(exch_msg_buffer_);
        length_to_be_processed_ -= this_bse_message_bodylength_;
        exch_msg_buffer_ += this_bse_message_bodylength_;

      } break;

      case TID_ORDER_EXEC_NOTIFICATION:  // this is book order execution
      {
        processBSEOrderExecutionResponse(exch_msg_buffer_);
        length_to_be_processed_ -= this_bse_message_bodylength_;
        exch_msg_buffer_ += this_bse_message_bodylength_;

      } break;

      case TID_NEW_ORDER_NR_RESPONSE:  // this is for standard order
      {
        processBSENewOrderNRResponse(exch_msg_buffer_);
        length_to_be_processed_ -= this_bse_message_bodylength_;
        exch_msg_buffer_ += this_bse_message_bodylength_;

      } break;

      case TID_DELETE_ORDER_NR_RESPONSE:  // this is for lean orders
      {
        processBSECancelOrderSingleNRResponse(exch_msg_buffer_);
        length_to_be_processed_ -= this_bse_message_bodylength_;
        exch_msg_buffer_ += this_bse_message_bodylength_;

      } break;

      case TID_REJECT: {
        processBSERejectResponse(exch_msg_buffer_);
        length_to_be_processed_ -= this_bse_message_bodylength_;
        exch_msg_buffer_ += this_bse_message_bodylength_;

      } break;

      case TID_ORDER_EXEC_RESPONSE:  // this is book order execution
      {
        processBSEImmediateExecutionResponse(exch_msg_buffer_);
        length_to_be_processed_ -= this_bse_message_bodylength_;
        exch_msg_buffer_ += this_bse_message_bodylength_;

      } break;

      case TID_ORDER_EXEC_REPORT_BROADCAST: {
        processBSEExtendedOrderInfoResponse(exch_msg_buffer_);
        length_to_be_processed_ -= this_bse_message_bodylength_;
        exch_msg_buffer_ += this_bse_message_bodylength_;
 
      } break;

      case TID_DELETE_ALL_ORDER_QUOTE_EVENT_BROADCAST: {

        if (is_alert_batch_cancellation){
                is_alert_batch_cancellation=false;
                p_engine_listener_->OnBatchCxlAlert(username_);
        }
        DBGLOG_CLASS_FUNC_LINE_ERROR << "Received Batch ORDER CANCELLATION : "
                                     << username_ << "\n" << DBGLOG_ENDL_FLUSH;
        processBSEMassCancellationResponse(exch_msg_buffer_);
        length_to_be_processed_ -= this_bse_message_bodylength_;
        exch_msg_buffer_ += this_bse_message_bodylength_;

      } break;

      case TID_HEARTBEAT_NOTIFICATION: {
        processBSEHeartbeatResponse(exch_msg_buffer_);
        length_to_be_processed_ -= this_bse_message_bodylength_;
        exch_msg_buffer_ += this_bse_message_bodylength_;

      } break;

      case TID_GATEWAY_RESPONSE: {
        processBSEConnectionGatewayResponse(exch_msg_buffer_);
        length_to_be_processed_ -= this_bse_message_bodylength_;
        exch_msg_buffer_ += this_bse_message_bodylength_;

      } break;
      case TID_LOGON_RESPONSE: {
        processBSESessionLogonResponse(exch_msg_buffer_);
        length_to_be_processed_ -= this_bse_message_bodylength_;
        exch_msg_buffer_ += this_bse_message_bodylength_;

      } break;

      case TID_USER_LOGIN_RESPONSE: {
        processBSEUserLoginResponse(exch_msg_buffer_);
        length_to_be_processed_ -= this_bse_message_bodylength_;
        exch_msg_buffer_ += this_bse_message_bodylength_;

      } break;

      case TID_USER_LOGOUT_RESPONSE: {
        processBSEUserLogoutResponse(exch_msg_buffer_);
        length_to_be_processed_ -= this_bse_message_bodylength_;
        exch_msg_buffer_ += this_bse_message_bodylength_;

      } break;

      case TID_LOGOUT_RESPONSE: {
        processBSESessionLogoutResponse(exch_msg_buffer_);
        length_to_be_processed_ -= this_bse_message_bodylength_;
        exch_msg_buffer_ += this_bse_message_bodylength_;

      } break;

      case TID_FORCED_LOGOUT_NOTIFICATION: {
        processBSESessionForcedLogoutResponse(exch_msg_buffer_);
        length_to_be_processed_ -= this_bse_message_bodylength_;
        exch_msg_buffer_ += this_bse_message_bodylength_;

      } break;

      case TID_USER_PASSWORD_CHANGE_RESPONSE: {
        processBSEUserPasswordChangeResponse(exch_msg_buffer_);
        length_to_be_processed_ -= this_bse_message_bodylength_;
        exch_msg_buffer_ += this_bse_message_bodylength_;
      } break;

      case TID_SESSION_PASSWORD_CHANGE_RESPONSE: {
        processBSESessionPasswordChangeResponse(exch_msg_buffer_);
        length_to_be_processed_ -= this_bse_message_bodylength_;
        exch_msg_buffer_ += this_bse_message_bodylength_;
      } break;

      case TID_SESSION_REGISTRATION_RESPONSE:{
        processBSESessionRegistrationResponse(exch_msg_buffer_);
        length_to_be_processed_ -= this_bse_message_bodylength_;
        exch_msg_buffer_ += this_bse_message_bodylength_;
      }break;
      
      case TID_RISK_COLLATERAL_ALERT_ADMIN_BROADCAST: {
        processBSERiskCollateralAlertResponse(exch_msg_buffer_);
        length_to_be_processed_ -= this_bse_message_bodylength_;
        exch_msg_buffer_ += this_bse_message_bodylength_;
      } break;

      case TID_RISK_COLLATERAL_ALERT_BROADCAST: {
        processBSERiskCollateralAlertTraderResponse(exch_msg_buffer_);
        length_to_be_processed_ -= this_bse_message_bodylength_;
        exch_msg_buffer_ += this_bse_message_bodylength_;
      } break;

      default: {
        dbglogger_ << " ERROR : Engine Failure Unhandled Template " << this_bse_template_id_
                   << " Length To be Processed : " << length_to_be_processed_
                   << " Initial Length : " << this_msg_length_ << " Current Length : " << this_bse_message_bodylength_
                   << "\n";
        dbglogger_.DumpCurrentBuffer();

        std::cout << " Hex Bytes : "
                  << "\n";
        printHexString(bse_msg_char_buf, this_msg_length_);

        std::cout << " String Format : "
                  << "\n";
        printStringFromByte(bse_msg_char_buf, this_msg_length_);

        // Best We can do here, assuming something hasn't gone wrong already otherwise corrupting memory which can be
        // critical
        length_to_be_processed_ -= this_bse_message_bodylength_;
        exch_msg_buffer_ += this_bse_message_bodylength_;

      } break;
    }
  }

  // This will mark complete message processing
  return 0;
}


uint32_t BSEEngine::ProcessEncryptedExchangeResponse(char *bse_msg_char_buf, const int &this_msg_length_) {

  //dbglogger_ << "BSEEngine::ProcessEncryptedExchangeResponse " << "\n";
  int length_to_be_processed_ = this_msg_length_;
  const char *exch_msg_buffer_ = bse_msg_char_buf;


  // process multiple messages till the length to be processed is >0, reduce as we process the messages
  while (length_to_be_processed_ > 0) {

    // Broken Header - Don't even know how much more data is to be read
    if ((uint32_t)length_to_be_processed_ < sizeof(uint32_t)) {
      dbglogger_ << " Incomplete Header, requires : " << sizeof(uint32_t)
                 << " and we have : " << length_to_be_processed_ << " Initial Length : " << this_msg_length_
                 << " go for socket read \n";
      dbglogger_.DumpCurrentBuffer();

      memmove((void *)(bse_msg_char_buf), (void *)(exch_msg_buffer_), length_to_be_processed_);
      return length_to_be_processed_;
    }

    // At least we have header now

    // TODO for most messages pre-computed length can be used, but this is safer and not any worse
    uint32_t this_bse_message_bodylength_ = *((uint32_t *)(exch_msg_buffer_));
    
    // Incomplete Message, requires more socket read
    if ((uint32_t)length_to_be_processed_ < this_bse_message_bodylength_) {
      dbglogger_ << " Incomplete Message, requires : " << this_bse_message_bodylength_
                 << " and we have : " << length_to_be_processed_ << " Initial Length : " << this_msg_length_
                 << " go for socket read \n";
      dbglogger_.DumpCurrentBuffer();

      // memset can be done here, but if there isn't any issue with partial handling it's not necessary since we know
      // length to process
      memmove((void *)(bse_msg_char_buf), (void *)(exch_msg_buffer_), length_to_be_processed_);
      return length_to_be_processed_;  // left over bytes
    }

    uint16_t this_bse_template_id_ = *((uint16_t *)(exch_msg_buffer_ + 4));  // first 4 bytes are for body length

    //dbglogger_ << "TEMPLATE ID " << this_bse_template_id_ << "\n";

   //char input_buffer[8192];
   //memcpy(input_buffer,exch_msg_buffer_,this_bse_message_bodylength_);

    memset(&input_buffer,0,sizeof(input_buffer));
    char const * msg_ptr = input_buffer;

    //Copying the header which is not encrypted
    memcpy((void*)msg_ptr,(void*)exch_msg_buffer_,BSE_PACKET_RESPONSE_LENGTH);
   //decrypt all msg except secure box registration and Gateway response
   if(false == is_secure_box_registered_){
     memcpy((void*)msg_ptr,(void*)exch_msg_buffer_,this_bse_message_bodylength_);
   }else {
     open_ssl_crypto_.aes_decrypt((unsigned char*)exch_msg_buffer_+BSE_PACKET_RESPONSE_LENGTH, this_bse_message_bodylength_ - BSE_PACKET_RESPONSE_LENGTH);
     //dbglogger_ << "SIZE OF DECRYPTED MESSAGE " << open_ssl_crypto_.decrypt_len;
     memcpy((void*)(msg_ptr+BSE_PACKET_RESPONSE_LENGTH),(void*)open_ssl_crypto_.decrypt_data,open_ssl_crypto_.decrypt_len);     
   }

    tcp_direct_client_zocket_.LogAuditIn(msg_ptr,BSE_PACKET_RESPONSE_LENGTH,msg_ptr+BSE_PACKET_RESPONSE_LENGTH, this_bse_message_bodylength_-BSE_PACKET_RESPONSE_LENGTH);

    // Test the message type against the pre-computed network order
    switch (this_bse_template_id_) {
      case TID_GW_ORDER_ACKNOWLEDGEMENT: {
        length_to_be_processed_ -= this_bse_message_bodylength_;
        exch_msg_buffer_  += this_bse_message_bodylength_;
      } break;

      case TID_MODIFY_ORDER_NR_RESPONSE: {
        processBSEModifyOrderNRResponse((const char*)msg_ptr);
        length_to_be_processed_ -= this_bse_message_bodylength_;
        exch_msg_buffer_ += this_bse_message_bodylength_;

      } break;

      
      case TID_ORDER_EXEC_NOTIFICATION:  // this is book order execution
      {
        processBSEOrderExecutionResponse((const char*)msg_ptr);
        length_to_be_processed_ -= this_bse_message_bodylength_;
        exch_msg_buffer_ += this_bse_message_bodylength_;

      } break;

      case TID_NEW_ORDER_NR_RESPONSE:  // this is for standard order
      {
        processBSENewOrderNRResponse((const char*)msg_ptr);
        length_to_be_processed_ -= this_bse_message_bodylength_;
        exch_msg_buffer_ += this_bse_message_bodylength_;

      } break;

      case TID_DELETE_ORDER_NR_RESPONSE:  // this is for lean orders
      {
        processBSECancelOrderSingleNRResponse((const char*)msg_ptr);
        length_to_be_processed_ -= this_bse_message_bodylength_;
        exch_msg_buffer_ += this_bse_message_bodylength_;

      } break;

      case TID_REJECT: {
        processBSERejectResponse((const char*)msg_ptr);
        length_to_be_processed_ -= this_bse_message_bodylength_;
        exch_msg_buffer_ += this_bse_message_bodylength_;

      } break;

      case TID_ORDER_EXEC_RESPONSE:  // this is book order execution
      {
        processBSEImmediateExecutionResponse((const char*)msg_ptr);
        length_to_be_processed_ -= this_bse_message_bodylength_;
        exch_msg_buffer_ += this_bse_message_bodylength_;

      } break;

      case TID_ORDER_EXEC_REPORT_BROADCAST: {
        processBSEExtendedOrderInfoResponse((const char*)msg_ptr);
        length_to_be_processed_ -= this_bse_message_bodylength_;
        exch_msg_buffer_ += this_bse_message_bodylength_;
 
      } break;

      case TID_DELETE_ALL_ORDER_QUOTE_EVENT_BROADCAST: {

        if (is_alert_batch_cancellation){
                is_alert_batch_cancellation=false;
                p_engine_listener_->OnBatchCxlAlert(username_);
        }
        DBGLOG_CLASS_FUNC_LINE_ERROR << "Received Batch ORDER CANCELLATION : "
                                     << username_ << "\n" << DBGLOG_ENDL_FLUSH;
        processBSEMassCancellationResponse((const char*)msg_ptr);
        length_to_be_processed_ -= this_bse_message_bodylength_;
        exch_msg_buffer_ += this_bse_message_bodylength_;

      } break;

      case TID_HEARTBEAT_NOTIFICATION: {
        processBSEHeartbeatResponse((const char*)msg_ptr);
        length_to_be_processed_ -= this_bse_message_bodylength_;
        exch_msg_buffer_ += this_bse_message_bodylength_;

      } break;

      case TID_GATEWAY_RESPONSE: {
        processBSEConnectionGatewayResponse((const char*)msg_ptr);
        length_to_be_processed_ -= this_bse_message_bodylength_;
        exch_msg_buffer_ += this_bse_message_bodylength_;

      } break;
      case TID_LOGON_RESPONSE: {
        processBSESessionLogonResponse((const char*)msg_ptr);
        length_to_be_processed_ -= this_bse_message_bodylength_;
        exch_msg_buffer_ += this_bse_message_bodylength_;

      } break;

      case TID_USER_LOGIN_RESPONSE: {
        processBSEUserLoginResponse((const char*)msg_ptr);
        length_to_be_processed_ -= this_bse_message_bodylength_;
        exch_msg_buffer_ += this_bse_message_bodylength_;

      } break;

      case TID_USER_LOGOUT_RESPONSE: {
        processBSEUserLogoutResponse((const char*)msg_ptr);
        length_to_be_processed_ -= this_bse_message_bodylength_;
        exch_msg_buffer_ += this_bse_message_bodylength_;

      } break;

      case TID_LOGOUT_RESPONSE: {
        processBSESessionLogoutResponse((const char*)msg_ptr);
        length_to_be_processed_ -= this_bse_message_bodylength_;
        exch_msg_buffer_ += this_bse_message_bodylength_;

      } break;

      case TID_FORCED_LOGOUT_NOTIFICATION: {
        processBSESessionForcedLogoutResponse(exch_msg_buffer_);
        length_to_be_processed_ -= this_bse_message_bodylength_;
        exch_msg_buffer_ += this_bse_message_bodylength_;

      } break;

      case TID_USER_PASSWORD_CHANGE_RESPONSE: {
        processBSEUserPasswordChangeResponse((const char*)msg_ptr);
        length_to_be_processed_ -= this_bse_message_bodylength_;
        exch_msg_buffer_ += this_bse_message_bodylength_;
      } break;

      case TID_SESSION_PASSWORD_CHANGE_RESPONSE: {
        processBSESessionPasswordChangeResponse((const char*)msg_ptr);
        length_to_be_processed_ -= this_bse_message_bodylength_;
        exch_msg_buffer_ += this_bse_message_bodylength_;
      } break;

      case TID_SESSION_REGISTRATION_RESPONSE:{
        processBSESessionRegistrationResponse((const char*)msg_ptr);
        length_to_be_processed_ -= this_bse_message_bodylength_;
        exch_msg_buffer_ += this_bse_message_bodylength_;
      }break;
      
      case TID_RISK_COLLATERAL_ALERT_ADMIN_BROADCAST: {
        processBSERiskCollateralAlertResponse((const char*)msg_ptr);
        length_to_be_processed_ -= this_bse_message_bodylength_;
        exch_msg_buffer_ += this_bse_message_bodylength_;
      } break;

      case TID_RISK_COLLATERAL_ALERT_BROADCAST: {
        processBSERiskCollateralAlertTraderResponse((const char*)msg_ptr);
        length_to_be_processed_ -= this_bse_message_bodylength_;
        exch_msg_buffer_ += this_bse_message_bodylength_;
      } break;

      default: {
        dbglogger_ << " ERROR : Engine Failure Unhandled Template " << this_bse_template_id_
                   << " Length To be Processed : " << length_to_be_processed_
                   << " Initial Length : " << this_msg_length_ << " Current Length : " << this_bse_message_bodylength_
                   << "\n";
        dbglogger_.DumpCurrentBuffer();

        std::cout << " Hex Bytes : "
                  << "\n";
        printHexString(bse_msg_char_buf, this_msg_length_);

        std::cout << " String Format : "
                  << "\n";
        printStringFromByte(bse_msg_char_buf, this_msg_length_);

        // Best We can do here, assuming something hasn't gone wrong already otherwise corrupting memory which can be
        // critical
        length_to_be_processed_ -= this_bse_message_bodylength_;
        exch_msg_buffer_ += this_bse_message_bodylength_;

      } break;
    }
  }

  return 0;  // Message Was Processed Completely
}


void BSEEngine::processBSEConnectionGatewayResponse(const char *bse_msg_char_buf) {
    
  GatewayResponseT *this_gateway_response_ = (GatewayResponseT *)(bse_msg_char_buf);
  dbglogger_ << "GatewayResponse BodyLen: " << (this_gateway_response_->MessageHeaderOut).BodyLen << " " <<
                "TemplateID: " << (this_gateway_response_->MessageHeaderOut).TemplateID << " " <<
                "MsgSeqNum: " << (this_gateway_response_->ResponseHeader).MsgSeqNum << " " <<
                "GatewayUserId: " << username_ << " " << 
                "GatewayID: " << this_gateway_response_->GatewayID << " " << 
                "GatewaySubID: " << this_gateway_response_->GatewaySubID << " " <<
                "SecondaryGatewayID: " << this_gateway_response_->SecondaryGatewayID << " " <<
                "SecondaryGatewaySubID: " << this_gateway_response_->SecondaryGatewaySubID << " " <<
                "SessionMode: " << this_gateway_response_->SessionMode << " " <<
                "TradSesMode: " << this_gateway_response_->TradSesMode << " " <<
                "SecurityKey: " << this_gateway_response_->cryptographic_key << " " <<
                "InitializationVector " << this_gateway_response_->cryptographic_iv << " " <<
                "GatewayResponseT: " << sizeof(GatewayResponseT) << " " << "\n";

  std::ostringstream t_temp_session_socket_ip_stream_;
  // this four bytes would contain the information for IP address, process each byte as an int to get the ip address
  t_temp_session_socket_ip_stream_ << ((unsigned int)(*(uint8_t *)(bse_msg_char_buf + 35))) << "."
                                   << ((unsigned int)(*(uint8_t *)(bse_msg_char_buf + 34))) << "."
                                   << ((unsigned int)(*(uint8_t *)(bse_msg_char_buf + 33))) << "."
                                   << ((unsigned int)(*(uint8_t *)(bse_msg_char_buf + 32)));

  bse_session_logon_request_primary_ip_address_ = t_temp_session_socket_ip_stream_.str();
  bse_session_logon_request_primary_port_ = this_gateway_response_->GatewaySubID;

  std::ostringstream t_temp_sec_gateway_;

  t_temp_sec_gateway_ << ((unsigned int)(*(uint8_t *)(bse_msg_char_buf + 43))) << "."
                      << ((unsigned int)(*(uint8_t *)(bse_msg_char_buf + 42))) << "."
                      << ((unsigned int)(*(uint8_t *)(bse_msg_char_buf + 41))) << "."
                      << ((unsigned int)(*(uint8_t *)(bse_msg_char_buf + 40)));

  bse_session_logon_request_secondary_ip_address_ = t_temp_sec_gateway_.str();
  bse_session_logon_request_secondary_port_ = this_gateway_response_->SecondaryGatewaySubID;;

  dbglogger_ << " Primary Gateway : " << t_temp_session_socket_ip_stream_.str() << " "
             << bse_session_logon_request_primary_port_ << "\n";
  dbglogger_ << "Secondary Gateway : " << t_temp_sec_gateway_.str() << " " 
             << bse_session_logon_request_secondary_port_ << "\n";
  dbglogger_.DumpCurrentBuffer();


  memcpy((void *)crypto_key_, (void *)(bse_msg_char_buf+50), 32);
  memcpy((void *)crypto_iv_, (void *)(bse_msg_char_buf+82), 16);

  //Initialize Crypto
  open_ssl_crypto_.encrypt_EVP_aes_256_gcm_init_BSE_with_fixed_iv_length((unsigned char*)crypto_key_,(unsigned char*)crypto_iv_);
  open_ssl_crypto_.decrypt_EVP_aes_256_gcm_init_BSE_with_fixed_iv_length((unsigned char*)crypto_key_,(unsigned char*)crypto_iv_);

  dbglogger_ << "Going For SecureBoxRegistrationRequest " << "\n";
  //dbglogger_.DumpCurrentBuffer(); 

  SessionRegistrationRequest(); 
  //SessionLogin();  // BSE has a two stop login process, create a new socket using the gateway assigned ip/port
}

void BSEEngine::processBSESessionLogonResponse(const char *bse_msg_char_buf) {
  LogonResponseT *this_session_logon_response_ = (LogonResponseT *)(bse_msg_char_buf);
  dbglogger_ << "SessionLogonResponse BodyLen: " << (this_session_logon_response_->MessageHeaderOut).BodyLen << " " <<
                "TemplateID: " << (this_session_logon_response_->MessageHeaderOut).TemplateID << " " << 
                "MsgSeqNum: " << (this_session_logon_response_->ResponseHeader).MsgSeqNum << " " << 
                "SessionUserId: " << username_ << " " << 
                "RequestTime: " << (this_session_logon_response_->ResponseHeader).RequestTime << " " <<
                "SendingTime: " << (this_session_logon_response_->ResponseHeader).SendingTime << " " <<
                "ThrottleTimeInterval: " << this_session_logon_response_->ThrottleTimeInterval << " " <<
                "LastLoginTime: " << this_session_logon_response_->LastLoginTime << " " <<
                "LastLoginIP: " << this_session_logon_response_->LastLoginIP << " " <<
                "ThrottleNoMsgs: " << this_session_logon_response_->ThrottleNoMsgs << " " <<
                "ThrottleDisconnectLimit: " << this_session_logon_response_->ThrottleDisconnectLimit << " " <<
                "HeartBtInt: " << this_session_logon_response_->HeartBtInt << " " <<
                "SessionInstanceID: " << this_session_logon_response_->SessionInstanceID << " " <<
                "TradSesMode: " << this_session_logon_response_->TradSesMode << " " <<
                "NoOfPartition: " << this_session_logon_response_->NoOfPartition << " " <<
                "DaysLeftForPasswdExpiry: " << this_session_logon_response_->DaysLeftForPasswdExpiry << " " <<
                "GraceLoginsLeft: " << this_session_logon_response_->GraceLoginsLeft << " " <<
                "DefaultCstmApplVerID: " << this_session_logon_response_->DefaultCstmApplVerID << " " <<
                "LogonResponseT: " << sizeof(LogonResponseT) << " " << "\n";
  dbglogger_.DumpCurrentBuffer();

  std::cout << "Session Logon response completed\n";
  dbglogger_ << "Session Logon response completed\n";
  //is_secure_box_registered_ = true;

  Login();
}

void BSEEngine::processBSESessionLogoutResponse(const char *bse_msg_char_buf) {
  LogoutResponseT *this_session_logout_response_ = (LogoutResponseT *)(bse_msg_char_buf);
  dbglogger_ << "SessionLogoutResponse BodyLen: " << (this_session_logout_response_->MessageHeaderOut).BodyLen << " " <<
                "TemplateID: " << (this_session_logout_response_->MessageHeaderOut).TemplateID << " " <<
                "MsgSeqNum: " << (this_session_logout_response_->ResponseHeader).MsgSeqNum << " " <<
                "SessionUserId: " << username_ << " " << 
                "RequestTime: " << (this_session_logout_response_->ResponseHeader).RequestTime << " " <<
                "SendingTime: " << (this_session_logout_response_->ResponseHeader).SendingTime << " " <<
                "LogoutResponseT: " << sizeof(LogoutResponseT) << " " << "\n";
  dbglogger_.DumpCurrentBuffer();

  is_logged_in_ = false;
  p_engine_listener_->OnLogout();

  is_connected_ = false;
  p_engine_listener_->OnDisconnect();
  tcp_direct_client_zocket_.Close();

}

void BSEEngine::processBSESessionForcedLogoutResponse(const char *bse_msg_char_buf) {
  ForcedLogoutNotificationT *this_session_forced_logout_response_ = (ForcedLogoutNotificationT *)(bse_msg_char_buf);
  dbglogger_ << "SessionForcedLogoutResponse BodyLen: " << (this_session_forced_logout_response_->MessageHeaderOut).BodyLen << " " <<
                "TemplateID: " << (this_session_forced_logout_response_->MessageHeaderOut).TemplateID << " " <<
                "SessionUserId: " << username_ << " " << 
                "VarText: " << this_session_forced_logout_response_->VarText << " " <<
                "ForcedLogoutNotificationT: " << sizeof(ForcedLogoutNotificationT) << " " << "\n";
  dbglogger_.DumpCurrentBuffer();

  is_logged_in_ = false;
  p_engine_listener_->OnLogout();

  is_connected_ = false;
  p_engine_listener_->OnDisconnect();
  tcp_direct_client_zocket_.Close();

}

void BSEEngine::processBSESessionPasswordChangeResponse(const char *bse_msg_char_buf) {
  SessionPasswordChangeResponseT *this_passwd_response_ = (SessionPasswordChangeResponseT *)(bse_msg_char_buf);
  dbglogger_ << "SesionPasswordChangeResponse BodyLen: " << (this_passwd_response_->MessageHeaderOut).BodyLen << " " <<
                "TemplateID: " << (this_passwd_response_->MessageHeaderOut).TemplateID << " " <<
                "MsgSeqNum: " << (this_passwd_response_->ResponseHeader).MsgSeqNum << " " <<
                "SessionUserId: " << username_ << " " << 
                "RequestTime: " << (this_passwd_response_->ResponseHeader).RequestTime << " " <<
                "SendingTime: " << (this_passwd_response_->ResponseHeader).SendingTime << " " <<
                "LogoutResponseT: " << sizeof(SessionPasswordChangeResponseT) << " " << "\n";
  dbglogger_.DumpCurrentBuffer();

}

void BSEEngine::processBSESessionRegistrationResponse(const char* bse_msg_char_buf){
  SessionRegistrationResponseT *this_session_registration_response = (SessionRegistrationResponseT*)(bse_msg_char_buf);

  dbglogger_ << "STATUS OF SECURE REGISTRATION " << this_session_registration_response->status  << "\n";
  if( 'Y' == this_session_registration_response->status ){
    dbglogger_ <<  "SECURE SESSION REGISTRATION COMPLETED\n";
    is_secure_box_registered_ = true;
  }
  else{
    dbglogger_ <<  "SESSION SESSION REGISTRATION FAILED\n";
    return;
  }

  dbglogger_ << "SessionRegistrationResponseT BodyLen: " << (this_session_registration_response->MessageHeaderOut).BodyLen << " "
                "TemplateID: " << (this_session_registration_response->MessageHeaderOut).TemplateID << " " <<
                "MsgSeqNum: " << (this_session_registration_response->ResponseHeader).MsgSeqNum << " " <<
                "Registration Status " << this_session_registration_response->status << " " << "\n";

    SessionLogin();  // BSE has a two stop login process, create a new socket using the gateway assigned ip/port  
}

void BSEEngine::processBSEUserPasswordChangeResponse(const char *bse_msg_char_buf) {
  UserPasswordChangeResponseT *this_passwd_response_ = (UserPasswordChangeResponseT *)(bse_msg_char_buf);
  dbglogger_ << "UserPasswordChangeResponse BodyLen: " << (this_passwd_response_->MessageHeaderOut).BodyLen << " " <<
                "TemplateID: " << (this_passwd_response_->MessageHeaderOut).TemplateID << " " <<
                "MsgSeqNum: " << (this_passwd_response_->ResponseHeader).MsgSeqNum << " " <<
                "UserId: " << username_ << " " << 
                "RequestTime: " << (this_passwd_response_->ResponseHeader).RequestTime << " " <<
                "SendingTime: " << (this_passwd_response_->ResponseHeader).SendingTime << " " <<
                "LogoutResponseT: " << sizeof(UserPasswordChangeResponseT) << " " << "\n";
  dbglogger_.DumpCurrentBuffer();

  // Updating to latest value to write back to config file
  setting_.setValue("PasswordChangeDate", HFSAT::DateTime::GetCurrentIsoDateLocalAsString());
  setting_.setValue("Password", new_user_password);

  p_engine_listener_->DumpSettingsToFile();
  
  EmailPasswordChange(std::string(" Changed Password for " + std::to_string(username_))); 
  
}

void BSEEngine::processBSEUserLoginResponse(const char *bse_msg_char_buf) {
  UserLoginResponseT *this_user_login_response_ = (UserLoginResponseT *)(bse_msg_char_buf);
  dbglogger_ << "UserLoginResponse BodyLen: " << (this_user_login_response_->MessageHeaderOut).BodyLen << " " <<
                "TemplateID: " << (this_user_login_response_->MessageHeaderOut).TemplateID << " " <<
                "MsgSeqNum: " << (this_user_login_response_->ResponseHeader).MsgSeqNum << " " << 
                "LoginUserId: " << username_ << " " << 
                "RequestTime: " << (this_user_login_response_->ResponseHeader).RequestTime << " " <<
                "SendingTime: " << (this_user_login_response_->ResponseHeader).SendingTime << " " <<
                "LastLoginTime: " << this_user_login_response_->LastLoginTime << " " <<
                "DaysLeftForPasswdExpiry: " << this_user_login_response_->DaysLeftForPasswdExpiry << " " <<
                "GraceLoginsLeft: " << this_user_login_response_->GraceLoginsLeft << " " <<
                "UserLoginResponseT: " << sizeof(UserLoginResponseT) << " " << "\n";
  dbglogger_.DumpCurrentBuffer();


  is_logged_in_ = true;

  p_engine_listener_->OnLogin(true);

  struct timeval login_time;
  gettimeofday(&login_time, NULL);
  DBGLOG_CLASS_FUNC_LINE_INFO << "LoggedIn @ : " << login_time.tv_sec << DBGLOG_ENDL_FLUSH;
  DBGLOG_DUMP;

  SimpleSecuritySymbolIndexer::GetUniqueInstance().AddSSSIListener(this);

  if (HFSAT::DaysBetweenDates(last_user_pw_change_date, HFSAT::DateTime::GetCurrentIsoDateUTC()) >= 7) {
    // We have to change password
    memset(new_user_password, 0, LEN_PASSWORD);
    BSE::BSEUserPasswordChangeRequest bse_user_password_change_request_(username_);
    GetNewPassword(new_user_password);
    dbglogger_ << "User Old Password  " << user_password_ << "\n";
    dbglogger_ << "Changing User Password to " << new_user_password << "\n";
    dbglogger_.DumpCurrentBuffer();

    bse_user_password_change_request_.setBSEUserPasswordChangeStaticFields(username_, new_user_password, LEN_PASSWORD, user_password_, LEN_PASSWORD);
    bse_user_password_change_request_.setBSEUserPasswordChangeMessageSequence(next_seq_num_);
    next_seq_num_++;
    
    int write_len_ = bse_user_password_change_request_.getBSEUserPasswordChangeRequestMsgLength();

    char send_buffer[1024];
    memset((void*)send_buffer,0,1024); 
    memcpy((void*)send_buffer,&bse_user_password_change_request_.bse_user_password_change_request_.MessageHeaderIn, 16);

    open_ssl_crypto_.aes_encrypt((unsigned char*)&bse_user_password_change_request_.bse_user_password_change_request_+16, bse_user_password_change_request_.getBSEUserPasswordChangeRequestMsgLength()-16);

    memcpy((void*)(send_buffer+16), (void*)open_ssl_crypto_.encrypt_text, open_ssl_crypto_.encrypt_len);

    if (tcp_direct_client_zocket_.WriteLockFreeN(16+open_ssl_crypto_.encrypt_len, (void*)send_buffer) < write_len_) {
      DBGLOG_CLASS_FUNC_LINE_FATAL << "FAILED TO WRITE COMPLETE USERPasswordChange MESSAGE TO EXCHANGE SOCKET" << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
      exit(-1);
    }
    dbglogger_ << "UserPasswrd BodyLen: " << bse_user_password_change_request_.bse_user_password_change_request_.MessageHeaderIn.BodyLen << " " <<
                "TemplateID: " << bse_user_password_change_request_.bse_user_password_change_request_.MessageHeaderIn.TemplateID << " " <<
                "MsgSeqNum: " << bse_user_password_change_request_.bse_user_password_change_request_.RequestHeader.MsgSeqNum << " " <<
                "Username: " << bse_user_password_change_request_.bse_user_password_change_request_.Username << " " <<
                "Password: " << bse_user_password_change_request_.bse_user_password_change_request_.Password << " " <<
                "NewPassword: " << bse_user_password_change_request_.bse_user_password_change_request_.NewPassword << " " <<
                "UserPssCHnageRequestT: " << sizeof(BSEUserPasswordChangeRequest) << " " << "\n";
    dbglogger_.DumpCurrentBuffer();
  }

}

void BSEEngine::processBSEUserLogoutResponse(const char *bse_msg_char_buf) {
  UserLogoutResponseT *this_User_logout_response_ = (UserLogoutResponseT *)(bse_msg_char_buf);
  dbglogger_ << "UserLogoutResponse BodyLen: " << (this_User_logout_response_->MessageHeaderOut).BodyLen << " " <<
                "TemplateID: " << (this_User_logout_response_->MessageHeaderOut).TemplateID << " " <<
                "MsgSeqNum: " << (this_User_logout_response_->ResponseHeader).MsgSeqNum << " " <<
                "LogoutUserId: " << username_ << " " <<
                "RequestTime: " << (this_User_logout_response_->ResponseHeader).RequestTime << " " <<
                "SendingTime: " << (this_User_logout_response_->ResponseHeader).SendingTime << " " <<
                "UserLogoutResponseT: " << sizeof(UserLogoutResponseT) << " " << "\n";
  dbglogger_.DumpCurrentBuffer();

  SessionLogout();
}

void BSEEngine::processBSERejectResponse(const char *bse_msg_char_buf) {
  RejectT *this_reject_response_ = (RejectT *)(bse_msg_char_buf);
/*
  dbglogger_ << "RejectT MsgSeqNum: " << (this_reject_response_->NRResponseHeaderME).MsgSeqNum << " " <<
                "UserId: " << username_ << " " << 
                "SessionRejectReason: " << this_reject_response_->SessionRejectReason << " " <<
                "SessionStatus: " << this_reject_response_->SessionStatus << " " <<
                "VarText: " << this_reject_response_->VarText << " " << "\n"; 
  dbglogger_.DumpCurrentBuffer();
*/

  uint8_t bse_reject_session_status_ = (this_reject_response_->SessionStatus);

  if ((int)bse_reject_session_status_ ==
      BSE_SESSION_STATUS_LOGOUT) {  // session got disconnected due to last msg, reject

    is_logged_in_ = false;
    keep_engine_running_ = false;

    DisConnect();

    return;
  }

  // reject code and sequence
  uint32_t bse_reject_sequence_ = (this_reject_response_->NRResponseHeaderME).MsgSeqNum;
  uint32_t bse_reject_reason_code_ = (this_reject_response_->SessionRejectReason);

  // get the rejection reason text

  std::string this_bse_reject_reason_str_(this_reject_response_->VarText, this_reject_response_->VarTextLen);

  auto pair = GetSaosTypePairUsingSeqNo(bse_reject_sequence_);
  int saos = pair.first;

  // Reject for order not found. If reject code is 10000, cancel reject
  if (bse_reject_reason_code_ == 10000) {
    dbglogger_ << "RejectT MsgSeqNum: " << bse_reject_sequence_ << " " <<
                  "UserId: " << username_ << " " << 
                  "SessionRejectReason: " << bse_reject_reason_code_ << " " <<
                  "SessionStatus: " << bse_reject_session_status_ << " " <<
                  "VarText: " << this_bse_reject_reason_str_ << " " << "\n"; 
    dbglogger_.DumpCurrentBuffer();
    return;
  }

  if (saos != -1) {
    char order_state = pair.second;

    switch (order_state) {
      // Modify
      case 'M': {
        if ( bse_reject_reason_code_ != 10000 ) {
          dbglogger_ << " Reject For Modify Order SAOS: " << saos << " Msg Seq : " << bse_reject_sequence_
                     << " SYMBOL: " << saos_to_symbol_[saos] << " ErrorCode : " << bse_reject_reason_code_
                     << " ErrorDescription : " << this_bse_reject_reason_str_ << "\n";
          dbglogger_.DumpCurrentBuffer();
        }
        p_engine_listener_->OnOrderModReject(saos);
      } break;

      // Cxl
      case 'C': {
        if ( bse_reject_reason_code_ != 10000 ) {
          dbglogger_ << " Reject For Cancel Order SAOS: " << saos << " Msg Seq : " << bse_reject_sequence_
                     << " SYMBOL: " << saos_to_symbol_[saos] << " ErrorCode : " << bse_reject_reason_code_
                     << " ErrorDescription : " << this_bse_reject_reason_str_ << "\n";
          dbglogger_.DumpCurrentBuffer();
        }
        p_engine_listener_->OnCxlReject(saos, HFSAT::kCxlRejectReasonTooLate);

      } break;

      // Send
      case 'S': {
        if ( bse_reject_reason_code_ != 10000 ) {
          dbglogger_ << " Reject For Send Order SAOS: " << saos << " Msg Seq : " << bse_reject_sequence_
                     << " SYMBOL: " << saos_to_symbol_[saos] << " ErrorCode : " << bse_reject_reason_code_
                     << " ErrorDescription : " << this_bse_reject_reason_str_ << "\n";
          dbglogger_.DumpCurrentBuffer();
        }
        p_engine_listener_->OnReject(saos);
      } break;

      default: {
        DBGLOG_CLASS_FUNC_LINE_ERROR
            << " Reject Received From Exchange, but we are unable to idbsefy the type of reject : SAOS: " << saos
            << " SYMBOL: " << saos_to_symbol_[saos] << " ExchSeqnum: " << bse_reject_sequence_ << " ErrorCode : " << bse_reject_reason_code_
            << " ErrorDescription : " << this_bse_reject_reason_str_ << " OurSeqNum: " << bse_reject_sequence_
            << " OrderState: " << order_state << DBGLOG_ENDL_FLUSH;
      } break;
    }

  } else {

    dbglogger_ << "Unexpected BSE Sequence Received For Reject : " << bse_reject_sequence_
               << " Text : " << this_bse_reject_reason_str_ << "\n";
    dbglogger_.DumpCurrentBuffer();
  }
}

void BSEEngine::processBSEHeartbeatResponse(const char *bse_msg_char_buf) {
#if BSE_DEBUG_INFO
  HeartbeatNotificationT *this_heart_beat_notification = (HeartbeatNotificationT *)(bse_msg_char_buf);
  dbglogger_ << "HeartbeatNotificationT BodyLen: " << (this_heart_beat_notification->MessageHeaderOut).BodyLen << " " <<
                "TemplateID: " << (this_heart_beat_notification->MessageHeaderOut).TemplateID << " " << 
                "SendingTime: " << (this_heart_beat_notification->NotifHeader).SendingTime << " " << "\n";
  dbglogger_.DumpCurrentBuffer();
#endif

/*
  if (num_unacked_heartbeats_ == 1)
    sendHeartbeat();
  else
    num_unacked_heartbeats_++;
*/
}

void BSEEngine::processBSENewOrderNRResponse(const char *bse_msg_char_buf) {
  NewOrderNRResponseT *this_new_order_nr_response_ = (NewOrderNRResponseT *)(bse_msg_char_buf);
#if BSE_DEBUG_INFO
  dbglogger_ << "NewOrderNRResponseT BodyLen: " << (this_new_order_nr_response_->MessageHeaderOut).BodyLen << " " <<
                "TemplateID: " << (this_new_order_nr_response_->MessageHeaderOut).TemplateID << " " <<
                "MsgSeqNum: " << (this_new_order_nr_response_->NRResponseHeaderME).MsgSeqNum << " " <<
                "LastFragment: " << (this_new_order_nr_response_->NRResponseHeaderME).LastFragment << " " <<
                "OrderID: " << this_new_order_nr_response_->OrderID << " " <<
                "ClOrdID: " << this_new_order_nr_response_->ClOrdID << " " <<
                "SecurityID: " << this_new_order_nr_response_->SecurityID << " " <<
                "PriceMkToLimitPx: " << this_new_order_nr_response_->PriceMkToLimitPx << " " <<
                "Yield: " << this_new_order_nr_response_->Yield << " " <<
                "UnderlyingDirtyPrice: " << this_new_order_nr_response_->UnderlyingDirtyPrice << " " <<
                "OrdStatus: " << this_new_order_nr_response_->OrdStatus << " " <<
                "ExecType: " << this_new_order_nr_response_->ExecType << " " <<
                "ExecRestatementReason: " << this_new_order_nr_response_->ExecRestatementReason << " " <<
                "ProductComplex: " << this_new_order_nr_response_->ProductComplex << " " <<
                "NewOrderNRResponseT: " << sizeof(NewOrderNRResponseT) << " " << "\n";
  dbglogger_.DumpCurrentBuffer();
#endif

  uint64_t bse_new_order_nr_exch_orderid_ = (this_new_order_nr_response_->OrderID);
  uint64_t bse_new_order_nr_cl_orderid_ = (this_new_order_nr_response_->ClOrdID);
  uint64_t bse_new_order_nr_activity_time_ = (this_new_order_nr_response_->ActivityTime);

  double conf_price = (double)((int64_t)(this_new_order_nr_response_->PriceMkToLimitPx + 0.5) / (double)ETI_FIXED_DECIMAL_OFFSET_MULTIPLIER_FOR_PRICE);
  int32_t conf_size = BSE_DUMMY_ORDER_SIZE_VALUE;   // dummy values

  // Nothing Invalid Goes Beyond This
  p_engine_listener_->OnOrderConf(bse_new_order_nr_cl_orderid_, "", conf_price, conf_size, 0,
                                  bse_new_order_nr_exch_orderid_, bse_new_order_nr_activity_time_, bse_new_order_nr_activity_time_);

  securityid_to_saos_orderid_map_[this_new_order_nr_response_->SecurityID][bse_new_order_nr_cl_orderid_] = bse_new_order_nr_exch_orderid_;
}

void BSEEngine::processBSEModifyOrderNRResponse(const char *bse_msg_char_buf) {
  ModifyOrderNRResponseT *modify_order_response = (ModifyOrderNRResponseT *)(bse_msg_char_buf);
#if BSE_DEBUG_INFO
  dbglogger_ << "ModifyOrderNRResponseT BodyLen: " << (modify_order_response->MessageHeaderOut).BodyLen << " " <<
                "TemplateID: " << (modify_order_response->MessageHeaderOut).TemplateID << " " <<
                "MsgSeqNum: " << (modify_order_response->NRResponseHeaderME).MsgSeqNum << " " <<
                "LastFragment: " << (modify_order_response->NRResponseHeaderME).LastFragment << " " <<
                "OrderID: " << modify_order_response->OrderID << " " <<
                "ClOrdID: " << modify_order_response->ClOrdID << " " <<
                "OrigClOrdID: " << modify_order_response->OrigClOrdID << " " <<
                "SecurityID: " << modify_order_response->SecurityID << " " <<
                "PriceMkToLimitPx: " << modify_order_response->PriceMkToLimitPx << " " <<
                "Yield: " << modify_order_response->Yield << " " <<
                "UnderlyingDirtyPrice: " << modify_order_response->UnderlyingDirtyPrice << " " <<
                "LeavesQty: " << modify_order_response->LeavesQty << " " <<
                "CumQty: " << modify_order_response->CumQty << " " <<
                "CxlQty: " << modify_order_response->CxlQty << " " <<
                "OrdStatus: " << modify_order_response->OrdStatus << " " <<
                "ExecType: " << modify_order_response->ExecType << " " <<
                "ExecRestatementReason: " << modify_order_response->ExecRestatementReason << " " <<
                "ProductComplex: " << modify_order_response->ProductComplex << " " <<
                "ModifyOrderNRResponseT: " << sizeof(ModifyOrderNRResponseT) << " " << "\n";
  dbglogger_.DumpCurrentBuffer();
#endif

//  dbglogger_ << "MODIFY : " << modify_order_response->LeavesQty << " " << modify_order_response->OrdStatus[0] << "\n";
//  dbglogger_.DumpCurrentBuffer();

  uint64_t bse_modify_order_nr_cl_orderid_ = (modify_order_response->ClOrdID);
  if ( modify_order_response->LeavesQty == 0 && modify_order_response->OrdStatus[0] == '4' ) {
      p_engine_listener_->OnOrderCxl(bse_modify_order_nr_cl_orderid_, modify_order_response->OrderID);

#if BSE_DEBUG_INFO
      dbglogger_ << "SIMULATED CXL FOR MODIFY : " << modify_order_response->LeavesQty << " " << modify_order_response->OrdStatus[0] 
                 << "SAOS: " << bse_modify_order_nr_cl_orderid_ << "\n";
      dbglogger_.DumpCurrentBuffer();
#endif

    if (securityid_to_saos_orderid_map_[modify_order_response->SecurityID].find(bse_modify_order_nr_cl_orderid_) != securityid_to_saos_orderid_map_[modify_order_response->SecurityID].end()) {
      securityid_to_saos_orderid_map_[modify_order_response->SecurityID].erase(bse_modify_order_nr_cl_orderid_);
    }
    
  }
  else {
//    DBGLOG_CLASS_FUNC_LINE_INFO << "Price: " << modify_order_response->PriceMkToLimitPx << " SAOS: " << modify_order_response->OrderID << DBGLOG_ENDL_FLUSH;
    double modified_price = (double)((int64_t)((modify_order_response->PriceMkToLimitPx) + 0.5) / (double)ETI_FIXED_DECIMAL_OFFSET_MULTIPLIER_FOR_PRICE);
    p_engine_listener_->OnOrderCancelReplaced(bse_modify_order_nr_cl_orderid_, modify_order_response->OrderID,
                                              modified_price, modify_order_response->LeavesQty, fast_px_convertor_vec_,
                                              modify_order_response->ActivityTime, modify_order_response->ActivityTime);
  }

}

/*
void BSEEngine::processBSEMassCancellationEventResponse(const char *bse_msg_char_buf_) {
  DeleteAllOrderQuoteEventBroadcastT *bse_order_mass_cancellation_response_ =
      (DeleteAllOrderQuoteEventBroadcastT *)(bse_msg_char_buf_);

  std::string bse_order_mass_cancellation_event_apl_msg_id_ =
      (bse_order_mass_cancellation_response_->RBCHeaderME).ApplMsgID;

  uint8_t bse_order_mass_cancellation_mass_action_reason_ = (bse_order_mass_cancellation_response_->MassActionReason);

  std::string mass_cancellation_event_reason_ = "";

  switch (bse_order_mass_cancellation_mass_action_reason_) {
    case 105: {
      mass_cancellation_event_reason_ = " Product State Halt ";
    } break;
    case 106: {
      mass_cancellation_event_reason_ = " Product State Holiday ";
    } break;
    case 107: {
      mass_cancellation_event_reason_ = " Instrument Suspended ";
    } break;
    case 109: {
      mass_cancellation_event_reason_ = " Complex Instrument Delbseon ";
    } break;
    case 110: {
      mass_cancellation_event_reason_ = " Volitility Interruption";
    } break;

    default: { mass_cancellation_event_reason_ = " INVALID REASON "; } break;
  }

  uint8_t bse_order_mass_cancellation_exec_inst_ = (bse_order_mass_cancellation_response_->ExecInst);

  std::string cancellation_scope_ = "";

  switch (bse_order_mass_cancellation_exec_inst_) {
    case 1: {
      cancellation_scope_ = " Persistent Orders ";
    } break;
    case 2: {
      cancellation_scope_ = " Non Persistent Orders ";
    } break;
    case 3: {
      cancellation_scope_ = " Persistent & Non Persistent Orders ";
    } break;
    default: { cancellation_scope_ = " INVALID SCOPE "; } break;
  }
}
*/

void BSEEngine::SendOrder(ORS::Order *order) {
#if BSE_DEBUG_INFO
  DBGLOG_CLASS_FUNC_LINE_INFO << "SendOrder: SecID: " << order->security_id_ << DBGLOG_ENDL_FLUSH;
#endif
  if (!is_mkt_order_)
    SendOrder(order, container_.inst_desc_[order->security_id_].GetToken());
  else
    SendMktOrder(order, container_.inst_desc_[order->security_id_].GetToken(), container_.inst_desc_[order->security_id_].ProductId());
}

void BSEEngine::CancelOrder(ORS::Order *order) { 
  CancelOrder(order, container_.inst_desc_[order->security_id_].GetToken(), container_.inst_desc_[order->security_id_].ProductId()); 
}

void BSEEngine::ModifyOrder(ORS::Order *order, ORS::Order *orig_order) {
  ModifyOrder(order, container_.inst_desc_[order->security_id_].GetToken(), orig_order);
}

void BSEEngine::processBSEOrderExecutionResponse(const char *bse_msg_char_buf) {
  OrderExecNotificationT *bse_execution_notice_response_ = (OrderExecNotificationT *)(bse_msg_char_buf);

  uint64_t bse_order_execution_exch_ord_id_ = (bse_execution_notice_response_->OrderID);
  uint64_t bse_order_execution_cl_ord_id_ = (bse_execution_notice_response_->ClOrdID);
  int64_t bse_order_execution_security_id_ = (bse_execution_notice_response_->SecurityID);
  std::string bse_symbol_ = bse_daily_token_symbol_handler_.GetInternalSymbolFromToken(bse_order_execution_security_id_, bse_segment_type_);
#if BSE_DEBUG_INFO
  DBGLOG_CLASS_FUNC_LINE_INFO << bse_symbol_ << DBGLOG_ENDL_FLUSH;
#endif

  int32_t bse_order_execution_leaves_qty_ = (bse_execution_notice_response_->LeavesQty);
  uint8_t bse_order_execution_no_fills_ = (bse_execution_notice_response_->NoFills);
  uint64_t bse_order_execution_activity_time_ = (bse_execution_notice_response_->ActivityTime);

  std::string exchange_symbol_ = HFSAT::BSESecurityDefinitions::ConvertDataSourceNametoExchSymbol(bse_symbol_);

  // Initialize
  int32_t lookup_total_trades_qty_ = 0;
  register uint8_t fill_group_counter_ = 0;
  int32_t cumulative_trade_qty_ = 0;

#if BSE_DEBUG_INFO
  if (bse_order_execution_no_fills_ > 1 && bse_order_execution_leaves_qty_ == 0) {
    dbglogger_ << " Handled Missing Exec Successfully, No Of Fills : " << bse_order_execution_no_fills_
               << " Remaining : " << bse_order_execution_leaves_qty_ << " SAOS : " << bse_order_execution_cl_ord_id_
               << "\n";
    dbglogger_.DumpCurrentBuffer();
  }
#endif

  // Invalid
  HFSAT::TradeType_t this_trade_type_ = HFSAT::kTradeTypeNoInfo;

  // Fetch Total Trade Qty
  for (fill_group_counter_ = 0; fill_group_counter_ < bse_order_execution_no_fills_; fill_group_counter_++) {
    lookup_total_trades_qty_ += (bse_execution_notice_response_->FillsGrp[fill_group_counter_]).FillQty;
  }

#if BSE_DEBUG_INFO
  dbglogger_ << "OrderExecNotificationT: " << " " <<
                "Symbol: " << bse_symbol_ << " " <<
                "OrderID: " << bse_order_execution_exch_ord_id_ << " " <<
                "ClOrdID: " << bse_order_execution_cl_ord_id_ << " " <<
                "SecurityID: " << bse_order_execution_security_id_ << " " <<
                "LeavesQty: " << bse_order_execution_leaves_qty_ << " " <<
                "NoFills: " << bse_order_execution_no_fills_ << " " <<
                "ActivityTime: " << bse_order_execution_activity_time_ << " " <<
                "TotalTrade: " << lookup_total_trades_qty_ << " ";
#endif

    for (fill_group_counter_ = 0; fill_group_counter_ < bse_order_execution_no_fills_; fill_group_counter_++) {
      int64_t bse_order_execution_fill_price_ = (bse_execution_notice_response_->FillsGrp[fill_group_counter_]).FillPx;
      int32_t bse_order_execution_fill_qty_ = (bse_execution_notice_response_->FillsGrp[fill_group_counter_]).FillQty;

      // Accumulate Trades Qty
      cumulative_trade_qty_ += bse_order_execution_fill_qty_;

      double price_ = (double)((bse_order_execution_fill_price_) / (double)(100000000));
#if BSE_DEBUG_INFO
      dbglogger_ << "Price: " << price_ << " " << "Qty: " << bse_order_execution_fill_qty_ << " ";
#endif

//      DBGLOG_CLASS_FUNC_LINE_INFO << "SAOS: " << bse_order_execution_cl_ord_id_
//                                  << "FillQty: " << bse_order_execution_fill_qty_
//                                  << "SizeRem: " << ((lookup_total_trades_qty_ + bse_order_execution_leaves_qty_) - cumulative_trade_qty_) << DBGLOG_ENDL_FLUSH;
      p_engine_listener_->OnOrderExec(
          bse_order_execution_cl_ord_id_, exchange_symbol_.c_str(),
          this_trade_type_, price_, bse_order_execution_fill_qty_,
          ((lookup_total_trades_qty_ + bse_order_execution_leaves_qty_) - cumulative_trade_qty_),
          bse_order_execution_exch_ord_id_,0,bse_order_execution_activity_time_);
    }
    if (bse_order_execution_leaves_qty_ == 0) {
      if (securityid_to_saos_orderid_map_[bse_order_execution_security_id_].find(bse_order_execution_cl_ord_id_) != securityid_to_saos_orderid_map_[bse_order_execution_security_id_].end()) {
        securityid_to_saos_orderid_map_[bse_order_execution_security_id_].erase(bse_order_execution_cl_ord_id_);
      }
    }

#if BSE_DEBUG_INFO
  dbglogger_ << "\n";
  dbglogger_.DumpCurrentBuffer();
  DBGLOG_CLASS_FUNC_LINE_INFO << bse_symbol_ << "DONE" << DBGLOG_ENDL_FLUSH;
#endif
}

void BSEEngine::processBSEImmediateExecutionResponse(const char *bse_msg_char_buf) {
  OrderExecResponseT *bse_execution_notice_response_ = (OrderExecResponseT *)(bse_msg_char_buf);

  uint64_t bse_order_execution_exch_ord_id_ = (bse_execution_notice_response_->OrderID);
  uint64_t bse_order_execution_cl_ord_id_ = (bse_execution_notice_response_->ClOrdID);
  int64_t bse_order_execution_security_id_ = (bse_execution_notice_response_->SecurityID);
  std::string bse_symbol_ = bse_daily_token_symbol_handler_.GetInternalSymbolFromToken(bse_order_execution_security_id_, bse_segment_type_); 
#if BSE_DEBUG_INFO
  DBGLOG_CLASS_FUNC_LINE_INFO << bse_symbol_ << DBGLOG_ENDL_FLUSH;
#endif

  int32_t bse_order_execution_leaves_qty_ = (bse_execution_notice_response_->LeavesQty);
  int32_t bse_order_execution_total_exec_qty_ = (bse_execution_notice_response_->CumQty);
  uint8_t bse_order_execution_no_fills_ = (bse_execution_notice_response_->NoFills);
  uint64_t bse_order_execution_activity_time_ = (bse_execution_notice_response_->ActivityTime);
  int64_t bse_order_execution_price_ = 0;
  int32_t bse_order_execution_conf_total_qty_ = bse_order_execution_leaves_qty_ + bse_order_execution_total_exec_qty_;

  std::string exchange_symbol_ = HFSAT::BSESecurityDefinitions::ConvertDataSourceNametoExchSymbol(bse_symbol_);

  // Initialize
  int32_t lookup_total_trades_qty_ = 0;
  register uint8_t fill_group_counter_ = 0;
  int32_t cumulative_trade_qty_ = 0;

#if BSE_DEBUG_INFO
  if (bse_order_execution_no_fills_ > 1 && bse_order_execution_leaves_qty_ == 0) {
    dbglogger_ << " Handled Missing Exec Successfully, No Of Fills : " << bse_order_execution_no_fills_
               << " Remaining : " << bse_order_execution_leaves_qty_ << " SAOS : " << bse_order_execution_cl_ord_id_
               << "\n";
    dbglogger_.DumpCurrentBuffer();
  }
#endif

  // Invalid
  HFSAT::TradeType_t this_trade_type_ = HFSAT::kTradeTypeNoInfo;

  // Fetch Total Trades Qty For Notify Operations
  for (fill_group_counter_ = 0; fill_group_counter_ < bse_order_execution_no_fills_; fill_group_counter_++) {
    lookup_total_trades_qty_ += ((bse_execution_notice_response_->FillsGrp)[fill_group_counter_]).FillQty;
    bse_order_execution_price_ = ((bse_execution_notice_response_->FillsGrp)[fill_group_counter_]).FillPx;
  }

#if BSE_DEBUG_INFO
  dbglogger_ << "OrderImmediateExecResponseT: " << " " <<
                "MsgSeqNum: " << bse_execution_notice_response_->ResponseHeaderME.MsgSeqNum << " " << 
                "Symbol: " << bse_symbol_ << " " <<
                "OrderID: " << bse_order_execution_exch_ord_id_ << " " <<
                "ClOrdID: " << bse_order_execution_cl_ord_id_ << " " <<
                "SecurityID: " << bse_order_execution_security_id_ << " " <<
                "LeavesQty: " << bse_order_execution_leaves_qty_ << " " <<
                "CumQty: " << bse_order_execution_total_exec_qty_ << " " <<
                "NoFills: " << bse_order_execution_no_fills_ << " " <<
                "ActivityTime: " << bse_order_execution_activity_time_ << " " <<
                "TotalTrade: " << lookup_total_trades_qty_ << " ";
#endif

  double exec_price_ = (double)((bse_order_execution_price_) / (double)(100000000));
#if BSE_DEBUG_INFO
  dbglogger_ << "ExecPrice: " << exec_price_ << " ";
#endif
  HFSAT::ORS::Order* p_this_order_ = order_manager_.GetOrderByOrderSequence(bse_order_execution_cl_ord_id_);

  if (p_this_order_ != NULL) {
    if (!(p_this_order_->is_confirmed_)) {
      p_engine_listener_->OnOrderConf(bse_order_execution_cl_ord_id_, "", exec_price_, bse_order_execution_conf_total_qty_, 0,
                                      bse_order_execution_exch_ord_id_, bse_order_execution_activity_time_, bse_order_execution_activity_time_);
      securityid_to_saos_orderid_map_[bse_order_execution_security_id_][bse_order_execution_cl_ord_id_] = bse_order_execution_exch_ord_id_;
    }
    else {
      p_engine_listener_->OnOrderCancelReplaced(bse_order_execution_cl_ord_id_, bse_order_execution_exch_ord_id_,
                                                exec_price_, bse_order_execution_leaves_qty_, fast_px_convertor_vec_,
                                                bse_order_execution_activity_time_, bse_order_execution_activity_time_);
    }
  } else {
     DBGLOG_CLASS_FUNC_LINE_ERROR << "ImmediateTrade Received for " << bse_symbol_ << " not in order_manager: SAOS: " << bse_order_execution_cl_ord_id_
                                  << DBGLOG_ENDL_FLUSH;
  }

  for (fill_group_counter_ = 0; fill_group_counter_ < bse_order_execution_no_fills_; fill_group_counter_++) {
    int64_t bse_order_execution_fill_price_ = ((bse_execution_notice_response_->FillsGrp)[fill_group_counter_]).FillPx;
    int32_t bse_order_execution_fill_qty_ = ((bse_execution_notice_response_->FillsGrp)[fill_group_counter_]).FillQty;

    // Accumulate Trades Qty
    cumulative_trade_qty_ += bse_order_execution_fill_qty_;

    double price_ = (double)((bse_order_execution_fill_price_) / (double)(100000000));
#if BSE_DEBUG_INFO
    dbglogger_ << "Price: " << price_ << " " << "Qty: " << bse_order_execution_fill_qty_ << " ";
#endif

//    DBGLOG_CLASS_FUNC_LINE_INFO << "SAOS: " << bse_order_execution_cl_ord_id_
//                                << "FillQty: " << bse_order_execution_fill_qty_
//                                << "SizeRem: " << ((lookup_total_trades_qty_ + bse_order_execution_leaves_qty_) - cumulative_trade_qty_) << DBGLOG_ENDL_FLUSH;
    p_engine_listener_->OnOrderExec(
        bse_order_execution_cl_ord_id_, exchange_symbol_.c_str(),
        this_trade_type_, price_, bse_order_execution_fill_qty_,
        ((lookup_total_trades_qty_ + bse_order_execution_leaves_qty_) - cumulative_trade_qty_),
        bse_order_execution_exch_ord_id_,0,bse_order_execution_activity_time_);

    if (0 == price_ || 0 == bse_order_execution_fill_qty_) {
#if BSE_DEBUG_INFO
      dbglogger_ << " Execution : " << bse_order_execution_cl_ord_id_ << " Price : " << price_
                 << " SIze : " << bse_order_execution_fill_qty_ << "\n";
      dbglogger_.DumpCurrentBuffer();

      printHexString(bse_msg_char_buf, MAX_BSE_RESPONSE_BUFFER_SIZE);
#endif

      //exit(-1);
    }
  }
  
  if (bse_order_execution_leaves_qty_ == 0) {
    if (securityid_to_saos_orderid_map_[bse_order_execution_security_id_].find(bse_order_execution_cl_ord_id_) != securityid_to_saos_orderid_map_[bse_order_execution_security_id_].end()) {
      securityid_to_saos_orderid_map_[bse_order_execution_security_id_].erase(bse_order_execution_cl_ord_id_);
    }
  }

#if BSE_DEBUG_INFO
  dbglogger_ << "\n";
  dbglogger_.DumpCurrentBuffer();
/*
  // Immediate Partial Execution is a special case
  if (bse_order_execution_leaves_qty_ > 0 && ((char)(bse_execution_notice_response_->OrdStatus[0]) == '1')) {
    // Send Confirmation For the Remaining Size
    const char *order_id_ = (const char *)(&bse_execution_notice_response_->OrderID);

    dbglogger_ << " Immediate Response, Remaining Qty : " << bse_order_execution_leaves_qty_
               << " Order Status : " << bse_execution_notice_response_->OrdStatus[0]
               << " RestatementReason : " << bse_execution_notice_response_->ExecRestatementReason
               << " SAOS : " << bse_order_execution_cl_ord_id_ << " Exchange Seq : " << order_id_ << "\n";
    dbglogger_.DumpCurrentBuffer();

    double conf_price = BSE_DUMMY_ORDER_PRICE_VALUE;

    p_engine_listener_->OnOrderConf(bse_order_execution_cl_ord_id_, order_id_, conf_price,
                                    bse_order_execution_leaves_qty_, sizeof(uint64_t));
  }
*/
  DBGLOG_CLASS_FUNC_LINE_INFO << bse_symbol_ << "DONE" << DBGLOG_ENDL_FLUSH;
#endif
}

/*
void BSEEngine::processBSECancelOrderSingleResponse(const char *bse_msg_char_buf) {  // Standard Orders

  DeleteOrderResponseT *bse_cancel_order_single_nr_response_ = (DeleteOrderResponseT *)(bse_msg_char_buf);

  uint64_t bse_cancel_order_single_nr_cl_order_id_ = (bse_cancel_order_single_nr_response_->ClOrdID);

  int32_t bse_cancel_order_single_nr_cum_qty_ = (bse_cancel_order_single_nr_response_->CumQty);
  int32_t bse_cancel_order_single_nr_cxl_qty_ = (bse_cancel_order_single_nr_response_->CxlQty);

  p_engine_listener_->OnOrderCxlETS(bse_cancel_order_single_nr_cl_order_id_, bse_cancel_order_single_nr_cum_qty_,
                                    bse_cancel_order_single_nr_cxl_qty_);
}
*/

void BSEEngine::processBSECancelOrderSingleNRResponse(const char *bse_msg_char_buf) {  // Lean Orders

  DeleteOrderNRResponseT *bse_cancel_order_single_nr_response_ = (DeleteOrderNRResponseT *)(bse_msg_char_buf);
#if BSE_DEBUG_INFO
  dbglogger_ << "DeleteOrderNRResponseT BodyLen: " << (bse_cancel_order_single_nr_response_->MessageHeaderOut).BodyLen << " " <<
                "TemplateID: " << (bse_cancel_order_single_nr_response_->MessageHeaderOut).TemplateID << " " <<
                "MsgSeqNum: " << (bse_cancel_order_single_nr_response_->NRResponseHeaderME).MsgSeqNum << " " <<
                "LastFragment: " << (bse_cancel_order_single_nr_response_->NRResponseHeaderME).LastFragment << " " <<
                "OrderID: " << bse_cancel_order_single_nr_response_->OrderID << " " <<
                "ClOrdID: " << bse_cancel_order_single_nr_response_->ClOrdID << " " <<
                "SecurityID: " << bse_cancel_order_single_nr_response_->SecurityID << " " <<
                "CumQty: " << bse_cancel_order_single_nr_response_->CumQty << " " <<
                "CxlQty: " << bse_cancel_order_single_nr_response_->CxlQty << " " <<
                "OrdStatus: " << bse_cancel_order_single_nr_response_->OrdStatus << " " <<
                "ExecType: " << bse_cancel_order_single_nr_response_->ExecType << " " <<
                "ExecRestatementReason: " << bse_cancel_order_single_nr_response_->ExecRestatementReason << " " <<
                "ProductComplex: " << bse_cancel_order_single_nr_response_->ProductComplex << " " <<
                "DeleteOrderNRResponseT: " << sizeof(DeleteOrderNRResponseT) << " " << "\n";
  dbglogger_.DumpCurrentBuffer();
#endif


  uint64_t bse_cancel_order_single_nr_cl_order_id_ = (bse_cancel_order_single_nr_response_->ClOrdID);

  uint64_t bse_cancel_order_single_nr_order_id_ = (bse_cancel_order_single_nr_response_->OrderID);

  p_engine_listener_->OnOrderCxl(bse_cancel_order_single_nr_cl_order_id_, bse_cancel_order_single_nr_order_id_);

  if (securityid_to_saos_orderid_map_[bse_cancel_order_single_nr_response_->SecurityID].find(bse_cancel_order_single_nr_cl_order_id_) != securityid_to_saos_orderid_map_[bse_cancel_order_single_nr_response_->SecurityID].end()) {
    securityid_to_saos_orderid_map_[bse_cancel_order_single_nr_response_->SecurityID].erase(bse_cancel_order_single_nr_cl_order_id_);
  }
}

void BSEEngine::processBSEOrderConfirmationResponse(const char *bse_msg_char_buf) {

#if BSE_DEBUG_INFO
  GwOrderAcknowledgementT *bse_order_acknowledgement_ = (GwOrderAcknowledgementT *)(bse_msg_char_buf);
  dbglogger_ << "GwOrderAcknowledgementT BodyLen: " << (bse_order_acknowledgement_->MessageHeaderOut).BodyLen << " " <<
                "TemplateID: " << (bse_order_acknowledgement_->MessageHeaderOut).TemplateID << " " <<
                "MsgSeqNum: " << (bse_order_acknowledgement_->ResponseHeader).MsgSeqNum << " " <<
                "PrimaryOrderID: " << bse_order_acknowledgement_->PrimaryOrderID << " " <<
                "ClOrdID: " << bse_order_acknowledgement_->ClOrdID << " " <<
                "MessageTag: " << bse_order_acknowledgement_->MessageTag << " " <<
                "GwOrderAcknowledgementT: " << sizeof(GwOrderAcknowledgementT) << " " << "\n";
  dbglogger_.DumpCurrentBuffer();
#endif

}

void BSEEngine::processBSEExtendedOrderInfoResponse(const char *bse_msg_char_buf) {

  OrderExecReportBroadcastT *bse_extended_order_info_ = (OrderExecReportBroadcastT *)(bse_msg_char_buf);
#if BSE_DEBUG_INFO
  dbglogger_ << "OrderExecReportBroadcastT: " << bse_extended_order_info_->ToString() << "\n";
  dbglogger_.DumpCurrentBuffer();

#endif

  uint64_t bse_extended_order_cl_order_id_ = (bse_extended_order_info_->ClOrdID);
  if ( (bse_extended_order_info_->OrdStatus[0] == '4') && (bse_extended_order_info_->ExecRestatementReason == 246) ) {
      p_engine_listener_->OnOrderCxl( bse_extended_order_cl_order_id_, bse_extended_order_info_->OrderID);

#if BSE_DEBUG_INFO
      dbglogger_ << "SIMULATED CXL FOR ExtendedOrderInfo : " << bse_extended_order_info_->ExecRestatementReason << " " << bse_extended_order_info_->OrdStatus[0] 
                 << "SAOS: " << bse_extended_order_cl_order_id_ << "\n";
      dbglogger_.DumpCurrentBuffer();
#endif

    if (securityid_to_saos_orderid_map_[bse_extended_order_info_->SecurityID].find(bse_extended_order_cl_order_id_) != securityid_to_saos_orderid_map_[bse_extended_order_info_->SecurityID].end()) {
      securityid_to_saos_orderid_map_[bse_extended_order_info_->SecurityID].erase(bse_extended_order_cl_order_id_);
    }
  }  

}

void BSEEngine::processBSEMassCancellationResponse(const char *bse_msg_char_buf) {

  DeleteAllOrderQuoteEventBroadcastT* bse_mass_cancel_event_ = (DeleteAllOrderQuoteEventBroadcastT *)(bse_msg_char_buf);
//#if BSE_DEBUG_INFO
  dbglogger_ << "DeleteAllOrderQuoteEventBroadcastT: " << bse_mass_cancel_event_->ToString() << "\n";
  dbglogger_.DumpCurrentBuffer();

//#endif

  std::string partition_segmentid_ = std::to_string(bse_mass_cancel_event_->RBCHeaderME.PartitionID) + "-" + std::to_string(bse_mass_cancel_event_->MarketSegmentID);

  std::vector<uint32_t> security_id_vec_ = HFSAT::BSESecurityDefinitions::GetSecurityIdVectorFromPartionSegmentId(partition_segmentid_);

  for (auto& sec_id : security_id_vec_) { 
    std::tr1::unordered_map<uint64_t,uint64_t> saos_to_orderid_map_ = securityid_to_saos_orderid_map_[sec_id];
    for (auto& map_iter : saos_to_orderid_map_) { 
      p_engine_listener_->OnOrderCxl(map_iter.first, map_iter.second);
      dbglogger_ << "MassCancelSecId: " << sec_id << " PartSegId: " << partition_segmentid_
	         << " SYMBOL: " << saos_to_symbol_[map_iter.first]
                 << " SAOS: " << map_iter.first << " ORDERID: " << map_iter.second << DBGLOG_ENDL_FLUSH; 
      securityid_to_saos_orderid_map_[sec_id].erase(map_iter.first);
      if (securityid_to_saos_orderid_map_[sec_id].find(map_iter.first) == securityid_to_saos_orderid_map_[sec_id].end()) {
        DBGLOG_CLASS_FUNC_LINE_INFO << "SAOS " << map_iter.first
                                    << " REMOVED FROM SECID MAP FOR " << sec_id << DBGLOG_ENDL_FLUSH;
      }
    }
  }

}

void BSEEngine::processBSERiskCollateralAlertResponse(const char *bse_msg_char_buf) {
  RiskCollateralAlertAdminBroadcastT *this_risk_collateral_alert_response_ = (RiskCollateralAlertAdminBroadcastT *)(bse_msg_char_buf);
  dbglogger_ << "RiskCollateralAlertAdminBroadcast BodyLen: " << (this_risk_collateral_alert_response_->MessageHeaderOut).BodyLen << " " <<
                "TemplateID: " << (this_risk_collateral_alert_response_->MessageHeaderOut).TemplateID << " " <<
                "PartitionID: " << (this_risk_collateral_alert_response_->RBCHeader).PartitionID << " " <<
                "ApplResendFlag: " << (this_risk_collateral_alert_response_->RBCHeader).ApplResendFlag << " " <<
                "ApplID: " << (this_risk_collateral_alert_response_->RBCHeader).ApplID << " " <<
                "LastFragment: " << (this_risk_collateral_alert_response_->RBCHeader).LastFragment << " " <<
                "TotalCollateral: " << this_risk_collateral_alert_response_->TotalCollateral << " " <<
                "UtilizedCollateral: " << this_risk_collateral_alert_response_->UtilizedCollateral << " " <<
                "UnutilizedCollateral: " << this_risk_collateral_alert_response_->UnutilizedCollateral << " " <<
                "PercentageUtilized: " << this_risk_collateral_alert_response_->PercentageUtilized << " " <<
                "MarketSegmentID: " << this_risk_collateral_alert_response_->MarketSegmentID << " " <<
                "MarketID: " << this_risk_collateral_alert_response_->MarketID << " " <<
                "VarTextLen: " << this_risk_collateral_alert_response_->VarTextLen << " " <<
                "RRMState: " << this_risk_collateral_alert_response_->RRMState << " " <<
                "MemberType: " << this_risk_collateral_alert_response_->MemberType << " " <<
                "IncrementDecrementStatus: " << this_risk_collateral_alert_response_->IncrementDecrementStatus << " " <<
                "SegmentIndicator: " << this_risk_collateral_alert_response_->SegmentIndicator << " " <<
                "Duration: " << this_risk_collateral_alert_response_->Duration << " " <<
                "BusinessUnitSymbol: " << this_risk_collateral_alert_response_->BusinessUnitSymbol << " " <<
                "VarText: " << this_risk_collateral_alert_response_->VarText << " " <<
                "RiskCollateralAlertResponseT: " << sizeof(RiskCollateralAlertAdminBroadcastT) << " " << "\n";
  dbglogger_.DumpCurrentBuffer();

}
void BSEEngine::EmailGeneric(char* alert_msg_) {
  // also send an alert
  char hostname[128];
  hostname[127] = '\0';
  gethostname(hostname, 127);

  std::string mode_str_ = "BSE";

  std::string alert_message = " Alert" + std::string(hostname) + alert_msg_ + std::to_string(username_);

  HFSAT::Email e;

  struct timeval current_time;
  gettimeofday(&current_time, NULL);

  e.setSubject(mode_str_);
  e.addRecepient("ravi.parikh@tworoads.co.in, nseall@tworoads.co.in");
  e.addSender("ravi.parikh@tworoads.co.in");
  e.content_stream << "exch: " << mode_str_ << "<br/>";
  e.content_stream << "host_machine: " << hostname << "<br/>";
  e.content_stream << "Message : " << alert_msg_ << " <br/>";
  e.content_stream << "Current Time: " << ctime(&(current_time.tv_sec)) << " <br/>";
  e.sendMail();
}
void BSEEngine::EnableDisableOnlyIOC(const bool is_risk_reduction_set_){
	is_risk_reduction_set = is_risk_reduction_set_;
	if ( is_risk_reduction_set == true) {
		dbglogger_ << "INFO: Risk Reduction Only IOC: Enabled User: " << username_<< "\n";
    char str_msg[128] = "INFO: Risk Reduction Only IOC: Enabled User: ";
    //Margin >=90 hence we only want to send IOC orders =3
    bse_new_order_single_short_.setBSENewOrderSingleShortTimeInforce(ENUM_TIMEINFORCE_IOC);
		EmailGeneric(str_msg); 
	} else {
		dbglogger_ << "INFO: Risk Reduction Only IOC: Disbaled User: " << username_ << "\n";
    char str_msg[128] = "INFO: Risk Reduction Only IOC: Disbaled User: ";
    //Margin <90 hence we can revert back to Normal orders (Session orders GFS =7)
    bse_new_order_single_short_.setBSENewOrderSingleShortTimeInforce(ENUM_TIMEINFORCE_GTCL);
    //Do we need to change for bse_new_order_single also because they only send Mkt and Cancel order?
		EmailGeneric(str_msg);
	}

}
void BSEEngine::processBSERiskCollateralAlertTraderResponse(const char *bse_msg_char_buf) {
  RiskCollateralAlertBroadcastT *this_risk_collateral_alert_trader_response_ = (RiskCollateralAlertBroadcastT *)(bse_msg_char_buf);
  dbglogger_ << "RiskCollateralAlertTraderBroadcast BodyLen: " << (this_risk_collateral_alert_trader_response_->MessageHeaderOut).BodyLen << " " <<
                "TemplateID: " << (this_risk_collateral_alert_trader_response_->MessageHeaderOut).TemplateID << " " <<
                "PartitionID: " << (this_risk_collateral_alert_trader_response_->RBCHeader).PartitionID << " " <<
                "ApplResendFlag: " << (this_risk_collateral_alert_trader_response_->RBCHeader).ApplResendFlag << " " <<
                "ApplID: " << (this_risk_collateral_alert_trader_response_->RBCHeader).ApplID << " " <<
                "LastFragment: " << (this_risk_collateral_alert_trader_response_->RBCHeader).LastFragment << " " <<
                "OrigTime: " << this_risk_collateral_alert_trader_response_->OrigTime << " " <<
                "PercentageUtilized: " << this_risk_collateral_alert_trader_response_->PercentageUtilized << " " <<
                "MarketSegmentID: " << this_risk_collateral_alert_trader_response_->MarketSegmentID << " " <<
                "MarketID: " << this_risk_collateral_alert_trader_response_->MarketID << " " <<
                "VarTextLen: " << this_risk_collateral_alert_trader_response_->VarTextLen << " " <<
                "RRMState: " << this_risk_collateral_alert_trader_response_->RRMState << " " <<
                "MemberType: " << this_risk_collateral_alert_trader_response_->MemberType << " " <<
                "IncrementDecrementStatus: " << this_risk_collateral_alert_trader_response_->IncrementDecrementStatus << " " <<
                "SegmentIndicator: " << this_risk_collateral_alert_trader_response_->SegmentIndicator << " " <<
                "Duration: " << this_risk_collateral_alert_trader_response_->Duration << " " <<
                "FreeText1: " << this_risk_collateral_alert_trader_response_->FreeText1 << " " <<
                "BusinessUnitSymbol: " << this_risk_collateral_alert_trader_response_->BusinessUnitSymbol << " " <<
                "VarText: " << this_risk_collateral_alert_trader_response_->VarText << " " <<
                "RiskCollateralAlertBroadcastT: " << sizeof(RiskCollateralAlertBroadcastT) << " " << "\n";
  dbglogger_.DumpCurrentBuffer();

  //TODO : We are currently only passing the values to the strats same as NSE 
  //We aren't handling Broadcast Cancel / Enable IOC only modes as the BSE Engine as of now doesn't have those support added

  //These are put in for extracting margin utilized value, It's quite weird that BSE is always sending PercentageUtilized as 0 and hence can't use that 
  //but we have to get the margin value from the VarText Field 
  char risk_msg[2048];
  memset((void*)risk_msg, 0, sizeof(risk_msg));
  memcpy((void*)risk_msg, this_risk_collateral_alert_trader_response_->VarText, sizeof(this_risk_collateral_alert_trader_response_->VarText));

  PerishableStringTokenizer st_(risk_msg, sizeof(risk_msg));
  const std::vector<const char*>& tokens_ = st_.GetTokens();

  double margin_used = 0;
  std::string margin_used_string = "";
  uint32_t margin_index = 0;
  for(uint32_t tc = 0; tc < tokens_.size(); tc++){

    //Assuming the format and wording of the text won't change as no other way of extracting the values 
    //Sample msg as below
    //VarText: TM 90044  :  70 Percent MARGIN LIMIT Crossed.  Amount:  86858896 RiskCollateralAlertBroadcastT: 2088
    if(std::string("Percent") == tokens_[tc]){
      margin_index = tc - 1;	    
      break;
    }
  }

  //Valid Extraction also tested against numeric numbers between 1-99 
  if(margin_index >= 0){

    margin_used_string = tokens_[margin_index];
    std::regex pattern(R"([1-9][0-9]?)");

    if(true == std::regex_match(margin_used_string, pattern)){
      margin_used = std::stod(margin_used_string);
    }

  }

  //Only Pass to Strats if all extraction went fine
  if(margin_used > 0){
    //Since BSE dones't provide actual values of total margin / used margin and hence unlike NSE we can't precisely 
    //know if it's just crossing above 70 which is what the account manager is checking >70, hence adding 0.1 to make a breach on 70
    p_engine_listener_->SetMarginValue(margin_used+0.1);
    if(margin_used>=90){
      EnableDisableOnlyIOC(true);
    }
    // else{ //Not disabled in NSE need to check if we want to do this?
    //   EnableDisableOnlyIOC(false);
    // }
  }else{ //A margin breach msg without valid margin value, send alert once
   
    static bool alert_sent = false ;     
    dbglogger_ << "<ERROR> FAILED TO EXTRACT MARGIN PERCENT VALUE FROM THE VAR TEXT, EITHER THE FORMART / SYNTEX OF THE MSG HAS CHANGED FROM BSE" << "\n";
    dbglogger_.DumpCurrentBuffer();

    char hostname[128];
    hostname[127] = '\0';
    gethostname(hostname, 127);
    HFSAT::Email e;
    
    e.setSubject("Error: Margin Breach Risk Notification Received From Exchange But Can't Extract Value: " + std::string(hostname));
    e.addRecepient("ravi.parikh@tworoads.co.in, nseall@tworoads.co.in");
    e.addSender("ravi.parikh@tworoads.co.in");
    e.content_stream << "host_machine: " << hostname << "<br/>";
    if(false == alert_sent){
      e.sendMail();
      alert_sent = true;
    }
  }

}

void BSEEngine::GetNewPassword(char* str) {
  int today_date = HFSAT::DateTime::GetCurrentIsoDateUTC();
  int dd = today_date % 100;
  today_date /= 100;
  int mm = today_date % 100;
  today_date /= 100;
  int yy = (today_date % 10000) % 100;

  static const char *month[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

  sprintf(str, "%02d%s@%02d", dd, month[mm - 1], yy);
}

void BSEEngine::EmailPasswordChange(std::string alert_msg_) {
  // also send an alert
  char hostname[128];
  hostname[127] = '\0';
  gethostname(hostname, 127);
  std::string mode_str_ = "BSE";
  std::string alert_message = " New Password Applied For " + mode_str_ + " at " + std::string(hostname) + alert_msg_;
  HFSAT::Email e;
  struct timeval current_time;
  gettimeofday(&current_time, NULL);
  e.setSubject(mode_str_ + " -- Password Changed");
  e.addRecepient("ravi.parikh@tworoads.co.in, nseall@tworoads.co.in");
  e.addSender("ravi.parikh@tworoads.co.in");
  e.content_stream << "exch: " << mode_str_ << "<br/>";
  e.content_stream << "host_machine: " << hostname << "<br/>";
  e.content_stream << "password : " << alert_msg_ << " <br/>";
  e.content_stream << "Current Time: " << ctime(&(current_time.tv_sec)) << " <br/>";
  e.sendMail();
}


void BSEEngine::ProcessGeneralControlCommand(const char *input_stream, int stream_length) {
  dbglogger_ << "Received Control Command in BSE Engine : " << input_stream << "\n";
  dbglogger_.DumpCurrentBuffer();
  const size_t len = strlen(input_stream);
  char *temp_inp_str = new char[len + 1];
  strncpy(temp_inp_str, input_stream, len);

  try {
    PerishableStringTokenizer st_(const_cast<char *>(temp_inp_str), stream_length);
    const std::vector<const char *> &tokens_ = st_.GetTokens();

    if ((tokens_.size() >= 2) && (strcmp(tokens_[0], "CONTROLCOMMAND") == 0)) {
      if (strcmp(tokens_[2], "SwitchToPostMarketSetting") == 0) {
        if (ValidatePostMarketTimings()) {
          is_mkt_order_ = true;
          DBGLOG_CLASS_FUNC << tokens_[0] << " " << tokens_[2] << " Enabled." << DBGLOG_ENDL_FLUSH;
        }
      } else if (strcmp(tokens_[2], "SwitchToNormalMarketSetting") == 0) {  // Regular Market
        is_mkt_order_ = false;
      } else if (strcmp(tokens_[2], "BroadcastBatchCancelSAOS") == 0) {
        if( tokens_.size() < 4 ){
          DBGLOG_CLASS_FUNC_LINE_ERROR << "NOT ENOUGH ARGS PROVIDED, EXPECTED 4 ARGUMENTS, PROVIDED :" << tokens_.size() << DBGLOG_ENDL_FLUSH ;
          DBGLOG_CLASS_FUNC_LINE_INFO << "USAGE : BroadcastBatchCancel SAOS " << DBGLOG_ENDL_FLUSH;
        }else{
          p_engine_listener_->ForceBatchCancelBroadcastSAOS(tokens_[3]);
        }
      } else if (strcmp(tokens_[2], "BroadcastBatchCancelSecurity") == 0) {
        if( tokens_.size() < 4 ){
          DBGLOG_CLASS_FUNC_LINE_ERROR << "NOT ENOUGH ARGS PROVIDED, EXPECTED 4 ARGUMENTS, PROVIDED :" << tokens_.size() << DBGLOG_ENDL_FLUSH ;
          DBGLOG_CLASS_FUNC_LINE_INFO << "USAGE : BroadcastBatchCancel Shortcode" << DBGLOG_ENDL_FLUSH;
        }else{
          p_engine_listener_->ForceBatchCancelBroadcastSymbol(tokens_[3]);
        }
      } else if (strcmp(tokens_[2], "BroadcastBatchCancelAll") == 0) {
        p_engine_listener_->ForceBatchCancelBroadcastAll();
      } else if (strcmp(tokens_[2], "BroadcastBatchCancelAllOrderRemoval") == 0) {
        DBGLOG_CLASS_FUNC_LINE_INFO << "NOT IMPLEMENTED YET" << DBGLOG_ENDL_FLUSH;
//        p_engine_listener_->ForceBatchCancelBroadcastAllOrderRemoval();
      }else if ( strcmp(tokens_[2], "RiskReductionEnable") == 0) {
        DBGLOG_CLASS_FUNC_LINE_INFO << "EnableDisableOnlyIOC TRUE" << DBGLOG_ENDL_FLUSH;
        EnableDisableOnlyIOC(true);
      }else if ( strcmp(tokens_[2], "RiskReductionDisable") == 0) {
        DBGLOG_CLASS_FUNC_LINE_INFO << "EnableDisableOnlyIOC FALSE" << DBGLOG_ENDL_FLUSH;
        EnableDisableOnlyIOC(false);
      }
    }
  } catch (...) {
    dbglogger_ << "Exception in ProcessGeneralControlCommand(). But not Exiting."
               << "\n";
    dbglogger_.DumpCurrentBuffer();
  }
}

bool BSEEngine::ValidatePostMarketTimings() {
  time_t t = time(0);

  //! Note the naming convention of the variables are as per NSE market timings,
  //! but the assigned time are in GMT.
  struct tm tm_curr = *localtime(&t);
  struct tm tm_15_30 = *localtime(&t);
  struct tm tm_16_00 = *localtime(&t);

  tm_15_30.tm_hour = 10;
  tm_15_30.tm_min = 0;
  tm_15_30.tm_sec = 0;

  tm_16_00.tm_hour = 10;
  tm_16_00.tm_min = 30;
  tm_16_00.tm_sec = 0;

  time_t time_curr = mktime(&tm_curr);
  time_t time_15_30 = mktime(&tm_15_30);
  time_t time_16_00 = mktime(&tm_16_00);

  if (difftime(time_curr, time_15_30) >= 0 && difftime(time_curr, time_16_00) <= 0) {
    return true;
  }
  return false;
}

}
}

