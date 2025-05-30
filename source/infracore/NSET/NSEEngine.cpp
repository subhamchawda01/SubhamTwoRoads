// =====================================================================================
//
//       Filename:  NSEEngine.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  06/18/2014 08:50:31 AM
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

#include "dvccode/CDef/security_definitions.hpp"

#include "infracore/NSET/NSEEngine.hpp"
#include "infracore/NSET/nse_tap_invitation_manager.hpp"

#include <iostream>

#define NSE_DEBUG_INFO_CM 0
#define EXP_NUMBER 10000000000 

#define MAX_TCP_CLIENT_NORMAL_SOCKET_READ_BUFFER_LENGTH 1024

namespace HFSAT {
namespace NSE {

NSEEngine::NSEEngine(HFSAT::ORS::Settings &settings, HFSAT::DebugLogger &dbglogger, std::string output_log_dir,
                     int32_t engine_id, AsyncWriter *pWriter, AsyncWriter *pReader)
    : HFSAT::ORS::BaseEngine(settings, dbglogger),
      keep_engine_running_(false),
      last_send_time_(),
      nse_gr_socket_(nullptr),
      open_ssl_crypto_(),
      broadcast_strat_cmds(dbglogger),
      read_offset_(-1),
      processed_packet_header_(NULL),
      processed_inner_response_header_(NULL),
      processed_response_header_(NULL),
      processed_system_information_response_(NULL),
      processed_logon_response_(NULL),
      processed_gr_response_(NULL),
      processed_boxlogin_response_(NULL),
      processed_security_update_info_response_(NULL),
      processed_security_status_update_response_(NULL),
      processed_participant_update_info_response_(NULL),
      processed_index_update_info_response_(NULL),
      processed_index_map_update_info_response_(NULL),
      processed_instrument_update_info_response_(NULL),
      processed_order_response_(NULL),
      processed_trade_response_(NULL),
      processed_order_response_non_tr_(NULL),
      is_optimized_modify_supported_(false),
      is_risk_reduction_set(false),
      nse_gateway_ip_list_(),
      nse_gateway_ip_("10.23.5.204"),
      nse_gateway_port_(9602),
      dbglogger_(dbglogger),
      use_affinity_(false),
      next_message_sequnece_(0),
      last_processed_sequnce_(0),
      is_read_socket_open_(false),
      is_secure_box_registered_(false),
      is_box_logged_in_(false),
      is_logged_in_(false),
      is_connected_(false),
      allow_new_orders_(false),
      exchange_symbol_to_exchange_security_code_map_(),
      exchange_security_code_to_exchange_symbol_map_(),
      msecs_from_midnight_(0),
      last_midnight_sec_(HFSAT::DateTime::GetTimeMidnightUTC(HFSAT::DateTime::GetCurrentIsoDateLocal())),
      unique_message_sequence_to_saos_vec_(),
      branch_code_('A'),
      branch_sequence_number_(0),
      heartbeat_interval_(0),
      user_id_(0),
      pan_(""),
      broker_id_(""),
      branch_id_(0),
      version_(0),
      branch_wise_limit_(0),
      nnf_(0.0),
      price_multiplier_(settings.getIntValue("PriceMultiplier", 100)),
      account_(""),
      subaccount_(""),
      executing_giveup_firm_number_(""),
      origin_type_('\0'),
      origin_type_str_(""),
      gmd_pending_sequences_stack_(),
      container_(),
      exch_order_num_to_saos_(),
      ref_data_loader(HFSAT::Utils::NSERefDataLoader::GetUniqueInstance(HFSAT::DateTime::GetCurrentIsoDateLocal())),
      bcast_manager_(BroadcastManager::GetUniqueInstance(dbglogger, "", 0,
                                                         (atoi(settings.getValue("Client_Base").c_str())) << 16)),
      nse_daily_token_symbol_handler_(
          HFSAT::Utils::NSEDailyTokenSymbolHandler::GetUniqueInstance(HFSAT::DateTime::GetCurrentIsoDateLocal())),
      token_to_internal_exchange_symbol_(),
      nse_sec_def_(HFSAT::NSESecurityDefinitions::GetUniqueInstance(HFSAT::DateTime::GetCurrentIsoDateLocal())),
      nse_segment_type_('I'),
      fast_px_convertor_vec_(DEF_MAX_SEC_ID, NULL),
      keep_trying_different_gateways_(true),
      is_local_db_updated_(false),
      setting_(settings),
      is_mkt_order_(false),
      is_pre_open_(false),
      is_post_open(false),
      trading_ip_(""),
      trading_port_(0),
      session_key_(),
      box_id_(0),
      tap_ip_(),
      tap_gateway_(),
      tap_interface_(),
      tcp_direct_read_buffer_(NULL),
      tcp_direct_read_length_(-1),
      was_tcp_direct_read_successful_(false),
      tcp_client_normal_socket_read_buffer_(new char[MAX_TCP_CLIENT_NORMAL_SOCKET_READ_BUFFER_LENGTH]),
      tcp_client_normal_socket_read_length_(-1),
      secid_to_secinfocashmarket_(DEF_MAX_SEC_ID, NULL),
      algo_id_(0),
      dummy_order_(),
      is_alert_batch_cancellation(true),
      error_code_reason_(),
      id_(engine_id),
      pReader_(pReader),
      tcp_direct_client_zocket_(pWriter, pReader, output_log_dir + "/audit." + settings.getValue("UserName") + "." +
                                                      HFSAT::DateTime::GetCurrentIsoDateLocalAsString(),
                                settings.getValue("TAPInterface"), HFSAT::EXCHANGE_KEYS::kExchSourceNSEStr) {
  if (!settings.has("ExchangeGatewayIP") || !settings.has("ExchangeGatewayPort") ||
      !settings.has("SocketConnectHost") || !settings.has("UserName") || !settings.has("Password") ||
      !settings.has("BrokerID") || !settings.has("Version") || !settings.has("BranchID") ||
      !settings.has("BranchWiseTradingLimit") || !settings.has("NNF") || !settings.has("PriceMultiplier") ||
      !settings.has("HeartbeatIntervalInSeconds") || !settings.has("UseAffinity") || !settings.has("BoxId") ||
      !settings.has("PasswordChangeDate") || !settings.has("TAPIP") || !settings.has("TAPGateway") ||
      !settings.has("TAPInterface") || !settings.has("AlgoId")) {
    DBGLOG_CLASS_FUNC_LINE_FATAL
        << "Config File Missing One Of the, ExchangeGatewayIP, SocketConnectHost, ExchangeGatewayPort, UserName, "
           "Password, ReferenceFileCompletePath, HeartbeatIntervalInSeconds, LoginMode, "
           "SubAccount, ExecutingGiveUpFirm, OriginType, ClearingInfotag50, UseAffinity, PasswordChangeDate, TAPIP, "
           "TAPGateway, BoxId, TAPInterface AlgoId"
        << DBGLOG_ENDL_FLUSH;

    DBGLOG_DUMP;
    //    exit ( -1 ) ;
  }

  std::string openssl_certificate_file="";
  if (std::string(settings.getValue("Exchange")) == std::string("NSE_FO")){
    openssl_certificate_file="/spare/local/tradeinfo/NSE_Files/ssl_certificates/fo_certificate_file";
  }else if (std::string("NSE_EQ") == std::string(settings.getValue("Exchange"))){
    openssl_certificate_file="/spare/local/tradeinfo/NSE_Files/ssl_certificates/cm_certificate_file";
  }

  nse_gr_socket_ = new HFSAT::Utils::OpenSSLTLSClientSocket(openssl_certificate_file);

  memset((void*)tcp_client_normal_socket_read_buffer_,0,sizeof(MAX_TCP_CLIENT_NORMAL_SOCKET_READ_BUFFER_LENGTH));

  if (std::string("NSE_FO") != std::string(settings.getValue("Exchange")) &&
      std::string("NSE_CD") != std::string(settings.getValue("Exchange")) &&
      std::string("NSE_EQ") != std::string(settings.getValue("Exchange"))) {
    DBGLOG_CLASS_FUNC_LINE_FATAL << "EXCHANGE HAS TO BE EITHER NSE_CD , NSE_FO or NSE_EQ, WHAT WE HAVE IN CONFIG IS : "
                                 << settings.getValue("Exchange") << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;

    exit(-1);
  }


  // Decide segment and msg handler based on exchange provided in config
  if (std::string(settings.getValue("Exchange")) == std::string("NSE_FO")) {
    nse_segment_type_ = NSE_FO_SEGMENT_MARKING;
    nse_msgs_handler_ = new NseMsgHandlerDerivatives();
    is_optimized_modify_supported_ = true;
  } else if (std::string(settings.getValue("Exchange")) == std::string("NSE_CD")) {
    nse_segment_type_ = NSE_CD_SEGMENT_MARKING;
    nse_msgs_handler_ = new NseMsgHandlerDerivatives();
    is_optimized_modify_supported_ = true;
  } else if (std::string(settings.getValue("Exchange")) == std::string("NSE_EQ")) {
    nse_segment_type_ = NSE_EQ_SEGMENT_MARKING;
    nse_msgs_handler_ = new NseMsgHandlerCashMarket();
  }

  memset((void *)nse_msg_buffer_, 0, MAX_NSE_RESPONSE_BUFFER_SIZE);

  nse_gateway_ip_ = settings.getValue("ExchangeGatewayIP");

  // Get the valid list of gateway IPs in a string vector
  // IP list  to be input as IP1~IP2~IP3
  std::string ip_list = settings.getValue("SocketConnectHost");
  std::replace(ip_list.begin(), ip_list.end(), '~', ' ');
  HFSAT::PerishableStringTokenizer ips(const_cast<char *>((ip_list).c_str()), 1024);
  const std::vector<const char *> ip_vec = ips.GetTokens();
  for (auto ip : ip_vec) {
    nse_gateway_ip_list_.push_back(ip);
  }

  nse_gateway_port_ = settings.getIntValue("ExchangeGatewayPort", -1);

  user_id_ = settings.getIntValue("UserName", 0);
  pan_ = settings.getValue("Pan", "INVALID");
  accountnumber_ = settings.getValue("ACCOUNTNUMBER", "INVALID");
  settlor_ = settings.getValue("SETTLOR","INVALID");
  broker_id_ = settings.getValue("BrokerID");
  branch_id_ = settings.getIntValue("BranchID", -1);
  version_ = settings.getIntValue("Version", 1);
  proclient_ = settings.getIntValue("ProClient", 2); // ‘1’ - represents the client’s order. ‘2’ - represents a broker’s order
  branch_wise_limit_ = settings.getIntValue("BranchWiseTradingLimit", 0);
  nnf_ = atof(settings.getValue("NNF", "111111111111000").c_str());
  tap_gateway_ = settings.getValue("TAPGateway");
  tap_interface_ = settings.getValue("TAPInterface");
  box_id_ = (int16_t)(settings.getIntValue("BoxId", 0));
  tap_ip_ = settings.getValue("TAPIP");
  algo_id_ = settings.getIntValue("AlgoId", 0);
  new_password = settings.getValue("NewPassword", "        ");
  username = settings.getValue("UserName");

  if (pan_ == "INVALID" || pan_.size() != NSE_ORDERENTRY_REQUEST_PAN_LENGTH) {
    DBGLOG_CLASS_FUNC_LINE_FATAL << "Cannot Start Trading due to INVALID PAN " << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    std::exit(-1);
  } else {
    DBGLOG_CLASS_FUNC_LINE_INFO << "PAN : " << pan_ << DBGLOG_ENDL_FLUSH;
  }
  if (0 == algo_id_) {
    DBGLOG_CLASS_FUNC_LINE_FATAL << "ALGO ID CAN'T BE 0" << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    std::exit(-1);
  }

  DBGLOG_CLASS_FUNC_LINE_INFO << "BranchWiseTradingLimit: " << branch_wise_limit_ << DBGLOG_ENDL_FLUSH;

  if (branch_id_ == -1) {
    DBGLOG_CLASS_FUNC_LINE_ERROR << "Cannnot Start Trading REJECTED due to INVALID BRANCHID" << DBGLOG_ENDL_FLUSH;
  } else {
    DBGLOG_CLASS_FUNC_LINE_INFO << "BRANCHID : " << branch_id_ << DBGLOG_ENDL_FLUSH;
  }

  DBGLOG_CLASS_FUNC_LINE_INFO << "NNF : " << (int64_t)nnf_ << DBGLOG_ENDL_FLUSH;
  DBGLOG_CLASS_FUNC_LINE_INFO << "ProClient Set To : " << (int16_t)proclient_ << DBGLOG_ENDL_FLUSH;
  std::string password = settings.getValue("Password");

  int last_pw_change_date = settings.getIntValue("PasswordChangeDate", -1);

  if (HFSAT::DaysBetweenDates(last_pw_change_date, HFSAT::DateTime::GetCurrentIsoDateUTC()) >= 7) {
    // We have to change password
    if (new_password == "        ") {
      new_password = GetNewPassword();
    }

  } else {
    // We dont want to change password. If someone wants to explicitly change password, he should reduce
    // PasswordChangeDate and specify NewPassword.
    new_password = "        ";
  }

  std::cout << "newpass" << new_password << ".\n";

  std::string reference_file_ = settings.getValue("ReferenceFileCompletePath");

  uint32_t heartbeat_interval = settings.getIntValue("HeartbeatIntervalInSeconds", 20);

  heartbeat_interval_ = heartbeat_interval - 3;

  account_ = settings.getValue("Account");
  
  nse_msgs_handler_->gr_request_->SetPreLoadedGRRequestsFields(user_id_, broker_id_, box_id_);
  nse_msgs_handler_->secure_box_registration_request_->SetPreLoadedSecureBoxRegistrationFields(user_id_, box_id_);
  nse_msgs_handler_->boxlogin_request_->SetPreLoadedBoxLoginRequestsFields(user_id_, broker_id_, box_id_);
  nse_msgs_handler_->logon_request_->SetPreLoadedLogonRequestsFields(user_id_, password.c_str(), broker_id_.c_str(),
                                                                     version_, 5, "1234567       ",
                                                                     "TWOROADS                 ", new_password.c_str(), branch_id_);
  nse_msgs_handler_->system_info_request_->SetPreLoadedSystemInfoRequestFields(user_id_);
  nse_msgs_handler_->logout_request_.SetPreLoadedLogoffRequestFields(user_id_);
  nse_msgs_handler_->update_local_db_request_->SetPreLoadedUpdateLocalDatabaseRequestFields(user_id_);
  nse_msgs_handler_->heartbeat_request_.SetPreLoadedHeartbeatFields(user_id_);
  nse_msgs_handler_->new_order_->SetPreLoadedOrderEntryRequestFields(user_id_, broker_id_.c_str(), nnf_, algo_id_,
                                                                     pan_.c_str(), branch_id_);
  nse_msgs_handler_->new_three_leg_order->SetPreLoadedOrderEntryRequestFields(user_id_, broker_id_.c_str(), nnf_, algo_id_,
                                                                     pan_.c_str(), branch_id_);
  nse_msgs_handler_->spread_new_order_->SetPreLoadedOrderEntryRequestFields(user_id_, broker_id_.c_str(), nnf_, algo_id_,
                                                                     pan_.c_str(), branch_id_);
  nse_msgs_handler_->cancel_order_->SetPreLoadedOrderCancelRequestFields(user_id_, broker_id_.c_str(), nnf_, algo_id_,
                                                                         pan_.c_str(), branch_id_);
  nse_msgs_handler_->modify_order_->SetPreLoadedOrderModifyRequestFields(user_id_, broker_id_.c_str(), nnf_, algo_id_,
                                                                         pan_.c_str(), branch_id_);
  nse_msgs_handler_->spread_cancel_order_.SetPreLoadedOrderEntryRequestFields(user_id_, broker_id_.c_str(), nnf_, branch_id_);
  nse_msgs_handler_->spread_modify_order_.SetPreLoadedOrderEntryRequestFields(user_id_, broker_id_.c_str(), nnf_, branch_id_);

  pr_modify_order_.SetPreLoadedOrderEntryRequestFields(user_id_);
  nse_msgs_handler_->kill_switch_order->SetPreLoadedKillSwitchRequestFields(user_id_, broker_id_.c_str(), nnf_, algo_id_,
                                                                     pan_.c_str(), branch_id_);
  nse_msgs_handler_->kill_switch_order->SetTransactionCode(KILL_SWITCH_IN);

  if ( proclient_ != 2){
    DBGLOG_CLASS_FUNC_LINE_INFO << "Setting ProClient : " << (int16_t)proclient_ << DBGLOG_ENDL_FLUSH;
    nse_msgs_handler_->new_order_->SetProClient(proclient_);
    nse_msgs_handler_->modify_order_->SetProClient(proclient_);
    nse_msgs_handler_->cancel_order_->SetProClient(proclient_);
    nse_msgs_handler_->spread_new_order_->SetProClient(proclient_);
    nse_msgs_handler_->spread_cancel_order_.SetProClient(proclient_);
    nse_msgs_handler_->spread_modify_order_.SetProClient(proclient_);
    nse_msgs_handler_->kill_switch_order->SetProClient(proclient_);
  }
  settlor_ = getRightPaddingString(settlor_, 12, ' ');
  if ( settlor_.find("INVALID") == std::string::npos){
    DBGLOG_CLASS_FUNC_LINE_INFO << "Setting Settlor : " << settlor_ << DBGLOG_ENDL_FLUSH;
    nse_msgs_handler_->new_order_->SetSettlor(settlor_.c_str());
    nse_msgs_handler_->kill_switch_order->SetSettlor(settlor_.c_str());
    nse_msgs_handler_->modify_order_->SetSettlor(settlor_.c_str());
    nse_msgs_handler_->cancel_order_->SetSettlor(settlor_.c_str());
    nse_msgs_handler_->kill_switch_order->SetSettlor(settlor_.c_str());
//    nse_msgs_handler_->spread_new_order_.SetSettlor(settlor_);
//    nse_msgs_handler_->spread_cancel_order_.SetSettlor(settlor_);
//    nse_msgs_handler_->spread_modify_order_.SetSettlor(settlor_);
  }

   // Currently Broker ID is Set in AccountNumber
  accountnumber_ = getRightPaddingString(accountnumber_, 10, ' ');
  if ( accountnumber_.find("INVALID") == std::string::npos){
    if ( nse_segment_type_ == NSE_FO_SEGMENT_MARKING){
	DBGLOG_CLASS_FUNC_LINE_INFO << "Setting AccountNumber FO : " << accountnumber_ << DBGLOG_ENDL_FLUSH; // default setaccount for FO should be blank
	nse_msgs_handler_->new_order_->UpdateAccountNumber(accountnumber_.c_str());
        nse_msgs_handler_->modify_order_->UpdateAccountNumber(accountnumber_.c_str());
        nse_msgs_handler_->cancel_order_->UpdateAccountNumber(accountnumber_.c_str());
        nse_msgs_handler_->kill_switch_order->UpdateAccountNumber(accountnumber_.c_str());
    }
    else {
    	DBGLOG_CLASS_FUNC_LINE_INFO << "Setting AccountNumber : " << accountnumber_ << DBGLOG_ENDL_FLUSH;
    	nse_msgs_handler_->new_order_->SetAccountNumber(accountnumber_.c_str());
    	nse_msgs_handler_->modify_order_->SetAccountNumber(accountnumber_.c_str());
    	nse_msgs_handler_->cancel_order_->SetAccountNumber(accountnumber_.c_str());
        nse_msgs_handler_->kill_switch_order->SetAccountNumber(accountnumber_.c_str());
    }
//    nse_msgs_handler_->spread_new_order_.SetAccountNumber(accountnumber_);
//    nse_msgs_handler_->spread_cancel_order_.SetAccountNumber(accountnumber_);
//    nse_msgs_handler_->spread_modify_order_.SetAccountNumber(accountnumber_);
  }
  nse_msgs_handler_->kill_switch_order->SetMd5sum();
  memset((void *)nse_msg_buffer_, 0, MAX_NSE_RESPONSE_BUFFER_SIZE);
  read_offset_ = 0;
  keep_engine_running_ = true;

  for (uint32_t ii = 0; ii < DEF_MAX_SEC_ID; ii++) {
    secid_to_secinfocashmarket_[ii] = new SecurityInfoCashMarket();
  }

  // By default start the ORS in Pre-Open Mode based on market timings
  is_pre_open_ = ValidatePreOpenMarketTimings();

  send_write_length = nse_msgs_handler_->new_order_->GetOrderEntryMsgLength();

  std::cout << "SEND WRITE LENGTH INITALIZE " << send_write_length << std::endl;
  send_three_leg_write_length = nse_msgs_handler_->new_three_leg_order->GetThreeLegOrderEntryMsgLength();
  send_spread_write_length = nse_msgs_handler_->spread_new_order_->GetSpreadOrderEntryMsgLength();
  cancel_write_length = nse_msgs_handler_->cancel_order_->GetOrderCancelMsgLength();
  modify_write_length = nse_msgs_handler_->modify_order_->GetOrderModifyMsgLength();
  
  modify_price_write_length = sizeof(OrderPriceModifyRequest);
  order_entry_request_buffer_ = (void *)nse_msgs_handler_->new_order_->GetOrderEntryRequestBuffer();
  order_entry_three_leg_request_buffer_ = (void *)nse_msgs_handler_->new_three_leg_order->GetOrderEntryThreeLegRequestDerivativesBuffer();
  order_entry_spread_request_buffer_ = (void *)nse_msgs_handler_->spread_new_order_->GetSpreadOrderEntryRequestBuffer();
  order_modify_request_buffer_ = (void *)nse_msgs_handler_->modify_order_->GetOrderModifyRequestBuffer();
  order_cancel_request_buffer_ = (void *)nse_msgs_handler_->cancel_order_->GetOrderCancelRequestBuffer();
  price_modify_request_buffer_ = pr_modify_order_.GetOrderPriceModifyRequestBuffer();
  for (int i=0 ; i< (DEF_MAX_SEC_ID+1);i++){
    kill_switch_msg_buffer[i] = nullptr;
  }
 
  LoadExchageRejects(std::string(settings.getValue("Exchange")));
// KillSwitchNewForSecId(0);
  kill_switch_msg_length = nse_msgs_handler_->kill_switch_order->GetKillSwitchMsgLength();
  //std::cout<<"kill_switch_msg_length: "<<kill_switch_msg_length<<std::endl;
}

void NSEEngine::LoadExchageRejects(std::string segment){

  dbglogger_ << "Loading Exchange Rejects File" << "\n";
  std::ostringstream t_temp_oss;

  t_temp_oss << "/home/pengine/prod/live_configs/error_code_description";
  std::string exchange_rejects_file = t_temp_oss.str();
  dbglogger_ << exchange_rejects_file;
  dbglogger_.DumpCurrentBuffer();
  dbglogger_ << "\n";

  if (HFSAT::FileUtils::ExistsAndReadable(exchange_rejects_file)) {

    std::ifstream rejects_file_;
    rejects_file_.open(exchange_rejects_file.c_str(), std::ifstream::in);
    char readline_buffer_[1024];
    if (rejects_file_.is_open()) {
      while (rejects_file_.good()) {
        memset(readline_buffer_, 0, sizeof(readline_buffer_));
        rejects_file_.getline(readline_buffer_, sizeof(readline_buffer_));
        std::vector<char*> tokens_;
        std::string trimmed_str_;
        // create a copy of line read before using non-const tokenizer
        char readline_buffer_copy_[1024];
        memset(readline_buffer_copy_, 0, sizeof(readline_buffer_copy_));
        strcpy(readline_buffer_copy_, readline_buffer_);

        HFSAT::PerishableStringTokenizer::NonConstStringTokenizer(readline_buffer_copy_, " ", tokens_);

        if (3!=tokens_.size()) continue;

        std::string exchange_ = "NSE_"+std::string(tokens_[0]);

        if(exchange_!=segment) continue;

        int error_code = atoi(tokens_[1]);
        std::string error_reason =  tokens_[2];
        //dbglogger_ << error_code << " " << error_reason << "\n";
        //dbglogger_.DumpCurrentBuffer();
        error_code_reason_[error_code]=error_reason;

      }  // end while
      //std::cout << "Closing the file now" <<std::endl;
    } 
    rejects_file_.close();
  } else {
    std::cerr << "Fatal error - could not read Exchange Rejects file " << exchange_rejects_file << "\n";
  }
 return;
}

std::string NSEEngine::GetNewPassword() {
  int today_date = HFSAT::DateTime::GetCurrentIsoDateUTC();
  int dd = today_date % 100;
  today_date /= 100;
  int mm = today_date % 100;
  today_date /= 100;
  int yy = (today_date % 10000) % 100;

  static const char *month[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

  char str[10];
  sprintf(str, "%02d%s@%02d", dd, month[mm - 1], yy);
  std::string new_password = str;

  return new_password;
}

void NSEEngine::CleanUp() {
  // Ensure the SAOS and BranchCode/Sequence Are Always in sync
  HFSAT::ORS::SequenceGenerator::GetUniqueInstance().persist_seq_num();
}

NSEEngine::~NSEEngine() {
  // Deallocate fast_px_convertor_vec_
  for (int secid = 0; secid < DEF_MAX_SEC_ID; secid++) {
    if (fast_px_convertor_vec_[secid] != NULL) {
      delete fast_px_convertor_vec_[secid];
      fast_px_convertor_vec_[secid] = NULL;
    }
  }
}

void NSEEngine::Connect() {

  //For testing latency on crypto lib
//  InstrumentDesc *inst_desc = &container_.inst_desc_[0];
//  SecurityInfoCashMarket *sec_info = secid_to_secinfocashmarket_[0];
//  nse_msgs_handler_->new_order_->SetDynamicOrderEntryRequestFields(0, 0, 0, &dummy_order_, inst_desc, sec_info, false, false);
//
//  std::string temp_key = "ABCDEFGHIJKLMNOPQRSTUVWXYZ123456";
//  std::string crypto_iv = "919980189377";
//  open_ssl_crypto_.encrypt_EVP_aes_256_gcm_init((unsigned char*)temp_key.c_str(),(unsigned char*)crypto_iv.c_str());
//
//  for(int i=0; i<10000; i++){
//   printHexString((const char*)order_entry_request_buffer_+22, send_write_length - 22);
//
//   HFSAT::CpucycleProfiler::GetUniqueInstance().Start(1); 
//   open_ssl_crypto_.aes_encrypt((unsigned char*)(order_entry_request_buffer_+22), send_write_length - 22);
//   HFSAT::CpucycleProfiler::GetUniqueInstance().End(1); 
//
//   std::cout << " ================== ENCRYPT ==================="; 
//   printHexString((const char*)open_ssl_crypto_.encrypt_text, open_ssl_crypto_.encrypt_len);
//  }
//
//  std::cout << "Summary " << HFSAT::CpucycleProfiler::GetUniqueInstance(9).GetCpucycleSummaryString() << std::endl;
//  std::exit(-1);
     
  struct timeval login_time;
  if (is_connected_) return;
  for (auto ip : nse_gateway_ip_list_) {
    nse_gateway_ip_ = ip;
    is_connected_ = true;  // making this true before Connect() call to avoid race condition from reply thread

    system("route");

    std::string route_cmd = "sudo ip route del " + nse_gateway_ip_ + "/32";
    std::cout << " INITIATING ROUTING CHNAGES...." << route_cmd << std::endl;

    system(route_cmd.c_str());

    route_cmd = "sudo ip route add " + nse_gateway_ip_ + "/32 via " + tap_gateway_ + " dev " + tap_interface_ +
                " src " + tap_ip_;
    system(route_cmd.c_str());

    std::cout << " UPDATED ROUTING TABLE NOW... " << route_cmd << std::endl;

    system("route");

    nse_gr_socket_->Connect(nse_gateway_ip_, nse_gateway_port_);

    gettimeofday(&login_time, NULL);
    DBGLOG_CLASS_FUNC_LINE_INFO << "Trying IP: " << nse_gateway_ip_ << ":" << nse_gateway_port_
                                << " @ :" << login_time.tv_sec << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;

    GRRequest();

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

void NSEEngine::GRRequest() {
  if (is_logged_in_) return;

  if (!is_connected_) {
    DBGLOG_CLASS_FUNC_LINE_FATAL << "LOGIN CALLED WITHOUT BEING CONNECTED" << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    exit(-1);
  }

  next_message_sequnece_++;
  nse_msgs_handler_->gr_request_->SetDynamicGRRequestsFieldsAndUpdateChecksum(next_message_sequnece_);

  last_send_time_ = time(NULL);
  int32_t write_length = nse_gr_socket_->WriteN(nse_msgs_handler_->gr_request_->GetGRRequestMsgLength(),
                                                (void *)nse_msgs_handler_->gr_request_->GetGRRequestBuffer());

  DBGLOG_CLASS_FUNC_LINE_INFO << "WROTE : " << write_length << DBGLOG_ENDL_FLUSH;
  DBGLOG_DUMP;

  if (write_length < (int32_t)nse_msgs_handler_->gr_request_->GetGRRequestMsgLength()) {
    DBGLOG_CLASS_FUNC_LINE_FATAL << "FAILD TO WRITE COMPLETE MESSAGE TO EXCHANGE SOCKET: WRITE_LENGTH: " << write_length <<  DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    exit(-1);
  }

  char buffer[1024];
  memset((void *)buffer, 0, 1024);

  int32_t read_length = nse_gr_socket_->ReadN(1024, buffer);

  ProcessExchangeResponse(buffer, read_length);
}

void NSEEngine::SecureBoxRegistrationRequest() {
  if (is_logged_in_) return;

  if (!is_connected_) {
    DBGLOG_CLASS_FUNC_LINE_FATAL << "SecureBoxRegistrationRequest CALLED WITHOUT BEING CONNECTED" << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    exit(-1);
  }

  if ("0" == trading_ip_ || 0 == trading_port_) {
    DBGLOG_CLASS_FUNC_LINE_FATAL << "COULDN'T GET IP AND PORT FROM EXCHANGE FOR LOGIN" << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    exit(-1);
  }

  std::cout << " CONNECTING TO : " << trading_ip_ << " " << trading_port_ << std::endl;
  system("route");
  std::cout << " INITIATING ROUTING CHNAGES...." << std::endl;
  std::string route_cmd = "sudo ip route del " + trading_ip_ + "/32";
  system(route_cmd.c_str());
  route_cmd =
      "sudo ip route add " + trading_ip_ + "/32 via " + tap_gateway_ + " dev " + tap_interface_ + " src " + tap_ip_;
  system(route_cmd.c_str());

  std::cout << " UPDATED ROUTING TABLE NOW... " << std::endl;
  system("route");

  tcp_direct_client_zocket_.ConnectAndAddZocketToMuxer(trading_ip_, trading_port_);

  next_message_sequnece_++;
  nse_msgs_handler_->secure_box_registration_request_->SetDynamicSecureBoxRegistrationFieldsAndUpdateChecksum(next_message_sequnece_);

  std::cout << "WRITING MSG TO NSE : " << next_message_sequnece_ << std::endl;

  last_send_time_ = time(NULL);

  int32_t write_length = tcp_direct_client_zocket_.WriteLockFreeN(nse_msgs_handler_->secure_box_registration_request_->GetSecureBoxRegistrationMsgLength(),
                                                (void *)nse_msgs_handler_->secure_box_registration_request_->GetSecureBoxRegistrationBuffer());

  DBGLOG_CLASS_FUNC_LINE_INFO << "WROTE : " << write_length << DBGLOG_ENDL_FLUSH;
  DBGLOG_DUMP;

  if (write_length < (int32_t)nse_msgs_handler_->secure_box_registration_request_->GetSecureBoxRegistrationMsgLength()) {
    DBGLOG_CLASS_FUNC_LINE_FATAL << "FAILD TO WRITE COMPLETE MESSAGE TO EXCHANGE SOCKET: WRITE_LENGTH: " << write_length <<  DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    exit(-1);
  }
  is_read_socket_open_ = true;

}


void NSEEngine::BoxLoginRequest() {
  if (is_logged_in_) return;

  if (!is_connected_) {
    DBGLOG_CLASS_FUNC_LINE_FATAL << "LOGIN CALLED WITHOUT BEING CONNECTED" << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    exit(-1);
  }

  std::cout << "Processing Encryption BoxLogin Request " << std::endl;

  next_message_sequnece_++;
  nse_msgs_handler_->boxlogin_request_->SetDynamicBoxLoginRequestsFieldsAndUpdateChecksum(next_message_sequnece_,
                                                                                          session_key_);

  last_send_time_ = time(NULL);

  char send_buffer[1024];
  memset((void*)send_buffer,0,1024); 
  memcpy((void*)send_buffer, (void *)nse_msgs_handler_->boxlogin_request_->GetBoxLoginRequestBuffer(), 22);

  open_ssl_crypto_.aes_encrypt((unsigned char*)nse_msgs_handler_->boxlogin_request_->GetBoxLoginRequestBuffer()+22, nse_msgs_handler_->boxlogin_request_->GetBoxLoginRequestMsgLength() - 22);

  memcpy((void*)(send_buffer+22), (void*)open_ssl_crypto_.encrypt_text, open_ssl_crypto_.encrypt_len);
  int32_t write_length = tcp_direct_client_zocket_.WriteLockFreeN(22+open_ssl_crypto_.encrypt_len, (void*)send_buffer);

  std::cout << "Write Login From TCP Direct : " << write_length << std::endl;


  DBGLOG_CLASS_FUNC_LINE_INFO << "WROTE : " << write_length << DBGLOG_ENDL_FLUSH;
  DBGLOG_DUMP;

  if (write_length < (int32_t)nse_msgs_handler_->boxlogin_request_->GetBoxLoginRequestMsgLength()) {
    DBGLOG_CLASS_FUNC_LINE_FATAL << "FAILD TO WRITE COMPLETE MESSAGE TO EXCHANGE SOCKET: WRITE_LENGTH: " << write_length << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    exit(-1);
  }

  std::cout << " DONE WITH BOXLOGIN : " << std::endl;
}

void NSEEngine::Login() {
  
  if (is_logged_in_) return;

  if (!is_connected_ || !is_box_logged_in_) {
    DBGLOG_CLASS_FUNC_LINE_FATAL << "LOGIN CALLED WITHOUT BEING CONNECTED" << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    exit(-1);
  }

  next_message_sequnece_++;
  nse_msgs_handler_->logon_request_->SetDynamicLogonRequestsFieldsAndUpdateChecksum(next_message_sequnece_);

  std::cout << "LENGTH OF LOGIN MSG : " << nse_msgs_handler_->logon_request_->GetLogonRequestMsgLength() << std::endl;
  std::cout << "Setting Time : " << std::endl;
  last_send_time_ = time(NULL);

  std::cout << "Going for Login Encryption Using NOrmal Socket : "<< std::endl;

  char send_buffer[1024];
  memset((void*)send_buffer,0,1024); 
  std::cout << "Copying Packet Header into login msg " << std::endl;
  memcpy((void*)send_buffer, (void *)nse_msgs_handler_->logon_request_->GetLogonRequestBuffer(), 22);
  std::cout << "Copied Packet Header into login msg " << std::endl;

  std::cout << "Calling Encrypt Function For Length : " << nse_msgs_handler_->logon_request_->GetLogonRequestMsgLength() - 22 << std::endl;

  open_ssl_crypto_.aes_encrypt((unsigned char*)(nse_msgs_handler_->logon_request_->GetLogonRequestBuffer()+22), nse_msgs_handler_->logon_request_->GetLogonRequestMsgLength() - 22);

  std::cout << "Encrypted Length : " << open_ssl_crypto_.encrypt_len << std::endl;

  memcpy((void*)(send_buffer+22), (void*)open_ssl_crypto_.encrypt_text, open_ssl_crypto_.encrypt_len);
  int32_t write_length = tcp_direct_client_zocket_.WriteLockFreeN(22+open_ssl_crypto_.encrypt_len, (void*)send_buffer);

  std::cout << "Write length here for box login : "<< write_length << std::endl;

  std::cout << " SENT : " << write_length << " " << std::endl;

  if (write_length < (int32_t)nse_msgs_handler_->logon_request_->GetLogonRequestMsgLength()) {
    DBGLOG_CLASS_FUNC_LINE_FATAL << "FAILD TO WRITE COMPLETE MESSAGE TO EXCHANGE SOCKET: WRITE_LENGTH: " << write_length << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    exit(-1);
  }

}

void NSEEngine::Logout() {

  //std::cout << "Summary " << HFSAT::CpucycleProfiler::GetUniqueInstance(9).GetCpucycleSummaryString() << std::endl;
 
  if (!is_logged_in_) {
    DBGLOG_CLASS_FUNC_LINE_ERROR << "DISCONNECT CALLED WITHOUT BEING LOGGED IN" << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
  }
  if (!is_connected_) {
    DBGLOG_CLASS_FUNC_LINE_ERROR << "DISCONNECT CALLED WITHOUT BEING CONNECTED" << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
  }

  next_message_sequnece_++;
  nse_msgs_handler_->logout_request_.SetDynamicLogoffRequestFields(next_message_sequnece_);

  std::cout << "Going for Logout Encryption Using NOrmal Socket : "<< std::endl;

  char send_buffer[1024];
  memset((void*)send_buffer,0,1024); 
  memcpy((void*)send_buffer, (void *)nse_msgs_handler_->logout_request_.GetLogoffRequestBuffer(), 22);

  std::cout << "Calling Encrypt Function For Length : " << NSE_LOGOFF_REQUEST_LENGTH - 22 << std::endl;

  open_ssl_crypto_.aes_encrypt((unsigned char*)(nse_msgs_handler_->logout_request_.GetLogoffRequestBuffer()+22), NSE_LOGOFF_REQUEST_LENGTH - 22);

  std::cout << "Encrypted Length : " << open_ssl_crypto_.encrypt_len << std::endl;

  memcpy((void*)(send_buffer+22), (void*)open_ssl_crypto_.encrypt_text, open_ssl_crypto_.encrypt_len);
  int32_t write_length = tcp_direct_client_zocket_.WriteLockFreeN(22+open_ssl_crypto_.encrypt_len, (void*)send_buffer);

  std::cout << "Write length here for box logout : "<< write_length << std::endl;

  if (write_length < (int32_t)NSE_LOGOFF_REQUEST_LENGTH) {
    DBGLOG_CLASS_FUNC_LINE_FATAL << "FAILD TO WRITE COMPLETE MESSAGE TO EXCHANGE SOCKET: WRITE_LENGTH: " << write_length << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
   //exit(-1);
  }

}

void NSEEngine::DisConnect() {
  
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

void NSEEngine::SendSystemInformationRequest() {
 
  if (!is_logged_in_ || !is_connected_) {
    return;
  }

  if (nse_segment_type_ == NSE_CD_SEGMENT_MARKING) {
    sleep(2);
  }

  next_message_sequnece_++;
  last_send_time_ = time(NULL);

  nse_msgs_handler_->system_info_request_->SetDynamicSystemInfoRequestFieldsAndUpdateChecksum(next_message_sequnece_);

  std::cout << "Going for SendSystemInformationRequest Encryption Using NOrmal Socket : "<< std::endl;

  char send_buffer[1024];
  memset((void*)send_buffer,0,1024); 
  memcpy((void*)send_buffer, (void *)nse_msgs_handler_->system_info_request_->GetSystemInfoRequestBuffer(), 22);
  std::cout << "Calling Encrypt Function For Length : " << nse_msgs_handler_->system_info_request_->GetSystemInfoMsgLength() - 22 << std::endl;

  open_ssl_crypto_.aes_encrypt((unsigned char*)(nse_msgs_handler_->system_info_request_->GetSystemInfoRequestBuffer()+22), nse_msgs_handler_->system_info_request_->GetSystemInfoMsgLength() - 22);
  std::cout << "Encrypted Length : " << open_ssl_crypto_.encrypt_len << std::endl;

  memcpy((void*)(send_buffer+22), (void*)open_ssl_crypto_.encrypt_text, open_ssl_crypto_.encrypt_len);
  int32_t write_length = tcp_direct_client_zocket_.WriteLockFreeN(22+open_ssl_crypto_.encrypt_len, (void*)send_buffer);

  std::cout << "Write length here for SendSystemInformationRequest : "<< write_length << std::endl;
  
  if (write_length < (int32_t)nse_msgs_handler_->system_info_request_->GetSystemInfoMsgLength()) {
    DBGLOG_CLASS_FUNC_LINE_FATAL << "FAILD TO WRITE COMPLETE MESSAGE TO EXCHANGE SOCKET: WRITE_LENGTH: " << write_length << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    exit(-1);
  }

}

void NSEEngine::SendUpdateLocalDBRequest(st_market_status &st_mkt_status, st_ex_market_status &st_ex_mkt_status,
                                         st_pl_market_status &st_pl_mkt_status, int16_t call_auction_1,
                                         int16_t call_auction_2) {
 
  if (!is_logged_in_ || !is_connected_) {
    return;
  }

  if (nse_segment_type_ == NSE_CD_SEGMENT_MARKING) {
    sleep(2);
  }

  next_message_sequnece_++;
  last_send_time_ = time(NULL);

  nse_msgs_handler_->update_local_db_request_->SetDynamicUpdateLocalDatabaseRequestFields(
      next_message_sequnece_, st_mkt_status, st_ex_mkt_status, st_pl_mkt_status, call_auction_1, call_auction_2);

  std::cout << "Going for SendUpdateLocalDBRequest Encryption Using NOrmal Socket : "<< std::endl;

  char send_buffer[1024];
  memset((void*)send_buffer,0,1024); 
  memcpy((void*)send_buffer, (void *)nse_msgs_handler_->update_local_db_request_->GetUpdateLocalDatabaseRequestBuffer(), 22);
  std::cout << "Calling Encrypt Function For Length : " << nse_msgs_handler_->update_local_db_request_->GetUpdateLocalDatabaseRequestLength() - 22 << std::endl;

  open_ssl_crypto_.aes_encrypt((unsigned char*)(nse_msgs_handler_->update_local_db_request_->GetUpdateLocalDatabaseRequestBuffer()+22), nse_msgs_handler_->update_local_db_request_->GetUpdateLocalDatabaseRequestLength() - 22);
  std::cout << "Encrypted Length : " << open_ssl_crypto_.encrypt_len << std::endl;

  memcpy((void*)(send_buffer+22), (void*)open_ssl_crypto_.encrypt_text, open_ssl_crypto_.encrypt_len);
  int32_t write_length = tcp_direct_client_zocket_.WriteLockFreeN(22+open_ssl_crypto_.encrypt_len, (void*)send_buffer);

  std::cout << "Write length here for SendSystemInformationRequest : "<< write_length << std::endl;

  if (write_length < (int32_t)nse_msgs_handler_->update_local_db_request_->GetUpdateLocalDatabaseRequestLength()) {
    DBGLOG_CLASS_FUNC_LINE_FATAL << "FAILD TO WRITE COMPLETE MESSAGE TO EXCHANGE SOCKET: WRITE_LENGTH: " << write_length << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    exit(-1);
  }

  std::cout << "DONE WITH UPDATE DB " << std::endl;

}

inline void NSEEngine::SendOrder(HFSAT::ORS::Order *order, InstrumentDesc *inst_desc) {
//  dbglogger_ << "sendorder\n";
//  std::cout<<"sendorder"<<std::endl;
  int16_t buy_sell = (order->buysell_ == kTradeTypeBuy) ? 1 : 2;
  int32_t price = (int32_t)((order->price_ * price_multiplier_) + 0.5);
  if ( is_risk_reduction_set ) {
	order->is_ioc = true;
	DBGLOG_CLASS_FUNC_LINE_FATAL << "Only IOC Order Sent " << order->toStringShort();
 	DBGLOG_DUMP;	
  }
  next_message_sequnece_++;

#if NSE_DEBUG_INFO_CM
  DBGLOG_CLASS_FUNC_LINE_INFO << "NEW ORDER REQUEST::\n" << order->toString() << DBGLOG_ENDL_FLUSH;
  DBGLOG_CLASS_FUNC_LINE << "BuySell: " << buy_sell << " Price: " << price << " Size: " << order->size_remaining_
                         << " Size Disclosed: " << order->size_disclosed_
                         << " Saos: " << order->server_assigned_order_sequence_ << " BranchID: " << branch_id_
                         << " NNF: " << nnf_ << DBGLOG_ENDL_FLUSH;

#endif

  //HFSAT::CpucycleProfiler::GetUniqueInstance().Start(2); 
  nse_msgs_handler_->new_order_->SetDynamicOrderEntryRequestFields(
      next_message_sequnece_, buy_sell, price, order, inst_desc, secid_to_secinfocashmarket_[order->security_id_],
      is_mkt_order_, is_pre_open_);
  //HFSAT::CpucycleProfiler::GetUniqueInstance().End(2); 

  char send_buffer[1024];
  memset((void*)send_buffer,0,1024); 
  memcpy((void*)send_buffer, (void *)order_entry_request_buffer_, 22);

  //HFSAT::CpucycleProfiler::GetUniqueInstance().Start(1); 
  open_ssl_crypto_.aes_encrypt((unsigned char*)((char*)order_entry_request_buffer_+22), send_write_length - 22);
  //HFSAT::CpucycleProfiler::GetUniqueInstance().End(1); 

  memcpy((void*)(send_buffer+22), (void*)open_ssl_crypto_.encrypt_text, open_ssl_crypto_.encrypt_len);

  //HFSAT::CpucycleProfiler::GetUniqueInstance().Start(3); 
  int32_t write_length = tcp_direct_client_zocket_.WriteLockFreeN(22+open_ssl_crypto_.encrypt_len, (void*)send_buffer);
  //HFSAT::CpucycleProfiler::GetUniqueInstance().End(3); 

  order->ors_end_cycle_count_ = HFSAT::GetCpucycleCountForTimeTick();

#if NSE_DEBUG_INFO_CM
  std::cout << "Send Order : " << order->server_assigned_order_sequence_ << std::endl;
  printHexString((char *)order_entry_request_buffer_, send_write_length);
#endif

  if (write_length < (int32_t)send_write_length) {
    DBGLOG_CLASS_FUNC_LINE_FATAL << "FAILD TO WRITE COMPLETE MESSAGE TO EXCHANGE SOCKET: WRITE_LENGTH: " << write_length << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    exit(-1);
  }
 // static int numorder_send=1;
 // if (numorder_send==10){
  //  p_engine_listener_->SetMarginValue(71.5);
   // KillSwitch();
 // }
  // if(numorder_send==20){
  //   p_engine_listener_->SetMarginValue(81.5);
  //   KillSwitch();
  // }
  //numorder_send++;

}


inline void NSEEngine::KillSwitchNewForSecId( int32_t id) {
  int sec_id = id + 1;
  if (sec_id != 0)
  {
    nse_msgs_handler_->kill_switch_order->SetSecInfo(secid_to_secinfocashmarket_[sec_id-1]);
    std::cout<<"inst_desc_:: "<< (container_.inst_desc_[sec_id-1].ToString());
    nse_msgs_handler_->kill_switch_order->SetInstrumentDescInfo(&container_.inst_desc_[sec_id-1]);
    nse_msgs_handler_->kill_switch_order->SetMd5sum();
  }
  
  std::cout<<"here KillSwitchNewForSecId"<<std::endl;
  //open_ssl_crypto_.aes_encrypt((unsigned char*)((char*)(nse_msgs_handler_->kill_switch_order->GetKillSwitchRequestBuffer())+22), (nse_msgs_handler_->kill_switch_order->GetKillSwitchMsgLength()) - 22);
  kill_switch_msg_buffer[sec_id] = new char[nse_msgs_handler_->kill_switch_order->GetKillSwitchMsgLength()];
  memcpy((void*)kill_switch_msg_buffer[sec_id], (void*)(nse_msgs_handler_->kill_switch_order->GetKillSwitchRequestBuffer()), kill_switch_msg_length);

  // no encryption as it changes with run of same buffer.       //memcpy((void*)(kill_switch_msg_buffer[sec_id]+22), (void*)open_ssl_crypto_.encrypt_text, open_ssl_crypto_.encrypt_len);
  std::cout<<"KILLSWITCH for SEC_ID: "<< sec_id-1 <<" "<<(kill_switch_msg_buffer[sec_id])<<"\n";
 printHexString(kill_switch_msg_buffer[sec_id],nse_msgs_handler_->kill_switch_order->GetKillSwitchMsgLength());
}

inline void NSEEngine::KillSwitch(int id){
  int sec_id = id + 1;
  next_message_sequnece_++;
  nse_msgs_handler_->kill_switch_order->SetDynamicKillSwitchRequestFields(next_message_sequnece_);
  char killswitch_buffer[1024];
  memset((void*)killswitch_buffer,0,1024);
  std::cout<<"/n KILLSWITCH"<<std::endl;
  printHexString(kill_switch_msg_buffer[sec_id],nse_msgs_handler_->kill_switch_order->GetKillSwitchMsgLength());
  memcpy((void*)killswitch_buffer, (void*)(nse_msgs_handler_->kill_switch_order->GetKillSwitchRequestBuffer()), size_t(6));
  memcpy((void*)(killswitch_buffer+6), (void*)(kill_switch_msg_buffer[sec_id]+6), 16);
  open_ssl_crypto_.aes_encrypt((unsigned char*)((char*)(kill_switch_msg_buffer[sec_id])+22), (nse_msgs_handler_->kill_switch_order->GetKillSwitchMsgLength()) - 22);
  memcpy((void*)(killswitch_buffer+22), (void*)open_ssl_crypto_.encrypt_text, open_ssl_crypto_.encrypt_len);
  std::cout<< " PreComputed KILLSWITCH for secid: "<<sec_id-1 <<" " << open_ssl_crypto_.encrypt_len<<" "<<kill_switch_msg_length<<"\n";
  printHexString(killswitch_buffer,kill_switch_msg_length);
  dbglogger_ << " PreComputed KILLSWITCH for secid: "<<sec_id-1 <<"\n";
  dbglogger_.DumpCurrentBuffer();
  int32_t write_length = tcp_direct_client_zocket_.WriteLockFreeN(kill_switch_msg_length, killswitch_buffer);
  dbglogger_.DumpCurrentBuffer();
  if (write_length < (int32_t)kill_switch_msg_length) {
    DBGLOG_CLASS_FUNC_LINE_FATAL << "FAILD TO WRITE COMPLETE MESSAGE TO EXCHANGE SOCKET: WRITE_LENGTH: " << write_length << " KILLSWITCHORDER " <<DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
  }
}

inline void NSEEngine::CancelOrder(HFSAT::ORS::Order *order, InstrumentDesc *inst_desc) {
//  std::cout<<"CancelOrder"<<std::endl;
//  dbglogger_ << "CancelOrder\n";

  int16_t buy_sell = (order->buysell_ == kTradeTypeBuy) ? 1 : 2;
  int32_t price = (int32_t)(order->price_ * price_multiplier_);
  next_message_sequnece_++;

#if NSE_DEBUG_INFO_CM
  DBGLOG_CLASS_FUNC_LINE_INFO << "CANCEL ORDER REQUEST::\n" << order->toString() << DBGLOG_ENDL_FLUSH;
  DBGLOG_CLASS_FUNC_LINE << "BuySell: " << buy_sell << " Price: " << price << " Size: " << order->size_remaining_
                         << " Size Disclosed: " << order->size_disclosed_
                         << " Saos: " << order->server_assigned_order_sequence_
                         << " OrderNum: " << (int64_t)order->exch_assigned_seq_ << " Entry: " << order->entry_dt_
                         << " LastModified: " << order->last_mod_dt_ << " BranchID: " << branch_id_ << " NNF: " << nnf_
                         << DBGLOG_ENDL_FLUSH;

#endif

  //HFSAT::CpucycleProfiler::GetUniqueInstance().Start(2); 
  nse_msgs_handler_->cancel_order_->SetDynamicOrderCancelRequestFields(
      next_message_sequnece_, buy_sell, price, order, inst_desc, secid_to_secinfocashmarket_[order->security_id_],
      is_pre_open_);
  //HFSAT::CpucycleProfiler::GetUniqueInstance().End(2); 

  char send_buffer[1024];
  memset((void*)send_buffer,0,1024); 
  memcpy((void*)send_buffer, (void *)order_cancel_request_buffer_, 22);

  //HFSAT::CpucycleProfiler::GetUniqueInstance().Start(1); 
  open_ssl_crypto_.aes_encrypt((unsigned char*)((char*)order_cancel_request_buffer_+22), cancel_write_length - 22);
  //HFSAT::CpucycleProfiler::GetUniqueInstance().End(1); 

  memcpy((void*)(send_buffer+22), (void*)open_ssl_crypto_.encrypt_text, open_ssl_crypto_.encrypt_len);

  //HFSAT::CpucycleProfiler::GetUniqueInstance().Start(3); 
  int32_t write_length = tcp_direct_client_zocket_.WriteLockFreeN(22+open_ssl_crypto_.encrypt_len, (void*)send_buffer);
  //HFSAT::CpucycleProfiler::GetUniqueInstance().End(3); 

  order->ors_end_cycle_count_ = HFSAT::GetCpucycleCountForTimeTick();
#if NSE_DEBUG_INFO_CM
  std::cout << "Cancel Order : " << order->server_assigned_order_sequence_ << std::endl;
  printHexString((char *)order_cancel_request_buffer_, cancel_write_length);
#endif

  if (write_length < (int32_t)cancel_write_length) {
    DBGLOG_CLASS_FUNC_LINE_FATAL << "FAILD TO WRITE COMPLETE MESSAGE TO EXCHANGE SOCKET: WRITE_LENGTH: " << write_length << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    exit(-1);
  }

}

inline void NSEEngine::OptModifyOrder(HFSAT::ORS::Order *order, int32_t inst_id, ORS::Order *orig_order) {
  
  int16_t buy_sell = (order->buysell_ == kTradeTypeBuy) ? 1 : 2;
  int32_t price = (int32_t)((order->price_ * price_multiplier_) + 0.5);
  next_message_sequnece_++;

#if NSE_DEBUG_INFO
  DBGLOG_CLASS_FUNC_LINE_INFO << "PR MODIFY ORDER REQUEST::\n" << order->toString() << DBGLOG_ENDL_FLUSH;
  DBGLOG_CLASS_FUNC_LINE
      << "BuySell: " << buy_sell << " Price: " << price << " Size: " << order->size_remaining_
      << " Size Disclosed: " << order->size_disclosed_ << " Saos: " << order->server_assigned_order_sequence_
      //                         << " OrderNum: " << (int64_t)order->eaos_.exch_assigned_seq_
      //                         << " LastModified: " << last_modified_date_time << " BranchID: " << branch_id_
      << " NNF: " << nnf_ << DBGLOG_ENDL_FLUSH;
  std::cout << "Sending PRICE MODIFY order::" << pr_modify_order_.ToString() << std::endl;
  std::cout << "sizeof(OrderPriceModifyRequest):" << modify_price_write_length << std::endl;
  printHexString(price_modify_request_buffer_, modify_price_write_length);
#endif
  //-------------------Price Modification-----------------------

  //HFSAT::CpucycleProfiler::GetUniqueInstance().Start(2); 
  pr_modify_order_.SetDynamicOrderEntryRequestFields(next_message_sequnece_, buy_sell, price, order, inst_id);
  //HFSAT::CpucycleProfiler::GetUniqueInstance().End(2); 
  //  HFSAT::CpucycleProfiler::GetUniqueInstance().Start(1);
  
  char send_buffer[1024];
  memset((void*)send_buffer,0,1024); 
  memcpy((void*)send_buffer, (void *)price_modify_request_buffer_, 22);

  //HFSAT::CpucycleProfiler::GetUniqueInstance().Start(1); 
  open_ssl_crypto_.aes_encrypt((unsigned char*)((char*)price_modify_request_buffer_+22), modify_price_write_length - 22);
  //HFSAT::CpucycleProfiler::GetUniqueInstance().End(1); 

  memcpy((void*)(send_buffer+22), (void*)open_ssl_crypto_.encrypt_text, open_ssl_crypto_.encrypt_len);

  //HFSAT::CpucycleProfiler::GetUniqueInstance().Start(3); 
  int32_t write_length = tcp_direct_client_zocket_.WriteLockFreeN(22+open_ssl_crypto_.encrypt_len, (void*)send_buffer);
  //HFSAT::CpucycleProfiler::GetUniqueInstance().End(3); 

  orig_order->ors_end_cycle_count_ = HFSAT::GetCpucycleCountForTimeTick();
  if (write_length < (int32_t)modify_price_write_length) {
    DBGLOG_CLASS_FUNC_LINE_FATAL << "FAILD TO WRITE COMPLETE MESSAGE TO EXCHANGE SOCKET: WRITE_LENGTH: " << write_length << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    exit(-1);
  }

}

void NSEEngine::ModifyOrder(HFSAT::ORS::Order *order, InstrumentDesc *inst_desc, ORS::Order *orig_order) {
// std::cout<<"ModifyOrder"<<std::endl;
//  dbglogger_ << "ModifyOrder\n";

  int16_t buy_sell = (order->buysell_ == kTradeTypeBuy) ? 1 : 2;
  int32_t price = (int32_t)((order->price_ * price_multiplier_) + 0.5);
  next_message_sequnece_++;

#if NSE_DEBUG_INFO_CM
  DBGLOG_CLASS_FUNC_LINE_INFO << "MODIFY ORDER REQUEST::\n" << order->toString() << DBGLOG_ENDL_FLUSH;
  DBGLOG_CLASS_FUNC_LINE << "BuySell: " << buy_sell << " Price: " << price << " Size: " << order->size_remaining_
                         << " Size Disclosed: " << order->size_disclosed_
                         << " Saos: " << order->server_assigned_order_sequence_
                         << " OrderNum: " << (int64_t)order->exch_assigned_seq_ << " Entry: " << order->entry_dt_
                         << " LastModified: " << order->last_mod_dt_ << " BranchID: " << branch_id_ << " NNF: " << nnf_
                         << " sym: " << inst_desc->symbol_ << " eq " << inst_desc->option_type_ << DBGLOG_ENDL_FLUSH;

#endif
  //-------------------Normal Modification ----------------------

  //HFSAT::CpucycleProfiler::GetUniqueInstance().Start(2);
  nse_msgs_handler_->modify_order_->SetDynamicOrderModifyRequestFields(
      next_message_sequnece_, buy_sell, price, order, inst_desc, secid_to_secinfocashmarket_[order->security_id_],
      is_pre_open_);
  //HFSAT::CpucycleProfiler::GetUniqueInstance().End(2);

  //  HFSAT::CpucycleProfiler::GetUniqueInstance().Start(1);


  char send_buffer[1024];
  memset((void*)send_buffer,0,1024); 
  memcpy((void*)send_buffer, (void *)order_modify_request_buffer_, 22);

  //HFSAT::CpucycleProfiler::GetUniqueInstance().Start(1);
  open_ssl_crypto_.aes_encrypt((unsigned char*)((char*)order_modify_request_buffer_+22), modify_write_length - 22);
  //HFSAT::CpucycleProfiler::GetUniqueInstance().End(1);


  memcpy((void*)(send_buffer+22), (void*)open_ssl_crypto_.encrypt_text, open_ssl_crypto_.encrypt_len);
  //HFSAT::CpucycleProfiler::GetUniqueInstance().Start(3);
  int32_t write_length = tcp_direct_client_zocket_.WriteLockFreeN(22+open_ssl_crypto_.encrypt_len, (void*)send_buffer);
  //HFSAT::CpucycleProfiler::GetUniqueInstance().End(3);

  orig_order->ors_end_cycle_count_ = HFSAT::GetCpucycleCountForTimeTick();

#if NSE_DEBUG_INFO_CM
  std::cout << "Cancel Order : " << order->server_assigned_order_sequence_ << std::endl;
  printHexString((char *)order_modify_request_buffer_, modify_write_length);
#endif

  if (write_length < (int32_t)modify_write_length) {
    DBGLOG_CLASS_FUNC_LINE_FATAL << "FAILD TO WRITE COMPLETE MESSAGE TO EXCHANGE SOCKET: WRITE_LENGTH: " << write_length << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    exit(-1);
  }

}

void NSEEngine::ProcessFakeSend(HFSAT::ORS::Order *ord, ORQType_t type) {
  if (!is_logged_in_) {
    return;
  }

  InstrumentDesc *inst_desc = &container_.inst_desc_[ord->security_id_];
  SecurityInfoCashMarket *sec_info = secid_to_secinfocashmarket_[ord->security_id_];
  if (inst_desc->GetToken() == 0) return;
  nse_msgs_handler_->new_order_->SetDynamicOrderEntryRequestFields(0, 0, 0, ord, inst_desc, sec_info, false, false);
}

void NSEEngine::CheckToSendHeartbeat() {

  if (!is_logged_in_ || !is_local_db_updated_) {
    return;
  }

  time_t cptime = time(NULL);
  unsigned int diff = cptime - lastMsgSentAtThisTime();
  if (diff >= heartbeat_interval_ - 1) {
    SendHeartbeatReply();
  }

  if (true == is_pre_open_) {
    is_pre_open_ = ValidatePreOpenMarketTimings();
    DBGLOG_CLASS_FUNC_LINE_INFO << "PRE MARKET SESSION ACTIVE " << DBGLOG_ENDL_FLUSH;
  }

  if (true == is_post_open) {
    is_post_open = ValidatePostMarketTimings();
    DBGLOG_CLASS_FUNC_LINE_INFO << "POST MARKET SESSION ACTIVE " << DBGLOG_ENDL_FLUSH;
  }
  
}

void NSEEngine::SendHeartbeatReply() {
  
  if (!is_logged_in_ || !is_connected_) return;

  next_message_sequnece_++;
  nse_msgs_handler_->heartbeat_request_.SetDynamicHeartbeatFields(next_message_sequnece_);

  DBGLOG_CLASS_FUNC_LINE_INFO << "Sending HeartBeat: " << DBGLOG_ENDL_FLUSH;

  char send_buffer[1024];
  memset((void*)send_buffer,0,1024); 
  memcpy((void*)send_buffer, (void *)nse_msgs_handler_->heartbeat_request_.GetHeartbeatBuffer(), 22);

  //  memcpy((void*)heartbeat_msg_buffer, (void *)nse_msgs_handler_->heartbeat_request_.GetHeartbeatBuffer(), 6);
  open_ssl_crypto_.aes_encrypt((unsigned char*)(nse_msgs_handler_->heartbeat_request_.GetHeartbeatBuffer()+22), NSE_HEARTBEAT_LENGTH - 22);

  memcpy((void*)(send_buffer+22), (void*)open_ssl_crypto_.encrypt_text, open_ssl_crypto_.encrypt_len);
  int32_t write_length = tcp_direct_client_zocket_.WriteLockFreeN(NSE_HEARTBEAT_LENGTH, (void*)send_buffer);
  struct timeval tv;
  gettimeofday(&tv, NULL);
  //#if NSE_DEBUG_INFO
  last_send_time_ = time(NULL);
  if (true == is_pre_open_ || true ==  is_post_open) {
  //    struct timeval tv;
  //    gettimeofday(&tv, NULL);

      DBGLOG_CLASS_FUNC_LINE_INFO << "Sending HeartBeat: " << HFSAT::GetCpucycleCountForTimeTick() << " " << tv.tv_sec
      << " " << id_ << " " << write_length << " @ : " << last_send_time_ << DBGLOG_ENDL_FLUSH;
      //  printHexString(nse_msgs_handler_->heartbeat_request_.GetHeartbeatBuffer(), NSE_HEARTBEAT_LENGTH);
  }

  if (write_length < (int32_t)NSE_HEARTBEAT_LENGTH) {
    DBGLOG_CLASS_FUNC_LINE_FATAL << "FAILD TO WRITE COMPLETE MESSAGE TO EXCHANGE SOCKET: WRITE_LENGTH: " << write_length << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    exit(-1);
  }

//  std::cout << "Sent HBT : " << std::endl;

}

void NSEEngine::ProcessGRResponse(const char *msg_ptr) {
  processed_gr_response_ = nse_msgs_handler_->gr_response_->ProcessGRResponse(msg_ptr);
  DBGLOG_CLASS_FUNC_LINE_INFO << processed_gr_response_->ToString() << DBGLOG_ENDL_FLUSH;

  //#if NSE_DEBUG_INFO
  std::cout << "gr_response string\n";
  printHexString(msg_ptr - (NSE_PACKET_REQUEST_LENGTH + NSE_RESPONSE_MESSAGE_HEADER_LENGTH), NSE_GR_REQUEST_LENGTH);
  //#endif

  trading_port_ = processed_gr_response_->port;
  trading_ip_ = processed_gr_response_->ip;
  memcpy((void *)session_key_, (void *)processed_gr_response_->signon_key, 8);

  memcpy((void *)crypto_key_, (void *)processed_gr_response_->cryptographic_key, 32);
  memcpy((void *)crypto_iv_, (void *)processed_gr_response_->cryptographic_iv, 16);

  //Initialize Crypto
  open_ssl_crypto_.encrypt_EVP_aes_256_gcm_init((unsigned char*)crypto_key_,(unsigned char*)crypto_iv_);
  open_ssl_crypto_.decrypt_EVP_aes_256_gcm_init((unsigned char*)crypto_key_,(unsigned char*)crypto_iv_);

  std::cout << "Going For SecureBoxRegistrationRequest " << std::endl;

  SecureBoxRegistrationRequest();

}

void NSEEngine::ProcessSecureBoxRegistrationResponse(const char *msg_ptr){

  std::cout << "secure_box_registration_response string\n";
  is_secure_box_registered_ = true;
  BoxLoginRequest();

}

void NSEEngine::ProcessBoxLoginResponse(const char *msg_ptr) {
  processed_boxlogin_response_ = nse_msgs_handler_->boxlogin_response_->ProcessBoxLoginResponse(msg_ptr);
  DBGLOG_CLASS_FUNC_LINE_INFO << processed_boxlogin_response_->ToString() << DBGLOG_ENDL_FLUSH;

  //#if NSE_DEBUG_INFO
  std::cout << "boxlogin_response string\n";
  printHexString(msg_ptr - (NSE_PACKET_REQUEST_LENGTH + NSE_RESPONSE_MESSAGE_HEADER_LENGTH),
                 NSE_BoxLogin_REQUEST_LENGTH);
  //#endif

  is_box_logged_in_ = true;
  Login();
  //  WaitForLoginSuccess();
}

void NSEEngine::ProcessLogonResponse(const char *msg_ptr) {
  processed_logon_response_ = nse_msgs_handler_->logon_response_->ProcessLogon(msg_ptr);
  DBGLOG_CLASS_FUNC_LINE_INFO << processed_logon_response_->ToString() << DBGLOG_ENDL_FLUSH;

#if NSE_DEBUG_INFO
  std::cout << "logon_conf string\n";
  printHexString(msg_ptr - (NSE_PACKET_REQUEST_LENGTH + NSE_RESPONSE_MESSAGE_HEADER_LENGTH), NSE_LOGON_REQUEST_LENGTH);
#endif

  is_logged_in_ = true;
  p_engine_listener_->OnLogin(true);
  struct timeval login_time;
  gettimeofday(&login_time, NULL);
  DBGLOG_CLASS_FUNC_LINE_INFO << "LoggedIn @ : " << login_time.tv_sec << DBGLOG_ENDL_FLUSH;
  DBGLOG_DUMP;

  if (new_password != "        ") {
    DBGLOG_CLASS_FUNC_LINE_INFO << "Changing Password to " << new_password << DBGLOG_ENDL_FLUSH;
  
    // Updating to latest value to write back to config file
    setting_.setValue("PasswordChangeDate", HFSAT::DateTime::GetCurrentIsoDateLocalAsString());
    setting_.setValue("Password", new_password);
  
    EmailPasswordChange(std::string(" Changed Password for " + username));
  
    p_engine_listener_->DumpSettingsToFile();
  }

  SimpleSecuritySymbolIndexer::GetUniqueInstance().AddSSSIListener(this);
  // After we are logged in, we need to send the system information IN request
  KillSwitchNewForSecId(-1);
  SendSystemInformationRequest();

}

void NSEEngine::ProcessLogonFailure(const char *msg_ptr, int16_t t_error_code) {
  // Login failed. Parse error msg and log in dbg logger. Will help to debug
  dbglogger_ << "Login Failed. Error Code: " << t_error_code << ". Exiting.\n Login Error Msg: ";
  // Prints the login error msg
  for (int i = 0; i < nse_msgs_handler_->logon_response_->GetLogonErrorMsgLength(); i++) {
    dbglogger_ << msg_ptr[nse_msgs_handler_->logon_response_->GetLogonErrorMsgOffset() + i];
  }
  dbglogger_ << "\n";
  dbglogger_.DumpCurrentBuffer();
  exit(1);
}

void NSEEngine::ProcessSystemInformationResponse(const char *msg_ptr) {
  processed_system_information_response_ = nse_msgs_handler_->system_info_response_->ProcessSystemInfoResponse(msg_ptr);
  DBGLOG_CLASS_FUNC_LINE_INFO << processed_system_information_response_->ToString() << DBGLOG_ENDL_FLUSH;
#if NSE_DEBUG_INFO
  DBGLOG_CLASS_FUNC_LINE_INFO << processed_system_information_response_->ToString() << DBGLOG_ENDL_FLUSH;
#endif

  SendUpdateLocalDBRequest(
      processed_system_information_response_->st_mkt_status, processed_system_information_response_->st_ex_mkt_status,
      processed_system_information_response_->st_pl_mkt_status, processed_system_information_response_->call_auction_1,
      processed_system_information_response_->call_auction_2);
}

void NSEEngine::ProcessUpdateLocalDBResponse(const char *msg_ptr) {
  processed_inner_response_header_ = nse_msgs_handler_->response_inner_header_.ProcessInnerHeader(msg_ptr);

#if NSE_DEBUG_INFO
  DBGLOG_CLASS_FUNC_LINE_INFO << processed_inner_response_header_->ToString() << DBGLOG_ENDL_FLUSH;
#endif

  msg_ptr += NSE_RESPONSE_INNERMESSAGE_HEADER_LENGTH;

  switch (processed_inner_response_header_->transaction_code) {
    case BCAST_SECURITY_MSTR_CHG: {
      ProcessBcastSecurityMessage(msg_ptr);
    } break;

    case BCAST_SECURITY_STATUS_CHG: {
      ProcessBcastSecurityStatusMessage(msg_ptr);
    } break;

    case BCAST_PART_MSTR_CHG: {
      ProcessBcastParticipantMessage(msg_ptr);
    } break;

    case BCAST_INSTR_MSTR_CHG: {
      ProcessBcastInstrumentMessage(msg_ptr);
    } break;

    case BCAST_INDEX_MSTR_CHG: {
      ProcessBcastIndexMessage(msg_ptr);
    } break;

    case BCAST_INDEX_MAP_TABLE: {
      ProcessBcastIndexMapMessage(msg_ptr);
    } break;

    case KILL_SWITCH_RESPONSE:{
          dbglogger_ << "KILL SWITCH RESPONSE RECEIVED\n";
          p_engine_listener_->ForceBatchCancelBroadcastAll();
      }break;

    default: {
      DBGLOG_CLASS_FUNC_LINE_INFO << "Unhandled inner header trans code : "
                                  << processed_inner_response_header_->transaction_code << DBGLOG_ENDL_FLUSH;
    } break;
  }
  msg_ptr -= NSE_RESPONSE_INNERMESSAGE_HEADER_LENGTH;
}

void NSEEngine::ProcessBcastSecurityMessage(const char *msg_ptr) {
// processed_security_update_info_response_ = security_master_update_response_.ProcessSecurityUpdate(msg_ptr);
#if NSE_DEBUG_INFO
  DBGLOG_CLASS_FUNC_LINE_INFO << processed_security_update_info_response_->ToString() << DBGLOG_ENDL_FLUSH;
#endif
}

void NSEEngine::ProcessBcastSecurityStatusMessage(const char *msg_ptr) {
// processed_security_status_update_response_ = security_status_update_response_.GetProcessedSecurityStatus(msg_ptr);
#if NSE_DEBUG_INFO
  DBGLOG_CLASS_FUNC_LINE_INFO << processed_security_update_info_response_->ToString() << DBGLOG_ENDL_FLUSH;
#endif
}

void NSEEngine::ProcessBcastParticipantMessage(const char *msg_ptr) {
// processed_participant_update_info_response_ = participant_update_response_.GetParticipanUpdateInfo(msg_ptr);
#if NSE_DEBUG_INFO
  DBGLOG_CLASS_FUNC_LINE_INFO << processed_participant_update_info_response_->ToString() << DBGLOG_ENDL_FLUSH;
#endif
}

void NSEEngine::ProcessBcastIndexMessage(const char *msg_ptr) {
// processed_index_update_info_response_ = index_update_response_.ProcessIndexInfo(msg_ptr);
#if NSE_DEBUG_INFO
  DBGLOG_CLASS_FUNC_LINE_INFO << processed_index_update_info_response_->ToString() << DBGLOG_ENDL_FLUSH;
#endif
}

void NSEEngine::ProcessBcastIndexMapMessage(const char *msg_ptr) {
// processed_index_map_update_info_response_ = index_map_update_rersponse_.ProcessIndexMapDetails(msg_ptr);
#if NSE_DEBUG_INFO
  DBGLOG_CLASS_FUNC_LINE_INFO << processed_index_map_update_info_response_->ToString() << DBGLOG_ENDL_FLUSH;
#endif
}

void NSEEngine::ProcessBcastInstrumentMessage(const char *msg_ptr) {
// processed_instrument_update_info_response_ = instrument_update_info_response_.ProcessInstrumentUpdate(msg_ptr);
#if NSE_DEBUG_INFO
  DBGLOG_CLASS_FUNC_LINE_INFO << processed_instrument_update_info_response_->ToString() << DBGLOG_ENDL_FLUSH;
#endif
}

inline void NSEEngine::ProcessHeartbeatResponse(const char *msg_ptr) {}

void NSEEngine::ProcessLogoutResponse(const char *msg_ptr) {
  DBGLOG_CLASS_FUNC_LINE_INFO << "Logout Received" << DBGLOG_ENDL_FLUSH;
  is_logged_in_ = false;
  p_engine_listener_->OnLogout();
  tcp_direct_client_zocket_.Close();
}

void NSEEngine::ProcessOrderConfirmation(const char *msg_ptr) {
  msg_ptr -= NSE_RESPONSE_MESSAGE_HEADER_LENGTH;
  processed_order_response_ = nse_msgs_handler_->order_response_->ProcessOrderResponse(msg_ptr);

#if NSE_DEBUG_INFO_CM
  DBGLOG_CLASS_FUNC_LINE_INFO << processed_order_response_->ToString() << DBGLOG_ENDL_FLUSH;
#endif

  int32_t saos = processed_order_response_->saos;
  double price = (double)(processed_order_response_->price) / price_multiplier_;
  int32_t size = processed_order_response_->size;
  int64_t exch_order_num = processed_order_response_->order_number;
  int32_t entry_dt = processed_order_response_->entry_date_time;
  int64_t last_activity_ref = processed_order_response_->last_activity_reference;
  p_engine_listener_->OnOrderConf(saos, "", price, size, 0, exch_order_num, entry_dt, last_activity_ref);

  exch_order_num_to_saos_[exch_order_num] = saos;
  //  DBGLOG_CLASS_FUNC_LINE_INFO << " EXCH ORD : " << exch_order_num << " SAOS " << saos << DBGLOG_ENDL_FLUSH ;
  //  DBGLOG_DUMP ;

  msg_ptr += NSE_RESPONSE_MESSAGE_HEADER_LENGTH;
}

void NSEEngine::ProcessSpreadOrderConfirmation(const char *msg_ptr) {
  msg_ptr -= NSE_RESPONSE_MESSAGE_HEADER_LENGTH;
  std::cout <<"ProcessSpreadOrderConfirmation" << std::endl;
  return;
  processed_spread_order_response = nse_msgs_handler_->order_response_three_leg->ProcessOrderResponse(msg_ptr);

#if NSE_DEBUG_INFO_CM
  DBGLOG_CLASS_FUNC_LINE_INFO << processed_spread_order_response->ToString() << DBGLOG_ENDL_FLUSH;
#endif

  int32_t saos = processed_spread_order_response->saos;
  double price = (double)(processed_spread_order_response->price) / price_multiplier_;
  int32_t size = processed_spread_order_response->size;
  int64_t exch_order_num = processed_spread_order_response->order_number;
  int32_t entry_dt = processed_spread_order_response->entry_date_time;
  int64_t last_activity_ref = 0; // NEed to check processed_spread_order_response->last_activity_reference;
  p_engine_listener_->OnOrderConf(saos, "", price, size, 0, exch_order_num, entry_dt, last_activity_ref);

  exch_order_num_to_saos_[exch_order_num] = saos;
  //  DBGLOG_CLASS_FUNC_LINE_INFO << " EXCH ORD : " << exch_order_num << " SAOS " << saos << DBGLOG_ENDL_FLUSH ;
  //  DBGLOG_DUMP ;

  msg_ptr += NSE_RESPONSE_MESSAGE_HEADER_LENGTH;
}

void NSEEngine::ProcessThreeLegOrderConfirmation(const char *msg_ptr) {
 // msg_ptr -= NSE_RESPONSE_MESSAGE_HEADER_LENGTH;
  processed_spread_order_response = nse_msgs_handler_->order_response_three_leg->ProcessOrderResponse(msg_ptr);

#if NSE_DEBUG_INFO_CM
  DBGLOG_CLASS_FUNC_LINE_INFO << processed_spread_order_response->ToString() << DBGLOG_ENDL_FLUSH;
#endif

  int32_t saos = processed_spread_order_response->saos;
  double price_1 = (double)(processed_spread_order_response->price) / price_multiplier_;
  double price_2 = (double)(processed_spread_order_response->price_2) / price_multiplier_;
  double price_3 = (double)(processed_spread_order_response->price_3) / price_multiplier_;
  int32_t size_1 = processed_spread_order_response->size;
  int32_t size_2 = processed_spread_order_response->size_2;
  int32_t size_3 = processed_spread_order_response->size_3;
  int64_t exch_order_num = processed_spread_order_response->order_number;
  int32_t entry_dt = processed_spread_order_response->entry_date_time;
  int64_t last_activity_ref = 0; // Need to check processed_spread_order_response->last_activity_reference;
  p_engine_listener_->OnOrderConf(saos, "", price_1, size_1, 0, exch_order_num, entry_dt, last_activity_ref);
  exch_order_num_to_saos_[exch_order_num] = saos;

  p_engine_listener_->OnOrderConf(saos + 1, "", price_2, size_2, 0, exch_order_num, entry_dt, last_activity_ref);
  exch_order_num = processed_spread_order_response->order_number + processed_spread_order_response->token_2 * EXP_NUMBER;
  exch_order_num_to_saos_[exch_order_num] = saos + 1;

  p_engine_listener_->OnOrderConf(saos + 2, "", price_3, size_3, 0, exch_order_num, entry_dt, last_activity_ref);
  exch_order_num = processed_spread_order_response->order_number + processed_spread_order_response->token_3 * EXP_NUMBER;
  exch_order_num_to_saos_[exch_order_num] = saos + 2;


  //  DBGLOG_CLASS_FUNC_LINE_INFO << " EXCH ORD : " << exch_order_num << " SAOS " << saos << DBGLOG_ENDL_FLUSH ;
  //  DBGLOG_DUMP ;

 // msg_ptr += NSE_RESPONSE_MESSAGE_HEADER_LENGTH;
}

void NSEEngine::ProcessOrderCancellation(const char *msg_ptr) {
  msg_ptr -= NSE_RESPONSE_MESSAGE_HEADER_LENGTH;
  processed_order_response_ = nse_msgs_handler_->order_response_->ProcessOrderResponse(msg_ptr);

#if NSE_DEBUG_INFO_CM
  DBGLOG_CLASS_FUNC_LINE_INFO << processed_order_response_->ToString() << DBGLOG_ENDL_FLUSH;
#endif

  //  DBGLOG_CLASS_FUNC_LINE_INFO << "ON ORDER CXL : " << processed_order_response_->saos << " "
  //  <<processed_order_response_->order_number << DBGLOG_ENDL_FLUSH ;
  //  DBGLOG_DUMP ;
  p_engine_listener_->OnOrderCxl(processed_order_response_->saos, processed_order_response_->order_number);

  auto iter = exch_order_num_to_saos_.find(processed_order_response_->order_number);
  if (iter != exch_order_num_to_saos_.end()) {
    exch_order_num_to_saos_.erase(iter);
  } else {
    DBGLOG_CLASS_FUNC_LINE_ERROR << "ExchangeOrderNumber : " << processed_order_response_->order_number
                                 << " SAOS: " << processed_order_response_->saos
                                 << " not found in exch_order_num_to_saos_ map" << DBGLOG_ENDL_FLUSH;
  }

  msg_ptr += NSE_RESPONSE_MESSAGE_HEADER_LENGTH;
}

void NSEEngine::ProcessSpreadOrderCancellation(const char *msg_ptr) {
  msg_ptr -= NSE_RESPONSE_MESSAGE_HEADER_LENGTH;
  return ;
  processed_spread_order_response = nse_msgs_handler_->order_response_three_leg->ProcessOrderResponse(msg_ptr);

#if NSE_DEBUG_INFO_CM
  DBGLOG_CLASS_FUNC_LINE_INFO << processed_spread_order_response->ToString() << DBGLOG_ENDL_FLUSH;
#endif

  //  DBGLOG_CLASS_FUNC_LINE_INFO << "ON ORDER CXL : " << processed_spread_order_response->saos << " "
  //  <<processed_spread_order_response->order_number << DBGLOG_ENDL_FLUSH ;
  //  DBGLOG_DUMP ;
  p_engine_listener_->OnOrderCxl(processed_spread_order_response->saos, processed_spread_order_response->order_number);

  auto iter = exch_order_num_to_saos_.find(processed_spread_order_response->order_number);
  if (iter != exch_order_num_to_saos_.end()) {
    exch_order_num_to_saos_.erase(iter);
  } else {
    DBGLOG_CLASS_FUNC_LINE_ERROR << "ExchangeOrderNumber : " << processed_spread_order_response->order_number
                                 << " SAOS: " << processed_spread_order_response->saos
                                 << " not found in exch_order_num_to_saos_ map" << DBGLOG_ENDL_FLUSH;
  }


  msg_ptr += NSE_RESPONSE_MESSAGE_HEADER_LENGTH;
}

void NSEEngine::ProcessThreeLegOrderCancellation(const char *msg_ptr) {
 // msg_ptr -= NSE_RESPONSE_MESSAGE_HEADER_LENGTH;
  processed_spread_order_response = nse_msgs_handler_->order_response_three_leg->ProcessOrderResponse(msg_ptr);

#if NSE_DEBUG_INFO_CM
  DBGLOG_CLASS_FUNC_LINE_INFO << processed_spread_order_response->ToString() << DBGLOG_ENDL_FLUSH;
#endif

  //  DBGLOG_CLASS_FUNC_LINE_INFO << "ON ORDER CXL : " << processed_spread_order_response->saos << " "
  //  <<processed_spread_order_response->order_number << DBGLOG_ENDL_FLUSH ;
  //  DBGLOG_DUMP ;
  p_engine_listener_->OnOrderCxl(processed_spread_order_response->saos, processed_spread_order_response->order_number);
  p_engine_listener_->OnOrderCxl(processed_spread_order_response->saos + 1, processed_spread_order_response->order_number + processed_spread_order_response->token_2 * EXP_NUMBER); // Check if Order can have partial execution..
  p_engine_listener_->OnOrderCxl(processed_spread_order_response->saos + 2, processed_spread_order_response->order_number + processed_spread_order_response->token_3 * EXP_NUMBER);  // TODO CHECK if problem can come with same order Number

  auto iter = exch_order_num_to_saos_.find(processed_spread_order_response->order_number);
  if (iter != exch_order_num_to_saos_.end()) {
    exch_order_num_to_saos_.erase(iter);
  } else {
    DBGLOG_CLASS_FUNC_LINE_ERROR << "ExchangeOrderNumber : " << processed_spread_order_response->order_number
                                 << " SAOS: " << processed_spread_order_response->saos
                                 << " not found in exch_order_num_to_saos_ map" << DBGLOG_ENDL_FLUSH;
  }
   iter = exch_order_num_to_saos_.find(processed_spread_order_response->order_number + processed_spread_order_response->token_2 * EXP_NUMBER);
  if (iter != exch_order_num_to_saos_.end()) {
    exch_order_num_to_saos_.erase(iter);
  } else {
    DBGLOG_CLASS_FUNC_LINE_ERROR << "ExchangeOrderNumber3Leg 2nd : " << processed_spread_order_response->order_number
                                 << " SAOS: " << processed_spread_order_response->saos
                                 << " not found in exch_order_num_to_saos_ map" << DBGLOG_ENDL_FLUSH;
  }
   iter = exch_order_num_to_saos_.find(processed_spread_order_response->order_number + processed_spread_order_response->token_3 * EXP_NUMBER);
  if (iter != exch_order_num_to_saos_.end()) {
    exch_order_num_to_saos_.erase(iter);
  } else {
    DBGLOG_CLASS_FUNC_LINE_ERROR << "ExchangeOrderNumber3Leg 3rd : " << processed_spread_order_response->order_number
                                 << " SAOS: " << processed_spread_order_response->saos
                                 << " not found in exch_order_num_to_saos_ map" << DBGLOG_ENDL_FLUSH;
  }

//  msg_ptr += NSE_RESPONSE_MESSAGE_HEADER_LENGTH;
}

void NSEEngine::ProcessOrderModification(const char *msg_ptr) {
  msg_ptr -= NSE_RESPONSE_MESSAGE_HEADER_LENGTH;
  processed_order_response_ = nse_msgs_handler_->order_response_->ProcessOrderResponse(msg_ptr);

#if NSE_DEBUG_INFO_CM
  DBGLOG_CLASS_FUNC_LINE_INFO << processed_order_response_->ToString() << DBGLOG_ENDL_FLUSH;
#endif

  double price = (double)(processed_order_response_->price) / price_multiplier_;
  p_engine_listener_->OnOrderCancelReplaced(processed_order_response_->saos, processed_order_response_->order_number,
                                            price, processed_order_response_->size, fast_px_convertor_vec_,
                                            processed_order_response_->last_modified_date_time, processed_order_response_->last_activity_reference);

  msg_ptr += NSE_RESPONSE_MESSAGE_HEADER_LENGTH;
}

void NSEEngine::ProcessOrderCxlRejection(const char *msg_ptr) {
  msg_ptr -= NSE_RESPONSE_MESSAGE_HEADER_LENGTH;
  processed_order_response_ = nse_msgs_handler_->order_response_->ProcessOrderResponse(msg_ptr);
  int32_t saos = processed_order_response_->saos;
  p_engine_listener_->OnCxlReject(saos, kExchCancelReject);
  msg_ptr += NSE_RESPONSE_MESSAGE_HEADER_LENGTH;
}

void NSEEngine::ProcessOrderModRejection(const char *msg_ptr) {
  msg_ptr -= NSE_RESPONSE_MESSAGE_HEADER_LENGTH;
  processed_order_response_ = nse_msgs_handler_->order_response_->ProcessOrderResponse(msg_ptr);
  int32_t saos = processed_order_response_->saos;
  p_engine_listener_->OnOrderModReject(saos);
  msg_ptr += NSE_RESPONSE_MESSAGE_HEADER_LENGTH;
}

void NSEEngine::ProcessOrderRejection(const char *msg_ptr) {
  msg_ptr -= NSE_RESPONSE_MESSAGE_HEADER_LENGTH;
  processed_order_response_ = nse_msgs_handler_->order_response_->ProcessOrderResponse(msg_ptr);
  //  DBGLOG_CLASS_FUNC_LINE_INFO << processed_order_response_->ToString() << DBGLOG_ENDL_FLUSH;
  LogExchangeRejects(processed_order_response_->error_code);
  int32_t saos = processed_order_response_->saos;
  int16_t error_code = processed_order_response_->error_code;
  DBGLOG_CLASS_FUNC_LINE_INFO << "Received Order Reject_tr : Reason : " << error_code << DBGLOG_ENDL_FLUSH;
  if (error_code == 16418 && nse_segment_type_ == NSE_EQ_SEGMENT_MARKING) { // CM Order_entered_has_invalid_data
    // send Freeze to Strat
    dbglogger_ << "Exchange reject 16418 Sending Freeze To Strat For Saos:" << saos << "\n";
    p_engine_listener_->OnReject(HFSAT::kExchDataEntryOrderReject);
  } else {
    p_engine_listener_->OnReject(saos);
  }
  msg_ptr += NSE_RESPONSE_MESSAGE_HEADER_LENGTH;
}

void NSEEngine::ProcessTradeConfirmation(const char *msg_ptr) {
  msg_ptr -= NSE_RESPONSE_MESSAGE_HEADER_LENGTH;
  processed_trade_response_ = nse_msgs_handler_->trade_conf_response_->ProcessTradeConfirmationResponse(msg_ptr);

#if NSE_DEBUG_INFO_CM
  DBGLOG_CLASS_FUNC_LINE_INFO << processed_trade_response_->ToString() << DBGLOG_ENDL_FLUSH;
#endif

  int32_t saos = -1;
  int32_t token = processed_trade_response_->token;
  const char *symbol;
  char exch_sym[15];  // 10 char long symbol + "NSE_"

  // For cash market, get the symbol  as "NSE_"processed_trade_response_->symbol
  if (!strcmp(processed_trade_response_->series, "EQ")) {
    strcpy(exch_sym, "NSE_");
    strcat(exch_sym, trimwhitespace(processed_trade_response_->symbol));
    symbol = exch_sym;
  } else {
    // Check For Error Case
    if (token_to_internal_exchange_symbol_.find(token) == token_to_internal_exchange_symbol_.end()) {
      std::string datasource_symbol =
          nse_daily_token_symbol_handler_.GetInternalSymbolFromToken(token, nse_segment_type_);
      token_to_internal_exchange_symbol_[token] = nse_sec_def_.ConvertDataSourceNametoExchSymbol(datasource_symbol);
    }

    symbol = token_to_internal_exchange_symbol_[token].c_str();
  }
#if NSE_DEBUG_INFO
  DBGLOG_CLASS_FUNC_LINE_INFO << "Exec for Internal Symbol : " << symbol << DBGLOG_ENDL_FLUSH;
#endif

  TradeType_t buy_sell = (processed_trade_response_->buy_sell == 1) ? kTradeTypeBuy : kTradeTypeSell;
  double price = (double)((double)processed_trade_response_->price) / price_multiplier_;
  int32_t size_executed = processed_trade_response_->size_executed;
  int32_t size_remaining = processed_trade_response_->size_remaining;

  int64_t exch_order_num = processed_trade_response_->order_number;
  auto iter = exch_order_num_to_saos_.find(exch_order_num);
  if (iter != exch_order_num_to_saos_.end()) {
    saos = exch_order_num_to_saos_[exch_order_num];

    p_engine_listener_->OnOrderExec(saos, symbol, buy_sell, price, size_executed, size_remaining, exch_order_num,
                                    processed_trade_response_->activity_time, processed_trade_response_->last_activity_reference);

    // SimpleSecuritySymbolIndexer &indexer = SimpleSecuritySymbolIndexer::GetUniqueInstance();

    // Removing from map as order should no longer exist
    if (size_remaining == 0) {
      //      DBGLOG_CLASS_FUNC_LINE_ERROR << " SIZE REM 0 : " << DBGLOG_ENDL_FLUSH ;
      //      DBGLOG_DUMP ;
      exch_order_num_to_saos_.erase(iter);
    }

    //    if (strike_price <= 0) {
    //      nse_config_based_checks_.OnOrderExecutionUpdateTurnOverValue(price, size_executed);
    //    } else {
    //      nse_config_based_checks_.OnOrderExecutionUpdateTurnOverValue(strike_price, size_executed);
    //    }

  } /*  else {
        auto iter_2 = exch_order_num_to_saos_.find(exch_order_num + token * EXP_NUMBER);
    if (iter_2 != exch_order_num_to_saos_.end()) {
      saos = exch_order_num_to_saos_[exch_order_num + token * EXP_NUMBER];
      p_engine_listener_->OnOrderExec(saos, symbol, buy_sell, price, size_executed, size_remaining, exch_order_num + token * EXP_NUMBER,
                                      processed_trade_response_->activity_time, processed_trade_response_->last_activity_reference);
      exch_order_num_to_saos_.erase(iter_2); // can remove as its For 3leg and have IOC

    } */
    else {
      DBGLOG_CLASS_FUNC_LINE_ERROR << "Trade Received for order not in Map: ExchOrderID: " << exch_order_num
                                 << DBGLOG_ENDL_FLUSH;
    }
//  }

  //  DBGLOG_CLASS_FUNC_LINE_INFO << " SIZE REMAINING: " << size_remaining << " " << saos << DBGLOG_ENDL_FLUSH ;
  //  DBGLOG_DUMP ;

  msg_ptr += NSE_RESPONSE_MESSAGE_HEADER_LENGTH;
}

void NSEEngine::ProcessOrderRejectionNonTR(const char *msg_ptr, int16_t error_code) {
  processed_order_response_non_tr_ = nse_msgs_handler_->order_response_non_tr_.ProcessOrderResponse(msg_ptr);
  //  DBGLOG_CLASS_FUNC_LINE_INFO << processed_order_response_non_tr_->ToString() << DBGLOG_ENDL_FLUSH;
  int32_t saos = processed_order_response_non_tr_->saos;
  if (error_code == 16419 && nse_segment_type_ == NSE_FO_SEGMENT_MARKING) { // FO This error code will be returned for invalid data in the order packet.
    // send Freeze to Strat
    dbglogger_ << "Exchange reject 16419 Sending Freeze To Strat For Saos:" << saos << "\n";
  }
  p_engine_listener_->OnReject(saos);
}

void NSEEngine::ProcessSpreadOrderRejectionNonTR(const char *msg_ptr, int16_t error_code) {
  processed_spread_order_response = nse_msgs_handler_->order_response_three_leg->ProcessOrderResponse(msg_ptr);
  //  DBGLOG_CLASS_FUNC_LINE_INFO << processed_order_response_non_tr_->ToString() << DBGLOG_ENDL_FLUSH;
  int32_t saos = processed_spread_order_response->saos;
  p_engine_listener_->OnReject(saos);
}

void NSEEngine::ProcessThreeLegOrderRejectionNonTR(const char *msg_ptr, int16_t error_code) {
  processed_spread_order_response = nse_msgs_handler_->order_response_three_leg->ProcessOrderResponse(msg_ptr);
  //  DBGLOG_CLASS_FUNC_LINE_INFO << processed_order_response_non_tr_->ToString() << DBGLOG_ENDL_FLUSH;

  int32_t saos = processed_spread_order_response->saos;
  p_engine_listener_->OnReject(saos);
  saos++;
  p_engine_listener_->OnReject(saos);
  saos++;
  p_engine_listener_->OnReject(saos);
}

void NSEEngine::ProcessOrderCxlRejectionNonTR(const char *msg_ptr, int16_t error_code) {
  processed_order_response_non_tr_ = nse_msgs_handler_->order_response_non_tr_.ProcessOrderResponse(msg_ptr);
  int32_t saos = processed_order_response_non_tr_->saos;
  p_engine_listener_->OnCxlReject(saos, kExchCancelReject);
}

void NSEEngine::ProcessOrderModRejectionNonTR(const char *msg_ptr, int16_t error_code) {
  processed_order_response_non_tr_ = nse_msgs_handler_->order_response_non_tr_.ProcessOrderResponse(msg_ptr);
  int32_t saos = processed_order_response_non_tr_->saos;
  p_engine_listener_->OnOrderModReject(saos);
}

void NSEEngine::ProcessControlMessage(const char *msg_ptr) {
  int32_t trader_id = ntoh32(*((int32_t *)(msg_ptr)));
  char action_code[4];
  memset(action_code, 0, sizeof(action_code));
  memcpy(action_code, msg_ptr + 4, sizeof(action_code) - 1);

  int16_t bcast_len = ntoh16(*((int16_t *)(msg_ptr + 8)));

  char bcast_msg[240];
  memset(bcast_msg, 0, sizeof(bcast_msg));
  memcpy(bcast_msg, msg_ptr + 10, bcast_len);
  std::cout << "ControlMessage: TraderID: " << trader_id << " ActionCode: " << action_code << " BcastLen: " << bcast_len
            << " BcastMsg: " << bcast_msg << "\n";
  dbglogger_ << "ControlMessage: TraderID: " << trader_id << " ActionCode: " << action_code << " BcastLen: " << bcast_len
             << " BcastMsg: " << bcast_msg << "\n";
  dbglogger_.DumpCurrentBuffer();
  if ( strcmp(action_code,"MAR")== 0){
      if (strstr(bcast_msg, "MARGIN") != NULL ){
        try {
              PerishableStringTokenizer st_(bcast_msg, bcast_len);
              const std::vector<const char*>& tokens_ = st_.GetTokens();
	      for (unsigned int i= 0; i<tokens_.size(); i++){
		      std::cout<<tokens_[i]<<" ";
		}
		std::cout<<std::endl;
              if ((tokens_.size() >= 12) && (strcmp(tokens_[8], "Amount:") == 0) && (strcmp(tokens_[10], "Limit:") == 0)) {
                double marginamount = atof(tokens_[9]);
                double marginlimit = atof(tokens_[11]);
                double marginpercent= (marginamount/marginlimit)*100;
                p_engine_listener_->SetMarginValue(marginpercent);
              }
          }catch (...) {
              dbglogger_ << "Exception in ProcessGeneralControlCommand(). But not Exiting."
                   << "\n";
              dbglogger_.DumpCurrentBuffer();
          }
        if (strstr(bcast_msg, "80.0 %%") != NULL || strstr(bcast_msg, "85.0 %%") != NULL){
          EmailCtrlAlert(bcast_msg,trader_id);
             dbglogger_  << "Asking Strat Not Send Order Go Flat" << "\n";
	// Need to Handle Manually            broadcast_strat_cmds.sendStratCmd(HFSAT::kControlMessageCodeGetflat); // Send Flat To strat
        } else if (strstr(bcast_msg, "90.0 %%") != NULL || strstr(bcast_msg, "95.0 %%") != NULL || strstr(bcast_msg, "100.0 %%") != NULL ){
           // dbglogger_ << "Cancelling All Order at Strat " << "\n";
           dbglogger_ << "ForceBatchCancelBroadcastAll " << "\n";
           p_engine_listener_->ForceBatchCancelBroadcastAll(true); // Do we need it ? 
           EmailCtrlAlert(bcast_msg,trader_id);
           EnableDisableOnlyIOC(true); // Only IOC
            dbglogger_ << "Cancelled All Order at Strat " << "\n";
	// Need to Handle Manually            broadcast_strat_cmds.sendStratCmd(HFSAT::kControlMessageCodeGetflat); // Send Flat To strat
        }
      }
  }
}

void NSEEngine::EnableDisableOnlyIOC(bool is_risk_reduction_set_){
	is_risk_reduction_set = is_risk_reduction_set_;
	if ( is_risk_reduction_set == true) {
		dbglogger_ << "INFO: Risk Reduction Only IOC: Enabled User: " << user_id_<< "\n";
    char str_msg[128] = "INFO: Risk Reduction Only IOC: Enabled User: ";
		EmailGeneric(str_msg); 
	} else {
		dbglogger_ << "INFO: Risk Reduction Only IOC: Disbaled User: " << user_id_ << "\n";
    char str_msg[128] = "INFO: Risk Reduction Only IOC: Disbaled User: ";
		EmailGeneric(str_msg);
	}

}

void NSEEngine::LogExchangeRejects(int16_t error_code){

  auto error_found = error_code_reason_.find(error_code);
  dbglogger_ << "Checking for Error code " << error_code << "\n";
  
  if(error_code_reason_.end() != error_found){
      dbglogger_ << "Exchange Error :  "  << error_code << "   " << error_found->second << "\n";
  }
  else{
      dbglogger_ << "EXCHANGE REJECT ERROR CODE NOT FOUND CHECK NNF \n"; 
  }

  dbglogger_.DumpCurrentBuffer();

  return;
}

/*
void NSEEngine::LoadKillSwitchBuffer(){

  memcpy((char*)kill_switch_msg_buffer,(void*)order_entry_request_buffer_ ,send_write_length);
  *((int16_t *)(kill_switch_msg_buffer + 0 + (sizeof(int16_t) + sizeof(int32_t) + 16))) =
        hton16(2062);
  //memcpy(kill_switch_msg_buffer+(((0 + (sizeof(int16_t) + sizeof(int32_t) + 16)) + sizeof(int16_t)) + sizeof(int32_t)),' ',12);
}
*/

/*
void NSEEngine::KillSwitch(){


  dbglogger_ << "SENDING KILL SWITCH COMMAND FOR USER " << user_id_ << "\n";

  HFSAT::MD5::MD5((unsigned char *)(kill_switch_msg_buffer + NSE_REQUEST_MESSAGE_HEADER_TRANSACTION_CODE_OFFSET),
                    NSE_CM_ORDERENTRY_REQUEST_LENGTH - NSE_PACKET_REQUEST_LENGTH,
                    (unsigned int *)(kill_switch_msg_buffer + NSE_PACKET_CHECKSUM_OFFSET));

  int32_t write_length = tcp_direct_client_zocket_.WriteLockFreeN(send_write_length,kill_switch_msg_buffer);

  if (write_length < (int32_t)send_write_length) {
    DBGLOG_CLASS_FUNC_LINE_FATAL << "FAILED TO WRITE COMPLETE MESSAGE TO EXCHANGE SOCKET: WRITE_LENGTH: " << write_length << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    exit(-1);
  }
  return;
}
*/
uint32_t NSEEngine::ProcessEncryptedExchangeResponse(char *nse_msg_buffer, const uint32_t &msg_length) {
  uint32_t length_to_be_processed = msg_length;
  const char *msg_ptr = nse_msg_buffer;

#if NSE_DEBUG_INFO
  std::cout << "Received Message\n";
  printHexString(nse_msg_buffer, msg_length);
  std::cout.flush();

#endif

  // process multiple messages till the length to be processed is >0, reduce as we process the messages
  while (length_to_be_processed > 0) {

    if (length_to_be_processed < NSE_PACKET_RESPONSE_LENGTH) {
      DBGLOG_CLASS_FUNC_LINE_ERROR << "INCOMPLETE HEADER, Requires : " << NSE_PACKET_RESPONSE_LENGTH
                                   << " Messsage Length Left : " << length_to_be_processed
                                   << " Initial Length Given : " << msg_length << DBGLOG_ENDL_FLUSH;
      memmove((void *)nse_msg_buffer, (void *)msg_ptr, length_to_be_processed);

      return length_to_be_processed;
    }

    processed_packet_header_ = nse_msgs_handler_->packet_response_.ProcessPakcet(msg_ptr);

#if NSE_DEBUG_INFO_CM
    DBGLOG_CLASS_FUNC_LINE_INFO << processed_packet_header_->ToString();
#endif

    if (length_to_be_processed < ((uint32_t)processed_packet_header_->packet_length)) {
      memmove((void *)nse_msg_buffer, (void *)msg_ptr, length_to_be_processed);
      return length_to_be_processed;
    }
    const char* packet_start = msg_ptr;
    msg_ptr += NSE_PACKET_RESPONSE_LENGTH;

    const char *decrypted_msg_ptr = nullptr;

    //decrypt all msg except secure box registration
    if(true == is_secure_box_registered_){
      open_ssl_crypto_.aes_decrypt((unsigned char*)msg_ptr, processed_packet_header_->packet_length - NSE_PACKET_RESPONSE_LENGTH);
      decrypted_msg_ptr = (char*)open_ssl_crypto_.decrypt_data;
    }else{
      decrypted_msg_ptr = msg_ptr;
    }    
    tcp_direct_client_zocket_.LogAuditIn(packet_start,NSE_PACKET_RESPONSE_LENGTH,decrypted_msg_ptr, processed_packet_header_->packet_length-NSE_PACKET_RESPONSE_LENGTH);
    

    // Incorrect Parsing here. Not every response msg has header info. The parsed values may contain junk.
    // The transaction code is correct coz offset = 0 in all structures known for now. The error code may be incorrect.
    processed_response_header_ = nse_msgs_handler_->response_header_.ProcessHeader(decrypted_msg_ptr);

//    DBGLOG_CLASS_FUNC_LINE_INFO << processed_response_header_->ToString();

    msg_ptr += NSE_RESPONSE_MESSAGE_HEADER_LENGTH;
    decrypted_msg_ptr += NSE_RESPONSE_MESSAGE_HEADER_LENGTH;

    int16_t error_code = processed_response_header_->error_code;
#if NSE_DEBUG_INFO_CM
    DBGLOG_CLASS_FUNC_LINE_INFO << "  TC:" << processed_response_header_->transaction_code << "ERROR COD:  " << error_code << DBGLOG_ENDL_FLUSH;
#endif

    //    DBGLOG_CLASS_FUNC_LINE_INFO << " TXN CODE : " << processed_response_header_->transaction_code <<
    //    DBGLOG_ENDL_FLUSH ;
    //    DBGLOG_DUMP ;

    // Test the message type against the pre-computed network order
    switch (processed_response_header_->transaction_code) {
      case SIGN_ON_REQUEST_OUT: {
        if (error_code == 0) {
          ProcessLogonResponse(decrypted_msg_ptr);
        } else
          ProcessLogonFailure(decrypted_msg_ptr, processed_response_header_->error_code);

      } break;

      case GR_REQUEST_OUT: {
        DBGLOG_CLASS_FUNC_LINE_INFO << "ERROR CODE : " << error_code
                                    << " TC : " << processed_response_header_->transaction_code << DBGLOG_ENDL_FLUSH;
        ProcessGRResponse(decrypted_msg_ptr);
      } break;

      case SECURE_BOX_REGISTRATION_RESPONSE: {
        DBGLOG_CLASS_FUNC_LINE_INFO << "ERROR CODE : " << error_code
                                    << " TC : " << processed_response_header_->transaction_code << DBGLOG_ENDL_FLUSH;
        ProcessSecureBoxRegistrationResponse(decrypted_msg_ptr);
      } break;

      case BoxLogin_REQUEST_OUT: {
        DBGLOG_CLASS_FUNC_LINE_INFO << "ERROR CODE : " << error_code
                                    << " TC : " << processed_response_header_->transaction_code << DBGLOG_ENDL_FLUSH;
        ProcessBoxLoginResponse(decrypted_msg_ptr);
      } break;

      case SYSTEM_INFORMATION_OUT: {
        ProcessSystemInformationResponse(decrypted_msg_ptr);
      } break;

      case NSE_HEARTBEAT: {
        ProcessHeartbeatResponse(decrypted_msg_ptr);
      } break;

      case SIGN_OFF_REQUEST_OUT: {
        ProcessLogoutResponse(decrypted_msg_ptr);
      } break;

      case UPDATE_LOCALDB_HEADER: {
        DBGLOG_CLASS_FUNC_LINE_INFO << "Received LocalDB Header \n" << DBGLOG_ENDL_FLUSH;
      } break;

      case UPDATE_LOCALDB_DATA: {
        // Not processing LocalDB data template. 1. irrelevant 2. in FO, doesn't follow the structure defined in docs:
        // Abhishek
        DBGLOG_CLASS_FUNC_LINE_INFO << "Received LocalDB Data \n" << DBGLOG_ENDL_FLUSH;
      } break;

      case ORDER_CONFIRMATION: {
        //        ProcessOrderConfirmation(msg_ptr);
      } break;

      case ORDER_CONFIRMATION_TR: {
        ProcessOrderConfirmation(decrypted_msg_ptr);
      } break;

      case ORDER_MOD_CONFIRMATION_TR: {
        ProcessOrderModification(decrypted_msg_ptr);
      } break;

      case ORDER_CXL_CONFIRMATION_TR: {
        //        DBGLOG_CLASS_FUNC_LINE_INFO << " ORDER CANCELLATION : " << DBGLOG_ENDL_FLUSH ;
        //        DBGLOG_DUMP ;
        ProcessOrderCancellation(decrypted_msg_ptr);
      } break;

      case ORDER_CANCEL_REJECT: {
        ProcessOrderCxlRejectionNonTR(decrypted_msg_ptr, error_code);
      } break;

      case ORDER_CANCEL_REJECT_TR: {
        ProcessOrderCxlRejection(decrypted_msg_ptr);
      } break;

      case ORDER_MOD_REJECT_TR: {
        ProcessOrderModRejection(decrypted_msg_ptr);
      } break;

      case ORDER_MOD_REJECT: {
        ProcessOrderModRejectionNonTR(decrypted_msg_ptr, error_code);
      } break;

      case ORDER_ERROR_TR: {
        //DBGLOG_CLASS_FUNC_LINE_INFO << "Inside ORDER_ERROR_TR " << DBGLOG_ENDL_FLUSH; 
        DBGLOG_CLASS_FUNC_LINE_INFO << "Received Order Reject : Reason : " << processed_response_header_->error_code
                                    << DBGLOG_ENDL_FLUSH; // Req by Rejects_Alerts_FO.py script for reporting
        ProcessOrderRejection(decrypted_msg_ptr);
      } break;

      case ORDER_ERROR: {
        DBGLOG_CLASS_FUNC_LINE_INFO << "Received Order Reject : Reason : " << processed_response_header_->error_code
                                    << DBGLOG_ENDL_FLUSH;
        ProcessOrderRejectionNonTR(decrypted_msg_ptr, error_code); // FO
      } break;

      case TRADE_CONFIRMATION_TR: {
        //        DBGLOG_CLASS_FUNC_LINE_INFO << " TRADE CONF: " << DBGLOG_ENDL_FLUSH ;
        //        DBGLOG_DUMP ;
        ProcessTradeConfirmation(decrypted_msg_ptr);
      } break;

      case UPDATE_LOCALDB_TRAILER: {
        is_local_db_updated_ = true;
        DBGLOG_CLASS_FUNC_LINE_INFO << "Received LocalDB Trailer \n" << DBGLOG_ENDL_FLUSH;
      } break;

      case CTRL_MSG_TO_TRADER: {
        ProcessControlMessage(decrypted_msg_ptr);
      } break;

      case TAP_INVITATION_MESSAGE: {
        //        nse_tap_invitation_.UpdateInvitationValue(true);
      } break;
      case BATCH_ORDER_CANCELLATION_TR: {
        p_engine_listener_->OnBatchCxlAlert(user_id_); // Send Alert For All Batch Cancellaton 
        DBGLOG_CLASS_FUNC_LINE_ERROR << "Received Batch ORDER CANCELLATION : "
                                      << processed_response_header_->transaction_code<< "\n" << DBGLOG_ENDL_FLUSH;     
        if ( processed_response_header_->error_code == 16795 && nse_segment_type_ == NSE_FO_SEGMENT_MARKING){ // Order_cancelled_due_to_voluntary_close_out FOR FO
          dbglogger_ << "Cancelling All Order at Strat " << "\n";
          dbglogger_ << "ForceBatchCancelBroadcastAll " << "\n";
          p_engine_listener_->ForceBatchCancelBroadcastAll();
	  EnableDisableOnlyIOC(true);
          dbglogger_ << "Cancelled All Order at Strat " << "\n";
        } else if ( processed_response_header_->error_code == 17017 && nse_segment_type_ == NSE_EQ_SEGMENT_MARKING){ // Order_cancelled_due_to_voluntary_close_out FOR CM
          dbglogger_ << "Cancelling All Order at Strat " << "\n";
          dbglogger_ << "ForceBatchCancelBroadcastAll " << "\n";
          p_engine_listener_->ForceBatchCancelBroadcastAll();
	  EnableDisableOnlyIOC(true);
          dbglogger_ << "Cancelled All Order at Strat " << "\n";
	}
        } break;
      case BCAST_SEC_MSTR_CHNG_PERIODIC: {
        ProcessBcastSecurityMessage(decrypted_msg_ptr);
        DBGLOG_CLASS_FUNC_LINE_INFO << "Received Security Master Periodic Update " << DBGLOG_ENDL_FLUSH;
	} break;
      case THRL_ORDER_CONFIRMATION :{
          ProcessThreeLegOrderConfirmation(decrypted_msg_ptr);
      } break;
      case THRL_ORDER_CXL_CONFIRMATION :{
          ProcessThreeLegOrderCancellation(decrypted_msg_ptr);
      } break;
      case THRL_ORDER_ERROR :{
          DBGLOG_CLASS_FUNC_LINE_INFO << "Received THRL_ORDER_ERROR reject : Reason : " << error_code
                                    << DBGLOG_ENDL_FLUSH;
          ProcessThreeLegOrderRejectionNonTR(decrypted_msg_ptr, error_code);
      } break;
      case ORDER_CANCEL_CONFIRMATION:{
        DBGLOG_CLASS_FUNC_LINE_INFO << "ORDER_CANCEL_CONFIRMATION" << DBGLOG_ENDL_FLUSH;
      }break;
      case SP_ORDER_CONFIRMATION:{
        DBGLOG_CLASS_FUNC_LINE_INFO << "SP_ORDER_CONFIRMATION.. " << DBGLOG_ENDL_FLUSH;
        ProcessSpreadOrderConfirmation(decrypted_msg_ptr);
      } break;

      case SP_ORDER_ERROR :{
        DBGLOG_CLASS_FUNC_LINE_INFO << "Received Spread Order Reject : Reason : " << processed_response_header_->error_code
                                    << DBGLOG_ENDL_FLUSH;
        ProcessSpreadOrderRejectionNonTR(decrypted_msg_ptr, error_code);
      } break;
      case SP_ORDER_CXL_CONFIRMATION:{
        DBGLOG_CLASS_FUNC_LINE_INFO << "SP_ORDER_CXL_CONFIRMATION" << DBGLOG_ENDL_FLUSH;
        ProcessSpreadOrderCancellation(decrypted_msg_ptr);
      } break;

      case GIVEUP_APP_CONFIRM_TM:{
          //         Process Given Up message 
        } break;
      case KILL_SWITCH_RESPONSE:{
          dbglogger_ << "KILL SWITCH RESPONSE RECEIVED\n";
      }break;
      default: {
        DBGLOG_CLASS_FUNC_LINE_ERROR << "NO HANDLING YET FOR THIS MESSAGE TYPE : "
                                     << processed_response_header_->transaction_code << DBGLOG_ENDL_FLUSH;
        std::cout << "Unhandled Message: " << processed_response_header_->transaction_code << "\n";
        printHexString(nse_msg_buffer, msg_length);
        std::cout.flush();

      } break;
    }

    msg_ptr +=
        (processed_packet_header_->packet_length - NSE_PACKET_RESPONSE_LENGTH - NSE_RESPONSE_MESSAGE_HEADER_LENGTH);
    length_to_be_processed -= (processed_packet_header_->packet_length);
  }

  return 0;  // Message Was Processed Completely
}


uint32_t NSEEngine::ProcessExchangeResponse(char *nse_msg_buffer, const uint32_t &msg_length) {
  uint32_t length_to_be_processed = msg_length;
  const char *msg_ptr = nse_msg_buffer;

#if NSE_DEBUG_INFO
  std::cout << "Received Message\n";
  printHexString(nse_msg_buffer, msg_length);
  std::cout.flush();

#endif

  // process multiple messages till the length to be processed is >0, reduce as we process the messages
  while (length_to_be_processed > 0) {
    if (length_to_be_processed < NSE_PACKET_RESPONSE_LENGTH) {
      DBGLOG_CLASS_FUNC_LINE_ERROR << "INCOMPLETE HEADER, Requires : " << NSE_PACKET_RESPONSE_LENGTH
                                   << " Messsage Length Left : " << length_to_be_processed
                                   << " Initial Length Given : " << msg_length << DBGLOG_ENDL_FLUSH;

      memmove((void *)nse_msg_buffer, (void *)msg_ptr, length_to_be_processed);

      return length_to_be_processed;
    }

    processed_packet_header_ = nse_msgs_handler_->packet_response_.ProcessPakcet(msg_ptr);

#if NSE_DEBUG_INFO_CM
    DBGLOG_CLASS_FUNC_LINE_INFO << processed_packet_header_->ToString();
#endif

    if (length_to_be_processed < ((uint32_t)processed_packet_header_->packet_length)) {
      memmove((void *)nse_msg_buffer, (void *)msg_ptr, length_to_be_processed);
      return length_to_be_processed;
    }

    msg_ptr += NSE_PACKET_RESPONSE_LENGTH;

    // Incorrect Parsing here. Not every response msg has header info. The parsed values may contain junk.
    // The transaction code is correct coz offset = 0 in all structures known for now. The error code may be incorrect.
    processed_response_header_ = nse_msgs_handler_->response_header_.ProcessHeader(msg_ptr);

#if NSE_DEBUG_INFO_CM
    DBGLOG_CLASS_FUNC_LINE_INFO << processed_response_header_->ToString();
#endif

    msg_ptr += NSE_RESPONSE_MESSAGE_HEADER_LENGTH;

    int16_t error_code = processed_response_header_->error_code;
#if NSE_DEBUG_INFO_CM
    DBGLOG_CLASS_FUNC_LINE_INFO << "  TC:" << processed_response_header_->transaction_code << DBGLOG_ENDL_FLUSH;
#endif

    //    DBGLOG_CLASS_FUNC_LINE_INFO << " TXN CODE : " << processed_response_header_->transaction_code <<
    //    DBGLOG_ENDL_FLUSH ;
    //    DBGLOG_DUMP ;

    // Test the message type against the pre-computed network order
    switch (processed_response_header_->transaction_code) {
      case SIGN_ON_REQUEST_OUT: {
        if (error_code == 0) {
          ProcessLogonResponse(msg_ptr);
        } else
          ProcessLogonFailure(msg_ptr, processed_response_header_->error_code);

      } break;

      case GR_REQUEST_OUT: {
        DBGLOG_CLASS_FUNC_LINE_INFO << "ERROR CODE : " << error_code
                                    << " TC : " << processed_response_header_->transaction_code << DBGLOG_ENDL_FLUSH;
        ProcessGRResponse(msg_ptr);
      } break;

      case BoxLogin_REQUEST_OUT: {
        DBGLOG_CLASS_FUNC_LINE_INFO << "ERROR CODE : " << error_code
                                    << " TC : " << processed_response_header_->transaction_code << DBGLOG_ENDL_FLUSH;
        ProcessBoxLoginResponse(msg_ptr);
      } break;

      case SYSTEM_INFORMATION_OUT: {
        ProcessSystemInformationResponse(msg_ptr);
      } break;

      case NSE_HEARTBEAT: {
        ProcessHeartbeatResponse(msg_ptr);
      } break;

      case SIGN_OFF_REQUEST_OUT: {
        ProcessLogoutResponse(msg_ptr);
      } break;

      case UPDATE_LOCALDB_HEADER: {
        DBGLOG_CLASS_FUNC_LINE_INFO << "Received LocalDB Header \n" << DBGLOG_ENDL_FLUSH;
      } break;

      case UPDATE_LOCALDB_DATA: {
        // Not processing LocalDB data template. 1. irrelevant 2. in FO, doesn't follow the structure defined in docs:
        // Abhishek
        DBGLOG_CLASS_FUNC_LINE_INFO << "Received LocalDB Data \n" << DBGLOG_ENDL_FLUSH;
      } break;

      case ORDER_CONFIRMATION: {
        //        ProcessOrderConfirmation(msg_ptr);
      } break;

      case ORDER_CONFIRMATION_TR: {
        ProcessOrderConfirmation(msg_ptr);
      } break;

      case ORDER_MOD_CONFIRMATION_TR: {
        ProcessOrderModification(msg_ptr);
      } break;

      case ORDER_CXL_CONFIRMATION_TR: {
        //        DBGLOG_CLASS_FUNC_LINE_INFO << " ORDER CANCELLATION : " << DBGLOG_ENDL_FLUSH ;
        //        DBGLOG_DUMP ;
        ProcessOrderCancellation(msg_ptr);
      } break;

      case ORDER_CANCEL_REJECT: {
        ProcessOrderCxlRejectionNonTR(msg_ptr, error_code);
      } break;

      case ORDER_CANCEL_REJECT_TR: {
        ProcessOrderCxlRejection(msg_ptr);
      } break;

      case ORDER_MOD_REJECT_TR: {
        ProcessOrderModRejection(msg_ptr);
      } break;

      case ORDER_MOD_REJECT: {
        ProcessOrderModRejectionNonTR(msg_ptr, error_code);
      } break;

      case ORDER_ERROR_TR: {
        //DBGLOG_CLASS_FUNC_LINE_INFO << "Inside ORDER_ERROR_TR " << DBGLOG_ENDL_FLUSH; 
        DBGLOG_CLASS_FUNC_LINE_INFO << "Received Order Reject : Reason : " << processed_response_header_->error_code
                                    << DBGLOG_ENDL_FLUSH; // Req by Rejects_Alerts_FO.py script for reporting
        ProcessOrderRejection(msg_ptr);
      } break;

      case ORDER_ERROR: {
        DBGLOG_CLASS_FUNC_LINE_INFO << "Received Order Reject : Reason : " << processed_response_header_->error_code
                                    << DBGLOG_ENDL_FLUSH;
        ProcessOrderRejectionNonTR(msg_ptr, error_code); // FO
      } break;

      case TRADE_CONFIRMATION_TR: {
        //        DBGLOG_CLASS_FUNC_LINE_INFO << " TRADE CONF: " << DBGLOG_ENDL_FLUSH ;
        //        DBGLOG_DUMP ;
        ProcessTradeConfirmation(msg_ptr);
      } break;

      case UPDATE_LOCALDB_TRAILER: {
        is_local_db_updated_ = true;
        DBGLOG_CLASS_FUNC_LINE_INFO << "Received LocalDB Trailer \n" << DBGLOG_ENDL_FLUSH;
      } break;

      case CTRL_MSG_TO_TRADER: {
        ProcessControlMessage(msg_ptr);
      } break;

      case TAP_INVITATION_MESSAGE: {
        //        nse_tap_invitation_.UpdateInvitationValue(true);
      } break;
      case BATCH_ORDER_CANCELLATION_TR: {
        p_engine_listener_->OnBatchCxlAlert(user_id_); // Send Alert For All Batch Cancellaton 
        DBGLOG_CLASS_FUNC_LINE_ERROR << "Received Batch ORDER CANCELLATION : "
                                      << processed_response_header_->transaction_code<< "\n" << DBGLOG_ENDL_FLUSH;     
        if ( processed_response_header_->error_code == 16795 && nse_segment_type_ == NSE_FO_SEGMENT_MARKING){ // Order_cancelled_due_to_voluntary_close_out FOR FO
          dbglogger_ << "Cancelling All Order at Strat " << "\n";
          dbglogger_ << "ForceBatchCancelBroadcastAll " << "\n";
          p_engine_listener_->ForceBatchCancelBroadcastAll();
	  EnableDisableOnlyIOC(true);
          dbglogger_ << "Cancelled All Order at Strat " << "\n";
        } else if ( processed_response_header_->error_code == 17017 && nse_segment_type_ == NSE_EQ_SEGMENT_MARKING){ // Order_cancelled_due_to_voluntary_close_out FOR CM
          dbglogger_ << "Cancelling All Order at Strat " << "\n";
          dbglogger_ << "ForceBatchCancelBroadcastAll " << "\n";
          p_engine_listener_->ForceBatchCancelBroadcastAll();
	  EnableDisableOnlyIOC(true);
          dbglogger_ << "Cancelled All Order at Strat " << "\n";
	}
        } break;
      case BCAST_SEC_MSTR_CHNG_PERIODIC: {
        ProcessBcastSecurityMessage(msg_ptr);
        DBGLOG_CLASS_FUNC_LINE_INFO << "Received Security Master Periodic Update " << DBGLOG_ENDL_FLUSH;
	} break;
      case THRL_ORDER_CONFIRMATION :{
          ProcessThreeLegOrderConfirmation(msg_ptr);
      } break;
      case THRL_ORDER_CXL_CONFIRMATION :{
          ProcessThreeLegOrderCancellation(msg_ptr);
      } break;
      case THRL_ORDER_ERROR :{
          DBGLOG_CLASS_FUNC_LINE_INFO << "Received THRL_ORDER_ERROR reject : Reason : " << error_code
                                    << DBGLOG_ENDL_FLUSH;
          ProcessThreeLegOrderRejectionNonTR(msg_ptr, error_code);
      } break;
      case ORDER_CANCEL_CONFIRMATION:{
        DBGLOG_CLASS_FUNC_LINE_INFO << "ORDER_CANCEL_CONFIRMATION" << DBGLOG_ENDL_FLUSH;
      }break;
      case SP_ORDER_CONFIRMATION:{
        DBGLOG_CLASS_FUNC_LINE_INFO << "SP_ORDER_CONFIRMATION.. " << DBGLOG_ENDL_FLUSH;
        ProcessSpreadOrderConfirmation(msg_ptr);
      } break;

      case SP_ORDER_ERROR :{
        DBGLOG_CLASS_FUNC_LINE_INFO << "Received Spread Order Reject : Reason : " << processed_response_header_->error_code
                                    << DBGLOG_ENDL_FLUSH;
        ProcessSpreadOrderRejectionNonTR(msg_ptr, error_code);
      } break;
      case SP_ORDER_CXL_CONFIRMATION:{
        DBGLOG_CLASS_FUNC_LINE_INFO << "SP_ORDER_CXL_CONFIRMATION" << DBGLOG_ENDL_FLUSH;
        ProcessSpreadOrderCancellation(msg_ptr);
      } break;

     case GIVEUP_APP_CONFIRM_TM:{
          //         Process Given Up message 
        } break;
      case KILL_SWITCH_RESPONSE:{
          dbglogger_ << "KILL SWITCH RESPONSE RECEIVED\n";
      }break;
      default: {
        DBGLOG_CLASS_FUNC_LINE_ERROR << "NO HANDLING YET FOR THIS MESSAGE TYPE : "
                                     << processed_response_header_->transaction_code << DBGLOG_ENDL_FLUSH;
        std::cout << "Unhandled Message: " << processed_response_header_->transaction_code << "\n";
        printHexString(nse_msg_buffer, msg_length);
        std::cout.flush();

      } break;
    }

    msg_ptr +=
        (processed_packet_header_->packet_length - NSE_PACKET_RESPONSE_LENGTH - NSE_RESPONSE_MESSAGE_HEADER_LENGTH);
    length_to_be_processed -= (processed_packet_header_->packet_length);
  }

  return 0;  // Message Was Processed Completely
}

inline void NSEEngine::thread_main() {}

void EmailForDisconnectWithoutLogout() {
  // also send an alert
  char hostname[128];
  hostname[127] = '\0';
  gethostname(hostname, 127);
  HFSAT::Email e;

  e.setSubject("Error: Disconnected from exchange without sending logout : " + std::string(hostname));
  e.addRecepient("ravi.parikh@tworoads.co.in, nseall@tworoads.co.in");
  e.addSender("ravi.parikh@tworoads.co.in");
  e.content_stream << "host_machine: " << hostname << "<br/>";

  e.sendMail();
}

void NSEEngine::EmailControlMsgToTrader(std::string alert_msg_) {
  // also send an alert
  char hostname[128];
  hostname[127] = '\0';
  gethostname(hostname, 127);

  std::string mode_str_ = "NSE";

  HFSAT::Email e;

  struct timeval current_time;
  gettimeofday(&current_time, NULL);

  e.setSubject(mode_str_ + " -- CONTROL MESSAGE TO TRADER");
  e.addRecepient("ravi.parikh@tworoads.co.in, nseall@tworoads.co.in");
  e.addSender("ravi.parikh@tworoads.co.in");
  e.content_stream << "exch: " << mode_str_ << "<br/>";
  e.content_stream << "host_machine: " << hostname << "<br/>";
  e.content_stream << "Control Trader Message  : " << alert_msg_ << " <br/>";
  e.content_stream << "Current Time: " << ctime(&(current_time.tv_sec)) << " <br/>";
  e.sendMail();
}

inline void NSEEngine::ProcessLockFreeTCPDirectRead() {

  if(!is_read_socket_open_) return;

  InstrumentDesc *inst_desc = &container_.inst_desc_[0];
  SecurityInfoCashMarket *sec_info = secid_to_secinfocashmarket_[0];
  if (inst_desc->GetToken() == 0) return;
  nse_msgs_handler_->new_order_->SetDynamicOrderEntryRequestFields(0, 0, 0, &dummy_order_, inst_desc, sec_info, false,
                                                                   false);
  tcp_direct_client_zocket_.WriteLockFreeNWarmOnly(send_write_length,
                                                   (void *)nse_msgs_handler_->new_order_->GetOrderEntryRequestBuffer());

  tcp_direct_read_buffer_ =
      tcp_direct_client_zocket_.ReadLockFreeN(tcp_direct_read_length_, was_tcp_direct_read_successful_);

  if (true == was_tcp_direct_read_successful_) {
    onInputAvailable(0, tcp_direct_read_buffer_, tcp_direct_read_length_);
  }

}

inline int32_t NSEEngine::onInputAvailable(int32_t _socket_, char *buffer, int32_t length) {
  if (keep_engine_running_) {
    int32_t read_length = length;
    //        nse_session_socket_->ReadN(MAX_NSE_RESPONSE_BUFFER_SIZE - read_offset_, nse_msg_buffer_ + read_offset_);

    memcpy((char *)nse_msg_buffer_ + read_offset_, buffer, length);

    //#if NSE_DEBUG_INFO
    //    DBGLOG_CLASS_FUNC_LINE_INFO << "onInpAvail: " << read_length << " " << _socket_ << " "
    //                                << nse_session_socket_->socket_file_descriptor() << DBGLOG_ENDL_FLUSH;
    //    DBGLOG_CLASS_FUNC_LINE_INFO << "SOCKET READ RETURNED : " << read_length << DBGLOG_ENDL_FLUSH;
    //#endif

    if (read_length > 0) {
      read_offset_ = ProcessEncryptedExchangeResponse(nse_msg_buffer_, read_offset_ + read_length);
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

void NSEEngine::OnAddString(uint32_t num_sid) {
  SimpleSecuritySymbolIndexer &indexer = SimpleSecuritySymbolIndexer::GetUniqueInstance();
  std::string exchange_symbol = indexer.GetSecuritySymbolFromId(num_sid - 1);

  // Get data symbol. For cash market, data symbol is shortcode itself
  std::string data_symbol = nse_sec_def_.ConvertExchSymboltoDataSourceName(exchange_symbol.c_str());
  if (std::string("INVALID") == data_symbol) {
    DBGLOG_CLASS_FUNC_LINE_FATAL << "FAILED TO ADD SYMBOL STRING AS DATASYMBOL WAS INVALID, SYMBOL : "
                                 << exchange_symbol << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;

    // Should never have reached here
    //exit(-1);
    return;
  }
  DBGLOG_CLASS_FUNC_LINE_INFO << "SecID: " << num_sid << " ExchSym: " << exchange_symbol << DBGLOG_ENDL_FLUSH;

  if (nse_sec_def_.IsSpreadExchSymbol(exchange_symbol)) {  // spread symbol added

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
        nse_daily_token_symbol_handler_.GetTokenFromInternalSymbol(data_symbol_1.c_str(), nse_segment_type_);
    int32_t token_2 =
        nse_daily_token_symbol_handler_.GetTokenFromInternalSymbol(data_symbol_2.c_str(), nse_segment_type_);

    std::cout << "RefDataLeg1: " << ref_data_loader.GetNSERefData(nse_segment_type_)[token_1].ToString() << "\n";
    std::cout << "RefDataLeg2: " << ref_data_loader.GetNSERefData(nse_segment_type_)[token_2].ToString() << "\n";

    char *instrument_1 = ref_data_loader.GetNSERefData(nse_segment_type_)[token_1].instrument;
    char *instrument_2 = ref_data_loader.GetNSERefData(nse_segment_type_)[token_2].instrument;

    char symbol_1[11];
    memset(symbol_1, ' ', sizeof(symbol_1));
    memcpy(symbol_1, ref_data_loader.GetNSERefData(nse_segment_type_)[token_1].symbol,
           strlen(ref_data_loader.GetNSERefData(nse_segment_type_)[token_1].symbol));
    symbol_1[10] = 0;

    char symbol_2[11];
    memset(symbol_2, ' ', sizeof(symbol_2));
    memcpy(symbol_2, ref_data_loader.GetNSERefData(nse_segment_type_)[token_2].symbol,
           strlen(ref_data_loader.GetNSERefData(nse_segment_type_)[token_2].symbol));
    symbol_2[10] = 0;

    char *option_type_1 = ref_data_loader.GetNSERefData(nse_segment_type_)[token_1].option_type;
    char *option_type_2 = ref_data_loader.GetNSERefData(nse_segment_type_)[token_2].option_type;

    int32_t strike_price_1 =
        (int32_t)(ref_data_loader.GetNSERefData(nse_segment_type_)[token_1].strike_price * price_multiplier_);
    int32_t strike_price_2 =
        (int32_t)(ref_data_loader.GetNSERefData(nse_segment_type_)[token_2].strike_price * price_multiplier_);

    int32_t expiry_1 = ref_data_loader.GetNSERefData(nse_segment_type_)[token_1].expiry;
    int32_t expiry_2 = ref_data_loader.GetNSERefData(nse_segment_type_)[token_2].expiry;

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
    /*container_.spread_inst_desc_[num_sid - 1]->SetSpreadInstrumentDesc(token_1, instrument_1, symbol_1, expiry_1,
       strike_price_1, option_type_1, token_2,
            instrument_2, symbol_2, expiry_2, strike_price_2, option_type_2);*/
    // DBGLOG_CLASS_FUNC_LINE_FATAL << container_.spread_inst_desc_[num_sid - 1]->ToString() << DBGLOG_ENDL_FLUSH;
    container_.spread_inst_desc_[num_sid - 1].SetSpreadInstrumentDesc(
        token_1, instrument_1, symbol_1, expiry_1, strike_price_1, option_type_1, token_2, instrument_2, symbol_2,
        expiry_2, strike_price_2, option_type_2);
#endif
  } else {
    int32_t token = nse_daily_token_symbol_handler_.GetTokenFromInternalSymbol(data_symbol.c_str(), nse_segment_type_);
    char *instrument = ref_data_loader.GetNSERefData(nse_segment_type_)[token].instrument;

    char symbol[11];
    memset(symbol, ' ', sizeof(symbol));
    memcpy(symbol, ref_data_loader.GetNSERefData(nse_segment_type_)[token].symbol,
           strlen(ref_data_loader.GetNSERefData(nse_segment_type_)[token].symbol));
    symbol[10] = 0;

    char *option_type = ref_data_loader.GetNSERefData(nse_segment_type_)[token].option_type;
    int32_t strike_price =
        (int32_t)(std::round(ref_data_loader.GetNSERefData(nse_segment_type_)[token].strike_price * price_multiplier_));
    int32_t expiry = ref_data_loader.GetNSERefData(nse_segment_type_)[token].expiry;

    if (strcmp(option_type, "XX") == 0) {
      strike_price = -1;
      std::cout << "strike changed to -1 \n";
    }

    // For Cash Market, we require symbol and series info only as part of instrument descriptor.
    // We are using option_type field as series info (2 bytes each).
    if (HFSAT::NSESecurityDefinitions::IsEquity(data_symbol)) {  // data symbol is shc for cash segment
      strcpy(option_type, "EQ");
      std::string eq_symbol = exchange_symbol.substr(4);
      strncpy(symbol, eq_symbol.c_str(), eq_symbol.length());  // do not append '/0'
    }
    std::cout << "Token: " << token << " ExchSym: " << exchange_symbol
              << " RefData: " << ref_data_loader.GetNSERefData(nse_segment_type_)[token].ToString()
              << " Symbol: " << symbol << " Instrument: " << instrument << " Expiry: " << expiry
              << " Strike: " << strike_price << " Option_type: " << option_type << "\n"
              << std::endl;

    container_.inst_desc_[num_sid - 1].SetInstrumentDesc(token, instrument, symbol, expiry, strike_price, option_type);
    secid_to_secinfocashmarket_[num_sid - 1]->SetSecurityInfo(symbol, option_type);
    // Add to FastPxConverter map
    HFSAT::SecurityDefinitions &security_definitions_ =
        HFSAT::SecurityDefinitions::GetUniqueInstance(HFSAT::DateTime::GetCurrentIsoDateLocal());
    HFSAT::ShortcodeContractSpecificationMap &this_contract_specification_map_ =
        security_definitions_.contract_specification_map_;
    HFSAT::ExchangeSymbolManager::SetUniqueInstance(HFSAT::DateTime::GetCurrentIsoDateLocal());

    for (auto itr_ = this_contract_specification_map_.begin(); itr_ != this_contract_specification_map_.end(); itr_++) {
      std::string shortcode_ = (itr_->first);
      if (itr_->second.exch_source_ != HFSAT::kExchSourceNSE) {
        continue;
      }
      std::string this_symbol = nse_sec_def_.GetExchSymbolNSE(shortcode_);
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

void NSEEngine::OnControlHubDisconnectionShutdown() {
  DBGLOG_CLASS_FUNC_LINE_FATAL << "RECEIVED SHUTDOWN REQUEST AS CONTROL HUB COULDN'T BE REACHED...."
                               << DBGLOG_ENDL_FLUSH;
  DBGLOG_CLASS_FUNC_LINE_ERROR << "INITIATING BLOCKING OF FURTHER ORDERS THROUGH ORS...." << DBGLOG_ENDL_FLUSH;

  allow_new_orders_ = false;
  sleep(1);

  DBGLOG_CLASS_FUNC_LINE_ERROR << "CANCELLING EXISTING ORDERS.... " << DBGLOG_ENDL_FLUSH;
  p_engine_listener_->CancelAllPendingOrders();

  DBGLOG_CLASS_FUNC_LINE_ERROR << "SENDING LOGOUT..., INFORMING LISTENERS..." << DBGLOG_ENDL_FLUSH;
  Logout();
  is_logged_in_ = false;
  p_engine_listener_->OnLogout();

  DBGLOG_CLASS_FUNC_LINE_ERROR << "CLEANING UP SOCKETS... " << DBGLOG_ENDL_FLUSH;
  DisConnect();
  is_connected_ = false;

  DBGLOG_CLASS_FUNC_LINE_ERROR << "DONE WITH CLEAN UP... BRINGING SYSTEM DOWN... " << DBGLOG_ENDL_FLUSH;
  exit(-1);
}

bool IsSpread(int sec_id) {
  SimpleSecuritySymbolIndexer &indexer = SimpleSecuritySymbolIndexer::GetUniqueInstance();
  std::string exchange_symbol = indexer.GetSecuritySymbolFromId(sec_id);
  if (exchange_symbol.find("-") != std::string::npos) {
    return true;
  }
  return false;
}

void NSEEngine::SendOrder(ORS::Order *order) {
#if NSE_DEBUG_INFO
  DBGLOG_CLASS_FUNC_LINE_INFO << "SendOrder: SecID: " << order->security_id_ << DBGLOG_ENDL_FLUSH;
#endif

  SendOrder(order, &container_.inst_desc_[order->security_id_]);
}

void NSEEngine::SendSpreadOrder(HFSAT::ORS::Order *order1_, HFSAT::ORS::Order *order2_) {
  
    std::cout << "Sending Spread Order " << std::endl;
    int16_t order1_buy_sell = (order1_->buysell_ == kTradeTypeBuy) ? 1 : 2;
    int16_t order2_buy_sell = (order2_->buysell_ == kTradeTypeBuy) ? 1 : 2;
    InstrumentDesc *inst_desc_1 = &container_.inst_desc_[order1_->security_id_];
    InstrumentDesc *inst_desc_2 = &container_.inst_desc_[order2_->security_id_];
    int32_t order1_price = (int32_t)((order1_->price_ * price_multiplier_) + 0.5);
    int32_t order2_price = (int32_t)((order2_->price_ * price_multiplier_) + 0.5);
    next_message_sequnece_++;

    nse_msgs_handler_->spread_new_order_->SetDynamicOrderEntryRequestFields(
      next_message_sequnece_, order1_buy_sell, order2_buy_sell, order1_price, order2_price,
      order1_, order2_, inst_desc_1, inst_desc_2, secid_to_secinfocashmarket_[order1_->security_id_],secid_to_secinfocashmarket_[order2_->security_id_], is_mkt_order_, is_pre_open_);

    std::cout << "Going for SendSpreadOrder Encryption Using NOrmal Socket : "<< std::endl;

    char send_buffer[1024];
    memset((void*)send_buffer,0,1024); 
    memcpy((void*)send_buffer, (void *)order_entry_spread_request_buffer_, 22);
    std::cout << "Calling Encrypt Function For Length : " << send_spread_write_length - 22 << std::endl;

    open_ssl_crypto_.aes_encrypt((unsigned char*)((char*)order_entry_spread_request_buffer_+22), send_spread_write_length - 22);
    std::cout << "Encrypted Length : " << open_ssl_crypto_.encrypt_len << std::endl;

    memcpy((void*)(send_buffer+22), (void*)open_ssl_crypto_.encrypt_text, open_ssl_crypto_.encrypt_len);
    int32_t write_length = tcp_direct_client_zocket_.WriteLockFreeN(22+open_ssl_crypto_.encrypt_len, (void*)send_buffer);

    std::cout << "Write length here for SendSystemInformationRequest : "<< write_length << std::endl;

    order1_->ors_end_cycle_count_ = HFSAT::GetCpucycleCountForTimeTick();
    order2_->ors_end_cycle_count_ = HFSAT::GetCpucycleCountForTimeTick();

    if (write_length < (int32_t)send_spread_write_length) {
    DBGLOG_CLASS_FUNC_LINE_FATAL << "FAILD TO WRITE COMPLETE MESSAGE TO EXCHANGE SOCKET: WRITE_LENGTH: " << write_length << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    exit(-1);
  }
}

void NSEEngine::SendThreeLegOrder(HFSAT::ORS::Order *order1_, HFSAT::ORS::Order *order2_, HFSAT::ORS::Order *order3_) {
  
    int16_t order1_buy_sell = (order1_->buysell_ == kTradeTypeBuy) ? 1 : 2;
    int16_t order2_buy_sell = (order2_->buysell_ == kTradeTypeBuy) ? 1 : 2;
    int16_t order3_buy_sell = (order3_->buysell_ == kTradeTypeBuy) ? 1 : 2;
    InstrumentDesc *inst_desc_1 = &container_.inst_desc_[order1_->security_id_];
    InstrumentDesc *inst_desc_2 = &container_.inst_desc_[order2_->security_id_];
    InstrumentDesc *inst_desc_3 = &container_.inst_desc_[order3_->security_id_];
    int32_t order1_price = (int32_t)((order1_->price_ * price_multiplier_) + 0.5);
    int32_t order2_price = (int32_t)((order2_->price_ * price_multiplier_) + 0.5);
    int32_t order3_price = (int32_t)((order3_->price_ * price_multiplier_) + 0.5);
    next_message_sequnece_++;

    nse_msgs_handler_->new_three_leg_order->SetDynamicOrderEntryRequestFields(
      next_message_sequnece_, order1_buy_sell, order2_buy_sell, order3_buy_sell, order1_price, order2_price, order3_price,
      order1_, order2_, order3_, inst_desc_1, inst_desc_2, inst_desc_3, secid_to_secinfocashmarket_[order1_->security_id_],secid_to_secinfocashmarket_[order2_->security_id_],
      secid_to_secinfocashmarket_[order3_->security_id_], is_mkt_order_, is_pre_open_);

    std::cout << "Going for SendThreeLegOrder Encryption Using NOrmal Socket : "<< std::endl;

    char send_buffer[1024];
    memset((void*)send_buffer,0,1024); 
    memcpy((void*)send_buffer, (void *)order_entry_three_leg_request_buffer_, 22);
    std::cout << "Calling Encrypt Function For Length : " << send_three_leg_write_length - 22 << std::endl;

    open_ssl_crypto_.aes_encrypt((unsigned char*)((char*)order_entry_three_leg_request_buffer_+22), send_three_leg_write_length - 22);
    std::cout << "Encrypted Length : " << open_ssl_crypto_.encrypt_len << std::endl;

    memcpy((void*)(send_buffer+22), (void*)open_ssl_crypto_.encrypt_text, open_ssl_crypto_.encrypt_len);
    int32_t write_length = tcp_direct_client_zocket_.WriteLockFreeN(22+open_ssl_crypto_.encrypt_len, (void*)send_buffer);

    std::cout << "Write length here for SendThreeLegOrder : "<< write_length << std::endl;

    order1_->ors_end_cycle_count_ = HFSAT::GetCpucycleCountForTimeTick();
    order2_->ors_end_cycle_count_ = HFSAT::GetCpucycleCountForTimeTick();
    order3_->ors_end_cycle_count_ = HFSAT::GetCpucycleCountForTimeTick();

    if (write_length < (int32_t)send_spread_write_length) {
      DBGLOG_CLASS_FUNC_LINE_FATAL << "FAILD TO WRITE COMPLETE MESSAGE TO EXCHANGE SOCKET: WRITE_LENGTH: " << write_length << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
      exit(-1);
    }
}

void NSEEngine::SendTwoLegOrder(HFSAT::ORS::Order *order1_, HFSAT::ORS::Order *order2_) {
  std::cout <<"SendTwoLegOrder Empty below...."<< std::endl;
}

void NSEEngine::CancelOrder(ORS::Order *order) { CancelOrder(order, &container_.inst_desc_[order->security_id_]); }

void NSEEngine::ModifyOrder(ORS::Order *order, ORS::Order *orig_order) {
  if (!is_optimized_modify_supported_ || order->IsSizeModified()) {
    ModifyOrder(order, &container_.inst_desc_[order->security_id_], orig_order);
  } else {
    OptModifyOrder(order, container_.inst_desc_[order->security_id_].GetTwiddledToken(), orig_order);
  }
}

std::vector<int> NSEEngine::init() {
  vector<int> ss;
  //  ss.push_back(nse_session_socket_->socket_file_descriptor());
  return ss;
}

void NSEEngine::WaitForLoginSuccess() {
  int trails = 10;  // Max Wait time of 10 seconds
  while (!is_logged_in_ && trails) {
    sleep(1);
    trails--;
  }
  if (!is_logged_in_) {
    DBGLOG_CLASS_FUNC_LINE_INFO << " Could not Login till now." << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
  }
}

// Note: This function returns a pointer to a substring of the original string.
// If the given string was allocated dynamically, the caller must not overwrite
// that pointer with the returned value, since the original pointer must be
// deallocated using the same allocator with which it was allocated.  The return
// value must NOT be deallocated using free() etc.
char *NSEEngine::trimwhitespace(char *str) {
  char *end;

  // Trim leading space
  while (isspace((unsigned char)*str)) str++;

  if (*str == 0)  // All spaces?
    return str;

  // Trim trailing space
  end = str + strlen(str) - 1;
  while (end > str && isspace((unsigned char)*end)) end--;

  // Write new null terminator
  *(end + 1) = 0;

  return str;
}

void NSEEngine::printHexString(const char *c, int len) {
  int i;
  unsigned char buff[17];
  unsigned char *pc = (unsigned char *)c;

  if (len == 0) {
    printf("  ZERO LENGTH\n");
    return;
  }
  if (len < 0) {
    printf("  NEGATIVE LENGTH: %i\n", len);
    return;
  }

  // Process every byte in the data.
  for (i = 0; i < len; i++) {
    // Multiple of 16 means new line (with line offset).

    if ((i % 16) == 0) {
      // Just don't print ASCII for the zeroth line.
      if (i != 0) printf("  %s\n", buff);

      // Output the offset.
      printf("  %04x ", i);
    }

    // Now the hex code for the specific character.
    printf(" %02x", pc[i]);

    // And store a printable ASCII character for later.
    if ((pc[i] < 0x20) || (pc[i] > 0x7e))
      buff[i % 16] = '.';
    else
      buff[i % 16] = pc[i];
    buff[(i % 16) + 1] = '\0';
  }

  // Pad out last line if not exactly 16 characters.
  while ((i % 16) != 0) {
    printf("   ");
    i++;
  }

  // And print the final ASCII bit.
  printf("  %s\n", buff);

  printf("\n");
  fflush(stdout);
}

void NSEEngine::EmailPasswordChange(std::string alert_msg_) {
  // also send an alert
  char hostname[128];
  hostname[127] = '\0';
  gethostname(hostname, 127);

  std::string mode_str_ = "NSE";

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

void NSEEngine::EmailCtrlAlert(char* alert_msg_,int trader_id_) {
  // also send an alert
  char hostname[128];
  hostname[127] = '\0';
  gethostname(hostname, 127);

  std::string mode_str_ = "NSE";

  std::string alert_message = " CTRL_MSG_TO_TRADER " + std::to_string(trader_id_) + mode_str_ + " at " + std::string(hostname) + alert_msg_;

  HFSAT::Email e;

  struct timeval current_time;
  gettimeofday(&current_time, NULL);

  e.setSubject(mode_str_ + " -- CTRL_MSG_TO_TRADER");
  e.addRecepient("ravi.parikh@tworoads.co.in, nseall@tworoads.co.in");
  e.addSender("ravi.parikh@tworoads.co.in");
  e.content_stream << "exch: " << mode_str_ << "<br/>";
  e.content_stream << "host_machine: " << hostname << "<br/>";
  e.content_stream << "Message : " << alert_msg_ << " <br/>";
  e.content_stream << "Current Time: " << ctime(&(current_time.tv_sec)) << " <br/>";
  e.sendMail();
}

void NSEEngine::EmailGeneric(char* alert_msg_) {
  // also send an alert
  char hostname[128];
  hostname[127] = '\0';
  gethostname(hostname, 127);

  std::string mode_str_ = "NSE";

  std::string alert_message = " Alert" + std::string(hostname) + alert_msg_ + std::to_string(user_id_);

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

//"CONTROLCOMMAND" "GEN" "word1" "arg1" "arg2"
void NSEEngine::ProcessGeneralControlCommand(const char *input_stream, int stream_length) {
  dbglogger_ << "Received Control Command in NSE Engine : " << input_stream << "\n";
  dbglogger_.DumpCurrentBuffer();
  char temp_inp_str[stream_length];
  strncpy(temp_inp_str, input_stream, stream_length);

  try {
    PerishableStringTokenizer st_(temp_inp_str, stream_length);
    const std::vector<const char*>& tokens_ = st_.GetTokens();

    if ((tokens_.size() >= 2) && (strcmp(tokens_[0], "CONTROLCOMMAND") == 0)) {
      if (strcmp(tokens_[2], "SwitchToPostMarketSetting") == 0) {
        if (ValidatePostMarketTimings()) {
          is_mkt_order_ = true;
          DBGLOG_CLASS_FUNC << tokens_[0] << " " << tokens_[2] << " Enabled." << DBGLOG_ENDL_FLUSH;
        }
      } else if (strcmp(tokens_[2], "SwitchToNormalMarketSetting") == 0) {  // Regular Market
        is_mkt_order_ = false;
        is_pre_open_ = false;
      } else if (strcmp(tokens_[2], "BroadcastBatchCancelSAOS") == 0) { 
        if( tokens_.size() < 4 ){
          DBGLOG_CLASS_FUNC_LINE_ERROR << "NOT ENOUGH ARGS PROVIDED, EXPECTED 4 ARGUMENTS, PROVIDED :" << tokens_.size() << DBGLOG_ENDL_FLUSH ;
          DBGLOG_CLASS_FUNC_LINE_INFO << "USAGE : BroadcastBatchCancel SAOS " << DBGLOG_ENDL_FLUSH;
        }else{
          dbglogger_ << "ForceBatchCancelBroadcastSAOS: " << tokens_[3] << "\n";
          p_engine_listener_->ForceBatchCancelBroadcastSAOS(tokens_[3]);
        }
      } else if (strcmp(tokens_[2], "BroadcastBatchCancelSecurity") == 0) { 
        if( tokens_.size() < 4 ){
          DBGLOG_CLASS_FUNC_LINE_ERROR << "NOT ENOUGH ARGS PROVIDED, EXPECTED 4 ARGUMENTS, PROVIDED :" << tokens_.size() << DBGLOG_ENDL_FLUSH ;
          DBGLOG_CLASS_FUNC_LINE_INFO << "USAGE : BroadcastBatchCancel Shortcode" << DBGLOG_ENDL_FLUSH;
        }else{
          dbglogger_ << "ForceBatchCancelBroadcastSymbol: " << tokens_[3] << "\n";
          p_engine_listener_->ForceBatchCancelBroadcastSymbol(tokens_[3]);
        }
      } else if (strcmp(tokens_[2], "BroadcastBatchCancelAll") == 0) {
        dbglogger_ << "ForceBatchCancelBroadcastAll " << "\n";
        p_engine_listener_->ForceBatchCancelBroadcastAll();
      } else if (strcmp(tokens_[2], "BroadcastBatchCancelAllOrderRemoval") == 0) {
        DBGLOG_CLASS_FUNC_LINE_INFO << "NOT IMPLEMENTED YET" << DBGLOG_ENDL_FLUSH;
//        p_engine_listener_->ForceBatchCancelBroadcastAllOrderRemoval();
      } else if ( strcmp(tokens_[2], "RiskReductionEnable") == 0) {
        dbglogger_ << "EnableDisableOnlyIOC TRUE" << "\n";
        EnableDisableOnlyIOC(true);// ErrorCode: 16388 in CM if order is not at TOP	
      } else if ( strcmp(tokens_[2], "RiskReductionDisable") == 0) {
        dbglogger_ << "EnableDisableOnlyIOC FALSE" << "\n";
        EnableDisableOnlyIOC(false);
      } else if ( strcmp(tokens_[2], "BroadcastMsg") == 0) {
        if (tokens_.size() >= 4) {
           
           std::string msg;
           for (size_t i = 4; i < tokens_.size(); ++i) {
            if (i != 4) {
              msg += " "; 
            }
            msg += tokens_[i];
           }
           int16_t len = msg.length();
           char* buff = new char[len+10];
           char* ptr= buff;
           int32_t user_id_net= htonl(user_id_);
           memcpy(ptr, &user_id_net, sizeof(user_id_net));
           ptr += sizeof(user_id_net);
           memcpy(ptr, "MAR ", 4);
           ptr +=4;
           int16_t len_net = htons(len);
          memcpy(ptr, &len_net, sizeof(len_net));
          ptr += sizeof(len_net);
          memcpy(ptr, msg.c_str(), len);
           
        ProcessControlMessage(buff);
        }
       }
    }
  } catch (...) {
    dbglogger_ << "Exception in ProcessGeneralControlCommand(). But not Exiting."
               << "\n";
    dbglogger_.DumpCurrentBuffer();
  }
}

bool NSEEngine::ValidatePreOpenMarketTimings() {
  time_t t = time(0);

  //! Note the naming convention of the variables are as per NSE market timings,
  //! but the assigned time are in GMT.
  struct tm tm_curr = *localtime(&t);
  struct tm tm_9_10 = *localtime(&t);

  tm_9_10.tm_hour = 3;
  tm_9_10.tm_min = 40;
  tm_9_10.tm_sec = 0;

  time_t time_curr = mktime(&tm_curr);
  time_t time_9_10 = mktime(&tm_9_10);

  if (difftime(time_curr, time_9_10) <= 0) {
    return true;
  }
  return false;
}

bool NSEEngine::ValidatePostMarketTimings() {
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
